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
#include <string.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sasmsync.h"
#include "sassimplespace.h"

#include "sphsinglepcqueue.h"
#include "sphcompoundpcqheap.h"
#include "sascompoundheap.h"

struct SASCompoundHeapHeader;

#define DEFAULT_LOAD_FACTOR 75

typedef struct SASCompoundExpandList
{
  block_size_t count;
  block_size_t max_count;
  SASCompoundHeapHeader *heap[254];
} SASCompoundExpandList;

typedef struct SASCompoundHeapHeader
{
  SASBlockHeader blockHeader;
  block_size_t pageSize;
  long loadFactor;
  void *dummy2;
  void *dummy3;
  void *dummy4;
  SASSimpleSpace_t expandSpace;
  SASCompoundExpandList *expandList;
  freeNode *headerFreeSpace;
} SASCompoundHeapHeader;

static inline int
SASCompoundHeapAvail (SASCompoundHeapHeader * headerBlock)
{
  return (headerBlock->blockHeader.blockFreeSpace != NULL);
}

static inline int
SASCompoundHeapPercentFree (SASCompoundHeapHeader * heapBlock)
{
  int percent = 0;
  block_size_t heapFree = 0;

  if (heapBlock->blockHeader.blockFreeSpace != NULL)
    {
      heapFree =
	freeNode_freeSpaceTotal (heapBlock->blockHeader.blockFreeSpace);
      percent = (int) ((heapFree * 100) / heapBlock->blockHeader.blockSize);
    }
#if 0
  sas_printf ("SASCompoundHeapPercentFree(%p) = %d for (%ld, %ld)\n",
	      heapBlock, percent, heapFree, heapUsed);
#endif
  return percent;
}

static inline int
SASCompoundHeapPercentUsed (SASCompoundHeapHeader * heapBlock)
{
  int percent = 100;
  block_size_t heapFree = 0;
  block_size_t heapUsed = 0;

  if (heapBlock->blockHeader.blockFreeSpace != NULL)
    {
      heapFree =
	freeNode_freeSpaceTotal (heapBlock->blockHeader.blockFreeSpace);
      heapUsed = heapBlock->blockHeader.blockSize - heapFree;
      percent = (int) ((heapUsed * 100) / heapBlock->blockHeader.blockSize);
    }
#if 0
  sas_printf ("SASCompoundHeapPercentFree(%p) = %d for (%ld, %ld)\n",
	      heapBlock, percent, heapFree, heapUsed);
#endif
  return percent;
}

static inline int
SASCompoundHeapIsExpanding (SASCompoundHeapHeader * headerBlock)
{
  return (headerBlock->expandList != NULL);
}

static inline int
SASCompoundHeapContains (SASCompoundHeapHeader * headerBlock,
			 SASSimpleHeap_t simpleHeap)
{
  block_size_t containedHeap = (block_size_t) simpleHeap;
  block_size_t containerLow = (block_size_t) headerBlock;
  block_size_t containerHigh = (block_size_t) headerBlock +
    headerBlock->blockHeader.blockSize;
  return ((containedHeap > containerLow) && (containedHeap < containerHigh));
}

static inline void
SASCompoundHeapFreeInternal (SASCompoundHeapHeader * headerBlock,
			     SASSimpleHeap_t free_block)
{
  freeNode *free_node = (freeNode *) free_block;
  block_size_t simpleSize = headerBlock->pageSize;

  memset (free_block, 0, simpleSize);
  freeNode_init (free_node, simpleSize);
  freeNode_deallocSpace (free_node, &headerBlock->blockHeader.blockFreeSpace,
			 simpleSize);
}

