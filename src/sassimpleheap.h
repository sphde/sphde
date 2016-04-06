/*
 * Copyright (c) 2003-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - documentation
 */

#ifndef __SAS_SIMPLE_HEAP_H
#define __SAS_SIMPLE_HEAP_H

#include "sastype.h"

/*!
 * \file  sassimpleheap.h
 * \brief Shared Address Space Simple Heap.
 *
 * Allocate a SAS block to be used as an heap using its internal functions to
 * allocate/deallocate objects. The heap is created with function 
 * SASSimpleHeapCreate and destroyed with SASSimpleHeapDestroy. Objects are 
 * allocated with function SASSimpleHeapAlloc, SASSimpleHeapNearAlloc, and with
 * their NoLock variants. The objects are freed using SASSimpleHeapFree,
 * SASSimpleHeapNearDealloc, and with their NoLock variants.
 */

/*! 
 * \brief Handle to SAS Simple Space.
 * The type is SAS_RUNTIME_SIMPLEHEAP
 */
typedef void *SASSimpleHeap_t; 

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Initialize a shared storage as a simple heap.
 *
 * Initialize the control blocks within the specified storage block as a
 * Simple Heap. The store block must be power of two in size and have the
 * same power of two (or better) alignment.
 * The SAS type created is SAS_RUNTIME_SIMPLEHEAP.
 *
 * @param heap_block Block of allocated SAS storage.
 * @param sasType Type of SAS object (should contain SAS_SIMPLEHEAP_TYPE type).
 * @param heap_size Size of the simple heap within the block.
 * @return A handle to the initialized SASSimpleHeap_t or 0 if an error
 * occurs.
 */
extern __C__ SASSimpleHeap_t 
SASSimpleHeapInit (void *heap_block , sas_type_t sasType, 
		block_size_t heap_size);

/*!
 * \brief Allocate a SAS block large enough to contain the requested
 * SAS Simple Heap.
 *
 * Create and initialize a Simple Heap. The storage block must be power
 * of two in size and SAS type returned is SAS_RUNTIME_SIMPLEHEAP.
 *
 * @param heap_size Size of the Simple Heap to create.
 * @return A handle to the created SASSimpleHeap_t.
 */
extern __C__ SASSimpleHeap_t 
SASSimpleHeapCreate (block_size_t heap_size);

/*!
 * \brief Destroy a SASSimpleHeap_t and free the shared storage block.
 *
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. Destroy holds an 
 * exclusive lock write while clearing the control blocks and freeing the SAS
 * block.
 *
 * @param heap Handle of the SASSimpleHeap_t to be destroyed.
 * @return 0 indicates success, otherwise failure.
 */
extern __C__ int 
SASSimpleHeapDestroy (SASSimpleHeap_t heap);

/*!
 * \brief Return the available space from Simple Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while reading the empty spaces from heap object.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @return Size in bytes of the remaining free Heap.
 */
extern __C__ block_size_t 
SASSimpleHeapFreeSpace (SASSimpleHeap_t heap);

/*!
 * \brief Return if the Simple Heap \a heap has no space left.
 *
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while reading the empty spaces from heap object.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @return 0 if heap is not empty and has some space left for additional
 * storage or 1 otherwise.
 */
extern __C__ int
SASSimpleHeapEmpty (SASSimpleHeap_t heap);

/*!
 * \brief Allocate a block of \a alloc_size bytes from Simple Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while reading the empty spaces from heap object.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @param alloc_size Size in byte to allocate.
 * @return A valid memory address or NULL if \a heap is not a
 * SAS_RUNTIME_SIMPLEHEAP or if there is no space left in \a heap.
 */
extern __C__ void *
SASSimpleHeapAlloc (SASSimpleHeap_t heap, block_size_t alloc_size);

/*!
 * \brief Deallocate the memory block \a free_block of size \a alloc_size
 * from Simple Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while freeing the allocated block from heap.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @param free_block Memory block previously allocated using SASSimpleHeapAlloc
 * function.
 * @param alloc_size Size in bytes of the allocated block size.
 * @return 0 if the block was successfully freed, -1 if \a alloc_size is
 * incorrect, or -2 if \a heap is not a SAS_RUNTIME_SIMPLEHEAP.
 */
extern __C__ int 
SASSimpleHeapFree (SASSimpleHeap_t heap, 
		void *free_block, 
		block_size_t alloc_size);

/*!
 * \brief Find the associate SASSimpleHeap_t control block near \a nearObj.
 * @param nearObj Handle of a SAS Simple Heap.
 * @return Associated Simple Heap handler or NULL if the address \a nearObj
 * is not a SASSimpleHeap_t.
 */
extern __C__ SASSimpleHeap_t
SASSimpleHeapNearFind(void *nearObj);

