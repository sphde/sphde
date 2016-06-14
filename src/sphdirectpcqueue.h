/*
 * Copyright (c) 2015 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */
#ifndef SRC_SPHDIRECTPCQUEUE_H_
#define SRC_SPHDIRECTPCQUEUE_H_
#include <stdint.h>

/** \file sphdirectpcqueue.h
*  \brief Shared Persistent Heap, single producer single consumer queue direct API.
*
*
*       For shared memory multi-thread/multi-core applications.
*       This implementation is based on the Lock
*       Free Producer/Consumer Queue (SPHSinglePCQueue_t) but
*       simplifies access to the Entry for lower latency.
*
*       This API supports atomic allocation of storage for queue
*       entries for zero copy persistence and sharing. Zero copy queues
*       divides the process of producing a queue entry in to three steps:
*       - Allocate the queue entry (and initialize the header)
*       - Use the return entry handle to fill in application specific data.
*       - Marks the entry complete in the header.
*
*       \code
#include <sphdirectpcqueue.h>

  sphLFEntryID_t entry_tmp;

  // only need to do this once per pcqueue and so should be
  // outside of the primary producer message loop.
  entry_tmp = SPHSinglePCQueueGetEntryTemplate(pqueue);

  SPHLFEntryDirect_t handle;

  int *array;

  handle = SPHSinglePCQueueAllocStrideDirectSpin (pqueue);
  if (handle)
    {
      array = (int *) SPHLFEntryDirectGetFreePtr (handle);
      array[0] = val1;
      array[1] = val2;
      array[2] = val3;
      SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2);
    }
  else
    {
      // error handling
    }
*       \endcode
*
*       The consumer can access queue entries once they are marked complete.
*       The consumer:
*       - checks (spins) for the next allocated entry to become complete.
*       - uses the returned entry handle to directly access the entry contents.
*       - When done processing the queue entry, it marks the entry header
*       invalid and deallocates the entry.
*       - This makes the next queue entry available, if any.
*
*       \code
#include <sphdirectpcqueue.h>

  int *array;
  int data1, data2, data3;

  handle = SPHSinglePCQueueGetNextCompleteDirectSpin (cqueue);
  if (handle)
    {
      array = (int *) SPHLFEntryDirectGetFreePtr (handle);
      data1 = array[0];
      data2 = array[1];
      data3 = array[2];

      if (SPHSinglePCQueueFreeNextEntryDirect (cqueue, handle))
        {
          // complete handling of message
        }
      else
        { // error handling
          printf ("SPHSinglePCQueueFreeNextEntry() = failed\n");
        }
    }
  else
    { // error handling
      printf ("SPHSinglePCQueueGetNextCompleteDirectSpin() = failed\n");
    }

*       \endcode
*
*       In this implementation the allocation of the entry is minimally
*       serialized based on the assumption that only one (producer) thread
*       will be allocating queue entries.
*       Likewise the assumption is that there is only one consumer thread
*       per SPHSinglePCQueue_t instance.
*       This allows independent producer/consumer thread pairs to interact
*       with a queue instance with minimum synchronization and overhead.
*
*       As an option the queue entry allocator will fill in a 4 byte
*       entry header with:
*       - Entry status and length.
*       - Entry identifying Category and SubCategory codes.
*
*       Any additional storage allocated to the entry (after the header)
*       is available for application specific data.  This API also provides
*       a direct pointer mechanism to store application data.
*       The API provides a completion function (SPHSinglePCQueueEntryComplete)
*       which provides any memory barriers required by the platform and
*       marks the entry complete.
*
*       The API support simple circular queues and requires a constant
*       entry stride. A stride that matches or is multiple of the
*       cache line size can improve performance by avoiding
*       "false sharing" of cache lines containing multiple queue entries
*       across cores/sockets.
*
*/
#include "sastype.h"
#include "sasatom.h"
#include "sphlfentry.h"
#include "sphsinglepcqueue.h"


/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Instance of a Lock Free event direct data Handle.
*
*   Contains fields required to: locate the entry, record the total
*   space allocated to the entry, and manage the next location within
*   the entry and remaining storage.
*
*       Entry Handles should be allocated in private (local stack) storage
*       to allow concurrent access to independent entries from multiple threads.
*/
typedef void* SPHLFEntryDirect_t;

/** \brief Marks the entry specified by the entry handle as complete.
*   Also executes any memory barriers required by the platform to ensure
*   that all previous stores by this thread to this entry are complete.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @param entry_template from SPHSinglePCQueueGetEntryTemplate().
*   @param catcode Category code to the completed entry.
*   @param subcode Subcategory code to the completed entry.
*   @return a 1 value indicates success.
*/
static inline int
SPHLFEntryDirectComplete (SPHLFEntryDirect_t directHandle,
                          sphLFEntryID_t entry_template,
                          int catcode, int subcode)
{
    int rc = 1;
    SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;
    sphLFEntry_t        entrytemp;

    sas_read_barrier();
    /* the template should have allocated and len already set.  */
    entrytemp.idUnit = entry_template;
    entrytemp.detail.valid = 1;
    entrytemp.detail.category = catcode;
    entrytemp.detail.subcat = subcode;

    entryPtr->entryID.idUnit = entrytemp.idUnit;

    return (rc);
}

