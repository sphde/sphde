/*
 * Copyright (c) 2010-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SPH_LOCK_FREE_LOGGER_H
#define __SPH_LOCK_FREE_LOGGER_H

/*! \file sphlflogger.h
*  \brief Shared Persistent Heap, logger.
*	For shared memory multi-thread/multi-core applications.
*	This implementation uses atomic operations to implement Lock
*	Free Loggers (SPHLFLogger_t).
*
*	This Logger API supports atomic allocation of storage for event
*	entries for zero copy persistence and sharing.
*	Only the allocation of the entry is serialized.
*	If there are multiple threads or processes generating log entries,
*	on a multi-core processor, hardware threads can fill in Log entries
*	in parallel with other threads.
*
*	The API support simple loggers and circular loggers.
*	Simple loggers stop allocating entries when the log buffer is full.
*	Circular loggers wrap to the beginning of the log buffer when the
*	log buffer fills. This overwrites the oldest log entries. but allows
*	logging on a continuous basis. Circular loggers use a fixed entry
*	stride to insure the oldest entry can be found easily. A stride that
*	matches the cache line size can improve performance by avoiding
*	"false sharing" of cache lines containing multiple logger entries
*	across cores/sockets.
*
*	For example:
*	\code
*   SPHLFLogger_t logger;
*   SPHContext_t context;
*
*   SASJoinRegion();
*   // set up shared circular logger with fixed size entries
*   // Stride is a full cache line for better performance
*   logger = SPHLFCircularLoggerCreate (log_alloc, 128);
*   if ( logger )
*   {
*       context = SPHSetupProjectContext ("myproject");
*       if (context)
*       {
*           SPHContextAddName (context, "flight_recorder", logger)
*       }
*   }
*   \endcode
*
*	As an option the logger allocator will fill in a 16 byte
*	entry header with: \n Entry status and length.  \n Entry identifying
*	Category and SubCategory codes. \n Process and Thread Ids.  \n High
*	resolution timestamp.
*
*   \code
*   SPHLFLogger_t logger;
*   SPHContext_t context;
*   SPHLFLoggerHandle_t *handlex, handle_data;
*
*   SASJoinRegion();
*   // get up shared circular logger
*   context getProjectContextByName ("myproject");
*   if (context)
*   {
*      logger = (SPHLFLogger_t)SPHContextFindByName (context, "flight_recorder");
*    	...
*      // create a flight_recorder entry with application category/subcategory codes
*      // with process/thread IDs and timestamp.
*      if (logger)
*      {
*    	 handex = SPHLFLoggerAllocStrideTimeStamped (logger, category_x, subcat_y, handle_data);
*    	 ...
*      }
*   }
*   \endcode
*
*	Any additional storage allocated to the entry
*	is available for application specific data.
*	The sphlflogentry.h API also provides
*	several mechanisms to store application data including; direct
*	array or structure overlay, and a streams like mechanism.
*	The API provides a completion function (SPHLFLoggerEntryComplete)
*	which provides any memory barriers required by the platform and
*	marks the entry complete.
*
*   \code
*       ...
*       // create a flight_recorder entry with application cat/subcat codes
*       // rumtime provides process/thread IDs and time_stamp.
*
*       handex = SPHLFLoggerAllocStrideTimeStamped (logger, category_x, subcat_y, handle_data);
*       if (handex)
*       {
*           // insert a stream of application specific data
*           SPHLFLogEntryAddInt(handlex, int_data1);
*           SPHLFLogEntryAddInt(handlex, int_data2);
*           SPHLFLogEntryAddPtr(handlex, ptr_1);
*           SPHLFLogEntryAddDouble(handlex, double_data1);
*           SPHLFLogEntryAddString(handlex, "any_c_string");
*    		...
*    	    // Insure the entry is coherent and mark this entry complete
*    	    SPHLFLogEntryComplete (handlex);
*       }
*   \endcode
*
*   The logger contents are shared and persistent and will not be lost if the
*   logging process crashes. Logger entries can be retrieved at any time
*   after it is marked complete.
*
*	The API provides Log Iterator functions that support sequential read-back
*	of (completed) Logger entries. First create a log iterator
*	(with SPHLFloggerCreateIterator). Then use the resulting iterator to
*	sequentially step through log entries (via SPHLFLoggerItertorNext).
*	The iterator runtime handles both simple and circular loggers and insures
*	entries are read-out (oldest to newest) in order.
*
*   \code
*   SPHLFLogIterator_t *iter, iter0;
*   SPHLFLoggerHandle_t *handlex, handle_data;
*   ...
*   // create a Log iterator over the flight recorder
*   iter = SPHLFLoggerCreateIterator(logger, &iter0);
*   if (iter)
*   {
*       handlex = SPHLFLoggerIteratorNext (iter, &handle_data);
*       while (handex)
*       {
*           sphtimer_t entry_timestamp;
*           int cat, subcat, PID, TID:
*           int int_data1, int_data1;
*           void * ptr_1;
*           double double_data1;
*           char * entry_str;
*
*           // extract entry data
*           PID = SPHLFLogEntryPID (handlex);
*           TID = SPHLFLogEntryTID (handlex);
*           cat = SPHLFLogEntryCategory (handlex);
*           subcat = SPHLFLogEntrySubcat (handlex);
*           entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
*           ...
*           // extract application specific data, in insertion order
*           int_data1 = SPHLFLogEntryGetNextInt(handlex);
*           int_data2 = SPHLFLogEntryGetNextInt(handlex);
*           ptr_1 = SPHLFLogEntryGetNextPtr(handlex);
*           double_data1 = SPHLFLogEntryGetNextDouble(handlex);
*           entry_str SPHLFLogEntryGetNextString(handlex);
*		    ...
*           // do something with the date
*           ...
*           // get next entry
*           handlex = SPHLFLoggerIteratorNext (iter, &handle_data);
*       }
*   }
*   \endcode
*/

