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

#ifndef _SASATOMIC_POWERPC_H
#define _SASATOMIC_POWERPC_H

#if defined(__powerpc64__)
/* ALl PPC64 platforms the code cares are ISA V-2.0+ */
#define __arch_sas_write_barrier() __asm ("isync"  ::: "memory")
#define __arch_sas_read_barrier()  __asm ("lwsync" ::: "memory")
#define __arch_sas_full_barrier()  __asm ("sync"   ::: "memory")
#elif defined(__powerpc__)
/* PPC32 might be older ISA-1.0 machine */
#define __arch_sas_write_barrier() __asm ("isync"  ::: "memory")
#if defined(_ARCH_PWR4)
/* For ISA 2.0 or later we can use light weight sync.  */
#define __arch_sas_read_barrier()  __asm ("lwsync" ::: "memory")
#else
#define __arch_sas_read_barrier()  __asm ("sync"   ::: "memory")
#endif
#define __arch_sas_full_barrier()  __asm ("sync"   ::: "memory")
#endif

#if defined(_ARCH_PWR7)
/* For ISA 2.6 or later we can change the thread priority.  */
#define __arch_sas_PPR_low()  __asm ("or 1,1,1;" ::: "memory")
#define __arch_sas_PPR_medium_low()  __asm ("or 6,6,6;" ::: "memory")
#define __arch_sas_PPR_medium()  __asm ("or 2,2,2;" ::: "memory")
#endif

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

#if defined(__64BIT__) || defined(__powerpc64__) || defined(__ppc64__)
#  define LPARX "ldarx"
#  define STPCX "stdcx."
#else
#  define LPARX "lwarx"
#  define STPCX "stwcx."
#endif

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
  void *temp = 0;

  __asm__ (
    "0: " LPARX " %0,0,%1;"
    "    add    11,%0,%2;"
    "   " STPCX " 11,0,%1;"
    "    bne    0b;"
    : "+b" (temp)
    : "p" (pointer), "r" (delta)
    : "r11", "cr0", "memory"
  );
  return temp;
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
  long int temp = delta;
  __asm__ (
    "    ori    12,%0,0;"
    "0:	" LPARX " %0,0,%1;"
    "    add    11,%0,12;"
    "	" STPCX " 11,0,%1;"
    "	bne     0b;"
   : "+b" (temp)
   : "p" (pointer)
   : "r11", "r12", "memory"
  );
  return temp;
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
__arch_compare_and_swap (volatile long int *p, long int oldval,
  long int newval)
{
  int ret;

  __asm__ __volatile__ (
    "0:   " LPARX " %0,0,%1 ;"
    "      xor. %0,%3,%0;"
    "      bne 1f;"
    "     " STPCX " %2,0,%1;"
    "      bne- 0b;"
    "1:    isync"
   : "=&r"(ret)
   : "r"(p), "r"(newval), "r"(oldval)
   : "cr0", "memory"
  );

  return ret == 0;
}
#endif

#if GCC_VERSION >= 40700
static inline long int
__arch_atomic_swap (long int *pointer, long int replace)
{
  return __atomic_exchange_n (pointer, replace, __ATOMIC_RELAXED);
}
#else
static inline long int
__arch_atomic_swap (long int *p, long int replace)
{
  long int temp = replace;

  __asm__ (
    "	 ori    12,%0,0;"
    "0:	" LPARX "	%0,0,%1;"
    "	" STPCX "	12,0,%1;"
    "	bne     0b;"
    : "+b" (temp)
    : "p" (p)
    : "r12", "memory"
  );

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
__arch_atomic_inc(long int *p)
{
  long int temp = 0;

  __asm__ __volatile__ (
    "0:	" LPARX " %0,0,%1;"
    "	 addi   %0,%0,1;"
    "	" STPCX " %0,0,%1;"
    "	 bne     0b;"
   : "+b" (temp)
   : "p" (p), "m" (*p)
   : "cr0", "memory"
  );
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
__arch_atomic_dec(long int *p)
{
  long int temp = 0;

  __asm__ __volatile__ (
    "0: " LPARX " %0,0,%1;"
    "    addi   %0,%0,-1;"
    "   " STPCX " %0,0,%1;"
    "    bne    0b;"
   : "+b" (temp)
   : "b" (p), "m" (*p)
   : "cr0", "memory"
  );
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
  unsigned int __tmp;

  /* sas_spin_lock_t is an unsigned int, so lwarx/stwcx. is suffice */
  asm volatile (
       "1:	lwarx	%0,0,%1\n"
       "	cmpwi	0,%0,0\n"
       "	bne-	2f\n"
       "	stwcx.	%2,0,%1\n"
       "	bne-	2f\n"
       "	isync\n"
       "	b	3f\n"
       "2:	lwz	%0,0(%1)\n"
       "	cmpwi	0,%0,0\n"
       "	bne	2b\n"
       "	b	1b\n"
       "3:\n"
       : "=&r" (__tmp)
       : "r" (lock), "r" (1)
       : "cr0", "memory");
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
  int notlocked = 1;
  unsigned int __tmp;

  /* sas_spin_lock_t is an unsigned int, so lwarx/stwcx. is suffice */
  asm volatile (
       "	li	%1,1\n"
       "1:	lwarx	%0,0,%2\n"
       "	cmpwi	0,%0,0\n"
       "	bne-	2f\n"
       "	stwcx.	%1,0,%2\n"
       "	bne-	1b\n"
       "	isync\n"
       "	li	%1,0\n"
       "2:	\n"
       : "=&r" (__tmp), "=&r" (notlocked)
       : "r" (lock)
       : "cr0", "memory");

  return notlocked;
}
#endif

#endif // _SASATOMIC_POWERPC_H
