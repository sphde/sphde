/*
 * Copyright (c) 2005-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

//#define __SASDebugPrint__ 2
#define sas_printf printf
#include <stdlib.h>
#include <stdio.h>
#include "sasallocpriv.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sassimpleheap.h"
#include "sasindex.h"
#include "sasindexnode.h"
#include "sasindexpriv.h"
#include "sasindexnodepriv.h"


#ifdef __SASDebugPrint__
static void
SASIndexNodePrint (SASIndexNode_t header)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short i;

  sas_printf ("(");
  if (node->branch[0] != NULL)
    SASIndexNodePrint (node->branch[0]);
  for (i = 1; i <= node->count; i++)
    {
      if (i > 1)
	sas_printf (" ");
      if (node->keys[i])
        sas_printf ("%lx", node->keys[i]->data[0]);
      if (node->branch[i] != NULL)
	SASIndexNodePrint (node->branch[i]);
    }
  sas_printf (")");
}

static void
SASIndexNodePrintValue (SASIndexNode_t header)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short i;

  sas_printf ("(");
  if (node->branch[0] != NULL)
    SASIndexNodePrintValue (node->branch[0]);
  for (i = 1; i <= node->count; i++)
    {
      if (i > 1)
	sas_printf (" ");
      sas_printf ("%p", node->vals[i]);
      if (node->branch[i] != NULL)
	SASIndexNodePrintValue (node->branch[i]);
    }
  sas_printf (")");
}
#endif


SASIndexNode_t
SASIndexSpillInit (void *heap_seg, sas_type_t sasType, block_size_t heap_size)
{
  SASIndexNodeHeader *heapBlock = (SASIndexNodeHeader *) heap_seg;
  char *heapStart = NULL;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + heap_offset;
      initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType,
		       heap_size, heapStart);
    }
  heapBlock->count = 0;
  heapBlock->max_count = 0;

  heapBlock->keys = NULL;
  heapBlock->branch = NULL;
  heapBlock->vals = NULL;

  heapBlock->spill = NULL;
  heapBlock->spill2 = NULL;
  heapBlock->spill3 = NULL;

  return (SASIndexNode_t) heapBlock;
}

SASIndexNode_t
SASIndexNodeInit (void *heap_seg, sas_type_t sasType, block_size_t heap_size)
{
  SASIndexNodeHeader *heapBlock = (SASIndexNodeHeader *) heap_seg;
  char *heapStart = NULL;
  short i;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + heap_offset;
      initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType,
		       heap_size, heapStart);
    }
  heapBlock->count = 0;
  heapBlock->max_count = default_max;

  heapBlock->keys = (SASIndexKey_t **)
    SASIndexNodeAllocNoLock ((SASIndexNode_t) heapBlock,
			     (sizeof (void *) * (heapBlock->max_count + 1)));

  heapBlock->branch = (SASIndexNodeHeader **)
    SASIndexNodeAllocNoLock ((SASIndexNode_t) heapBlock,
			     (sizeof (void *) * (heapBlock->max_count + 1)));

  heapBlock->vals = (void **)
    SASIndexNodeAllocNoLock ((SASIndexNode_t) heapBlock,
			     (sizeof (void *) * (heapBlock->max_count + 1)));

  heapBlock->spill = NULL;
  heapBlock->spill2 = NULL;
  heapBlock->spill3 = NULL;

  for (i = 0; i < heapBlock->max_count; i++)
    {
      heapBlock->keys[i] = NULL;
      heapBlock->branch[i] = NULL;
      heapBlock->vals[i] = NULL;
    }

  return (SASIndexNode_t) heapBlock;
}

SASIndexNode_t
SASIndexNodeCreate (block_size_t heap_size)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      return SASIndexNodeInit (heapBlock, SAS_RUNTIME_INDEXNODE, heap_size);
    }
  else
    return NULL;
}

void *
SASIndexNodeAllocNoLock (SASIndexNode_t heap, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  freeNode *mem = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_SIMPLEHEAP))
    {
      heapSize = headerBlock->blockSize - heap_offset;
      if (alloc_size < heapSize)
	{
	  mem = freeNode_allocSpace (headerBlock->blockFreeSpace,
				     &headerBlock->blockFreeSpace,
				     alloc_size);
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASIndexNodeAlloc(%p, %zu) range check failed\n",
		      heap, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexNodeAlloc(%p, %zu) type check failed\n",
		  heap, alloc_size);
#endif
    }
  return mem;
}

void *
SASIndexNodeAlloc (SASIndexNode_t heap, block_size_t alloc_size,
			lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  void *mem = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_SIMPLEHEAP))
    {
	  if (lock_on) SASLock (heap, SasUserLock__WRITE);
      mem = SASIndexNodeAllocNoLock (heap, alloc_size);
      if (lock_on) SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexNodeAlloc(%p, %zu, %d) type check failed\n",
		  heap, alloc_size, lock_on);
#endif
    }
  return mem;
}

int
SASIndexNodeFreeNoLock (SASIndexNode_t heap,
			void *free_block, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  freeNode *free_node = (freeNode *) free_block;
  int rc;

  freeNode_init (free_node, alloc_size);

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      heapSize = headerBlock->blockSize - heap_offset;
      if ((alloc_size < heapSize)
	  && ((unsigned long) free_block >=
	      ((unsigned long) headerBlock + heap_offset)))
	{
	  freeNode_deallocSpace (free_node, &headerBlock->blockFreeSpace,
				 alloc_size);
	  rc = 0;
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf ("SASIndexNodeFree(%p, %p, %zu) range check failed\n",
		      heap, free_block, alloc_size);
#endif
	}
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeFree(%p, ...) does not match type/subtype\n",
		  heap);
#endif
    }
  return rc;
}

int
SASIndexNodeFree (SASIndexNode_t heap,
		  void *free_block, block_size_t alloc_size,
		  lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  int rc;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_INDEXNODE))
    {
	  if (lock_on) SASLock (heap, SasUserLock__WRITE);
      rc = SASIndexNodeFreeNoLock (heap, free_block, alloc_size);
      if (lock_on) SASUnlock (heap);
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeFree(%p,%d ...) does not match type/subtype\n",
		  heap, lock_on);
#endif
    }
  return rc;
}

int
SASIndexNodeIsSpill (SASIndexNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) heap;
  int result = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      result = node->max_count == 0;
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexNodeIsSpill(%p) does not match type/subtype\n",
		  heap);
#endif
    }
  return result;
}

/* The assumption is this is a free from the local heap of a IndexNode
 * that is already locked.  However it might be a non-local allocation
 * from a spill node.  So find the header near the free_block and compare it
 * to the address of the local node.  If they match, free_block must be
 * local to this node and we can use FreeNoLock. Otherwise it must be 
 * non-local and we need to lock the spill node before we free the block.  */