SASCompoundHeap_t
SASCompoundHeapExpandInit (void *heap_seg,
			   block_size_t heap_size, block_size_t page_size)
{
  SASCompoundHeapHeader *heapBlock = (SASCompoundHeapHeader *) heap_seg;
  char *heapStart = NULL;
  node_size_t remaining;
  block_size_t alloc_page = default_page;

  if ( alloc_page < page_size)
	  alloc_page = page_size;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + alloc_page;
      initSOMSASBlock ((SASBlockHeader *) heapBlock,
		       SAS_RUNTIME_COMPOUNDHEAP, heap_size, heapStart);
    }

  heapBlock->pageSize = page_size;

  remaining = alloc_page - sizeof (SASCompoundHeapHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);

  heapBlock->expandList = NULL;
  heapBlock->loadFactor = DEFAULT_LOAD_FACTOR;

  return (SASCompoundHeap_t) heapBlock;
}

SASCompoundHeap_t
SASCompoundHeapInit (void *heap_seg,
		     block_size_t heap_size,
		     block_size_t page_size, int expanding)
{
  SASCompoundHeapHeader *heapBlock = (SASCompoundHeapHeader *) heap_seg;
  SASCompoundExpandList *list;
  char *heapStart = NULL;
  node_size_t remaining;
  block_size_t alloc_page = default_page;

  if ( alloc_page < page_size)
	  alloc_page = page_size;

  if (heapBlock)
    {
      heapStart = (char *) heapBlock + alloc_page;
      initSOMSASBlock ((SASBlockHeader *) heapBlock,
		       SAS_RUNTIME_COMPOUNDHEAP, heap_size, heapStart);
    }

  heapBlock->pageSize = page_size;

  remaining = alloc_page - sizeof (SASCompoundHeapHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);
  heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
  heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;
  heapBlock->loadFactor = 100;

  if (expanding)
    {
      list =
	(SASCompoundExpandList *) freeNode_allocSpace (heapBlock->
						       headerFreeSpace,
						       &heapBlock->
						       headerFreeSpace,
						       sizeof
						       (SASCompoundExpandList));
      if (list != NULL)
	{
	  heapBlock->expandList = list;
	  heapBlock->loadFactor = DEFAULT_LOAD_FACTOR;
	  list->count = 1;
	  list->max_count = 254;
	  list->heap[0] = heapBlock;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASCompoundHeapInit(%p, %zu, %zu) ExpandList alloc failed\n",
	     heap_seg, heap_size, page_size);
#endif
	}
    }

  return (SASCompoundHeap_t) heapBlock;
}

SASCompoundHeap_t
SASCompoundHeapExpandCreate (SASCompoundHeap_t heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  block_size_t heap_size = headerBlock->blockHeader.blockSize;
  block_size_t page_size = headerBlock->pageSize;
  SASCompoundExpandList *list = headerBlock->expandList;
  SASBlockHeader *heapBlock = NULL;
  SASCompoundHeap_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      if (list->count < list->max_count)
	{
	  SASCompoundHeapHeader *newHeader, *prevHeader;
	  newHeap = SASCompoundHeapExpandInit (heapBlock,
					       heap_size, page_size);
	  newHeader = (SASCompoundHeapHeader *) newHeap;
	  newHeader->blockHeader.baseBlock = &headerBlock->blockHeader;
	  newHeader->blockHeader.nextBlock = &headerBlock->blockHeader;
	  newHeader->loadFactor = headerBlock->loadFactor;
	  prevHeader = list->heap[list->count - 1];
	  list->heap[list->count] = newHeader;
	  list->count++;
	  prevHeader->blockHeader.nextBlock = &newHeader->blockHeader;
	}
      else
	{
	  if (headerBlock->expandSpace == NULL)
	    {
	      SASSimpleSpace_t expandBlock;
	      SASCompoundExpandList *expandNew;
	      long countNew;
	      expandBlock = SASSimpleSpaceCreate (heap_size - default_page);
	      if (expandBlock)
		{
#ifdef __SASDebugPrint__
		  sas_printf
		    ("SASCompoundHeapExpandCreate(%p) extended expand list @%p\n",
		     headerBlock, expandBlock);
#endif
		  expandNew = (SASCompoundExpandList *)
		    SASSimpleSpaceToAddr (expandBlock);
		  countNew =
		    ((heap_size - default_page) / sizeof (void *)) - 2;
		  memcpy (expandNew, list, sizeof (SASCompoundExpandList));
		  headerBlock->expandSpace = expandBlock;
		  headerBlock->expandList = expandNew;
		  expandNew->max_count = countNew;
		  SASBlockDealloc (heapBlock, heap_size);
		  newHeap = SASCompoundHeapExpandCreate (heap);
		}
	      else
		{
#ifdef __SASDebugPrint__
		  sas_printf
		    ("SASCompoundHeapExpandCreate(%p) extended expand list alloc failed\n",
		     headerBlock);
#endif
		  SASBlockDealloc (heapBlock, heap_size);
		}
	    }
	  else
	    {
#ifdef __SASDebugPrint__
	      sas_printf
		("SASCompoundHeapExpandCreate(%p) failed, list full\n",
		 headerBlock);
#endif
	      SASBlockDealloc (heapBlock, heap_size);
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapExpandCreate(%p) SASBlockAlloc failed\n",
		  headerBlock);
#endif
    }
  return newHeap;
}

