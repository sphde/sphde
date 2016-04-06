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

#ifndef __SAS_STRINGBTREENODE_H
#define __SAS_STRINGBTREENODE_H

#include "sastype.h"

/*!
 * \file  sasstringbtreenode.h
 * \brief Shared Address Space B-tree node and element operations.
 *
 * \warning SASStringBTreeNode_t are the internal data structures of the
 * SASStringBTree_t and should not be accessed directly by applications.
 *
 * The basic function to allocate entries is ::SASStringBTreeAlloc and it
 * returns a ::SASStringBTreeNode_t (mode info on sasstringbtreenode.h). The
 * function ::SASStringBTreeNearAlloc is similar, but it accepts a memory
 * pointer to SASStringBTree_t as argument.
 *
 * Once a SAS B-Tree is created its member can be accessed using the functions
 * ::SASStringBTreeNodeGetCount to get the node number,
 * ::SASStringBTreeNodeGetKeyIndexed to get the key from a specific child
 * node, ::SASStringBTreeNodeGetValIndexed to get the value from a specific
 * child node, and ::SASStringBTreeNodeGetBranchIndexed to get child nodes.
 * For example, to interact over all nodes from a B-Tree:
 *
 * The SAS B-tree element (SBE) is a tuple used with SAS B-Tree
 * (sasstringbtree.h). It is basically a key element (a null terminated
 * string) and a value (a memory pointer).
 *
 * A new SBE can be created with ::SASStringBTreeNodeCreate or with
 * ::SASStringBTreeNodeInit. The SBE can be destroyed using the function
 * ::SASStringBTreeNodeDestroy.
 *
 * Memory can be allocated using ::SASStringBTreeNodeAlloc or
 * ::SASStringBTreeNodeNearAlloc and freed using ::SASStringBTreeNodeFree.
 * 
 * Information about the node can be queried using the functions 
 * ::SASStringBTreeNodeGetKeyIndexed (to query children keys),
 * ::SASStringBTreeNodeGetValIndexed (to query children values),
 * ::SASStringBTreeNodeGetCount (to query children number), and
 * ::SASStringBTreeNodeGetBranchIndexed (to query children
 * SASStringBTreeNode_t).
 *
 * Nodes can search by using ::SASStringBTreeNodeSearch, 
 * ::SASStringBTreeNodeSearchGT, ::SASStringBTreeNodeSearchGE,
 * ::SASStringBTreeNodeSearchLT, and ::SASStringBTreeNodeSearchLE.
 *
 * The follow example shows how to interact over SASStringBTree_t
 * using the SASStringBTreeNode functions:
 *
 * \code
 * #include <sphde/sasstringbtree.h>
 *
 * static inline void
 * sasbtree_print_node(SASStringBTreeNode_t node)
 * {
 *   int i, n;
 *   n = SASStringBTreeNodeGetCount(node);
 *   for (i = 0; i <= n; ++i)
 *     {
 *       SASStringBTreeNode_t br;
 *       const char *key = SASStringBTreeNodeGetKeyIndexed(node, i);
 *       if (!key)
 *         continue;
 *       printf("(%s : %s)", key,
 *         (char*)SASStringBTreeNodeGetValIndexed(node, i));
 *       if ((br = SASStringBTreeNodeGetBranchIndexed(node, i)))
 *         sasbtree_print_node(br);
 *       if (i != n)
 *         printf(", ");
 *     }
 * }
 *
 * static inline void
 * sasbtree_print(SASStringBTree_t btree)
 * {
 *   SASStringBTreeNode_t root = SASStringBTreeGetRootNode(btree);
 *   if (root)
 *     {
 *       printf("{");
 *       sasbtree_print_node(root);
 *       printf("}");
 *     }
 *   printf("\n");
 * }
 * \endcode
 *
 * The code above obtains no locks over the B-Tree. For concurrent access
 * a read/write lock should be obtained to avoid race-conditions.
 * The \a examples/example_sph_3.c shows an example on how to print a SAS
 * SAS B-Tree.
 */

/*!
 * \brief Handle to an instance of String B-tree element.
 */
typedef void *SASStringBTreeNode_t;
#ifndef LOCK_ON_T
#define LOCK_ON_T
typedef int lock_on_t;
const lock_on_t LOCK_ON = 1;
const lock_on_t LOCK_OFF = 0;
#endif

/*!
 * Result from search functions.
 */
