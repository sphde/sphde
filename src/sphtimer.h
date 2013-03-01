
#ifndef __SPH_TIMER_H
#define __SPH_TIMER_H

/*
 * Copyright (c) 2010, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - bugfixes and documentation
 */

/*!
 * \file  sphtimer.h
 * \brief Functions to access the Time Base (PPC) or Time Stamp Counter
 * (Intel) register to measure time at nanossecond level.
 *
 * The Time Base (TB) and Time Stamp Counter (TSC) are both 64-bits special
 * registers that are incremented periodically with an 
 * implementation-dependent frequency (not guaranteed to be constant). 
 * 
 * The TB/TSC value can be obtained using the function ::sphgettimer and the
 * update frequency by using the functions ::sphgetcpufreq or preferably 
 * sphfastcpufreq.
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
#elif defined(__x86_64__)
/* TODO: the accuracy might be susceptible to CPU clock throttling (mainly
 * due CPU scaling. Find a better way to get the time base by maybe using
 * HPET (although its interface uses syscall on Linux). */
#define __TIME_BASE(__time_v)				\
  do {							\
    unsigned int __t_hi, __t_lo;			\
    __asm__ __volatile__ (				\
      "rdtsc"						\
      : "=a" (__t_lo),					\
      "=d" (__t_hi));					\
    __time_v = ((sphtimer_t) __t_hi << 32) | __t_lo;	\
  } while (0)
#elif defined(__i386__)
#define __TIME_BASE(__time_v)				\
  do {							\
    unsigned int __t;					\
    __asm__ __volatile__ (				\
      "rdtsc"						\
      : "=A" (__t));					\
    __time_v = __t;					\
  } while (0)
#endif
    
/*!
 * \brief Read and return the value of TB/TSC register.
 *
 * @return The TB/TSC value.
 */
static inline sphtimer_t
sphgettimer (void)
{
  sphtimer_t t;
  __TIME_BASE (t);
  return t;
}

/*!
 * \brief Frequency which TB/TSC is updated by system.
 * \warning  This variable should not be used directly, the function
 * ::sphfastcpufreq is the correct way since it issue a TB/TSC update on
 * first access.
 */
extern sphtimer_t sph_cpu_frequency;

/*!
 * \brief Return the TB/TSC frequency update.
 *
 * Reads the TB/TSC value from system and update the ::sph_cpu_frequency.
 *
 * @return The TB/TSC frequency update value.
 */
extern __C__ sphtimer_t
sphgetcpufreq (void);

/*!
 * \brief Return the TB/TSC frequency update (fast version).
 *
 * Do not update the TB/TSC if value was already read (the function
 * ::sphgetcpufreq forces an update).
 *
 * @return The TB/TSC frequency update value.
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
