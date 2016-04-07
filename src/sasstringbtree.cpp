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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sasalloc.h"
#include "freenode.h"
#include "sasio.h"
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sasstringbtree.h"
#include "sasstringbtreepriv.h"
#include "sasstringbtreenodepriv.h"

static inline int
SASStringBTreeAvail (SASStringBTreeHeader * headerBlock)
{
  return (headerBlock->blockHeader.blockFreeSpace != NULL);
}

static inline int
SASStringBTreePercentFree (SASStringBTreeHeader * heapBlock)
{
  int percent = 0;
  int divisor;
  block_size_t heapFree = 0;

  if (heapBlock->blockHeader.blockFreeSpace != NULL)
    {
      heapFree =
	freeNode_freeSpaceTotal (heapBlock->blockHeader.blockFreeSpace);
      divisor = __builtin_ctzl(heapBlock->blockHeader.blockSize);
      percent = (int)((heapFree  * 100) >> divisor);
    }
#if 0
  sas_printf ("SASStringBTreePercentFree(%p) = %d for (%ld, %ld)\n",
	      heapBlock, percent, heapFree, heapUsed);
#endif
  return percent;
}

static inline int
SASStringBTreePercentUsed (SASStringBTreeHeader * heapBlock)
{
  int percent = 100;
  int divisor;
  block_size_t heapFree = 0;
  block_size_t heapUsed = 0;

  if (heapBlock->blockHeader.blockFreeSpace != NULL)
    {
      heapFree =
	freeNode_freeSpaceTotal (heapBlock->blockHeader.blockFreeSpace);
      heapUsed = heapBlock->blockHeader.blockSize - heapFree;
      divisor = __builtin_ctzl(heapBlock->blockHeader.blockSize);
      percent = (int)((heapUsed  * 100) >> divisor);
    }
#if 0
  sas_printf ("SASStringBTreePercentFree(%p) = %d for (%ld, %ld)\n",
	      heapBlock, percent, heapFree, heapUsed);
#endif
  return percent;
}

static inline int
SASStringBTreeIsExpanding (SASStringBTreeHeader * headerBlock)
{
  return (headerBlock->expandList != NULL);
}

static inline int
SASStringBTreeContains (SASStringBTreeHeader * headerBlock,
			SASStringBTreeNode_t simpleHeap)
{
  block_size_t containedHeap = (block_size_t) simpleHeap;
  block_size_t containerLow = (block_size_t) headerBlock;
  block_size_t containerHigh = (block_size_t) headerBlock +
    headerBlock->blockHeader.blockSize;
  return ((containedHeap > containerLow) && (containedHeap < containerHigh));
}

static inline void
SASStringBTreeFreeInternal (SASStringBTreeHeader * headerBlock,
			    SASStringBTreeNode_t free_block)
{
  freeNode *free_node = (freeNode *) free_block;
  block_size_t simpleSize = headerBlock->pageSize;

  memset (free_block, 0, simpleSize);
  freeNode_init (free_node, simpleSize);
  freeNode_deallocSpace (free_node, &headerBlock->blockHeader.blockFreeSpace,
			 simpleSize);
}

SASStringBTree_t
SASStringBTreeExpandInit (void *heap_seg,
			  block_size_t heap_size, block_size_t page_size)
{
  SASStringBTreeHeader *heapBlock = (SASStringBTreeHeader *) heap_seg;
  SASStringBTreeSpillList *spill_lst;
  char *heapStart = NULL;
  node_size_t remaining;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + default_page;
      initSOMSASBlock ((SASBlockHeader *) heapBlock,
		       SAS_RUNTIME_STRINGBTREE, heap_size, heapStart);
    }

  heapBlock->pageSize = page_size;

  remaining = default_page - sizeof (SASStringBTreeHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);

  heapBlock->expandList = NULL;

  spill_lst =
    (SASStringBTreeSpillList *)
    freeNode_allocSpace (heapBlock->headerFreeSpace,
			 &heapBlock->headerFreeSpace,
			 sizeof (SASStringBTreeSpillList));
  if (spill_lst != NULL)
    {
      heapBlock->spillList = spill_lst;
      spill_lst->count = 0;
      spill_lst->max_count = SASBTREE_SPILLLIST_SIZE;
      for (int i = 0; i < SASBTREE_SPILLLIST_SIZE; i++)
	{
	  spill_lst->spillHeap[i] = NULL;
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeInit(%p, %zu, %zu) spillList alloc failed\n",
		  heap_seg, heap_size, page_size);
#endif
    }

  return (SASStringBTree_t) heapBlock;
}

SASStringBTree_t
SASStringBTreeInit (void *heap_seg,
		    block_size_t heap_size,
		    block_size_t page_size, int expanding)
{
  SASStringBTreeHeader *heapBlock = (SASStringBTreeHeader *) heap_seg;
  SASStringBTreeCommon *commonAlloc;
  SASCompoundExpandList *list;
  SASStringBTreeSpillList *spill_lst;
  char *heapStart = NULL;
  node_size_t remaining;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + default_page;
      initSOMSASBlock ((SASBlockHeader *) heapBlock,
		       SAS_RUNTIME_STRINGBTREE, heap_size, heapStart);
    }

  heapBlock->pageSize = page_size;

  remaining = default_page - sizeof (SASStringBTreeHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);
  heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
  heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;

  if (expanding)
    {
      list =
	(SASCompoundExpandList *)
	freeNode_allocSpace (heapBlock->headerFreeSpace,
			     &heapBlock->headerFreeSpace,
			     sizeof (SASCompoundExpandList));
      if (list != NULL)
	{
	  heapBlock->expandList = list;
	  list->count = 1;
	  list->max_count = 254;
	  list->heap[0] = heapBlock;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASStringBTreeInit(%p, %zu, %zu) ExpandList alloc failed\n",
	     heap_seg, heap_size, page_size);
#endif
	}
    }

  commonAlloc =
    (SASStringBTreeCommon *) freeNode_allocSpace (heapBlock->headerFreeSpace,
						  &heapBlock->headerFreeSpace,
						  sizeof
						  (SASStringBTreeCommon));
  if (commonAlloc != NULL)
    {
      heapBlock->common = commonAlloc;
      commonAlloc->version = 0;
      commonAlloc->modCount = 1;
      commonAlloc->max_key = NULL;
      commonAlloc->min_key = NULL;
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeInit(%p, %zu, %zu) Common alloc failed\n",
		  heap_seg, heap_size, page_size);