SASCompoundHeap_t
SASCompoundFixedHeapCreate (block_size_t heap_size)
{
  SASBlockHeader *heapBlock = NULL;
  SASCompoundHeap_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      SASCompoundHeapHeader *headerBlock =
	(SASCompoundHeapHeader *) heapBlock;

      newHeap = SASCompoundHeapInit (heapBlock,
				     heap_size, default_page, false);
      headerBlock->loadFactor = 100;
    }
  return newHeap;
}

SASCompoundHeap_t
SASCompoundHeapCreatePageSize (block_size_t heap_size, block_size_t page_size)
{
  SASBlockHeader *heapBlock = NULL;
  SASCompoundHeap_t newHeap = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      newHeap = SASCompoundHeapInit (heapBlock, heap_size, page_size, true);
    }
  return newHeap;
}

SASCompoundHeap_t
SASCompoundHeapCreate (block_size_t heap_size)
{
  return SASCompoundHeapCreatePageSize (heap_size, default_page);
}

void
SASCompoundHeapSetLoadFactor (SASCompoundHeap_t * heap, int load)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
    {
      headerBlock->loadFactor = load;
    }
}

int
SASCompoundHeapGetLoadFactor (SASCompoundHeap_t * heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  int result = -1;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
    {
      result = headerBlock->loadFactor;
    }
  return result;
}

static SASSimpleHeap_t
SASCompoundHeapAllocInternal (SASCompoundHeapHeader * headerBlock)
{
  SASBlockHeader *simpleBlock;
  block_size_t heapSize;
  block_size_t simpleSize;
  freeNode *mem = NULL;
  SASSimpleHeap_t newHeap = NULL;

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
	  newHeap = SASSimpleHeapInit (mem, SAS_RUNTIME_SIMPLEHEAP,
				       simpleSize);
	  simpleBlock->baseBlock = (SASBlockHeader *) headerBlock;
	}
    }
  return newHeap;
}

