/*
 * Copyright (c) 2012-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SPH_SINGLE_PC_QUEUE_H
#define __SPH_SINGLE_PC_QUEUE_H

/**! \file sphsinglepcqueue.h
*  \brief Shared Persistent Heap, single producer single consumer queue.
*	For shared memory multi-thread/multi-core applications.
*	This implementation uses atomic operations to implement Lock
*	Free Producer/Consumer queues (SPHSinglePCQueue_t).
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
SPHLFEntryHandle_t *handle, handle0;
typedef struct data_struct11 {
	double	field0;
	int		field1;
	int		field2;
	void	*field3;
	} data_struct11;

	// Allocate zero copy queue entry
	handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, CAT_CODE, SUBCAT_CODE, &handle0);
	if (handle)
	{	// insert data into the allocated queue entry
		data_struct11 *struct_ptr;
		if (SPHLFEntryAddPtr (handle, (void*) sas_data_buff)
		{
			printf("error  SPHLFEntryAddPtr(%p, sas_data_buff) failed)\n",
				   handle);
		}
		if (SPHLFEntryAddInt (handle, data_int1))
		{
			printf("error  SPHLFEntryAddInt(%p, data_int1) failed)\n",
				   handle);
		}
		if (SPHLFEntryAddString (handle, (char*)data_string1))
		{
			printf("error  SPHLFEntryAddString(%p, data_string1) failed)\n",
				   handle);
		}

		struct_ptr  = (data_struct11*)SPHENTRYALLOCSTRUCT (handle, data_struct11);
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
		SPHSinglePCQueueEntryComplete(handle)
	} else {
		while (SPHSinglePCQueueFull(pcqueue))
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
SPHLFEntryHandle_t *handle, handle0;
typedef struct data_struct11 {
	double	field0;
	int		field1;
	int		field2;
	void	*field3;
	} data_struct11;

	// Get next queue entry
	handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
	if (handle)
	{	// insert data into the allocated queue entry
		data_struct11 *struct_ptr;
		sas_data_buff = SPHLFEntryGetNextPtr (handle);
		if (!sas_data_buff)
		{
			printf("error  SPHLFEntryAddPtr(%p, sas_data_buff) failed)\n",
				   handle);
		}
		data_int1 = SPHLFEntryGetNextInt (handle);
		data_string1 = SPHLFEntryGetNextString (handle);
		if (data_string1)
		{
			printf("error  SPHLFEntryGetNextString(%p, data_string1) failed)\n",
				   handle);
		}

		struct_ptr  = (data_struct11*)SPHENTRYALLOCSTRUCT (handle, data_struct11);
		if (struct_ptr)
		{	// access struct fields directly from queue entry
			data_double1	= struct_ptr->field0;
			data_int2		= struct_ptr->field1;
			data_int3		= struct_ptr->field2;
			sas_data_buff2	= struct_ptr->field3;
		} else {
			printf("error  SPHENTRYALLOCSTRUCT(%p, data_struct11) failed)\n",
				   handle);
		}
		// Mark the entry free and available for reuse
		SPHSinglePCQueueFreeNextEntry(pcqueue)
	} else {
		while (SPHSinglePCQueueEmpty(pcqueue))
		{
			// pacing code
		}
	}
*	\endcode
*
*	In this implementation the allocation of the entry is minimally
*	serialized based on the assumption that only one (producer) thread
*	will be allocating queue entries.
*	Likewise the assumption is that there is only one consumer thread
*	per SPHSinglePCQueue_t instance.
*	This allows independent producer/consumer thread pairs to interact
*	with a queue instance with minimum synchronization and overhead.
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
*	The API provides a completion function (SPHSinglePCQueueEntryComplete)
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
*
*/

#include "sastype.h"
#include "sphlfentry.h"


/** \brief Handle to an instance of SPH Lock Free Single Producer,
*   Single Consumer Queue.
*
*	The type is SAS_RUNTIME_PCQUEUE
*/
typedef void *SPHSinglePCQueue_t;

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
#define SPHSPCQUEUE_CIRCULAR (1)
/** \brief internal options flag set when circular log buffers have wrapped **/
#define SPHSPCQUEUE_CIRCULAR_WRAPED (1<<1)
/** \brief internal options flag set when circular log buffers have wrapped multiple times **/
#define SPHSPCQUEUE_CIRCULAR_NOTFIRST (1<<2)
/** \brief internal options flag for prefetching the immediate (0 offset) cache-line **/
#define SPHSPCQUEUE_CACHE_PREFETCH0 (1<<3)
/** \brief internal options flag for prefetching the next (line size offset) cache-line **/
#define SPHSPCQUEUE_CACHE_PREFETCH1 (1<<4)
/** \brief internal options mask flag used the reset circular log buffers **/
#define SPHSPCQUEUE_CIRCULAR_RESETMASK (SPHSPCQUEUE_CIRCULAR | \
		SPHSPCQUEUE_CACHE_PREFETCH0 | \
		SPHSPCQUEUE_CACHE_PREFETCH1)

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
*	@return a handle to the initialized SPHSinglePCQueue_t.
*/
extern __C__ SPHSinglePCQueue_t
SPHSinglePCQueueInit (void *buf_seg , block_size_t buf_size);

