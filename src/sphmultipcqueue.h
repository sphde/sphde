/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Paul Clarke - based heavily on sphsinglepcqueue
 */

#ifndef __SPH_MULTI_PC_QUEUE_H
#define __SPH_MULTI_PC_QUEUE_H

/**! \file sphmultipcqueue.h
*  \brief Shared Persistent Heap, multi producer multi consumer queue.
*	For shared memory multi-thread/multi-core applications.
*	This implementation uses atomic operations to implement Lock
*	Free Producer/Consumer queues (SPHMultiPCQueue_t).
*
*	This API supports atomic allocation of storage for queue
*	entries for zero copy persistence and sharing. Zero copy queues
*	divides the process of producing a queue entry in to three steps:
*	- Allocate the queue entry (and initialize the header)
*	- Use the return entry handle to fill in application specific data.
*	- Marks the entry complete in the header.
*
*	\code
#include <sphlfentry.h>
SPHLFEntryHandle_t *handle;
typedef struct data_struct11 {
	double	field0;
	int		field1;
	int		field2;
	void	*field3;
} data_struct11;

// Allocate zero copy queue entry
handle = SPHMultiPCQueueAllocStrideDirectTM (pcqueue);
if (handle)
{	// insert data into the allocated queue entry
	sphLFEntryID_t tmpl;
	data_struct11 *struct_ptr;

	tmpl = SPHMultiPCQueueGetEntryTemplate(pcq);

	struct_ptr = (data_struct11 *)SPHLFEntryDirectGetFreePtr(handle);
	if (struct_ptr)
	{	// store struct fields directly into allocated queue entry
		struct_ptr->field0 = data_double1;
		struct_ptr->field1 = data_int2;
		struct_ptr->field2 = data_int3;
		struct_ptr->field3 = (void*)sas_data_buff2;
	} else {
		printf("error  SPHENTRYALLOCSTRUCT(%p, data_struct11) failed)\n",
			   handle);
	}
	// Mark the entry complete and available to the consumer
	SPHLFEntryDirectComplete(handle)
} else {
	while (SPHMultiPCQueueFull(pcqueue))
	{
		// pacing code
	}
}
*	\endcode
*
*	The consumer can access queue entries once they are marked complete.
*	The consumer:
*	- checks (spins) for the next allocated entry to become complete.
*	- uses the returned entry handle to directly access the entry contents.
*	- When done processing the queue entry, it marks the entry header
*	invalid and deallocates the entry.
*	- This makes the next queue entry available, if any.
*
*	\code
#include <sphlfentry.h>
SPHLFEntryHandle_t *handle;
typedef struct data_struct11 {
	double	field0;
	int	field1;
	int	field2;
	void	*field3;
} data_struct11;

// Get next queue entry
handle = SPHMultiPCQueueGetNextComplete (pcqueue, &handle0);
if (handle)
{	// insert data into the allocated queue entry
	data_struct11 *struct_ptr;
	struct_ptr  = (data_struct11*)SPHLFEntryGetNextPtr (handle);
	if (struct_ptr)
	{	// access struct fields directly from queue entry
		data_double1	= struct_ptr->field0;
		data_int2	= struct_ptr->field1;
		data_int3	= struct_ptr->field2;
		sas_data_buff2	= struct_ptr->field3;
	} else {
		printf("error  SPHLFEntryGetNextPtr(%p, data_struct11) failed)\n",
			   handle);
	}
	// Mark the entry free and available for reuse
	SPHMultiPCQueueFreeNextEntry(pcqueue)
} else {
	while (SPHMultiPCQueueEmpty(pcqueue))
	{
		// pacing code
	}
}
*	\endcode

*	In this implementation the allocation of the entry is serialized
*	using Transactional Memory for atomic updates.
*
*	As an option the queue entry allocator will fill in a 4 or 16 byte
*	entry header with: 
*       - Entry status and length.
*       - Entry identifying Category and SubCategory codes. 
*       - Process and Thread Ids.
*       - High resolution timestamp.
*
*	Any additional storage allocated to the entry (after the header)
*	is available for application specific data.  This API also provides
*	several mechanisms to store application data including; direct
*	array or structure overlay, and a streams like mechanism.
*	The API provides a completion function (SPHLFEntryDirectComplete)
*	which provides any memory barriers required by the platform and
*	marks the entry complete.
*
*	The API support simple circular queues and requires a constant
*	entry stride. A stride that matches or is multiple of the
*	cache line size can improve performance by avoiding
*	"false sharing" of cache lines containing multiple queue entries
*	across cores/sockets.
*
*  \todo Additional work will include automatic pacing with Hysteresis
*/

