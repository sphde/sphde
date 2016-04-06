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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "sasalloc.h"
#include "freenode.h"
#include "sasio.h"
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sasindexpriv.h"
#include "sasindexnodepriv.h"

//#define __SASDebugPrint__ 1

static inline int
SASIndexAvail(SASIndexHeader *headerBlock)
{
	return (headerBlock->blockHeader.blockFreeSpace != NULL);
}

static inline int 
SASIndexPercentFree (SASIndexHeader	*heapBlock)
{
	int percent = 0;
	int divisor;
	block_size_t	heapFree = 0;
	
	if (heapBlock->blockHeader.blockFreeSpace != NULL)
	{
		heapFree = freeNode_freeSpaceTotal(heapBlock->blockHeader.blockFreeSpace);
		divisor = __builtin_ctzl(heapBlock->blockHeader.blockSize);
		percent = (int)((heapFree  * 100) >> divisor);
	}
#if 0
    sas_printf("SASIndexPercentFree(%p) = %d for (%ld, %ld)\n", 
               heapBlock, percent, heapFree, heapUsed);
#endif
	return percent;
}

static inline int 
SASIndexPercentUsed (SASIndexHeader	*heapBlock)
{
	int percent = 100;
	int divisor;
	block_size_t	heapFree = 0;
	block_size_t	heapUsed = 0;
	
	if (heapBlock->blockHeader.blockFreeSpace != NULL)
	{
		heapFree = freeNode_freeSpaceTotal(heapBlock->blockHeader.blockFreeSpace);
		heapUsed = heapBlock->blockHeader.blockSize - heapFree;
		divisor = __builtin_ctzl(heapBlock->blockHeader.blockSize);
		percent = (int)((heapUsed  * 100) >> divisor);
	}
#if 0
    sas_printf("SASIndexPercentFree(%p) = %d for (%ld, %ld)\n", 
               heapBlock, percent, heapFree, heapUsed);
#endif
	return percent;
}

static inline int
SASIndexIsExpanding(SASIndexHeader *headerBlock)
{
	return (headerBlock->expandList != NULL);
}

static inline int
SASIndexContains(SASIndexHeader *headerBlock, 
                        SASIndexNode_t simpleHeap)
{
	block_size_t	containedHeap	= (block_size_t) simpleHeap;
	block_size_t	containerLow	= (block_size_t) headerBlock;
	block_size_t	containerHigh	= (block_size_t) headerBlock +
										headerBlock->blockHeader.blockSize;
	return ((containedHeap > containerLow) && (containedHeap < containerHigh));
}

static inline void
SASIndexFreeInternal (SASIndexHeader *headerBlock, 
                             SASIndexNode_t free_block)
{
    freeNode		*free_node = (freeNode*)free_block;
    block_size_t	simpleSize = headerBlock->pageSize;
    
    memset (free_block, 0, simpleSize);
    freeNode_init(free_node, simpleSize);
    freeNode_deallocSpace(free_node, &headerBlock->blockHeader.blockFreeSpace, 
	                        simpleSize);
}

SASIndex_t 
SASIndexExpandInit (void* heap_seg, 
                    block_size_t heap_size,
                    block_size_t page_size)
{
    SASIndexHeader	*heapBlock = (SASIndexHeader*)heap_seg;
    SASIndexSpillList	*spill_lst;
    char		*heapStart = NULL; 
    node_size_t		remaining;
    
    if ( heapBlock )
    {
		heapStart = (char*) heapBlock + default_page;
		initSOMSASBlock((SASBlockHeader*)heapBlock, 
                                       SAS_RUNTIME_INDEX,
                                       heap_size, heapStart);
    }
    
    heapBlock->pageSize = page_size;
   
    remaining= default_page - sizeof(SASIndexHeader);
    heapBlock->headerFreeSpace = (freeNode *)&heapBlock[1];
    freeNode_init(heapBlock->headerFreeSpace, remaining);
    
    heapBlock->expandList = NULL;
    
    spill_lst = (SASIndexSpillList*)freeNode_allocSpace(heapBlock->headerFreeSpace,
			&heapBlock->headerFreeSpace, 
			sizeof(SASIndexSpillList));
	if (spill_lst != NULL)
	{
		heapBlock->spillList = spill_lst;
		spill_lst->count = 0;
		spill_lst->max_count = SASBTREE_SPILLLIST_SIZE;
		for (int i=0; i<SASBTREE_SPILLLIST_SIZE; i++)
		{
			spill_lst->spillHeap[i] = NULL;
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexInit(%p, %zu, %zu) spillList alloc failed\n", 
    	           heap_seg, heap_size, page_size);
#endif
	}

    return (SASIndex_t)heapBlock;
}

SASIndex_t 
SASIndexInit (void* heap_seg, 
                     block_size_t heap_size,
                     block_size_t page_size,
                     int	expanding)
{
    SASIndexHeader	*heapBlock = (SASIndexHeader*)heap_seg;
    SASIndexCommon	*commonAlloc;
    SASCompoundExpandList	*list;
    SASIndexSpillList	*spill_lst;
    char		*heapStart = NULL; 
    node_size_t		remaining;
    
    if ( heapBlock )
    {
		heapStart = (char*) heapBlock + default_page;
		initSOMSASBlock((SASBlockHeader*)heapBlock, 
		                               SAS_RUNTIME_INDEX,
		                               heap_size, heapStart);
    }
    
    heapBlock->pageSize = page_size;
   
    remaining= default_page - sizeof(SASIndexHeader);
    heapBlock->headerFreeSpace = (freeNode *)&heapBlock[1];
    freeNode_init(heapBlock->headerFreeSpace, remaining);
    heapBlock->blockHeader.baseBlock = (SASBlockHeader*)heapBlock;
    heapBlock->blockHeader.nextBlock = (SASBlockHeader*)heapBlock;
    
    if (expanding)
    {
	    list = (SASCompoundExpandList*)freeNode_allocSpace(heapBlock->headerFreeSpace,
				&heapBlock->headerFreeSpace, 
				sizeof(SASCompoundExpandList));
		if (list != NULL)
		{
			heapBlock->expandList = list;
			list->count = 1;
			list->max_count = 254;
			list->heap[0] = heapBlock;
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexInit(%p, %zu, %zu) ExpandList alloc failed\n", 
	    	           heap_seg, heap_size, page_size);
#endif
		}
    }
    
    commonAlloc = (SASIndexCommon*)freeNode_allocSpace(heapBlock->headerFreeSpace,
			&heapBlock->headerFreeSpace, 
			sizeof(SASIndexCommon));
	if (commonAlloc != NULL)
	{
		heapBlock->common = commonAlloc;
		commonAlloc->version = 0;
		commonAlloc->modCount = 1;
		commonAlloc->max_key = NULL;
		commonAlloc->min_key = NULL;
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexInit(%p, %zu, %zu) Common alloc failed\n", 
    	           heap_seg, heap_size, page_size);
