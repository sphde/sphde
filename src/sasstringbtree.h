/*
 * Copyright (c) 2004-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - bugfixes and documentation
 */

#ifndef __SAS_STRINGBTREE_H
#define __SAS_STRINGBTREE_H

#include "sasstringbtreenode.h"
/*!
 * \file  sasstringbtree.h
 * \brief Shared Address Space B-tree.
 *
 * Allocate SAS block storage and manage it as a B-tree data structure
 * using null terminate "C" strings as keys and addresses as associated
 * value. This provides "map" (or association) between a string and an
 * arbitrary address.
 * One expected usage is to provide human readable labels for arbitrary
 * data structures within SAS storage. These labels can be passed as
 * program arguments to allow programs to find data persistent elements
 * from previous sessions.
 *
 * The String B-tree keeps the string data sorted and allows
 * search/insert/delete in logarithmic time.
 * The B-Tree is organized into page-size nodes to
 * improve storage locality for search and minimize paging when very large
 * B-Trees are needed.
 *
 * A new SBT can be constructed using the functions ::SASStringBTreeCreate,
 * ::SASStringBTreeExpandCreate, or ::SASStringBTreeCreatePageSize. The
 * functions differs for the options provides. A SAS block can be initialized
 * as a SBT by using the function ::SASStringBTreeInit.
 *
 * \code
 * SASStringBTree_t stringBTree;
 *
 * stringBTree = SASStringBTreeCreate (blockSize);
 * if (stringBTree)
 *   {
 *      rc = SASStringBTreePut (stringBTree, "\mykeys\", mydata);
 *      if (rc)
 *        {
 *          printf("inserted \mykeys\ into BTree@%p for %p", stringBTree, mtdate);
 *        }
 *   }
 * \endcode
 *
 * The helper functions ::SASStringBTreePut, ::SASStringBTreeReplace, and 
 * ::SASStringBTreeRemove can be used to insert, replace and remove a 
 * string/pointer tuple from SBT. Others useful functions are 
 * ::SASStringBTreeContainsKey, which returns if a key exists; and
 * ::SASStringBTreeGet, which returns the value of a key.
 *
 * The enumeration API from sasstringbtreeenum.h can be use to iterate
 * over the (in whole or part) of contents of BTree in key order.
 *
 * The functions above apply SASLock and SASUnlock around each string
 * operation to insure consistency of the string.
 *
 * If at process needs exclusive access or needs to scan or populate a
 * string quickly, the application can SASLock the SASStringBTree_t, then use
 * the *_nolock forms of the function above for faster access.
 * \code
 * SASStringBTree_t stringBTree;
 * SASStringBTreeEnum_t senum;
 * char *keyref;
 *
 * SASLock (stringBTree, SasUserLock__READ);
 * senum = SASStringBTreeEnumCreate (stringBTree);
 * if (!senum)
 * {
 *   printf ("SASStringBTreeEnumCreate (%p) failed", stringBTree);
 *   return 1;
 * }
 *
 * while (SASStringBTreeEnumHasMore (senum))
 *   {
 *     keyref = ((char *) SASStringBTreeEnumNext_nolock (senum);
 *     if (keyref)
 *     {
 *       // process reference value associated with next enum
 *     }
 *   }
 * SASUnlock (stringBTree);
 * \endcode
 *
 * Finally, a created SAS B-Tree can be destroy with ::SASStringBTreeDestroy
 */

/*!
 * \brief Handle to an instance of String B-tree.
 *
 * The type is SAS_RUNTIME_STRINGBTREE
 */
typedef void *SASStringBTree_t;

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
extern __C__ SASStringBTree_t
SASStringBTreeFixedCreate (block_size_t block_size);

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
extern __C__ SASStringBTree_t
SASStringBTreeExpandCreate (SASStringBTree_t btree);

/*!
 * \brief Create a new expanding SAS B-Tree with initial \a heap_size
 * size and default page_size for nodes.
 *
 * Create and initialize a B-Tree. The storage block must be power of two in
 * size and SAS type returned is SAS_RUNTIME_STRINGBTREE. The internal page
 * size used is the default one defined in sasalloc.h (4096).
 *
 * @param block_size Size of the B-Tree to create.
 * @return A handle to created SASStringBTree_t or 0 if creation fails.
 */
extern __C__ SASStringBTree_t
SASStringBTreeCreate (block_size_t block_size);

/*!
 * \brief Create a new expanding SAS B-Tree with \a heap_size size
 * and \a page_size node size.
 *
 * Similar to ::SASStringBTreeCreate but with additional option to set the
 * internal node page size.
 *
 * @param block_size Size of the B-Tree to create.
 * @param page_size Size of the internal node pages.
 * @return A handle to created SASCompoundHeap_t or 0 if creation fails.
 */