/** \brief Initialize a shared storage block as a Lock Free
*   Single Producer Single Consumer Queue
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
*	@return a handle to the initialized SPHSinglePCQueue_t.
*/
extern __C__ SPHSinglePCQueue_t
SPHSinglePCQueueInitWithStride (void* buf_seg,  block_size_t buf_size,
                           unsigned short entry_stride,
                           unsigned int options);

/** \brief Allocate and initialize a shared storage block as a Lock Free
*	Single Producer Single Consumer Queue.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a PC Queue.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SPHSinglePCQueue_t.
*/
extern __C__ SPHSinglePCQueue_t
SPHSinglePCQueueCreate (block_size_t buf_size);

/** \brief Allocate and initialize a shared storage block as a Lock Free
*	Single Producer Single Consumer Queue.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a Logger.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@param stride the stride offset is bytes between allocated entries.
*	@return a handle to the initialized SPHSinglePCQueue_t.
*/
extern __C__ SPHSinglePCQueue_t
SPHSinglePCQueueCreateWithStride (block_size_t buf_size,
                           unsigned short stride);

/** \brief Allows the Producer thread to return the address of a
*   (raw) queue entry allocated from the specified
*   Single Producer Single Consumer Queue.
*
*	The allocation size is rounded up to the next quadword
*	boundary. Mostly for internal use and testing.
*	If the specified queue is full the allocation may fail.
*
*	\warning This function is primarily for internal testing and should
*	not be used by applications.
*
*	@param log Handle to a Logger.
*	@return the address of the raw queue Entry is returned if successful,
*	or NULL if unsuccessful.
*	For example the Allocate may fail if the queue is full.
*/
extern __C__ void *
SPHSinglePCQueueAllocRaw (SPHSinglePCQueue_t log);

/** \brief Allows the producer thread to allocate and initialize the
*   header of a queue entry for access.	The allocation is from the
*   specified Single Producer Single Consumer Queue.
*
*	The allocation size is the stride set when the PC queue was
*	initialized/created.
*	The Entry status, Category, Subcategory, and length
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlfentry.h API.
*	If the specified queue is full the allocation may fail.
*
*	\note The queue entry is not ready for access by the Consumer
*	thread, until additional application data is inserted and the
*	entry is completed (via SPHSinglePCQueueEntryComplete).
*
*	@param queue Handle of a producer consumer queue.
*	@param catcode Category code to the new entry.
*	@param subcode Subcategory code to the new entry.
*	@param handlespace Address of local area that will be initialized
*	as a SPHLFEntryHandle_t for the allocated entry.
*	@return Handle of the initialized queue entry handle,
*	from the handlespace parm, or 0 (NULL) if the allocation failed.
*	For example the Allocate may fail if the queue is full.
*/
extern __C__ SPHLFEntryHandle_t *
SPHSinglePCQueueAllocStrideEntry (SPHSinglePCQueue_t queue,
                                  int catcode, int subcode,
                                  SPHLFEntryHandle_t *handlespace);

/** \brief Allows the producer thread to allocate and initialize the
*   header, of a queue entry for access. The allocation is from the
*   specified Single Producer Single Consumer Queue.
*
*	The allocation size is the stride set when the PC queue was
*	initialized/created.
*	The Category, Subcategory, PID, TID and high precision timestamp
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlflogentry.h API.
*	If the specified queue is full the allocation may fail.
*
*	\note The queue entry is not ready for access by the Consumer
*	thread, until additional application data is inserted and the
*	entry is completed (via SPHSinglePCQueueEntryComplete).
*
*	@param queue Handle of a producer consumer queue.
*	@param catcode Category code to the new entry.
*	@param subcode subcategory code to the new entry.
*	@param handlespace Address of local area that will be initialized as a
*	SPHLFEntryHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry,
*	from the handlespace parm, or 0 (NULL) if the allocation failed.
*	For example the Allocate may fail if the queue
*	is full.
*/
extern __C__ SPHLFEntryHandle_t *
SPHSinglePCQueueAllocStrideTimeStamped (SPHSinglePCQueue_t queue,
                                   int catcode, int subcode,
                                   SPHLFEntryHandle_t *handlespace);