#endif
    }

  spill_lst =
    (SASStringBTreeSpillList *)
    freeNode_allocSpace (heapBlock->headerFreeSpace,
			 &heapBlock->headerFreeSpace,
			 sizeof (SASStringBTreeSpillList));
  if (spill_lst != NULL)
    {
      heapBlock->spillList = spill_lst;
      spill_lst->count = 0;
      spill_lst->max_count = SASBTREE_SPILLLIST_SIZE;
      for (int i = 0; i < SASBTREE_SPILLLIST_SIZE; i++)
	{
	  spill_lst->spillHeap[i] = NULL;
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeInit(%p, %zu, %zu) spillList alloc failed\n",
		  heap_seg, heap_size, page_size);
#endif
    }

  return (SASStringBTree_t) heapBlock;
}

SASStringBTree_t
SASStringBTreeFixedCreate (block_size_t heap_size)
{
  SASBlockHeader *heapBlock = NULL;
  SASStringBTree_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      newHeap = SASStringBTreeInit (heapBlock, heap_size,
				    default_page, false);
    }
  return newHeap;
}

SASStringBTree_t
SASStringBTreeExpandCreate (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  block_size_t heap_size = headerBlock->blockHeader.blockSize;
  block_size_t page_size = headerBlock->pageSize;
  SASCompoundExpandList *list = headerBlock->expandList;
  SASStringBTreeCommon *commonPtr = headerBlock->common;
  SASBlockHeader *heapBlock = NULL;
  SASStringBTree_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      if (list->count < list->max_count)
	{
	  SASStringBTreeHeader *newHeader, *prevHeader;
	  newHeap = SASStringBTreeExpandInit (heapBlock,
					      heap_size, page_size);
	  newHeader = (SASStringBTreeHeader *) newHeap;
	  newHeader->blockHeader.baseBlock = &headerBlock->blockHeader;
	  newHeader->blockHeader.nextBlock = &headerBlock->blockHeader;
	  newHeader->common = commonPtr;
	  prevHeader = list->heap[list->count - 1];
	  list->heap[list->count] = newHeader;
	  list->count++;
	  prevHeader->blockHeader.nextBlock = &newHeader->blockHeader;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASStringBTreeExpandCreate(%p) failed\n", headerBlock);
#endif
	}
    }
  return newHeap;
}

SASStringBTree_t
SASStringBTreeCreatePageSize (block_size_t heap_size, block_size_t page_size)
{
  SASBlockHeader *heapBlock = NULL;
  SASStringBTree_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      newHeap = SASStringBTreeInit (heapBlock, heap_size, page_size, true);
    }
  return newHeap;
}

SASStringBTree_t
SASStringBTreeCreate (block_size_t heap_size)
{
  return SASStringBTreeCreatePageSize (heap_size, default_page);
}

static SASStringBTreeNode_t
SASStringBTreeAllocInternal (SASStringBTreeHeader * headerBlock)
{
  SASBlockHeader *simpleBlock;
  block_size_t heapSize;
  block_size_t simpleSize;
  freeNode *mem = NULL;
  SASStringBTreeNode_t newHeap = NULL;

  heapSize = headerBlock->blockHeader.blockSize;
  simpleSize = headerBlock->pageSize;
  if (simpleSize < heapSize)
    {
      mem = freeNode_allocSpace (headerBlock->blockHeader.blockFreeSpace,
				 &headerBlock->blockHeader.blockFreeSpace,
				 simpleSize);
      if (mem != NULL)
	{
	  simpleBlock = (SASBlockHeader *) mem;
	  newHeap = SASStringBTreeNodeInit (mem, SAS_RUNTIME_STRINGBTREENODE,
					    simpleSize);
	  simpleBlock->baseBlock = (SASBlockHeader *) headerBlock;
	}
    }
  return newHeap;
}