extern __C__ SASStringBTree_t
SASStringBTreeCreatePageSize (block_size_t block_size, block_size_t page_size);

/*!
 * \brief Destroy the SAS B-Tree \a btree.
 *
 * The type must be SAS_RUNTIME_STRINGBTREE. Destroy holds an exclusive
 * write lock while clearing the control blocks and freeing the SAS block
 * (or blocks for a expanding B-Tree).
 *
 * @param btree Handle of the SASStringBTree_t to be destroyed.
 */
extern __C__ void
SASStringBTreeDestroy (SASStringBTree_t btree);

/*!
 * \brief Return the page size used for node allocation for \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREE.
 *
 * @param btree Handle the to SASStringBTree_t.
 * @return The page size value in bytes.
 */
extern __C__ block_size_t
SASStringBTreeAllocSize (SASStringBTree_t btree);

/*!
 * \brief Return the total available node free space for \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREE. The function holds a write
 * lock while calculating the total node free space from B-Tree.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return The total available free node space in bytes.
 */
extern __C__ block_size_t
SASStringBTreeFreeSpace (SASStringBTree_t btree);

/*!
 * \brief Internal function to allocate a new SASStringBTreeNode_t
 * for SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREE. The function holds a write
 * lock over B-Tree \a btree.
 * The created SASStringBTreeNode_t can be modified using the functions at
 * sasstringbtreenode.h.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return A new SASStringBTreeNode_t handle or 0 if creation fails.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeAlloc (SASStringBTree_t btree);

/*!
 * \brief Internal function to allocate a new SASStringBTreeNode_t from
 * the containing SAS B-Tree near an existing \a nearObj.
 *
 * The function is similar to ::SASStringBTreeAlloc but accepts a memory
 * address of another SASStringBTreeNode_t from within the containing
 * SASStringBTree_t.
 *
 * \todo I wonder if nearObj should be SASStringBTreeNode_t to made the
 * usage/intent clear.
 *
 * @param nearObj Memory address of the SASStringBTree_t handler.
 * @return A new SASStringBTreeNode_t handle or 0 if creation fails.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNearAlloc (void *nearObj);

/*!
 * \brief Internal function to return a SASStringBTreeNode_t \a free_block
 * to the free space of B-tree \a btree for later reuse as a node.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREE. The functions holds a write
 * lock over B-Tree \a btree.
 * 
 * @param btree Handle to the SASStringBTree_t.
 * @param free_block Handle to the SASStringBTreeNode_t to be freed.
 */
extern __C__ void
SASStringBTreeFree (SASStringBTree_t btree, SASStringBTreeNode_t free_block);

/*!
 * \brief Internal function to free the SASStringBTreeNode_t at \a memAddr
 * from its associated SASStringBTree_t B-Tree block.
 *
 * The Function is similar to ::SASStringBTreeFree but the \a memAddr is used
 * as the SASStringBTreeNode_t and the associated containing SASStringBTree_t
 * address is calculated from the node.
 *
 * @param memAddr Memory address associated with the SASStringBTreeNode_t to
 * be freed.
 */
extern __C__ void
SASStringBTreeNearDealloc (void *memAddr);

/*!
 * \brief Internal function that returns the root SASStringBTreeNode_t node
 * for \a btree B-Tree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREE. The functions holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return The root node or 0 if no root is available or if an error occurs.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeGetRootNode (SASStringBTree_t btree);

/*!
 * \brief Internal function that returns the root SASStringBTreeNode_t node
 * for \a btree B-Tree.
 *
 * Similar to function ::SASStringBTreeGetRootNode but the function holds no
 * lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return The root node or 0 if no root is available or if an error occurs.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeGetRootNodeNoLock (SASStringBTree_t btree);


/*!
 * \brief Return the number or insert/replace/remove operations performed on
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 * An initialized SAS B-Tree starts with mod count 1 and it is incremented
 * each time a insert/replace/remove operation is performed.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return The number of insert/replace/remove operations performed on
 * \a btree.
 */
extern __C__ long
SASStringBTreeGetModCount (SASStringBTree_t btree);

/*!
 * \brief Return the number or insert/replace/remove operations performed on
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * An initialized SAS B-Tree starts with mod count 1 and it is incremented
 * each time a insert/replace/remove operation is performed.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return The number of insert/replace/remove operations performed on
 * \a btree.
 */
extern __C__ long
SASStringBTreeGetModCount_nolock (SASStringBTree_t btree);