#endif
	}
    
    spill_lst = (SASIndexSpillList*)freeNode_allocSpace(heapBlock->headerFreeSpace,
			&heapBlock->headerFreeSpace, 
			sizeof(SASIndexSpillList));
	if (spill_lst != NULL)
	{
		heapBlock->spillList = spill_lst;
		spill_lst->count = 0;
		spill_lst->max_count = SASBTREE_SPILLLIST_SIZE;
		for (int i=0; i<SASBTREE_SPILLLIST_SIZE; i++)
		{
			spill_lst->spillHeap[i] = NULL;
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexInit(%p, %zu, %zu) spillList alloc failed\n", 
    	           heap_seg, heap_size, page_size);
#endif
	}

    return (SASIndex_t)heapBlock;
}

SASIndex_t 
SASIndexFixedCreate (block_size_t heap_size)
{
    SASBlockHeader	*heapBlock = NULL;
    SASIndex_t newHeap = NULL;
    
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)heap_size);
    if ( heapBlock )
    {
		newHeap = SASIndexInit (heapBlock,  heap_size, 
		                              default_page, false);
    }
    return newHeap;
}

SASIndex_t 
SASIndexExpandCreate (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    block_size_t heap_size	= headerBlock->blockHeader.blockSize;
    block_size_t page_size	= headerBlock->pageSize;
    SASCompoundExpandList	*list = headerBlock->expandList;
    SASIndexCommon	*commonPtr = headerBlock->common;
    SASBlockHeader	*heapBlock = NULL;
    SASIndex_t newHeap = NULL;
    
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)heap_size);
    if ( heapBlock )
    {
    	if ( list->count < list->max_count )
    	{
	    	SASIndexHeader *newHeader, *prevHeader;
			newHeap = SASIndexExpandInit (heapBlock,  
			                                    heap_size, page_size);
			newHeader = (SASIndexHeader*)newHeap;
			newHeader->blockHeader.baseBlock = &headerBlock->blockHeader;
			newHeader->blockHeader.nextBlock = &headerBlock->blockHeader;
			newHeader->common = commonPtr;
			prevHeader = list->heap[list->count - 1];
			list->heap[list->count] = newHeader;
			list->count++;
			prevHeader->blockHeader.nextBlock = &newHeader->blockHeader;
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexExpandCreate(%p) failed\n", 
	    	           headerBlock);
#endif
    	}
    }
    return newHeap;
}

SASIndex_t 
SASIndexCreatePageSize (block_size_t heap_size,
                              block_size_t page_size)
{
    SASBlockHeader	*heapBlock = NULL;
    SASIndex_t newHeap = NULL;
    
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)heap_size);
    if ( heapBlock )
    {
		newHeap = SASIndexInit (heapBlock, heap_size, page_size, true);
    }
    return newHeap;
}

SASIndex_t 
SASIndexCreate (block_size_t heap_size)
{
    return SASIndexCreatePageSize (heap_size, default_page);
}

static SASIndexNode_t 
SASIndexAllocInternal (SASIndexHeader *headerBlock)
{
    SASBlockHeader	*simpleBlock;
    block_size_t	heapSize;
    block_size_t	simpleSize;
    freeNode		*mem = NULL;
    SASIndexNode_t newHeap = NULL;

	heapSize = headerBlock->blockHeader.blockSize;
	simpleSize = headerBlock->pageSize;
	if ( simpleSize < heapSize )
	{
	    mem = freeNode_allocSpace(headerBlock->blockHeader.blockFreeSpace,
		&headerBlock->blockHeader.blockFreeSpace, simpleSize);
		if (mem != NULL)
		{
			simpleBlock = (SASBlockHeader*)mem;
			newHeap = SASIndexNodeInit (mem, SAS_RUNTIME_INDEXNODE, 
			                             simpleSize);
			simpleBlock->baseBlock = (SASBlockHeader*)headerBlock;
		}
	}
	return newHeap;
}

SASIndexNode_t 
SASIndexAllocNoLock (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    SASIndexNode_t newHeap = NULL;
   
	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
	                                SAS_RUNTIME_INDEX))
    {
		if (SASIndexIsExpanding(headerBlock))
		{
	    	SASCompoundExpandList	*list = headerBlock->expandList;
	    	SASIndexHeader	*expandHeader;
	    	block_size_t	i;
	    	expandHeader = list->heap[list->count-1];
	    	
	    	if (SASIndexPercentUsed(expandHeader) >= DEFAULT_LOAD_FACTOR)
	    	{   
				expandHeader = NULL;
				for ( i = 0; i < list->count-1; i++ )
				{
					SASIndexHeader	*expandBlock = list->heap[i];
			    	if ( SASIndexPercentUsed (expandBlock) 
			    	       < DEFAULT_LOAD_FACTOR )
			    	{
			    		expandHeader = expandBlock;
			    		break;
			    	}
				}
				if ( expandHeader == NULL )
				{
					expandHeader = (SASIndexHeader*)
					                 SASIndexExpandCreate (heap);
				}
	    	}
	    	if (  expandHeader != NULL )
				newHeap = SASIndexAllocInternal (expandHeader);
		} else {
			newHeap = SASIndexAllocInternal (headerBlock);
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexAllocNoLock(%p) type check failed\n", heap);
#endif
    }
    return newHeap;
}

SASIndexNode_t 
SASIndexAlloc (SASIndex_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    SASIndexNode_t newHeap = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_INDEX) )
    {
    	SASIndexHeader	*heapHeader = (SASIndexHeader*)heap;
    	SASLock(heap, SasUserLock__WRITE);
		if (SASIndexIsExpanding(heapHeader))
		{
	    	SASCompoundExpandList	*list = heapHeader->expandList;
	    	SASIndexHeader	*lastHeader;
	    	SASIndexHeader	*expandHeader = NULL;
	    	block_size_t	i;
	    	block_size_t	last_lock = 0;
	    	lastHeader = list->heap[list->count-1];
	    	if (lastHeader != heapHeader)
	    		SASLock(lastHeader, SasUserLock__WRITE);
	    	
	    	if (SASIndexPercentUsed(lastHeader) >= DEFAULT_LOAD_FACTOR)
	    	{
				for ( i = 0; i < list->count-1; i++ )
				{
					SASIndexHeader	*expandBlock = list->heap[i];
					if ( i > 0 )
	    				SASLock(expandBlock, SasUserLock__WRITE);
			    	if ( SASIndexPercentUsed (expandBlock) 
			    	       < DEFAULT_LOAD_FACTOR )
			    	{
			    		expandHeader = expandBlock;
			    		last_lock = i;
			    		break;
			    	}
				}
				if ( expandHeader == NULL )
				{
					expandHeader = (SASIndexHeader*)
					                 SASIndexExpandCreate (heap);
		    		last_lock = list->count-2;
				}
	    	} else {
	    		expandHeader = lastHeader;
	    	}
	    	if (  expandHeader != NULL )
				newHeap = SASIndexAllocInternal (expandHeader);
				
	    	if (lastHeader != expandHeader)
	    	{
				for ( i = 1; i <= last_lock; i++ )
				{
	    			SASUnlock(list->heap[i]);
				}
	    	} 
	    	if (lastHeader != heapHeader)
				SASUnlock(lastHeader);
		} else {
			newHeap = SASIndexAllocInternal (heapHeader);
		} 
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexAlloc(%p) type check failed\n", heap);
#endif
    }
    return newHeap;
}