#include "sastype.h"
#include "sphtimer.h"


/*! \brief Handle to an instance of SPH Lock Free Logger.
*
*	The type is SAS_RUNTIME_LOCKFREELOGGER
*/
typedef void *SPHLFLogger_t; 

/** \brief ignore this macro behined the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*! \brief unsigned int type, consistent with the size of a pointer and used for pointer calculations **/
typedef unsigned long longPtr_t;

/*! \brief sphLogEntry_t.
*	Fields defining the entry details word.
*/
typedef struct {
	/** Boolean: Indicates that this entry is valid (complete). **/
	unsigned int 	valid : 1;
	/** Boolean: Indicates that this entry is time stamped. **/
	unsigned int 	timestamped : 1;
	/** reserved **/
	unsigned int 	__reserved : 2;
	/** Application defined Event category code **/
	unsigned int 	category : 12;
	/** Application defined Event sub-category code **/
	unsigned int 	subcat : 8;
	/** Length of this entry **/
	unsigned int 	len : 8;
	} sphLogEntryLayout_t;

/*! \brief Aggregate type for handling sphLogEntryLayout_t. **/
typedef unsigned int sphLogEntryID_t;

/*! \brief Union of Entry details with 32-bit word for atomic update.
*/
typedef union {
	/** Logger Entry header as a Unit **/
	sphLogEntryID_t		idUnit;
	/** Logger Entry header in detail **/
	sphLogEntryLayout_t	detail;
	} sphLogEntry_t;


/*! \brief Common type for PID/TID values stored in Logger entries.  **/
typedef unsigned short sphpid16_t;

/*! \brief Instance of a Lock Free Logger Entry Header.
*
*   Contains fields defining the standard timestamped entry header.
*/
typedef struct {
		/** Entry header detailes **/
        sphLogEntry_t   entryID;
        /** Process ID **/
        sphpid16_t      PID;
        /** Thread ID **/
        sphpid16_t      TID;
        /** High resolution 64-bit Time Stamp **/
        sphtimer_t      timeStamp;
        } SPHLFLogHeader_t;


/*! \brief Instance of a Lock Free Logger Entry Handle.
*
*   Contains fields required to: locate the entry, record the total
*   space allocated to the entry, and manage the next location within
*   the entry and remaining storage.
*
*	Entry Handles should be allocated in private (local stack) storage
*	to allow concurrent access to independent entries from multiple threads.
*/
typedef struct {
    /** address of the start of the event entry **/
	SPHLFLogHeader_t	*entry;
	/** address of the next avail byte(s) of storage **/
	char			*next;
	/** Total allocated size of the event entry **/
	unsigned short int	total_size;
	/** Remaining bytes of free storage in the entry **/
	unsigned short int	remaining;
	} SPHLFLoggerHandle_t;

