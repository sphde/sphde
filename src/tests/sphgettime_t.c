/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Paul Clarke - initial implementation (from sphgtod_t.c)
 */

#include "sphgettime.h"
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#if __WORDSIZE == 64
# define NS_P_S 1000000000UL
# define NS_P_uS 1000UL
# define ALLOWABLE_DRIFT 9000UL
#else
# define NS_P_S 1000000000ULL
# define NS_P_uS 1000ULL
# define ALLOWABLE_DRIFT 9500ULL
#endif
#define COUNT 10000000

int
main (int argc, char *argv[])
{
  struct timespec tp1, tp2, tp3, tp4;
  struct timespec ts1, ts2;
  uint64_t diff;
  double gettime_cost;
  sphtimer_t factor, timestamp;
  int i;
  uint64_t gettime_ns, sphgettime_ns = 0;

  /* start timing the loop.  Doing this COUNT times will smooth jitter and
     amortize the clock_gettime expense across COUNT iterations. */

  sphgettime(&tp2);
  clock_gettime (CLOCK_MONOTONIC, &tp3);

  printf ("starting sphgettime value is      %"PRIu64" nanoseconds\n",
    (uint64_t)tp2.tv_sec * NS_P_S + (uint64_t) tp2.tv_nsec);

  printf ("starting clock_gettime value is %"PRIu64" nanoseconds\n",
    (uint64_t)tp3.tv_sec * NS_P_S + (uint64_t) tp3.tv_nsec);

  clock_gettime(CLOCK_MONOTONIC, &ts1);
  for(i=0; i < COUNT; i++)
        sphgettime(&tp1);
  clock_gettime(CLOCK_MONOTONIC, &ts2);

  /* calculate the delta */
  ts2.tv_sec -= ts1.tv_sec;
  ts2.tv_nsec -= ts1.tv_nsec;
  diff = (uint64_t) ts2.tv_sec * NS_P_S
                + (uint64_t) ts2.tv_nsec;

  gettime_cost = (double)diff/(COUNT+1);
  printf ("sphgettime takes %f nanoseconds\n", gettime_cost);

  sphgettime(&tp2);
  clock_gettime (CLOCK_MONOTONIC, &tp3);

  sphgettime_ns = (uint64_t)tp2.tv_sec * NS_P_S + (uint64_t) tp2.tv_nsec;
  printf ("Ending sphgettime value is       %"PRIu64" nanoseconds\n", sphgettime_ns);

  gettime_ns = (uint64_t)tp3.tv_sec * NS_P_S + (uint64_t) tp3.tv_nsec;
  printf ("Ending clock_gettime value is    %"PRIu64" nanoseconds\n", gettime_ns);

  if (llabs(gettime_ns - sphgettime_ns) > ALLOWABLE_DRIFT)
    {
      printf ("The difference between clock_gettime and sphgettime (%llins)\n",
	llabs(gettime_ns - sphgettime_ns));
      printf ("exceeded the allowable drift of %"PRIu64" ns\n",
	ALLOWABLE_DRIFT);
      return 1;
    }

 factor = sphget_gettime_conv_factor();
 timestamp = sphgettimer();
 clock_gettime (CLOCK_MONOTONIC, &tp3);
 sphtb2gettime_withfactor (&tp4,timestamp, factor);

 sphgettime_ns = (uint64_t)tp4.tv_sec * NS_P_S + (uint64_t) tp4.tv_nsec;
 printf ("sphtb2gettime_withfactor value is %"PRIu64" nanoseconds\n", sphgettime_ns);

 gettime_ns = (uint64_t)tp3.tv_sec * NS_P_S + (uint64_t) tp3.tv_nsec;
 printf ("gettimeofday value is             %"PRIu64" nanoseconds\n", gettime_ns);

 if (llabs(gettime_ns - sphgettime_ns) > ALLOWABLE_DRIFT)
   {
     printf ("The difference between clock_gettime and sphtb2gettime_withfactor (%llins)\n",
	      llabs(gettime_ns - sphgettime_ns));
     printf ("exceeded the allowable drift of %"PRIu64" ns\n",
	      ALLOWABLE_DRIFT);
     return 1;
   }

 return 0;
}
