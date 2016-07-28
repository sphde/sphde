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

#ifndef _SASATOMIC_X86_64_H
#define _SASATOMIC_X86_64_H

#define __arch_sas_full_barrier()  __sync_synchronize()
#define __arch_sas_read_barrier()  __sync_synchronize()
#define __arch_sas_write_barrier() __sync_synchronize()

static inline void
__arch_pause (void)
{
  __asm__ (
    "  pause;"
   :
   :
   : "memory"
  );
}

#if GCC_VERSION >= 40700
/* GCC is at least version 4.7.  */

static inline void *
__arch_fetch_and_add_ptr (void **pointer, long int delta)
{
  void *temp;
  temp = __atomic_fetch_add(pointer, delta, __ATOMIC_RELAXED);
  return temp;
}

static inline long
__arch_fetch_and_add (long *pointer, long int delta)
{
  long temp;

  temp = __atomic_fetch_add(pointer, delta, __ATOMIC_RELAXED);

  return temp;
}
#else
/* GCC is version 4.6 or less.  */

static inline void *
__arch_fetch_and_add_ptr (void **pointer, long int delta)
{
  void *temp = (void*)delta;
  __asm__ (
    "  lock;"
    "  xadd %0,(%1);"
   : "+r" (temp)
   : "p" (pointer)
   : "memory"
  );
  return temp;
}

static inline long
__arch_fetch_and_add (long *pointer, long int delta)
{
  long temp = delta;

  __asm__ (
    " lock;"
    " xadd %0,(%1);"
   : "+r" (temp)
   : "p" (pointer)
   : "memory"
  );

  return temp;
}
#endif /* GCC is version 4.6 or less.  */

static inline int
__arch_compare_and_swap (volatile long int *p, long int oldval, long int newval)
{
  char ret;
  __asm__ __volatile__ ("lock; cmpxchg %2, %1; sete %0"
                        : "=a" (ret), "=m" (*p)
                        : "r" (newval), "m" (*p), "0" (oldval)
			: "memory");
  return ret;
}

static inline long int
__arch_atomic_swap (long int *p, long int replace)
{
  long int temp;

  __asm__ (
    " lock;"
    " xchg %0,(%1);"
   : "+r" (temp)
   : "p" (p)
   : "memory"
  );

  return temp;
}

static inline void
__arch_atomic_inc (long int *p)
{
  __asm__ __volatile__ (
    "  lock;"
    "  incl (%0);"
   :
   : "p" (p)
   : "memory"
  );
}

static inline void
__arch_atomic_dec (long int *p)
{
  __asm__ (
    "  lock;"
    "  decl (%0);"
   :
   : "p" (p)
   : "memory"
  );
}

static inline void
__arch_sas_spin_lock (volatile sas_spin_lock_t *lock)
{
  char rc;

  do {
    __asm__ __volatile__ ("lock; cmpxchg %2, %1; sete %0"
                          : "=a" (rc), "=m" (*lock)
                          : "r" (1), "m" (*lock), "0" (0)
                          : "memory");
  } while (!rc);
}

static inline int
__arch_sas_spin_trylock (volatile sas_spin_lock_t *lock)
{
  int notlocked = 1;
  char rc;
 
  __asm__ __volatile__ ("lock; cmpxchg %2, %1; sete %0"
                        : "=a" (rc), "=m" (*lock)
                        : "r" (1), "m" (*lock), "0" (0)
                        : "memory");
  notlocked = (rc == 0);
  return notlocked;
}

#endif //_SASATOMIC_X86_64_H