/*! \brief Instance of a Lock Free Logger Iterator.
*
*	Contains fields required to access entries in the
*	containing logger and track the current entries and detect end
*	conditions (last complete entry in the logger).
*
*	Iterators should be allocated in private (local stack) storage to allow
*	concurrent access from multiple threads.
*/
typedef struct {
    /** address of the Logger this Iterator scanning **/
	SPHLFLogger_t		logger;
    /** address of the current Logger entry for this Iterator **/
	longPtr_t		current;
    /** address of the next free Logger entry **/
	longPtr_t		free;
    /** address of the start of the Logger space **/
	longPtr_t		start_log;
    /** address of the end of the Logger space **/
	longPtr_t		end_log;
    /** Option flags and status **/
	unsigned int		options;
	} SPHLFLogIterator_t;

/*! \brief internal options flag for circular log buffers **/
#define SPHLFLOGGER_CIRCULAR (1)
/*! \brief internal options flag set when circular log buffers have wrapped **/
#define SPHLFLOGGER_CIRCULAR_WRAPED (1<<1)
/*! \brief internal options flag set when circular log buffers have wrapped multiple times **/
#define SPHLFLOGGER_CIRCULAR_NOTFIRST (1<<2)
/*! \brief internal options flag for prefetching the immediate (0 offset) cache-line **/
#define SPHLFLOGGER_CACHE_PREFETCH0 (1<<3)
/*! \brief internal options flag for prefetching the next (line size offset) cache-line **/
#define SPHLFLOGGER_CACHE_PREFETCH1 (1<<4)
/*! \brief internal options mask flag used the reset circular log buffers **/
#define SPHLFLOGGER_CIRCULAR_RESETMASK (SPHLFLOGGER_CIRCULAR | \
		SPHLFLOGGER_CACHE_PREFETCH0 | \
		SPHLFLOGGER_CACHE_PREFETCH1)

/*! \brief Initialize a shared storage block as a Lock Free Event Logger.
*
*	Initialize the specified storage block as Lock Free Logger
*	control blocks.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_LOCKFREELOGGER.
*
*	@param buf_seg a block of allocated SAS storage matching the buf_size.
*	@param buf_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLFLogger_t 
SPHLFLoggerInit (void *buf_seg , block_size_t buf_size);

/*! \brief Initialize a shared storage block as a Lock Free Event Logger
*	with a fixed entry stride.
*
*	Initialize the specified storage block as Lock Free Logger
*	control blocks.
*	The stride and control flags are also stored.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_LOCKFREELOGGER.
*
*	@param buf_seg a block of allocated SAS storage matching the buf_size.
*	@param buf_size power of two size of the heap to be initialized.
*	@param entry_stride the stride offset is bytes between allocated entries.
*	@param options option bits.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLFLogger_t 
SPHLFLoggerInitWithStride (void* buf_seg,  block_size_t buf_size,
                           unsigned short entry_stride,
                           unsigned int options);

/*! \brief Allocate and initialize a shared storage block as a Lock Free
*	Event Logger.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a Logger.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLFLogger_t 
SPHLFLoggerCreate (block_size_t buf_size);

/*! \brief Allocate and initialize a shared storage block as a Lock Free
*	Event Logger. Mark the logger as circular and specify a fixed
*	Stride.
*
*	Allocate a block from SAS storage and initialize that block
*	block as a Logger.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the heap to be initialized.
*	@param stride the stride offset is bytes between allocated entries.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLFLogger_t
SPHLFCircularLoggerCreate (block_size_t buf_size,
                           unsigned short stride);

/*! \brief Return the address of a (raw) Logger entry allocated from
*   the specified logger.
*
*	The allocation size is rounded up to the next quadword
*	boundary. Mostly for internal use and testing.
*	If the specified logger is full the allocation may fail.
*
*	@param log Handle to a Logger.
*	@param alloc_size size in bytes of the entry to be allocated.
*	@return the address of the raw Log Entry is returned if successful,
*	or NULL if unsuccessful.
*	For example the Allocate may fail if the logger is full.
*/
extern __C__ void *
SPHLFLoggerAllocRaw (SPHLFLogger_t log, 
                     block_size_t alloc_size);

/*! \brief Allocate and initialize the header, of a timestamped logger entry,
*	from the specified logger.
*
*	The allocation size is rounded up to the next quadword
*	boundary and does not include the implicit size of the Timestamp
*	entry header.
*	The Category, Subcategory, PID, TID and high persision timestamp
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlflogentry.h API.
*	If the specified logger is full the allocation may fail.
*
*	@param log Handle to a Logger.
*	@param catcode Category code to the new entry.
*	@param subcode subcategory code to the new entry.
*	@param alloc_size Size in bytes of the entry to be allocated.
*	The actual entry will be +16 bytes to include the entry header.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry handle,
*	from the handlespace parm, or 0 (NULL) if the allocatation failed.
*	For example the Allocate may fail if the logger is full.
*/
extern __C__ SPHLFLoggerHandle_t *
SPHLFLoggerAllocTimeStamped (SPHLFLogger_t log,
                             int catcode, int subcode,
                             block_size_t alloc_size,
                             SPHLFLoggerHandle_t *handlespace);