SASSimpleHeap_t
SASCompoundHeapAllocNoLock (SASCompoundHeap_t heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  SASSimpleHeap_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
    {
      if (SASCompoundHeapIsExpanding (headerBlock))
	{
	  SASCompoundExpandList *list = headerBlock->expandList;
	  SASCompoundHeapHeader *expandHeader;
	  block_size_t i;
	  expandHeader = list->heap[list->count - 1];

	  if (SASCompoundHeapPercentUsed (expandHeader)
	      >= expandHeader->loadFactor)
	    {
	      expandHeader = NULL;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (SASCompoundHeapPercentUsed (expandBlock)
		      < expandBlock->loadFactor)
		    {
		      expandHeader = expandBlock;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASCompoundHeapHeader *)
		    SASCompoundHeapExpandCreate (heap);
		}
	    }
	  if (expandHeader != NULL)
	    newHeap = SASCompoundHeapAllocInternal (expandHeader);
	}
      else
	{
	  newHeap = SASCompoundHeapAllocInternal (headerBlock);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapAllocNoLock(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

SASSimpleHeap_t
SASCompoundHeapAlloc (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASSimpleHeap_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASCompoundHeapHeader *heapHeader = (SASCompoundHeapHeader *) heap;
      SASLock (heap, SasUserLock__WRITE);
      if (SASCompoundHeapIsExpanding (heapHeader))
	{
	  SASCompoundExpandList *list = heapHeader->expandList;
	  SASCompoundHeapHeader *lastHeader;
	  SASCompoundHeapHeader *expandHeader = NULL;
	  block_size_t i;
	  block_size_t last_lock = 0;
	  lastHeader = list->heap[list->count - 1];
	  if (lastHeader != heapHeader)
	    SASLock (lastHeader, SasUserLock__WRITE);

	  if (SASCompoundHeapPercentUsed (lastHeader)
	      >= heapHeader->loadFactor)
	    {
	      last_lock = list->count - 1;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASCompoundHeapPercentUsed (expandBlock)
		      < expandBlock->loadFactor)
		    {
		      expandHeader = expandBlock;
		      last_lock = i;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASCompoundHeapHeader *)
		    SASCompoundHeapExpandCreate (heap);
		}
	    }
	  else
	    {
	      expandHeader = lastHeader;
	    }
	  if (expandHeader != NULL)
	    newHeap = SASCompoundHeapAllocInternal (expandHeader);

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
	  newHeap = SASCompoundHeapAllocInternal (heapHeader);
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapAlloc(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

void
SASCompoundHeapFreeNoLock (SASCompoundHeap_t heap, SASSimpleHeap_t free_block)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
				  SAS_RUNTIME_SIMPLEHEAP))
    {
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  if (SASCompoundHeapIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (SASCompoundHeapContains (expandBlock, free_block))
		    {
		      SASCompoundHeapFreeInternal (expandBlock, free_block);
		      break;
		    }
		}
	    }
	  else
	    {
	      if (SASCompoundHeapContains (headerBlock, free_block))
		{
		  SASCompoundHeapFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SASCompoundHeapFreeNoLock(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASCompoundHeapFreeNoLock(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapFreeNoLock(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

void
SASCompoundHeapFree (SASCompoundHeap_t heap, SASSimpleHeap_t free_block)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
				  SAS_RUNTIME_SIMPLEHEAP))
    {
      SASLock (heap, SasUserLock__WRITE);
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  if (SASCompoundHeapIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASCompoundHeapContains (expandBlock, free_block))
		    {
		      SASCompoundHeapFreeInternal (expandBlock, free_block);
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
	      if (SASCompoundHeapContains (headerBlock, free_block))
		{
		  SASCompoundHeapFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SASCompoundHeapFree(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASCompoundHeapFree(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapFree(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

SASSimpleHeap_t
SASCompoundHeapNearAllocNoLock (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;
  SASCompoundHeapHeader *baseHeader = NULL;
  SASSimpleHeap_t newHeap = NULL;
  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader;

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_COMPOUNDHEAP))
	    {
	      if (SASCompoundHeapIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASCompoundHeapHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}

	      if (SASCompoundHeapAvail (compoundHeader))
		newHeap = SASCompoundHeapAllocInternal (compoundHeader);
	      else
		newHeap =
		  SASCompoundHeapAllocNoLock ((SASCompoundHeap_t) baseHeader);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASCompoundHeapNearAllocNoLock(%p) -> %p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	}
    }
  return newHeap;
}

SASSimpleHeap_t
SASCompoundHeapNearAlloc (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;
  SASCompoundHeapHeader *baseHeader = NULL;
  SASSimpleHeap_t newHeap = NULL;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader;

	  SASLock (compoundHeader, SasUserLock__WRITE);

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_COMPOUNDHEAP))
	    {
	      if (SASCompoundHeapIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASCompoundHeapHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}
#if 0
	      sas_printf ("SASCompoundHeapNearAlloc(%p) %p %p\n",
			  nearObj, compoundHeader, baseHeader);
#endif
	      if (SASCompoundHeapAvail (compoundHeader))
		{
		  newHeap = SASCompoundHeapAllocInternal (compoundHeader);
		}
	      else
		{
		  newHeap =
		    SASCompoundHeapAlloc ((SASCompoundHeap_t) baseHeader);
		}
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASCompoundHeapNearAlloc(%p) -> %p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	  SASUnlock (compoundHeader);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapNearAlloc(%p) block check failed\n",
		  nearObj);
#endif
    }
  return newHeap;
}

void
SASCompoundHeapNearDeallocNoLock (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;
  if (nearHeader != NULL)
    {
      if ((nearHeader->baseBlock != nearHeader)
	  && (nearHeader->baseBlock != NULL))
	compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
      else
	compoundHeader = (SASCompoundHeapHeader *) nearHeader;

      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  SASCompoundHeapFreeInternal (compoundHeader, nearHeader);
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASCompoundHeapNearDeallocNoLock(%p) type check failed near=%p compound=%p\n",
	     nearObj, nearHeader, compoundHeader);
#endif
	}
    }
}

void
SASCompoundHeapNearDealloc (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;

  if (nearHeader != NULL && SOMSASCheckBlockSig (nearHeader))
    {
      if ((nearHeader->baseBlock != nearHeader)
	  && (nearHeader->baseBlock != NULL))
	compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
      else
	compoundHeader = (SASCompoundHeapHeader *) nearHeader;

      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  SASLock (compoundHeader, SasUserLock__WRITE);
	  SASCompoundHeapFreeInternal (compoundHeader, nearHeader);
	  SASUnlock (compoundHeader);
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASCompoundHeapNearDealloc(%p) type check failed near=%p compound=%p\n",
	     nearObj, nearHeader, compoundHeader);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapNearDealloc(%p) block check failed\n",
		  nearObj);
#endif
    }
}

block_size_t
SASCompoundHeapAllocSize (SASCompoundHeap_t heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  block_size_t simpleSize = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
    {
      simpleSize = headerBlock->pageSize;
    }
  return simpleSize;
}

block_size_t
SASCompoundHeapFreeSpaceNoLock (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASCompoundHeapHeader *heapBlock = (SASCompoundHeapHeader *) heap;
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
SASCompoundHeapFreeSpace (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__WRITE);
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  for (i = 1; i < list->count; i++)
	    {
	      SASLock (list->heap[i], SasUserLock__WRITE);
	    }

	  heapFree = SASCompoundHeapFreeSpaceNoLock (heap);

	  for (i = 1; i < list->count; i++)
	    {
	      SASUnlock (list->heap[i]);
	    }
	}
      else
	{
	  heapFree = SASCompoundHeapFreeSpaceNoLock (heap);
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapFreeSpace(%p) type check failed\n", heap);
#endif
    }
  return heapFree;
}

