/*
 * Copyright (c) 2004-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - documentation
 */

#ifndef __SAS_COMPOUND_HEAP_H
#define __SAS_COMPOUND_HEAP_H

#include "sassimpleheap.h"

/*!
 * \file  src/sascompoundheap.h
 * \brief Shared Address Space Compound Heap.
 *
 * Allocate a SAS block to be used as a Heap of SASSimpleHeap_t heaps.
 * This is a useful constuct for managing complex data structures
 * while maintaining some storage (cache, page, block segment) affinity.
 * The implementation is based on allocation of sub-heaps and allocating
 * structures "near" an already allocated structure (New Near).
 *
 * With a Compound Heap it is easy to start a new sub group by allocating
 * a new SASSimpleHeap_t from the containing SASCompoundHeap_t.
 * The application allocates the "top" or "root" structure directly from
 * this Simple Heap. Subsequent allocations that need good locality to
 * the "root" can use the SASSimpleHeapNearAlloc() API. This API allocates
 * storage from the Simple Heap nearest that provided (near object) address.
 *
 * The allocated Simple Heaps are always a power of two size allocated
 * on a matching power of two boundary.
 * The runtime can always find the containing Simple Heap based on the
 * address of any contained structure. The runtime will allocate from
 * the immediate containing Simple heap if free space is available there.

 * The runtime can also find the containing Compound Heap for any Simple
 * Heap allocated from it.
 * This allows a number of extended near allocation schemes. For example
 * allocations can be from adjacent Simple Heaps within the same
 * Compound Heap. Or allocations can be from a spill area associated
 * with the containing Compound Heap header.
 *
 * \todo A future improvement would specify a Near Window that would
 * restrict near allocations to be from a power of two sub-block of the
 * containing Compound Heap. This could be used with large pages to
 * restrict spill allocations to be from the same large page. For
 * example if the Simple Heaps where allocated as 4K blocks but the
 * physical Page size was 64K or 16M.
 *
 * SAS Compound Heaps can be "expanding" or "fixed". Fixed Compound
 * Heaps will fail that allocation once internal space is exhausted.
 * Expanding Compound Heaps will respond to an allocation failure by
 * allocating a block the size of the original compound heap,
 * initialize it as a compound heap, and chain it the original.
 * If expansion is successful the requested simple heap is allocated
 * from the new space.
 * Finally the storage associated with entire collection of related data
 * structures allocated from a Compound Heap can be freed for reuse
 * (destroyed) with one call.
 *
 * A Compound Heap and the contained complex data structures can be
 * arbitrarily large (up the the limits of the Region size or available
 * disk space). Since SAS blocks are backed by memory mapped files,
 * contained data structures, can be persistent and larger then
 * available system memory.
 *
 * Naturally pages of SAS storage (including Compound Heaps) will be
 * paged into and out of system memory by the OS as need. The natural
 * storage locality of a properly used Compound Heap will help to
 * minimize paging. However is sometimes useful to advise the OS of
 * the programs usage pattern. For example which storage (blocks/pages):
 * - will be needed in the near future,
 * - should be written to disk for checking-pointing
 * - should be written to disk and removed from real storage
 * - Whether we expect sequential or random access to virtual storage
 * and backing file.
 *
 * The SAS
 * Compound Heap supports these operations (above) as described in sasmsync.h.
 *
 *
 * To create fixed compound heap use the function ::SASCompoundFixedHeapCreate
 * or create the initial allocation of an expanding heap with
 * ::SASCompoundHeapCreate or ::SASCompoundHeapCreatePageSize.
 * The Default allocation is 4K which can be overridden with
 * ::SASCompoundHeapCreatePageSize.
 * The function ::SASCompoundHeapExpandCreate is used to expand the an
 * expanding Compound Heap.
 * The function ::SASCompoundHeapDestroy destroys the entire Compound Heap
 * including any expansion blocks .
 *
 * A new Simple Heap can be sub-allocated from a Compound Heap using
 * ::SASCompoundHeapAlloc or ::SASCompoundHeapNearAlloc and freed by
 * using the functions ::SASCompoundHeapFree and
 * ::SASCompoundHeapNearDealloc respectively.
 *
 * \note the implementations of sasstringbtree.h and sasindex.h are
 * derived from the Compound Heap. The internal BTree nodes are derived
 * from the sassimpleheap.h implementation.
 * This improves locality of reference as each node (key list and key
 * data) is contained within a single page (unless the node fills with
 * long keys and some data spills).
 *
 * \todo A lock Free Compound Heap to compliment the SPH Lock Free Heap
 * implementation of sphlockfreeheap.h would be a useful addition to
 * the SAS/SPH runtime.
 *
 */