int
SASIndexNodeNearDealloc (SASIndexNode_t heap, void *free_block,
			 block_size_t alloc_size, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASBlockHeader *nearHeader = SASFindHeader (free_block);
  int rc;

  if (headerBlock == nearHeader)
    {				/* This is a local dealloc.  */
      rc = SASIndexNodeFreeNoLock (heap, free_block, alloc_size);
    }
  else
    {				/* This is non-local, i.e. a spill area.  */
      SASIndexNode_t newHeap;
      newHeap = SASIndexNodeVerify ((SASIndexNode_t) nearHeader);
      if (newHeap != NULL)
	{
      if (lock_on) SASLock (newHeap, SasUserLock__WRITE);
	  rc = SASIndexNodeFreeNoLock (newHeap, free_block, alloc_size);
	  if (lock_on) SASUnlock (newHeap);
	}
      else
	{
	  rc = -1;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SASIndexNodeNearDealloc(%p, %p, %zu, %d) not local/spill\n", heap,
	     free_block, alloc_size, lock_on);
#endif
	}
    }
  return rc;
}

void *
SASIndexNodeNearAlloc (void *nearObj, long allocSize, lock_on_t lock_on)
{
  void *result;
  result = SASNearAlloc (nearObj, allocSize);
  if (result == NULL)
    {
      SASBlockHeader *nearHeader = SASFindHeader (nearObj);
      SASIndexHeader *compoundHeader = NULL;
      SASIndexSpillList *spill_lst = NULL;
      SASIndexNode_t newHeap;

#if __SASDebugPrint__ > 1
      sas_printf ("NearAlloc@%p, size=%d far lock_on=%d\n", newHeap, allocSize, lock_on);
#endif
      newHeap = SASIndexNodeVerify ((SASIndexNode_t) nearHeader);
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeNearAlloc(%p, %zu, %d) failed\n",
		  newHeap, allocSize, lock_on);
      sas_printf (" freespace=%zu,  fragments=%zu,  max_free_block=%zu\n",
		  SASIndexNodeFreeSpaceNoLock (newHeap),
		  SASIndexNodeFreeFragmentsNoLock (newHeap),
		  SASIndexNodeMaxFragmentNoLock (newHeap));
#endif
      if (nearHeader != NULL)
	{
	  if (SOMSASCheckBlockSig (nearHeader))
	    {
	      if ((nearHeader->baseBlock != nearHeader)
		  && (nearHeader->baseBlock != NULL))
		{
		  compoundHeader = (SASIndexHeader *) nearHeader->baseBlock;
		}
	      spill_lst = SASIndexGetSpill (compoundHeader);
	      if (lock_on) SASLock (spill_lst, SasUserLock__WRITE);
	      if (spill_lst->count == 0)
		{
		  newHeap = SASIndexSpillAlloc (nearObj, lock_on);
		  if (newHeap != NULL)
		    {
		      result = SASIndexNodeAllocNoLock (newHeap, allocSize);
#ifdef __SASDebugPrint__
		      sas_printf
			("SASIndexNodeNearAlloc(%p, %zu, %d) added spill area@%p\n",
			 nearObj, allocSize, lock_on, newHeap);
#endif
		    }
		}

	      if (result == NULL)
		{
		  for (int i = 0; i < spill_lst->count; i++)
		    {
		      newHeap = (SASIndexNode_t) spill_lst->spillHeap[i];
		      result = SASIndexNodeAlloc (newHeap, allocSize, lock_on);
		      if (result != NULL)
			break;
		    }
		}

	      if (result == NULL)
		{
		  newHeap = SASIndexSpillAlloc (nearObj, lock_on);
		  if (newHeap != NULL)
		    {
		      result = SASIndexNodeAllocNoLock (newHeap, allocSize);
#ifdef __SASDebugPrint__
		      sas_printf
			("SASIndexNodeNearAlloc(%p, %zu, %d) added spill area@%p\n",
			 nearObj, allocSize, lock_on, newHeap);
#endif
		    }
		}
	      if (lock_on) SASUnlock (spill_lst);
	    }
	}
    }
  return result;
}

/*
void SASIndexNodeNearDealloc(void *memAddr, long allocSize)
{							
    SASNearDealloc (memAddr, allocSize);
}
*/
block_size_t
SASIndexNodeFreeFragmentsNoLock (SASIndexNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (headerBlock->blockFreeSpace != NULL)
	heapFree = freeNode_freeFragmentsTotal (headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASIndexNodeFreeFragmentNoLock(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return heapFree;
}

block_size_t
SASIndexNodeMaxFragmentNoLock (SASIndexNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (headerBlock->blockFreeSpace != NULL)
	heapFree = freeNode_maxFragment (headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASIndexNodeMaxFragmentNoLock(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return heapFree;
}

block_size_t
SASIndexNodeFreeSpaceNoLock (SASIndexNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (headerBlock->blockFreeSpace != NULL)
	heapFree = freeNode_freeSpaceTotal (headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexNodeFreeSpace(%p) does not match type/subtype\n",
		  heap);
#endif
    }
  return heapFree;
}

block_size_t
SASIndexNodeFreeSpace (SASIndexNode_t heap, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
	  if (lock_on) SASLock (heap, SasUserLock__WRITE);
      heapFree = SASIndexNodeFreeSpaceNoLock (heap);
      if (lock_on) SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexNodeFreeSpace(%p, %d) does not match type/subtype\n",
		  heap, lock_on);
#endif
    }
  return heapFree;
}

int
SASIndexNodeDestroyNoLock (SASIndexNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_INDEXNODE))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (heap, heapSize);
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeDestroy(%p) does not match type/subtype\n",
		  heap);
