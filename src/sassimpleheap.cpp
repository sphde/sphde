/*
 * Copyright (c) 2003-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#define sas_printf printf
#include <stdlib.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sassimpleheap.h"

SASSimpleHeap_t 
SASSimpleHeapInit (void* heap_seg,  sas_type_t sasType,
		block_size_t heap_size)
{
    SASBlockHeader	*heapBlock = (SASBlockHeader*)heap_seg;
    char		*heapStart = NULL;
    
    if ( heapBlock )
    {
		heapStart = (char*) heapBlock + heap_offset;
		initSOMSASBlock(heapBlock, sasType, 
		                               heap_size, heapStart);
    }

    return (SASSimpleHeap_t)heapBlock;
}

SASSimpleHeap_t
SASSimpleHeapCreate (block_size_t heap_size)
{
    SASBlockHeader	*heapBlock = NULL;
    
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)heap_size);
    if ( heapBlock )
    {
	    return SASSimpleHeapInit(heapBlock, 
	                                                 SAS_RUNTIME_SIMPLEHEAP, 
		                                             heap_size);
    } else 
	    return NULL;
}

void *
SASSimpleHeapAllocNoLock (SASSimpleHeap_t heap, 
				block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    freeNode		*mem = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_SIMPLEHEAP) )
    {
		heapSize = headerBlock->blockSize - heap_offset;
		if ( alloc_size < heapSize )
		{
		    mem = freeNode_allocSpace(headerBlock->blockFreeSpace,
		                    &headerBlock->blockFreeSpace, alloc_size);
#ifdef __SASDebugPrint__
		} else {
	    	sas_printf("SASSimpleHeapAlloc(%p, %zu) range check failed\n", 
	    	                 heap, alloc_size);
#endif
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASSimpleHeapAlloc(%p, %zu) type check failed\n",
    	          heap, alloc_size);
#endif
    }
    return mem;
}

void *
SASSimpleHeapAlloc (SASSimpleHeap_t heap, block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    void		*mem = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_SIMPLEHEAP) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	mem = SASSimpleHeapAllocNoLock(heap, alloc_size); 
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SASSimpleHeapAlloc(%p, %zu) type check failed\n",
    	          heap, alloc_size);
#endif
    }
    return mem;
}

int
SASSimpleHeapFreeNoLock (SASSimpleHeap_t heap, 
			void* free_block, 
			block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    freeNode		*free_node = (freeNode*)free_block;
    int rc;

    freeNode_init(free_node, alloc_size);
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
		heapSize = headerBlock->blockSize - heap_offset;
		if (( alloc_size < heapSize )
		&& ( (unsigned long)free_block >= ((unsigned long)headerBlock + heap_offset) ) )
		{
		    freeNode_deallocSpace(free_node, &headerBlock->blockFreeSpace, alloc_size);
		    rc = 0;
		} else {
			rc = -2;
#ifdef __SASDebugPrint__
        	sas_printf("SASSimpleHeapFree(%p, %p, %zu) range check failed\n",
        				heap, free_block, alloc_size);
#endif
		}
    } else {
			rc = -1;
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleHeapFree(%p, ...) does not match type/subtype\n",
        				heap);
#endif
    }
    return rc;
}

int
SASSimpleHeapFree (SASSimpleHeap_t heap, 
			void* free_block, 
			block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    int rc;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	rc = SASSimpleHeapFreeNoLock(heap, free_block, alloc_size);
		SASUnlock(heap);
    } else {
			rc = -1;
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleHeapFree(%p, ...) does not match type/subtype\n",
        				heap);
#endif
    }
    return rc;
}

SASSimpleHeap_t
SASSimpleHeapNearFind(void *nearObj)
{
    SASSimpleHeap_t	result = NULL;
    SASBlockHeader	*headerBlock;
	
    headerBlock = (SASBlockHeader*)SASFindHeader (nearObj);
    if (headerBlock)
    {
	if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
	{
	    result = (SASSimpleHeap_t)headerBlock;
#ifdef __SASDebugPrint__
	} else {
	    sas_printf("SASSimpleHeapNearFind(%p) doesn't match type/subtype\n",
        				nearObj);
#endif
	}
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapNearFind(%p) header not found\n",
    				nearObj);
#endif
    }
    return result;
}

void*
SASSimpleHeapNearAllocNoLock(void *nearObj, block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock;
    block_size_t	heapSize;
    freeNode		*mem = NULL;

	
    headerBlock = (SASBlockHeader*)SASFindHeader (nearObj);
    if (headerBlock)
    {
	if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
	{
	    heapSize = headerBlock->blockSize - heap_offset;
	    if ( alloc_size < heapSize )
	    {
		mem = freeNode_allocSpace(headerBlock->blockFreeSpace,
		          &headerBlock->blockFreeSpace, alloc_size);
#ifdef __SASDebugPrint__
	    } else {
		sas_printf("SASSimpleHeapNearAllocNoLock(%p, %ld) range check failed\n", 
	    	                 nearObj, alloc_size);
#endif
	    }
#ifdef __SASDebugPrint__
	} else {
	    sas_printf("SASSimpleHeapNearAllocNoLock(%p, %ld) doesn't match type/subtype\n",
        				nearObj, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapNearAllocNoLock(%p, %ld) header not found\n",
    				nearObj, alloc_size);
#endif
    }
    return mem;
}

void* SASSimpleHeapNearAlloc(void *nearObj, long allocSize)
{
    return SASNearAlloc (nearObj, allocSize);
}

void
SASSimpleHeapNearDeallocNoLock(void *nearObj, block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock;
    block_size_t	heapSize;
    freeNode		*free_node = (freeNode*)nearObj;

	
    headerBlock = (SASBlockHeader*)SASFindHeader (nearObj);
    if (headerBlock)
    {
	if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
	{
	    freeNode_init(free_node, alloc_size);
	    heapSize = headerBlock->blockSize - heap_offset;
	    if (( alloc_size < heapSize )
	    && ((unsigned long)nearObj >= ((unsigned long)headerBlock + heap_offset)))
	    {
		freeNode_deallocSpace(free_node, &headerBlock->blockFreeSpace, alloc_size);
#ifdef __SASDebugPrint__
	    } else {
		sas_printf("SASSimpleHeapNearDeallocNoLock(%p, %ld) range check failed\n", 
	    	            nearObj, alloc_size);
#endif
	    }
#ifdef __SASDebugPrint__
	} else {
	    sas_printf("SASSimpleHeapNearDeallocNoLock(%p, %ld) doesn't match type/subtype\n",
        		nearObj, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapNearDeallocNoLock(%p, %ld) header not found\n",
    		    nearObj, alloc_size);
#endif
    }    
}

void SASSimpleHeapNearDealloc(void *memAddr, long allocSize)
{							
    SASNearDealloc (memAddr, allocSize);
}

block_size_t
SASSimpleHeapFreeSpaceNoLock (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	if (headerBlock->blockFreeSpace != NULL)
    		heapFree = freeNode_freeSpaceTotal(headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapFreeSpace(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return heapFree;
}

block_size_t 
SASSimpleHeapFreeSpace (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	heapFree = SASSimpleHeapFreeSpaceNoLock(heap);
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapFreeSpace(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return heapFree;
}

int
SASSimpleHeapEmptyNoLock (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree, heapSize;
    int				empty = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	heapFree = SASSimpleHeapFreeSpaceNoLock(heap);
	heapSize = headerBlock->blockSize - heap_offset;
	empty = ( heapFree == heapSize );
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapEmptyNoLock(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return empty;
}

int
SASSimpleHeapEmpty (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree, heapSize;
    int				empty = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	heapFree = SASSimpleHeapFreeSpaceNoLock(heap);
		heapSize = headerBlock->blockSize - heap_offset;
		empty = ( heapFree == heapSize );
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleHeapEmpty(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return empty;
}

int
SASSimpleHeapDestroyNoLock (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_SIMPLEHEAP) )
    {
		heapSize = headerBlock->blockSize;
		SASBlockDealloc (heap, heapSize);
		rc = 0;
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleHeapDestroy(%p) does not match type/subtype\n",
        				heap);
#endif
		rc = -1;
    }
    return rc;
}

int
SASSimpleHeapDestroy (SASSimpleHeap_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_SIMPLEHEAP) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	rc = SASSimpleHeapDestroyNoLock(heap);
		SASUnlock(heap);
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleHeapDestroy(%p) does not match type/subtype\n",
        				heap);
#endif
		rc = -1;
    }
    return rc;
}