void 
SASIndexFreeNoLock (SASIndex_t heap, 
                          SASIndexNode_t free_block)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)free_block, 
                                    SAS_RUNTIME_INDEXNODE))
    {
	    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
	                                     SAS_RUNTIME_INDEX))
	    {
			if (SASIndexIsExpanding(headerBlock))
			{
		    	SASCompoundExpandList	*list = headerBlock->expandList;
		    	block_size_t	i;
				for ( i = 0; i < list->count; i++ )
				{
					SASIndexHeader	*expandBlock = list->heap[i];
					if (SASIndexContains(expandBlock, free_block))
					{
						SASIndexFreeInternal (expandBlock, free_block);
			    		break;
			    	}
				}
			} else {
				if (SASIndexContains(headerBlock, free_block))
				{
					SASIndexFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
			    } else {
			    	sas_printf("SASIndexFreeNoLock(%p, %p) free block not contained\n", 
					                 heap, free_block);
#endif
				}
			}
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexFreeNoLock(%p, %p) type check failed\n", 
			                 heap, free_block);
#endif
	    }
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexFreeNoLock(%p, %p) type check failed\n", 
		                 heap, free_block);
#endif
    }
}

void 
SASIndexFree (SASIndex_t heap, 
                    SASIndexNode_t free_block)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)free_block, 
	                                SAS_RUNTIME_INDEXNODE) )
    {
    	SASLock(heap, SasUserLock__WRITE);
	    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
	                                     SAS_RUNTIME_INDEX))
	    {
			if (SASIndexIsExpanding(headerBlock))
			{
		    	SASCompoundExpandList	*list = headerBlock->expandList;
		    	block_size_t	i;
				for ( i = 0; i < list->count; i++ )
				{
					SASIndexHeader	*expandBlock = list->heap[i];
					if ( i > 0 )
						SASLock(expandBlock, SasUserLock__WRITE);
					if (SASIndexContains(expandBlock, free_block))
					{
						SASIndexFreeInternal (expandBlock, free_block);					
						if ( i > 0 )
							SASUnlock(heap);
			    		break;
			    	}
					if ( i > 0 )
			    		SASUnlock(heap);
				}
			} else {
				if (SASIndexContains(headerBlock, free_block))
				{
					SASIndexFreeInternal (headerBlock, free_block);
#ifdef __SASDebugPrint__
			    } else {
			    	sas_printf("SASIndexFree(%p, %p) free block not contained\n", 
					                 heap, free_block);
#endif
				}
			}
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexFree(%p, %p) type check failed\n", 
			                 heap, free_block);
#endif
	    } 
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexFree(%p, %p) type check failed\n", 
		                 heap, free_block);
#endif
    }
}
		
SASIndexNode_t 
SASIndexNearAllocNoLock(void *nearObj)
{
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASIndexHeader	*compoundHeader = NULL;
    SASIndexHeader	*baseHeader = NULL;
    SASIndexNode_t newHeap = NULL;
	if (nearHeader != NULL)
	{
		if ( SOMSASCheckBlockSig (nearHeader) )
		{
			if ((nearHeader->baseBlock != nearHeader)
			&& (nearHeader->baseBlock != NULL))
				compoundHeader = (SASIndexHeader*) nearHeader->baseBlock;
			else
				compoundHeader = (SASIndexHeader*)nearHeader;
				   
		    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)compoundHeader, 
			                                SAS_RUNTIME_INDEX) )
		    {
				if (SASIndexIsExpanding(compoundHeader))
				{
					baseHeader = compoundHeader;
				} else {
					if ((compoundHeader->blockHeader.baseBlock 
					 != (SASBlockHeader *)compoundHeader)
			        && (compoundHeader->blockHeader.baseBlock != NULL))
			        	baseHeader = (SASIndexHeader*)compoundHeader
			        	              ->blockHeader.baseBlock;
			        else
			        	baseHeader = compoundHeader;
				}
				
				if (SASIndexAvail(compoundHeader))
					newHeap = SASIndexAllocInternal (compoundHeader);
				else 
					newHeap = SASIndexAllocNoLock ((SASIndex_t)baseHeader);
#ifdef __SASDebugPrint__
		    } else {
		    	sas_printf("SASIndexNearAllocNoLock(%p)->%p type check failed\n", 
				                 nearObj, compoundHeader);
#endif
		    }
		}
	}
    return newHeap;
}
		
SASIndexNode_t 
SASIndexNearAlloc(void *nearObj)
{
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASIndexHeader	*compoundHeader = NULL;
    SASIndexHeader	*baseHeader = NULL;
    SASIndexNode_t newHeap = NULL;
    
    if (nearHeader != NULL)
    {
		if ( SOMSASCheckBlockSig (nearHeader) )
		{
			if ((nearHeader->baseBlock != nearHeader)
			&& (nearHeader->baseBlock != NULL))
				compoundHeader = (SASIndexHeader*) nearHeader->baseBlock;
			else
				compoundHeader = (SASIndexHeader*)nearHeader;
				
    		SASLock(compoundHeader, SasUserLock__WRITE);
				   
		    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)compoundHeader, 
			                                SAS_RUNTIME_INDEX) )
		    {
				if (SASIndexIsExpanding(compoundHeader))
				{
					baseHeader = compoundHeader;
				} else {
					if ((compoundHeader->blockHeader.baseBlock 
					 != (SASBlockHeader *)compoundHeader)
			        && (compoundHeader->blockHeader.baseBlock != NULL))
			        	baseHeader = (SASIndexHeader*)compoundHeader
			        	              ->blockHeader.baseBlock;
			        else
			        	baseHeader = compoundHeader;
				}
#if 0
				sas_printf("SASIndexNearAlloc(%p) %p %p\n", 
				                 nearObj, compoundHeader, baseHeader);
#endif
				if (SASIndexAvail(compoundHeader))
				{
					newHeap = SASIndexAllocInternal (compoundHeader);
				} else {
					newHeap = SASIndexAlloc ((SASIndex_t)baseHeader);
				}
#ifdef __SASDebugPrint__
		    } else {
		    	sas_printf("SASIndexNearAlloc(%p)->%p type check failed\n", 
				                 nearObj, compoundHeader);
#endif
		    }
		    SASUnlock(compoundHeader);
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexNearAlloc(%p) block check failed\n", nearObj);
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

