/*
 * Copyright (c) 2010-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - bugfixes and documentation
 */

#ifndef __SPH_TIMER_H
#define __SPH_TIMER_H

/*!
 * \file  sphtimer.h
 * \brief Functions to access the Time Base register (PPC) or
 * clock_gettime(CLOCK_MONOTONIC) measure time at high resolution.
 * The POWER timebase is safe to use because it is; separate from CPU
 * clock, invariant, and synchronized across cores and sockets.
 *
 * The Intel (x86/x86_64) Time Stamp Counter is not invariant except on
 * the latest processors. As we want to support earlier processors back
 * to the initial Core2 design, we use clock_gettime (CLOCK_MONOTONIC)
 * by default for Intel. On modern Linux kernels, clock_gettime is
 * implemented as a VDSO function which provides reasonable performance.
 *
 * Where applications know they will be running on Intel processors with
 * invariant TSC they can enable inlined rdtsc timers by
 * defining __x86_64_INVARIANT_TSC and/or __x86_INVARIANT_TSC and
 * rebuilding libsphde.
 * 
 * The TB/CLOCK_MONOTONIC value can be obtained using the function
 * ::sphgettimer and the update frequency by using the functions
 * ::sphgetcpufreq or preferably sphfastcpufreq.
 *
 * For instance, to measure the time spend in some computation:
 *
 * \code
 *
 * sphtimer_t before, after;
 * sphtimer_t timer_freg;
 * double seconds;
 * 
 * timer_freg = sphfastcpufreq ();
 * before = sphgettimer ();
 * // computation
 * after = sphgettimer ();
 * seconds = (double) (after - before) / (double) timer_freg;
 * printf ("sec spent: %lf\n", seconds);
 *
 * \endcode
 */

/** \brief ignore this macro behind the curtain. **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Value from TB/TSC register (64-bits on all platforms).
 */
typedef unsigned long long int sphtimer_t;

#if defined(__powerpc64__)
#define __TIME_BASE(__time_v)				\
  do {							\
    sphtimer_t __t;					\
    __asm__ __volatile__ (				\
      "mfspr %0,268"					\
      : "=r" (__t));					\
    __time_v = __t;					\
  } while (0)
#elif defined(__powerpc__)
#define __TIME_BASE(__time_v)				\
  do {							\
    unsigned long __tbu, __tbl, __tmp;			\
    __asm__ volatile (					\
      "0:\n"						\
      "mftbu %0\n"					\
      "mftbl %1\n"					\
      "mftbu %2\n"					\
      "cmpw %0, %2\n"					\
      "bne- 0b"						\
      : "=r" (__tbu), "=r" (__tbl), "=r" (__tmp) );	\
    __time_v = (( (sphtimer_t) __tbu << 32) | __tbl);	\
  } while (0) 
#elif defined(__x86_64__) && defined(__x86_64_INVARIANT_TSC)
/* TODO: the accuracy might be susceptible to CPU clock throttling (mainly
 * due to CPU scaling. So we disable rdtsc unless the user explicitly defines
 * __x86_64_INVARIANT_TSC.
 */
#define __TIME_BASE(__time_v)				\
  do {							\
    unsigned int __t_hi, __t_lo;			\
    __asm__ __volatile__ (				\
      "rdtsc"						\
      : "=a" (__t_lo),					\
      "=d" (__t_hi));					\
    __time_v = ((sphtimer_t) __t_hi << 32) | __t_lo;	\
  } while (0)
#elif defined(__i386__) && defined(__x86_INVARIANT_TSC)
/* TODO: the accuracy might be susceptible to CPU clock throttling (mainly
 * due to CPU scaling. So we disable rdtsc unless the user explicitly defines
 * __x86_INVARIANT_TSC.
 */
#define __TIME_BASE(__time_v)				\
  do {							\
    unsigned int __t;					\
    __asm__ __volatile__ (				\
      "rdtsc"						\
      : "=A" (__t));					\
    __time_v = __t;					\
  } while (0)
#else
#include <time.h>
/** \brief Read the Time Base value. **/
#define __TIME_BASE(__time_v)				\
  do {							\
    struct timespec __ts;					\
    sphtimer_t __t;					\
    clock_gettime (CLOCK_MONOTONIC, &__ts);	\
    __t = ((sphtimer_t)__ts.tv_sec * 1000000000L)	\
	+ (sphtimer_t)__ts.tv_nsec; \
    __time_v = __t;					\
  } while (0)

#endif
    
/*!
 * \brief Read and return the Timebase value.
 *
 * @return The Timebase value.
 */
static inline sphtimer_t
sphgettimer (void)
{
  sphtimer_t t;
  __TIME_BASE (t);
  return t;
}

/*!
 * \brief Frequency which Timebase is updated by system.
 * \warning  This variable should not be used directly, the function
 * ::sphfastcpufreq is fastest since it uses the cached value after
 * the first call.
 */
extern sphtimer_t sph_cpu_frequency;

/*!
 * \brief Return the Timebase update frequency.
 *
 * Reads the Timebase frequency and updates ::sph_cpu_frequency.
 *
 * @return The Timebase update frequency update.
 */
extern __C__ sphtimer_t
sphgetcpufreq (void);

/*!
 * \brief Return the Timebase update frequency (fast version).
 *
 * Will not read the Timebase frequency if the value was already cached
 * (the function ::sphgetcpufreq forces an read/update of
 * sph_cpu_frequency).
 *
 * @return The Timebase frequency value.
 */
static inline sphtimer_t
sphfastcpufreq (void)
{
  sphtimer_t result = sph_cpu_frequency;

  if (__builtin_expect ((result == 0), 0))
    {
      result = sphgetcpufreq ();
    }
  return result;
}

#endif /* __SPH_TIMER_H */