/*! 
 * \brief Handle to SAS Compound Heap.
 * The type is SAS_RUNTIME_COMPOUNDHEAP.
 */
typedef void *SASCompoundHeap_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Initialize a shared storage as a compound heap.
 *
 * Initialize the control blocks within the specified storage block as a
 * Compound Heap. Both \a heap_size and \a page_size must be power of two in
 * size and have the same power of two (or better) alignment.
 * The SAS type created is SAS_RUNTIME_COMPOUNDHEAP.
 *
 * @param heap_block Block of allocated SAS storage.
 * @param heap_size Size of the simple heap within the block.
 * @param page_size Size of each Simple Heap to be created.
 * @param expanding Set the compound heap to expand and associate a load
 * factor to used to determine when to expand.
 * @return A handle to the initialized SASSimpleHeap_t or 0 if an error
 * occurs.
 */
extern __C__ SASCompoundHeap_t
SASCompoundHeapInit (void *heap_block, block_size_t heap_size,
		     block_size_t page_size, int expanding);

/*!
 * \brief Create an expanding SAS Compound Heap based on existing \a heap.
 *
 * Create an expanding SAS Compound Heap and add the \a heap on its internal
 * list.
 * Intended for internal runtime use but can be used to force expansion
 * of the specified Compound Heap.
 *
 * @param heap The SAS Compound Heap to add on expanding heap.
 * @return A new SASCompoundHeap handle or 0 if the new heap creation fails.
 */
extern __C__ SASCompoundHeap_t
SASCompoundHeapExpandCreate (SASCompoundHeap_t heap);

/*!
 * \brief Create a new SAS Compound Heap with \a heap_size size.
 *
 * Create and initialize a Compound Heap. The storate block must be power of
 * two in size and SAS type returned is SAS_RUNTIME_COMPOUNDHEAP. The internal
 * page size used is the default one defined in sasalloc.h (4096).
 *
 * @param heap_size Size of the Compound Heap to create.
 * @return A handle to created SASCompoundHeap_t or 0 if creation fails.
 */
extern __C__ SASCompoundHeap_t SASCompoundHeapCreate (block_size_t heap_size);

/*!
 * \brief Create a new SAS Compound Heap with \a heap_size size and \a 
 * page_size page size.
 *
 * Similar to ::SASCompoundHeapCreate but with additional option to set the
 * internal page size.
 *
 * @param heap_size Size of the Compound Heap to create.
 * @param page_size Size of the internal page.
 * @return A handle to created SASCompoundHeap_t or 0 if creation fails.
 */
extern __C__ SASCompoundHeap_t
SASCompoundHeapCreatePageSize (block_size_t heap_size,
			       block_size_t page_size);

/*!
 * \brief Create a new non expanding SAS Compound heap with \a heap_size.
 *
 * Similar to ::SASCompoundHeapCreate but without the option to expand when
 * load factor allows it.
 *
 * @param heap_size Size of the Compound Heap to create.
 * @return A handle to created SASCompoundHeap_t or 0 if the creating fails.
 */
extern __C__ SASCompoundHeap_t
SASCompoundFixedHeapCreate (block_size_t heap_size);

/*!
 * \brief Destroy the SAS Compound Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP. Destroy holds an exclusive
 * write lock while clearing the control blocks and freeing the SAS block.
 *
 * @param heap Handle of the SASCompoundHeap_t to be destroyed.
 */
extern __C__ void SASCompoundHeapDestroy (SASCompoundHeap_t heap);

/*!
 * \brief Set the SAS Compound Heap \a heap load factor to \a load.
 *
 * @param heap Handle to the SASCompoundHeap_t to be adjusted.
 * @param load Load factor to set.
 */
extern __C__ void
SASCompoundHeapSetLoadFactor (SASCompoundHeap_t * heap, int load);

/*!
 * \brief Return the load factor from SAS Compound Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return The load factor from heap or -1 if and error occurs.
 */