block_size_t
SASCompoundHeapAllocSpace (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	    }
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapAllocSpace(%p) type check failed\n", heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapWriteAll (SASCompoundHeap_t heap, int asyncBool)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncWrite (subHeap,
			     subHeap->blockHeader.blockSize, asyncBool);
	    }
	}
      sasMsyncWrite (headerBlock, c_heap->blockHeader.blockSize, asyncBool);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapWriteAll(%p) type check failed\n", heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapPurgeAll (SASCompoundHeap_t heap, int asyncBool)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncPurge (subHeap,
			     subHeap->blockHeader.blockSize, asyncBool);
	    }
	}
      sasMsyncPurge (headerBlock, c_heap->blockHeader.blockSize, asyncBool);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapPurgeAll(%p) type check failed\n", heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapBringAll (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncBring (subHeap, subHeap->blockHeader.blockSize);
	    }
	}
      sasMsyncBring (headerBlock, c_heap->blockHeader.blockSize);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapBringAll(%p) type check failed\n", heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapReleaseAll (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncRelease (subHeap, subHeap->blockHeader.blockSize);
	    }
	}
      sasMsyncRelease (headerBlock, c_heap->blockHeader.blockSize);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapBringAll(%p) type check failed\n", heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapSeqAccessAll (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncSequential (subHeap, subHeap->blockHeader.blockSize);
	    }
	}
      sasMsyncSequential (headerBlock, c_heap->blockHeader.blockSize);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapSeqAccessAll(%p) type check failed\n",
		  heap);
