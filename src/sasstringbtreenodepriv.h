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

#ifndef __SAS_STRINGBTREENODE_PRIVH
#define __SAS_STRINGBTREENODE_PRIVH

typedef struct SASStringBTreeNodeHeader {
		SASBlockHeader blockHeader;
		short			count;
		short			max_count;
		char			**keys;
		SASStringBTreeNodeHeader	**branch;
		void			**vals;
		SASStringBTreeNodeHeader	*spill;
		SASStringBTreeNodeHeader	*spill2;
		SASStringBTreeNodeHeader	*spill3;
		void			*dummy7;
		} SASStringBTreeNodeHeader;

static inline SASStringBTreeNode_t
SASStringBTreeNodeVerify (SASStringBTreeNode_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    SASStringBTreeNode_t	btreeNode = NULL;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	btreeNode =(SASStringBTreeNode_t) headerBlock;
    }
    return btreeNode;
}

extern block_size_t
SASStringBTreeNodeFreeFragmentsNoLock (SASStringBTreeNode_t heap);

extern block_size_t
SASStringBTreeNodeMaxFragmentNoLock (SASStringBTreeNode_t heap);

extern int
SASStringBTreeNodeIsSpill (SASStringBTreeNode_t heap);

extern SASStringBTreeNode_t 
SASStringBTreeSpillInit (void* heap_seg,  sas_type_t sasType,
                         block_size_t heap_size);

extern __C__ int
SASStringBTreeNodeNearDealloc(SASStringBTreeNode_t heap, void* free_block, 
                               block_size_t alloc_size);

#endif /* __SAS_STRINGBTREENODE_PRIVH */