/** \brief Return the first free byte address for the direct entry
 *  specified by the direct entry handle. This is normally the byte
 *  after the sphLFEntry_t.
*
*   \warning This function should be used carefully. It is may not
*   provide the correct alignment for the data that follow and does not
*   manage the space within the direct entry, if multiple application
*   functions may update the same entry.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @return address the entries free space.
*/
static inline void*
SPHLFEntryDirectGetFreePtr (SPHLFEntryDirect_t directHandle)
{
        char    *ptr    = (char*)directHandle + sizeof (sphLFEntry_t);

        return ((void*)ptr);
}

/** \brief Return the first free byte address, with required alignment,
 *  within the direct entry specified by the direct entry handle.
 *  This is normally the address after the sphLFEntry_t plus alignment
 *  padding.
*
*   \warning This function does not manage the space within the direct
*   entry, this may be an issue if multiple application functions
*   update the same entry.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @param alignval required alignment of the next value to be added.
*   @return address the entries free space.
*/
static inline void*
SPHLFEntryDirectGetPtrAligned (SPHLFEntryDirect_t directHandle,
		size_t alignval)
{
        uintptr_t	ptr		= (uintptr_t)directHandle;
    	uintptr_t	adjust	= alignval - 1;
    	uintptr_t	mask	= ~adjust;

    	ptr = ((ptr + sizeof (sphLFEntry_t) + adjust) & mask);

        return ((void*)ptr);
}

/** \brief Return the next free byte address within direct entry
 *  specified by a current address within that direct entry.
*
*   \warning This function does not manage the space within the direct
*   entry, this may be an issue if multiple application functions
*   update the same entry.
*
*   @param directptr current data address within Entry.
*   @param incval size of last entry address to stepped over.
*   @param alignval required alignment of the next value to be added.
*   @return next address within the entry with require alignment.
*/
static inline void*
SPHLFEntryDirectIncAndAlign (void *directptr, size_t incval, size_t alignval)
{
        uintptr_t	ptr		= (uintptr_t)directptr;
    	uintptr_t	adjust	= alignval - 1;
    	uintptr_t	mask	= ~adjust;

    	ptr = ((ptr + incval + adjust) & mask);
        return ((void*)ptr);
}

/** \brief Return the status of the entry specified by the direct entry
 *  handle.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @return true if the entry was complete (SPHLFLoggerEntryComplete
*           has been called fo this entry). Otherwise False.
*/
static inline int
SPHLFEntryDirectIsComplete (SPHLFEntryDirect_t directHandle)
{
    SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;

    return (entryPtr->entryID.detail.valid == 1);
}

/** \brief Return the status of the entry specified by the direct entry
 *  handle.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @return true if the entry was time stamped. Otherwise False.
*/
static inline int
SPHLFEntryDirectIsTimestamped (SPHLFEntryDirect_t directHandle)
{
    SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;

    return ((entryPtr->entryID.detail.valid == 1)
          &&(entryPtr->entryID.detail.timestamped == 1));
}

/** \brief Return the entry category for the entry specified by the
 *  direct entry handle.
*
*   @param directHandle Entry Handle for an allocated entry.
*   @return the category from the entry, if the entry was valid.
*   Otherwise return 0.
*/
static inline int
SPHLFEntryDirectCategory (SPHLFEntryDirect_t directHandle)
{
    SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;
    int result = -1;
#if 0
    printf ("SPHLFEntryCategory(%p) entry=%p id=%x\n",
                handlespace, entryPtr, entryPtr->entryID.idUnit);
#endif
    if (entryPtr->entryID.detail.valid == 1)
        result = entryPtr->entryID.detail.category;

    return (result);
}

/** \brief Return the entry sub-category for the entry specified by the
 *  direct entry handle.
*
*   @param directHandle Entry Handle for an allocated entry.
*       @return the sub-category from the entry, if the entry was valid.
*       Otherwise return 0.
*/
static inline int
SPHLFEntryDirectSubcat (SPHLFEntryDirect_t directHandle)
{
    SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;
    int result = -1;
#if 0
    printf ("SPHLFEntrySubcat(%p) entry=%p id=%x\n",
                handlespace, entryPtr, entryPtr->entryID.idUnit);
#endif
    if (entryPtr->entryID.detail.valid == 1)
        result = entryPtr->entryID.detail.subcat;

    return (result);
}

