/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Paul Clarke - initial implementation (from sphgtod.c)
 */

//#define __SASDebugPrint__ 1
#ifdef __SASDebugPrint__
#include <stdlib.h>
#include <stdio.h>
#endif

#include <time.h>
#include <inttypes.h>
#include <stdint.h>
#include "sphtimer.h"
#include <stddef.h>

#if __GNUC__ == 4 && __GNUC_MINOR__ >= 6 && __SIZEOF_INT128__ == 16
#define _SPH_PLATFORM_HAS_INT128 1
#else
#define _SPH_PLATFORM_HAS_INT128 0
#endif

/* Time since epoch in units equivalent to those returned by the timebase
   register.  */
static sphtimer_t tb2gettime;

/* This is cached in libsphde but let's cache it here in the local exec as
   well (since this object file will be statically linked).  */
static sphtimer_t tb_freq;

#if defined __powerpc64__ || _SPH_PLATFORM_HAS_INT128
/* On PowerPC64 (Power7 and later) we can use special instructions to perform
   fast integer mathematics.  */
# if !defined _ARCH_PWR7
# include <math.h>
# endif
/* This is a 2^64 scaled fractional representation shifted left 24 bits.
   It is precalculated in the constructor.  */
static sphtimer_t tb_freq_shifted_recip;
#else
static unsigned long long half_tb_freq;
#endif

/* Microseconds per second.  */
#define nS_P_S 1000000000UL

/* Compute the timebase offset from epoch, used in later calculations.  */
static void
sphget_tb2gettime_factor (void)
{
  struct timespec gettime;

  uint64_t tb1;
  uint64_t gettime_tb;
  uint64_t tb2;

  tb_freq = sphfastcpufreq();

  tb1 = sphgettimer ();
  clock_gettime(CLOCK_MONOTONIC, &gettime);

  tb2 = sphgettimer ();
/* compute average while avoiding overflow.  */
  tb1 = tb1 >> 1;
  tb2 = tb2 >> 1;
  tb1 = (tb1 + tb2);

  gettime_tb = (gettime.tv_sec * tb_freq) + ((gettime.tv_nsec * tb_freq) /  nS_P_S);
  tb2gettime = gettime_tb - tb1;
#ifdef __SASDebugPrint__
  printf ("sphget_tb2gettime_factor tb_freq=%llu, %llx\n",
		  tb_freq, tb_freq);
  printf ("sphget_tb2gettime_factor tb1=%lu, %lx\n",
		  tb1, tb1);
  printf ("sphget_tb2gettime_factor tb2=%lu, %lx\n",
		  tb2, tb2);
  printf ("sphget_tb2gettime_factor gettime_tb=%lu, %lx\n",
		  gettime_tb, gettime_tb);
  printf ("sphget_tb2gettime_factor tb2gettime=%llu, %llx\n",
		  tb2gettime, tb2gettime);
#endif
}

sphtimer_t
sphget_gettime_conv_factor (void)
{
	return tb2gettime;
}

