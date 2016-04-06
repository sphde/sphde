/*
 * Copyright (c) 2004-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

//#define __SASDebugPrint__ 1
#define sas_printf printf
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sasallocpriv.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sassimpleheap.h"
#include "sasstringbtree.h"
#include "sasstringbtreenode.h"
#include "sasstringbtreepriv.h"
#include "sasstringbtreenodepriv.h"

#ifdef __SASDebugPrint__
static void
SASStringBTreeNodePrint (SASStringBTreeNode_t header)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short i;

  sas_printf ("(");
  if (node->branch[0] != NULL)
    SASStringBTreeNodePrint (node->branch[0]);
  for (i = 1; i <= node->count; i++)
    {
      if (i > 1)
	sas_printf (" ");
      sas_printf ("%s", node->keys[i]);
      if (node->branch[i] != NULL)
	SASStringBTreeNodePrint (node->branch[i]);
    }
  sas_printf (")");
}

static void
SASStringBTreeNodePrintValue (SASStringBTreeNode_t header)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short i;

  sas_printf ("(");
  if (node->branch[0] != NULL)
    SASStringBTreeNodePrintValue (node->branch[0]);
  for (i = 1; i <= node->count; i++)
    {
      if (i > 1)
	sas_printf (" ");
      sas_printf ("%p", node->vals[i]);
      if (node->branch[i] != NULL)
	SASStringBTreeNodePrintValue (node->branch[i]);
    }
  sas_printf (")");
}
#endif // __SASDebugPrint__ > 1


struct __SBTnodeKeyRef
{
  SASStringBTreeNode_t node;
  char *key;
  void *val;
  int dupKey;
};

SASStringBTreeNode_t
SASStringBTreeSpillInit (void *heap_seg, sas_type_t sasType,
			 block_size_t heap_size)
{
  SASStringBTreeNodeHeader *heapBlock = (SASStringBTreeNodeHeader *) heap_seg;
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

  return (SASStringBTreeNode_t) heapBlock;
}

SASStringBTreeNode_t
SASStringBTreeNodeInit (void *heap_seg, sas_type_t sasType,
			block_size_t heap_size)
{
  SASStringBTreeNodeHeader *heapBlock = (SASStringBTreeNodeHeader *) heap_seg;
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

  heapBlock->keys = (char **)
    SASStringBTreeNodeAllocNoLock ((SASStringBTreeNode_t) heapBlock,
				   (sizeof (void *) *
				    (heapBlock->max_count + 1)));

  heapBlock->branch = (SASStringBTreeNodeHeader **)
    SASStringBTreeNodeAllocNoLock ((SASStringBTreeNode_t) heapBlock,
				   (sizeof (void *) *
				    (heapBlock->max_count + 1)));

  heapBlock->vals = (void **)
    SASStringBTreeNodeAllocNoLock ((SASStringBTreeNode_t) heapBlock,
				   (sizeof (void *) *
				    (heapBlock->max_count + 1)));

  heapBlock->spill = NULL;
  heapBlock->spill2 = NULL;
  heapBlock->spill3 = NULL;

  for (i = 0; i < heapBlock->max_count; i++)
    {
      heapBlock->keys[i] = NULL;
      heapBlock->branch[i] = NULL;
      heapBlock->vals[i] = NULL;
    }

  return (SASStringBTreeNode_t) heapBlock;
}

SASStringBTreeNode_t
SASStringBTreeNodeCreate (block_size_t heap_size)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      return SASStringBTreeNodeInit (heapBlock,
				     SAS_RUNTIME_STRINGBTREENODE, heap_size);
    }
  else
    return NULL;
}

void *
SASStringBTreeNodeAllocNoLock (SASStringBTreeNode_t heap,
			       block_size_t alloc_size)
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
	  sas_printf ("SASStringBTreeNodeAlloc(%p, %zu) range check failed\n",
		      heap, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeNodeAlloc(%p, %zu) type check failed\n",
		  heap, alloc_size);
#endif
    }
  return mem;
}

void *
SASStringBTreeNodeAlloc (SASStringBTreeNode_t heap, block_size_t alloc_size,
			lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  void *mem = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_SIMPLEHEAP))
    {
      if (lock_on) SASLock (heap, SasUserLock__WRITE);
      mem = SASStringBTreeNodeAllocNoLock (heap, alloc_size);
      if (lock_on) SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeNodeAlloc(%p, %zu, %d) type check failed\n",
		  heap, alloc_size, lock_on);
#endif
    }
  return mem;
}

int
SASStringBTreeNodeFreeNoLock (SASStringBTreeNode_t heap,
			      void *free_block, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      heapSize = headerBlock->blockSize - heap_offset;
      if ((alloc_size < heapSize)
	  && ((unsigned long) free_block >=
	      ((unsigned long) headerBlock + heap_offset)))
	{
	  freeNode_deallocSpace ((FreeNode *) free_block,
				 &headerBlock->blockFreeSpace, alloc_size);
	  rc = 0;
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SASStringBTreeNodeFree(%p, %p, %zu) range check failed\n", heap,
	     free_block, alloc_size);
#endif
	}
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeFree(%p, ...) does not match type/subtype\n",
	 heap);
#endif
    }
  return rc;
}

int
SASStringBTreeNodeFree (SASStringBTreeNode_t heap,
			void *free_block, block_size_t alloc_size,
			lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  int rc;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_STRINGBTREENODE))
    {
      if (lock_on) SASLock (heap, SasUserLock__WRITE);
      rc = SASStringBTreeNodeFreeNoLock (heap, free_block, alloc_size);
      if (lock_on) SASUnlock (heap);
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeFree(%p,%d, ...) does not match type/subtype\n",
	 heap, lock_on);
#endif
    }
  return rc;
}

int
SASStringBTreeNodeIsSpill (SASStringBTreeNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  int result = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      result = node->max_count == 0;
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASStringBTreeNodeIsSpill(%p) does not match type/subtype\n", heap);
#endif
    }
  return result;
}

/* The assumption is this is a free from the local heap of a StringBTreeNode
 * that is already locked.  However it might be a non-local allocation
 * from a spill node.  So find the header near the free_block and compare it
 * to the address of the local node.  If they match, free_block must be
 * local to this node and we can use FreeNoLock. Otherwise it must be 
 * non-local and we need to lock the spill node before we free the block.  */