#endif
    }
  return heapAlloc;
}

block_size_t
SASCompoundHeapRandomAccessAll (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASCompoundHeapHeader *c_heap;
  block_size_t heapAlloc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASLock (heap, SasUserLock__READ);
      c_heap = (SASCompoundHeapHeader *) heap;
      heapAlloc = c_heap->blockHeader.blockSize;
      SASCompoundExpandList *list =
	((SASCompoundHeapHeader *) headerBlock)->expandList;
      block_size_t i;
      if (list != NULL)
	{
	  SASCompoundHeapHeader *subHeap;
	  for (i = 1; i < list->count; i++)
	    {
	      subHeap = list->heap[i];
	      heapAlloc += subHeap->blockHeader.blockSize;
	      sasMsyncRandom (subHeap, subHeap->blockHeader.blockSize);
	    }
	}
      sasMsyncRandom (headerBlock, c_heap->blockHeader.blockSize);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapRandomAccessAll(%p) type check failed\n",
		  heap);
#endif
    }
  return heapAlloc;
}

void
SASCompoundHeapDestroyNoLock (SASCompoundHeap_t heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  block_size_t heapSize;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
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
	  if (headerBlock->expandSpace != NULL)
	    {
	      SASSimpleSpaceDestroy (headerBlock->expandSpace);
	    }
	}
      SASBlockDealloc (heap, heapSize);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapDestroyNoLock(%p) type check failed\n",
		  heap);
#endif
    }
}

void
SASCompoundHeapDestroy (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;

  if (SOMSASCheckBlockSig (headerBlock))
    {
      SASLock (heap, SasUserLock__WRITE);
      SASCompoundHeapDestroyNoLock (heap);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundHeapDestroy(%p) block check failed\n", heap);
#endif
    }
}



static SPHSinglePCQueue_t
SPHCompoundPCQAllocInternal (SASCompoundHeapHeader * headerBlock)
{
  SASBlockHeader *simpleBlock;
  block_size_t heapSize;
  block_size_t simpleSize;
  freeNode *mem = NULL;
  SPHSinglePCQueue_t newHeap = NULL;
  unsigned short stride = 128;

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
	  newHeap = /*SASSimpleHeapInit (mem, SAS_RUNTIME_SIMPLEHEAP,
				       simpleSize)*/

      SPHSinglePCQueueInitWithStride (simpleBlock, simpleSize,
					     stride, SPHSPCQUEUE_CIRCULAR);
	  simpleBlock->baseBlock = (SASBlockHeader *) headerBlock;
	}
    }
  return newHeap;
}

