/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, M.P. Johnson - initial API and implementation
 */

#include "sphthread.h"
#include "saslock.h"
#include "sasulock.h"
#include <string.h>             // memset()

//#define mylockdebug
//#define coherenceCheck

#if defined(mylockdebug) || defined(coherenceCheck)
#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>
#endif

// See instructions in SasUserLockTest.C to explain what SasOstream.H
// is and when it would be used.
#ifdef lockthreadtest
#include "sasostrm.H"
#endif

#define WAKE_ALL	FALSE

#define WAKE_ONE	TRUE
/*
extern "C"
{
 * prototypes from kern/syscall_subr.c *
extern kern_return_t
syscall_vm_thread_sleep(vm_address_t		event,
                        long *			clear,
                        mach_msg_timeout_t	timeout);

extern kern_return_t
syscall_vm_thread_wakeup(vm_address_t		event,
                         boolean_t		wake_one);
}
*/

// Constructor
SasUserLock::SasUserLock(vm_address_t addrToLock)
{
#ifdef collectstats
  useageCount = 0;
#endif

  address = addrToLock;
  spin_lock_init(&data_lock);
  spin_lock(&data_lock);

  //eyecatcher = 0x756C636B;
  status                        = SasUserLock__not_exclusive;
  total_readers_count           = 0;
  readers_waiting               = 0;
  sem_init(&readers_waiting_sem, 1, MAX_READER_THREADS);
  writers_waiting               = 0;
  sem_init(&writers_waiting_sem, 1, 1);
  writer_thread_id              = 0;
  writer_task_id                = 0;
  writer_thread_lock_count      = 0;
  memset((void *)readers, 0x00, sizeof(readers));
  //eyecatcher2 = 0x6B636C75;

  spin_unlock(&data_lock);
}

// Destructor
SasUserLock::~SasUserLock(void)
{
// Unlike SasLock, we do not have to lock the lock object
// itself before destroying it.  The only time it will
// be destroyed is when its containing list is destroyed,
// which, in turn, is destroyed when the SasMasterLock
// object is destroyed.  During SasMasterLock destruction,
// each list is write-locked in its destructor, so there's
// no need to lock any of the lock objects on the list.
#ifdef mylockdebug
  pid_t this_thread = sphdeGetTID();
  pid_t this_task = getpid();
#endif
  spin_lock(&data_lock);
  sem_destroy(&readers_waiting_sem);
  sem_destroy(&writers_waiting_sem);
  if ((total_readers_count == 0) &&
      (readers_waiting == 0) &&
      (writers_waiting == 0))
  {
    return;
  }
  else
  {
    spin_unlock(&data_lock);
#ifdef mylockdebug
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, " thread[%ld,%ld]\n",
      (long int) this_task, (long int) this_thread);
    fprintf (stderr, " object  = %p\n", this);
#endif
  }
}

int
SasUserLock::operator==(vm_address_t addrToLock)
{
#ifdef mylockdebug
    pid_t this_thread = sphdeGetTID();
    pid_t this_task = getpid();
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, " thread[%ld,%ld]\n",
      (long int) this_task, (long int) this_thread);
    fprintf (stderr, "  input address: %p", addrToLock);
    fprintf (stderr, " | lock object's address: %p\n", address);
#endif
    if (address == addrToLock)
        return TRUE;
    return FALSE;
}

void
SasUserLock::thread_sleep (	vm_address_t event,
				sem_t       *sem,
  				spin_lock_t *lock)
{
#ifdef mylockdebug
    pid_t this_thread = sphdeGetTID();
    pid_t this_task = getpid();
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, "  thread[%ld,%ld] : %p, %p\n",
      (long int) this_task, (long int) this_thread, event, sem);
#endif
    spin_unlock(&data_lock);
    sem_wait(sem);
}

void
SasUserLock::thread_wakeup(	vm_address_t event,
				sem_t       *sem,
  				boolean_t wake_one)
{
    int	waiters = wake_one ? 1 : *(int*)event;
    int i;

#ifdef mylockdebug
    pid_t this_thread = sphdeGetTID();
    pid_t this_task = getpid();
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, "  thread[%ld,%ld] : %p %i\n",
      (long int) this_task, (long int) this_thread, event, waiters);