static SASIndexNode_t 
SASIndexSpillInternal (SASIndexHeader *headerBlock)
{
    SASBlockHeader	*simpleBlock;
    block_size_t	heapSize;
    block_size_t	simpleSize;
    freeNode		*mem = NULL;
    SASIndexNode_t newHeap = NULL;

	heapSize = headerBlock->blockHeader.blockSize;
	simpleSize = headerBlock->pageSize;
	if ( simpleSize < heapSize )
	{
	    mem = freeNode_allocSpace(headerBlock->blockHeader.blockFreeSpace,
		&headerBlock->blockHeader.blockFreeSpace, simpleSize);
		if (mem != NULL)
		{
			simpleBlock = (SASBlockHeader*)mem;
			newHeap = SASIndexSpillInit (mem, SAS_RUNTIME_INDEXNODE, 
			                             simpleSize);
			simpleBlock->baseBlock = (SASBlockHeader*)headerBlock;
		}
	}
	return newHeap;
}

SASIndexNode_t 
SASIndexSpillAllocExtended (SASIndex_t heap, lock_on_t lock_on)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    SASIndexNode_t newHeap = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_INDEX) )
    {
    	SASIndexHeader	*heapHeader = (SASIndexHeader*)heap;
        if (lock_on) SASLock(heap, SasUserLock__WRITE);
		if (SASIndexIsExpanding(heapHeader))
		{
	    	SASCompoundExpandList	*list = heapHeader->expandList;
	    	SASIndexHeader	*lastHeader;
	    	SASIndexHeader	*expandHeader = NULL;
	    	block_size_t	i;
	    	block_size_t	last_lock = 0;
	    	lastHeader = list->heap[list->count-1];
	    	if (lastHeader != heapHeader)
                    if (lock_on) SASLock(lastHeader, SasUserLock__WRITE);
	    	
	    	if (SASIndexPercentUsed(lastHeader) >= DEFAULT_LOAD_FACTOR)
	    	{
	    		last_lock = list->count-2;
				for ( i = 0; i < list->count-1; i++ )
				{
					SASIndexHeader	*expandBlock = list->heap[i];
					if ( i > 0 )
                                        if (lock_on) SASLock(expandBlock, SasUserLock__WRITE);
			    	if ( SASIndexPercentUsed (expandBlock) 
			    	       < DEFAULT_LOAD_FACTOR )
			    	{
			    		expandHeader = expandBlock;
			    		last_lock = i;
			    		break;
			    	}
				}
				if ( expandHeader == NULL )
				{
					expandHeader = (SASIndexHeader*)
					                 SASIndexExpandCreate (heap);
				}
	    	} else {
	    		expandHeader = lastHeader;
	    	}
	    	if (  expandHeader != NULL )
				newHeap = SASIndexSpillInternal (expandHeader);
				
	    	if (lastHeader != expandHeader)
	    	{
				for ( i = 1; i <= last_lock; i++ )
				{
                                if (lock_on) SASUnlock(list->heap[i]);
				}
	    	} 
	    	if (lastHeader != heapHeader)
				if (lock_on) SASUnlock(lastHeader);
		} else {
			newHeap = SASIndexSpillInternal (heapHeader);
		} 
		if (lock_on) SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexSpillAllocExtended(%p) type check failed\n", heap);
#endif
    }
    return newHeap;
}
		
SASIndexNode_t 
SASIndexSpillAlloc(void *nearObj, lock_on_t lock_on)
{
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASIndexHeader	*compoundHeader = NULL;
    SASIndexHeader	*baseHeader = NULL;
    SASIndexNode_t newHeap = NULL;
    SASIndexSpillList	*spill_lst;
    
    if (nearHeader != NULL)
    {
		if ( SOMSASCheckBlockSig (nearHeader) )
		{
			if ((nearHeader->baseBlock != nearHeader)
			&& (nearHeader->baseBlock != NULL))
				compoundHeader = (SASIndexHeader*) nearHeader->baseBlock;
			else
				compoundHeader = (SASIndexHeader*)nearHeader;
				
                    if (lock_on) SASLock(compoundHeader, SasUserLock__WRITE);
				   
		    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)compoundHeader, 
			                                SAS_RUNTIME_INDEX) )
		    {
				if (SASIndexIsExpanding(compoundHeader))
				{
					baseHeader = compoundHeader;
				} else {
					if ((compoundHeader->blockHeader.baseBlock 
					 != (SASBlockHeader *)compoundHeader)
			        && (compoundHeader->blockHeader.baseBlock != NULL))
			        	baseHeader = (SASIndexHeader*)compoundHeader
			        	              ->blockHeader.baseBlock;
			        else
			        	baseHeader = compoundHeader;
				}
#if 0
				sas_printf("SASIndexSpillAlloc(%p) %p %p\n", 
				                 nearObj, compoundHeader, baseHeader);
#endif
				spill_lst = compoundHeader->spillList;
				if (lock_on) SASLock(spill_lst, SasUserLock__WRITE);
				if (spill_lst->count < spill_lst->max_count)
				{
					if (SASIndexAvail(compoundHeader))
					{
						newHeap = SASIndexSpillInternal (compoundHeader);
					} else {
						newHeap = SASIndexSpillAllocExtended ((SASIndex_t)baseHeader, lock_on);
					}
					if (newHeap != NULL)
					{
						spill_lst->spillHeap[spill_lst->count] = 
							(SASIndexNodeHeader*)newHeap;
						spill_lst->count++;
					}
				}
				if (lock_on) SASUnlock(spill_lst);
#ifdef __SASDebugPrint__
		    } else {
		    	sas_printf("SASIndexSpillAlloc(%p)->%p type check failed\n", 
				                 nearObj, compoundHeader);
#endif
		    }
		    if (lock_on) SASUnlock(compoundHeader);
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexSpillAlloc(%p) block check failed\n", nearObj);
#endif
    }
    return newHeap;
}

void 
SASIndexNearDeallocNoLock(void *nearObj)
{							
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASIndex_t	btreeHeader = NULL;
	if (nearHeader != NULL)
	{
		if ((nearHeader->baseBlock != nearHeader)
		&& (nearHeader->baseBlock != NULL))
			btreeHeader = (SASIndex_t) nearHeader->baseBlock;
		else
			btreeHeader = (SASIndex_t)nearHeader;
				   
	    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)btreeHeader, 
		                                SAS_RUNTIME_INDEX) )
	    {
			SASIndexFreeInternal ((SASIndexHeader*)btreeHeader, 
			                            nearHeader);
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexNearDeallocNoLock(%p) type check failed near=%p compound=%p\n", 
	    	           nearObj, nearHeader, btreeHeader);
#endif
	    }
	}
}

