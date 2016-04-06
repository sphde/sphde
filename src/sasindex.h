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


#ifndef __SAS_INDEX_H
#define __SAS_INDEX_H

#include "sasindexkey.h"
#include "sasindexnode.h"

/*!
 * \file  sasindex.h
 * \brief Shared Address Space B-tree based on binary values including
 * virtual addresses.
 *
 * Allocate SAS block storage and manage it as a B-tree data structure
 * using SASIndexKey_t struct as keys which are associated address
 * value. This provides "map" (or association) between a binary value
 * and an arbitrary address (an block or SPH object in SAS storage).
 * One expected usage is to provide the reverse mapping from the address
 * of a SAS block or SPH object back to its human readable label stored
 * in a SASStringBTree_t. This is in fact the mechanism used by the
 * SPHContext_t implementation.
 *
 * The Index B-tree keeps the binary key data sorted and allows
 * search/insert/delete in logarithmic time. The B-Tree is organized
 * into page-size nodes to improve storage locality for search and
 * minimize paging when very large B-Trees are needed.
 *
 * A new Index can be constructed using the functions ::SASIndexCreate,
 * ::SASIndexExpandCreate, or ::SASIndexCreatePageSize. The
 * functions differs for the options provides. A SAS block can be initialized
 * as a SBT by using the function ::SASIndexInit.
 *
 * \code
 * SASIndex_t indexBTree;
 * SASIndexKey_t mykey;
 * char *myorigdata, *myassocdata;
 *
 * indexBTree = SASIndexCreate (blockSize);
 * if (indexBTree)
 *   {  // Use index to associate addresses of myassocdata with myorigdata
 *     SASIndexKeyInitRef(&mykey, myorigdata);
 *     rc = SASIndexPut (indexBTree, &mykey, myassocdata);
 *     if (rc)
 *       {
 *         printf("Associated @%p with @%p in BTree@%p",
 *           myassocdata, myorigdata, stringBTree);
 *      }
 *   }
 * \endcode
 *
 * The helper functions ::SASIndexPut, ::SASIndexReplace, and
 * ::SASIndexRemove can be used to insert, replace and remove a
 * string/pointer tuple from SBT. Others useful functions are
 * ::SASIndexContainsKey, which returns if a key exists; and
 * ::SASIndexGet, which returns the value of a key.
 *
 * The enumeration API from sasindexenum.h can be use to iterate
 * over the (in whole or part) of contents of BTree in key order.
 *
 * The functions above apply SASLock and SASUnlock around each Index
 * operation to insure consistency of the Index.
 *
 * If at process needs exclusive access or needs to scan or populate an
 * Index quickly, the application can SASLock the SASIndex_t, then use
 * the *_nolock forms of the function above for faster access.
 * \code
 * SASIndex_t indexBTree;
 * SASIndexEnum_t senum;
 * unsigned long long *keyref;
 *
 * SASLock (indexBTree, SasUserLock__READ);
 * senum = SASIndexEnumCreate (index);
 * if (!senum)
 *   {
 *     printf ("SASIndexEnumCreate (%p) failed", index);
 *     return 1;
 *  }
 *
 * while (SASIndexEnumHasMore (senum))
 *  {
 *    keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
 *    if (keyref)
 *      {
 *      // process reference value associated with next enum
 *	    }
 *   }
 * SASUnlock (indexBTree);
 * \endcode
 *
 * Finally, a created SAS binary B-Tree \a SASIndex_t can be destroy
 * with ::SASIndexDestroy.
 */

/*!
 * \brief Handle to an instance of binary index B-tree.
 *
 * The type is SAS_RUNTIME_INDEX
 */
typedef void *SASIndex_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Create a fixed SAS B-Tree of block_size size capacity.
 *
 * Create and initialize a B-Tree. The storage block must be power of two in
 * size and the SAS type returned is SAS_RUNTIME_STRINGBTREE. The internal
 * page size used is the default one defined in sasalloc.h (4096).
 *
 * @param block_size Size of the B-Tree to create.
 * @return A handle to created SASStringBtree_t or 0 if creation fails.
 */
extern __C__ SASIndex_t
SASIndexFixedCreate (block_size_t block_size);

/*!
 * \brief Internal function that creates new block of B-Tree nodes for
 * an expanding SAS B-Tree \a btree.
 *
 * Create a block and initialize it to provide additional B-Tree node
 * space to an existing expanding SAS B-Tree \a btree.
 * This block is added the internal block list of the B-Tree.
 *
 * @param btree The SAS B-Tree to be expanded by one block.
 * @return A new SASStringBtree_t handle or 0 if the block allocate or
 * initialization fails.
 */
extern __C__ SASIndex_t
SASIndexExpandCreate (SASIndex_t btree);

/*!
 * \brief Create a new expanding SAS B-Tree with \a heap_size size
 * and \a page_size node size.
 *
 * Similar to ::SASIndexCreate but with additional option to set the
 * internal node page size.
 *
 * @param block_size Size of the B-Tree to create.
 * @param page_size Size of the internal node pages.
 * @return A handle to created SASCompoundHeap_t or 0 if creation fails.
 */
