/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef _SASATOMIC_H
#define _SASATOMIC_H

#include <sched.h> // for sched_yield

/*!
 * \file   sasatom.h
 * \brief  Type and functions for SAS atomic operations.
 *
 * This file contains generic SAS atomic functions. Architecture specific code
 * is in its correpondent header ('sasatom_ppc.h' for  PowerPC32 and PowerPC64,
 * 'sasatom_i386.h' for i386, and 'sasatom_x86_64.h' for X86_64).
 * The 'sasatom_generic.h' header is provided as reference implementation and
 * it is not garanted to work correctly.
 */

/// @cond HIDE_FROM_DOXYGEN
#define GCC_VERSION \
        (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
/// @endcond

/*! Spinlock type used on sas_spin_xxx functions */
typedef unsigned int  sas_spin_lock_t;
/*! Pointer to sas_spin_lock_t type */
typedef void*         sas_lock_ptr_t;

#if defined(__powerpc64__) || defined (__powerpc__)
#include "sasatom_powerpc.h"
#elif defined(__x86_64__)
#include "sasatom_x86_64.h"
#elif defined(__i386__)
#include "sasatom_i386.h"
#else
#include "sasatom_generic.h"
#endif

/*!
 * Memory barrier for store operations.
 */
#define sas_write_barrier() __arch_sas_write_barrier()

/*!
 * Memory barrier for load operations.
 */
#define sas_read_barrier()  __arch_sas_read_barrier()

/*!
 * Memory barrier for both load/store operations.
 */
#define sas_full_barrier()  __arch_sas_full_barrier()

/*!
 * Memory barrier for compiler code motion.
 */
#define sas_code_barrier()  __asm ("" ::: "memory")

/*!
 * Atomic fetch and add operation on memory referenced by \a pointer.
 *
 * Performs the atomic operation:
 * { tmp = **pointer; **pointer = tmp + delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline void *
sas_fetch_and_add_ptr(void **pointer, long int delta)
{
  return __arch_fetch_and_add_ptr(pointer, delta);
}

/*!
 * Atomic fetch and add operation on memory \a pointer.
 *
 * Performs the atomic operation:
 * { tmp = *pointer; *pointer = tmp + delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline long int
sas_fetch_and_add(long *pointer, long int delta)
{
  return __arch_fetch_and_add(pointer, delta);
}

/*!
 * Atomic fetch and and operation on memory \a pointer of int type.
 *
 * Performs the atomic operation:
 * { tmp = *pointer; *pointer = tmp & delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline long int
sas_fetch_and_and(unsigned int *pointer, int delta)
{
#if GCC_VERSION >= 40700
  return __atomic_fetch_and(pointer, delta, __ATOMIC_ACQ_REL);
#else
  return __sync_fetch_and_and(pointer, delta);
#endif
}

/*!
 * Atomic fetch and and operation on memory \a pointer of long type.
 *
 * Performs the atomic operation:
 * { tmp = *pointer; *pointer = tmp & delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline long int
sas_fetch_and_and_long(unsigned long *pointer, long int delta)
{
#if GCC_VERSION >= 40700
  return __atomic_fetch_and(pointer, delta, __ATOMIC_ACQ_REL);
#else
  return __sync_fetch_and_and(pointer, delta);
#endif
}

/*!
 * Atomic fetch and or operation on memory \a pointer of int type.
 *
 * Performs the atomic operation:
 * { tmp = *pointer; *pointer = tmp | delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline long int
sas_fetch_and_or(unsigned int *pointer, int delta)
{
#if GCC_VERSION >= 40700
  return __atomic_fetch_or(pointer, delta, __ATOMIC_ACQ_REL);
#else
  return __sync_fetch_and_or(pointer, delta);
#endif
}

/*!
 * Atomic fetch and or operation on memory \a pointer of long type.
 *
 * Performs the atomic operation:
 * { tmp = *pointer; *pointer = tmp | delta; return *pointer }
 *
 * Returns \a *pointer before update.
 */
static inline long int
sas_fetch_and_or_long(unsigned long *pointer, long int delta)
{
#if GCC_VERSION >= 40700
  return __atomic_fetch_or(pointer, delta, __ATOMIC_ACQ_REL);
#else
  return __sync_fetch_and_or(pointer, delta);
#endif
}
/*!
 * Atomic compare and swap operation.
 *
 * Performs the atomic operation:
 * { *p == oldval ? *p = newval : *p; return *p }
 *
 * Returns \a oldval.
 */
static inline int
sas_compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  return __arch_compare_and_swap(p, oldval, newval);
}

/*!
 * Atomic swap operation.
 *
 * Performs the atomic operation:
 * { tmp = *p; *p = tmp; return tmp }
 */
static inline long int
sas_atomic_swap (long int *p, long int replace)
{
  return __arch_atomic_swap(p, replace);
}

/*!
 * Atomic increment, performing { (*p)++ }
 */
static inline void
sas_atomic_inc(long int *p)
{
  __arch_atomic_inc(p);
}

/*!
 * Atomic decrement, performing { (*p)-- }
 */
static inline void
sas_atomic_dec(long int *p)
{
  __arch_atomic_dec(p);
}

/*!
 * Initializes the spinlock \a lock so it can be used with lock and 
 * unlock functions.
 */