int sphtb2gettime_withfactor (struct timespec *tp, sphtimer_t timestamp, sphtimer_t tb2gettime_factor)
{
  /* timezone is not used/honored.  */
  uint64_t tb;
  uint64_t tb2;
#if defined __powerpc64__ || _SPH_PLATFORM_HAS_INT128
  unsigned long long int frac_secs;
  const unsigned long long int nsec_conv = nS_P_S;
#ifdef __powerpc64__
  uint64_t tmp1;
#endif
#else
  uint64_t tb2_sec, tb2_rem, tb2_ns;
#endif

  tb = timestamp;

  tb2 = tb + tb2gettime_factor;

#if  defined __powerpc64__
  __asm__ volatile (
    "mulld %0, %4, %5\n\t"
    "mulhdu %1, %4, %5\n\t"
    "srdi %2, %1, 24\n\t"
    "insrdi %0, %1, 24, 40\n\t"
    "rotrdi %3, %0, 24\n\t"
    "mulhdu %3, %3, %6\n\t"
    : "=&r" (frac_secs), "=&r" (tmp1), "=&r" (tp->tv_sec), "=&r" (tp->tv_nsec)
    : "r" (tb2), "r" (tb_freq_shifted_recip), "r" (nsec_conv));
#elif _SPH_PLATFORM_HAS_INT128
  unsigned __int128 qtemp;

  qtemp = (__int128)tb2 * (__int128)tb_freq_shifted_recip;
  qtemp = qtemp >> 24;
  tp->tv_sec = (unsigned long long int)(qtemp >> 64);
  frac_secs = (unsigned long long int)qtemp;
  qtemp = frac_secs;
  qtemp = qtemp * (unsigned __int128)nsec_conv;
  tp->tv_nsec = (unsigned long long int)(qtemp >> 64);
#else
  tb2_sec = tb2 / tb_freq;

  /* Number of timebase ticks that are a fraction of a second.  */
  tb2_rem = tb2 % tb_freq;

  /* This needs to be in nanoseconds, so we scale the remainder by the
     number of nanoseconds in a second (so that dividing by the tb_freq
     actually make sense).  We add half the tb_freq value to get the correct
     rounding for the last fraction of a nanosecond when we divide by the
     tb_freq.  */
  tb2_ns = ((tb2_rem * nS_P_S) + half_tb_freq) / tb_freq;
  tp->tv_sec = tb2_sec;
  tp->tv_nsec = tb2_ns;
#endif

  return 0;
}

int sphgettime_withfactor (struct timespec *tp, sphtimer_t tb2gettime_factor)
{
  sphtimer_t tb;

  tb = sphgettimer();
  sphtb2gettime_withfactor (tp, tb, tb2gettime_factor);

  return 0;
}

int sphgettime (struct timespec *tp)
{
  /* timezone is not used/honored.  */
  sphtimer_t tb;

  tb = sphgettimer();
  /* -O3 compile should inline this.  */
  sphtb2gettime_withfactor (tp, tb, tb2gettime);
  return 0;
}

void __attribute__ ((constructor)) __attribute__ ((visibility ("hidden")))
__sphgettime_init (void)
{
  /* This calculates a close approximation of the delta in timebase resolution
     that when added to the current timebase value gives the epoch in timebase
     resolution. This is used in all emulated gettime calls.  */
  sphget_tb2gettime_factor ();

#if defined __powerpc64__ || _SPH_PLATFORM_HAS_INT128
  /* The 'one' used in this calculation is left-shifted 24 bits.  We're using
     it to precalculate 1/tb_freq in 128-bit fractional fixed-point math so
     that we can do reciprocal fixed-point multiplication in place of a divide
     when we do the timebase to clock_gettime computations.

     We're relying on the fact that the timebase values, when added to the
     timebase epoch factor and divided by the frequency don't fill out the
     high order bits of the high doubleword used in 128-bit fractional math.

     So we bias '1' with a left shift of 24 bits to give more space for
     fractional second accuracy and then right shift the result at the end to
     properly scale the result.
     */
# if  _ARCH_PWR7
  {
    /* This is '1' left shifted 24 bits.  */
    const unsigned long long int one = 0x1000000ULL;
    __asm__ volatile (
      "divdeu %0, %1, %2\n\t"
      : "=r" (tb_freq_shifted_recip)
      : "r" (one), "r" (tb_freq));
  }
# else
  /* Do it the slow way using long double fp math on systems that precede
     Power7.  */
  {
    long double tmp;
    tmp = (0x1.0p88L) / (long double) tb_freq;
    tb_freq_shifted_recip = (uint64_t) tmp;
  }
# endif
#ifdef __SASDebugPrint__
  printf ("__sphgettime_init tb_freq_shifted_recip=%llu, %llx\n",
		  tb_freq_shifted_recip, tb_freq_shifted_recip);
#endif
#else /* __WORDSIZE == 32  */
#endif
}