int
SASStringBTreeNodeNearDealloc (SASStringBTreeNode_t heap, void *free_block,
			       block_size_t alloc_size, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASBlockHeader *nearHeader = SASFindHeader (free_block);
  int rc;

  if (headerBlock == nearHeader)
    {				/* This is a local dealloc.  */
      rc = SASStringBTreeNodeFreeNoLock (heap, free_block, alloc_size);
    }
  else
    {				/* This is non-local, i.e. a spill area.  */
      SASStringBTreeNode_t newHeap;
      newHeap = SASStringBTreeNodeVerify ((SASStringBTreeNode_t) nearHeader);
      if (newHeap != NULL)
	{
	  if (lock_on) SASLock (newHeap, SasUserLock__WRITE);
	  rc = SASStringBTreeNodeFreeNoLock (newHeap, free_block, alloc_size);
	  if (lock_on) SASUnlock (newHeap);
	}
      else
	{
	  rc = -1;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SASStringBTreeNodeNearDealloc(%p, %p, %zu, %d) not local/spill\n",
	     heap, free_block, alloc_size, lock_on);
#endif
	}
    }
  return rc;
}

void *
SASStringBTreeNodeNearAlloc (void *nearObj, long allocSize, lock_on_t lock_on)
{
  void *result;
  result = SASNearAlloc (nearObj, allocSize);
  if (result == NULL)
    {
      SASBlockHeader *nearHeader = SASFindHeader (nearObj);
      SASStringBTreeHeader *compoundHeader = NULL;
      SASStringBTreeSpillList *spill_lst = NULL;
      SASStringBTreeNode_t newHeap;

#if __SASDebugPrint__ > 1
      sas_printf ("NearAlloc@%p, size=%d far lock_on=%d\n", newHeap, allocSize, lock_on);
#endif
      newHeap = SASStringBTreeNodeVerify ((SASStringBTreeNode_t) nearHeader);
#ifdef __SASDebugPrint__
      sas_printf ("SASStringBTreeNodeNearAlloc(%p, %zu, %d) failed\n",
		  newHeap, allocSize, lock_on);
      sas_printf (" freespace=%zu,  fragments=%zu,  max_free_block=%zu\n",
		  SASStringBTreeNodeFreeSpaceNoLock (newHeap),
		  SASStringBTreeNodeFreeFragmentsNoLock (newHeap),
		  SASStringBTreeNodeMaxFragmentNoLock (newHeap));
#endif
      if (nearHeader != NULL)
	{
	  if (SOMSASCheckBlockSig (nearHeader))
	    {
	      if ((nearHeader->baseBlock != nearHeader)
		  && (nearHeader->baseBlock != NULL))
		{
		  compoundHeader =
		    (SASStringBTreeHeader *) nearHeader->baseBlock;
		}
	      spill_lst = SASStringBTreeGetSpill (compoundHeader);
	      if (lock_on) SASLock (spill_lst, SasUserLock__WRITE);
	      if (spill_lst->count == 0)
		{
		  newHeap = SASStringBTreeSpillAlloc (nearObj, lock_on);
		  if (newHeap != NULL)
		    {
		      result =
			SASStringBTreeNodeAllocNoLock (newHeap, allocSize);
#ifdef __SASDebugPrint__
		      sas_printf
			("SASStringBTreeNodeNearAlloc(%p, %ld, %d) added spill area@%p\n",
			 nearObj, allocSize, lock_on, newHeap);
#endif
		    }
		}

	      if (result == NULL)
		{
		  for (int i = 0; i < spill_lst->count; i++)
		    {
		      newHeap =
			(SASStringBTreeNode_t) spill_lst->spillHeap[i];
		      result = SASStringBTreeNodeAlloc (newHeap, allocSize, lock_on);
		      if (result != NULL)
			break;
		    }
		}

	      if (result == NULL)
		{
		  newHeap = SASStringBTreeSpillAlloc (nearObj, lock_on);
		  if (newHeap != NULL)
		    {
		      result =
			SASStringBTreeNodeAllocNoLock (newHeap, allocSize);
#ifdef __SASDebugPrint__
		      sas_printf
			("SASStringBTreeNodeNearAlloc(%p, %ld, %d) added spill area@%p\n",
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
void SASStringBTreeNodeNearDealloc(void *memAddr, long allocSize)
{							
    SASNearDealloc (memAddr, allocSize);
}
*/
block_size_t
SASStringBTreeNodeFreeFragmentsNoLock (SASStringBTreeNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = -1;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (headerBlock->blockFreeSpace != NULL)
	heapFree = freeNode_freeFragmentsTotal (headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASStringBTreeNodeFreeFragmentNoLock(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return heapFree;
}

block_size_t
SASStringBTreeNodeMaxFragmentNoLock (SASStringBTreeNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = -1;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (headerBlock->blockFreeSpace != NULL)
	heapFree = freeNode_maxFragment (headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASStringBTreeNodeMaxFragmentNoLock(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return heapFree;
}

block_size_t
SASStringBTreeNodeFreeSpaceNoLock (SASStringBTreeNode_t heap)
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
      sas_printf
	("SASStringBTreeNodeFreeSpace(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return heapFree;
}

block_size_t
SASStringBTreeNodeFreeSpace (SASStringBTreeNode_t heap, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
    {
      if (lock_on) SASLock (heap, SasUserLock__WRITE);
      heapFree = SASStringBTreeNodeFreeSpaceNoLock (heap);
      if (lock_on) SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASStringBTreeNodeFreeSpace(%p,%d) does not match type/subtype\n",
	 heap, lock_on);
#endif
    }
  return heapFree;
}

void
SASStringBTreeNodeDestroyNoLock (SASStringBTreeNode_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_STRINGBTREENODE))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (heap, heapSize);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeDestroy(%p) does not match type/subtype\n", heap);
#endif
    }
}

void
SASStringBTreeNodeDestroy (SASStringBTreeNode_t heap, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_STRINGBTREENODE))
    {
      if (lock_on) SASLock (heap, SasUserLock__WRITE);
      SASStringBTreeNodeDestroyNoLock (heap);
      if (lock_on) SASUnlock (heap);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeDestroy(%p,%d) does not match type/subtype\n", heap, lock_on);
#endif
    }
}

char *
SASStringBTreeNodeGetKeyIndexed (SASStringBTreeNode_t header, short pos)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  char *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeGetKeyIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = node->keys[pos];
    }
  return result;
}

SASStringBTreeNode_t
SASStringBTreeNodeGetBranchIndexed (SASStringBTreeNode_t header, short pos)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeGetBranchIndexed(%p, %hd) does not match type/subtype\n",
	 header, pos);
#endif
    }
  else
    {
      result = (SASStringBTreeNode_t) node->branch[pos];
    }
  return result;
}