/*! \brief Allocate and initialize the header, of a timestamped logger entry,
*	from the specified logger.
*
*	The allocation size fixed as specified by the SPHLFCircularLoggerCreate.
*	The Category, Subcategory, PID, TID and high persision timestamp
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlflogentry.h API.
*	If the specified logger is full the allocation may fail.
*
*	@param log Handle to a Logger.
*	@param catcode Category code to the new entry.
*	@param subcode subcategory code to the new entry.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry,
*	from the handlespace parm, or 0 (NULL) if the allocatation failed.
*	For example the Allocate may fail if the logger
*	is full and is not mark circular.
*/
extern __C__ SPHLFLoggerHandle_t *
SPHLFLoggerAllocStrideTimeStamped (SPHLFLogger_t log,
                                   int catcode, int subcode,
                                   SPHLFLoggerHandle_t *handlespace);

/*! \brief Allocate and initialize the header of a timestamped logger entry
*	from the specified logger, without locking or memory barriers.
*
*	The allocation size is rounded up to the next quadword
*	boundary and does not include the implicit size of the Timestamp
*	entry header.
*	The Category, Subcategory, PID, TID and high persision timestamp
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlflogentry.h API.
*	If the specified logger is full the allocation may fail.
*
*	\warning The NoLock form is only appropriate for single thread
*	applications where the logger is not shared with other processes.
*	Or if used in a multi-threaded application each thread should have
*	it's own private logger instance.
*
*	\note Also recommended that the NoLock allocates should be completed with
*	SPHLFLogEntryWeakComplete() from sphlflogentry.h.
*
*	@param log Handle to a Logger.
*	@param catcode Category code to the new entry.
*	@param subcode subcategory code to the new entry.
*	@param alloc_size Size in bytes of the entry to be allocated.
*	The actual entry will be +16 bytes to include the entry header.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry handle,
*	from the handlespace parm, or 0 (NULL) if the allocatation failed.
*	For example the Allocate may fail if the logger is full.
*/
extern __C__ SPHLFLoggerHandle_t *
SPHLFLoggerAllocTimeStampedNoLock (SPHLFLogger_t log,
                             int catcode, int subcode,
                             block_size_t alloc_size,
                             SPHLFLoggerHandle_t *handlespace);

/*! \brief Allocate and initialize the header, of a timestamped logger entry,
*	from the specified logger, without locking or memory barriers.
*
*	The allocation size fixed as specified by the SPHLFCircularLoggerCreate.
*	The Category, Subcategory, PID, TID and high persision timestamp
*	are stored in the header of the new entry.
*	Returns an entry handle which allows the application to insert
*	application specific data into the entry via the sphlflogentry.h API.
*	If the specified logger is full the allocation may fail.
*
*	\warning The NoLock form is only appropriate for single thread
*	applications where the logger is not shared with other processes.
*	Or if used in a multi-threaded application each thread should have
*	it's own private logger instance.
*
*	\note Also recommended that the NoLock allocates should be completed with
*	SPHLFLogEntryWeakComplete() from sphlflogentry.h.
*
*	@param log Handle to a Logger.
*	@param catcode Category code to the new entry.
*	@param subcode subcategory code to the new entry.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return Handle of the initialized logger entry,
*	from the handlespace parm, or 0 (NULL) if the allocatation failed.
*	For example the Allocate may fail if the logger
*	is full and is not mark circular.
*/
extern __C__ SPHLFLoggerHandle_t *
SPHLFLoggerAllocStrideTimeStampedNoLock (SPHLFLogger_t log,
                                   int catcode, int subcode,
                                   SPHLFLoggerHandle_t *handlespace);

/*! \brief Marks the entry specified by the entry handle as complete.
*
*	Also executes any memory barriers required by the platform to ensure
*	that all previous stores to this entry by this thread are visible
*	to other threads.
*
*	@param entryhandle log entry Handle for an allocated entry.
*	@return 0 if successful.
*/
extern __C__ int
SPHLFLoggerEntryComplete (SPHLFLoggerHandle_t *entryhandle);

/*! \brief Return the status of the entry specified by the entry handle.
*
*	@param entryhandle log entry Handle for an allocated entry.
*	@return true if the entry was complete (SPHLFLoggerEntryComplete
*	has been called fo this entry). Otherwise False.
*/
extern __C__ int
SPHLFLoggerEntryIsComplete (SPHLFLoggerHandle_t *entryhandle);