extern __C__ int SASCompoundHeapGetLoadFactor (SASCompoundHeap_t * heap);

/*!
 * \brief Return the page size from SAS Compound Heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return The \a page size value in bytes. Used to allocate Simple Heaps.
 */
extern __C__ block_size_t SASCompoundHeapAllocSize (SASCompoundHeap_t heap);

/*!
 * \brief Return the total available free space on SAS Compound heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP. The function holds a
 * write lock while calculating the total free space from heap.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return The total available free space in bytes.
 */
extern __C__ block_size_t SASCompoundHeapFreeSpace (SASCompoundHeap_t heap);

/*!
 * \brief Return the total block space allocated to this SAS
 * Compound heap \a heap.
 *
 * The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP. The function holds a
 * read lock while calculating the total block space,
 * including expansion blocks.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return The total block space in bytes.
 */
extern __C__ block_size_t SASCompoundHeapAllocSpace (SASCompoundHeap_t heap);

/*!
 * \brief Write down the SAS Compound Heap to persistent storage.
 *
 * The function basically call sasMsyncWrite from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function hold a read lock over the heap's memory segments.
 *
 * @param heap Handle the the SASCompoundHeap_t.
 * @param asyncBool Flag for asynchronous action if true.
 */
extern __C__ block_size_t
SASCompoundHeapWriteAll (SASCompoundHeap_t heap, int asyncBool);

/*!
 * \brief Write down the SAS Compound Heap to persistent storage and inform
 * the kernel that those pages can be removed from real memory.
 *
 * The function basically calls sasMsyncPurge from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function hold a read lock over the heap's memory segments.
 *
 * @param heap Handle the the SASCompoundHeap_t.
 * @param asyncBool Flag for asynchronous action if true.
 */
extern __C__ block_size_t
SASCompoundHeapPurgeAll (SASCompoundHeap_t heap, int asyncBool);

/*!
 * \brief Inform the kernel that the SAS Compound Heap memory segments can be
 * removed from real memory.
 *
 * The function basically calls sasMsyncRelease from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function hold a read lock over the heap's memory segments.
 *
 * @param heap Handle the the SASCompoundHeap_t.
 */
extern __C__ block_size_t SASCompoundHeapReleaseAll (SASCompoundHeap_t heap);

/*!
 * \brief Inform the kernel that the SAS Compound Heap memory segments pages
 * will be needed soon.
 *
 * The function basically calls sasMsyncBring from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function hold a read lock over the heap's memory segments.
 *
 * @param heap Handle the the SASCompoundHeap_t.
 */
extern __C__ block_size_t SASCompoundHeapBringAll (SASCompoundHeap_t heap);

/*!
 * \brief Inform the kernel that the SAS Compound Heap memory segments pages
 * will be needed soon and will be accessed in sequential order.
 *
 * The function basically calls sasMsyncSequential from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function hold a read lock over the heap's memory segments.
 *
 * @param heap Handle the the SASCompoundHeap_t.
 */
extern __C__ block_size_t
SASCompoundHeapSeqAccessAll (SASCompoundHeap_t heap);

/*!
 * \brief Inform the kernel that the SAS Compound Heap memory segments pages
 * will be needed soon and will be accessed in random order.
 *
 * The function basically calls sasMsyncRandom from sasmsync.h on each 
 * internal SAS Simple Heap. The sas_type_t must be SAS_RUNTIME_COMPOUNDHEAP.
 * The function holds a read lock over the heap memory segments.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 */
extern __C__ block_size_t
SASCompoundHeapRandomAccessAll (SASCompoundHeap_t heap);

/*!
 * \brief Sub-Allocate a new SAS Simple Heap from a SAS Compound Heaps
 * internal space.
 *
 * The sas_type_t of \a heap must be SAS_RUNTIME_COMPOUNDHEAP. The allocated 
 * block is initialized as a SAS Simple Heap. The function holds a write lock
 * on the Compound Heap during this operation.
 *
 * @param heap Handle to the SASCompoundHeap_t. 
 * @return A newly created SASSimpleHeap_t or 0 if an error occurs.
 */
extern __C__ SASSimpleHeap_t SASCompoundHeapAlloc (SASCompoundHeap_t heap);