void *
SASStringBTreeNodeGetValIndexed (SASStringBTreeNode_t header, short pos)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeGetValIndexed(%p, %hd) does not match type/subtype\n",
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
SASStringBTreeNodePutValIndexed (SASStringBTreeNode_t header,
				 short pos, void *val)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  void *result = NULL;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeGetValIndexed(%p, %hd) does not match type/subtype\n",
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

int
SASStringBTreeNodeGetCount (SASStringBTreeNode_t header)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  int result = -1;

  if (SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
      result = node->count;
    }
  return result;
}

// we must return
// a combined "found" and "position" result. If "found"
// result is >=0 and == "position". Otherwise result < 0 and
// "position == result + 256;
short
SASStringBTreeNodeSearchNode (SASStringBTreeNode_t header, char *target)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short position;
  int found;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearchNode(%p) does not match type/subtype\n",
	 header);
#endif
      return (short) -256;
    }

#ifdef __SASDebugPrint__
  sas_printf ("SearchNode target=%s keys[1]=%s\n", target, node->keys[1]);
#endif

  if (strcmp (target, node->keys[1]) < 0)
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
		 && (strcmp (target, node->keys[position]) < 0))
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("SearchNode keys[%d]=%s\n",
			  position, node->keys[position]);
#endif
	      position--;
	    };
	  found = (strcmp (target, node->keys[position]) == 0);
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
		  sas_printf ("SearchNode keys[%d]=%s m=%d\n",
			      position, node->keys[position], m);
#endif
		  if (strcmp (target, node->keys[position]) < 0)
		    {
		      position -= m;
		    }
		  else
		    {
		      if (strcmp (target, node->keys[position]) > 0)
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
	      if (strcmp (target, node->keys[position]) < 0)
		{		// target between positions; fix up
#ifdef __SASDebugPrint__
		  sas_printf ("SearchNode keys[%d]=%s fix up\n",
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
	  found = (strcmp (target, node->keys[position]) == 0);
	}
    }

  if (!found)
    {
      position = (short) (position - 256);
    }
  return position;
}

int
SASStringBTreeNodeSearch (SASStringBTreeNode_t header,
			  char *target, SBTnodePosRef * ref)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearch(%p, %s, %p) does not match type/subtype\n",
	 header, target, ref);
#endif
      return false;
    }

  pos = SASStringBTreeNodeSearchNode (header, target);
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
  sas_printf ("Search target=%s pos=%d\n", target, pos);
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
	  result = SASStringBTreeNodeSearch (node->branch[pos], target, ref);
	}
    }
  return result;
}