#endif
      rc = -1;
    }
  return rc;
}

int
SASIndexNodeDestroy (SASIndexNode_t heap, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_INDEXNODE))
    {
	  if (lock_on) SASLock (heap, SasUserLock__WRITE);
      rc = SASIndexNodeDestroyNoLock (heap);
      if (lock_on) SASUnlock (heap);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeDestroy(%p, %d) does not match type/subtype\n",
		  heap, lock_on);
#endif
      rc = -1;
    }
  return rc;
}

int
SASIndexNodeGetCount (SASIndexNode_t header)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  int result = -1;

  if (SOMSASCheckBlockSigAndTypeAndSubtype
      ((SASBlockHeader *) header, SAS_RUNTIME_INDEXNODE))
    {
      result = node->count;
    }
  return result;
}

SASIndexKey_t *
SASIndexNodeGetKeyIndexed (SASIndexNode_t header, short pos)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  SASIndexKey_t *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeGetKeyIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = node->keys[pos];
    }
  return result;
}

SASIndexNode_t
SASIndexNodeGetBranchIndexed (SASIndexNode_t header, short pos)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeGetBranchIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = (SASIndexNode_t) node->branch[pos];
    }
  return result;
}

void *
SASIndexNodeGetValIndexed (SASIndexNode_t header, short pos)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeGetValIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = node->vals[pos];
    }
  return result;
}

void *
SASIndexNodePutValIndexed (SASIndexNode_t header, short pos, void *val)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeGetValIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = node->vals[pos];
      node->vals[pos] = val;
    }
  return result;
}

// we must return
// a combined "found" and "position" result. If "found"
// result is >=0 and == "position". Otherwise result < 0 and
// "position == result + 256;
short
SASIndexNodeSearchNode (SASIndexNode_t header, SASIndexKey_t * target)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short position;
  int found;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASIndexNodeSearchNode(%p) does not match type/subtype\n",
		  header);
#endif
      return (short) -256;
    }

#ifdef __SASDebugPrint__
  sas_printf ("SearchNode target=%p keys[1]=%p\n", target, node->keys[1]);
#endif

  if (SASIndexKeyCompare (target, node->keys[1]) < 0)
    {
      position = 0;
      found = false;
    }
  else
    {
      if (node->count < 8)
	{
	  position = node->count;
	  while ((position > 1)
		 && (SASIndexKeyCompare (target, node->keys[position]) < 0))
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("SearchNode keys[%d]=%lx\n",
			  position, node->keys[position]->data[0]);
#endif
	      position--;
	    };
	  found = (SASIndexKeyCompare (target, node->keys[position]) == 0);
	}
      else
	{
	  short m = 8;

	  while (m <= node->count)
	    {
	      m = (short) (m + m);
	    };

	  position = (short) (m >> 1);
	  m = (short) (position >> 1);

	  while (m > 0)
	    {
	      if (position > node->count)
		{
#if __SASDebugPrint__ > 1
		  sas_printf ("SearchNode pos=%d, > count, m=%d\n",
			      position, m);
#endif
		  position -= m;
		}
	      else
		{
#if __SASDebugPrint__ > 1
		  sas_printf ("SearchNode keys[%d]=%lx m=%d\n",
			      position, node->keys[position]->data[0], m);
#endif
		  if (SASIndexKeyCompare (target, node->keys[position]) < 0)
		    {
		      position -= m;
		    }
		  else
		    {
		      if (SASIndexKeyCompare (target, node->keys[position]) >
			  0)
			{
			  position += m;
			}
		      else
			{	// must be ==
			  m = 0;
			};
		    };
		};
	      m >>= 1;
	    };

	  if (position <= node->count)
	    {
	      if (SASIndexKeyCompare (target, node->keys[position]) < 0)
		{		// target between positions; fix up
#ifdef __SASDebugPrint__
		  sas_printf ("SearchNode keys[%d]=%p fix up\n",
			      position, node->keys[position]);
#endif
		  position--;
		};
	    }
	  else
	    {
#ifdef __SASDebugPrint__
	      sas_printf ("SearchNode pos=%d count=%d should not happen\n",
			  position, node->count);
#endif
	      position = node->count;
	    }
	  found = (SASIndexKeyCompare (target, node->keys[position]) == 0);
	}
    }

  if (!found)
    {
      position = (short) (position - 256);
    }
  return position;
}

int
SASIndexNodeSearch (SASIndexNode_t header,
		    SASIndexKey_t * target, __IDXnodePosRef * ref)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeSearch(%p, %lx, %p) does not match type/subtype\n",
	 header, target->data[0], ref);
#endif
      return false;
    }

  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("Search target=%p pos=%d\n", target, pos);
#endif
  if (found)
    {
      ref->node = header;
      ref->pos = pos;
      result = true;
#ifdef __SASDebugPrint__
      sas_printf (" found vals[pos]=%p\n", node->vals[pos]);
#endif
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
#ifdef __SASDebugPrint__
	  sas_printf ("\n");
#endif
	  result = SASIndexNodeSearch (node->branch[pos], target, ref);
	}
    }
  return result;
}

int
SASIndexNodeSearchGT (SASIndexNode_t header,
		      SASIndexKey_t * target, __IDXnodePosRef * ref)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeSearchGT(%p, %lx, %p) does not match type/subtype\n",
	 header, target->data[0], ref);
#endif
      return false;
    }

  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("SearchGT target=%lx pos=%d\n", target->data[0], pos);