#endif

    for ( i = 0; i < waiters; i++ )
	sem_post(sem);
}

// Get a shared read lock.  Many threads can have a read lock.
// While read locks are held no write lock will be granted.
// The lockObj pointer passed in allows us to release the
// lock on the list on which this SasUserLock object resides
// (see SasLockList class).
void
SasUserLock::read_lock(SasUserLock * lockObj, vm_address_t lockAddr)
{
  // mach_thread_self() is a ukernel call so let's
  // isue it only once in this routine.  This is
  // a local automatic variable so it is also
  // thread-safe...
  pid_t this_thread = sphdeGetTID();
  pid_t this_task = getpid();
#ifdef mylockdebug
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld] - beginning attempt to lock",
      (long int) this_task, (long int) this_thread);
  fprintf (stderr, " object = %p\n", this);
#endif

  spin_lock(&data_lock);
// OK--now that we've got the spin lock, we must check if we need
// to unlock the SasLockList lock.  If the caller did not
// specify a lock object, then by default, lockObj is set = NULL.
// So, if lockObj == NULL, that implies the user did not pass in
// a lock object for their SasLockList and therefore, do not need us
// to unlock it.
  if (lockObj != NULL)
      lockObj->unlock();


  // if this thread already has a write lock then
  // just inc write lock count and return
  if ((writer_thread_id == this_thread) &&
      (writer_task_id == this_task))
  {
    writer_thread_lock_count++;
#ifdef mylockdebug
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, " thread[%ld,%ld] - lock obtained",
      (long int) this_task, (long int) this_thread);
    fprintf (stderr, " | object = %p", this);
    fprintf (stderr, " | writer thread count = %i\n",
      writer_thread_lock_count);
#endif
#ifdef collectstats
    ++useageCount;