typedef struct
{
  /*@{ */
  SASStringBTreeNode_t node;   /**< Reference to result node */
  short pos;		       /**< Position of searched node */
  /*@} */
} SBTnodePosRef;


#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Initialize a B-tree element.
 *
 * Initialize the control blocks within the store block \a heap_block as a
 * SAS B-tree element. The \a heap_size must be power of two in size and 
 * have the same power of two (or better) alignment. The SAS type \a sasType
 * must be SAS_RUNTIME_STRINGBTREENODE.
 *
 * @param heap_block Block of allocated SAS storage.
 * @param sasType SAS type must be, must be SAS_RUNTIME_STRINGBTREENODE.
 * @param heap_size Size of the B-tree within the block.
 * @return A handle to the initialized SASStringBTreeNode_t or 0 if an error
 * occurs.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNodeInit (void *heap_block, sas_type_t sasType,
			block_size_t heap_size);

/*!
 * \brief Create a new SAS B-tree element with \a heap_size.
 *
 * Create and initialize a new B-tree element. The storage block must be
 * power of two in size and SAS type returned is SAS_RUNTIME_STRINGBTREENODE.
 * The internal page size used is default one defined in sasalloc.h (4096).
 *
 * @param heap_size Size of the B-Tree element to create.
 * @return A handle to created SASStringBTreeNode_t or 0 if creation fails.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNodeCreate (block_size_t heap_size);

/*!
 * \brief Destroy the SAS B-Tree element \a heap.
 *
 * The type must be SAS_RUNTIME_STRINGBTREENODE. Destroy holds an exclusive
 * write lock.
 *
 * @param heap Handle of the SASStringBTreeNode_t to be destroyed.
 */
extern __C__ void
SASStringBTreeNodeDestroy (SASStringBTreeNode_t heap, lock_on_t lock_on);

/*!
 * \brief Return the total available free space on SAS B-Tree element.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREENODE.
 *
 * @param heap Handle to the SASStringBTreeNode_t.
 * @return The total available free space in bytes.
 */
extern __C__ block_size_t
SASStringBTreeNodeFreeSpace (SASStringBTreeNode_t heap, lock_on_t lock_on);

/*!
 * \brief Return the total number of free blocks available.
 *
 * The function holds no locks.
 *
 * \param \heap Handle to the SASStringBTreeNode_t.
 * \return The number of available or -1 if an error occurs.
 */
extern __C__ block_size_t
SASStringBTreeNodeFreeFragmentsNoLock (SASStringBTreeNode_t heap);

/*!
 * \brief Return the largest free block available.
 *
 * The function holds no locks.
 *
 * \param \heap Handle to the SASStringBTreeNode_t.
 * \return The size in bytes or -1 if an error occurs.
 */
extern __C__ block_size_t
SASStringBTreeNodeMaxFragmentNoLock (SASStringBTreeNode_t heap);

/*!
 * \brief Allocate \a alloc_size bytes from the SAS B-tree element \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREENODE. The function holds a
 * write lock over B-Tree \a heap. The \a alloc_size must be power of two in
 * size.
 *
 * @param heap Handle to the SASStringBtreeNode_t.
 * @param alloc_size Size in bytes to allocate from the element.
 * @return The allocated memory region or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeNodeAlloc (SASStringBTreeNode_t heap, block_size_t alloc_size,
				lock_on_t lock_on);

/*!
 * \brief Free the memory address \a free_block of size \a alloc_size from
 * \a heap SAS B-tree node.
 *
 * The sas_type_t must be SAS_RUNTIME_STRINGBTREENODE. The functions holds a
 * write lock over B-Tree element \a heap.
 *
 * @param heap Handle to the SASStringBTreeNode_t.
 * @param free_block Memory address to be freed.
 * @param alloc_size Size of the memory region.
 * @return 0 if free succeed, -1 if \a heap is not SAS B-tree element, or -2
 * if \a alloc_size does not hold the correct size of \a heap.
 */
extern __C__ int
SASStringBTreeNodeFree (SASStringBTreeNode_t heap, void *free_block,
			block_size_t alloc_size, lock_on_t lock_on);