SASStringBTreeNode_t
SASStringBTreeAllocNoLock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  SASStringBTreeNode_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_STRINGBTREE))
    {
      if (SASStringBTreeIsExpanding (headerBlock))
	{
	  SASCompoundExpandList *list = headerBlock->expandList;
	  SASStringBTreeHeader *expandHeader;
	  block_size_t i;
	  expandHeader = list->heap[list->count - 1];

	  if (SASStringBTreePercentUsed (expandHeader) >= DEFAULT_LOAD_FACTOR)
	    {
	      expandHeader = NULL;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASStringBTreeHeader *expandBlock = list->heap[i];
		  if (SASStringBTreePercentUsed (expandBlock)
		      < DEFAULT_LOAD_FACTOR)
		    {
		      expandHeader = expandBlock;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASStringBTreeHeader *)
		    SASStringBTreeExpandCreate (heap);
		}
	    }
	  if (expandHeader != NULL)
	    newHeap = SASStringBTreeAllocInternal (expandHeader);
	}
      else
	{
	  newHeap = SASStringBTreeAllocInternal (headerBlock);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeAllocNoLock(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

SASStringBTreeNode_t
SASStringBTreeAlloc (SASStringBTree_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASStringBTreeNode_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_STRINGBTREE))
    {
      SASStringBTreeHeader *heapHeader = (SASStringBTreeHeader *) heap;
      SASLock (heap, SasUserLock__WRITE);
      if (SASStringBTreeIsExpanding (heapHeader))
	{
	  SASCompoundExpandList *list = heapHeader->expandList;
	  SASStringBTreeHeader *lastHeader;
	  SASStringBTreeHeader *expandHeader = NULL;
	  block_size_t i;
	  block_size_t last_lock = 0;
	  lastHeader = list->heap[list->count - 1];
	  if (lastHeader != heapHeader)
	    SASLock (lastHeader, SasUserLock__WRITE);

	  if (SASStringBTreePercentUsed (lastHeader) >= DEFAULT_LOAD_FACTOR)
	    {
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASStringBTreeHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASStringBTreePercentUsed (expandBlock)
		      < DEFAULT_LOAD_FACTOR)
		    {
		      expandHeader = expandBlock;
		      last_lock = i;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASStringBTreeHeader *)
		    SASStringBTreeExpandCreate (heap);
	          last_lock = list->count - 2;
		}
	    }
	  else
	    {
	      expandHeader = lastHeader;
	    }
	  if (expandHeader != NULL)
	    newHeap = SASStringBTreeAllocInternal (expandHeader);

	  if (lastHeader != expandHeader)
	    {
	      for (i = 1; i <= last_lock; i++)
		{
		  SASUnlock (list->heap[i]);
		}
	    }
	  if (lastHeader != heapHeader)
	    SASUnlock (lastHeader);
	}
      else
	{
	  newHeap = SASStringBTreeAllocInternal (heapHeader);
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeAlloc(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

void
SASStringBTreeFreeNoLock (SASStringBTree_t heap,
			  SASStringBTreeNode_t free_block)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
				  SAS_RUNTIME_STRINGBTREENODE))
    {
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_STRINGBTREE))
	{
	  if (SASStringBTreeIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASStringBTreeHeader *expandBlock = list->heap[i];
		  if (SASStringBTreeContains (expandBlock, free_block))
		    {
		      SASStringBTreeFreeInternal (expandBlock, free_block);
		      break;
		    }
		}
	    }
	  else
	    {
	      if (SASStringBTreeContains (headerBlock, free_block))
		{
		  SASStringBTreeFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SASStringBTreeFreeNoLock(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASStringBTreeFreeNoLock(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeFreeNoLock(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

void
SASStringBTreeFree (SASStringBTree_t heap, SASStringBTreeNode_t free_block)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
				  SAS_RUNTIME_STRINGBTREENODE))
    {
      SASLock (heap, SasUserLock__WRITE);
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_STRINGBTREE))
	{
	  if (SASStringBTreeIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASStringBTreeHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASStringBTreeContains (expandBlock, free_block))
		    {
		      SASStringBTreeFreeInternal (expandBlock, free_block);
		      if (i > 0)
			SASUnlock (heap);
		      break;
		    }
		  if (i > 0)
		    SASUnlock (heap);
		}
	    }
	  else
	    {
	      if (SASStringBTreeContains (headerBlock, free_block))
		{
		  SASStringBTreeFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SASStringBTreeFree(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASStringBTreeFree(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeFree(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

SASStringBTreeNode_t
SASStringBTreeNearAllocNoLock (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASStringBTreeHeader *compoundHeader = NULL;
  SASStringBTreeHeader *baseHeader = NULL;
  SASStringBTreeNode_t newHeap = NULL;
  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASStringBTreeHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASStringBTreeHeader *) nearHeader;

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_STRINGBTREE))
	    {
	      if (SASStringBTreeIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASStringBTreeHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}

	      if (SASStringBTreeAvail (compoundHeader))
		newHeap = SASStringBTreeAllocInternal (compoundHeader);
	      else
		newHeap =
		  SASStringBTreeAllocNoLock ((SASStringBTree_t) baseHeader);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASStringBTreeNearAllocNoLock(%p)->%p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	}
    }
  return newHeap;
}

SASStringBTreeNode_t
SASStringBTreeNearAlloc (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASStringBTreeHeader *compoundHeader = NULL;
  SASStringBTreeHeader *baseHeader = NULL;
  SASStringBTreeNode_t newHeap = NULL;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASStringBTreeHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASStringBTreeHeader *) nearHeader;

	  SASLock (compoundHeader, SasUserLock__WRITE);

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_STRINGBTREE))
	    {
	      if (SASStringBTreeIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASStringBTreeHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}
#if 0
	      sas_printf ("SASStringBTreeNearAlloc(%p) %p %p\n",
			  nearObj, compoundHeader, baseHeader);
#endif
	      if (SASStringBTreeAvail (compoundHeader))
		{
		  newHeap = SASStringBTreeAllocInternal (compoundHeader);
		}
	      else
		{
		  newHeap =
		    SASStringBTreeAlloc ((SASStringBTree_t) baseHeader);
		}
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASStringBTreeNearAlloc(%p)->%p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	  SASUnlock (compoundHeader);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeNearAlloc(%p) block check failed\n",
		  nearObj);
#endif
    }
  return newHeap;
}

/* This section handles allocation of spill heaps and adds them to the 
 * spillList.  The preference is to allocate a spill heap within the same
 * compoundHead as the near object.  If space is not available in the near
 * compoundHeap it will allocate from another compoundHeap in the expanded
 * list.  If the local spill list is full or the a spill heap can not be
 * allocated from the expanded list, return a NULL.  */

static SASStringBTreeNode_t
SASStringBTreeSpillInternal (SASStringBTreeHeader * headerBlock)
{
  SASBlockHeader *simpleBlock;
  block_size_t heapSize;
  block_size_t simpleSize;
  freeNode *mem = NULL;
  SASStringBTreeNode_t newHeap = NULL;

  heapSize = headerBlock->blockHeader.blockSize;
  simpleSize = headerBlock->pageSize;
  if (simpleSize < heapSize)
    {
      mem = freeNode_allocSpace (headerBlock->blockHeader.blockFreeSpace,
				 &headerBlock->blockHeader.blockFreeSpace,
				 simpleSize);
      if (mem != NULL)
	{
	  simpleBlock = (SASBlockHeader *) mem;
	  newHeap = SASStringBTreeSpillInit (mem, SAS_RUNTIME_STRINGBTREENODE,
					     simpleSize);
	  simpleBlock->baseBlock = (SASBlockHeader *) headerBlock;
	}
    }
  return newHeap;
}

