/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, M. P. Johnson - initial API and implementation
 */

#ifndef _SASUSERLOCK_H
#define _SASUSERLOCK_H

#include <stdlib.h>	// size_t
#include <unistd.h>	// pid_t
#include <pthread.h> // pthread_t
#include <sched.h> // sched_yield
#include <semaphore.h> // sem_t
#include "sasatom.h"

typedef pid_t task_t;
typedef pthread_t thread_t;
typedef volatile long spin_lock_t;

static inline void
spin_lock_init(spin_lock_t *data_lock)
{
	*data_lock = 0;
}

static inline void
spin_lock(spin_lock_t *data_lock)
{
	unsigned int count = 0;
	while(! sas_compare_and_swap(data_lock, 0L, 1L) ) 
	{
		if ( (++count % 8) == 0 )
		{
			sched_yield();
		}
	};
}

static inline void
spin_unlock(spin_lock_t *data_lock)
{
	sas_read_barrier();
	*data_lock = 0;
}


typedef enum
{
	FALSE,
	TRUE
} boolean_t;

const vm_address_t NullAddress = NULL;

// This class provides a shared read lock, and exclusive write lock.
// It is also a counting lock, so that a single thread may get
// the write lock more than once.  That thread should also
// unlock just as many times as it locks.
// NOTE:  This class is designed to be used in conjunction with
// SasLockList class.  Refer to SasLockList.H for more details.
typedef enum
{
  SasUserLock__SUCCESS,
  SasUserLock__LOCK_NOT_HELD,
  SasUserLock__LOCK_DESTRUCT_ERROR
} SasUserLock__error_code_t;


typedef enum
{
  SasUserLock__not_exclusive,
  SasUserLock__exclusive
} SasUserLock__status_t;

typedef struct
{
  task_t        reader_task_id;
  task_t        reader_thread_id;
  int           reader_thread_lock_count;
} reader_id_t;

#define  MAX_READER_THREADS  10

class SasUserLock
{
public:
  SasUserLock(vm_address_t addr = NullAddress);
  ~SasUserLock(void);
//  void * operator new(size_t size);
//  void operator delete(void * object);
  int operator==(vm_address_t addrToLock);
  unsigned long getAddrKey(void) { return (unsigned long) address; }
  unsigned long getWriterPID(void) { return (unsigned long) writer_task_id; }
  unsigned long getWriterTID(void) { return (unsigned long) writer_thread_id; }
  void read_lock(SasUserLock * lockObj = NULL,
		 vm_address_t lockAddr = NullAddress);
  void write_lock(SasUserLock * lockObj = NULL,
		  vm_address_t lockAddr = NullAddress);
  void unlock(void);
#ifdef collectstats
  unsigned int getUseageCount(void) {return useageCount; }
#endif
  boolean_t	waiters(void);
private:
  void thread_sleep (	vm_address_t event,
			sem_t       *sem,
  			spin_lock_t *lock);
  void thread_wakeup(	vm_address_t event,
			sem_t       *sem,
  			boolean_t wake_one);
			
  //unsigned int		eyecatcher;
  spin_lock_t           data_lock;
  volatile SasUserLock__status_t     status;
  volatile int          total_readers_count;
  volatile int          readers_waiting;
  sem_t                 readers_waiting_sem;
  volatile int          writers_waiting;
  sem_t                 writers_waiting_sem;
  pid_t                 writer_task_id;
  pid_t                 writer_thread_id;
  volatile int          writer_thread_lock_count;
  reader_id_t           readers[MAX_READER_THREADS];
  vm_address_t		address;
#ifdef collectstats
  unsigned int		useageCount;
#endif
			
  //unsigned int		eyecatcher2;
};

#endif // _SasUserLock_H