#endif
    address = lockAddr;
    spin_unlock(&data_lock);
    return;
  }

  // Since here, this thread does NOT have a write lock.
  // It may however, already have a read lock.  If it
  // does, we want to let it get it again and get out.
  // If it does not, then we may or may not give it the
  // lock.  If a writer thread is waiting for the lock
  // we will not allow any new readers.  And of course,
  // if a writer thread had the lock, we obviously will
  // not give this thread the lock now.
  // So, first scan to see if this thread has a
  // read lock already.  Note that we still have
  // the spin lock.
  if (total_readers_count)
  { // only scan if there ARE readers
    for (int i = 0; i < MAX_READER_THREADS; i++)
    {
      if ((readers[i].reader_thread_id == this_thread) &&
          (readers[i].reader_task_id == this_task))
      {
        // this thread already has a read lock
        readers[i].reader_thread_lock_count++;
        total_readers_count++;
#ifdef mylockdebug
        fprintf (stderr, "%s\n", __FUNCTION__);
        fprintf (stderr, " thread[%ld,%ld] - lock obtained",
          (long int) this_task, (long int) this_thread);
        fprintf (stderr, " | object = %p", this);
        fprintf (stderr, " | reader count = %i", readers[i].reader_thread_lock_count);
        fprintf (stderr, " | total reader count = %i\n", total_readers_count);
#endif
#ifdef collectstats
	++useageCount;
#endif
	address = lockAddr;
        spin_unlock(&data_lock);
        return;
      }
    }  // end scan for has reader lock already
  }    // end if(total_readers_count)

  // wait until all the writers are done.
  while (status == SasUserLock__exclusive || writers_waiting)
    {
      readers_waiting++;

      SasUserLock::thread_sleep((vm_address_t)&readers_waiting,
				&readers_waiting_sem, &data_lock);

      spin_lock(&data_lock);	// need to relock since above unlocks
      if (readers_waiting)
          readers_waiting--;

    } // while

  // Ok, we have the spin lock here, AND
  // we have no exclusive or writer waiting flags
  // so this means WE GET THE LOCK.  We need
  // to track WHO we are so add it to the
  // list of reader locks.
  for (int j = 0; j < MAX_READER_THREADS; j++)
    {
      if (readers[j].reader_thread_lock_count > 0)
        {
          // not an open slot
#ifdef coherenceCheck
          if ((readers[j].reader_thread_id == this_thread) &&
              (readers[j].reader_task_id == this_task))
            {
              // should never already be in the table here.
              fprintf (stderr, "%s\n", __FUNCTION__);
              fprintf (stderr, " thread[%ld,%ld] - thread is already in",
                (long int) this_task, (long int) this_thread);
              fprintf (stderr, "the table with %d read thread lock count\n",
                total_readers_count++);
              // guess if this happens we will
              // just give it to him...
              readers[j].reader_thread_lock_count++;
              total_readers_count++;
            }
#endif
        }
      else
        {
          // an open slot - use it
#ifdef coherenceCheck
          if (readers[j].reader_thread_lock_count < 0)
            {
              fprintf (stderr, "%s\n", __FUNCTION__);
              fprintf (stderr, " thread[%ld,%ld] - reader thread count is "
                               "negative %i\n",
                (long int) this_task, (long int) this_thread,
                readers[j].reader_thread_lock_count);
            }
#endif
          readers[j].reader_thread_id =
            this_thread;
          readers[j].reader_task_id = this_task;
          readers[j].reader_thread_lock_count = 1;
          total_readers_count++;

#ifdef mylockdebug
          fprintf (stderr, "%s\n", __FUNCTION__);
          fprintf (stderr, " thread[%ld,%ld] - lock obtained\n",
            (long int) this_task, (long int) this_thread);
          fprintf (stderr, " object  = %p", this);
          fprintf (stderr, " | reader thread count = %i",
            readers[j].reader_thread_lock_count);
          fprintf (stderr, " | total readers count = %i\n",
            total_readers_count);
#endif
#ifdef collectstats
	  ++useageCount;
#endif
	  address = lockAddr;
          spin_unlock(&data_lock);
          return;
        }  // end open slot in table
    }   // end for loop looking for a slot

  // if we get here, we still have the spin lock,
  // and we did not find a slot to use.  I can't
  // readily hang this thread, but I CAN hang onto
  // the spin lock so that all other threads will
  // lock up.  Oughta notice that.  Thus,
  // I intentionally didn't release the spin lock
  // here.
#ifdef mylockdebug
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld] - no opne slots in reader table\n",
    (long int) this_task, (long int) this_thread);
#endif
}

// Get non-shared write lock.  A thread can call this more than once.
// The lock count will be increamented.  In a thread you must
// unlock as many times as you lock.
// The lockObj pointer passed in allows us to release the
// lock on the list on which this SasUserLock object resides
// (see SasLockList class).
void
SasUserLock::write_lock(SasUserLock * lockObj, vm_address_t lockAddr)
{
  // mach_thread_self() is a ukernel call so let's
  // isue it only once in this routine.  This is
  // a local automatic variable so it is also
  // thread-safe...
  pid_t this_thread = sphdeGetTID();
  pid_t this_task = getpid();
#ifdef mylockdebug
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld] - attempt\n",
    (long int) this_task, (long int) this_thread);
  fprintf (stderr, " object  = %p\n", this);
#endif

  boolean_t	wakeup_writers = FALSE;
  boolean_t	wakeup_readers = FALSE;
  spin_lock(&data_lock);

// OK--now that we've got the spin lock, we must check if we need
// to unlock another lock object.  If the caller did not
// specify a lock object, then by default, lockObj is set = NULL,
// so there's nothing to do.
  if (lockObj != NULL)
      lockObj->unlock();


  // if this thread already has the the write lock
  // then inc count and return
  if ((writer_thread_id == this_thread) &&
      (writer_task_id == this_task))
  {
    writer_thread_lock_count++;
#ifdef mylockdebug
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, " thread[%ld,%ld] - obtained\n",
    (long int) this_task, (long int) this_thread);
    fprintf (stderr, " object  = %p | count = %i\n",
      this, writer_thread_lock_count);
