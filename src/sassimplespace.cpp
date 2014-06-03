/*
 * Copyright (c) 2006-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */


#define __SASDebugPrint__ 1
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
#include "sassimplespace.h"

typedef struct SASSimpleSpaceHeader {
		SASBlockHeader blockHeader;
		node_size_t		start_space;
		node_size_t		end_space;
		node_size_t		end_alloc;
		node_size_t		align_mask;
		void			*dummy4;
		void			*dummy5;
		void			*dummy6;
		freeNode		*headerFreeSpace;
		} SASSimpleSpaceHeader;

#ifdef __WORDSIZE_64
#define HEAP_OFFSET 128
#define DEFAULT_PAGE 4096
#else
#define HEAP_OFFSET 64
#define DEFAULT_PAGE 4096
#endif
#define DEFAULT_ALIGN_MASK	(~(sizeof(void*) + sizeof(void*) - 1));

SASSimpleSpace_t 
SASSimpleSpaceInitInternal (void* heap_seg,  sas_type_t sasType,
                            block_size_t block_size,
                            block_size_t space_size)
{
    SASSimpleSpaceHeader	*heapBlock = (SASSimpleSpaceHeader*)heap_seg;
    char		*spaceStart = NULL;
    char		*spaceEnd = NULL;
    char		*allocEnd = NULL;
    node_size_t		remaining;
    
    if ( heapBlock )
	initSOMSASBlock((SASBlockHeader*)heapBlock, sasType, 
		                block_size, NULL);
    
    spaceStart = (char*) heapBlock + DEFAULT_PAGE;
    spaceEnd   = (char*) heapBlock + DEFAULT_PAGE + space_size;
    allocEnd   = (char*) heapBlock + block_size;
    heapBlock->start_space = (node_size_t)spaceStart;
    heapBlock->end_space  = (node_size_t)spaceEnd;
    heapBlock->end_alloc  = (node_size_t)allocEnd;
    heapBlock->align_mask = (node_size_t)DEFAULT_ALIGN_MASK;
   
    remaining= DEFAULT_PAGE - sizeof(SASSimpleSpaceHeader);
    heapBlock->headerFreeSpace = (freeNode *)&heapBlock[1];
    freeNode_init(heapBlock->headerFreeSpace, remaining);
    heapBlock->blockHeader.baseBlock = (SASBlockHeader*)heapBlock;
    heapBlock->blockHeader.nextBlock = (SASBlockHeader*)heapBlock;

    return (SASSimpleSpace_t)heapBlock;
}

SASSimpleSpace_t 
SASSimpleSpaceInit (void* heap_seg,
		block_size_t block_size,
		block_size_t space_size)
{
    return SASSimpleSpaceInitInternal(heap_seg, 
                                      SAS_RUNTIME_SIMPLESPACE, 
	                              block_size, 
	                              space_size);
}

SASSimpleSpace_t
SASSimpleSpaceCreate (block_size_t space_size)
{
    SASBlockHeader	*heapBlock = NULL;
    block_size_t	block_size;
    block_size_t	min_size = (space_size + DEFAULT_PAGE);
    block_size_t	p2_size = (DEFAULT_PAGE + DEFAULT_PAGE);

    while (min_size > p2_size)
	p2_size = p2_size + p2_size;
    
    block_size = p2_size;
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)block_size);
    if ( heapBlock )
    {
	    return SASSimpleSpaceInit(heapBlock, block_size, space_size);
    } else 
	    return NULL;
}

void* 
SASSimpleSpaceToAddr (SASSimpleSpace_t space)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)space;
    void		*result = NULL;
    
    if ( SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                     SAS_SIMPLESPACE_TYPE) )
    {
        SASSimpleSpaceHeader	*heapBlock = (SASSimpleSpaceHeader*)space;
	result = (void*) (heapBlock->start_space);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleSpaceAddr(%p) does not match type/subtype\n",
        				space);
#endif
    }
    return result;
}

SASSimpleSpace_t
SASSimpleSpaceFromAddr (void *space)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)((char*)space
					- DEFAULT_PAGE);
    SASSimpleSpace_t	result = NULL;
    
    if ( SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                     SAS_SIMPLESPACE_TYPE) )
    {
	result = (SASSimpleSpace_t)headerBlock;
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleSpaceAddr(%p) does not match type/subtype\n",
        				space);
#endif
    }
    return result;
}

block_size_t
SASSimpleSpaceFreeSpaceNoLock (SASSimpleSpace_t space)
{
    SASSimpleSpaceHeader	*headerBlock = (SASSimpleSpaceHeader*)space;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                     SAS_SIMPLESPACE_TYPE) )
    {
		heapFree = headerBlock->end_alloc - headerBlock->end_space;
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleSpaceFreeSpace(%p) does not match type/subtype\n",
        				space);
#endif
    }
    return heapFree;
}

block_size_t 
SASSimpleSpaceFreeSpace (SASSimpleSpace_t space)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)space;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock, 
                                     SAS_SIMPLESPACE_TYPE) )
    {
    	SASLock(space, SasUserLock__WRITE);
    	heapFree = SASSimpleSpaceFreeSpaceNoLock(space);
		SASUnlock(space);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SASSimpleSpaceFreeSpace(%p) does not match type/subtype\n",
        				space);
#endif
    }
    return heapFree;
}

int
SASSimpleSpaceDestroyNoLock (SASSimpleSpace_t space)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)space;
    block_size_t	heapSize;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_SIMPLESPACE) )
    {
		heapSize = headerBlock->blockSize;
		SASBlockDealloc (space, heapSize);
		rc = 0;
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleSpaceDestroy(%p) does not match type/subtype\n",
        				space);
#endif
		rc = -1;
    }
    return rc;
}

int
SASSimpleSpaceDestroy (SASSimpleSpace_t space)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)space;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_SIMPLESPACE) )
    {
    	SASLock(space, SasUserLock__WRITE);
    	rc = SASSimpleSpaceDestroyNoLock(space);
		SASUnlock(space);
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SASSimpleSpaceDestroy(%p) does not match type/subtype\n",
        				space);
#endif
		rc = -1;
    }
    return rc;
}