/*! \brief Return the status of the entry specified by the entry handle.
*
*	@param entryhandle log entry Handle for an allocated entry.
*	@return true if the entry was timestamped, otherwise False.
*/
extern __C__ int
SPHLFLoggerEntryIsTimestamped (SPHLFLoggerHandle_t *entryhandle);

/*! \brief For the provided Logger Iterator creates a entry handle for the
*	next sequential Logger entry.
*	The resulting entry handle can be used to read the contents of
*	the Logger entry.
*
*	@param iterator Handle associated with a Logger.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return a pointer to the handle space provided, initialized as a
*	Logger entry handle for the next sequential entry if successful.
*	Otherwise NULL.
*/
extern __C__ SPHLFLoggerHandle_t *
SPHLFLoggerIteratorNext (SPHLFLogIterator_t *iterator,
                         SPHLFLoggerHandle_t *handlespace);

/*! \brief Creates a logger Iterator for reading sequential entries from a specific logger.
*
*	@param log Handle to a Logger.
*	@param iteratorSpace Address of local area that will be initialied as a
*	SPHLFLogIterator_t associated with the log.
*	@return a pointer to the iterator space provided,
*	initialized as a Logger Iterator if successful.
*	Otherwise NULL.
*/
extern __C__ SPHLFLogIterator_t *
SPHLFLoggerCreateIterator(SPHLFLogger_t log,
                          SPHLFLogIterator_t *iteratorSpace);

/*! \brief Return the status of the specified logger.
*
*	@param log Handle to a Logger.
*	@return true if the logger is currently Empty (no entries).
*	Otherwise False.
*/
extern __C__ int
SPHLFLoggerEmpty (SPHLFLogger_t log);

/*! \brief Return the status of the specified logger.
*
*	@param log Handle to a Logger.
*	@return true if the logger is circular and has wrapped (at least once).
*	Otherwise False.
*/
extern __C__ int
SPHLFLoggerWrapped (SPHLFLogger_t log);

/*! \brief Returns the amount of free space (in bytes) remaining in the specified logger.
*
*	@param log Handle to a Logger.
*	@return number of bytes of free space remainign in the Logger buffer.
*/
extern __C__ block_size_t
SPHLFLoggerFreeSpace (SPHLFLogger_t log);

/*! \brief Return the status of the specified logger.
*
*	@param log Handle to a Logger.
*	@return true if the logger is currently full.
*	Otherwise False.
*/
extern __C__ int
SPHLFLoggerFull (SPHLFLogger_t log);

/*! \brief Resets the specific logger to empty state synchronously if it is currently full.
*
*	Internal use for testing.
*
*	@param log Handle to a Logger.
*	@return 0 if the logger was full and this thread did the reset.
*	Otherwise 1 if not full or another thread already reset this logger.
*	Other values indicated a serious error, for example the reference
*	is not a logger.
*/
extern __C__ int
SPHLFLoggerResetIfFullSync (SPHLFLogger_t log);

/*! \brief Resets the specific logger to empty state asynchronously (without locking or atomic updates).
*
*	Internal use for testing.
*
*	@param log Handle to a Logger.
*	@return 0 if successful.
*/
extern __C__ int
SPHLFLoggerResetAsync (SPHLFLogger_t log);

/*! \brief Prefetch pages from the specific logger.
*
*	@param log Handle to a Logger.
*	@return 0 if successful.
*/
extern __C__ int
SPHLFLoggerPrefetch (SPHLFLogger_t log);

/*! \brief Set the cache-line prefetch options for entry allocate.
*
*   prefetch == 0; No prefetch issued.
*   \n prefetch == 1; Prefetch the currently allocated cache-line.
*   \n prefetch == 2; Prefetch the cache-line following the allocated entry.
*   \n prefetch == 3; Prefetch the current and next cache-lines.
*
*	@param log Handle to a Logger.
*	@param prefetch prefetch option code.
*	@return 0 if successful.
*/
extern __C__ int
SPHLFLoggerSetCachePrefetch (SPHLFLogger_t log, int prefetch);

/*! \brief Destroys the logger and frees the SAS storage for reuse.
*
*	@param log Handle to a Logger to be destroyed.
*	@return 0 if successful.
*/
extern __C__ int 
SPHLFLoggerDestroy (SPHLFLogger_t log);

#endif /* __SPH_LOCK_FREE_LOGGER_H */