static inline void
sas_spin_lock_init (volatile sas_spin_lock_t *lock)
{
  sas_read_barrier();
  *lock = 0;
}

/*!
 * Locks the spinlock \a lock.
 *
 * The function uses a busylock algorithm in the form:
 * while ({tmp = *lock; *lock = 1; });
 */
static inline void
sas_spin_lock (volatile sas_spin_lock_t *lock)
{
  __arch_sas_spin_lock(lock);
}

/*!
 * Try to lock the spinlock \a lock.
 *
 * It returns if the lock \a lock can not be locked.
 *
 * Returns 1 if the spinlock was locked, 0 otherwise.
 */
static inline int
sas_spin_trylock (volatile sas_spin_lock_t *lock)
{
  return __arch_sas_spin_trylock(lock);
}

/*!
 * Unlocks the spinlock \a lock reseting it to initial value.
 */
static inline void
sas_spin_unlock (volatile sas_spin_lock_t *lock)
{
  sas_read_barrier();
  *lock = 0;
}

/*!
 * Initializes the spinlock pointer \a lock so it can be used with lock
 * and unlock functions.
 */
static inline void
sas_lock_ptr_init (volatile sas_lock_ptr_t *lock)
{
  sas_read_barrier();
  *lock = NULL;
}

/*!
 * Locks the spinlock pointer \a lock and returns its unlocked value.
 */
static inline void*
sas_lock_ptr (volatile sas_lock_ptr_t *lock)
{
  int rc;
  long unlocked, locked;
  do {
    unlocked = (long)(*lock) & -2L;
    locked = unlocked | 1;
    rc = sas_compare_and_swap ((long int *)lock, unlocked, locked);
  } while (!rc);
  sas_write_barrier();
  
  return (void*)unlocked;
}

/*!
 * Spins over spinlock pointer \a lock until it is unlocked then swap its
 * value with \a newptr locked value.
 */
static inline void
sas_set_unlocked_ptr (volatile sas_lock_ptr_t *lock, sas_lock_ptr_t newptr)
{
  int rc;
  do {
    long unlocked= (long)(*lock) & -2L;
    long newlocked= (long)newptr | 1;
    rc = sas_compare_and_swap ((long int *)lock, unlocked, newlocked);
  } while (!rc);
  sas_write_barrier();
}

/*!
 * Spins over lock \a lock until it is locked then swap its value with
 * \a newptr locked value.
 */
static inline void
sas_set_locked_ptr (volatile sas_lock_ptr_t *lock, sas_lock_ptr_t newptr)
{
  int rc;
  do {
    long locked= (long)(*lock) | 1;
    long newlocked= (long)newptr | 1;
    rc = sas_compare_and_swap ((long int *)lock, locked, newlocked);
  } while (!rc);
}

/*!
 * The function tried to lock \a lock. If the operation is successful the
 * function return 0 or 1 otherwise.
 */
static inline int
sas_trylock_ptr (volatile sas_lock_ptr_t *lock)
{
  int rc;
  {
    long unlocked= (long)(*lock) & -2L;
    long locked= unlocked | 1;
    rc = sas_compare_and_swap ((long int *)lock, unlocked, locked);
  }
  sas_write_barrier();
  
  return !rc;
}

/*!
 * Unlocks the spinlock \a lock.
 */
static inline void
sas_unlock_ptr (volatile sas_lock_ptr_t *lock)
{
  int rc;
  
  sas_read_barrier();
  do {
    long unlocked= (long)(*lock) & -2L;
    long locked= unlocked | 1;
    rc = sas_compare_and_swap ((long int *)lock, locked, unlocked);
  } while (!rc);
}

/*!
 * The function tries 4 times to lock \a lock and if it fails it calls
 * 'sched_yield' returning with another lock try.
 */
static inline void
sas_spin_lock_with_yield (volatile sas_spin_lock_t *lock)
{
  int rc;
  
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
  if (sas_spin_trylock(lock) == 0)
    return;
    
  do
    {
      sched_yield();
      rc = sas_spin_trylock(lock);
    } 
  while (rc);
}

/*!
 * The function tried 4 times to lock \a lock pointer and if it fails
 * it calls 'sched_yield' returning with another lock try.
 */
static inline void
sas_lock_ptr_with_yield (volatile sas_lock_ptr_t *lock)
{
  int rc;
  
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
  if (sas_trylock_ptr(lock) == 0)
    return;
    
  do
    {
      sched_yield();
      rc = sas_trylock_ptr(lock);
    } 
  while (rc);

  return;
}
#if 1
/*!
 * Atomically increments \a value by 1.
 */
static inline long
sas_atomic_inc_long (volatile long *value)
{
  long result;
  long delta = 1;

#if GCC_VERSION >= 40700
  result = __atomic_fetch_add(value, delta, __ATOMIC_ACQUIRE);
#else
  result = __sync_fetch_and_add(value, delta);
#endif
  
  return result;
}

/*!
 * Atomically decrements \a value by 1.
 */
static inline long
sas_atomic_dec_long (volatile long *value)
{
  long result;
  long delta = -1;

#if GCC_VERSION >= 40700
  result = __atomic_fetch_add(value, delta, __ATOMIC_ACQUIRE);
#else
  result = __sync_fetch_and_add(value, delta);
#endif
  
  return result;
}
#endif
#endif /*_SASATOMIC_H */