/*!
 * \brief Allocate \a allocSize bytes from SAS B-Tree element \a nearObj.
 *
 * The function is similar to ::SASStringBTreeNodeAlloc but accepts a memory
 * address where the SASStringBTree_t is allocated.
 *
 * @param nearObj Memory address of the SASStringBtreeNode_t handler.
 * @param allocSize Size in bytes to allocate from the element.
 * @return The allocated memory region or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeNodeNearAlloc (void *nearObj, long allocSize, lock_on_t lock_on);

/*!
 * \brief Destroy the SAS B-Tree element \a heap.
 *
 * Similar to ::SASStringBTreeNodeDestroy but the function holds no lock.
 *
 * @param heap Handle of the SASStringBTreeNode_t to be destroyed.
 */
extern __C__ void
SASStringBTreeNodeDestroyNoLock (SASStringBTreeNode_t heap);

/*!
 * \brief Return the total available free space on SAS B-Tree element.
 *
 * Similar to function ::SASStringBTreeNodeFreeSpace but holds no lock.
 *
 * @param heap Handle to the SASStringBTreeNode_t.
 * @return The total available free space in bytes.
 */
extern __C__ block_size_t
SASStringBTreeNodeFreeSpaceNoLock (SASStringBTreeNode_t heap);

/*!
 * \brief Allocate \a alloc_size bytes from the SAS B-tree element \a heap.
 *
 * Similar to ::SASStringBTreeNodeAlloc but function holds no lock.
 *
 * @param heap Handle to the SASStringBtreeNode_t.
 * @param alloc_size Size in bytes to allocate from the element.
 * @return The allocated memory region or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeNodeAllocNoLock (SASStringBTreeNode_t heap, 
			       block_size_t alloc_size);

/*!
 * \brief Free the memory address \a free_block of size \a alloc_size from
 * \a heap SAS B-tree node.
 *
 * Similar to ::SASStringBTreeNodeFree but the function holds no lock.
 *
 * @param heap Handle to the SASStringBTreeNode_t.
 * @param free_block Memory address to be freed.
 * @param alloc_size Size of the memory region.
 * @return 0 if free succeed, -1 if \a heap is not SAS B-tree element, or -2
 * if \a alloc_size does not hold the correct size of \a heap.
 */
extern __C__ int
SASStringBTreeNodeFreeNoLock (SASStringBTreeNode_t heap, void *free_block,
			      block_size_t alloc_size);

/*!
 * \brief Return the children key from SAS B-Tree \a header at position
 * \a pos.
 *
 * @param header SASStringBTreeNode_t to get the key.
 * @param pos Position of the children key in the element.
 * @return The key in a null terminated string or 0 if an error occurs.
 */
extern __C__ char *
SASStringBTreeNodeGetKeyIndexed (SASStringBTreeNode_t header, short pos);

/*!
 * \brief Return the children from SAS B-Tree \a header at position \a pos.
 *
 * @param header SASStringBTreeNode_t to get the children reference.
 * @param pos Position of the children in the element.
 * @return The element or 0 if an error occurs.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNodeGetBranchIndexed (SASStringBTreeNode_t header, short pos);

/*!
 * \brief Return the children value from SAS B-Tree \a header at position
 * \a pos.
 *
 * @param header SASStringBTreeNode_t to get the children reference.
 * @param pos Position of the children value in the element.
 * @return The children value or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeNodeGetValIndexed (SASStringBTreeNode_t header, short pos);

/*!
 * \brief Return the number of children elements from SAS B-tree element
 * \a header.
 *
 * @param header SASStringBTreeNode_t got get number of children.
 * @return The number of children or -1 if an error occurs.
 */
extern __C__ int
SASStringBTreeNodeGetCount (SASStringBTreeNode_t header);


/*!
 * \brief Replace the value indexed by \a pos within SAS B-tree element \a
 * header with value \a value.
 *
 * @param header SASStringBTreeNode_t to replace the value.
 * @param pos Position of the value within the node.
 * @param val New value to set.
 * @return The old value or NULL or 0 if an error occurs.
 */
extern __C__ void *
SASStringBTreeNodePutValIndexed (SASStringBTreeNode_t header, short pos,
			         void *val);

/*!
 * \brief Recursively search for key \a target within SAS B-tree node
 * \a header.
 *
 * The result are updated in \a ref indicating if node was found and its
 * position. The key is search using strcmp and the result is found
 * when the string compare function returns 0.
 *
 * @param header SASStringBTreeNode_t to search.
 * @param target Key to search for.
 * @param ref Search return with ref::node being a pointer to found
 * node or 0 if the node was not found and ref::pos indicating node's
 * position.
 * @return The position of the found key as an integer equal or higher than
 * 0 or a negative value if the functions fails.
 */