#include "sastype.h"
#include "sphlfentry.h"
#include "sphdirectpcqueue.h"

/** \brief Handle to an instance of SPH Lock Free Multi Producer,
*   Multi Consumer Queue.
*
*	The type is SAS_RUNTIME_PCQUEUE
*/
typedef void *SPHMultiPCQueue_t;

/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif
#if 0
/** \brief unsigned int type, consistent with the size of a pointer and used for pointer calculations **/
typedef unsigned long longPtr_t;
#endif
/** \brief internal options flag for circular log buffers **/
#define SPHMPCQUEUE_CIRCULAR (1)
/** \brief internal options flag set when circular log buffers have wrapped **/
#define SPHMPCQUEUE_CIRCULAR_WRAPPED (1<<1)
/** \brief internal options flag set when circular log buffers have wrapped multiple times **/
#define SPHMPCQUEUE_CIRCULAR_NOTFIRST (1<<2)
/** \brief internal options flag for prefetching the immediate (0 offset) cache-line **/
#define SPHMPCQUEUE_CACHE_PREFETCH0 (1<<3)
/** \brief internal options flag for prefetching the next (line size offset) cache-line **/
#define SPHMPCQUEUE_CACHE_PREFETCH1 (1<<4)
/** \brief internal options mask flag used the reset circular log buffers **/
#define SPHMPCQUEUE_CIRCULAR_RESETMASK (SPHMPCQUEUE_CIRCULAR | \
		SPHMPCQUEUE_CACHE_PREFETCH0 | \
		SPHMPCQUEUE_CACHE_PREFETCH1)

/** \brief Initialize a shared storage block as a Lock Free PC Queue
*
*	Initialize the specified storage block as Lock Free PC Queue
*	control blocks.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_PCQUEUE.
*
*	@param buf_seg a block of allocated SAS storage matching the buf_size.
*	@param buf_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SPHMultiPCQueue_t.
*/
extern __C__ SPHMultiPCQueue_t
SPHMultiPCQueueInit (void *buf_seg , block_size_t buf_size);

/** \brief Initialize a shared storage block as a Lock Free
*   Multi Producer Multi Consumer Queue
*	with a fixed entry stride.
*
*	Initialize the specified storage block as Lock Free PC Queue
*	control blocks.
*	The stride and control flags are also stored.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_PCQUEUE.
*
*	@param buf_seg a block of allocated SAS storage matching the buf_size.
*	@param buf_size power of two size of the heap to be initialized.
*	@param entry_stride the stride offset is bytes between allocated entries.
*	@param options option bits.
*	@return a handle to the initialized SPHMultiPCQueue_t.
*/
extern __C__ SPHMultiPCQueue_t
SPHMultiPCQueueInitWithStride (void* buf_seg,  block_size_t buf_size,
                           unsigned short entry_stride,
                           unsigned int options);

/** \brief Allocate and initialize a shared storage block as a Lock Free
*	Multi Producer Multi Consumer Queue.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a PC Queue.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SPHMultiPCQueue_t.
*/
extern __C__ SPHMultiPCQueue_t
SPHMultiPCQueueCreate (block_size_t buf_size);