#endif
  if (found)
    {
      if (node->branch[pos] == NULL)
	{
	  if (pos < node->count)
	    {
	      ref->node = header;
	      ref->pos = pos + 1;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchGT found next =%lx pos=%d\n",
			  target->data[0], ref->pos);
#endif
	    }
	  else
	    {
	      result = false;
	    }
	}
      else
	{
	  result = SASIndexNodeSearchGT (node->branch[pos], target, ref);
	}
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result = SASIndexNodeSearchGT (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos < node->count)
		{
		  ref->node = header;
		  ref->pos = pos + 1;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchGT not found =%lx pos=%d\n",
			      target->data[0], ref->pos);
#endif
		}
	    }
	}
      else
	{
	  if (pos < node->count)
	    {
	      ref->node = header;
	      ref->pos = pos + 1;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchGT not found =%lx pos=%d\n", target->data[0],
			  ref->pos);
#endif
	    }
	}
    }
  return result;
}

int
SASIndexNodeSearchGE (SASIndexNode_t header,
		      SASIndexKey_t * target, __IDXnodePosRef * ref)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeSearchGE(%p, %lx, %p) does not match type/subtype\n",
	 header, target->data[0], ref);
#endif
      return false;
    }
#ifdef __SASDebugPrint__
  sas_printf
	("SASIndexNodeSearchGE(%p, %lx, %p)\n",
	 header, target->data[0], ref);
#endif

  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("SearchGE target=%lx pos=%d\n", target->data[0], pos);
#endif
  if (found)
    {
      ref->node = header;
      ref->pos = pos;
      result = found;
#ifdef __SASDebugPrint__
      sas_printf ("SearchGE found =%lx pos=%d\n", target->data[0],
		  ref->pos);
#endif
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result = SASIndexNodeSearchGE (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos < node->count)
			{
			  ref->node = header;
			  ref->pos = pos + 1;
			  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchGE@%p not found =%lx pos=%d\n",
				  node->branch[pos], target->data[0], ref->pos);
#endif
			}
	    }
	}
      else
	{
	  if (pos < node->count)
	    {
	      ref->node = header;
	      ref->pos = pos + 1;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchGE not found =%lx pos=%d\n", target->data[0],
			  ref->pos);
#endif
	    }
	}
    }
  return result;
}

int
SASIndexNodeSearchLT (SASIndexNode_t header,
		      SASIndexKey_t * target, __IDXnodePosRef * ref)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeSearchLT(%p, %lx, %p) does not match type/subtype\n",
	 header, target->data[0], ref);
#endif
      return false;
    }

  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("SearchLT target=%lx pos=%d\n", target->data[0], pos);
#endif
  if (found)
    {
      if (node->branch[pos - 1] == NULL)
	{
	  if (pos > 1)
	    {
	      ref->node = header;
	      ref->pos = pos - 1;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchLT found next =%lx pos=%d\n",
			  target->data[0], ref->pos);
#endif
	    }
	  else
	    {
	      result = false;
	    }
	}
      else
	{
	  result = SASIndexNodeSearchLT (node->branch[pos - 1], target, ref);
	}
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result = SASIndexNodeSearchLT (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos > 0)
		{
		  ref->node = header;
		  ref->pos = pos;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchLT not found =%lx pos=%d\n",
			      target->data[0], ref->pos);
#endif
		}
	    }
	}
      else
	{
	  if (pos > 0)
	    {
	      ref->node = header;
	      ref->pos = pos;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchLT not found =%lx pos=%d\n",
			  target->data[0], ref->pos);
#endif
	    }
	  else
	    {
	      result = false;
	    }
	}
    }
  return result;
}

int
SASIndexNodeSearchLE (SASIndexNode_t header,
		      SASIndexKey_t * target, __IDXnodePosRef * ref)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_INDEXNODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASIndexNodeSearchLE(%p, %lx, %p) does not match type/subtype\n",
	 header, target->data[0], ref);
#endif
      return false;
    }

  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("SearchLE target=%lx pos=%d\n", target->data[0], pos);
#endif
  if (found)
    {
      ref->node = header;
      ref->pos = pos;
      result = true;
#ifdef __SASDebugPrint__
      sas_printf ("SearchLE found next =%lx pos=%d\n",
		  target->data[0], ref->pos);
#endif
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result = SASIndexNodeSearchLE (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos > 0)
		{
		  ref->node = header;
		  ref->pos = pos;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchLE not found =%lx pos=%d\n",
			      target->data[0], ref->pos);
#endif
		}
	    }
	}
      else
	{
	  if (pos > 0)
	    {
	      ref->node = header;
	      ref->pos = pos;
	      result = true;
#ifdef __SASDebugPrint__
	      sas_printf ("SearchLE not found =%lx pos=%d\n",
			  target->data[0], ref->pos);
#endif
	    }
	}
    }
  return result;
}

static inline void
SASIndexNodeKeyMove (SASIndexNode_t heap, short pos,
		SASIndexKey_t * key, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) heap;
  SASIndexKey_t *oldkey = node->keys[pos];
  SASIndexKey_t *tempkey;
  size_t key_len = SASIndexKeySize (key);

  tempkey = (SASIndexKey_t *) SASIndexNodeNearAlloc (heap, key_len, lock_on);
  SASIndexKeyCopy (tempkey, key);
  node->keys[pos] = tempkey;

  if (oldkey != NULL)
    {
      int keylen = SASIndexKeySize (oldkey);
      SASIndexNodeNearDealloc (heap, oldkey, keylen, lock_on);
      node->keys[pos] = NULL;
    }

  if (key != NULL)
    {
      if ((unsigned long) key >= getfastMemLow ())
	{
	  if ((unsigned long) key <= getfastMemHigh ())
	    SASIndexNodeNearDealloc (heap, key, key_len, lock_on);
	}
    }
}