/*!
 * \brief Return the maximum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 * The maximum key is the right most entry of the right most node.
 *
 * \note this value it stored in the header of the SASStringBTree_t
 * initial block and should never be modified by the application.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return Null terminated maximum key string from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ char *
SASStringBTreeGetMaxKey (SASStringBTree_t btree);

/*!
 * \brief Return the maximum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * The maximum key is the right most entry of the right most node.
 *
 * \note this value it stored in the header of the SASStringBTree_t
 * initial block and should never be modified by the application.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return Null terminated maximum key string from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ char *
SASStringBTreeGetMaxKey_nolock (SASStringBTree_t btree);

/*!
 * \brief Return the minimum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 * The minimum key is the left most entry of the left most node.
 *
 * \note this value it stored in the header of the SASStringBTree_t
 * initial block and should never be modified by the application.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return Null terminated minimum key string from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ char *
SASStringBTreeGetMinKey (SASStringBTree_t btree);

/*!
 * \brief Return the minimum key string from \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * The minimum key is the left most entry of the left most node.
 *
 * \note this value it stored in the header of the SASStringBTree_t
 * initial block and should never be modified by the application.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return Null terminated minimum key string from B-Tree \a btree or 0 if
 * the B-Tree does not have any key or if an error occurs.
 */
extern __C__ char *
SASStringBTreeGetMinKey_nolock (SASStringBTree_t btree);

/*!
 * \brief Return true if the SAS B-Tree \a btree contains the key \a key.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a
 * read lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and returns true if found.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Null terminated key string to search.
 * @return 1 if the key is within \a btree or 0 otherwise.
 */
extern __C__ int
SASStringBTreeContainsKey (SASStringBTree_t btree, char *key);

/*!
 * \brief Return true if the SAS B-Tree \a btree contains the key \a key.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * This function searches the B-Tree for a matching key and returns true if found.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Null terminated key string to search.
 * @return 1 if the key is within \a btree or 0 otherwise.
 */
extern __C__ int
SASStringBTreeContainsKey_nolock (SASStringBTree_t btree, char *key);

/*!
 * \brief Return the memory address value associated with \a key in
 * SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * returns the associated memory address value.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Null terminated key string to search.
 * @return The associated memory address with \a key or 0 if the B-Tree \a
 * btree does not contain the key or if an error occurs.
 */
extern __C__ void *
SASStringBTreeGet (SASStringBTree_t btree, char *key);

/*!
 * \brief Return the memory address value associated with \a key in
 * SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * This function searches the B-Tree for a matching key and if found,
 * returns the associated memory address value.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Null terminated key string to search.
 * @return The associated memory address with \a key or 0 if the B-Tree \a
 * btree does not contain the key or if an error occurs.
 */
extern __C__ void *
SASStringBTreeGet_nolock (SASStringBTree_t btree, char *key);

/*!
 * \brief Return true if the SAS B-Tree \a btree is empty.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return 1 if the B-Tree is not empty or 0 otherwise.
 */
extern __C__ int
SASStringBTreeIsEmpty (SASStringBTree_t btree);

/*!
 * \brief Return true if the SAS B-Tree \a btree is empty.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return 1 if the B-Tree is not empty or 0 otherwise.
 */
extern __C__ int
SASStringBTreeIsEmpty_nolock (SASStringBTree_t btree);

/*!
 * \brief Return the number of elements in the SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a read
 * lock over B-Tree \a btree.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @return Number of elements within the B-Tree \a btree.
 */
extern __C__ long
SASStringBTreeGetCurCount (SASStringBTree_t btree);


/*!
 * \brief Add a new element \a value with key \a key in the SAS B-Tree 
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a
 * write lock over B-Tree \a btree.
 * This function inserts the key and associated memory address value
 * into the B-Tree.
 * This B-Tree implementation does not allow duplicated key values.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to insert in the B-Tree.
 * @return 1 if the operation succeeds or 0 otherwise.
 * For example if the key already exist in this B-Tree.
 */
extern __C__ int
SASStringBTreePut (SASStringBTree_t btree, char *key, void *value);

/*!
 * \brief Add a new element \a value with key \a key in the SAS B-Tree
 * \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * This function inserts the key and associated memory address value
 * into the B-Tree.
 * This B-Tree implementation does not allow duplicated key values.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to insert in the B-Tree.
 * @return 1 if the operation succeeds or 0 otherwise.
 * For example if the key already exist in this B-Tree.
 */
extern __C__ int
SASStringBTreePut_nolock (SASStringBTree_t btree, char *key, void *value);


