#ifndef __SAS_INDEXNODE_H
#define __SAS_INDEXNODE_H

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

#include "sastype.h"
#include "sasindexkey.h"

typedef void *SASIndexNode_t;

typedef int lock_on_t;
const lock_on_t LOCK_ON = 1;
const lock_on_t LOCK_OFF = 0;

typedef struct __IDXnodePosRef
{
  SASIndexNode_t node;
  short pos;
} IDXnodePosRef;

typedef struct __IDXnodeKeyRef
{
  SASIndexNode_t node;
  SASIndexKey_t *key;
  void *val;
  int dupKey;
} __IDXnodeKeyRef;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

extern __C__ SASIndexNode_t
SASIndexNodeInit (void *heap_block, sas_type_t sasType,
		  block_size_t heap_size);

extern __C__ SASIndexNode_t SASIndexNodeCreate (block_size_t heap_size);

extern __C__ int SASIndexNodeDestroy (SASIndexNode_t heap, lock_on_t lock_on);

extern __C__ block_size_t SASIndexNodeFreeSpace (SASIndexNode_t heap, lock_on_t lock_on);

extern __C__ block_size_t
SASIndexNodeFreeFragmentsNoLock (SASIndexNode_t heap);

extern __C__ block_size_t SASIndexNodeMaxFragmentNoLock (SASIndexNode_t heap);

extern __C__ void *SASIndexNodeAlloc (SASIndexNode_t heap,
				      block_size_t alloc_size, lock_on_t lock_on);

extern __C__ int
SASIndexNodeFree (SASIndexNode_t heap,
		  void *free_block, block_size_t alloc_size, lock_on_t lock_on);

extern __C__ void *SASIndexNodeNearAlloc (void *nearObj, long allocSize, lock_on_t lock_on);

extern __C__ int SASIndexNodeDestroyNoLock (SASIndexNode_t heap);

extern __C__ block_size_t SASIndexNodeFreeSpaceNoLock (SASIndexNode_t heap);

extern __C__ void *SASIndexNodeAllocNoLock (SASIndexNode_t heap,
					    block_size_t alloc_size);

extern __C__ int
SASIndexNodeFreeNoLock (SASIndexNode_t heap,
			void *free_block, block_size_t alloc_size);

extern __C__ int SASIndexNodeGetCount (SASIndexNode_t header);

extern __C__ SASIndexKey_t *SASIndexNodeGetKeyIndexed (SASIndexNode_t header,
						       short pos);

extern __C__ SASIndexNode_t
SASIndexNodeGetBranchIndexed (SASIndexNode_t header, short pos);

extern __C__ void *SASIndexNodeGetValIndexed (SASIndexNode_t header,
					      short pos);

extern __C__ void *SASIndexNodePutValIndexed (SASIndexNode_t header,
					      short pos, void *val);

extern __C__ int
SASIndexNodeSearch (SASIndexNode_t header,
		    SASIndexKey_t * target, IDXnodePosRef * ref);

extern __C__ int
SASIndexNodeSearchGT (SASIndexNode_t header,
		      SASIndexKey_t * target, IDXnodePosRef * ref);

extern __C__ int
SASIndexNodeSearchGE (SASIndexNode_t header,
		      SASIndexKey_t * target, IDXnodePosRef * ref);

extern __C__ int
SASIndexNodeSearchLT (SASIndexNode_t header,
		      SASIndexKey_t * target, IDXnodePosRef * ref);

extern __C__ int
SASIndexNodeSearchLE (SASIndexNode_t header,
		      SASIndexKey_t * target, IDXnodePosRef * ref);

extern __C__ SASIndexNode_t
SASIndexNodeInitialize (SASIndexNode_t header,
			SASIndexKey_t * newkey, void *newval, lock_on_t lock_on);

extern __C__ SASIndexNode_t
SASIndexNodeInsert (SASIndexNode_t header,
		    SASIndexKey_t * newkey, void *newval, lock_on_t lock_on);

extern __C__ SASIndexNode_t
SASIndexNodeDelete (SASIndexNode_t header, SASIndexKey_t * target,
		lock_on_t lock_on);

#endif /* __SAS_INDEXNODE_H */