#endif
#ifdef collectstats
    ++useageCount;
#endif
    address = lockAddr;
    spin_unlock(&data_lock);
    return;
  }

  // wait until the other write lock is released
  while (status == SasUserLock__exclusive)
  {
    writers_waiting++;

    SasUserLock::thread_sleep(	(vm_address_t)&writers_waiting,
				&writers_waiting_sem, &data_lock);

    spin_lock(&data_lock);	// relock since sleep does an unlock
    if (writers_waiting)
      writers_waiting--;
  } // while

  // mark status so other threads can not acquire NEW read locks
  status = SasUserLock__exclusive;

  // now wait until all other users get done
  if (writers_waiting)
    wakeup_writers = TRUE;
  if (readers_waiting)
    wakeup_readers = TRUE;

  spin_unlock(&data_lock);

  // use local variables outside of spin lock
  if (wakeup_writers)
  {
    SasUserLock::thread_wakeup(	(vm_address_t)&writers_waiting,
				&writers_waiting_sem, WAKE_ONE);
  }

  if (wakeup_readers)
  {
    SasUserLock::thread_wakeup(	(vm_address_t)&readers_waiting,
				&readers_waiting_sem, WAKE_ALL);
  }

  spin_lock(&data_lock);
  // while still being used by the readers
  while (total_readers_count)
  {
    // look like a writer so that the unlock code wakes us up.
    writers_waiting++;

    SasUserLock::thread_sleep(	(vm_address_t)&writers_waiting,
				&writers_waiting_sem, &data_lock);

    spin_lock(&data_lock);
    if (writers_waiting)
      writers_waiting--;
  }
  // no more readers, so we now have the write lock

#ifdef coherenceCheck
  if (writer_thread_lock_count != 0)
    {
      fprintf (stderr, "%s\n", __FUNCTION__);
      fprintf (stderr, " thread[%ld,%ld] - just obtained write lock and "
        "writer lock count was not zero but was %i\n",
        (long int) this_task, (long int) this_thread,
        writer_thread_lock_count);
    }
#endif

#ifdef mylockdebug
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld] - obtained\n",
    (long int) this_task, (long int) this_thread);
  fprintf (stderr, " object  = %p\n", this);
#endif
#ifdef collectstats
  ++useageCount;
#endif
  address = lockAddr;
  writer_thread_id = this_thread;
  writer_task_id = this_task;
  writer_thread_lock_count = 1;
  spin_unlock(&data_lock);
}

void
SasUserLock::unlock(void)
{
  pid_t this_thread = sphdeGetTID();
  pid_t this_task = getpid();
#ifdef mylockdebug
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld]\n",
    (long int) this_task, (long int) this_thread);
  fprintf (stderr, " object  = %p\n", this);
