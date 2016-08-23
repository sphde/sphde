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

#ifndef __SAS_STRINGBTREE_PRIVH
#define __SAS_STRINGBTREE_PRIVH

#include <freenode.h>
#include "sasio.h"
#include <sasstringbtree.h>

#define DEFAULT_LOAD_FACTOR 75

struct SASStringBTreeHeader;
struct SASStringBTreeNodeHeader;

#define SASBTREE_EXPANDLIST_SIZE	254
typedef struct SASCompoundExpandList {
		block_size_t	count;
		block_size_t	max_count;
		struct SASStringBTreeHeader	*heap[SASBTREE_EXPANDLIST_SIZE];
		} SASCompoundExpandList;

#define SASBTREE_SPILLLIST_SIZE	15
typedef struct SASStringBTreeSpillList {
		unsigned short	count;
		unsigned short	max_count;
		struct SASStringBTreeNodeHeader	*spillHeap[SASBTREE_SPILLLIST_SIZE];
		} SASStringBTreeSpillList;

typedef struct SASStringBTreeCommon {
		unsigned int	version;
		long			modCount;
		long			count;
		char			*max_key;
		char			*min_key;
		} SASStringBTreeCommon;

typedef struct SASStringBTreeHeader {
		SASBlockHeader blockHeader;
		block_size_t	pageSize;
		SASStringBTreeNode_t			root;
		SASCompoundExpandList	*expandList;
		SASStringBTreeCommon	*common;
		SASStringBTreeSpillList	*spillList;
		void			*dummy6;
		void			*dummy7;
		FreeNode		*headerFreeSpace;
		} SASStringBTreeHeader;

static inline SASStringBTree_t
SASStringBTreeVerify (SASStringBTree_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    SASStringBTree_t	btreeNode = NULL;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_COMPOUNDHEAP_TYPE) )
    {
    	btreeNode = (SASStringBTree_t) headerBlock;
    }
    return btreeNode;
}

static inline SASStringBTreeSpillList*
SASStringBTreeGetSpill(SASStringBTreeHeader *headerBlock)
{
	return (headerBlock->spillList);
}

static inline SASStringBTreeSpillList*
SASStringBTreeGetSpillNear(void *nearObj)
{						
	SASBlockHeader *nearHeader = SASFindHeader (nearObj);
    SASStringBTreeHeader	*btreeHeader = NULL;
    SASStringBTreeSpillList	*list = NULL;
	if (nearHeader != NULL)
	{
		if ((nearHeader->baseBlock != nearHeader)
		&& (nearHeader->baseBlock != NULL))
			btreeHeader = (SASStringBTreeHeader*)nearHeader->baseBlock;
		else
			btreeHeader = (SASStringBTreeHeader*)nearHeader;
				   
	    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)btreeHeader, 
		                                SAS_RUNTIME_STRINGBTREE) )
	    {
			list = btreeHeader->spillList;
#ifdef __SASDebugPrint__
	    } else {
	    	sas_printf("SASStringBTreeGetSpillNear(%p) type check failed near=%p compound=%p\n", 
	    	           nearObj, nearHeader, btreeHeader);
#endif
	    }
	}
	return list;
}
		
extern void
SASStringBTreePrintStatsPriv (SASStringBTreeHeader *heap);

extern SASStringBTreeNode_t 
SASStringBTreeSpillAlloc(void *nearObj, lock_on_t lock_on);

#endif /* __SAS_STRINGBTREE_PRIVH */