static inline void
SASIndexNodeKeyCopy (SASIndexNode_t heap, short pos,
		SASIndexKey_t * key, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) heap;
  SASIndexKey_t *oldkey = node->keys[pos];
  SASIndexKey_t *tempkey;
  size_t key_len = SASIndexKeySize (key);

  tempkey = (SASIndexKey_t *) SASIndexNodeNearAlloc (heap, key_len, lock_on);
  SASIndexKeyCopy (tempkey, key);
  node->keys[pos] = tempkey;

  if (oldkey != NULL)
    {
      int keylen = SASIndexKeySize (oldkey);
      SASIndexNodeNearDealloc (heap, oldkey, keylen, lock_on);
    }
}

static inline void
SASIndexNodeKeyDelete (SASIndexNode_t heap, short pos,
		lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) heap;
  SASIndexKey_t *oldkey = node->keys[pos];
  size_t keylen;

  if (oldkey != NULL)
    {
      keylen = SASIndexKeySize (oldkey);
      SASIndexNodeNearDealloc (heap, oldkey, keylen, lock_on);
    }
}

static inline void
SASIndexNodeKeyReplace (SASIndexNode_t heap, short pos,
		SASIndexKey_t * key, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) heap;
  SASIndexKey_t *tempkey;
  size_t key_len = SASIndexKeySize (key);

  tempkey = (SASIndexKey_t *) SASIndexNodeNearAlloc (heap, key_len, lock_on);
  SASIndexKeyCopy (tempkey, key);
  node->keys[pos] = tempkey;
}

void
SASIndexNodePushIn (SASIndexNode_t node_t, __IDXnodeKeyRef * ref,
		short k, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  char *str_ptr;
  char *end_ptr;
  SASIndexKey_t *temp_key;
  size_t key_len, max_frag;
  short i;
#ifdef __SASDebugPrint__
  sas_printf ("PushIn@%p ref->key@%p=%lx k=%hd lock_on=%d\n",
		  node, ref->key, ref->key->data[0], k, lock_on);
#endif
  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
  for (i = node->count; i >= (k + 1); i--)
    {
      node->keys[i + 1] = node->keys[i];
      node->vals[i + 1] = node->vals[i];
      node->branch[i + 1] = node->branch[i];
      temp_key = node->keys[i + 1];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = SASIndexKeySize (temp_key);
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("PushIn@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i + 1), temp_key, key_len);
#endif
	      SASIndexNodeKeyCopy ((SASIndexNode_t) node, (i + 1), temp_key, lock_on);
	      max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
	    }
	}
    }

  SASIndexNodeKeyReplace (node, (k + 1), ref->key, lock_on);
  node->vals[k + 1] = ref->val;
  node->branch[k + 1] = (SASIndexNodeHeader *) ref->node;
  node->count++;
}

void
SASIndexNodeSplit (SASIndexNode_t node_t,
		   __IDXnodeKeyRef * xref, short k, __IDXnodeKeyRef * yref,
		   lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  long i, median;
  long min = node->max_count / 2;
  SASIndexKey_t **thisKeys = node->keys;
  SASIndexNodeHeader **thisBranch = node->branch;
  void **thisVals = node->vals;
  SASIndexNodeHeader *yr;	// temp in case xref & yref are same
  char *str_ptr;
  char *end_ptr;
  SASIndexKey_t *temp_key;
  size_t key_len, max_frag;

  if (k <= min)
    {
      median = min;
    }
  else
    {
      median = (short) (min + 1);
    }
#ifdef __SASDebugPrint__
  sas_printf ("Split@%p x=%p k=%hd median=%ld lock_on=%d\n", node, xref->key, k, median, lock_on);
#endif

  if (lock_on == LOCK_ON)
    yr = (SASIndexNodeHeader *) SASIndexNearAlloc (node_t);
  else
    yr = (SASIndexNodeHeader *) SASIndexNearAllocNoLock (node_t);

  SASIndexNodeHeader **yrBranch = yr->branch;
  void **yrVals = yr->vals;

  for (i = (median + 1); i <= node->max_count; i++)
    {
#if __SASDebugPrint__ > 1
	  sas_printf ("Split@%p copy pos=%hd key%lx to=%hd\n",
	    		  yr, i, thisKeys[i]->data[0], (i - median));
#endif

      SASIndexNodeKeyMove (yr, (i - median), thisKeys[i], lock_on);
      yrVals[i - median] = thisVals[i];
      yrBranch[i - median] = thisBranch[i];
    }
  yr->count = (short) (node->max_count - median);
  node->count = median;

  if (k <= min)
    {
      SASIndexNodePushIn (node_t, xref, k, lock_on);
    }
  else
    {
      SASIndexNodePushIn (yr, xref, (short) (k - median), lock_on);
    }

  yrBranch[0] = (SASIndexNodeHeader *) thisBranch[node->count];
  yref->node = yr;
  yref->key = thisKeys[node->count];
  yref->val = thisVals[node->count];
  node->count--;

  // clear out end of old node
  for (i = (short) (node->count + 1); i <= node->max_count; i++)
    {
      thisKeys[i] = NULL;
      thisVals[i] = NULL;
      thisBranch[i] = NULL;
    }

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
  for (i = 1; i <= (node->count); i++)
    {
      temp_key = node->keys[i];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = SASIndexKeySize (temp_key);
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("Split@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i + 1), temp_key, key_len);
#endif
	      SASIndexNodeKeyCopy ((SASIndexNode_t) node, i, temp_key, lock_on);
	      max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
	    }
	}
    }
}

int
SASIndexNodePushDown (SASIndexNode_t node_t,
		      SASIndexKey_t * newkey, void *newval,
		      __IDXnodeKeyRef * ref, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  short pos;			// AKA k
  int found;
  int pushup = false;

#ifdef __SASDebugPrint__
  sas_printf ("PushDown@%p newkey=%lx lock_on=%d\n", node, newkey->data[0], lock_on);
#endif
  pos = SASIndexNodeSearchNode (node_t, newkey);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    };

