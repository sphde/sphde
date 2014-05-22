/*
 * Copyright (c) 2011-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SPH_LOCK_FREE_HEAP_H
#define __SPH_LOCK_FREE_HEAP_H

/**! \file sphlockfreeheap.h
*  \brief Shared Persistent Heap, lock free heap allocator
*	for shared memory multi-thread/multi-core applications.
*
*	This implementation uses atomic bit vectors to control the
*	allocation of memory in a fast and thread-safe manner.
*	Each bit within the bit vector represents a fixed granule for
*	allocation of memory. A string of contiguous bits represent
*	larger allocations in multiples of the granule size.
*
*	Following the SPH design
*	blocks used for LF heaps must be power of 2 in size and matching
*	power of 2 alignment or better. This is required to support the
*	near forms of alloc/free where the head header is implied from
*	the contained address. The granules also must be power of 2 size
*	which supports the aligned forms of allocation.
*
*	Allocations are tracked using bit vectors where each bit
*	represents an power of 2 unit with matching alignment. Each
*	heap is configured for a specific allocation unit size and
*	different heaps can can be configured with varying unit sizes
*	as required. For each unit size the maximum individual
*	allocation size is limited the maximum atomic update granual of
*	the underlying processor. For most processors this is defined
*	by the largest compare-and-swap or equivalent atomic operation.
*	However for biarch systems we may choose that smaller granual
*	of the supported modes for consistency, normally 32-bits.
*
*	So any specific heap instance will support a maximum allocation
*	request of 32 * unit-size. The Minimun unit size is 16-bytes so
*	a heap configured with this size can support allocations of
*	16-byte multiples up to 512 bytes. If an application needs a
*	larger allocation size a different heap needs to be configured
*	with an appropriate unit size. For example if allocation up to
*	4K are need then a unit size of at least 128 bytes is needed.
*	Of course this unit-size will also be the minimum allocation
*	size and alignment for that heap.
*
*	We assume that this simple lock free heap will be used in the
*	implementation of a compound lock free heap. Such a compound
*	heap would automatically allocate multiple simple heaps with
*	unit-sizes selected to support the the dynamic requests of the
*	application.
*/

#include "sastype.h"

/** \brief Handle to an instance of SPH Lock Free Heap.
*
*	The type is SAS_RUNTIME_LOCKFREEHEAP
*/
typedef void *SPHLockFreeHeap_t; 

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Initialize a shared storage block as a lock free heap.
*
*	Initialize the control blocks within the specified storage
*	block as a lock free heap.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_LOCKFREEHEAP_TYPE.
*	The unit_size is the allocation granule in bytes and must be
*	a power of two size greater then 1.
*
*	@param heap_block a block of allocated SAS storage matching the heap_size.
*	@param sasType a SAS type code for internal consistency checking.
*	@param heap_size power of two size of the heap to be initialized.
*	@param unit_size power of 2 size of the allocation granule.
*	@return at handle to the initialized SPHLockFreeHeap_t
*/
extern __C__ SPHLockFreeHeap_t 
SPHLockFreeHeapInit (void *heap_block , sas_type_t sasType,
                     block_size_t heap_size, size_t unit_size);

/** \brief Create a lock free heap in a shared storage block.
*
*	Allocate a SAS block and initialize the control blocks as a
*	lock free heap.
*	The block size must be power of two in size.
*	The unit_size is the allocation granule in bytes and must be
*	a power of two size greater then 1.
*
*	@param heap_size power of 2 size of the heap to allocated and initialized.
*	@param unit_size power of 2 of the allocation granule.
*	@return a handle of the newly allocated SPHLockFreeHeap_t. Or NULL for the failure case.
*/
extern __C__ SPHLockFreeHeap_t 
SPHLockFreeHeapCreate (block_size_t heap_size, size_t unit_size);

/** \brief Destroy a lock free heap and free the shared storage block.
*
*	The sas_type_t must be of type SAS_LOCKFREEHEAP_TYPE.
*
*	@param heap to destroyed and the storage freed.
*	@return a 0 value indicated success, otherwise failure.
*/
extern __C__ int 
SPHLockFreeHeapDestroy (SPHLockFreeHeap_t heap);

/** \brief Return the remaining allocatable free space within the
*	specified lock free heap..
*
*	@param heap to checked for size.
*	@return the remaining free space for this heap in bytes.
*/
extern __C__ block_size_t 
SPHLockFreeHeapFreeSpace (SPHLockFreeHeap_t heap);

/** \brief Return a boolean result indicating if the lock free heap is
*	empty.
*
*	@param heap to checked for full/empty.
*	@return true if all available space is free.
*/
extern __C__ int
SPHLockFreeHeapEmpty (SPHLockFreeHeap_t heap);

/** \brief Return a boolean result indicating if the lock free heap is
*	completely full.
*
*	@param heap to checked for full/empty.
*	@return true if all available space is already allocated.
*/
extern __C__ int
SPHLockFreeHeapFull (SPHLockFreeHeap_t heap);

/** \brief Suballocate memory from a lock-free heap.
*
*	Request alloc_size bytes of memory from the heap.
*	The allocation size will be rounded up to the next intergral
*	unit_size. The minimal alignment of the allocation will be
*	the unit_size.
*	Returns a pointer to the allocated memory or NULL if no
*	contiguous space of the requested size is available.
*
*	@param heap to allocated from.
*	@param alloc_size size in bytes of the allocation request.
*	@return pointer to the successfully allocate storage. NULL otherwise.
*/
extern __C__ void *
SPHLockFreeHeapAlloc (SPHLockFreeHeap_t heap, block_size_t alloc_size);