int
SASStringBTreeNodeSearchGT (SASStringBTreeNode_t header,
			    char *target, SBTnodePosRef * ref)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearchGT(%p, %s, %p) does not match type/subtype\n",
	 header, target, ref);
#endif
      return false;
    }

  pos = SASStringBTreeNodeSearchNode (header, target);
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
  sas_printf ("SearchGT target=%s pos=%d\n", target, pos);
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
	      sas_printf ("SearchGT found next =%s pos=%d\n", target,
			  ref->pos);
#endif
	    }
	  else
	    {
	      result = false;
	    }
	}
      else
	{
	  result =
	    SASStringBTreeNodeSearchGT (node->branch[pos], target, ref);
	}
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result =
	    SASStringBTreeNodeSearchGT (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos < node->count)
		{
		  ref->node = header;
		  ref->pos = pos + 1;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchGT not found =%s pos=%d\n", target,
			      ref->pos);
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
	      sas_printf ("SearchGT not found =%s pos=%d\n", target,
			  ref->pos);
#endif
	    }
	}
    }
  return result;
}

int
SASStringBTreeNodeSearchGE (SASStringBTreeNode_t header,
			    char *target, SBTnodePosRef * ref)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearchGE(%p, %s, %p) does not match type/subtype\n",
	 header, target, ref);
#endif
      return false;
    }

  pos = SASStringBTreeNodeSearchNode (header, target);
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
  sas_printf ("SearchGE target=%s pos=%d\n", target, pos);
#endif
  if (found)
    {
      ref->node = header;
      ref->pos = pos;
      result = found;
#ifdef __SASDebugPrint__
      sas_printf ("SearchGE found =%s pos=%d\n", target, ref->pos);
#endif
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result =
	    SASStringBTreeNodeSearchGE (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos < node->count)
		{
		  ref->node = header;
		  ref->pos = pos + 1;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchGE not found =%s pos=%d\n", target,
			      ref->pos);
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
	      sas_printf ("SearchGE not found =%s pos=%d\n", target,
			  ref->pos);
#endif
	    }
	}
    }
  return result;
}

int
SASStringBTreeNodeSearchLT (SASStringBTreeNode_t header,
			    char *target, SBTnodePosRef * ref)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearchLT(%p, %s, %p) does not match type/subtype\n",
	 header, target, ref);
#endif
      return false;
    }

  pos = SASStringBTreeNodeSearchNode (header, target);
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
  sas_printf ("SearchLT target=%s pos=%d\n", target, pos);
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
	      sas_printf ("SearchLT found next =%s pos=%d\n", target,
			  ref->pos);
#endif
	    }
	  else
	    {
	      result = false;
	    }
	}
      else
	{
	  result =
	    SASStringBTreeNodeSearchLT (node->branch[pos - 1], target, ref);
	}
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result =
	    SASStringBTreeNodeSearchLT (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos > 0)
		{
		  ref->node = header;
		  ref->pos = pos;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchLT not found =%s pos=%d\n", target,
			      ref->pos);
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
	      sas_printf ("SearchLT not found =%s pos=%d\n", target,
			  ref->pos);
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
SASStringBTreeNodeSearchLE (SASStringBTreeNode_t header,
			    char *target, SBTnodePosRef * ref)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short pos;
  int found;
  int result = false;

  if (!SOMSASCheckBlockSigAndTypeAndSubtype ((SASBlockHeader *) header,
					     SAS_RUNTIME_STRINGBTREENODE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SASStringBTreeNodeSearchLE(%p, %s, %p) does not match type/subtype\n",
	 header, target, ref);
#endif
      return false;
    }

  pos = SASStringBTreeNodeSearchNode (header, target);
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
  sas_printf ("SearchLE target=%s pos=%d\n", target, pos);
#endif
  if (found)
    {
      ref->node = header;
      ref->pos = pos;
      result = true;
#ifdef __SASDebugPrint__
      sas_printf ("SearchLE found next =%s pos=%d\n", target, ref->pos);
#endif
    }
  else
    {
      if (node->branch[pos] != NULL)
	{
	  result =
	    SASStringBTreeNodeSearchLE (node->branch[pos], target, ref);
	  if (!result)
	    {
	      if (pos > 0)
		{
		  ref->node = header;
		  ref->pos = pos;
		  result = true;
#ifdef __SASDebugPrint__
		  sas_printf ("SearchLE not found =%s pos=%d\n", target,
			      ref->pos);
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
	      sas_printf ("SearchLE not found =%s pos=%d\n", target,
			  ref->pos);
#endif
	    }
	}
    }
  return result;
}

static inline void
SASStringBTreeNodeKeyMove (SASStringBTreeNode_t heap, short pos, char *key,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  char *oldkey = node->keys[pos];
  char *tempkey;
  int key_len = strlen (key) + 1;

  tempkey = (char *) SASStringBTreeNodeNearAlloc (heap, key_len, lock_on);
  node->keys[pos] = strcpy (tempkey, key);

  if (oldkey != NULL)
    {
      int keylen = strlen (oldkey) + 1;
      SASStringBTreeNodeNearDealloc (heap, oldkey, keylen, lock_on);
      node->keys[pos] = NULL;
    }

  if (key != NULL)
    {
      if ((unsigned long) key >= getfastMemLow ())
	{
	  if ((unsigned long) key <= getfastMemHigh ())
	    SASStringBTreeNodeNearDealloc (heap, key, key_len, lock_on);
	}
    }
}