SASStringBTreeNode_t
SASStringBTreeSpillAllocExtended (SASStringBTree_t heap, lock_on_t lock_on)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASStringBTreeNode_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_STRINGBTREE))
    {
      SASStringBTreeHeader *heapHeader = (SASStringBTreeHeader *) heap;
      if (lock_on) SASLock (heap, SasUserLock__WRITE);
      if (SASStringBTreeIsExpanding (heapHeader))
	{
	  SASCompoundExpandList *list = heapHeader->expandList;
	  SASStringBTreeHeader *lastHeader;
	  SASStringBTreeHeader *expandHeader = NULL;
	  block_size_t i;
	  block_size_t last_lock = 0;
	  lastHeader = list->heap[list->count - 1];
	  if (lastHeader != heapHeader)
	    if (lock_on) SASLock (lastHeader, SasUserLock__WRITE);

	  if (SASStringBTreePercentUsed (lastHeader) >= DEFAULT_LOAD_FACTOR)
	    {
	      last_lock = list->count - 2;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASStringBTreeHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    if (lock_on) SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASStringBTreePercentUsed (expandBlock)
		      < DEFAULT_LOAD_FACTOR)
		    {
		      expandHeader = expandBlock;
		      last_lock = i;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASStringBTreeHeader *)
		    SASStringBTreeExpandCreate (heap);
		}
	    }
	  else
	    {
	      expandHeader = lastHeader;
	    }
	  if (expandHeader != NULL)
	    newHeap = SASStringBTreeSpillInternal (expandHeader);

	  if (lastHeader != expandHeader)
	    {
	      for (i = 1; i <= last_lock; i++)
		{
		  if (lock_on) SASUnlock (list->heap[i]);
		}
	    }
	  if (lastHeader != heapHeader)
	    if (lock_on) SASUnlock (lastHeader);
	}
      else
	{
	  newHeap = SASStringBTreeSpillInternal (heapHeader);
	}
      if (lock_on) SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeSpillAllocExtended(%p) type check failed\n",
		  heap);
#endif
    }
  return newHeap;
}

SASStringBTreeNode_t
SASStringBTreeSpillAlloc (void *nearObj, lock_on_t lock_on)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASStringBTreeHeader *compoundHeader = NULL;
  SASStringBTreeHeader *baseHeader = NULL;
  SASStringBTreeNode_t newHeap = NULL;
  SASStringBTreeSpillList *spill_lst;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASStringBTreeHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASStringBTreeHeader *) nearHeader;

	  if (lock_on) SASLock (compoundHeader, SasUserLock__WRITE);

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_STRINGBTREE))
	    {
	      if (SASStringBTreeIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASStringBTreeHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}
#if 0
	      sas_printf ("SASStringBTreeSpillAlloc(%p) %p %p\n",
			  nearObj, compoundHeader, baseHeader);
#endif
	      spill_lst = compoundHeader->spillList;
	      if (lock_on) SASLock (spill_lst, SasUserLock__WRITE);
	      if (spill_lst->count < spill_lst->max_count)
		{
		  if (SASStringBTreeAvail (compoundHeader))
		    {
		      newHeap = SASStringBTreeSpillInternal (compoundHeader);
		    }
		  else
		    {
		      newHeap =
			SASStringBTreeSpillAllocExtended ((SASStringBTree_t)
							  baseHeader, lock_on);
		    }
		  if (newHeap != NULL)
		    {
		      spill_lst->spillHeap[spill_lst->count] =
			(SASStringBTreeNodeHeader *) newHeap;
		      spill_lst->count++;
		    }
		}
	      if (lock_on) SASUnlock (spill_lst);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASStringBTreeSpillAlloc(%p)->%p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	  if (lock_on) SASUnlock (compoundHeader);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeSpillAlloc(%p) block check failed\n",
		  nearObj);
#endif
    }
  return newHeap;
}

void
SASStringBTreeNearDeallocNoLock (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASStringBTree_t btreeHeader = NULL;
  if (nearHeader != NULL)
    {
      if ((nearHeader->baseBlock != nearHeader)
	  && (nearHeader->baseBlock != NULL))
	btreeHeader = (SASStringBTree_t) nearHeader->baseBlock;
      else
	btreeHeader = (SASStringBTree_t) nearHeader;

      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) btreeHeader,
				      SAS_RUNTIME_STRINGBTREE))
	{
	  SASStringBTreeFreeInternal ((SASStringBTreeHeader *) btreeHeader,
				      nearHeader);
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASStringBTreeNearDeallocNoLock(%p) type check failed near=%p compound=%p\n",
	     nearObj, nearHeader, btreeHeader);
#endif
	}
    }
}

void
SASStringBTreeNearDealloc (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASStringBTree_t btreeHeader = NULL;

  if (nearHeader != NULL && SOMSASCheckBlockSig (nearHeader))
    {
      if ((nearHeader->baseBlock != nearHeader)
	  && (nearHeader->baseBlock != NULL))
	btreeHeader = (SASStringBTree_t) nearHeader->baseBlock;
      else
	btreeHeader = (SASStringBTree_t) nearHeader;

      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) btreeHeader,
				      SAS_RUNTIME_STRINGBTREE))
	{
	  SASLock (btreeHeader, SasUserLock__WRITE);
	  SASStringBTreeFreeInternal ((SASStringBTreeHeader *) btreeHeader,
				      nearHeader);
	  SASUnlock (btreeHeader);
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASStringBTreeNearDealloc(%p) type check failed near=%p compound=%p\n",
	     nearObj, nearHeader, btreeHeader);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeNearDealloc(%p) block check failed\n",
		  nearObj);
#endif
    }
}

block_size_t
SASStringBTreeAllocSize (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  block_size_t nodeSize = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_STRINGBTREE))
    {
      nodeSize = headerBlock->pageSize;
    }
  return nodeSize;
}

block_size_t
SASStringBTreeFreeSpaceNoLock (SASStringBTree_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_STRINGBTREE))
    {
      SASStringBTreeHeader *heapBlock = (SASStringBTreeHeader *) heap;
      SASCompoundExpandList *list = heapBlock->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  for (i = 0; i < list->count; i++)
	    {
	      SASBlockHeader *expandBlock = &list->heap[i]->blockHeader;
	      if (expandBlock->blockFreeSpace != NULL)
		heapFree +=
		  freeNode_freeSpaceTotal (expandBlock->blockFreeSpace);
	    }
	}
      else
	{
	  if (headerBlock->blockFreeSpace != NULL)
	    heapFree = freeNode_freeSpaceTotal (headerBlock->blockFreeSpace);
	}
    }
  return heapFree;
}

block_size_t
SASStringBTreeFreeSpace (SASStringBTree_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);
      SASCompoundExpandList *list =
	((SASStringBTreeHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  for (i = 1; i < list->count; i++)
	    {
	      SASLock (list->heap[i], SasUserLock__WRITE);
	    }

	  heapFree = SASStringBTreeFreeSpaceNoLock (heap);

	  for (i = 1; i < list->count; i++)
	    {
	      SASUnlock (list->heap[i]);
	    }
	}
      else
	{
	  heapFree = SASStringBTreeFreeSpaceNoLock (heap);
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeFreeSpace(%p) type check failed\n", heap);
#endif
    }
  return heapFree;
}

