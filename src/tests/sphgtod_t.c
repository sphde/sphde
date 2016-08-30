/*
 * Copyright (c) 2013 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Ryan S. Arnold - initial implementation
 */

#include "sphgtod.h"
#include <sys/time.h>
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
# define ALLOWABLE_DRIFT 13000ULL
#endif
#define COUNT 10000000

int
main (int argc, char *argv[])
{
  struct timeval tv1, tv2, tv3, tv4;
  struct timespec ts1, ts2;
  uint64_t diff;
  double gtod_cost;
  sphtimer_t factor, timestamp;
  int i;
  uint64_t gtod_ns, sphgtod_ns = 0;

  /* start timing the loop.  Doing this COUNT times will smooth jitter and
     amortize the clock_gettime expense across COUNT iterations. */

  sphgtod(&tv2, NULL);
  gettimeofday (&tv3, NULL);

  printf ("starting sphgtod value is      %"PRIu64" nanoseconds\n",
    (uint64_t)tv2.tv_sec * NS_P_S + (uint64_t) tv2.tv_usec * NS_P_uS);

  printf ("starting gettimeofday value is %"PRIu64" nanoseconds\n",
    (uint64_t)tv3.tv_sec * NS_P_S + (uint64_t) tv3.tv_usec * NS_P_uS);

  clock_gettime(CLOCK_MONOTONIC, &ts1);
  for(i=0; i < COUNT; i++)
        sphgtod(&tv1, NULL);
  clock_gettime(CLOCK_MONOTONIC, &ts2);

  /* calculate the delta */
  ts2.tv_sec -= ts1.tv_sec;
  ts2.tv_nsec -= ts1.tv_nsec;
  diff = (uint64_t) ts2.tv_sec * NS_P_S
                + (uint64_t) ts2.tv_nsec;

  gtod_cost = (double)diff/(COUNT+1);
  printf ("sphgtod takes %f nanoseconds\n", gtod_cost);

  sphgtod(&tv2, NULL);
  gettimeofday (&tv3, NULL);

  sphgtod_ns = (uint64_t)tv2.tv_sec * NS_P_S + (uint64_t) tv2.tv_usec * NS_P_uS;
  printf ("Ending sphgtod value is        %"PRIu64" nanoseconds\n", sphgtod_ns);

  gtod_ns = (uint64_t)tv3.tv_sec * NS_P_S + (uint64_t) tv3.tv_usec * NS_P_uS;
  printf ("Ending gettimeofday value is   %"PRIu64" nanoseconds\n", gtod_ns);

  if (llabs(gtod_ns - sphgtod_ns) > ALLOWABLE_DRIFT)
    {
      printf ("The difference between gettimeofday and sphgtod (%llins)\n",
	llabs(gtod_ns - sphgtod_ns));
      printf ("exceeded the allowable drift of %"PRIu64" ns\n",
	ALLOWABLE_DRIFT);
      return 1;
    }

 factor = sphget_gtod_conv_factor();
 timestamp = sphgettimer();
 gettimeofday (&tv3, NULL);
 sphtb2gtod_withfactor (&tv4,timestamp, factor);

 sphgtod_ns = (uint64_t)tv4.tv_sec * NS_P_S + (uint64_t) tv4.tv_usec * NS_P_uS;
 printf ("sphtb2gtod_withfactor value is %"PRIu64" nanoseconds\n", sphgtod_ns);

 gtod_ns = (uint64_t)tv3.tv_sec * NS_P_S + (uint64_t) tv3.tv_usec * NS_P_uS;
 printf ("gettimeofday value is          %"PRIu64" nanoseconds\n", gtod_ns);

 if (llabs(gtod_ns - sphgtod_ns) > ALLOWABLE_DRIFT)
   {
     printf ("The difference between gettimeofday and sphtb2gtod_withfactor (%llins)\n",
	      llabs(gtod_ns - sphgtod_ns));
     printf ("exceeded the allowable drift of %"PRIu64" ns\n",
	      ALLOWABLE_DRIFT);
     return 1;
   }

 return 0;
}
