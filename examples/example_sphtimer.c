/*
 * Copyright (c) 2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 */

#include <stdio.h>
#include <time.h>

#include <sphde/sphtimer.h>

/*
 * This is example showing the sphtimer.h usage and machine timer resolution.
 */

int
main (int argc, char *argv[])
{
  sphtimer_t timer_freg = sphfastcpufreq ();
  sphtimer_t timer_val = sphgettimer ();
  sphtimer_t before, after;
  struct timespec t100ms = { 0, 100000000L };
  struct timespec t1000ms = { 1, 0 };
  double seconds;

  printf ("Time Base Frequency = %lld\nTime Base value = %lld\n\n",
    timer_freg, timer_val);

  /* warm up PLT */
  nanosleep (&t100ms, NULL);

  printf ("nanosleep  100ms - ");
  before = sphgettimer ();
  nanosleep (&t100ms, NULL);
  after = sphgettimer ();

  seconds = (double) (after - before) / (double) timer_freg;
  printf ("delta=%012lld %f sec\n", (after - before), seconds);

  printf ("nanosleep 1000ms - ");
  before = sphgettimer ();
  nanosleep (&t1000ms, NULL);
  after = sphgettimer ();

  seconds = (double) (after - before) / (double) timer_freg;
  printf ("delta=%012lld %f sec\n", (after - before), seconds);

  return 0;
}