/*!
 * \brief Replace the associated value of the element with key \a key
 * in SAS B-Tree \a btree with the value \a value.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a
 * write lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * replaces the associated memory address value with \a value, and
 * returns the previous associated value.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to replace in the B-Tree.
 * @return The address of the previous associated value for the
 * matching key, or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeReplace (SASStringBTree_t btree, char *key, void *value);

/*!
 * \brief Replace the associated value of the element with key \a key
 * in SAS B-Tree \a btree with the value \a value.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * This function searches the B-Tree for a matching key and if found,
 * replaces the associated memory address value with \a value, and
 * returns the previous associated value.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key to use as index for the value.
 * @param value Memory address to replace in the B-Tree.
 * @return The address of the previous associated value for the
 * matching key, or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeReplace_nolock (SASStringBTree_t btree, char *key, void *value);


/*!
 * \brief Remove the key \a key and its associated value from SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE. The function holds a write
 * lock over B-Tree \a btree.
 * This function searches the B-Tree for a matching key and if found,
 * removes the key and associates value from this B-Tree.
 *
 * \note removing the key and associated value from the B-Tree does not
 * remove or alter the data at that memory address. It only removes the
 * associated between the and key and the address from this B-Tree.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key value to be removed from this B-Tree.
 * @return The address of the previous item or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeRemove (SASStringBTree_t btree, char *key);

/*!
 * \brief Remove the key \a key and its associated value from SAS B-Tree \a btree.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGTREE.
 * This function searches the B-Tree for a matching key and if found,
 * removes the key and associates value from this B-Tree.
 *
 * \note removing the key and associated value from the B-Tree does not
 * remove or alter the data at that memory address. It only removes the
 * associated between the and key and the address from this B-Tree.
 *
 * This nolock form should only be used when the referenced SASStringBTree_t
 * is known to be locked by the application or contained within a
 * larger structure with a controlling lock.
 *
 * @param btree Handle to the SASStringBTree_t.
 * @param key Key value to be removed from this B-Tree.
 * @return The address of the previous item or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeRemove_nolock (SASStringBTree_t btree, char *key);

/*!
 * \brief Internal function to initialize storage as a B-tree.
 *
 * An internal function used to initialize the control blocks within
 * the specific storage block \a block as a SAS String B-tree.
 * Both \a block_size and \a page_size must be power of two in
 * size and have the same power of two (or better) alignment.
 * The SAS type created is SAS_RUNTIME_STRINGBTREE.
 *
 * @param block Block of allocated SAS storage.
 * @param block_size Size of the B-tree within the block.
 * @param page_size Size of page size to use.
 * @param expanding boolean indicates if the B-tree is expand or fixed.
 * @return A handle to the initialized SASStringBtree_t or 0 if an error
 * occurs.
 */
extern __C__ SASStringBTree_t
SASStringBTreeInit (void *block, block_size_t block_size,
		    block_size_t page_size, int expanding);

/*!
 * \brief Destroy the SAS B-Tree \a btree.
 *
 * Similar to ::SASStringBTreeDestroy but this function holds no lock.
 *
 * @param btree Handle of the SASStringBTree_t to be destroyed.
 */
extern __C__ void
SASStringBTreeDestroyNoLock (SASStringBTree_t btree);

/*!
 * \brief Internal function. Return the total available free space on SAS B-Tree \a btree.
 *
 * Similar to ::SASStringBTreeFreeSpace but this function holds no lock.
 *
 * @param btree Handle the to SASStringBTree_t.
 * @return The total available free space in bytes.
 */
extern __C__ block_size_t
SASStringBTreeFreeSpaceNoLock (SASStringBTree_t btree);

/*!
 * \brief Internal function. Allocate a new SASStringBTreeNode_t from SAS B-Tree \a btree.
 *
 * Similar to ::SASStringBTreeAlloc but this function hilds no lock.
 *
 * @param btree Handle the to SASStringBTree_t.
 * @return A new SASStringBTreeNode_t handle or 0 if creation fails.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeAllocNoLock (SASStringBTree_t btree);

/*!
 * \brief Internal function. Allocate a new SASStringBTreeNode_t from SAS B-Tree near
 * \a nearObj.
 *
 * Similar to ::SASStringBTreeNearAlloc but this function holds no lock.
 *
 * @param nearObj Memory address of the handle to the SASStringBTree_t.
 * @return A new SASStringBTreeNode_t handle or 0 if creation fails.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNearAllocNoLock (void *nearObj);

/*!
 * \brief Internal function. Free the SASStringBTreeNode_t on \a memAddr from its associated
 * SASStringBTree_t B-Tree.
 *
 * Similar to ::SASStringBTreeNearDealloc but this function holds no lock.
 *
 * @param memAddr Memory address associated with the SASStringBTreeNode_t to
 * be freed.
 */
extern __C__ void
SASStringBTreeNearDeallocNoLock (void *memAddr);

/*!
 * \brief Internal function. Free the SASStringBTreeNode_t \a free_block from B-tree \a btree.
 *
 * Similar to ::SASStringBTreeFree but this function holds no lock.
 * 
 * @param btree Handle to the SASStringBTree_t.
 * @param free_block Handle to the SASStringBTreeNode_t to be freed.
 */
extern __C__ void
SASStringBTreeFreeNoLock (SASStringBTree_t btree,
			  SASStringBTreeNode_t free_block);

#endif /* __SAS_STRINGBTREE_H */