/*!
 * \brief Free the allocated SAS Simple Heap \a block in the SAS Compound
 * Heap \a heap.
 *
 * The sas_type_t of \a heap must be SAS_RUNTIME_COMPOUNDHEAP and the type of
 * \a free_block must be SAS_RUNTIME_SIMPLEHEAP.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @param free_block The created SASSimpleHeap_t created from \a heap.
 */
extern __C__ void
SASCompoundHeapFree (SASCompoundHeap_t heap, SASSimpleHeap_t free_block);

/*!
 * \brief Allocate a new SAS Simple Heap from SAS Compound Heap \a nearObj.
 *
 * The address \a nearObj is used to find the associated Compound Heap object.
 * The sas_type_t of the heap must be SAS_RUNTIME_COMPOUNDHEAP. The allocated 
 * SAS Simple Heap is already initialized.
 *
 * @param nearObj Memory address of SASCompoundHeap_t. 
 * @return A newly created SASSimpleHeap_t or 0 if an error occurs.
 */
extern __C__ SASSimpleHeap_t SASCompoundHeapNearAlloc (void *nearObj);

/*!
 * \brief Free the allocated SAS Simple Heap based on a contained \a memAddr.
 *
 * The address \a memAddr is used to find the associated SAS Simple Heap and
 * the containing SAS Compound Heap. If both objects can be found, the SAS
 * Simple Heap is freed in the SAS Compound Heap.
 *
 * @param memAddr Memory Address of a SASSimpleHeap_t associated with a 
 * SASCompoundHeap_t.
 */
extern __C__ void SASCompoundHeapNearDealloc (void *memAddr);

/*!
 * \brief Destroy the SAS Compound Heap \a heap.
 *
 * Similar to ::SASCompoundHeapDestroy but do not hold the write lock on the
 * memory address.
 *
 * @param heap Handle of the SASCompoundHeap_t.
 */
extern __C__ void SASCompoundHeapDestroyNoLock (SASCompoundHeap_t heap);

/*!
 * \brief Return the total available free space on SAS Compound heap \a heap.
 *
 * Similar to ::SASCompoundHeapFreeSpace but do not hold the write lock on the
 * Compound Heap.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @return The total available free space in bytes.
 */
extern __C__ block_size_t
SASCompoundHeapFreeSpaceNoLock (SASCompoundHeap_t heap);

/*!
 * \brief Sub-Allocate a new SAS Simple Heap from a SAS Compound Heaps
 * internal space.
 *
 * Similar to ::SASCompoundHeapAlloc but do not hold the write lock on the 
 * Compound Heap.
 *
 * @param heap Handle to the SASCompoundHeap_t. 
 * @return A newly created SASSimpleHeap_t or 0 if an error occurs.
 */
extern __C__ SASSimpleHeap_t
SASCompoundHeapAllocNoLock (SASCompoundHeap_t heap);

/*!
 * \brief Allocate a new SAS Simple Heap from SAS Compound Heap \a nearObj.
 *
 * Similar to ::SASCompoundHeapNearAlloc but do not hold any locks.
 *
 * @param nearObj Memory address of SASCompoundHeap_t. 
 * @return A newly created SASSimpleHeap_t or 0 if an error occurs.
 */
extern __C__ SASSimpleHeap_t SASCompoundHeapNearAllocNoLock (void *nearObj);

/*!
 * \brief Free the allocated SAS Simple Heap \a memAddr from associated SAS
 * Compound Heap.
 *
 * Similar to ::SASCompoundHeapNearDealloc but do not hold any locks.
 *
 * @param memAddr Memory Address of a SASSimpleHeap_t associated with a 
 * SASCompoundHeap_t.
 */
extern __C__ void SASCompoundHeapNearDeallocNoLock (void *memAddr);

/*!
 * \brief Free the allocated SAS Simple Heap \a free_block from SAS Compound
 * Heap \a heap.
 *
 * Similar to ::SASCompoundHeapFree but do not hold the write lock.
 *
 * @param heap Handle to the SASCompoundHeap_t.
 * @param free_block The created SASSimpleHeap_t created from \a heap.
 */
extern __C__ void
SASCompoundHeapFreeNoLock (SASCompoundHeap_t heap,
			   SASSimpleHeap_t free_block);

#endif /* __SAS_COMPOUND_HEAP_H */
