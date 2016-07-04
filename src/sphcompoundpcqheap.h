/*
 * sphcompoundpcqheap.h
 *
 *  Created on: May 28, 2016
 *      Author: sjmunroe
 */

#ifndef SRC_SPHCOMPOUNDPCQHEAP_H_
#define SRC_SPHCOMPOUNDPCQHEAP_H_


/*!
 * \file  src/sphcompoundpcqheap.h
 * \brief Shared Address Space Compound Heap, PCQueue extension.
 *
 * This API extends the basic heap of heaps (CompoundHeap) concept to
 * allow sub-allocation of collections of other SAS or SPH objects.
 * In this case specifically a SPHSinglePCQueue_t.
 *
 * The base CompoundHeap allocates SAS blocks to be used as a Heap of
 * other SAS / SPH utility objects, in this case SPHSinglePCQueue_t.
 * This is a useful construct for managing complex data structures
 * while maintaining some storage (cache, page, block segment) affinity.
 * The implementation is based on allocation of sub-heaps and allocating
 * structures "near" an already allocated object (via NearAlloc).
 *
 * With a Compound Heap it is easy to allocate groups of related
 * SPHSinglePCQueue_t's together from the containing SASCompoundHeap_t
 * using the SPHCompoundPCQAlloc() API. This API honors the Load Factor
 * percentage. The Default load factor is 75% (reserving 25% of each
 * block for near allocation) but can be changed via the
 * SASCompoundHeapSetLoadFactor() API. A load factor of 100% fills
 * each block to capacity.
 *
 * \note For 100% load factor, there is no difference between Alloc
 * and NearAlloc.
 *
 * Allocatation continues from the current SAS block until
 * the Load Factor is exceeded for that block. The next allocation
 * forces the allocation of a new extended heap block and the PCQueue
 * allocation is satisfied from the new block.
 *
 * Any remaining space, above the Load Factor, and any subsequence
 * freed heap space is available for near allocation.
 * Subsequent allocations that need good locality to another PCQueue
 * can use the SPHCompoundPCQNearAlloc() API. This API allocates storage
 * from the Compound Heap block nearest that provided (near object)
 * address.
 *
 * The allocated PCQueues are always a power of two size allocated
 * on a matching power of two boundary.
 * The runtime can always find the containing PCQueue based on the
 * address of any contained structure. The runtime will allocate from
 * the immediate containing Simple heap if free space is available there.
 *
 * The runtime can also find the containing Compound Heap for any
 * PCQueue allocated from it.
 * This allows a number of extended near allocation schemes.
 * SAS Compound Heaps can be "expanding" or "fixed".
 * Finally the storage associated with entire collection of related data
 * structures allocated from a Compound Heap can be freed for reuse
 * (destroyed) with one call.
 *
 * If the user needs to manage PCQueues of multiple sizes via this
 * (SPHCompoundPCQ) API then it is necessary to create multiple
 * SASCompoundHeap_t's to allocate from the SASCompoundHeap_t
 * allocating the required PCQueue size.
 *
 * SASCompoundHeap_t's created via SASCompoundHeapCreate() will
 * allocate the default 4KB (4096 bytes) SPHSinglePCQueue_t.
 * To allocate smaller or larger PCQueues using this
 * (SASCompoundHeap_t) mechanism requires creating CompoundHeaps via
 * SASCompoundHeapCreatePageSize(), where the "page_size" parameter
 * will apply to all PCQueues allocated from that specific
 * CompoundHeap.  The smallest supported PCQueue size is 512 bytes
 * which is effectively one entry.
 *
 * The heap_size parameter effects the granularity of extension as
 * the heap expands. A smaller (PCQueue) page_size can be paired with a
 * smaller heap_size while larger page_size should be paired with
 * larger heap_size as appropriate.  The heap_size should be a
 * power of 2 multiple of the allocation page_size, between 256 and
 * 4096 times the page_size.
 *
 * A Compound Heap and the contained complex data structures can be
 * arbitrarily large (up the the limits of the Region size or available
 * disk space). Since SAS blocks are backed by memory mapped files,
 * contained data structures, can be persistent and larger then
 * available system memory.
 *
 * A new PCQueue can be sub-allocated from a Compound Heap using
 * ::SPHCompoundPCQAlloc or ::SPHCompoundPCQNearAlloc and freed by
 * using the functions ::SPHCompoundPCQFree respectively.
 */