/** \brief Allocate and initialize a shared storage block as a Lock Free
*	Multi Producer Multi Consumer Queue.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a Logger.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@param stride the stride offset is bytes between allocated entries.
*	@return a handle to the initialized SPHMultiPCQueue_t.
*/
extern __C__ SPHMultiPCQueue_t
SPHMultiPCQueueCreateWithStride (block_size_t buf_size,
                           unsigned short stride);

/** \brief Return the entry stride for an existing Lock Free
*       Multi Producer Multi Consumer Queue.
*
*       @param queue Handle of a producer consumer queue.
*       @return the entry stride of strided queues, 0 if not strided,
*       or -1 is not a valid SPHMultiPCQueue_t.
*/
extern __C__ int
SPHMultiPCQueueGetStride (SPHMultiPCQueue_t queue);

/** \brief Return the total number of entries for an existing Lock Free
*       Multi Producer Multi Consumer Queue.
*
*       @param queue Handle of a producer consumer queue.
*       @return total number of entries of strided queues, 0 if not strided,
*       or -1 is not a valid SPHMultiPCQueue_t.
*/
extern __C__ int
SPHMultiPCQueueGetEntries (SPHMultiPCQueue_t queue);

#if defined(SPH_INTERNAL)
/** \brief Resets the specific queue to empty state asynchronously (without locking or atomic updates).
*
*	Internal use for testing.
*
*	@param queue Handle to a queue.
*	@return 0 if successful.
*/
extern __C__ int
SPHMultiPCQueueResetAsync (SPHSinglePCQueue_t queue);
#endif

/** \brief Return the status of the specified queue.
*
*	@param queue Handle to a queue.
*	@return true if the queue is currently Empty (no entries).
*	Otherwise False.
*/
extern __C__ int
SPHMultiPCQueueEmpty (SPHMultiPCQueue_t queue);

/** \brief Return the status of the specified queue.
*
*	@param queue Handle to a queue.
*	@return true if the queue is currently full.
*	Otherwise False.
*/
extern __C__ int
SPHMultiPCQueueFull (SPHMultiPCQueue_t queue);

/** \brief Returns the amount of free space (in bytes) remaining in the specified queue.
*
*	@param queue Handle to a queue.
*	@return number of bytes of free space remaining in the queue buffer.
*/
extern __C__ block_size_t
SPHMultiPCQueueFreeSpace (SPHMultiPCQueue_t queue);

/** \brief Return the entry template for an existing Lock Free
*   Multi Producer Multi Consumer Queue.
*   This template is used later to mark an allocated entry complete.
*
*   @param queue Handle of a producer consumer queue.
*   @return the entry template for this queue or
*   0 if not a valid SPHMultiPCQueue_t.
*/
extern __C__ sphLFEntryID_t
SPHMultiPCQueueGetEntryTemplate (SPHMultiPCQueue_t queue);

/** \brief Allows the producer thread to allocate and initialize the
*   header of a queue entry for access. The allocation is from the
*   specified Multi Producer Multi Consumer Queue.
*
*       The allocation size is the stride set when the PC queue was
*       initialized/created.
*       The Entry status and length
*       are stored in the header of the new entry.
*       Returns an dire ctentry handle which allows the application to insert
*       application specific data into the entry via the sphlfentry.h API.
*       If the specified queue is full the allocation may fail.
*
*       \note The queue entry is not ready for access by the Consumer
*       thread, until additional application data is inserted and the
*       entry is completed (via SPHLFEntryDirectComplete).
*       Category and Subcategory may be supplied as the entry is completed.
*
*       @param queue Handle of a producer consumer queue.
*       @return Direct handle of the initialized queue entry,
*       or 0 (NULL) if the allocation failed.
*       For example the Allocate may fail if the queue is full.
*/
extern __C__ SPHLFEntryDirect_t
SPHMultiPCQueueAllocStrideDirectTM (SPHMultiPCQueue_t queue);

