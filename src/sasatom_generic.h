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

static inline void *
__arch_fetch_and_add_ptr(void **pointer, long int delta)
{
  return (void*)__sync_fetch_and_add((long int*)*pointer, delta);
}

static inline long int
__arch_fetch_and_add(void *pointer, long int delta)
{
  return __sync_fetch_and_add((long int*)pointer, delta);
}

static inline int
__arch_compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  return __sync_val_compare_and_swap(p, oldval, newval);
}

static inline long int
__arch_atomic_swap (long int *p, long int replace)
{
  long temp = *p;
  *p = replace;
  return temp;
}

static inline void
__arch_atomic_inc (long int *p)
{
  (*p)++;
}

static inline void
__arch_atomic_dec (long int *p)
{
  (*p)--;
}

static inline void
__arch_sas_spin_lock (volatile sas_spin_lock_t *lock)
{
}

static inline int
__arch_sas_spin_trylock (volatile sas_spin_lock_t *lock)
{
  return 1;
}

#endif // _SASATOMIC_GENERIC_H