void 
SASIndexNearDealloc(void *nearObj)
{							
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASIndex_t	btreeHeader = NULL;
    
    if (nearHeader != NULL
    && SOMSASCheckBlockSig (nearHeader) )
    {
		if ((nearHeader->baseBlock != nearHeader)
		&& (nearHeader->baseBlock != NULL))
			btreeHeader = (SASIndex_t) nearHeader->baseBlock;
		else
			btreeHeader = (SASIndex_t)nearHeader;
			
	    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)btreeHeader, 
		                                SAS_RUNTIME_INDEX))
	    {
    		SASLock(btreeHeader, SasUserLock__WRITE);
			SASIndexFreeInternal ((SASIndexHeader*)btreeHeader,
			                            nearHeader);
			SASUnlock(btreeHeader);
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASIndexNearDealloc(%p) type check failed near=%p compound=%p\n", 
	    	           nearObj, nearHeader, btreeHeader);
#endif
	    } 
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexNearDealloc(%p) block check failed\n", nearObj);
#endif
    }
}

block_size_t 
SASIndexAllocSize (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    block_size_t	nodeSize = 0;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                    SAS_RUNTIME_INDEX))
    {
		nodeSize = headerBlock->pageSize;
    }
	return nodeSize;
}

block_size_t 
SASIndexFreeSpaceNoLock (SASIndex_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
	                                SAS_RUNTIME_INDEX))
    {
    	SASIndexHeader	*heapBlock = (SASIndexHeader*)heap;
    	SASCompoundExpandList	*list = heapBlock->expandList;
    	block_size_t	i;
		if ( list != NULL )
		{
			for ( i = 0; i < list->count; i++ )
			{   
				SASBlockHeader	*expandBlock = &list->heap[i]->blockHeader;
		    	if (expandBlock->blockFreeSpace != NULL)
					heapFree += freeNode_freeSpaceTotal(expandBlock->blockFreeSpace);
			}
		} else {
	    	if (headerBlock->blockFreeSpace != NULL)
				heapFree = freeNode_freeSpaceTotal(headerBlock->blockFreeSpace);
		}
    }
    return heapFree;
}

block_size_t 
SASIndexFreeSpace (SASIndex_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
	                                SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
    	SASCompoundExpandList	*list = 
    		((SASIndexHeader*)headerBlock)->expandList;
    	block_size_t	i;
		if ( list != NULL )
		{
			for ( i = 1; i < list->count; i++ )
			{
    			SASLock(list->heap[i], SasUserLock__WRITE);
			}
			
    		heapFree = SASIndexFreeSpaceNoLock(heap);
    		
			for ( i = 1; i < list->count; i++ )
			{
    			SASUnlock(list->heap[i]);
			}
		} else {
    		heapFree = SASIndexFreeSpaceNoLock(heap); 
		}
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexFreeSpace(%p) type check failed\n", heap);
#endif
    }
    return heapFree;
}

void 
SASIndexDestroyNoLock (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    block_size_t	heapSize;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASCompoundExpandList	*list = headerBlock->expandList;
    	block_size_t	i;
		heapSize = headerBlock->blockHeader.blockSize;
		if ( list != NULL )
		{
			for ( i = 1; i < list->count; i++ )
			{
				SASBlockDealloc (list->heap[i], heapSize);
				list->heap[i] = NULL;
			}
			list->max_count = 1;
		}
		SASBlockDealloc (heap, heapSize);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexDestroyNoLock(%p) type check failed\n", heap);
#endif
    }
}

void 
SASIndexDestroy (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    block_size_t	heapSize;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
    	SASCompoundExpandList	*list = headerBlock->expandList;
    	block_size_t	i;
		heapSize = headerBlock->blockHeader.blockSize;
		if ( list != NULL )
		{
			for ( i = 1; i < list->count; i++ )
			{
				SASLock (list->heap[i], SasUserLock__WRITE);
			}
			for ( i = 1; i < list->count; i++ )
			{
				SASBlockDealloc (list->heap[i], heapSize);
			}
			for ( i = 1; i < list->count; i++ )
			{
				SASUnlock (list->heap[i]);
				list->heap[i] = NULL;
			}
			list->max_count = 1;
		}
		SASBlockDealloc (headerBlock, heapSize);
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexDestroy(%p) block check failed\n", heap);
#endif
    }
}

/******************************************************************/

SASIndexNode_t 
SASIndexGetRootNode(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexNode_t	result	= NULL;
	
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
    	result = btree->root;
		SASUnlock(heap);
    }
	return result;
};
SASIndexNode_t
SASIndexGetRootNode_nolock(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexNode_t	result	= NULL;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
    	result = btree->root;
    }
	return result;
};

long
SASIndexGetModCount(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	long	result	= 0L;
	
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
    	result = btree->common->modCount;
		SASUnlock(heap);
    }
	return result;
}

long
SASIndexGetModCount_nolock(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	long	result	= 0L;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
    	result = btree->common->modCount;
    }
	return result;
}

SASIndexKey_t*
SASIndexGetMaxKey(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexKey_t	*result	= NULL;
	
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
    	result = btree->common->max_key;
		SASUnlock(heap);
    }
	return result;
}

SASIndexKey_t*
SASIndexGetMaxKey_nolock(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexKey_t	*result	= NULL;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
    	result = btree->common->max_key;
    }
	return result;
}

SASIndexKey_t*
SASIndexGetMinKey(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexKey_t	*result	= NULL;
	
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
    	result = btree->common->min_key;
		SASUnlock(heap);
    }
	return result;
};

SASIndexKey_t*
SASIndexGetMinKey_nolock(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexKey_t	*result	= NULL;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
    	result = btree->common->min_key;
    }
	return result;
};

int
SASIndexContainsKey (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	__IDXnodePosRef ref = {NULL, 0};
	int found = false;
	
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
		if (btree->root != NULL)
		{
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		}
		SASUnlock(heap);
    }
	return found;
}

int
SASIndexContainsKey_nolock (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	__IDXnodePosRef ref = {NULL, 0};
	int found = false;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
		if (btree->root != NULL)
		{
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		}
    }
	return found;
}

void*
SASIndexGet (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	void *result = NULL;
	__IDXnodePosRef ref = {NULL, 0};
	int found;	
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
		if (btree->root != NULL)
		{
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    if (found)
		    {
				result = SASIndexNodeGetValIndexed(ref.node, ref.pos);
		    }
		}
		SASUnlock(heap);
    }
	return result;
}

void*
SASIndexGet_nolock (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	void *result = NULL;
	__IDXnodePosRef ref = {NULL, 0};
	int found;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
		if (btree->root != NULL)
		{
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    if (found)
		    {
				result = SASIndexNodeGetValIndexed(ref.node, ref.pos);
		    }
		}
    }
	return result;
}

int
SASIndexIsEmpty(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    int	result = false;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__READ);
    	result = (btree->common->count == 0);
		SASUnlock(heap);
    }
	return result;
}