/** \brief Allows the consumer to get the next completed queue entry
*       from the specified multi producer multi consumer queue.
*
*       Returns an direct entry handle which allows the application
*       to access the application specific data inserted by the
*       produced thread.
*       If the specified queue is empty or the next queue is not yet
*       completed the get may fail.
*
*       @param queue Handle of a producer consumer queue.
*       @return Direct Handle of the initialized logger entry,
*       or 0 (NULL) if the get failed.
*       For example the Get may fail if the queue
*       is empty or the next tail entry is not yet completed.
*/
extern __C__ SPHLFEntryDirect_t
SPHMultiPCQueueGetNextCompleteDirectTM (SPHMultiPCQueue_t queue);

/** \brief Return the status of the entry specified by the direct entry handle.
*
*       @param directHandle entry Handle for an allocated entry.
*       @return true if the entry was complete (SPHMultiPCQueueEntryComplete
*       has been called for this entry). Otherwise False.
*/
extern __C__ int
SPHMultiPCQueueEntryDirectIsComplete (SPHLFEntryDirect_t directHandle);

/** \brief Allows the consumer to free the queue entry it just processed
*   (using SPHMultiPCQueueGetNextComplete),
*	from the specified multi producer multi consumer queue.
*
*	Mark the provided entry as free (unallocated and invalid)
*	and if this entry is the current queue tail, bump the queue tail
*	pointer to the next unallocated entry.
*	If the specified queue is empty or the current tail entry is not yet
*	completed the Free may fail.
*
*	\warning The Consumer thread should not touch or modify a queue entry
*	after calling FreeEntry.
*	This is important to both correctness and performance.
*
*	@param queue Handle of a producer consumer queue.
*	@param entry queue entry to be marked free
*	@return True for successful tail free, otherwise indicated failure.
*	For example the Get may fail if the queue
*	is empty or the next tail entry is not yet completed.
*/
extern __C__ int
SPHMultiPCQueueFreeEntryDirect (SPHMultiPCQueue_t queue,
		SPHLFEntryDirect_t entry);

/** \brief Allows the consumer to get the next allocated queue entry
*       from the specified multi producer multi consumer queue.
*
*       Returns an direct entry handle which allows the application to
*       access the entry allocated by the produced thread.
*       If the specified queue is empty or the next queue is not yet
*       allocated the get may fail.
*       Returning a entry does not mean the the producer has completed
*       the entry and the consumer wait/spin (SPHLFEntryDirectIsComplete)
*       for the entry to become complete.
*
*       @param queue Handle of a producer consumer queue.
*       @return Direct Handle of the initialized logger entry,
*       or 0 (NULL) if the get failed.
*       For example the Get may fail if the queue
*       is empty or the next tail entry is not yet allocated.
*/
extern __C__ SPHLFEntryDirect_t
SPHMultiPCQueueGetNextEntry (SPHMultiPCQueue_t queue);

/** \brief Set the cache-line prefetch options for entry allocate.
*
*   prefetch == 0; No prefetch issued.
*   \n prefetch == 1; Prefetch the currently allocated cache-line.
*   \n prefetch == 2; Prefetch the cache-line following the allocated entry.
*   \n prefetch == 3; Prefetch the current and next cache-lines.
*
*	@param queue Handle to a queue.
*	@param prefetch prefetch option code.
*	@return 0 if successful.
*/
extern __C__ int
SPHMultiPCQueueSetCachePrefetch (SPHMultiPCQueue_t queue, int prefetch);

/** \brief Prefetch pages from the specific queue.
*
*	@param queue Handle to a queue.
*	@return 0 if successful.
*/
extern __C__ int
SPHMultiPCQueuePrefetch (SPHMultiPCQueue_t queue);

/** \brief Destroys the queue and frees the SAS storage for reuse.
*
*	@param queue Handle to a queue to be destroyed.
*	@return 0 if successful.
*/
extern __C__ int 
SPHMultiPCQueueDestroy (SPHMultiPCQueue_t queue);

#endif /* __SPH_MULTI_PC_QUEUE_H */