void
SASStringBTreeDestroyNoLock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  block_size_t heapSize;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASCompoundExpandList *list = headerBlock->expandList;
      block_size_t i;
      heapSize = headerBlock->blockHeader.blockSize;
      if (list != NULL)
	{
	  for (i = 1; i < list->count; i++)
	    {
	      SASBlockDealloc (list->heap[i], heapSize);
	      list->heap[i] = NULL;
	    }
	  list->max_count = 1;
	}
      SASBlockDealloc (heap, heapSize);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeDestroyNoLock(%p) type check failed\n",
		  heap);
#endif
    }
}

void
SASStringBTreeDestroy (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  block_size_t heapSize;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);
      SASCompoundExpandList *list = headerBlock->expandList;
      block_size_t i;
      heapSize = headerBlock->blockHeader.blockSize;
      if (list != NULL)
	{
	  for (i = 1; i < list->count; i++)
	    {
	      SASLock (list->heap[i], SasUserLock__WRITE);
	    }
	  for (i = 1; i < list->count; i++)
	    {
	      SASBlockDealloc (list->heap[i], heapSize);
	    }
	  for (i = 1; i < list->count; i++)
	    {
	      SASUnlock (list->heap[i]);
	      list->heap[i] = NULL;
	    }
	  list->max_count = 1;
	}
      SASBlockDealloc (headerBlock, heapSize);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeDestroy(%p) block check failed\n", heap);
#endif
    }
}

/******************************************************************/

SASStringBTreeNode_t
SASStringBTreeGetRootNode (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SASStringBTreeNode_t result = NULL;

  if (SOMSASCheckBlockSigAndType
      ((SASBlockHeader *) heap, SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = btree->root;
      SASUnlock (heap);
    }
  return result;
}

SASStringBTreeNode_t
SASStringBTreeGetRootNodeNoLock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SASStringBTreeNode_t result = NULL;

  if (SOMSASCheckBlockSigAndType
      ((SASBlockHeader *) heap, SAS_RUNTIME_STRINGBTREE))
    {
      result = btree->root;
    }
  return result;
}

long
SASStringBTreeGetModCount (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  long result = 0L;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = btree->common->modCount;
      SASUnlock (heap);
    }
  return result;
}

long
SASStringBTreeGetModCount_nolock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  long result = 0L;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      result = btree->common->modCount;
    }
  return result;
}

char *
SASStringBTreeGetMaxKey (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = btree->common->max_key;
      SASUnlock (heap);
    }
  return result;
}

char *
SASStringBTreeGetMaxKey_nolock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      result = btree->common->max_key;
    }
  return result;
}

char *
SASStringBTreeGetMinKey (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = btree->common->min_key;
      SASUnlock (heap);
    }
  return result;
};

char *
SASStringBTreeGetMinKey_nolock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      result = btree->common->min_key;
    }
  return result;
};

int
SASStringBTreeContainsKey (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SBTnodePosRef ref = { NULL, 0 };
  int found = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      if (btree->root != NULL)
	{
	  found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	}
      SASUnlock (heap);
    }
  return found;
}

int
SASStringBTreeContainsKey_nolock (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SBTnodePosRef ref = { NULL, 0 };
  int found = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      if (btree->root != NULL)
        {
          found = SASStringBTreeNodeSearch (btree->root, key, &ref);
        }
    }
  return found;
}

void *
SASStringBTreeGet (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  void *result = NULL;
  SBTnodePosRef ref = { NULL, 0 };
  int found;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      if (btree->root != NULL)
	{
	  found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	  if (found)
	    {
	      result = SASStringBTreeNodeGetValIndexed (ref.node, ref.pos);
	    }
	}
      SASUnlock (heap);
    }
  return result;
}

void *
SASStringBTreeGet_nolock (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  void *result = NULL;
  SBTnodePosRef ref = { NULL, 0 };
  int found;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      if (btree->root != NULL)
        {
          found = SASStringBTreeNodeSearch (btree->root, key, &ref);
          if (found)
            {
              result = SASStringBTreeNodeGetValIndexed (ref.node, ref.pos);
            }
        }
    }
  return result;
}

int
SASStringBTreeIsEmpty (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  int result = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = (btree->common->count == 0);
      SASUnlock (heap);
    }
  return result;
}

int
SASStringBTreeIsEmpty_nolock (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  int result = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      result = (btree->common->count == 0);
    }
  return result;
}

long
SASStringBTreeGetCurCount (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  long result = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      result = btree->common->count;
      SASUnlock (heap);
    }
  return result;
}

static inline void
SASStringBTreeHeaderDealloc (SASStringBTree_t heap,
			     void *memAddr, block_size_t size)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;

  freeNode_deallocSpace (((freeNode *) memAddr), &btree->headerFreeSpace,
			 size);
}

static inline void *
SASStringBTreeHeaderAlloc (SASStringBTree_t heap, block_size_t size)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  void *result = NULL;

  if (btree->headerFreeSpace)
    {
      result =
	freeNode_allocSpace (btree->headerFreeSpace, &btree->headerFreeSpace,
			     size);
    }
  return result;
}

static inline void
SASStringBTreeUpdateMax (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *oldkey = btree->common->max_key;
  char *tempkey;
  int key_len;

  if (key != NULL)
    {
      key_len = strlen (key) + 1;
      tempkey = (char *) SASStringBTreeHeaderAlloc (heap, key_len);
      btree->common->max_key = strcpy (tempkey, key);
    }
  else
    {
      btree->common->max_key = NULL;
    }

  if (oldkey != NULL)
    {
      key_len = strlen (oldkey) + 1;
      SASStringBTreeHeaderDealloc (heap, oldkey, key_len);
    }
}

static inline void
SASStringBTreeUpdateMin (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  char *oldkey = btree->common->min_key;
  char *tempkey;
  int key_len;

  if (key != NULL)
    {
      key_len = strlen (key) + 1;
      tempkey = (char *) SASStringBTreeHeaderAlloc (heap, key_len);
      btree->common->min_key = strcpy (tempkey, key);
    }
  else
    {
      btree->common->min_key = NULL;
    }

  if (oldkey != NULL)
    {
      key_len = strlen (oldkey) + 1;
      SASStringBTreeHeaderDealloc (heap, oldkey, key_len);
    }
}