int
SASIndexIsEmpty_nolock(SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    int	result = false;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
    	result = (btree->common->count == 0);
    }
	return result;
}

static inline void
SASIndexHeaderDealloc (SASIndex_t  heap, 
                             void *memAddr, block_size_t size)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
  	
    freeNode_deallocSpace(((freeNode *)memAddr), &btree->headerFreeSpace, size);
}

static inline void *
SASIndexHeaderAlloc (SASIndex_t  heap, block_size_t size)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    void	*result = NULL;
    
    if (btree->headerFreeSpace)
    {
		result = freeNode_allocSpace(btree->headerFreeSpace, &btree->headerFreeSpace, size);
    }
    return result;
}

static inline void
SASIndexUpdateMax (SASIndex_t  heap, SASIndexKey_t* key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    SASIndexKey_t 	*oldkey	= btree->common->max_key;
    SASIndexKey_t	*tempkey;
	size_t		key_len;
	
	if (key != NULL)
	{
		key_len = SASIndexKeySize(key);
	    tempkey	= (SASIndexKey_t*)SASIndexHeaderAlloc(heap, key_len);
	    SASIndexKeyCopy(tempkey, key);
	    btree->common->max_key = tempkey;
    } else {
    	btree->common->max_key = NULL;
    }
    
    if (oldkey != NULL)
    {
		key_len = SASIndexKeySize(oldkey);
    	SASIndexHeaderDealloc(heap, oldkey,  key_len);
    }
}

static inline void
SASIndexUpdateMin (SASIndex_t  heap, SASIndexKey_t* key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    SASIndexKey_t 	*oldkey	= btree->common->min_key;
    SASIndexKey_t	*tempkey;
	size_t		key_len;

	if (key != NULL)
	{
		key_len = SASIndexKeySize(key);
	    tempkey	= (SASIndexKey_t*)SASIndexHeaderAlloc(heap, key_len);
	    SASIndexKeyCopy(tempkey, key);
	    btree->common->min_key = tempkey;
    } else {
    	btree->common->min_key = NULL;
    }
    
    if (oldkey != NULL)
    {
		key_len = SASIndexKeySize(oldkey);
    	SASIndexHeaderDealloc(heap, oldkey,  key_len);
    }
}

int
SASIndexPut (SASIndex_t  heap, SASIndexKey_t *key, void *value)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    int result = false;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
		
		if (btree->root != NULL)
		{
			SASIndexNode_t node;
		    node = SASIndexNodeInsert(btree->root, key, value, LOCK_OFF);
		    if (node != NULL)
			{
			    btree->root = node;
				btree->common->modCount++;
			    if ( SASIndexKeyCompare(key, btree->common->min_key) < 0 ) 
			    	SASIndexUpdateMin(heap, key);
			    if ( SASIndexKeyCompare(key, btree->common->max_key) > 0 ) 
			    	SASIndexUpdateMax(heap, key);
			    result = true;
		    } 
		} else {
		    btree->root = SASIndexAlloc(heap);
		    SASIndexNodeInitialize(btree->root, key, value, LOCK_OFF);
		    SASIndexUpdateMin(heap, key);
		    SASIndexUpdateMax(heap, key);
			btree->common->modCount++;
			result = true;
		};
		btree->common->count++;
		SASUnlock(heap);
    }
	return result; /* False indicates duplicate key */
}

int
SASIndexPut_nolock (SASIndex_t  heap, SASIndexKey_t *key, void *value)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    int result = false;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {

		if (btree->root != NULL)
		{
			SASIndexNode_t node;
		    node = SASIndexNodeInsert(btree->root, key, value, LOCK_OFF);
		    if (node != NULL)
			{
			    btree->root = node;
				btree->common->modCount++;
			    if ( SASIndexKeyCompare(key, btree->common->min_key) < 0 )
			    	SASIndexUpdateMin(heap, key);
			    if ( SASIndexKeyCompare(key, btree->common->max_key) > 0 )
			    	SASIndexUpdateMax(heap, key);
			    result = true;
		    }
		} else {
		    btree->root = SASIndexAllocNoLock(heap);
		    SASIndexNodeInitialize(btree->root, key, value, LOCK_OFF);
		    SASIndexUpdateMin(heap, key);
		    SASIndexUpdateMax(heap, key);
			btree->common->modCount++;
			result = true;
		};
		btree->common->count++;
    }
	return result; /* False indicates duplicate key */
}

void *
SASIndexReplace (SASIndex_t  heap, SASIndexKey_t *key, void *value)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	__IDXnodePosRef ref = {NULL, 0};
    void	*result = NULL;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
		btree->common->modCount++;
		
		if (btree->root != NULL)
		{
			int found;
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    //found = getNode(key, &ref);
		    if (found)
		    {
				result = SASIndexNodePutValIndexed(ref.node, ref.pos, value);
		    }
		}
		btree->common->count++;
		SASUnlock(heap);
    }
	return result; // return prev value if already exist
}

void *
SASIndexReplace_nolock (SASIndex_t  heap, SASIndexKey_t *key, void *value)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	__IDXnodePosRef ref = {NULL, 0};
    void	*result = NULL;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
		btree->common->modCount++;

		if (btree->root != NULL)
		{
			int found;
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    //found = getNode(key, &ref);
		    if (found)
		    {
				result = SASIndexNodePutValIndexed(ref.node, ref.pos, value);
		    }
		}
		btree->common->count++;
    }
	return result; // return prev value if already exist
}

void *
SASIndexRemove (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexNode_t newRoot;
	__IDXnodePosRef ref = {NULL, 0};
    void	*result = NULL;
    SASIndexNodeHeader *node;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
		btree->common->modCount++;
		
		if (btree->root != NULL)
		{
			int found;
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    //found = getNode(key, &ref);
		    if (found)
		    {
				result = SASIndexNodeGetValIndexed(ref.node, ref.pos);
		    }
			  
		    newRoot = SASIndexNodeDelete(btree->root, key, LOCK_OFF);
		    if ( newRoot != btree->root )
		    {   //Delete the old root which is empty
				SASIndexNearDealloc(btree->root);
				btree->root = newRoot;
		    }
		    if ( btree->root != NULL )
		    {
				btree->common->count--;
				if (btree->common->count > 0)
				{
				    if(SASIndexKeyCompare(key, btree->common->min_key) == 0)
				    {
				        node = (SASIndexNodeHeader *) btree->root;
				        if (node->branch[0] != NULL)
				        {
				            node = node->branch[0];
				        }
				        SASIndexUpdateMin(heap, node->keys[1]);
				    }
				    if(SASIndexKeyCompare(key, btree->common->max_key) == 0)
				    {
				        node = (SASIndexNodeHeader *) btree->root;
				        if(node->branch[node->count] != NULL)
				        {
				            node = node->branch[node->count];
				        }
				        SASIndexUpdateMax(heap ,node->keys[(node->count)]);
				    }
				}
		    } else {
				btree->common->count = 0;
				SASIndexUpdateMax(heap, NULL);
				SASIndexUpdateMin(heap, NULL);
		    }
		} else {
		    btree->common->count = 0;
		}
		SASUnlock(heap);
    }
	return result; // return prev value if already exist
}