/*!
 * \brief Allocate a block of \a alloc_size bytes from Simple Heap \a nearObj.
 *
 * The address \a nearObj is used to find the associated Simple Heap object.
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while allocating from heap.
 *
 * @param nearObj Handle of a SAS Simple Heap.
 * @param allocSize Size in byte to allocate.
 * @return A valid memory address or NULL if \a heap if not a
 * SAS_RUNTIME_SIMPLEHEAP or if there is no space left in \a heap.
 */
extern __C__ void* 
SASSimpleHeapNearAlloc(void *nearObj, long allocSize);

/*!
 * \brief Deallocate the memory block \a memAddr of size \a allocSize.
 *
 * The address \a memAddr is used to find the associated Simple Heap object.
 * The sas_type_t must be SAS_RUNTIME_SIMPLEHEAP. The functions holds an 
 * exclusive write while reading the empty spaces from heap object.
 *
 * @param memAddr Handle of a SAS Simple Heap.
 * @param allocSize Size in bytes of the allocated block size.
 * @return 0 if the block was successfully freed, -1 if \a alloc_size is
 * incorrect, or -2 if \a heap is not a SAS_RUNTIME_SIMPLEHEAP.
 */
extern __C__ void
SASSimpleHeapNearDealloc(void *memAddr, long allocSize);

/*!
 * \brief Destroy a SASSimpleHeap_t and free the shared storage block.
 *
 * Similar to ::SASSimpleHeapDestroy but do not hold any write lock.
 *
 * @param heap handle of the SASSimpleHeap_t to be destroyed.
 * @return a 0 value indicates success, otherwise failure.
 */
extern __C__ int 
SASSimpleHeapDestroyNoLock (SASSimpleHeap_t heap);

/*!
 * \brief Return the available space from Simple Heap \a heap.
 *
 * Similar to ::SASSimpleHeapFreeSpace but do not hold any write lock.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @return The size in bytes of the available space on heap.
 */
extern __C__ block_size_t 
SASSimpleHeapFreeSpaceNoLock (SASSimpleHeap_t heap);

/*!
 * \brief Allocate a block of \a alloc_size bytes from Simple Heap \a heap.
 *
 * Similar to ::SASSimpleHeapAlloc but do not hold any write lock.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @param alloc_size Size in byte to allocate.
 * @return A valid memory address or NULL if \a heap is not a
 * SAS_RUNTIME_SIMPLEHEAP or if there is no space left in \a heap.
 */
extern __C__ void *
SASSimpleHeapAllocNoLock (SASSimpleHeap_t heap, 
			block_size_t alloc_size);

/*!
 * \brief Deallocate the memory block \a free_block of size \a alloc_size
 * from Simple Heap \a heap.
 *
 * Similar to ::SASSimpleHeapFree but do not hold any lock.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @param free_block Memory block previously allocated using SASSimpleHeapAlloc
 * function.
 * @param alloc_size Size in bytes of the allocated block size.
 * @return 0 if the block was successfully freed, -1 if \a alloc_size is
 * incorrect, or -2 if \a heap is not a SAS_RUNTIME_SIMPLEHEAP.
 */
extern __C__ int 
SASSimpleHeapFreeNoLock (SASSimpleHeap_t heap, 
			void *free_block, 
			block_size_t alloc_size);

/*!
 * \brief Allocate a block of \a allocSize bytes from Simple Heap \a heap.
 *
 * Similar to ::SASSimpleHeapNearAlloc but do not hold any write lock.
 *
 * @param nearObj Handle of a SAS Simple Heap.
 * @param allocSize Size in byte to allocate.
 * @return A valid memory address or NULL if \a heap is not a
 * SAS_RUNTIME_SIMPLEHEAP or if there is no space left in \a heap.
 */
extern __C__ void* 
SASSimpleHeapNearAllocNoLock(void *nearObj, long allocSize);

/*!
 * \brief Deallocate the memory block \a memAddr of size \a allocSize.
 *
 * Similar to ::SASSimpleHeapNearDealloc but do not hold any write lock.
 *
 * @param memAddr Handle of a SAS Simple Heap.
 * @param allocSize Size in bytes of the allocated block size.
 * @return 0 if the block was successfully freed, -1 if \a alloc_size is
 * \a alloc_size if incorrect, or -2 if \a heap is not a SAS_RUNTIME_SIMPLEHEAP.
 */
extern __C__ void
SASSimpleHeapNearDeallocNoLock(void *memAddr, long allocSize);

/*!
 * \brief Return if the Simple Heap \a heap has not space left.
 *
 * Similar to ::SASSimpleHeapEmpty but do not hold any write lock.
 *
 * @param heap Handle of a SAS Simple Heap.
 * @return 0 if heap is not empty and has some space left for additional
 * storage or 1 otherwise.
 */
extern __C__ int
SASSimpleHeapEmptyNoLock (SASSimpleHeap_t heap);

#endif /* __SAS_SIMPLE_HEAP_H */