int
SASStringBTreePut (SASStringBTree_t heap, char *key, void *value)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  int result = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);

      if (btree->root != NULL)
	{
	  SASStringBTreeNode_t node;
	  node = SASStringBTreeNodeInsert (btree->root, key, value, LOCK_OFF);
	  if (node != NULL)
	    {
	      btree->root = node;
	      btree->common->modCount++;
	      if (strcmp (key, btree->common->min_key) < 0)
		SASStringBTreeUpdateMin (heap, key);
	      if (strcmp (key, btree->common->max_key) > 0)
		SASStringBTreeUpdateMax (heap, key);
	      result = true;
	    }
	}
      else
	{
	  btree->root = SASStringBTreeAlloc (heap);
	  SASStringBTreeNodeInitialize (btree->root, key, value, LOCK_OFF);
	  SASStringBTreeUpdateMin (heap, key);
	  SASStringBTreeUpdateMax (heap, key);
	  btree->common->modCount++;
	  result = true;
	};
      btree->common->count++;
      SASUnlock (heap);
    }
  return result;		/* False indicates duplicate key */
}

int
SASStringBTreePut_nolock (SASStringBTree_t heap, char *key, void *value)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  int result = false;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      if (btree->root != NULL)
        {
          SASStringBTreeNode_t node;
          node = SASStringBTreeNodeInsert (btree->root, key, value, LOCK_OFF);
          if (node != NULL)
            {
              btree->root = node;
              btree->common->modCount++;
              if (strcmp (key, btree->common->min_key) < 0)
                SASStringBTreeUpdateMin (heap, key);
              if (strcmp (key, btree->common->max_key) > 0)
                SASStringBTreeUpdateMax (heap, key);
              result = true;
            }
        }
      else
        {
          btree->root = SASStringBTreeAllocNoLock (heap);
          SASStringBTreeNodeInitialize (btree->root, key, value, LOCK_OFF);
          SASStringBTreeUpdateMin (heap, key);
          SASStringBTreeUpdateMax (heap, key);
          btree->common->modCount++;
          result = true;
        };
      btree->common->count++;
    }
  return result;                /* False indicates duplicate key */
}

void *
SASStringBTreeReplace (SASStringBTree_t heap, char *key, void *value)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SBTnodePosRef ref = { NULL, 0 };
  void *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);
      btree->common->modCount++;

      if (btree->root != NULL)
	{
	  int found;
	  found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	  //found = getNode(key, &ref);
	  if (found)
	    {
	      result =
		SASStringBTreeNodePutValIndexed (ref.node, ref.pos, value);
	    }
	}
      btree->common->count++;
      SASUnlock (heap);
    }
  return result;		// return prev value if already exist
}

void *
SASStringBTreeReplace_nolock (SASStringBTree_t heap, char *key, void *value)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SBTnodePosRef ref = { NULL, 0 };
  void *result = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      btree->common->modCount++;

      if (btree->root != NULL)
        {
          int found;
          found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	  //found = getNode(key, &ref);
          if (found)
            {
              result =
                SASStringBTreeNodePutValIndexed (ref.node, ref.pos, value);
            }
        }
      btree->common->count++;
    }
  return result;                // return prev value if already exist
}

void *
SASStringBTreeRemove (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SASStringBTreeNode_t newRoot;
  SBTnodePosRef ref = { NULL, 0 };
  void *result = NULL;
  SASStringBTreeNodeHeader *node;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);
      btree->common->modCount++;

      if (btree->root != NULL)
	{
	  int found;
	  found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	  //found = getNode(key, &ref);
	  if (found)
	    {
	      result = SASStringBTreeNodeGetValIndexed (ref.node, ref.pos);
	    }

	  newRoot = SASStringBTreeNodeDelete (btree->root, key, LOCK_OFF);
	  if (newRoot != btree->root)
	    {			//Delete the old root which is empty
	      SASStringBTreeNearDealloc (btree->root);
	      btree->root = newRoot;
	    }
	  if (btree->root != NULL)
	    {
	      btree->common->count--;
              if (btree->common->count > 0)
                {
                  if(strcmp(key, btree->common->min_key) == 0)
                    {
                       node = (SASStringBTreeNodeHeader *) btree->root;
                       if (node->branch[0] != NULL)
                          node = node->branch[0];
                       SASStringBTreeUpdateMin(heap, node->keys[1]);
                    }
                  if(strcmp(key, btree->common->max_key) == 0)
                    {
                       node = (SASStringBTreeNodeHeader *) btree->root;
                       if (node->branch[node->count] != NULL)
                          node = node->branch[node->count];
                       SASStringBTreeUpdateMax(heap, node->keys[(node->count)]);
                    }
                }
	    }
	  else
	    {
	      btree->common->count = 0;
	      SASStringBTreeUpdateMax (heap, NULL);
	      SASStringBTreeUpdateMin (heap, NULL);
	    }
	}
      else
	{
	  btree->common->count = 0;
	}
      SASUnlock (heap);
    }
  return result;		// return prev value if already exist
}

void *
SASStringBTreeRemove_nolock (SASStringBTree_t heap, char *key)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;
  SASStringBTreeNode_t newRoot;
  SBTnodePosRef ref = { NULL, 0 };
  void *result = NULL;
  SASStringBTreeNodeHeader *node;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
                                  SAS_RUNTIME_STRINGBTREE))
    {
      btree->common->modCount++;

      if (btree->root != NULL)
        {
          int found;
          found = SASStringBTreeNodeSearch (btree->root, key, &ref);
	  //found = getNode(key, &ref);
          if (found)
            {
              result = SASStringBTreeNodeGetValIndexed (ref.node, ref.pos);
            }

          newRoot = SASStringBTreeNodeDelete (btree->root, key, LOCK_OFF);
          if (newRoot != btree->root)
	    {			//Delete the old root which is empty
              SASStringBTreeNearDeallocNoLock (btree->root);
              btree->root = newRoot;
            }
          if (btree->root != NULL)
            {
              btree->common->count--;
              if (btree->common->count > 0)
                {
                  if(strcmp(key, btree->common->min_key) == 0)
                    {
                       node = (SASStringBTreeNodeHeader *) btree->root;
                       if (node->branch[0] != NULL)
                          node = node->branch[0];
                       SASStringBTreeUpdateMin(heap, node->keys[1]);
                    }
                  if(strcmp(key, btree->common->max_key) == 0)
                    {
                       node = (SASStringBTreeNodeHeader *) btree->root;
                       if (node->branch[node->count] != NULL)
                          node = node->branch[node->count];
                       SASStringBTreeUpdateMax(heap, node->keys[(node->count)]);
                    }
                }
            }
          else
            {
              btree->common->count = 0;
              SASStringBTreeUpdateMax (heap, NULL);
              SASStringBTreeUpdateMin (heap, NULL);
            }
        }
      else
        {
          btree->common->count = 0;
        }
    }
  return result;                // return prev value if already exist
}