/** \brief Allows the producer thread to mark the queue entry,
*   specified by the entry handle, as complete. This makes the
*   queue entry accessible to the consumer thread.
*
*	Also executes any memory barriers required by the platform to ensure
*	that all previous stores to this entry by this thread are visible
*	to other threads.
*
*	\warning The Producer thread should not touch or modify a queue entry
*	after calling EntryComplete.
*	This is important to both correctness and performance.
*
*	@param entryhandle log entry Handle for an allocated entry.
*	@return 0 if successful.
*/
extern __C__ int
SPHSinglePCQueueEntryComplete (SPHLFEntryHandle_t *entryhandle);

/** \brief Return the status of the entry specified by the entry handle.
*
*	@param entryhandle log entry Handle for an allocated entry.
*	@return true if the entry was complete (SPHSinglePCQueueEntryComplete
*	has been called for this entry). Otherwise False.
*/
extern __C__ int
SPHSinglePCQueueEntryIsComplete (SPHLFEntryHandle_t *entryhandle);

/** \brief Allows the consumer to get the next completed queue entry
*	from the specified single producer single consumer queue.
*
*	Returns an entry handle which allows the application to access the
*	application specific data inserted by the produced thread.
*	If the specified queue is empty or the next queue is not yet
*	completed the get may fail.
*
*	@param queue Handle of a producer consumer queue.
*	@param handlespace Address of local area that will be initialized as a
*	SPHLFEntryHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry,
*	from the handlespace parm, or 0 (NULL) if the get failed.
*	For example the Get may fail if the queue
*	is empty or the next tail entry is not yet completed.
*/
extern __C__ SPHLFEntryHandle_t*
SPHSinglePCQueueGetNextComplete (SPHSinglePCQueue_t queue,
                                 SPHLFEntryHandle_t *handlespace);

/** \brief Allows the consumer to free the queue entry it just processed
*   (using SPHSinglePCQueueGetNextComplete),
*	from the specified single producer single consumer queue.
*
*	Mark the current queue tail entry as free (unallocated and invalid)
*	and bump the queue tail pointer to the next entry.
*	If the specified queue is empty or the current tail entry is not yet
*	completed the Free may fail.
*
*	\warning The Consumer thread should not touch or modify a queue entry
*	after calling FreeEntry.
*	This is important to both correctness and performance.
*
*	@param queue Handle of a producer consumer queue.
*	@return True for successful tail free, otherwise indicated failure.
*	For example the Get may fail if the queue
*	is empty or the next tail entry is not yet completed.
*/
extern __C__ int
SPHSinglePCQueueFreeNextEntry (SPHSinglePCQueue_t queue);

/** \brief Return the status of the specified queue.
*
*	@param queue Handle to a queue.
*	@return true if the queue is currently Empty (no entries).
*	Otherwise False.
*/
extern __C__ int
SPHSinglePCQueueEmpty (SPHSinglePCQueue_t queue);

/** \brief Returns the amount of free space (in bytes) remaining in the specified queue.
*
*	@param queue Handle to a queue.
*	@return number of bytes of free space remaining in the queue buffer.
*/
extern __C__ block_size_t
SPHSinglePCQueueFreeSpace (SPHSinglePCQueue_t queue);

/** \brief Return the status of the specified queue.
*
*	@param queue Handle to a queue.
*	@return true if the queue is currently full.
*	Otherwise False.
*/
extern __C__ int
SPHSinglePCQueueFull (SPHSinglePCQueue_t queue);


/** \brief Resets the specific queue to empty state asynchronously (without locking or atomic updates).
*
*	Internal use for testing.
*
*	@param queue Handle to a queue.
*	@return 0 if successful.
*/
extern __C__ int
SPHSinglePCQueueResetAsync (SPHSinglePCQueue_t queue);

/** \brief Prefetch pages from the specific queue.
*
*	@param queue Handle to a queue.
*	@return 0 if successful.
*/
extern __C__ int
SPHSinglePCQueuePrefetch (SPHSinglePCQueue_t queue);

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
SPHSinglePCQueueSetCachePrefetch (SPHSinglePCQueue_t queue, int prefetch);

/** \brief Destroys the queue and frees the SAS storage for reuse.
*
*	@param queue Handle to a queue to be destroyed.
*	@return 0 if successful.
*/
extern __C__ int 
SPHSinglePCQueueDestroy (SPHSinglePCQueue_t queue);

#endif /* __SPH_SINGLE_PC_QUEUE_H */
