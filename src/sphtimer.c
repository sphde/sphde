/*
 * Copyright (c) 2010, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include "sphtimer.h"
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

sphtimer_t sph_cpu_frequency = 0;

#ifdef __powerpc__
static sphtimer_t
get_cpu_freq_internal (void)
{
  sphtimer_t result = 0L;
  char buf[1024];
  ssize_t cnt;
  ssize_t hcnt = sizeof (buf) / 2;
  int fd;
  /* Is this annoying! There are functions inside glibc that do this and
     the PPC VDSO also provide direct access to the value we need, but
     they are hidden (not plublic) and we can't get to either. So we
     are forced to read /proc/cpuinfo. */
  fd = open ("/proc/cpuinfo", O_RDONLY);
  if (fd != -1)
    {
      /* Multi-core processors are getting scary big and this means that
         /prop/cpuinfo can also get very large. The cpuinfo for a full
         blown P7 795 is over 90KB. The bad news is the info we are after
         is at the end of the file (following the enumeration of the
         processors). We we will double buffer 512 byte chunks until we
         get to the end of the file. */
      cnt = read (fd, buf, sizeof (buf));

      if (cnt == sizeof (buf))
	{
	  /* did not get lucky, reading the whole cpuinfo in the 1st IO. */
	  do
	    {
	      /* move the bottom half up */
	      memcpy (buf, buf + hcnt, hcnt);
	      /* read the next block */
	      cnt = read (fd, buf + hcnt, hcnt);
	    }
	  while (cnt == hcnt);

	  cnt += hcnt;
	}
      if (cnt > 0)
	{
	  char *tb_str = (char *) memmem (buf, cnt, "timebase", 7);
	  char *bufmax = buf + cnt;

	  if (tb_str != NULL)
	    {
	      while (tb_str < bufmax)
		{
		  if (*tb_str >= '0' && (*tb_str <= '9' || *tb_str == '\n'))
		    break;
		  else
		    tb_str++;
		}

	      if (tb_str < bufmax)
		result = strtoll (tb_str, NULL, 10);
	    }
	}
    }

  return result;
}
#elif defined(__x86_64_INVARIANT_TSC) || defined(__x86_INVARIANT_TSC)
/* TODO: the accuracy of TSC might be susceptible to CPU clock throttling
 * (mainly due to CPU scaling). So we disable rdtsc and related support
 * unless the user explicitly defines __x86_INVARIANT_TSC or
 * __x86_64_INVARIANT_TSC.
 */
static sphtimer_t
get_cpu_freq_internal (void)
{
  sphtimer_t result = 0L;
  char buf[1024];
  ssize_t cnt;
  int fd;
  /* Is this annoying! There are functions inside glibc that do this,
     but they are hidden (not plublic) and we can't get to them.
     So we are forced to read /proc/cpuinfo. */
  fd = open ("/proc/cpuinfo", O_RDONLY);
  if (fd != -1)
    {
      /* For x86/x86_64 cpuinfo repeats "cpu MHz" for each cpu listed.
         So we can assume that the 1st cpu will be in the 1st buffer. */
      cnt = read (fd, buf, sizeof (buf));
      if (cnt > 0)
	{
	  char *tb_str = (char *) memmem (buf, cnt, "MHz", 3);
	  char *bufmax = buf + cnt;
	  double mhz = 0.0;

	  if (tb_str != NULL)
	    {
	      while (tb_str < bufmax)
		{
		  if (*tb_str >= '0' && (*tb_str <= '9' || *tb_str == '\n'))
		    break;
		  else
		    tb_str++;
		}

	      if (tb_str < bufmax)
		mhz = strtod (tb_str, NULL);
	      result = (sphtimer_t) (mhz * 1000000.0);
	    }
	}
    }

  return result;
}
#else
static sphtimer_t
get_cpu_freq_internal (void)
{
	/* default "resolution for clock_gettime(CLOCK_MONOTONIC).  */
	  return 1000000000UL;
}
#endif

sphtimer_t
sphgetcpufreq (void)
{
  sphtimer_t result = sph_cpu_frequency;

  if (result == 0)
    {
      sph_cpu_frequency = get_cpu_freq_internal ();
      result = sph_cpu_frequency;
    }
  return result;
}