static inline void
SASStringBTreeNodeKeyCopy (SASStringBTreeNode_t heap, short pos, char *key,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  char *oldkey = node->keys[pos];
  char *tempkey;
  int key_len = strlen (key) + 1;

  tempkey = (char *) SASStringBTreeNodeNearAlloc (heap, key_len, lock_on);
  node->keys[pos] = strcpy (tempkey, key);

  if (oldkey != NULL)
    {
      int keylen = strlen (oldkey) + 1;
      SASStringBTreeNodeNearDealloc (heap, oldkey, keylen, lock_on);
    }
}

static inline void
SASStringBTreeNodeKeyDelete (SASStringBTreeNode_t heap, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  char *oldkey = node->keys[pos];

  if (oldkey != NULL)
    {
      int keylen = strlen (oldkey) + 1;
      SASStringBTreeNodeNearDealloc (heap, oldkey, keylen, lock_on);
    }
}

static inline void
SASStringBTreeNodeKeyReplace (SASStringBTreeNode_t heap, short pos, char *key,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  char *tempkey;
  int key_len = strlen (key) + 1;

  tempkey = (char *) SASStringBTreeNodeNearAlloc (heap, key_len, lock_on);
  node->keys[pos] = strcpy (tempkey, key);
}

static void
SASStringBTreeNodePushIn (SASStringBTreeNode_t node_t,
			  __SBTnodeKeyRef * ref, short k, lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  char *str_ptr;
  char *end_ptr;
  char *temp_key;
  size_t key_len, max_frag;
  short i;
#ifdef __SASDebugPrint__
  sas_printf ("PushIn@%p ref->key=%p k=%hd lock_on=%d\n", node, ref->key, k, lock_on);
#endif
  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
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
	  key_len = strlen (temp_key) + 1;
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("PushIn@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i + 1), temp_key, key_len);
#endif
	      SASStringBTreeNodeKeyCopy ((SASStringBTreeNode_t) node,
					 (i + 1), temp_key, lock_on);
	      max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
	    }
	}
    }

  SASStringBTreeNodeKeyReplace (node, (k + 1), ref->key, lock_on);
  node->vals[k + 1] = ref->val;
  node->branch[k + 1] = (SASStringBTreeNodeHeader *) ref->node;
  node->count++;
}

static void
SASStringBTreeNodeSplit (SASStringBTreeNode_t node_t,
			 __SBTnodeKeyRef * xref,
			 short k, __SBTnodeKeyRef * yref,
			lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  short i, median;
  short min = node->max_count / 2;
  char **thisKeys = node->keys;
  SASStringBTreeNodeHeader **thisBranch = node->branch;
  void **thisVals = node->vals;
  SASStringBTreeNodeHeader *yr;	// temp in case xref & yref are same
  char *str_ptr;
  char *end_ptr;
  char *temp_key;
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
  sas_printf ("Split@%p x=%s k=%hd median=%hd lock_on=%d\n", node, xref->key, k, median, lock_on);
#endif

  if (lock_on == LOCK_ON)
    yr = (SASStringBTreeNodeHeader *) SASStringBTreeNearAlloc (node_t);
  else
    yr = (SASStringBTreeNodeHeader *) SASStringBTreeNearAllocNoLock (node_t);

  SASStringBTreeNodeHeader **yrBranch = yr->branch;
  void **yrVals = yr->vals;

  for (i = (short) (median + 1); i <= node->max_count; i++)
    {

      SASStringBTreeNodeKeyMove (yr, (i - median), thisKeys[i], lock_on);
      yrVals[i - median] = thisVals[i];
      yrBranch[i - median] = thisBranch[i];
    }
  yr->count = (short) (node->max_count - median);
  node->count = median;

  if (k <= min)
    {
      SASStringBTreeNodePushIn (node_t, xref, k, lock_on);
    }
  else
    {
      SASStringBTreeNodePushIn (yr, xref, (short) (k - median), lock_on);
    }

  yrBranch[0] = (SASStringBTreeNodeHeader *) thisBranch[node->count];
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
  max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
  for (i = 1; i <= (node->count); i++)
    {
      temp_key = node->keys[i];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = strlen (temp_key) + 1;
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("Split@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i + 1), temp_key, key_len);
#endif
	      SASStringBTreeNodeKeyCopy ((SASStringBTreeNode_t) node,
					 i, temp_key, lock_on);
	      max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
	    }
	}
    }
}

static int
SASStringBTreeNodePushDown (SASStringBTreeNode_t node_t,
			    char *newkey, void *newval, __SBTnodeKeyRef * ref,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  short pos;			// AKA k
  int found;
  int pushup = false;

#ifdef __SASDebugPrint__
  sas_printf ("PushDown@%p newkey=%s lock_on=%d\n", node, newkey, lock_on);
#endif
  pos = SASStringBTreeNodeSearchNode (node_t, newkey);
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
      sas_printf ("Duplicate key = <%s>\n", newkey);
#endif
      ref->dupKey = true;
      return false;
      //DupKeyException e = new DupKeyException(new Long(newkey));
      //throw e;
    };

  if (node->branch[pos] != NULL)
    {
      pushup = SASStringBTreeNodePushDown (node->branch[pos], newkey,
					   newval, ref, lock_on);
    }
  else
    {
      pushup = true;
      ref->key = newkey;
      ref->val = newval;
      ref->node = NULL;
    }

