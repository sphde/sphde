#ifndef __SAS_LOCK_H
#define __SAS_LOCK_H

/*
 * Copyright (c) 2003, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

/**! \file saslock.h
*  \brief Shared Address Space User Locks.
*   Address based locking for shared address space blocks and utility objects.
*
*   SAS Locks are not stored within the SAS blocks, which would create
*   recoverability issues at large scale. Instead locks are address
*   symbolic with active locks maintained in hash tables in shared memory.
*   The byte effective address of the data becomes the lock symbol. In SAS
*   storage addresses are context free across all processes sharing a SAS
*   region. This enables locking across all processes sharing a SAS region.
*
*   SAS Locks implements shared read locks and exclusive write locks.
*   SAS locks are recursive within a thread.
*
*   \todo In a future implementation the intent is to extend SAS Locks
*   to support Write Intent locks. Intent Locks would allow the holder
*   to gain a shared lock that can be upgraded to exclusive write lock
*   later, if needed. A Intent lock allows current holders of read locks
*   to continue, but prevents new read locks from being granted.
*   New read locks are held (not granted) until the corresponding
*   Intent Lock or upgraded Write Lock is released (unlockd).
*
*   \todo Need a API to return the PIDs associated with held locks.
*   This will be needed to recover from hung or crashed processes.
*
**/

/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Generic address to be used as symbolic lock ID.  **/
typedef void *vm_address_t;

/** \brief SAS Lock request types.  **/
typedef enum
{
  SasUserLock__READ,
  SasUserLock__WRITE
} sas_userlock_request_t;

/** \brief Lock Segment Owner
  TRUE if this process was the first to run and created the lock segment.
  Other process discover the exixting lock segment and simply attach it.
*/
extern __C__ int	SasLockOwner;

/** \brief Reset the SAS Lock tables.
*
*	Reset the lock tables to the initialized state. Any held locks will
*	be lost without resuming any waiters.
*
*	\note This API is intended to recover from hung or crashed processes.
*
*/
extern __C__ void
SASLockReset (void);

/** \brief Initialize the SAS Lock tables.
*
*	Initialize the lock tables when a SAS region is created or a process
*	attaches to the SAS Region after the system is restarted.
*	The first process creates a shared memory segment unique to the
*	region and sets SasLockOwner TRUE. Otherwise the existenting shm
*	segment is attached and SasLockOwner is FALSE.
*	This shm segment will hold the shared lock tables that hold active
*	symbolic locks.
*
*/
extern __C__ void
SASLockInit (void);


/** \brief Lock a SAS Address.
*
*	Lock an address for shared read or exclusive write access by this thread.
*
*	@param addr Data address to be locked.
*	@param lockT Lock (SasUserLock__READ, SasUserLock__WRITE) type.
*/
extern __C__ void
SASLock(vm_address_t addr,
	sas_userlock_request_t lockT);

/** \brief UnLock a SAS Address.
*
*	Unlock a previously locked address held by this thread.
*
*	@param addr Data address to be unlocked.
*/
extern __C__ void
SASUnlock(vm_address_t addr);	

/** \brief Print High level Lock Statistic.
*
*/
extern __C__ void
SASLockPrintHighLevelStats(void);

/** \brief Print detailed Lock Statistic.
*
*/
extern __C__ void
SASLockPrintDetailedStats(void);

/** \brief Detach the shared memory segment holding the SAS lock tables.
*
*	Used for cleanup, called from SASCleanup.
*/
extern __C__ void
SASLockDetach (void);

/** \brief Remove the shared memory segment holding the SAS lock tables.
*
*	Used for cleanup, called from SASRemove.
*/
extern __C__ void
SASLockRemove (void);

#endif //__SAS_LOCK_H