extern __C__ int
SASStringBTreeNodeSearch (SASStringBTreeNode_t header, char *target,
			  SBTnodePosRef * ref);

/*!
 * \brief Recursively search for greater than key \a target within SAS
 * B-tree node \a header.
 *
 * Similar to ::SASStringBTreeNodeSearch but search criteria is for a key
 * greater than the argument key \a target.
 *
 * @param header SASStringBTreeNode_t to search.
 * @param target Key to search for.
 * @param ref Search return with ref::node being a pointer to found
 * node or 0 if the node was not found and ref::pos indicating node's
 * position.
 * @return The position of the found key as an integer equal or higher than
 * 0 or a negative value if the functions fails.
 */
extern __C__ int
SASStringBTreeNodeSearchGT (SASStringBTreeNode_t header, char *target,
			    SBTnodePosRef * ref);

/*!
 * \brief Recursively search for greater than or equal key \a target within
 * SAS B-tree node \a header.
 *
 * Similar to ::SASStringBTreeNodeSearch but search criteria is for a key
 * greater than or equal the argument key \a target.
 *
 * @param header SASStringBTreeNode_t to search.
 * @param target Key to search for.
 * @param ref Search return with ref::node being a pointer to found
 * node or 0 if the node was not found and ref::pos indicating node's
 * position.
 * @return The position of the found key as an integer equal or higher than
 * 0 or a negative value if the functions fails.
 */
extern __C__ int
SASStringBTreeNodeSearchGE (SASStringBTreeNode_t header, char *target,
			    SBTnodePosRef * ref);

/*!
 * \brief Recursively search for less than \a target within SAS B-tree
 * node \a header.
 *
 * Similar to ::SASStringBTreeNodeSearch but search criteria is for a key
 * less than the argument key \a target.
 *
 * @param header SASStringBTreeNode_t to search.
 * @param target Key to search for.
 * @param ref Search return with ref::node being a pointer to found
 * node or 0 if the node was not found and ref::pos indicating node's
 * position.
 * @return The position of the found key as an integer equal or higher than
 * 0 or a negative value if the functions fails.
 */
extern __C__ int
SASStringBTreeNodeSearchLT (SASStringBTreeNode_t header, char *target,
			    SBTnodePosRef * ref);

/*!
 * \brief Recursively search for less than or equal \a target within SAS
 * B-tree node \a header.
 *
 * Similar to ::SASStringBTreeNodeSearch but search criteria is for a key
 * less than or equal the argument key \a target.
 *
 * @param header SASStringBTreeNode_t to search.
 * @param target Key to search for.
 * @param ref Search return with ref::node being a pointer to found
 * node or 0 if the node was not found and ref::pos indicating node's
 * position.
 * @return The position of the found key as an integer equal or higher than
 * 0 or a negative value if the functions fails.
 */
extern __C__ int
SASStringBTreeNodeSearchLE (SASStringBTreeNode_t header, char *target,
			    SBTnodePosRef * ref);

/*!
 * \brief Initialize the SAS B-tree element \a header with key \a newkey and
 * value \a newval.
 *
 * @param header SASStringBTreeNode_t to be initialized.
 * @param newkey Key to set on SAS B-tree node.
 * @param newval Valued to set on SAS B-tree node.
 */
extern __C__ void
SASStringBTreeNodeInitialize (SASStringBTreeNode_t header, char *newkey,
			      void *newval, lock_on_t lock_on);

/*!
 * \brief Insert a new node in SAS B-Tree element \a element using \a newkey
 * as key and \a newval as value.
 *
 * @param header SASStringBTreeNode_t to add the new SAS B-tree node.
 * @param newkey Key to set on new SAS B-tree node.
 * @param newval Valued to set on new SAS B-tree node.
 * @return A handle to new SAS B-tree node or 0 if an error occurs.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNodeInsert (SASStringBTreeNode_t header, char *newkey,
			  void *newval, lock_on_t lock_on);

/*!
 * \brief Remove the SAS B-tree node with key \a target from SAS B-tree node
 * \a header.
 *
 * @param header SASStringBTreeNode_t to remove the SAS B-tree node.
 * @param target Key to SAS B-tree node to remove.
 * @return A handle to \a header or new header if \a target is the key of
 * element itself.
 */
extern __C__ SASStringBTreeNode_t
SASStringBTreeNodeDelete (SASStringBTreeNode_t header, char *target,
				lock_on_t lock_on);

#endif /* __SAS_STRINGBTREENODE_H */
