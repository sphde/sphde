#ifndef __SAS_INDEXNODE_PRIVH

/*
 * Copyright (c) 2005, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */
#define __SAS_INDEXNODE_PRIVH

#include "sasindexkey.h"

typedef struct SASIndexNodeHeader {
		SASBlockHeader blockHeader;
		short			count;
		short			max_count;
		SASIndexKey_t	**keys;
		SASIndexNodeHeader	**branch;
		void			**vals;
		SASIndexNodeHeader	*spill;
		SASIndexNodeHeader	*spill2;
		SASIndexNodeHeader	*spill3;
		void			*dummy7;
		} SASIndexNodeHeader;

static inline SASIndexNode_t
SASIndexNodeVerify (SASIndexNode_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    SASIndexNode_t	btreeNode = NULL;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	btreeNode =(SASIndexNode_t) headerBlock;
    }
    return btreeNode;
}

extern block_size_t
SASIndexNodeFreeFragmentsNoLock (SASIndexNode_t heap);

extern block_size_t
SASIndexNodeMaxFragmentNoLock (SASIndexNode_t heap);

extern int
SASIndexNodeIsSpill (SASIndexNode_t heap);

extern SASIndexNode_t 
SASIndexSpillInit (void* heap_seg,  sas_type_t sasType,
                         block_size_t heap_size);

extern __C__ int
SASIndexNodeNearDealloc(SASIndexNode_t heap, void* free_block, 
                               block_size_t alloc_size);

#endif /* __SAS_INDEXNODE_PRIVH */