#ifdef __SASDebugPrint__
  sas_printf ("PushDown@%p  pos=%hd found=%d\n", node, pos, found);
#endif
  if (found)
    {
#ifdef __SASDebugPrint__
      sas_printf ("Duplicate key = <%p>\n", newkey);
#endif
      ref->dupKey = true;
      return false;
      //DupKeyException e = new DupKeyException(new Long(newkey));
      //throw e;
    };

  if (node->branch[pos] != NULL)
    {
      pushup = SASIndexNodePushDown (node->branch[pos], newkey, newval,
    		  ref, lock_on);
    }
  else
    {
      pushup = true;
      ref->key = newkey;
      ref->val = newval;
      ref->node = NULL;
    }

#ifdef __SASDebugPrint__
  sas_printf ("PushDown pushup=%d  ref.key=%lx, lock_on=%d\n", pushup, ref->key->data[0], lock_on);
#endif
  if (pushup)
    {
#ifdef __SASDebugPrint__
      sas_printf ("PushDown count=%hd\n", node->count);
#endif
      if (node->count < node->max_count)
	{
	  pushup = false;
	  SASIndexNodePushIn (node_t, ref, pos, lock_on);
	}
      else
	{
	  pushup = true;
	  SASIndexNodeSplit (node_t, ref, pos, ref, lock_on);
	}
    }
  return pushup;
}

SASIndexNode_t
SASIndexNodeInitialize (SASIndexNode_t node_t,
			SASIndexKey_t * newkey, void *newval, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  SASIndexNode_t result = node_t;

#ifdef __SASDebugPrint__
  sas_printf ("Initialize@%p newkey=%lx, newval=%p lock_on=%d\n",
	      node, newkey->data[0], newval, lock_on);
#endif
  node->count = 1;
  SASIndexNodeKeyCopy (node, 1, newkey, lock_on);
  node->vals[1] = newval;
  node->branch[1] = NULL;
  node->branch[0] = NULL;

  return result;
}

SASIndexNode_t
SASIndexNodeInsert (SASIndexNode_t node_t,
		    SASIndexKey_t * newkey, void *newval, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  int pushup;
  __IDXnodeKeyRef xref = { NULL, NULL, NULL, false };
  SASIndexNode_t result = node_t;

#ifdef __SASDebugPrint__
  sas_printf ("Insert@%p newkey=%lx lock_on=%d\n", node, newkey->data[0], lock_on);
#endif
  pushup = SASIndexNodePushDown (node_t, newkey, newval, &xref, lock_on);
  if (pushup)
    {
      SASIndexNodeHeader *new_node;


      if (lock_on == LOCK_ON)
        result = SASIndexNearAlloc (node_t);
      else
        result = SASIndexNearAllocNoLock (node_t);

      new_node = (SASIndexNodeHeader *) result;
      new_node->count = 1;
#if __SASDebugPrint__ > 1
	  sas_printf ("Insert:pushup@%p key@%p key=%lx val=%p to=1\n",
			  new_node, xref.key, xref.val, xref.key->data[0]);
#endif
      SASIndexNodeKeyMove (new_node, 1, xref.key, lock_on);
      new_node->vals[1] = xref.val;
      new_node->branch[1] = (SASIndexNodeHeader *) xref.node;
      new_node->branch[0] = node;
#if __SASDebugPrint__ > 1
	  sas_printf ("Insert:top@%p left@%p right=%p val=%p\n",
			  result, new_node->branch[0], new_node->branch[1], new_node->vals[1]);
#endif
    }
  else
    {
      if (xref.dupKey)
	  result = NULL;		/* we have a dup key error */
    }

  return result;
}