extern __C__ SASIndex_t
SASIndexCreatePageSize (block_size_t block_size, block_size_t page_size);

/*!
 * \brief Create a new expanding SAS B-Tree with initial \a heap_size
 * size and default page_size for nodes.
 *
 * Create and initialize a B-Tree. The storage block must be power of two in
 * size and SAS type returned is SAS_RUNTIME_INDEX. The internal page
 * size used is the default one defined in sasalloc.h (4096).
 *
 * @param block_size Size of the B-Tree to create.
 * @return A handle to created SASIndex_t or 0 if creation fails.
 */
extern __C__ SASIndex_t
SASIndexCreate (block_size_t block_size);

/*!
 * \brief Destroy the SAS B-Tree \a btree.
 *
 * The type must be SAS_RUNTIME_INDEX. Destroy holds an exclusive
 * write lock while clearing the control blocks and freeing the SAS block
 * (or blocks for a expanding B-Tree).
 *
 * @param btree Handle of the SASIndex_t to be destroyed.
 */
extern __C__ void
SASIndexDestroy (SASIndex_t btree);

/*!
 * \brief Return the number or insert/replace/remove operations performed on
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a read
 * lock over B-Tree \a btree.
 * An initialized SAS B-Tree starts with mod count 1 and it is incremented
 * each time a insert/replace/remove operation is performed.
 *
 * @param btree Handle to the SASIndex_t.
 * @return The number of insert/replace/remove operations performed on
 * \a btree.
 */
extern __C__ long
SASIndexGetModCount (SASIndex_t btree);

/*!
 * \brief Return the number or insert/replace/remove operations performed on
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * An initialized SAS B-Tree starts with mod count 1 and it is incremented
 * each time a insert/replace/remove operation is performed.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @return The number of insert/replace/remove operations performed on
 * \a btree.
 */
extern __C__ long
SASIndexGetModCount_nolock (SASIndex_t btree);

/*!
 * \brief Return the maximum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a read
 * lock over B-Tree \a btree.
 * The maximum key is the right most entry of the right most node.
 *
 * \note this value it stored in the header of the SASIndex_t
 * initial block and should never be modified by the application.
 *
 * @param btree Handle to the SASIndex_t.
 * @return handle to maximum SASIndexKey_t from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ SASIndexKey_t *
SASIndexGetMaxKey (SASIndex_t btree);

/*!
 * \brief Return the maximum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * The maximum key is the right most entry of the right most node.
 *
 * \note this value it stored in the header of the SASIndex_t
 * initial block and should never be modified by the application.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @return handle to maximum SASIndexKey_t from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ SASIndexKey_t *
SASIndexGetMaxKey_nolock (SASIndex_t btree);

/*!
 * \brief Return the minimum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a read
 * lock over B-Tree \a btree.
 * The minimum key is the left most entry of the left most node.
 *
 * \note this value it stored in the header of the SASIndex_t
 * initial block and should never be modified by the application.
 *
 * @param btree Handle to the SASIndex_t.
 * @return handle to maximum SASIndexKey_t from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ SASIndexKey_t *
SASIndexGetMinKey (SASIndex_t btree);

/*!
 * \brief Return the minimum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * The minimum key is the left most entry of the left most node.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * \note this value it stored in the header of the SASIndex_t
 * initial block and should never be modified by the application.
 *
 * @param btree Handle to the SASIndex_t.
 * @return handle to maximum SASIndexKey_t from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ SASIndexKey_t *
SASIndexGetMinKey_nolock (SASIndex_t btree);

/*!
 * \brief Return true if the SAS B-Tree \a btree contains the key \a key.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a
 * read lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and returns true if found.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Null terminated key string to search.
 * @return 1 if the key is within \a btree or 0 otherwise.
 */
