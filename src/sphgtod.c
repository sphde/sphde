/*
 * Copyright (c) 2013 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial implementation
 *                      Ryan S. Arnold - Fast computation, framework impl
 */

#include <sys/time.h>
#include <inttypes.h>
#include "sphtimer.h"
#include <stddef.h>

/* Time since epoch in units equivalent to those returned by the timebase
   register.  */
static sphtimer_t tb2gtod;

/* This is cached in libsphde but let's cache it here in the local exec as
   well (since this object file will be statically linked).  */
static sphtimer_t tb_freq;

/* On PowerPC64 (Power7 and later) we can use special instructions to perform
   fast integer mathematics.  */
#if __WORDSIZE == 64
# if !defined _ARCH_PWR7
# include <math.h>
# endif
/* This is a 2^64 scaled fractional representation shifted left 24 bits.
   It is precalculated in the constructor.  */
static uint64_t tb_freq_shifted_recip;
#else
static double tb_freq_recip;
static unsigned long long half_tb_freq;
#endif

/* Microseconds per second.  */
#define uS_P_S 1000000UL

/* Compute the timebase offset from epoch, used in later calculations.  */
static void
sphget_tb2gtod_factor (void)
{
  struct timeval gtod;

  uint64_t tb1;
  uint64_t gtod_tb;
  uint64_t tb2;

  tb_freq = sphfastcpufreq();

  tb1 = sphgettimer ();
  gettimeofday(&gtod, NULL);
#if 1
  tb2 = sphgettimer ();
  tb1 = (tb1 + tb2) / 2UL;
#endif

  gtod_tb = (gtod.tv_sec * tb_freq) + ((gtod.tv_usec * tb_freq) /  uS_P_S);
  tb2gtod = gtod_tb - tb1;
}

int sphgtod (struct timeval *tv, struct timezone *tz)
{
  /* timezone is not used/honored.  */
  uint64_t tb;
  uint64_t tb2;
#if __WORDSIZE == 64
  unsigned long long int frac_secs;
  unsigned long long int tmp1;
  const unsigned long long int usec_conv = uS_P_S;
#else
  uint64_t tb2_sec, tb2_rem, tb2_us;
#endif

#if __WORDSIZE == 64
  __asm__ volatile ("mfspr %0, 268" : "=r" (tb));
#else
  uint32_t __tbu, __tbl, __tmp; \
  __asm__ volatile (
    "0:\n\t"
    "mftbu %0\n\t"
    "mftbl %1\n\t"
    "mftbu %2\n\t"
    "cmpw %0, %2\n\t"
    "bne- 0b"
    : "=r" (__tbu), "=r" (__tbl), "=r" (__tmp));
  tb = (((uint64_t) __tbu << 32) | __tbl);
#endif

  tb2 = tb + tb2gtod;

#if __WORDSIZE == 64
  __asm__ volatile ( \
    "mulld %0, %4, %5\n\t" \
    "mulhdu %1, %4, %5\n\t" \
    "srdi %2, %1, 24\n\t" \
    "insrdi %0, %1, 24, 40\n\t" \
    "rotrdi %3, %0, 24\n\t" \
    "mulhdu %3, %3, %6\n\t" \
    : "=&r" (frac_secs), \
      "=&r" (tmp1), \
      "=&r" (tv->tv_sec), \
      "=&r" (tv->tv_usec) \
    : "r" (tb2), "r" (tb_freq_shifted_recip), "r" (usec_conv) \
    );
#else
  tb2_sec = tb2 / tb_freq;

  /* Number of timebase ticks that are a fraction of a second.  */
  tb2_rem = tb2 % tb_freq;

  /* This needs to be in microseconds, so we scale the remainder by the
     number of micro-seconds in a second (so that dividing by the tb_freq
     actually make sense).  We add half the tb_freq value to get the correct
     rounding for the last fraction of a micro-second when we divide by the
     tb_freq.  */
  tb2_us = ((tb2_rem * uS_P_S) + half_tb_freq) / tb_freq;
  tv->tv_sec = tb2_sec;
  tv->tv_usec = tb2_us;
#endif

  return 0;
}

void __attribute__ ((constructor)) __attribute__ ((visibility ("hidden")))
__sphgtod_init (void)
{
  /* This calculates a close approximation of the delta in timebase resolution
     that when added to the current timebase value gives the epoch in timebase
     resolution. This is used in all emulated gtod calls.  */
  sphget_tb2gtod_factor ();

#if __WORDSIZE == 64
  /* The 'one' used in this calculation is left-shifted 24 bits.  We're using
     it to precalculate 1/tb_freq in 128-bit fractional fixed-point math so
     that we can do reciprocal fixed-point multiplication in place of a divide
     when we do the timebase to gettimeofday computations.

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
      "divdeu %0, %1, %2\n\t" \
      : "=r" (tb_freq_shifted_recip) \
      : "r" (one), "r" (tb_freq) \
      : );
  }
# else
  /* Do it the slow way using long double fp math on systems that precede
     Power7.  */
  {
    long double tmp;
    tmp = powl(2.0L,88.0L) / (long double) tb_freq;
    tb_freq_shifted_recip = (uint64_t) tmp;
  }
# endif
#else /* __WORDSIZE == 32  */
  /* TODO: Take the time to provide a fast method for 32-bit.  */
  tb_freq_recip = (double)1.0 / tb_freq;
  half_tb_freq = tb_freq / 2UL;
#endif
}