SPHSinglePCQueue_t
SPHCompoundPCQAllocNoLock (SASCompoundHeap_t heap)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;
  SPHSinglePCQueue_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_COMPOUNDHEAP))
    {
      if (SASCompoundHeapIsExpanding (headerBlock))
	{
	  SASCompoundExpandList *list = headerBlock->expandList;
	  SASCompoundHeapHeader *expandHeader;
	  block_size_t i;
	  expandHeader = list->heap[list->count - 1];

	  if (SASCompoundHeapPercentUsed (expandHeader)
	      >= expandHeader->loadFactor)
	    {
	      expandHeader = NULL;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (SASCompoundHeapPercentUsed (expandBlock)
		      < expandBlock->loadFactor)
		    {
		      expandHeader = expandBlock;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASCompoundHeapHeader *)
		    SASCompoundHeapExpandCreate (heap);
		}
	    }
	  if (expandHeader != NULL)
	    newHeap = SPHCompoundPCQAllocInternal (expandHeader);
	}
      else
	{
	  newHeap = SPHCompoundPCQAllocInternal (headerBlock);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHCompoundPCQAllocNoLock(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

SPHSinglePCQueue_t
SPHCompoundPCQAlloc (SASCompoundHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHSinglePCQueue_t newHeap = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_COMPOUNDHEAP))
    {
      SASCompoundHeapHeader *heapHeader = (SASCompoundHeapHeader *) heap;
      SASLock (heap, SasUserLock__WRITE);
      if (SASCompoundHeapIsExpanding (heapHeader))
	{
	  SASCompoundExpandList *list = heapHeader->expandList;
	  SASCompoundHeapHeader *lastHeader;
	  SASCompoundHeapHeader *expandHeader = NULL;
	  block_size_t i;
	  block_size_t last_lock = 0;
	  lastHeader = list->heap[list->count - 1];
	  if (lastHeader != heapHeader)
	    SASLock (lastHeader, SasUserLock__WRITE);

	  if (SASCompoundHeapPercentUsed (lastHeader)
	      >= heapHeader->loadFactor)
	    {
	      last_lock = list->count - 1;
	      for (i = 0; i < list->count - 1; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASCompoundHeapPercentUsed (expandBlock)
		      < expandBlock->loadFactor)
		    {
		      expandHeader = expandBlock;
		      last_lock = i;
		      break;
		    }
		}
	      if (expandHeader == NULL)
		{
		  expandHeader = (SASCompoundHeapHeader *)
		    SASCompoundHeapExpandCreate (heap);
		}
	    }
	  else
	    {
	      expandHeader = lastHeader;
	    }
	  if (expandHeader != NULL)
	    newHeap = SPHCompoundPCQAllocInternal (expandHeader);

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
	  newHeap = SPHCompoundPCQAllocInternal (heapHeader);
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHCompoundPCQAlloc(%p) type check failed\n", heap);
#endif
    }
  return newHeap;
}

static inline void
SPHCompoundPCQFreeInternal (SASCompoundHeapHeader * headerBlock,
		SPHSinglePCQueue_t free_block)
{
  freeNode *free_node = (freeNode *) free_block;
  block_size_t simpleSize = headerBlock->pageSize;

  memset (free_block, 0, simpleSize);
  freeNode_init (free_node, simpleSize);
  freeNode_deallocSpace (free_node, &headerBlock->blockHeader.blockFreeSpace,
			 simpleSize);
}