/** \brief Return the entry template for an existing Lock Free
*   Single Producer Single Consumer Queue.
*   This template is used later to mark an allocated entry complete.
*
*   @param queue Handle of a producer consumer queue.
*   @return the entry template for this queue or
*   0 if not a valid SPHSinglePCQueue_t.
*/
extern __C__ sphLFEntryID_t
SPHSinglePCQueueGetEntryTemplate (SPHSinglePCQueue_t queue);

/** \brief Allows the producer thread to allocate and initialize the
*   header of a queue entry for access. The allocation is from the
*   specified Single Producer Single Consumer Queue.
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
SPHSinglePCQueueAllocStrideDirect (SPHSinglePCQueue_t queue);

/** \brief Allows the producer thread to allocate and initialize the
*   header of a queue entry for access. The allocation is from the
*   specified Single Producer Single Consumer Queue. If space is not
*   Immediately available, spin until it is.
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
SPHSinglePCQueueAllocStrideDirectSpin (SPHSinglePCQueue_t queue);

/** \brief Allows the producer thread to allocate and initialize the
*   header of a queue entry for access. The allocation is from the
*   specified Single Producer Single Consumer Queue. If space is not
*   Immediately available, spin until it is.
*   While spinning use appropriate arch specific instructions to free
*   up core resources for other threads.
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
SPHSinglePCQueueAllocStrideDirectSpinPause (SPHSinglePCQueue_t queue);

/** \brief Allows the consumer to get the next completed queue entry
*       from the specified single producer single consumer queue.
*
*       Returns an direct entry handle which allows the application
*       to access the application specific data inserted by the
*       produced thread.
*       If the specified queue is empty or the next queue is not yet
*       completed, spin until data is ready.
*
*       @param queue Handle of a producer consumer queue.
*       @return Direct Handle of the initialized logger entry,
*       or 0 (NULL) if the get failed.
*       For example the Get id the queue is not actually a
*       SPHSinglePCQueue_t.
*/
extern __C__ SPHLFEntryDirect_t
SPHSinglePCQueueGetNextCompleteDirectSpin (SPHSinglePCQueue_t queue);

/** \brief Allows the consumer to get the next completed queue entry
*       from the specified single producer single consumer queue.
*
*       Returns an direct entry handle which allows the application
*       to access the application specific data inserted by the
*       produced thread.
*       If the specified queue is empty or the next queue is not yet
*       completed, spin until data is ready.
*       While spinning use appropriate arch specific instructions to
*       free up core resources for other threads.
*
*       @param queue Handle of a producer consumer queue.
*       @return Direct Handle of the initialized logger entry,
*       or 0 (NULL) if the get failed.
*       For example the Get id the queue is not actually a
*       SPHSinglePCQueue_t.
*/
extern __C__ SPHLFEntryDirect_t
SPHSinglePCQueueGetNextCompleteDirectSpinPause (SPHSinglePCQueue_t queue);

/** \brief Allows the consumer to get the next completed queue entry
*       from the specified single producer single consumer queue.
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
SPHSinglePCQueueGetNextCompleteDirect (SPHSinglePCQueue_t queue);

/** \brief Allows the consumer to free the queue entry it just processed
*   (using SPHSinglePCQueueGetNextComplete),
*       from the specified single producer single consumer queue.
*
*       Mark the current queue tail entry as free (unallocated and invalid)
*       and bump the queue tail pointer to the next entry.
*       If the specified queue is empty or the current tail entry is not yet
*       completed the Free may fail.
*
*       \warning The Consumer thread should not touch or modify a queue entry
*       after calling FreeEntry.
*       This is important to both correctness and performance.
*
*       @param queue Handle of a producer consumer queue,
*       @param next_entry Direct handle of the queue entry to free.
*       @return True for successful tail free, otherwise indicated failure.
*       For example the Free may fail if the queue
*       is empty or the next tail entry is not yet completed.
*/
extern __C__ int
SPHSinglePCQueueFreeNextEntryDirect (SPHSinglePCQueue_t queue,
                                     SPHLFEntryDirect_t next_entry);

/** \brief Allows the consumer to get the next allocated queue entry
*       from the specified single producer single consumer queue.
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
SPHSinglePCQueueGetNextEntryDirect (SPHSinglePCQueue_t queue);

/** \brief Return the status of the entry specified by the direct entry handle.
*
*       @param directHandle entry Handle for an allocated entry.
*       @return true if the entry was complete (SPHSinglePCQueueEntryComplete
*       has been called for this entry). Otherwise False.
*/
extern __C__ int
SPHSinglePCQueueEntryIsCompleteDirect (SPHLFEntryDirect_t directHandle);

#endif /* SRC_SPHDIRECTPCQUEUE_H_ */
