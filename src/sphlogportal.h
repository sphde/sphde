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

#ifndef __SPH_LOG_PORTAL_H
#define __SPH_LOG_PORTAL_H

/**! \file sphlogportal.h
*  \brief Shared Persistent Heap, log Portal.
*	For shared memory multi-thread/multi-core applications.
*	This implementation uses atomic operations to collect a set of Lock
*	Free Loggers (SPHLFLogger_t) and control the switching to a fresh
*	logger when it fills up, in a non-blocking but thread safe manner.
*
*	A Log Portal manages a set of Lock Free Loggers. This includes:
*	-# Managing a list of Logger spaces available for logging events.
*	  - Allocating the next free slot for adding a logger to the list.
*	  - Managing the current Logger within the list.
*	  - Control access to the Logger list elements.
*	-# Passing Logger event allocation requests through to the current
*	   Logger.
*	-# Advancing the Current logger to next free Logger when the current
*	   Logger fills up.
*
*	A portal is used when the required capacity for logger entries
*	exceeds that posible for a single logger. Or we need to support
*	rolling/continuous logs over a long period of time. By switching out
*	filled loggers with new/empty loggers, logging can continue indeffinitely.
*/

#include "sastype.h"
#include "sphlflogger.h"

/** \brief Handle to an instance of SPH Log Portal.
*
*	The type is SAS_RUNTIME_LOGPORTAL
*/
typedef void *SPHLogPortal_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Instance of a Log Portal Iterator.
*
*	Contains a Log Iterator plus additional fields required to access the
*	containing log portal and trace the current logger and detect end
*	conditions (last entry in the last active logger).
*
*	Iterators should be allocated in private (local stack) storage to allow
*	concurrent access from multiple threads.
*/
typedef struct {
    /** Included instant of a Lock Free Logger Iterator **/
	SPHLFLogIterator_t	logIter;
    /** Handle of the portal containing this logger **/
	SPHLogPortal_t		portal;
    /** Index of the current logger **/
	longPtr_t			current;
    /** Index of the next free logger (end of the list for non-circular portals) **/
	longPtr_t			next_free;
	/** Capacity (number of attached loggers) of this portal **/
	longPtr_t			capacity;
	} SPHLFPortalIterator_t;

/** \brief Initialize a shared storage block as a Log Portal.
*
*	Initialize the control blocks within the specified storage
*	block as a Log Portal.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type will be SAS_RUNTIME_LOGPORTAL.
*
*	@param buf_seg a block of allocated SAS storage matching the buf_size.
*	@param buf_size power of two size of the portal to be initialized.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLogPortal_t
SPHLogPortalInit (void* buf_seg,  block_size_t buf_size);

/** \brief Allocate and initialize a shared storage block as a Log Portal.
*
*	Allocate a block from SAS storage and initialize that block
*	as a Log Portal.
*	The storage block must be power of two in size.
*
*	@param buf_size power of two size of the portal to be created.
*	@return a handle to the initialized SPHLFLogger_t.
*/
extern __C__ SPHLogPortal_t
SPHLogPortalCreate (block_size_t buf_size);

/** \brief Return the status of the specified Log Portal.
*
*	@param portal Handle to a Log Portal.
*	@return true if the portal is currently Empty (no entries).
*	Otherwise False.
*/
extern __C__ int
SPHLogPortalEmpty (SPHLogPortal_t portal);

/** \brief Return the number of active Loggers in the portal list.
*
*	@param portal Handle to a Log Portal.
*	@return current number of Logger entries for this portal.
*/
extern __C__ int
SPHLogPortalEntries (SPHLogPortal_t portal);

/** \brief Return the maximum number of Loggers the list can hold.
*
*	@param portal Handle to a Log Portal.
*	@return maximum number of Logger entries for this portal.
*/
extern __C__ int
SPHLogPortalCapacity (SPHLogPortal_t portal);

/** \brief Insert a SPHLFLogger_t into the next free slot og this Log Portal.
*	This Logger can be then be used to log events via the Log
*	Portal (when it becomes the current logger). Use the
*	SPHLogPortalAlloc* functions to allocate and complete log entries.
*
*	@param portal Handle to a Log Portal.
*	@param log Handle to a Lock Free Logger to be inserted.
*	@return The provided SPHLFLogger_t handle or NULL.
*	If successful, the provided Logger handle is returned.
*	Otherwise NULL (For example the Add may fail
*	if the Logger list is already at capacity.)
*/
extern __C__ SPHLFLogger_t
SPHLogPortalAddLogger (SPHLogPortal_t portal, SPHLFLogger_t log);

/** \brief Return the handle of the current Logger target of the Portal.
*
*	@param portal Handle to a Log Portal.
*	@return SPHLFLogger_t or NULL, The Logger handle is returned if
*	successful, or NULL if unsuccessful. For example the Get may fail
*	if the Logger list is currently empty or the last available Logger
*	is already full.
*/
extern __C__ SPHLFLogger_t
SPHLogPortalGetCurrentLogger (SPHLogPortal_t portal);