#ifdef __SASDebugPrint__
  sas_printf ("PushDown pushup=%d  ref.key=%s\n", pushup, ref->key);
#endif
  if (pushup)
    {
#ifdef __SASDebugPrint__
      sas_printf ("PushDown count=%hd\n", node->count);
#endif
      if (node->count < node->max_count)
	{
	  pushup = false;
	  SASStringBTreeNodePushIn (node_t, ref, pos, lock_on);
	}
      else
	{
	  pushup = true;
	  SASStringBTreeNodeSplit (node_t, ref, pos, ref, lock_on);
	}
    }
  return pushup;
}

void
SASStringBTreeNodeInitialize (SASStringBTreeNode_t node_t,
			      char *newkey, void *newval,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;

#ifdef __SASDebugPrint__
  sas_printf ("Initialize@%p newkey=%s, newval=%p lock_on=%d\n",
		node, newkey, newval, lock_on);
#endif
  node->count = 1;
  SASStringBTreeNodeKeyCopy (node, 1, newkey, lock_on);
  node->vals[1] = newval;
  node->branch[1] = NULL;
  node->branch[0] = NULL;
}

SASStringBTreeNode_t
SASStringBTreeNodeInsert (SASStringBTreeNode_t node_t,
			  char *newkey, void *newval, lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  int pushup;
  __SBTnodeKeyRef xref = { NULL, NULL, NULL, false };
  SASStringBTreeNode_t result = node_t;

#ifdef __SASDebugPrint__
  sas_printf ("Insert@%p newkey=%s lock_on=%d\n", node, newkey, lock_on);
#endif
  pushup = SASStringBTreeNodePushDown (node_t, newkey, newval, &xref, lock_on);
  if (pushup)
    {
      SASStringBTreeNodeHeader *new_node;
      if (lock_on == LOCK_ON)
        result = SASStringBTreeNearAlloc (node_t);
      else
        result = SASStringBTreeNearAllocNoLock (node_t);
      new_node = (SASStringBTreeNodeHeader *) result;
      new_node->count = 1;
      SASStringBTreeNodeKeyMove (new_node, 1, xref.key, lock_on);
      new_node->vals[1] = xref.val;
      new_node->branch[1] = (SASStringBTreeNodeHeader *) xref.node;
      new_node->branch[0] = node;
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
SASStringBTreeNodeRemove (SASStringBTreeNode_t node_t, short pos,
			lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  SASStringBTreeNodeHeader *q;
  char *str_ptr;
  char *end_ptr;
  char *temp_key;
  size_t key_len, max_frag;
  short i;

  q = ((SASStringBTreeNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("Remove@%p key[%hd]=%s branch=%p lock_on=%d\n",
	      node_t, pos, node->keys[pos], q, lock_on);
#endif
  SASStringBTreeNodeKeyDelete (node_t, pos, lock_on);

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
  for (i = (short) (pos + 1); i <= node->count; i++)
    {
      node->keys[i - 1] = node->keys[i];
      node->vals[i - 1] = node->vals[i];
      node->branch[i - 1] = node->branch[i];
#if  __SASDebugPrint__ > 1
      sas_printf ("Remove copy key[%hd]=%s to %d\n", i, node->keys[i],
		  (i - 1));
#endif
      temp_key = node->keys[i - 1];
      if (((unsigned long) temp_key < (unsigned long) str_ptr)
	  || ((unsigned long) temp_key > (unsigned long) end_ptr))
	{
	  /* far key */
	  key_len = strlen (temp_key) + 1;
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("Remove@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (i - 1), temp_key, key_len);
#endif
	      SASStringBTreeNodeKeyCopy ((SASStringBTreeNode_t) node,
					 (i - 1), temp_key, lock_on);
	      max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
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
      SASStringBTreeNearDealloc (q);
    }
#if __SASDebugPrint__ > 1
  sas_printf ("Remove: subtree=");
  SASStringBTreeNodePrint (node_t);
  sas_printf ("\n");
#endif
}

void
SASStringBTreeNodeCombine (SASStringBTreeNode_t node_t, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  SASStringBTreeNodeHeader *q, *r;
  short c;

#ifdef __SASDebugPrint__
  sas_printf ("Combine@%p pos=%hd lock_on=%d\n", node_t, pos, lock_on);
#endif

  q = ((SASStringBTreeNodeHeader *) node->branch[pos]);
  r = ((SASStringBTreeNodeHeader *) node->branch[pos - 1]);
  r->count++;
  //r->keys[r->count] = node->keys[pos];
  SASStringBTreeNodeKeyMove (r, r->count, node->keys[pos], lock_on);
  node->keys[pos] = NULL;	// Move frees the key string in node
  r->vals[r->count] = node->vals[pos];
  r->branch[r->count] = q->branch[0];
  q->branch[0] = NULL;

#ifdef __SASDebugPrint__
  sas_printf ("Combine move=%s\n", node->keys[pos]);
#endif
  for (c = 1; c <= q->count; c++)
    {
      r->count++;
      // r->keys[r->count] = q->keys[c];
      SASStringBTreeNodeKeyMove (r, r->count, q->keys[c], lock_on);
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
  SASStringBTreeNodeRemove (node_t, pos, lock_on);
// dispose of q, since it is empty
  SASStringBTreeNearDealloc (q);
}

void
SASStringBTreeNodeMoveLeft (SASStringBTreeNode_t node_t, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  SASStringBTreeNodeHeader *l, *r;
  char *str_ptr;
  char *end_ptr;
  char *temp_key;
  size_t key_len, max_frag;
  short c;

  l = ((SASStringBTreeNodeHeader *) node->branch[pos - 1]);
  r = ((SASStringBTreeNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("MoveLeft@%p pos=%hd left@%p right@%p lock_on=%d\n", node_t, pos, l, r, lock_on);
#endif
  l->count++;
  // l->keys[l->count] = node->keys[pos];
  SASStringBTreeNodeKeyMove (l, l->count, node->keys[pos], lock_on);
  node->keys[pos] = NULL;
  l->vals[l->count] = node->vals[pos];
  l->branch[l->count] = r->branch[0];

  //node->keys[pos] = r->keys[1];
  SASStringBTreeNodeKeyMove (node, pos, r->keys[1], lock_on);
  node->vals[pos] = r->vals[1];
  r->branch[0] = r->branch[1];
  r->count--;

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
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
	  key_len = strlen (temp_key) + 1;
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("MoveLeft@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (c), temp_key, key_len);
#endif
	      SASStringBTreeNodeKeyCopy ((SASStringBTreeNode_t) r,
					 (c), temp_key, lock_on);
	      max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
	    }
	}
    }
  r->keys[r->count + 1] = NULL;
  r->vals[r->count + 1] = NULL;
  r->branch[r->count + 1] = NULL;
#if  __SASDebugPrint__ > 1
  sas_printf ("MoveLeft: subtree=");
  SASStringBTreeNodePrint (node_t);
  sas_printf ("\n");
#endif
}

void
SASStringBTreeNodeMoveRight (SASStringBTreeNode_t node_t, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) node_t;
  SASStringBTreeNodeHeader *l, *r;
  char *str_ptr;
  char *end_ptr;
  char *temp_key;
  size_t key_len, max_frag;
  short c;

  l = ((SASStringBTreeNodeHeader *) node->branch[pos - 1]);
  r = ((SASStringBTreeNodeHeader *) node->branch[pos]);
#ifdef __SASDebugPrint__
  sas_printf ("MoveRight@%p pos=%hd left@%p right@%p lock_on=%d\n", node_t, pos, l, r, lock_on);
#endif

  str_ptr = (char *) node;
  end_ptr = str_ptr + node->blockHeader.blockSize;
  max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
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
	  key_len = strlen (temp_key) + 1;
	  if (key_len <= max_frag)
	    {
#if __SASDebugPrint__ > 1
	      sas_printf ("MoveRight@%p copy far pos=%hd key@%p len=%d\n",
			  node_t, (c + 1), temp_key, key_len);
#endif
	      SASStringBTreeNodeKeyCopy ((SASStringBTreeNode_t) r,
					 (c + 1), temp_key, lock_on);
	      max_frag = SASStringBTreeNodeMaxFragmentNoLock (node_t);
	    }
	}
    };
  // r->keys[1] = node->keys[pos];
  r->keys[1] = NULL;
  SASStringBTreeNodeKeyMove (r, 1, node->keys[pos], lock_on);
  node->keys[pos] = NULL;
  r->vals[1] = node->vals[pos];
  r->branch[1] = r->branch[0];
  r->count++;
#if  __SASDebugPrint__ > 1
  sas_printf ("MoveRight: right subtree=");
  SASStringBTreeNodePrint (node_t);
  sas_printf ("\n");
#endif

  // node->keys[pos] = l->keys[l->count];
  SASStringBTreeNodeKeyMove (node, pos, l->keys[l->count], lock_on);
  node->vals[pos] = l->vals[l->count];
  r->branch[0] = l->branch[l->count];

  l->keys[l->count] = NULL;
  l->vals[l->count] = NULL;
  l->branch[l->count] = NULL;
  l->count--;
#if __SASDebugPrint__ > 1
  sas_printf ("MoveRight: subtree=");
  SASStringBTreeNodePrint (node_t);
  sas_printf ("\n");
#endif
}

 // finds a key and inserts it into this.branch[pos] to restore minimums
void
SASStringBTreeNodeRestore (SASStringBTreeNode_t header, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  short min = node->max_count / 2;

#ifdef __SASDebugPrint__
  sas_printf ("Restore@%p pos=%hd lock_on=%d\n", header, pos, lock_on);
#endif
  if (pos > 0)
    {
      if (((SASStringBTreeNodeHeader *) node->branch[pos - 1])->count > min)
	{			// move key to right
	  SASStringBTreeNodeMoveRight (header, pos, lock_on);
	}
      else
	{
	  if (((SASStringBTreeNodeHeader *) node->branch[pos])->count > min)
	    {			// move key to left
#ifdef __SASDebugPrint__
	      sas_printf ("Restore branch[pos]->count=%hd\n",
			  ((SASStringBTreeNodeHeader *) node->branch[pos])->
			  count);
#endif
	      SASStringBTreeNodeMoveLeft (header, pos, lock_on);
	    }
	  else
	    {
	      if (node->count > pos)
		{
#ifdef __SASDebugPrint__
		  sas_printf ("Restore branch[pos+1]->count=%hd\n",
			      ((SASStringBTreeNodeHeader *) node->
			       branch[pos + 1])->count);
#endif
		  if (((SASStringBTreeNodeHeader *) node->branch[pos + 1])->
		      count > min)
		    {		// move key to left
		      SASStringBTreeNodeMoveLeft (header, (short) (pos + 1), lock_on);
		    }
		  else
		    {
		      SASStringBTreeNodeCombine (header, pos, lock_on);
		    }
		}
	      else
		{
		  SASStringBTreeNodeCombine (header, pos, lock_on);
		}
	    }
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("Restore branch[1]->count=%hd\n",
		  ((SASStringBTreeNodeHeader *) node->branch[1])->count);
#endif
      if (((SASStringBTreeNodeHeader *) node->branch[1])->count > min)
	{
	  SASStringBTreeNodeMoveLeft (header, (short) 1, lock_on);
	}
      else
	{
	  SASStringBTreeNodeCombine (header, (short) 1, lock_on);
	}
    }
}

// replaces this.key[k] by its immediate successor
void
SASStringBTreeNodeSuccessor (SASStringBTreeNode_t header, short pos,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  SASStringBTreeNodeHeader *q;

#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p pos=%hd lock_on=%d\n", header, pos, lock_on);
#endif
  q = ((SASStringBTreeNodeHeader *) node->branch[pos]);
  while (q->branch[0] != NULL)
    {
      q = ((SASStringBTreeNodeHeader *) q->branch[0]);
#ifdef __SASDebugPrint__
      sas_printf ("Successor@%p key=%s\n", q, q->keys[1]);
#endif
    }
#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p replace key=%s with key=%s\n ",
	      header, node->keys[pos], q->keys[1]);
#endif
  // node->keys[k] = q->keys[1];
  SASStringBTreeNodeKeyCopy (node, pos, q->keys[1], lock_on);
//      q->keys[1] = NULL;
  node->vals[pos] = q->vals[1];
//      q->vals[1] = NULL;
#ifdef __SASDebugPrint__
  sas_printf ("Successor@%p key[%hd]=%s @%p\n",
	      header, pos, node->keys[pos], node->keys[pos]);
  sas_printf ("Successor: subtree=");
  SASStringBTreeNodePrint (header);
  sas_printf ("\n");
#endif
}

// recursive delete
int
SASStringBTreeNodeRecDelete (SASStringBTreeNode_t header, char *target,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  SASStringBTreeNodeHeader *q;
  int found = false;
  short min = node->max_count / 2;
  short k, pos;

#ifdef __SASDebugPrint__
  sas_printf ("RecDelete@%p target=%s lock_on=%d\n", header, target, lock_on);
#endif
  pos = SASStringBTreeNodeSearchNode (header, target);
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
	  SASStringBTreeNodeRemove (header, k, lock_on);
	}
      else
	{			// replaces key[k] by its successor
	  SASStringBTreeNodeSuccessor (header, k, lock_on);
	  q = ((SASStringBTreeNodeHeader *) node->branch[k]);
	  if (q != NULL)
	    {
	      found = SASStringBTreeNodeRecDelete (q, node->keys[k], lock_on);
#if __SASDebugPrint__ > 1
	      sas_printf ("RecDelete after Successor found=%d\n", found);
	      sas_printf ("RecDelete: subtree=");
	      SASStringBTreeNodePrint (header);
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
      q = ((SASStringBTreeNodeHeader *) node->branch[k]);
      if (q != NULL)
	{
	  found = SASStringBTreeNodeRecDelete (q, target, lock_on);
	}
      else
	{
	  found = false;
	}
    }

  if (node->branch[k] != NULL)
    {
      if (((SASStringBTreeNodeHeader *) node->branch[k])->count < min)
	{
	  SASStringBTreeNodeRestore (header, k, lock_on);
	}
    }
#ifdef __SASDebugPrint__
  sas_printf ("RecDelete return=%d\n", found);
#if __SASDebugPrint__ > 1
  sas_printf ("RecDelete: subtree=");
  SASStringBTreeNodePrint (header);
  sas_printf ("\n");
#endif
#endif
  return found;
}

SASStringBTreeNode_t
SASStringBTreeNodeDelete (SASStringBTreeNode_t header, char *target,
				lock_on_t lock_on)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) header;
  SASStringBTreeNodeHeader *result = node;
  int found = false;

#ifdef __SASDebugPrint__
  sas_printf ("Delete target=%s lock_on=%d\n", target, lock_on);
#endif
  found = SASStringBTreeNodeRecDelete (header, target, lock_on);
  if (found)
    {
      if (node->count == 0)
	{
	  result = ((SASStringBTreeNodeHeader *) node->branch[0]);
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