#if 0
void
SASStringBTreePrint (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      if (btree->root != NULL)
	SASStringBTreeNodePrint (btree->root);
      else
	sas_printf ("<empty>");

      SASUnlock (heap);
    }
  sas_printf ("\n");
};

void
SASStringBTreePrintValues (SASStringBTree_t heap)
{
  SASStringBTreeHeader *btree = (SASStringBTreeHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) heap,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__READ);
      if (btree->root != NULL)
	SASStringBTreeNodePrintValue (btree->root);
      else
	sas_printf ("<empty>");

      SASUnlock (heap);
    }
  sas_printf ("\n");
}

void
SASStringBTreeNodePrintStatsPriv (SASStringBTreeNode_t heap)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  SASStringBTreeNode_t spill_t;
  char *key_str;
  size_t key_len;
  char *ptr;
  char *end_ptr;
  short c;
  short near_count, far_count;
  int near_sum, far_sum;
  size_t max_frag = SASStringBTreeNodeMaxFragmentNoLock (heap);

  sas_printf ("  node@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
	      heap,
	      SASStringBTreeNodeFreeSpaceNoLock (heap),
	      SASStringBTreeNodeFreeFragmentsNoLock (heap),
	      SASStringBTreeNodeMaxFragmentNoLock (heap));


  ptr = (char *) node;
  end_ptr = ptr + node->blockHeader.blockSize;
  near_count = 0;
  far_count = 0;
  near_sum = 0;
  far_sum = 0;
  for (c = 1; c <= node->count; c++)
    {
      key_str = node->keys[c];
      key_len = strlen (key_str) + 1;
      if (((unsigned long) key_str >= (unsigned long) ptr)
	  && ((unsigned long) key_str < (unsigned long) end_ptr))
	{
	  near_count++;
	  near_sum += key_len;
	}
      else
	{
	  if (key_len < max_frag)
	    {
	      sas_printf ("  node@%p[%hd], far=%s\n", heap, c, key_str);
	    }
	  far_count++;
	  far_sum += key_len;
	}
    }

  sas_printf ("   %d keys: %d near total %d,  %d far total %d\n",
	      node->count, near_count, near_sum, far_count, far_sum);

  if (node->spill != NULL)
    {
      spill_t = (SASStringBTreeNode_t) node->spill;

      sas_printf
	("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
	 spill_t, SASStringBTreeNodeFreeSpaceNoLock (spill_t),
	 SASStringBTreeNodeFreeFragmentsNoLock (spill_t),
	 SASStringBTreeNodeMaxFragmentNoLock (spill_t));
    }
  if (node->spill2 != NULL)
    {
      spill_t = (SASStringBTreeNode_t) node->spill2;

      sas_printf
	("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
	 spill_t, SASStringBTreeNodeFreeSpaceNoLock (spill_t),
	 SASStringBTreeNodeFreeFragmentsNoLock (spill_t),
	 SASStringBTreeNodeMaxFragmentNoLock (spill_t));
    }
  if (node->spill3 != NULL)
    {
      spill_t = (SASStringBTreeNode_t) node->spill3;

      sas_printf
	("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
	 spill_t, SASStringBTreeNodeFreeSpaceNoLock (spill_t),
	 SASStringBTreeNodeFreeFragmentsNoLock (spill_t),
	 SASStringBTreeNodeMaxFragmentNoLock (spill_t));
    }
}

#ifndef nodeAlign
#define nodeAlign (2*sizeof(void*))
#define nodeRound (2*sizeof(void*)-1)
#endif

void
SASStringBTreeNodeStatsPriv (SASStringBTreeNode_t heap,
			     long *key_count,
			     long *key_total,
			     long *near_key_count,
			     long *near_key_total,
			     long *far_key_count,
			     long *far_key_total,
			     long *fragment_count, long *free_total)
{
  SASStringBTreeNodeHeader *node = (SASStringBTreeNodeHeader *) heap;
  char *key_str;
  int key_len;
  char *ptr;
  char *end_ptr;
  short c;

  *fragment_count += SASStringBTreeNodeFreeFragmentsNoLock (heap);
  *free_total += SASStringBTreeNodeFreeSpaceNoLock (heap);

  ptr = (char *) node;
  end_ptr = ptr + node->blockHeader.blockSize;
  *key_count += node->count;

  for (c = 1; c <= node->count; c++)
    {
      key_str = node->keys[c];
      key_len = strlen (key_str) + 1;
      key_len = ((key_len + nodeRound) / nodeAlign) * nodeAlign;
      *key_total += key_len;
      if (((unsigned long) key_str >= (unsigned long) ptr)
	  && ((unsigned long) key_str < (unsigned long) end_ptr))
	{
	  *near_key_count += 1;
	  *near_key_total += key_len;
	}
      else
	{
	  *far_key_count += 1;
	  *far_key_total += key_len;
	}
    }
}