#include "sascompoundheap.h"

#include "sphsinglepcqueue.h"

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Sub-Allocate a new SPH PCQueue from a SAS Compound Heaps
 * internal space.
 *
 * The sas_type_t of \a heap must be SAS_RUNTIME_COMPOUNDHEAP. The allocated
 * block is initialized as a SPH PCQueue. The function holds a write lock
 * on the Compound Heap during this operation.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return A newly created SPHSinglePCQueue_t or 0 if an error occurs.
 */
extern __C__ SPHSinglePCQueue_t SPHCompoundPCQAlloc (SASCompoundHeap_t heap);

/*!
 * \brief Free the allocated SPH PCQueue \a block in the SAS Compound
 * Heap \a heap.
 *
 * The sas_type_t of \a heap must be SAS_RUNTIME_COMPOUNDHEAP and the type of
 * \a free_block must be SAS_RUNTIME_PCQUEUE.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @param free_block The created SPHSinglePCQueue_t created from \a heap.
 */
extern __C__ void
SPHCompoundPCQFree (SASCompoundHeap_t heap, SPHSinglePCQueue_t free_block);

/*!
 * \brief Allocate a new SPH PCQueue from SAS Compound Heap \a nearObj.
 *
 * The address \a nearObj is used to find the associated Compound Heap object.
 * The sas_type_t of the heap must be SAS_RUNTIME_COMPOUNDHEAP. The allocated
 * SPH PCQueue is already initialized.
 *
 * @param nearObj Memory address of SASCompoundHeap_t.
 * @return A newly created SPHSinglePCQueue_t or 0 if an error occurs.
 */
extern __C__ SPHSinglePCQueue_t SPHCompoundPCQNearAlloc (void *nearObj);

/*!
 * \brief Sub-Allocate a new SPH PCQueue from a SAS Compound Heaps
 * internal space.
 *
 * Similar to ::SPHCompoundPCQAlloc but does not take write lock the
 * Compound Heap.  This API assumes that the application is holding a
 * write lock on the referenced Compound Heap.  This allows an
 * application to batch a group of allocations with less overhead.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return A newly created SPHSinglePCQueue_t or 0 if an error occurs.
 */
extern __C__ SPHSinglePCQueue_t
SPHCompoundPCQAllocNoLock (SASCompoundHeap_t heap);

/*!
 * \brief Allocate a new SPH PCQueue from SAS Compound Heap \a nearObj.
 *
 * Similar to ::SPHCompoundPCQNearAlloc but does not take write lock the
 * Compound Heap. This API assumes that the application is holding a
 * write lock on the referenced Compound Heap.  This allows an
 * application to batch a group of allocations with less overhead.
 *
 * @param nearObj Memory address within a SASCompoundHeap_t.
 * @return A newly created SPHSinglePCQueue_t or 0 if an error occurs.
 */
extern __C__ SPHSinglePCQueue_t SPHCompoundPCQNearAllocNoLock (void *nearObj);

/*!
 * \brief Free the allocated SPH PCQueue \a free_block from SAS Compound
 * Heap \a heap.
 *
 * Similar to ::SPHCompoundPCQFree but does not take write lock the
 * Compound Heap. This API assumes that the application is holding a
 * write lock on the referenced Compound Heap.  This allows an
 * application to batch a group of frees with less overhead.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @param free_block The created SPHSinglePCQueue_t created from \a heap.
 */
extern __C__ void
SPHCompoundPCQFreeNoLock (SASCompoundHeap_t heap,
		SPHSinglePCQueue_t free_block);

#endif /* SRC_SPHCOMPOUNDPCQHEAP_H_ */