void *
SASIndexRemove_nolock (SASIndex_t  heap, SASIndexKey_t *key)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
	SASIndexNode_t newRoot;
	__IDXnodePosRef ref = {NULL, 0};
    void	*result = NULL;
    SASIndexNodeHeader *node;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap,
                                    SAS_RUNTIME_INDEX))
    {
		btree->common->modCount++;

		if (btree->root != NULL)
		{
			int found;
		    found = SASIndexNodeSearch(btree->root, key, &ref);
		    //found = getNode(key, &ref);
		    if (found)
		    {
				result = SASIndexNodeGetValIndexed(ref.node, ref.pos);
		    }

		    newRoot = SASIndexNodeDelete(btree->root, key, LOCK_OFF);
		    if ( newRoot != btree->root )
		    {   //Delete the old root which is empty
				SASIndexNearDeallocNoLock(btree->root);
				btree->root = newRoot;
		    }
		    if ( btree->root != NULL )
		    {
				btree->common->count--;
				if (btree->common->count > 0)
				{
				    if(SASIndexKeyCompare(key, btree->common->min_key) == 0)
				    {
				        node = (SASIndexNodeHeader *) btree->root;
				        if (node->branch[0] != NULL)
				        {
				            node = node->branch[0];
				        }
				        SASIndexUpdateMin(heap, node->keys[1]);
				    }
				    if(SASIndexKeyCompare(key, btree->common->max_key) == 0)
				    {
				        node = (SASIndexNodeHeader *) btree->root;
				        if(node->branch[node->count] != NULL)
				        {
				            node = node->branch[node->count];
				        }
				        SASIndexUpdateMax(heap ,node->keys[(node->count)]);
				    }
				}
		    } else {
				btree->common->count = 0;
				SASIndexUpdateMax(heap, NULL);
				SASIndexUpdateMin(heap, NULL);
		    }
		} else {
		    btree->common->count = 0;
		}
    }
	return result; // return prev value if already exist
}

#if 0
void 
SASIndexPrint (SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX) )
    {
    	SASLock(heap, SasUserLock__READ);
    	if (btree->root != NULL)
			SASIndexNodePrint(btree->root);
		else 	
    		sas_printf("<empty>");
    		
		SASUnlock(heap);
	}
	sas_printf("\n");
};

void
SASIndexPrintValues (SASIndex_t  heap)
{
    SASIndexHeader	*btree = (SASIndexHeader*)heap;
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)heap, 
                                    SAS_RUNTIME_INDEX) )
    {
    	SASLock(heap, SasUserLock__READ);
    	if (btree->root != NULL)
			SASIndexNodePrintValue(btree->root);
		else 	
    		sas_printf("<empty>");
    		
		SASUnlock(heap);
	}
	sas_printf("\n");
}

void
SASIndexNodePrintStatsPriv (SASIndexNode_t heap)
{
	SASIndexNodeHeader *node = (SASIndexNodeHeader*)heap;
	SASIndexNode_t	spill_t;
	SASIndexKey_t	*key_str;
	size_t	key_len;
	char *ptr;
	char *end_ptr;
    short	c;
    short	near_count, far_count;
    int		near_sum, far_sum;
    size_t	max_frag = SASIndexNodeMaxFragmentNoLock(heap);
	
	sas_printf("  node@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
				heap, 
				SASIndexNodeFreeSpaceNoLock(heap),
				SASIndexNodeFreeFragmentsNoLock(heap),
				SASIndexNodeMaxFragmentNoLock(heap));
	
    
	ptr = (char*)node;
	end_ptr = ptr + node->blockHeader.blockSize;
	near_count = 0;
	far_count = 0;
	near_sum = 0;
	far_sum = 0;
    for ( c = 1; c <= node->count; c++ )
    {
        key_str = node->keys[c];
		key_len = SASIndexKeySize(key_str);
		if (((unsigned long)key_str >= (unsigned long)ptr)
		&&  ((unsigned long)key_str < (unsigned long)end_ptr))
		{
			near_count++;
			near_sum += key_len;
		}
		else
		{
			if ( key_len < max_frag )
			{
				sas_printf("  node@%p[%hd], far=%p\n",
							heap, c, key_str);
			}
			far_count++;
			far_sum += key_len;
		}
    }
	
	sas_printf("   %d keys: %d near total %d,  %d far total %d\n",
				node->count, near_count, near_sum, far_count, far_sum);
    
	if (node->spill != NULL)
	{
		spill_t = (SASIndexNode_t)node->spill;
		
    	sas_printf("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
    				spill_t, 
    				SASIndexNodeFreeSpaceNoLock(spill_t),
    				SASIndexNodeFreeFragmentsNoLock(spill_t),
    				SASIndexNodeMaxFragmentNoLock(spill_t));
	}
	if (node->spill2 != NULL)
	{
		spill_t = (SASIndexNode_t)node->spill2;
		
    	sas_printf("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
    				spill_t, 
    				SASIndexNodeFreeSpaceNoLock(spill_t),
    				SASIndexNodeFreeFragmentsNoLock(spill_t),
    				SASIndexNodeMaxFragmentNoLock(spill_t));
	}
	if (node->spill3 != NULL)
	{
		spill_t = (SASIndexNode_t)node->spill3;
		
    	sas_printf("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu\n",
    				spill_t, 
    				SASIndexNodeFreeSpaceNoLock(spill_t),
    				SASIndexNodeFreeFragmentsNoLock(spill_t),
    				SASIndexNodeMaxFragmentNoLock(spill_t));
	}
}

#ifndef nodeAlign
#define nodeAlign (2*sizeof(void*))
#define nodeRound (2*sizeof(void*)-1)
#endif