void
SASStringBTreePrintStatsPriv (SASStringBTreeHeader * compoundHeader)
{
  SASStringBTreeSpillList *spill_lst;
  char *ptr;
  char *end_ptr;
  block_size_t psize;
  long key_count = 0;
  long key_total = 0;
  long near_key_count = 0;
  long near_key_total = 0;
  long far_key_count = 0;
  long far_key_total = 0;
  long fragment_count = 0;
  long free_total = 0;

  sas_printf ("SASStringBTreePrintStats(%p) freespace=%zu\n",
	      compoundHeader, SASStringBTreeFreeSpace (compoundHeader));
  ptr = (char *) compoundHeader;
  end_ptr = ptr + compoundHeader->blockHeader.blockSize;
  ptr = ptr + default_page;
  psize = compoundHeader->pageSize;
  sas_printf (" compound heap %p - %p + %zu\n", ptr, end_ptr, psize);

  spill_lst = compoundHeader->spillList;
  for (int i = 0; i < spill_lst->count; i++)
    {
      const char *near;
      SASStringBTreeNode_t spill_t;
      spill_t = SASStringBTreeNodeVerify (spill_lst->spillHeap[i]);

      if (((unsigned long) spill_t >= (unsigned long) ptr)
	  && ((unsigned long) spill_t < (unsigned long) end_ptr))
	near = "near";
      else
	near = "far";

      sas_printf
	("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu %s\n",
	 spill_t, SASStringBTreeNodeFreeSpaceNoLock (spill_t),
	 SASStringBTreeNodeFreeFragmentsNoLock (spill_t),
	 SASStringBTreeNodeMaxFragmentNoLock (spill_t), near);
    }

  for (; (unsigned long) ptr < (unsigned long) end_ptr; ptr += psize)
    {
      SASStringBTreeNode_t heap;
      heap = SASStringBTreeNodeVerify ((SASStringBTreeNode_t) ptr);

      if (heap != NULL)
	{
	  if (!SASStringBTreeNodeIsSpill (heap))
	    {
	      SASStringBTreeNodePrintStatsPriv (heap);
	      SASStringBTreeNodeStatsPriv (heap,
					   &key_count,
					   &key_total,
					   &near_key_count,
					   &near_key_total,
					   &far_key_count,
					   &far_key_total,
					   &fragment_count, &free_total);
	    }
	  else
	    {
	      /*
	         spill_t = (SASStringBTreeNode_t)node;

	         sas_printf("  spill@%p, freespace=%d, fragments=%d, max_free_block=%d\n",
	         spill_t, 
	         SASStringBTreeNodeFreeSpaceNoLock(spill_t),
	         SASStringBTreeNodeFreeFragmentsNoLock(spill_t),
	         SASStringBTreeNodeMaxFragmentNoLock(spill_t));
	       */
	    }
	}
    }

  sas_printf
    ("  totals keys=%ld/%ld, near=%ld/%ld, far=%ld/%ld, free=%ld/%ld\n",
     key_count, key_total, near_key_count, near_key_total, far_key_count,
     far_key_total, fragment_count, free_total);
}

void
SASStringBTreeStatsPriv (SASStringBTreeHeader * compoundHeader,
			 long *key_count,
			 long *key_total,
			 long *near_key_count,
			 long *near_key_total,
			 long *far_key_count,
			 long *far_key_total,
			 long *fragment_count,
			 long *free_total,
			 long *spill_near_count,
			 long *spill_far_count, long *spill_free_total)
{
  SASStringBTreeSpillList *spill_lst;
  char *ptr;
  char *end_ptr;
  block_size_t psize;
  /*
     sas_printf("SASStringBTreePrintStats(%p) freespace=%d\n",
     compoundHeader,
     SASStringBTreeFreeSpace(compoundHeader));
   */
  ptr = (char *) compoundHeader;
  end_ptr = ptr + compoundHeader->blockHeader.blockSize;
  ptr = ptr + default_page;
  psize = compoundHeader->pageSize;

  spill_lst = compoundHeader->spillList;
  for (int i = 0; i < spill_lst->count; i++)
    {
      SASStringBTreeNode_t spill_t;
      spill_t = SASStringBTreeNodeVerify (spill_lst->spillHeap[i]);

      if (((unsigned long) spill_t >= (unsigned long) ptr)
	  && ((unsigned long) spill_t < (unsigned long) end_ptr))
	*spill_near_count += 1;
      else
	*spill_far_count += 1;

      *spill_free_total += SASStringBTreeNodeFreeSpaceNoLock (spill_t);
    }
  for (; (unsigned long) ptr < (unsigned long) end_ptr; ptr += psize)
    {
      SASStringBTreeNode_t heap;
      heap = SASStringBTreeNodeVerify ((SASStringBTreeNode_t) ptr);

      if (heap != NULL)
	{
	  if (!SASStringBTreeNodeIsSpill (heap))
	    {
	      SASStringBTreeNodeStatsPriv (heap,
					   key_count,
					   key_total,
					   near_key_count,
					   near_key_total,
					   far_key_count,
					   far_key_total,
					   fragment_count, free_total);
	    }
	}
    }
}

void
SASStringBTreePrintStats (SASStringBTree_t heap)
{
  SASStringBTreeHeader *headerBlock = (SASStringBTreeHeader *) heap;
  long key_count = 0;
  long key_total = 0;
  long near_key_count = 0;
  long near_key_total = 0;
  long far_key_count = 0;
  long far_key_total = 0;
  long fragment_count = 0;
  long free_total = 0;
  long spill_near_count = 0;
  long spill_far_count = 0;
  long spill_free_total = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_STRINGBTREE))
    {
      SASLock (heap, SasUserLock__WRITE);
      SASCompoundExpandList *list = headerBlock->expandList;
      block_size_t i;
      SASStringBTreePrintStatsPriv (headerBlock);
      if (list != NULL)
	{
	  for (i = 1; i < list->count; i++)
	    {
	      SASLock (list->heap[i], SasUserLock__WRITE);
	    }
	  for (i = 1; i < list->count; i++)
	    {
	      SASStringBTreePrintStatsPriv (list->heap[i]);
	      SASStringBTreeStatsPriv (list->heap[i],
				       &key_count,
				       &key_total,
				       &near_key_count,
				       &near_key_total,
				       &far_key_count,
				       &far_key_total,
				       &fragment_count,
				       &free_total,
				       &spill_near_count,
				       &spill_far_count, &spill_free_total);
	    }
	  for (i = 1; i < list->count; i++)
	    {
	      SASUnlock (list->heap[i]);
	    }
	  list->max_count = 1;
	}
      SASUnlock (heap);
      sas_printf
	("totals keys=%ld/%ld, near=%ld/%ld, far=%ld/%ld,\n       free=%ld/%ld, spill=%ld/%ld/%ld\n",
	 key_count, key_total, near_key_count, near_key_total, far_key_count,
	 far_key_total, fragment_count, free_total, spill_near_count,
	 spill_far_count, spill_free_total);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreePrintStats(%p) block check failed\n", heap);
#endif
    }
}
#endif