/** \brief Return the handle of the Logger in the Portal list slot specified
*	by the index number.
*
*	@param portal Handle to a Log Portal.
*	@param index index of the logger to be returned.
*	@return SPHLFLogger_t or NULL, The Logger address is returned if
*	successful, or NULL if unsuccessful. For example the Get may fail
*	if the Logger list is currently empty or the index is out of range.
*/
extern __C__ SPHLFLogger_t
SPHLogPortalGetLoggerByIndex (SPHLogPortal_t portal, longPtr_t index);

/** \brief Return the index of the current Logger target of the Portal.
*
*	@param portal Handle to a Log Portal.
*	@return Non-negative int or -1, The current Logger index is
*	returned if successful, or -1 if unsuccessful. For example the Get
*	may fail if the Logger list is currently empty.
*/
extern __C__ int
SPHLogPortalGetCurrentIndex (SPHLogPortal_t portal);

/** \brief Return the handle of a Logger entry allocated from the current
*	logger of the Portal. If the current logger is full, attempt to
*	move current to the next available Logger and retry.
*
*	@param portal Handle to a Log Portal.
*	@param catcode Category code for the new entry.
*	@param subcode subcategory code for the new entry.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return address of handleorg or NULL, The address of the
*	initialized Log Entry Handle is returned if successful, or NULL
*	if unsuccessful (for example the Allocate may fail if the last
*	Logger in the list is full).
*/
extern __C__ SPHLFLoggerHandle_t*
SPHLogPortalAllocStrideTimeStamped (SPHLogPortal_t portal,
                             int catcode, int subcode,
                             SPHLFLoggerHandle_t *handlespace);

/** \brief Return the handle of a Logger entry allocated from the current
*	logger of the Portal. If the current logger is full, attempt to
*	move current to the next available Logger and retry.
*
*	@param portal Handle to a Log Portal.
*	@param catcode Category code for the new entry.
*	@param subcode subcategory code for the new entry.
*	@param alloc_size Size in bytes of the entry to be allocated.
*	The actual entry will be +16 bytes to include the entry header.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return address of handleorg or NULL, The address of the
*	initialized Log Entry Handle is returned if successful, or NULL
*	if unsuccessful (for example the allocate may fail if the last
*	Logger in the list is full).
*/
extern __C__ SPHLFLoggerHandle_t*
SPHLogPortalAllocTimeStamped (SPHLogPortal_t portal,
                             int catcode, int subcode,
                             block_size_t alloc_size,
                             SPHLFLoggerHandle_t *handlespace);

/** \brief Create an iterator positioned at the first entry of the first
*	logger attached to this portal.
*
*	Returns the handle of a Portal Iterator. The iterator can be used to
*	iterate through the completed entries of the attached loggers.
*	Starting with the initial logger and continuing in sequence through
*	any additional loggers that are non-empty.
*
*	@param portal Handle to a Log Portal.
*	@param iteratorSpace Address of local area that will be initialied as a
*	SPHLFLogIterator_t associated with the log.
*	@return the address of the provided Iterator_space or NULL.
*	If successful, The address of Iterator_space initialized as a
*	SPHLFPortalIterator_t is returned.
*	Otherwise NULL is returned (for example if the Portal was empty).
*/
extern __C__ SPHLFPortalIterator_t*
SPHLFPortalCreateIterator (SPHLogPortal_t portal,
                          SPHLFPortalIterator_t *iteratorSpace);

/** \brief Access a sequence of completed logger entries in-order.
*
*	Returns the handle of the current logger entry for the current logger
*	for the provided Portal Iterator. The logger entry handle can
*	of then be used to access the contents of the logger entry.
*	The iterator is advanced to the next logger entry.
*	If the iterator is positioned at the end of the current
*	logger then the iterator will be advanced to the first entry of
*	the next logger allocated to this portal.
*	If we are at the end of the list of allocated loggers we are done.
*
*	@param iterator Handle associated with a Log Portal.
*	@param handlespace Address of local area that will be initialied as a
*	SPHLFLoggerHandle_t for the allocated entry.
*	@return the address of the provided handlespace or NULL.
*	If successful, The address of handlespace initialized as a
*	SPHLFLoggerHandle_t is returned.
*	Otherwise NULL is returned (for example if the Portal was empty or
*	we are at the end of the last logger).
*/
extern __C__ SPHLFLoggerHandle_t*
SPHLFPortalIteratorNext (SPHLFPortalIterator_t *iterator,
                         SPHLFLoggerHandle_t *handlespace);

/** \brief Return the address of a (raw) Logger entry allocated from
*   the current logger.
*
*	The allocation size is rounded up to the next quadword
*	boundary. Mostly for internal use and testing.
*	If the specified portal's loggers are full the allocation may fail.
*
*	@param portal Handle to a Log Portal.
*	@param alloc_size size in bytes of the entry to be allocated.
*	@return the address of the raw Log Entry is returned if successful,
*	or NULL if unsuccessful.
*/
extern __C__ void *
SPHLogPortalAllocRaw (SPHLogPortal_t portal,
                         block_size_t alloc_size);

/** \brief Destroy the Log Portal and free the storage.
*
*	NOTE need to add code to free any attached Loggers
*
*	@param portal Handle to a Logger to be destroyed.
*	@return 0 if successful.
*/
extern __C__ int
SPHLogPortalDestroy(SPHLogPortal_t portal);

#endif /* __SPH_LOG_PORTAL_H */
