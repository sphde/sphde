/*
 * Copyright (c) 2007, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "sastype.h"
#include "sassim.h"
#include "sphthread.h"
#include "sphtimer.h"

static int thread_rc = 0;
static pid_t main_pid;
static pid_t main_tid;

#define N       4

static void *
tt1 (void *arg)
{
  pid_t th_pid, th_tid;
  char number[160];

  th_pid = sphdeGetPID ();
  th_tid = sphdeGetTID ();

  sprintf (number, "tt1(%ld): begin[%d,%d]", (long) arg, th_pid, th_tid);
  puts (number);

  if (th_pid != main_pid)
    {
      sprintf (number, "tt1(%ld): sphdeGetPID=%d should be %d",
	       (long) arg, th_pid, main_pid);
      puts (number);
      (void)__sync_fetch_and_add (&thread_rc, (int) 1);
    }

  if (th_tid == main_tid)
    {
      sprintf (number, "tt1(%ld): sphdeGetTID=%d should be %d",
	       (long) arg, th_tid, main_tid);
      puts (number);
      (void)__sync_fetch_and_add (&thread_rc, (int) 1);
    }

  th_pid = sphFastGetPID ();
  th_tid = sphFastGetTID ();

  if (th_pid != main_pid)
    {
      sprintf (number, "tt1(%ld): sphdeGetPID=%d should be %d",
	       (long) arg, th_pid, main_pid);
      puts (number);
      (void)__sync_fetch_and_add (&thread_rc, (int) 1);
    }

  if (th_tid == main_tid)
    {
      sprintf (number, "tt1(%ld): sphdeGetTID=%d should be %d",
	       (long) arg, th_tid, main_tid);
      puts (number);
      (void)__sync_fetch_and_add (&thread_rc, (int) 1);
    }


  sprintf (number, "tt1(%ld): end", (long) arg);
  puts (number);
  return NULL;
}

int
thread_thread_test (void)
{
  int rc = 0;
  int n;
  pthread_t th[N];

  thread_rc = 0;

  for (n = 0; n < N; ++n)
    if (pthread_create (&th[n], NULL, tt1, (void *) (long int) n) != 0)
      {
	puts ("thread_thread_test create failed");
	rc += 10;
	return rc;
      }

  puts ("thread_thread_test after create");

  for (n = 0; n < N; ++n)
    if (pthread_join (th[n], NULL) != 0)
      {
	puts ("thread_thread_test join failed");
	rc += 10;
	return rc;
      }

  puts ("thread_thread_test after join");

  return rc + thread_rc;
}

int
thread_basic_test (const char *argv0)
{
  int rc = 0;
  pid_t pid, pid2, pid3;
  pid_t tid, tid2, tid3;
  char *cl;

  pid = sphdeGetPID ();
  tid = sphdeGetTID ();

  if ((pid != 0) && (tid != 0) && (pid == tid))
    {
      printf ("thread_basic_test PID=%d, TID=%d\n", pid, tid);
    }
  else
    {
      printf ("thread_basic_test failed sphdeGetPID=%d, sphdeGetTID=%d\n",
	      pid, tid);
      rc++;
    }

  cl = sphdeGetCmdLine ();
  if ((cl != NULL) && (*cl != 0))
    {
      printf ("thread_basic_test sphdeGetCmdLine=%s\n", cl);
      if (0 != strcmp (cl, argv0))
	{
	  printf
	    ("thread_basic_test failed sphdeGetCmdLine=%s != ./sphthread_t\n",
	     cl);
	  rc++;
	}
    }
  else
    {
      printf ("thread_basic_test failed sphdeGetCmdLine=%s\n", cl);
      rc++;
    }

  pid2 = sphdeGetPID ();
  tid2 = sphdeGetTID ();

  if ((pid2 != 0) && (tid2 != 0) && (pid2 == tid2))
    {
      printf ("thread_basic_test PID=%d, TID=%d\n", pid2, tid2);
      if ((pid != pid2) || (tid != tid2))
	{
	  printf ("thread_basic_test failed PID %d != %d, TID %d != %d\n",
		  pid, pid2, tid, tid2);
	}
    }
  else
    {
      printf ("thread_basic_test failed sphdeGetPID=%d, sphdeGetTID=%d\n",
	      pid2, tid2);
      rc++;
    }

  pid3 = sphFastGetPID ();
  tid3 = sphFastGetTID ();

  if ((pid3 != 0) && (tid3 != 0) && (pid3 == tid3))
    {
      printf ("thread_basic_test fast PID=%d, TID=%d\n", pid3, tid3);
      if ((pid != pid3) || (tid != tid3))
	{
	  printf
	    ("thread_basic_test fast failed PID %d != %d, TID %d != %d\n",
	     pid, pid3, tid, tid3);
	}
    }
  else
    {
      printf ("thread_basic_test failed sphFastGetPID=%d, sphFastGetTID=%d\n",
	      pid3, tid3);
      rc++;
    }

  return rc;
}

int
timer_basic_test (void)
{
  int rc = 0;

  sphtimer_t timer_freg = sphfastcpufreq ();
  sphtimer_t timer_val = sphgettimer ();
  sphtimer_t before, after;
  struct timespec t100ms = { 0, 100000000L };
  struct timespec t1000ms = { 1, 0 };
  double seconds;

  printf ("timer_basic_test Freq=%lld, timer=%lld\n", timer_freg, timer_val);
  /* warm up PLT */
  nanosleep (&t100ms, NULL);

  before = sphgettimer ();
  nanosleep (&t100ms, NULL);
  after = sphgettimer ();

  seconds = (double) (after - before) / (double) timer_freg;
  if ((seconds < 0.1) && (seconds > 0.2))
    {
      printf ("100ms delta=%lld %f sec\n", (after - before), seconds);
      rc++;
    }

  before = sphgettimer ();
  nanosleep (&t1000ms, NULL);
  after = sphgettimer ();

  seconds = (double) (after - before) / (double) timer_freg;
  if ((seconds < 1.0) && (seconds > 1.2))
    {
      printf ("1000ms delta=%lld %f sec\n", (after - before), seconds);
      rc++;
    }

  return rc;
}

int
timed_basic_test (void)
{
  int rc = 0;

  sphtimer_t timer_freg = sphfastcpufreq ();
  sphtimer_t timer_val = sphgettimer ();
  sphtimer_t before, after;
  double seconds;
  int i, max_cnt;

  max_cnt = 1000000;

  printf ("timed_basic_test Freq=%lld, timer=%lld\n", timer_freg, timer_val);
  before = sphgettimer ();

  for (i = 0; i < max_cnt; i++)
    {
      sphdeGetPID ();
      sphdeGetTID ();
      sphgettimer ();
    }
  after = sphgettimer ();

  seconds = (double) (after - before) / (double) timer_freg;
  printf ("timed_basic_test %d iterations %f seconds\n", max_cnt, seconds);

  return rc;
}

int
main (int argc, char *argv[])
{
  int rc = 0;

  rc += thread_basic_test (argv[0]);

  main_pid = sphdeGetPID ();
  main_tid = sphdeGetTID ();

  rc += thread_thread_test ();

  rc += timer_basic_test ();

  rc += timed_basic_test ();

  return rc;
}