void
SASIndexNodeStatsPriv (SASIndexNode_t heap,
                             long *key_count,
                             long *key_total,
                             long *near_key_count,
                             long *near_key_total,
                             long *far_key_count,
                             long *far_key_total,
                             long *fragment_count,
                             long *free_total)
{
	SASIndexNodeHeader *node = (SASIndexNodeHeader*)heap;
	SASIndexKey_t	*key_str;
	int		key_len;
	char *ptr;
	char *end_ptr;
    short	c;
    
	*fragment_count += SASIndexNodeFreeFragmentsNoLock(heap);
	*free_total += SASIndexNodeFreeSpaceNoLock(heap);
    
	ptr = (char*)node;
	end_ptr = ptr + node->blockHeader.blockSize;
	*key_count += node->count;
	
    for ( c = 1; c <= node->count; c++ )
    {
        key_str = node->keys[c];
		key_len = SASIndexKeySize(key_str);
		key_len = ((key_len + nodeRound ) / nodeAlign ) * nodeAlign ;
		*key_total += key_len;
		if (((unsigned long)key_str >= (unsigned long)ptr)
		&&  ((unsigned long)key_str < (unsigned long)end_ptr))
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
SASIndexPrintStatsPriv (SASIndexHeader *compoundHeader)
{
    SASIndexSpillList	*spill_lst;
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
	
	sas_printf("SASIndexPrintStats(%p) freespace=%zu\n",
				compoundHeader,
				SASIndexFreeSpace(compoundHeader));
	ptr = (char*)compoundHeader;
	end_ptr = ptr + compoundHeader->blockHeader.blockSize;
	ptr = ptr + default_page;
	psize = compoundHeader->pageSize;
	sas_printf(" compound heap %p - %p + %zu\n", ptr, end_ptr, psize);
	
	spill_lst = compoundHeader->spillList;
	for (int i=0; i<spill_lst->count; i++)
	{
		const char	*near;
		SASIndexNode_t	spill_t;
		spill_t = SASIndexNodeVerify(spill_lst->spillHeap[i]);
		
		if (((unsigned long)spill_t >= (unsigned long)ptr)
		&&  ((unsigned long)spill_t < (unsigned long)end_ptr))
			near = "near";
		else
			near = "far";
	    		
    	sas_printf("   spill@%p, freespace=%zu, fragments=%zu, max_free_block=%zu %s\n",
    				spill_t, 
    				SASIndexNodeFreeSpaceNoLock(spill_t),
    				SASIndexNodeFreeFragmentsNoLock(spill_t),
    				SASIndexNodeMaxFragmentNoLock(spill_t),
    				near);
	}
	
	for (; (unsigned long)ptr < (unsigned long)end_ptr; ptr+=psize)
	{
		SASIndexNode_t	heap;
		heap = SASIndexNodeVerify((SASIndexNode_t)ptr);
		
		if (heap != NULL)
		{
			if (!SASIndexNodeIsSpill(heap))
		    {
		    	SASIndexNodePrintStatsPriv (heap);
				SASIndexNodeStatsPriv (heap,
                             &key_count,
                             &key_total,
                             &near_key_count,
                             &near_key_total,
                             &far_key_count,
                             &far_key_total,
                             &fragment_count,
                             &free_total);
		    } else {
		    	/*
	    		spill_t = (SASIndexNode_t)node;
	    		
		    	sas_printf("  spill@%p, freespace=%d, fragments=%d, max_free_block=%d\n",
		    				spill_t, 
		    				SASIndexNodeFreeSpaceNoLock(spill_t),
		    				SASIndexNodeFreeFragmentsNoLock(spill_t),
		    				SASIndexNodeMaxFragmentNoLock(spill_t));
		    				*/
		    }
		}
	}
	
	sas_printf("  totals keys=%ld/%ld, near=%ld/%ld, far=%ld/%ld, free=%ld/%ld\n",
                             key_count,
                             key_total,
                             near_key_count,
                             near_key_total,
                             far_key_count,
                             far_key_total,
                             fragment_count,
                             free_total);
}

void
SASIndexStatsPriv (SASIndexHeader *compoundHeader,
                         long *key_count,
                         long *key_total,
                         long *near_key_count,
                         long *near_key_total,
                         long *far_key_count,
                         long *far_key_total,
                         long *fragment_count,
                         long *free_total,
                         long *spill_near_count,
                         long *spill_far_count,
                         long *spill_free_total)
{
    SASIndexSpillList	*spill_lst;
	char *ptr;
	char *end_ptr;
	block_size_t psize;
	/*
	sas_printf("SASIndexPrintStats(%p) freespace=%d\n",
				compoundHeader,
				SASIndexFreeSpace(compoundHeader));
	*/
	ptr = (char*)compoundHeader;
	end_ptr = ptr + compoundHeader->blockHeader.blockSize;
	ptr = ptr + default_page;
	psize = compoundHeader->pageSize;
	
	spill_lst = compoundHeader->spillList;
	for (int i=0; i<spill_lst->count; i++)
	{
		SASIndexNode_t	spill_t;
		spill_t = SASIndexNodeVerify(spill_lst->spillHeap[i]);
		
		if (((unsigned long)spill_t >= (unsigned long)ptr)
		&&  ((unsigned long)spill_t < (unsigned long)end_ptr))
			*spill_near_count += 1;
		else
			*spill_far_count += 1;
			
		*spill_free_total += SASIndexNodeFreeSpaceNoLock(spill_t);
	}
	for (; (unsigned long)ptr < (unsigned long)end_ptr; ptr+=psize)
	{
		SASIndexNode_t	heap;
		heap = SASIndexNodeVerify((SASIndexNode_t)ptr);
		
		if (heap != NULL)
		{
			if (!SASIndexNodeIsSpill(heap))
		    {
				SASIndexNodeStatsPriv (heap,
                             key_count,
                             key_total,
                             near_key_count,
                             near_key_total,
                             far_key_count,
                             far_key_total,
                             fragment_count,
                             free_total);
		    }
		}
	}
}

void 
SASIndexPrintStats (SASIndex_t heap)
{
    SASIndexHeader	*headerBlock = (SASIndexHeader*)heap;
    block_size_t	heapSize;
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
    
    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                    SAS_RUNTIME_INDEX))
    {
    	SASLock(heap, SasUserLock__WRITE);
    	SASCompoundExpandList	*list = headerBlock->expandList;
    	block_size_t	i;
		heapSize = headerBlock->blockHeader.blockSize;
		SASIndexPrintStatsPriv (headerBlock);
		if ( list != NULL )
		{
			for ( i = 1; i < list->count; i++ )
			{
				SASLock (list->heap[i], SasUserLock__WRITE);
			}
			for ( i = 1; i < list->count; i++ )
			{
				SASIndexPrintStatsPriv (list->heap[i]);
				SASIndexStatsPriv (list->heap[i],
                             &key_count,
                             &key_total,
                             &near_key_count,
                             &near_key_total,
                             &far_key_count,
                             &far_key_total,
                             &fragment_count,
                             &free_total,
                             &spill_near_count,
                             &spill_far_count,
                             &spill_free_total);
			}
			for ( i = 1; i < list->count; i++ )
			{
				SASUnlock (list->heap[i]);
			}
			list->max_count = 1;
		}
		SASUnlock(heap);
		sas_printf("totals keys=%ld/%ld, near=%ld/%ld, far=%ld/%ld,\n       free=%ld/%ld, spill=%ld/%ld/%ld\n",
                             key_count,
                             key_total,
                             near_key_count,
                             near_key_total,
                             far_key_count,
                             far_key_total,
                             fragment_count,
                             free_total,
                             spill_near_count,
                             spill_far_count,
                             spill_free_total);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASIndexPrintStats(%p) block check failed\n", heap);
#endif
    }
}
#endif