#endif
  boolean_t	wakeup_writers = FALSE;
  boolean_t	wakeup_readers = FALSE;
  // mach_thread_self() is a ukernel call so let's
  // issue it only once in this routine.  This is
  // a local automatic variable so it is also
  // thread-safe...
  spin_lock(&data_lock);

  // if there are any readers, then no write locks,
  // so unlock a read lock
  if (total_readers_count)
  {
    total_readers_count--;
// Zero-out the address datamember to indicate the lock object
// is available for use.
    if (total_readers_count==0)
	address = NullAddress;

#ifdef coherenceCheck
    if (total_readers_count < 0)
    {
      fprintf (stderr, "%s\n", __FUNCTION__);
      fprintf (stderr, " thread[%ld,%ld] - total readers count is negative: %i\n",
        (long int) this_task, (long int) this_thread,
        total_readers_count);
    }
#endif
    if ((total_readers_count==0) && (writers_waiting))
      wakeup_writers = TRUE;
    // unlock the reader that has it
    for (int i = 0; i < MAX_READER_THREADS; i++)
    {
      if ((readers[i].reader_thread_id ==
           this_thread) &&
          (readers[i].reader_task_id ==
           this_task))
      {
        readers[i].reader_thread_lock_count--;
#ifdef coherenceCheck
        if (readers[i].reader_thread_lock_count < 0)
        {
          fprintf (stderr, "%s\n", __FUNCTION__);
          fprintf (stderr, " thread[%ld,%ld] - lock count just went negative %i\n",
            (long int) this_task, (long int) this_thread,
            readers[i].reader_thread_lock_count);
        }
#endif
#ifdef mylockdebug
        fprintf (stderr, "%s\n", __FUNCTION__);
        fprintf (stderr, " thread[%ld,%ld]\n",
          (long int) this_task, (long int) this_thread);
        fprintf (stderr, " object  = %p | read thread count = %i\n",
          this, readers[i].reader_thread_lock_count);
#endif
        if (readers[i].reader_thread_lock_count == 0)
        {
          // remove the thread id
          readers[i].reader_thread_id = 0;
          readers[i].reader_task_id = 0;
        }
      }
    } // end for loop

  }     // end readers were waiting
  else
  {
    if (status == SasUserLock__exclusive)	// unlock a write lock
    {
      writer_thread_lock_count--;
#ifdef coherenceCheck
      if (writer_thread_lock_count < 0)
      {
        fprintf (stderr, "%s\n", __FUNCTION__);
        fprintf (stderr, " thread[%ld,%ld] - write lock count is negative %i\n",
          (long int) this_task, (long int) this_thread,
          writer_thread_lock_count);
      }
#endif
      if (writer_thread_lock_count)
      {
#ifdef mylockdebug
        fprintf (stderr, "%s\n", __FUNCTION__);
        fprintf (stderr, " thread[%ld,%ld]\n",
          (long int) this_task, (long int) this_thread);
        fprintf (stderr, " object  = %p | writer count = %i\n",
          this, writer_thread_lock_count);
#endif
        spin_unlock(&data_lock);
        return;
      }
      // Zero-out the address datamember to indicate the lock object
      // is available for use.
      address = NullAddress;

      writer_thread_id = 0;
      writer_task_id = 0;
      status = SasUserLock__not_exclusive;

      if (writers_waiting)
        wakeup_writers = TRUE;

      if (readers_waiting)
        wakeup_readers = TRUE;

    }       // end status==lock_exclusive (a write lock)
    else	// change this to coherency check later
    {
#ifdef coherenceCheck
      fprintf (stderr, "%s\n", __FUNCTION__);
      fprintf (stderr, " thread[%ld,%ld]\n",
        (long int) this_task, (long int) this_thread);
      fprintf (stderr, " object  = %p | writer count = %i | reader count = %i\n",
        this, writer_thread_lock_count, total_readers_count);
#endif
      spin_unlock(&data_lock);
      return;
    }
#ifdef mylockdebug
    fprintf (stderr, "%s\n", __FUNCTION__);
    fprintf (stderr, " thread[%ld,%ld]\n",
      (long int) this_task, (long int) this_thread);
    fprintf (stderr, " object  = %p | write count = %i\n",
      this, writer_thread_lock_count);
#endif

  } // end else no readers waiting
  spin_unlock(&data_lock);

  if (wakeup_writers)
  {
    SasUserLock::thread_wakeup(	(vm_address_t)&writers_waiting,
				&writers_waiting_sem, WAKE_ONE);
  }
  if (wakeup_readers)
  {
    SasUserLock::thread_wakeup(	(vm_address_t)&readers_waiting,
				&readers_waiting_sem, WAKE_ALL);
  }
}

boolean_t
SasUserLock::waiters(void)
{
#ifdef mylockdebug
  pid_t this_thread = sphdeGetTID();
  pid_t this_task = getpid();
  fprintf (stderr, "%s\n", __FUNCTION__);
  fprintf (stderr, " thread[%ld,%ld]\n",
    (long int) this_task, (long int) this_thread);
#endif
  if ((readers_waiting) || (writers_waiting))
    return TRUE;
  else
    return FALSE;
}