/* removes key[k] & branch[k] from this node */
void
SASIndexNodeRemove (SASIndexNode_t node_t, short pos,
		lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  SASIndexNodeHeader *q;
  char *str_ptr;
  char *end_ptr;
  SASIndexKey_t *temp_key;
  size_t key_len, max_frag;
  short i;

  q = ((SASIndexNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("Remove@%p pos=%hd branch=%p lock_on=%d\n", node_t, pos, q, lock_on);
  if (node->keys[pos])
    sas_printf ("key[%hd]=%lx\n", pos, node->keys[pos]->data[0]);
#endif
  SASIndexNodeKeyDelete (node_t, pos, lock_on);

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
  for (i = (short) (pos + 1); i <= node->count; i++)
    {
      node->keys[i - 1] = node->keys[i];
      node->vals[i - 1] = node->vals[i];
      node->branch[i - 1] = node->branch[i];
#if  __SASDebugPrint__ > 1
      sas_printf ("Remove copy key[%hd]=%lx to %d\n",
		  i, node->keys[i]->data[0], (i - 1));
#endif
      temp_key = node->keys[i - 1];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = SASIndexKeySize (temp_key);
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("Remove@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i - 1), temp_key, key_len);
#endif
	      SASIndexNodeKeyCopy ((SASIndexNode_t) node, (i - 1),
	    		  temp_key, lock_on);
	      max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
	    }
	}
    }
  node->keys[node->count] = NULL;
  node->vals[node->count] = NULL;
  node->branch[node->count] = NULL;
  node->count--;
#ifdef __SASDebugPrint__
  sas_printf ("Remove count=%hd\n", node->count);
#endif

  if (q != NULL)
    {				// dispose of q, since it is no longer accessible 
#ifdef __SASDebugPrint__
      sas_printf ("Remove: delete branch for pos=%hd @%p\n", pos, q);
#endif
      SASIndexNearDealloc (q);
    }
#if __SASDebugPrint__ > 1
  sas_printf ("Remove: subtree=");
  SASIndexNodePrint (node_t);
  sas_printf ("\n");
#endif
}

void
SASIndexNodeCombine (SASIndexNode_t node_t, short pos,
		lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  SASIndexNodeHeader *q, *r;
  short c;

#ifdef __SASDebugPrint__
  sas_printf ("Combine@%p pos=%hd lock_on=%d\n", node_t, pos, lock_on);
#endif

  q = ((SASIndexNodeHeader *) node->branch[pos]);
  r = ((SASIndexNodeHeader *) node->branch[pos - 1]);
  r->count++;
  //r->keys[r->count] = node->keys[pos];
  SASIndexNodeKeyMove (r, r->count, node->keys[pos], lock_on);
  node->keys[pos] = NULL;	// Move frees the key string in node
  r->vals[r->count] = node->vals[pos];
  r->branch[r->count] = q->branch[0];
  q->branch[0] = NULL;

#ifdef __SASDebugPrint__
  sas_printf ("Combine move=%p\n", node->keys[pos]);
#endif
  for (c = 1; c <= q->count; c++)
    {
      r->count++;
      // r->keys[r->count] = q->keys[c];
      SASIndexNodeKeyMove (r, r->count, q->keys[c], lock_on);
      q->keys[c] = NULL;
      r->vals[r->count] = q->vals[c];
      q->vals[c] = NULL;
      r->branch[r->count] = q->branch[c];
      q->branch[c] = NULL;
#if __SASDebugPrint__ > 1
      sas_printf ("Combine copy=%s\n", r->keys[r->count]);
#endif
    }
// remove pivot, since it hase been combined
  node->branch[pos] = NULL;
  node->vals[pos] = NULL;
  SASIndexNodeRemove (node_t, pos, lock_on);
// dispose of q, since it is empty
  SASIndexNearDealloc (q);
}

void
SASIndexNodeMoveLeft (SASIndexNode_t node_t, short pos, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  SASIndexNodeHeader *l, *r;
  char *str_ptr;
  char *end_ptr;
  SASIndexKey_t *temp_key;
  size_t key_len, max_frag;
  short c;

  l = ((SASIndexNodeHeader *) node->branch[pos - 1]);
  r = ((SASIndexNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("MoveLeft@%p pos=%hd left@%p right@%p lock_on=%d\n", node_t, pos, l, r, lock_on);
#endif
  l->count++;
  // l->keys[l->count] = node->keys[pos];
  SASIndexNodeKeyMove (l, l->count, node->keys[pos], lock_on);
  node->keys[pos] = NULL;
  l->vals[l->count] = node->vals[pos];
  l->branch[l->count] = r->branch[0];

  //node->keys[pos] = r->keys[1];
  SASIndexNodeKeyMove (node, pos, r->keys[1], lock_on);
  node->vals[pos] = r->vals[1];
  r->branch[0] = r->branch[1];
  r->count--;

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
  for (c = 1; c <= r->count; c++)
    {
      r->keys[c] = r->keys[c + 1];
      r->vals[c] = r->vals[c + 1];
      r->branch[c] = r->branch[c + 1];
      temp_key = r->keys[c];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = SASIndexKeySize (temp_key);
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("MoveLeft@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (c), temp_key, key_len);
#endif
	      SASIndexNodeKeyCopy ((SASIndexNode_t) r, (c), temp_key, lock_on);
	      max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
	    }
	}
    }
  r->keys[r->count + 1] = NULL;
  r->vals[r->count + 1] = NULL;
  r->branch[r->count + 1] = NULL;
#if  __SASDebugPrint__ > 1
  sas_printf ("MoveLeft: subtree=");
  SASIndexNodePrint (node_t);
  sas_printf ("\n");
#endif
}

void
SASIndexNodeMoveRight (SASIndexNode_t node_t, short pos,
		lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) node_t;
  SASIndexNodeHeader *l, *r;
  char *str_ptr;
  char *end_ptr;
  SASIndexKey_t *temp_key;
  size_t key_len, max_frag;
  short c;

  l = ((SASIndexNodeHeader *) node->branch[pos - 1]);
  r = ((SASIndexNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("MoveRight@%p pos=%hd left@%p right@%p lock_on=%d\n", node_t, pos, l, r, lock_on);
#endif

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
  for (c = r->count; c >= 1; c--)
    {
      r->keys[c + 1] = r->keys[c];
      r->vals[c + 1] = r->vals[c];
      r->branch[c + 1] = r->branch[c];
      temp_key = r->keys[c + 1];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = SASIndexKeySize (temp_key);
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("MoveRight@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (c + 1), temp_key, key_len);
#endif
	      SASIndexNodeKeyCopy ((SASIndexNode_t) r, (c + 1),
	    		  temp_key, lock_on);
	      max_frag = SASIndexNodeMaxFragmentNoLock (node_t);
	    }
	}
    };
  // r->keys[1] = node->keys[pos];
  r->keys[1] = NULL;
  SASIndexNodeKeyMove (r, 1, node->keys[pos], lock_on);
  node->keys[pos] = NULL;
  r->vals[1] = node->vals[pos];
  r->branch[1] = r->branch[0];
  r->count++;
#if  __SASDebugPrint__ > 1
  sas_printf ("MoveRight: right subtree=");
  SASIndexNodePrint (node_t);
  sas_printf ("\n");
#endif

  // node->keys[pos] = l->keys[l->count];
  SASIndexNodeKeyMove (node, pos, l->keys[l->count], lock_on);
  node->vals[pos] = l->vals[l->count];
  r->branch[0] = l->branch[l->count];

  l->keys[l->count] = NULL;
  l->vals[l->count] = NULL;
  l->branch[l->count] = NULL;
  l->count--;
#if __SASDebugPrint__ > 1
  sas_printf ("MoveRight: subtree=");
  SASIndexNodePrint (node_t);
  sas_printf ("\n");
#endif
}

 // finds a key and inserts it into this.branch[pos] to restore minimums
void
SASIndexNodeRestore (SASIndexNode_t header, short pos, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  short min = node->max_count / 2;

#ifdef __SASDebugPrint__
  sas_printf ("Restore@%p pos=%hd lock_on=%d\n", header, pos, lock_on);
#endif
  if (pos > 0)
    {
      if (((SASIndexNodeHeader *) node->branch[pos - 1])->count > min)
	{			// move key to right
	  SASIndexNodeMoveRight (header, pos, lock_on);
	}
      else
	{
	  if (((SASIndexNodeHeader *) node->branch[pos])->count > min)
	    {			// move key to left
#ifdef __SASDebugPrint__
	      sas_printf ("Restore branch[pos]->count=%hd\n",
			  ((SASIndexNodeHeader *) node->branch[pos])->count);
#endif
	      SASIndexNodeMoveLeft (header, pos, lock_on);
	    }
	  else
	    {
	      if (node->count > pos)
		{
#ifdef __SASDebugPrint__
		  sas_printf ("Restore branch[pos+1]->count=%hd\n",
			      ((SASIndexNodeHeader *) node->branch[pos + 1])->
			      count);
#endif
		  if (((SASIndexNodeHeader *) node->branch[pos + 1])->count >
		      min)
		    {		// move key to left
		      SASIndexNodeMoveLeft (header, (short) (pos + 1), lock_on);
		    }
		  else
		    {
		      SASIndexNodeCombine (header, pos, lock_on);
		    }
		}
	      else
		{
		  SASIndexNodeCombine (header, pos, lock_on);
		}
	    }
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("Restore branch[1]->count=%hd\n",
		  ((SASIndexNodeHeader *) node->branch[1])->count);
#endif
      if (((SASIndexNodeHeader *) node->branch[1])->count > min)
	{
	  SASIndexNodeMoveLeft (header, (short) 1, lock_on);
	}
      else
	{
	  SASIndexNodeCombine (header, (short) 1, lock_on);
	}
    }
}

// replaces this.key[k] by its immediate successor
void
SASIndexNodeSuccessor (SASIndexNode_t header, short pos, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  SASIndexNodeHeader *q;

#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p pos=%hd lock_on=%d\n", header, pos, lock_on);
#endif
  q = ((SASIndexNodeHeader *) node->branch[pos]);
  while (q->branch[0] != NULL)
    {
      q = ((SASIndexNodeHeader *) q->branch[0]);
#ifdef __SASDebugPrint__
      sas_printf ("Successor@%p key=%lx\n", q, q->keys[1]->data[0]);
#endif
    }
#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p replace key=%p with key=%p\n ",
	      header, node->keys[pos], q->keys[1]);
#endif
  // node->keys[k] = q->keys[1];
  SASIndexNodeKeyCopy (node, pos, q->keys[1], lock_on);
//      q->keys[1] = NULL;
  node->vals[pos] = q->vals[1];
//      q->vals[1] = NULL;
#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p key[%hd]=%lx @%p\n",
	      header, pos, node->keys[pos]->data[0], node->keys[pos]);
  sas_printf ("Successor: subtree=");
  SASIndexNodePrint (header);
  sas_printf ("\n");
#endif
}

// recursive delete
int
SASIndexNodeRecDelete (SASIndexNode_t header, SASIndexKey_t * target,
		lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  SASIndexNodeHeader *q;
  int found = false;
  short min = node->max_count / 2;
  short k, pos;

#ifdef __SASDebugPrint__
  sas_printf ("RecDelete@%p target=%ld lock_on=%d\n", header, target->data[0], lock_on);
#endif
  pos = SASIndexNodeSearchNode (header, target);
  if (pos < 0)
    {
      pos = (short) (pos + (short) 256);
      found = false;
    }
  else
    {
      found = true;
    }
  k = pos;
#ifdef __SASDebugPrint__
  sas_printf ("RecDelete pos=%hd found=%d\n", pos, found);
#endif
  if (found)
    {
      if (node->branch[k - 1] == NULL)	// case: this is a leaf
	{			// removes key from position k of this
	  SASIndexNodeRemove (header, k, lock_on);
	}
      else
	{			// replaces key[k] by its successor
	  SASIndexNodeSuccessor (header, k, lock_on);
	  q = ((SASIndexNodeHeader *) node->branch[k]);
	  if (q != NULL)
	    {
	      found = SASIndexNodeRecDelete (q, node->keys[k], lock_on);
#if __SASDebugPrint__ > 1
	      sas_printf ("RecDelete after Successor found=%d\n", found);
	      sas_printf ("RecDelete: subtree=");
	      SASIndexNodePrint (header);
	      sas_printf ("\n");
#endif
	    }
	  else
	    {
	      found = false;
	    }
	  if (!found)
	    {
	      sas_printf
		("RecDelete Error->RecDelete: key should have been there!\n");
	    }
	}
    }
  else
    {				// target not found in this node
      q = ((SASIndexNodeHeader *) node->branch[k]);
      if (q != NULL)
	{
	  found = SASIndexNodeRecDelete (q, target, lock_on);
	}
      else
	{
	  found = false;
	}
    }

  if (node->branch[k] != NULL)
    {
      if (((SASIndexNodeHeader *) node->branch[k])->count < min)
	{
	  SASIndexNodeRestore (header, k, lock_on);
	}
    }
#ifdef __SASDebugPrint__
  sas_printf ("RecDelete return=%d\n", found);
#if __SASDebugPrint__ > 1
  sas_printf ("RecDelete: subtree=");
  SASIndexNodePrint (header);
  sas_printf ("\n");
#endif
#endif
  return found;
}

SASIndexNode_t
SASIndexNodeDelete (SASIndexNode_t header, SASIndexKey_t * target, lock_on_t lock_on)
{
  SASIndexNodeHeader *node = (SASIndexNodeHeader *) header;
  SASIndexNodeHeader *result = node;
  int found = false;

#ifdef __SASDebugPrint__
  sas_printf ("Delete target=%lx lock_on=%d\n", target->data[0], lock_on);
#endif
  found = SASIndexNodeRecDelete (header, target, lock_on);
  if (found)
    {
      if (node->count == 0)
	{
	  result = ((SASIndexNodeHeader *) node->branch[0]);
	  // caller should delete previous root;
	  node->branch[0] = NULL;
	};
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("Delete : delete target not found");
#endif
    }

  return result;
}
