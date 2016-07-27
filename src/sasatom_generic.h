/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 */

#ifndef _SASATOMIC_GENERIC_H
#define _SASATOMIC_GENERIC_H

#define __arch_sas_write_barrier() __asm ("" ::: "memory")
#define __arch_sas_read_barrier()  __asm ("" ::: "memory")
#define __arch_sas_full_barrier()  __asm ("" ::: "memory")

static inline void
__arch_pause (void)
{
  __asm__ (
    "  nop;"
   :
   :
   : "memory"
  );
}

#if GCC_VERSION >= 40700
static inline void *
__arch_fetch_and_add_ptr(void **pointer, long int delta)
{
  return __atomic_fetch_add (pointer, delta, __ATOMIC_RELAXED);
}
#else
static inline void *
__arch_fetch_and_add_ptr(void **pointer, long int delta)
{
  return (void*)__sync_fetch_and_add((long int*)*pointer, delta);
}
#endif

#if GCC_VERSION >= 40700
static inline long int
__arch_fetch_and_add(long int *pointer, long int delta)
{
  return __atomic_fetch_add (pointer, delta, __ATOMIC_RELAXED);
}
#else
static inline long int
__arch_fetch_and_add(void *pointer, long int delta)
{
  return __sync_fetch_and_add((long int*)pointer, delta);
}
#endif

#if GCC_VERSION >= 40700
static inline int
__arch_compare_and_swap (volatile long int *pointer,
		long int oldval, long int newval)
{
  long int temp = oldval;
  return __atomic_compare_exchange_n (pointer, &temp, newval, 1, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
}
#else
static inline int
__arch_compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  return __sync_val_compare_and_swap(p, oldval, newval);
}
#endif

#if GCC_VERSION >= 40700
static inline long int
__arch_atomic_swap (long int *pointer, long int replace)
{
  return __atomic_exchange_n (pointer, replace, __ATOMIC_RELAXED);
}
#else
static inline long int __arch_atomic_swap(long int *p, long int replace)
{
	long temp = *p;
	int success = 0;
	//*p = replace;
	do {
		temp = *p;
		success = __sync_bool_compare_and_swap (p, temp, replace);
	}while (!success);

	return temp;
}
#endif

#if GCC_VERSION >= 40700
static inline void
__arch_atomic_inc(long int *pointer)
{
  const long int inc_val = 1L;

  __atomic_fetch_add (pointer, inc_val, __ATOMIC_RELAXED);
}
#else
static inline void
__arch_atomic_inc (long int *pointer)
{
	const long int inc_val = 1L;
    //(*p)++;
	__sync_fetch_and_add (pointer, inc_val);
}
#endif

#if GCC_VERSION >= 40700
static inline void
__arch_atomic_dec(long int *pointer)
{
  const long int inc_val = -1L;

  __atomic_fetch_add (pointer, inc_val, __ATOMIC_RELAXED);
}
#else
static inline void
__arch_atomic_dec (long int *pointer)
{
	const long int inc_val = -1L;
    //(*p)--;
	__sync_fetch_and_add (pointer, inc_val);
}
#endif

#if GCC_VERSION >= 40700
static inline void
__arch_sas_spin_lock (volatile sas_spin_lock_t *lock)
{
  int oldval  = 0;
  int newval  = 1;
  int success = 0;

  do {
     success = __atomic_compare_exchange_n (lock, &oldval, newval, 1,
    		 __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
  } while (!success);
}
#else
static inline void
__arch_sas_spin_lock (volatile sas_spin_lock_t *lock)
{
	  int oldval  = 0;
	  int newval  = 1;
	  int success = 0;

	  do {
	     success = __sync_bool_compare_and_swap (lock, oldval, newval);
	  } while (!success);
}
#endif

#if GCC_VERSION >= 40700
static inline  int
__arch_sas_spin_trylock (volatile sas_spin_lock_t *lock)
{
  int oldval  = 0;
  int newval  = 1;
  int success = 0;

  success = __atomic_compare_exchange_n (lock, &oldval, newval, 1,
    		 __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);

  return (!success);
}
#else
static inline int
__arch_sas_spin_trylock (volatile sas_spin_lock_t *lock)
{
	  int oldval  = 0;
	  int newval  = 1;
	  int success;

	  success = __sync_bool_compare_and_swap (lock, oldval, newval);

	  return (!success);
}
#endif
#endif // _SASATOMIC_GENERIC_H