extern __C__ int
SASIndexContainsKey (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Return true if the SAS B-Tree \a btree contains the key \a key.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * This function searches the B-Tree for a matching key and returns
 * true if found.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Null terminated key string to search.
 * @return 1 if the key is within \a btree or 0 otherwise.
 */
extern __C__ int
SASIndexContainsKey_nolock (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Return the memory address value associated with \a key in
 * SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a read
 * lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * returns the associated memory address value.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key handle to maximum SASIndexKey_t key to search.
 * @return The associated memory address with \a key or 0 if the B-Tree \a
 * btree does not contain the key or if an error occurs.
 */
extern __C__ void *
SASIndexGet (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Return the memory address value associated with \a key in
 * SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * This function searches the B-Tree for a matching key and if found,
 * returns the associated memory address value.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key handle to maximum SASIndexKey_t key to search.
 * @return The associated memory address with \a key or 0 if the B-Tree \a
 * btree does not contain the key or if an error occurs.
 */
extern __C__ void *
SASIndexGet_nolock (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Return true if the SAS B-Tree \a btree is empty.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASIndex_t.
 * @return 1 if the B-Tree is not empty or 0 otherwise.
 */
extern __C__ int
SASIndexIsEmpty (SASIndex_t btree);

/*!
 * \brief Return true if the SAS B-Tree \a btree is empty.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @return 1 if the B-Tree is not empty or 0 otherwise.
 */
extern __C__ int
SASIndexIsEmpty_nolock (SASIndex_t btree);

/*!
 * \brief Add a new element \a value with key \a key in the SAS B-Tree
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a
 * write lock over B-Tree \a btree.
 * This function inserts the key and associated memory address value
 * into the B-Tree.
 * This B-Tree implementation does not allow duplicated key values.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to insert in the B-Tree.
 * @return 1 if the operation succeeds or 0 otherwise.
 * For example if the key already exist in this B-Tree.
 */
extern __C__ int
SASIndexPut (SASIndex_t btree, SASIndexKey_t * key, void *value);

/*!
 * \brief Add a new element \a value with key \a key in the SAS B-Tree
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * This function inserts the key and associated memory address value
 * into the B-Tree.
 * This B-Tree implementation does not allow duplicated key values.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to insert in the B-Tree.
 * @return 1 if the operation succeeds or 0 otherwise.
 * For example if the key already exist in this B-Tree.
 */
extern __C__ int
SASIndexPut_nolock (SASIndex_t btree, SASIndexKey_t * key, void *value);

/*!
 * \brief Replace the associated value of the element with key \a key
 * in SAS B-Tree \a btree with the value \a value.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a
 * write lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * replaces the associated memory address value with \a value, and
 * returns the previous associated value.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to replace in the B-Tree.
 * @return The address of the previous associated value for the
 * matching key, or 0 if an error occurs.
 */
extern __C__ void *
SASIndexReplace (SASIndex_t btree, SASIndexKey_t * key, void *value);

/*!
 * \brief Replace the associated value of the element with key \a key
 * in SAS B-Tree \a btree with the value \a value.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * This function searches the B-Tree for a matching key and if found,
 * replaces the associated memory address value with \a value, and
 * returns the previous associated value.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to replace in the B-Tree.
 * @return The address of the previous associated value for the
 * matching key, or 0 if an error occurs.
 */
extern __C__ void *
SASIndexReplace_nolock (SASIndex_t btree, SASIndexKey_t * key, void *value);

/*!
 * \brief Remove the key \a key and its associated value from SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a write
 * lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * removes the key and associates value from this B-Tree.
 *
 * \note removing the key and associated value from the B-Tree does not
 * remove or alter the data at that memory address. It only removes the
 * associated between the and key and the address from this B-Tree.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key value to be removed from this B-Tree.
 * @return The address of the previous item or 0 if an error occurs.
 */
extern __C__ void *
SASIndexRemove (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Remove the key \a key and its associated value from SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 * This function searches the B-Tree for a matching key and if found,
 * removes the key and associates value from this B-Tree.
 *
 * This nolock form should only be used when the referenced SASIndex_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * \note removing the key and associated value from the B-Tree does not
 * remove or alter the data at that memory address. It only removes the
 * associated between the and key and the address from this B-Tree.
 *
 * @param btree Handle to the SASIndex_t.
 * @param key Key value to be removed from this B-Tree.
 * @return The address of the previous item or 0 if an error occurs.
 */
extern __C__ void *
SASIndexRemove_nolock (SASIndex_t btree, SASIndexKey_t * key);

/*!
 * \brief Internal function to initialize storage as a B-tree.
 *
 * An internal function used to initialize the control blocks within
 * the specific storage block \a block as a SAS binary index B-tree.
 * Both \a block_size and \a page_size must be power of two in
 * size and have the same power of two (or better) alignment.
 * The SAS type created is SAS_RUNTIME_STRINGBTREE.
 *
 * @param btree_block Block of allocated SAS storage.
 * @param btree_size Size of the B-tree within the block.
 * @param page_size Size of page size to use.
 * @param expanding boolean indicates if the B-tree is expand or fixed.
 * @return A handle to the initialized SASIndex_t or 0 if an error
 * occurs.
 */
extern __C__ SASIndex_t
SASIndexInit (void *btree_block, block_size_t btree_size,
	      block_size_t page_size, int expanding);

/*!
 * \brief Return the page size used for node allocation for \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX.
 *
 * @param btree Handle the to SASIndex_t.
 * @return The page size value in bytes.
 */
extern __C__ block_size_t
SASIndexAllocSize (SASIndex_t btree);

/*!
 * \brief Return the total available node free space for \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_INDEX. The function holds a write
 * lock while calculating the total node free space from B-Tree.
 *
 * @param btree Handle to the SASIndex_t.
 * @return The total available free node space in bytes.
 */
extern __C__ block_size_t
SASIndexFreeSpace (SASIndex_t btree);

#endif /* __SAS_INDEX_H */