/** \brief Suballocate memory from a lock free heap at a specified
*	alignment.
*
*	Request alloc_size bytes of memory from the heap.
*	The allocation size will be rounded up to the next intergral
*	unit_size. The minimal alignment of the allocation will be
*	specified by alignment.
*	Returns a pointer to the allocated memory or NULL if no
*	contiguous space of the requested size or alignment is
*	available.
*
*	@param heap to allocated from.
*	@param alloc_size size in bytes of the allocation request.
*	@param alignment required power of 2 alignment.
*	@return pointer to the successfully allocate storage. NULL otherwise.
*/
extern __C__ void *
SPHLockFreeHeapAlignAlloc (SPHLockFreeHeap_t heap,
                           block_size_t alloc_size,
                           block_size_t alignment);

/** \brief Suballocate memory from a lock free heap, close to a previous allocation.
*
*	Request alloc_size bytes of memory from the heap.
*	The allocation size will be rounded up to the next integral
*	unit_size. The minimal alignment of the allocation will be
*	the unit_size. Attempt to allocate from the SPHLockFreeHeap
*	which contains the near object.
*	Returns a pointer to the allocated memory or NULL if no
*	contiguous space of the requested size is available in the
*	"near" SPHLockFreeHeap.
*
*	@param nearObj address of another lock free heap allocation.
*	@param alloc_size size in bytes of the allocation request.
*	@return pointer to the successfully allocate storage. NULL otherwise.
*/
extern __C__ void* 
SPHLockFreeHeapNearAlloc(void *nearObj, long alloc_size);

/** \brief Free memory allocated from a lock free heap for reuse.
*
*	The memory at *free_block is freed for reuse up to the length
*	of the original allocation.
*	Returns zero if the free operation is successful. Otherwise a
*	non-zero value for an error condition.
*
*	@param heap containing the allocation to be freed.
*	@param free_block pointer to the previously allocated storage to be freed.
*	@return 0 indicates success, otherwise the free operation failed.
*/
extern __C__ int 
SPHLockFreeHeapFree (SPHLockFreeHeap_t heap, 
                     void *free_block);

/** \brief Free memory allocated from a lock free heap for reuse.
*
*	The containing lock free heap is found, if possible, then the
*	memory at *free_block is freed for reuse up to the length
*	of the original allocation.
*	Returns zero if the free operation is successful. Otherwise a
*	non-zero value for an error condition.
*
*	@param free_block pointer to the previously allocated storage to be freed.
*	@return 0 indicates success, otherwise the free operation failed.
*/
extern __C__ int 
SPHLockFreeHeapFreeNear (void *free_block);

/** \brief Free memory allocated from a lock free heap for reuse.
*
*	The memory at *free_block is freed for reuse up to the length
*	of the original allocation.
*	Returns zero if the free operation is successful. Otherwise a
*	non-zero value for an error condition.
*
*	@param heap containing the allocation to be freed.
*	@param free_block pointer to the previously allocated storage to be freed.
*	@return 0 indicates success, otherwise the free operation failed.
*/
extern __C__ int 
SPHLockFreeHeapFreeIn (SPHLockFreeHeap_t heap, 
                       void *free_block);

/** \brief Verify size of the allocation before freeing memory.
*
*	The memory at *free_block is freed for reuse up to the length
*	of the original allocation.
*	The allocation size is checked against the original allocation.
*	Returns zero if the free operation is successful. Otherwise a
*	non-zero value for an error condition.
*
*	@param heap containing the allocation to be freed.
*	@param free_block pointer to the previously allocated storage to be freed.
*	@param alloc_size size in bytes of the original allocation.
*	@return 0 indicates success, otherwise the free operation failed.
*/
extern __C__ int 
SPHLockFreeHeapFreeChk (SPHLockFreeHeap_t heap, 
                        void *free_block, 
                        block_size_t alloc_size);

/** \brief Verify size of the allocation before freeing memory.
*
*	The containing lock free heap is found, if posible, then the
*	memory at *free_block is freed for reuse up to the length
*	of the original allocation.
*	The allocation size is checked against the original allocation.
*	Returns zero if the free operation is successful. Otherwise a
*	non-zero value for an error condition.
*
*	@param free_block pointer to the previously allocated storage to be freed.
*	@param alloc_size size in bytes of the original allocation.
*	@return 0 indicates success, otherwise the free operation failed.
*/
extern __C__ int 
SPHLockFreeHeapFreeNearChk (void *free_block,
                            block_size_t alloc_size);

/** \brief Find the containing SPHLockFreeHeap for a block of memory.
*
*	The containing lock free heap is found, if posible, then return
*	LF heap address or NULL.
*	From the near object pointer round down using increasing powers
*	of 2 until a valid block header is found. If the block header
*	is marked as a LockFreeHeap, return the block address.
*	Otherwise return NULL.
*
*	@param nearObj pointer to any current or previous allocation within the target heap.
*	@return pointer to the containing SPHLockFreeHeap_t, or NULL if not found.
*/
extern __C__ SPHLockFreeHeap_t
SPHLockFreeHeapNearFind(void *nearObj);

#if 0
/* Would be used for integral allocations but is currently not used */
extern __C__ void
SPHLockFreeHeapNearDealloc(void *memAddr, long allocSize);
#endif

#endif /* __SPH_LOCK_FREE_HEAP_H */
