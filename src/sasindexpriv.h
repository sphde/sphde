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

#ifndef __SAS_INDEX_PRIVH
#define __SAS_INDEX_PRIVH

#include <sastype.h>
#include <freenode.h>
#include "sasindex.h"
#include "sasio.h"
#include "sasindexkey.h"
#include "sasindexnode.h"

#define DEFAULT_LOAD_FACTOR 75

struct SASIndexHeader;
struct SASIndexNodeHeader;

#define SASBTREE_EXPANDLIST_SIZE	254
typedef struct SASCompoundExpandList
{
  block_size_t count;
  block_size_t max_count;
  struct SASIndexHeader *heap[SASBTREE_EXPANDLIST_SIZE];
} SASCompoundExpandList;

#define SASBTREE_SPILLLIST_SIZE		15
typedef struct SASIndexSpillList
{
  unsigned short count;
  unsigned short max_count;
  struct SASIndexNodeHeader *spillHeap[SASBTREE_SPILLLIST_SIZE];
} SASIndexSpillList;

typedef struct SASIndexCommon
{
  unsigned int version;
  long modCount;
  long count;
  SASIndexKey_t *max_key;
  SASIndexKey_t *min_key;
} SASIndexCommon;

typedef struct SASIndexHeader
{
  SASBlockHeader blockHeader;
  block_size_t pageSize;
  SASIndexNode_t root;
  SASCompoundExpandList *expandList;
  SASIndexCommon *common;
  SASIndexSpillList *spillList;
  void *dummy6;
  void *dummy7;
  FreeNode *headerFreeSpace;
} SASIndexHeader;

static inline SASIndex_t
SASIndexVerify (SASIndex_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SASIndex_t btreeNode = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_COMPOUNDHEAP_TYPE))
    {
      btreeNode = (SASIndex_t) headerBlock;
    }
  return btreeNode;
}

static inline SASIndexSpillList *
SASIndexGetSpill (SASIndexHeader * headerBlock)
{
  return (headerBlock->spillList);
}

static inline SASIndexSpillList *
SASIndexGetSpillNear (void *nearObj)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  SASIndexHeader *btreeHeader = NULL;
  SASIndexSpillList *list = NULL;
  if (nearHeader != NULL)
    {
      if ((nearHeader->baseBlock != nearHeader)
	  && (nearHeader->baseBlock != NULL))
	btreeHeader = (SASIndexHeader *) nearHeader->baseBlock;
      else
	btreeHeader = (SASIndexHeader *) nearHeader;

      if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) btreeHeader,
				      SAS_RUNTIME_INDEX))
	{
	  list = btreeHeader->spillList;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASIndexGetSpillNear(%p) type check failed near=%p compound=%p\n",
	     nearObj, nearHeader, btreeHeader);
#endif
	}
    }
  return list;
}

/*!
 * \brief Internal function to allocate a new SASIndexNode_t
 * for SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a write
 * lock over B-Tree \a btree.
 * The created SASIndexNode_t can be modified using the functions at
 * sasindexnode.h.
 *
 * @param btree Handle to the SASIndex_t.
 * @return A new SASIndexNode_t handle or 0 if creation fails.
 */
extern __C__ SASIndexNode_t
SASIndexAlloc (SASIndex_t btree);

/*!
 * \brief Internal function to return a SASIndexNode_t \a free_block
 * to the free space of B-tree \a btree for later reuse as a node.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The functions holds a write
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASIndex_t.
 * @param free_block address to the SASIndexNode_t to be freed.
 */
extern __C__ void
SASIndexFree (SASIndex_t btree, SASIndexNode_t free_block);

/*!
 * \brief Internal function to allocate a new SASIndexNode_t from
 * the containing SAS B-Tree near an existing \a nearObj.
 *
 * The function is similar to ::SASStringBTreeAlloc but accepts a memory
 * address of another SASStringBTreeNode_t from within the containing
 * SASStringBTree_t.
 *
 * \todo I wounder if nearObj should be SASIndexNode_t to made the
 * usage/intent clear.
 *
 * @param nearObj Memory address of the SASIndex_t handler.
 * @return A new SASIndexNode_t handle or 0 if creation fails.
 */
extern __C__ SASIndexNode_t
SASIndexNearAlloc (void *nearObj);

/*!
 * \brief Internal function to free the SASIndexNode_t at \a memAddr
 * from its associated SASIndex_t B-Tree block.
 *
 * The Function is similar to ::SASIndexFree but the \a memAddr is used
 * as the SASIndexNode_t and the associated containing SASIndex_t
 * address is calculated from the node.
 *
 * @param memAddr Memory address associated with the SASIndexNode_t to
 * be freed.
 */
extern __C__ void
SASIndexNearDealloc (void *memAddr);

/*!
 * \brief Internal function that returns the root SASIndexNode_t node
 * for \a btree B-Tree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The functions holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASIndex_t.
 * @return The root node or 0 if no root is available or if an error occurs.
 */
extern __C__ SASIndexNode_t
SASIndexGetRootNode (SASIndex_t btree);

/*!
 * \brief Internal function that returns the root SASIndexNode_t node
 * for \a btree B-Tree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The functions holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASIndex_t.
 * @return The root node or 0 if no root is available or if an error occurs.
 */
extern __C__ SASIndexNode_t
SASIndexGetRootNode_nolock (SASIndex_t btree);

extern SASIndexNode_t
SASIndexSpillAlloc (void *nearObj, lock_on_t lock_on);

extern void
SASIndexDestroyNoLock (SASIndex_t btree);

extern block_size_t
SASIndexFreeSpaceNoLock (SASIndex_t btree);

extern SASIndexNode_t
SASIndexAllocNoLock (SASIndex_t btree);

extern SASIndexNode_t
SASIndexNearAllocNoLock (void *nearObj);

extern void
SASIndexNearDeallocNoLock (void *memAddr);

extern void
SASIndexFreeNoLock (SASIndex_t btree, SASIndexNode_t free_block);

#endif /* __SAS_INDEX_PRIVH */