void
SPHCompoundPCQFreeNoLock (SASCompoundHeap_t heap, SPHSinglePCQueue_t free_block)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
		  SAS_RUNTIME_PCQUEUE))
    {
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  if (SASCompoundHeapIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (SASCompoundHeapContains (expandBlock, free_block))
		    {
			  SPHCompoundPCQFreeInternal (expandBlock, free_block);
		      break;
		    }
		}
	    }
	  else
	    {
	      if (SASCompoundHeapContains (headerBlock, free_block))
		{
	          SPHCompoundPCQFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SPHCompoundPCQFreeNoLock(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SPHCompoundPCQFreeNoLock(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHCompoundPCQFreeNoLock(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

void
SPHCompoundPCQFree (SASCompoundHeap_t heap, SPHSinglePCQueue_t free_block)
{
  SASCompoundHeapHeader *headerBlock = (SASCompoundHeapHeader *) heap;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) free_block,
		  SAS_RUNTIME_PCQUEUE))
    {
      SASLock (heap, SasUserLock__WRITE);
      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				      SAS_RUNTIME_COMPOUNDHEAP))
	{
	  if (SASCompoundHeapIsExpanding (headerBlock))
	    {
	      SASCompoundExpandList *list = headerBlock->expandList;
	      block_size_t i;
	      for (i = 0; i < list->count; i++)
		{
		  SASCompoundHeapHeader *expandBlock = list->heap[i];
		  if (i > 0)
		    SASLock (expandBlock, SasUserLock__WRITE);
		  if (SASCompoundHeapContains (expandBlock, free_block))
		    {
			  SPHCompoundPCQFreeInternal (expandBlock, free_block);
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
	      if (SASCompoundHeapContains (headerBlock, free_block))
		{
	          SPHCompoundPCQFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
		}
	      else
		{
		  sas_printf
		    ("SASCompoundPCQFree(%p, %p) free block not contained\n",
		     heap, free_block);
#endif
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASCompoundPCQFree(%p, %p) type check failed\n",
		      heap, free_block);
#endif
	}
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASCompoundPCQFree(%p, %p) type check failed\n",
		  heap, free_block);
#endif
    }
}

SPHSinglePCQueue_t
SPHCompoundPCQNearAllocNoLock (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;
  SASCompoundHeapHeader *baseHeader = NULL;
  SPHSinglePCQueue_t newHeap = NULL;
  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader;

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_COMPOUNDHEAP))
	    {
	      if (SASCompoundHeapIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASCompoundHeapHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}

	      if (SASCompoundHeapAvail (compoundHeader))
		newHeap = SPHCompoundPCQAllocInternal (compoundHeader);
	      else
		newHeap =
		  SASCompoundHeapAllocNoLock ((SASCompoundHeap_t) baseHeader);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SPHCompoundPCQNearAllocNoLock(%p) -> %p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	}
    }
  return newHeap;
}

SPHSinglePCQueue_t
SPHCompoundPCQNearAlloc (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASCompoundHeapHeader *compoundHeader = NULL;
  SASCompoundHeapHeader *baseHeader = NULL;
  SPHSinglePCQueue_t newHeap = NULL;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if ((nearHeader->baseBlock != nearHeader)
	      && (nearHeader->baseBlock != NULL))
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader->baseBlock;
	  else
	    compoundHeader = (SASCompoundHeapHeader *) nearHeader;

	  SASLock (compoundHeader, SasUserLock__WRITE);

	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) compoundHeader,
					  SAS_RUNTIME_COMPOUNDHEAP))
	    {
	      if (SASCompoundHeapIsExpanding (compoundHeader))
		{
		  baseHeader = compoundHeader;
		}
	      else
		{
		  if ((compoundHeader->blockHeader.baseBlock
		       != (SASBlockHeader *) compoundHeader)
		      && (compoundHeader->blockHeader.baseBlock != NULL))
		    baseHeader =
		      (SASCompoundHeapHeader *) compoundHeader->blockHeader.
		      baseBlock;
		  else
		    baseHeader = compoundHeader;
		}
#if 0
	      sas_printf ("SPHCompoundPCQNearAlloc(%p) %p %p\n",
			  nearObj, compoundHeader, baseHeader);
#endif
	      if (SASCompoundHeapAvail (compoundHeader))
		{
		  newHeap = SPHCompoundPCQAllocInternal (compoundHeader);
		}
	      else
		{
		  newHeap =
		    SASCompoundHeapAlloc ((SASCompoundHeap_t) baseHeader);
		}
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SPHCompoundPCQNearAlloc(%p) -> %p type check failed\n",
		 nearObj, compoundHeader);
#endif
	    }
	  SASUnlock (compoundHeader);
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHCompoundPCQNearAlloc(%p) block check failed\n",
		  nearObj);
#endif
    }
  return newHeap;
}
