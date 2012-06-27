/*
 * sphsinglepcqueue_ttt.c
 *
 *  Created on: Jun 21, 2012
 *      Author: sjmunroe
 */
/*
 * Copyright (c) 2012 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include "sasshm.h"
#include "sasconf.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "saslock.h"
#include "sphlfentry.h"
#include "sphsinglepcqueue.h"
#include "sphthread.h"
#include "sphtimer.h"

static int N_PROC_CONF = 1;
static int num_threads = 1;
static long thread_iterations;

#define MAX_THREADS 256

static SPHSinglePCQueue_t pcqueue;
static SPHSinglePCQueue_t pcq_list[256];

static block_size_t cap, units, p10, pcq_alloc, pcq_stride;

typedef void *(*test_ptr_t) (void *);
typedef int (*test_fill_ptr_t) (SPHSinglePCQueue_t, int, long);
static test_fill_ptr_t test_funclist[MAX_THREADS];

//#define DEBUG_PRINT 1

int
lfPCQentry_fasttest (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1 = 0;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
  if (!handle)
    {
      while (SPHSinglePCQueueFull (pcqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
    }
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      array[0] = val1;
      array[1] = val2;
      array[2] = val3;
      SPHLFEntryComplete (&handle0);
    }
  else
    {
      rc1 = 1;
    }

  return (rc1);
}

int
lfPCQentry_fastverify (SPHSinglePCQueue_t pcqueue, int val1, int val2,
		       int val3)
{
  int rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
  if (!handle)
    {
      while (SPHSinglePCQueueEmpty (pcqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
    }
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      rc1 = (array[0] != val1);
      rc2 = (array[1] != val2);
      rc3 = (array[2] != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_fastverify:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
	     tv1, tv2, tv3, val1, val2, val3);
	}

      if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	{
	  rc4 = 0;
	}
      else
	{
	  printf
	    ("lfPCQentry_fastverify:: SPHSinglePCQueueFreeNextEntry() = failed\n");
	  rc4 = 1;
	}
    }
  else
    {
      return 10;
    }

  return (rc1 | rc2 | rc3 | rc4);
}

int
lfPCQentry_test (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
  if (!handle)
    {
#ifdef DEBUG_PRINT
      char number[128];
      sprintf (number, "lfPCQentry_test(%p, %d) SPHSinglePCQueueFull\n",
	       pcqueue, val1);
      puts (number);
#endif
      while (SPHSinglePCQueueFull (pcqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
#ifdef DEBUG_PRINT
      sprintf (number, "lfPCQentry_test(%p, %d) retry handle=%p\n", pcqueue,
	       val1, handle);
      puts (number);
#endif
    }
  if (handle)
    {
      rc1 = SPHLFEntryAddInt (&handle0, val1);
      rc2 = SPHLFEntryAddInt (&handle0, val2);
      rc3 = SPHLFEntryAddInt (&handle0, val3);
      SPHSinglePCQueueEntryComplete (&handle0);
    }
  else
    {
      return -1;
    }

  return (rc1 | rc2 | rc3);
}

int
lfPCQentry_verify (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
  if (!handle)
    {
#ifdef DEBUG_PRINT
      char number[128];
      sprintf (number, "lfPCQentry_verify(%p, %d) SPHSinglePCQueueEmpty\n",
	       pcqueue, val1);
      puts (number);
#endif
      while (SPHSinglePCQueueEmpty (pcqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
#ifdef DEBUG_PRINT
      sprintf (number, "lfPCQentry_verify(%p, %d) retry handle=%p\n", pcqueue,
	       val1, handle);
      puts (number);
#endif
    }
  if (handle)
    {
      tv1 = SPHLFEntryGetNextInt (&handle0);
      tv2 = SPHLFEntryGetNextInt (&handle0);
      tv3 = SPHLFEntryGetNextInt (&handle0);
      rc1 = (tv1 != val1);
      rc2 = (tv2 != val2);
      rc3 = (tv3 != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_verify:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
	     tv1, tv2, tv3, val1, val2, val3);
	}

      if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	{
	  rc4 = 0;
	}
      else
	{
	  printf
	    ("lfPCQentry_verify:: SPHSinglePCQueueFreeNextEntry() = failed\n");
	  rc4 = 1;
	}
    }
  else
    {
      return 10;
    }

  return (rc1 | rc2 | rc3 | rc4);
}

int
test_thread_Producer_fill (SPHSinglePCQueue_t pcqueue,
			   int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_Producer_fill(%p, %d, %ld)\n", pcqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_test (pcqueue, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_Producer_fill SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
	     pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_consumer_verify (SPHSinglePCQueue_t pcqueue,
			     int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_consumer_verify(%p, %d, %ld)\n", pcqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_verify (pcqueue, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_consumer_verify SPHSinglePCQueueGetNextComplete (%p) failed\n",
	     pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_Producer_fastfill (SPHSinglePCQueue_t pcqueue,
			       int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_Producer_fill(%p, %d, %ld)\n", pcqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_fasttest (pcqueue, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_Producer_fill SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
	     pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_consumer_fastverify (SPHSinglePCQueue_t pcqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_consumer_verify(%p, %d, %ld)\n", pcqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_fastverify (pcqueue, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_consumer_verify SPHSinglePCQueueGetNextComplete (%p) failed\n",
	     pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

static void *
fill_test_parallel_thread (void *arg)
{
  SPHSinglePCQueue_t pcqueue;
  test_fill_ptr_t test_func;
  long result = 0;
  int tn = (int) (long int) arg;

  pcqueue = pcq_list[tn];
  test_func = test_funclist[tn];

  SASThreadSetUp ();
#ifdef DEBUG_PRINT
  char number[128];
  pid_t tid = sphFastGetTID ();
  sprintf (number, "ltt(%d, %d, @%p): begin", tn, tid, pcqueue);
  puts (number);
#endif

  result += (*test_func) (pcqueue, tn, thread_iterations);

#ifdef DEBUG_PRINT
  sprintf (number, "ltt(%d, %d): end", tn, tid);
  puts (number);
#endif
  SASThreadCleanUp ();

  return (void *) result;
}

static int
launch_test_threads (int t_cnt, test_ptr_t test_f, int iterations)
{
  long int n;
  pthread_t th[MAX_THREADS];
  int tid;
  int pid;
  long thread_result;
  int result = 0;

  pid = getpid ();
  tid = sphdeGetTID ();
  thread_iterations = iterations / t_cnt;
#ifdef DEBUG_PRINT
  printf ("creating thread from pid/tid = %d/%d\n", pid, tid);
#endif

  for (n = 0; n < t_cnt; ++n)
    {
      void *arg;
      arg = (void *) n;
      if (pthread_create (&th[n], NULL, test_f, arg) != 0)
	{
	  puts ("create failed");
	  exit (1);
	}
    }
#ifdef DEBUG_PRINT
  puts ("after create");
#endif

  for (n = 0; n < t_cnt; ++n)
    if (pthread_join (th[n], (void **) &thread_result) != 0)
      {
	puts ("join failed");
	exit (2);
      }
    else
      {
	result += thread_result;
      }

#ifdef DEBUG_PRINT
  puts ("after join");
#endif
  return result;
}

int
test_pcq_init (block_size_t pcq_size)
{
  int rc = 0;
  pcq_alloc = pcq_size;
  pcq_stride = 128;

  pcqueue = SPHSinglePCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue)
    {
      rc = SPHSinglePCQueueSetCachePrefetch (pcqueue, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);

      cap = SPHSinglePCQueueFreeSpace (pcqueue);

      units = cap / 128;

      printf ("SPHSinglePCQueueFreeSpace() = %zu units=%zu\n", cap, units);
    }
  else
    rc = 10;

  return rc;
}

int
test_pcq_reset ()
{
  int rc = 0;

  rc = SPHSinglePCQueueResetAsync (pcqueue);
  if (!rc)
    {
      printf ("\nSPHSinglePCQueueResetAsync (%p) success \n", pcqueue);

      cap = SPHSinglePCQueueFreeSpace (pcqueue);

      units = cap / 128;

      printf ("SPHSinglePCQueueFreeSpace() = %zu units=%zu\n", cap, units);
    }
  else
    rc = 10;

  return rc;
}

#if 0
int
test_thread_wraplog_verify (SPHSinglePCQueue_t pcqueue)
{
  int tempn_i, temp0_i, temp1_i;
  unsigned int temp2_i;
  SPHLFEntryHandle_t *handlex, handle5;
  sphpid16_t entry_pid, entry_tid;
  SPHLFLogIterator_t *iter, iter0;
  int *tarray;
  sphtimer_t entry_timestamp, prev_timestamp;
  int rtn = 0;
  long i;

  int t_PID[MAX_THREADS];
  int t_TID[MAX_THREADS];
  int t_seq[MAX_THREADS];
  sphtimer_t t_tts[MAX_THREADS];

#ifdef SPH_TIMERTEST_VERIFY
  for (i = 0; i < num_threads; i++)
    {
      t_PID[i] = -1;
      t_TID[i] = -1;
      t_seq[i] = -1;
      t_tts[i] = 0LL;
    }

  printf ("\ntest_thread_wraplog_verify() verify log contents\n");
  iter = SPHLFpcqueueCreateIterator (pcqueue, &iter0);
  if (iter)
    {
      printf ("  SPHLFpcqueueCreateIterator(%p,%p) = %p succeeded\n",
	      pcqueue, &iter0, iter);
    }
  else
    {
      printf ("  SPHLFpcqueueCreateIterator(%p,%p) = %p failed\n",
	      pcqueue, &iter0, iter);
      return (rtn + 10);
    }

  prev_timestamp = 0LL;
  do
    {
      handlex = SPHLFpcqueueIteratorNext (iter, &handle5);
      if (handlex)
	{
	  entry_pid = SPHLFLogEntryPID (handlex);
	  entry_tid = SPHLFLogEntryTID (handlex);
	  entry_timestamp = SPHLFLogEntryTimeStamp (handlex);

	  tarray = (int *) SPHLFLogEntryGetFreePtr (handlex);
	  temp0_i = tarray[0];
	  temp1_i = tarray[1];
	  temp2_i = tarray[2];
	  if ((temp1_i < num_threads) && (temp2_i == 0xdeadbeef))
	    {
	      tempn_i = t_seq[temp1_i];
	      if (tempn_i == -1)
		{
		  t_PID[temp1_i] = entry_pid;
		  t_TID[temp1_i] = entry_tid;
		  t_seq[temp1_i] = temp0_i;
		  t_tts[temp1_i] = entry_timestamp;
		}
	      else
		{
		  if ((temp0_i != (tempn_i + 1))
		      || (t_PID[temp1_i] != entry_pid)
		      || (t_TID[temp1_i] != entry_tid)
		      || (entry_timestamp < t_tts[temp1_i]))
		    {
		      printf
			("  SPHLFpcqueueIteratorNext() seq mismatch  %d,%d,%x, %llu,%llu\n",
			 temp0_i, temp1_i, temp2_i, t_tts[temp1_i],
			 entry_timestamp);
		      rtn++;
		    }
		  t_seq[temp1_i] = temp0_i;
		  t_tts[temp1_i] = entry_timestamp;
		}
	    }
	  else
	    {
	      printf
		("  SPHLFpcqueueIteratorNext() data mismatch found %d,%x,%x\n",
		 temp0_i, temp1_i, temp2_i);
	      rtn++;
	    }
	  prev_timestamp = entry_timestamp;
	}
    }
  while (handlex);

  printf ("test_thread_wraplog_verify() verify log complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */

  return rtn;
}

int
test_thread_wraplog_fill (SPHSinglePCQueue_t pcqueue,
			  int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;

  for (i = 0; i < iterations; i++)
    {
      rc = lflogentry_stridetest (pcqueue, i, thread_ID, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf ("test_thread_wraplog_fill thread=%d, iteration=%ld\n",
		  thread_ID, i);
	  printf ("SPHLFpcqueueAllocStrideTimeStamped (%p) failed\n",
		  pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_wraplog_fill_strong (SPHSinglePCQueue_t pcqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;

  for (i = 0; i < iterations; i++)
    {
      rc = lflogentry_stridetest_strong (pcqueue, i, thread_ID, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_wraplog_fill_strong thread=%d, iteration=%ld\n",
	     thread_ID, i);
	  printf ("SPHLFpcqueueAllocStrideTimeStamped (%p) failed\n",
		  pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_wraplog_fill_nolock (SPHSinglePCQueue_t pcqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;

  for (i = 0; i < iterations; i++)
    {
      rc = lflogentry_stridetest_nolock (pcqueue, i, thread_ID, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_wraplog_fill_nolock thread=%d, iteration=%ld\n",
	     thread_ID, i);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_thread_wraplog_fill_weak (SPHSinglePCQueue_t pcqueue,
			       int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;

  for (i = 0; i < iterations; i++)
    {
      rc = lflogentry_stridetest_weak (pcqueue, i, thread_ID, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf ("test_thread_wraplog_fill_weak thread=%d, iteration=%ld\n",
		  thread_ID, i);
	  printf ("SPHLFpcqueueAllocStrideTimeStamped (%p) failed\n",
		  pcqueue);
	  rtn++;
	  break;
	}
    }

  return rtn;
}

int
test_wraplog_init (block_size_t log_size)
{
  int rc = 0;
  //log_alloc = (4*1024*1024);
  log_alloc = log_size;


  pcqueue = SPHLFCircularpcqueueCreate (log_alloc, 128);
  if (pcqueue)
    {
      rc = SPHLFpcqueuePrefetch (pcqueue);
      printf ("\nSPHLFCirgularpcqueueCreate (%zu) success \n", log_alloc);

      cap = SPHLFpcqueueFreeSpace (pcqueue);
      units = cap / 128;

      printf ("SPHLFpcqueueFreeSpace() = %zu, units=%zu\n", cap, units);
    }
  else
    rc = 10;

  return rc;
}

int
test_wraplog_reset ()
{
  int rc = 0;


  rc = SPHLFpcqueueResetAsync (pcqueue);
  if (!rc)
    {
      printf ("\nSPHLFpcqueueResetAsync (%p) success \n", pcqueue);

      cap = SPHLFpcqueueFreeSpace (pcqueue);
      units = cap / 128;

      printf ("SPHLFpcqueueFreeSpace() = %zu, units=%zu\n", cap, units);
    }
  else
    rc = 10;

  return rc;
}

int
test_thread_wraplog_parallel_verify (void)
{

  int rc, rtn = 0;
  int i;

  for (i = 0; i < num_threads; i++)
    {
      rtn += test_thread_wraplog_verify (pcq_list[i]);
    }

  return rtn;
}

int
test_log_parallel_cleanup (void)
{
  int rc, rtn = 0;
  int i;

  for (i = 0; i < num_threads; i++)
    {
      rc = SPHLFpcqueueDestroy (pcq_list[i]);
      printf ("SPHLFpcqueueDestroy(%p) = %d\n", pcq_list[i], rc);

      pcq_list[i] = NULL;
    }

  return rtn;
}

int
test_wraplog_parallel_init (block_size_t log_size)
{
  int rc = 0;
  int i;
  //log_alloc = (4*1024*1024);
  log_alloc = log_size;

  for (i = 0; i < num_threads; i++)
    {
      pcq_list[i] = SPHLFCircularpcqueueCreate (log_alloc, 128);
      if (pcq_list[i])
	{
	  rc = SPHLFpcqueuePrefetch (pcq_list[i]);
	  printf ("\nSPHLFCirgularpcqueueCreate (%zu) success \n", log_alloc);

	  cap = SPHLFpcqueueFreeSpace (pcq_list[i]);
	  units = cap / 128;

	  printf ("SPHLFpcqueueFreeSpace() = %zu, units=%zu\n", cap, units);
	}
      else
	rc += 10;
    }

  return rc;
}

int
test_wraplog_parallel_reset ()
{
  int rc = 0;
  int i;

  for (i = 0; i < num_threads; i++)
    {
      rc = SPHLFpcqueueResetAsync (pcq_list[i]);
      if (!rc)
	{
	  printf ("\nSPHLFpcqueueResetAsync (%p) success \n", pcq_list[i]);

	  cap = SPHLFpcqueueFreeSpace (pcq_list[i]);
	  units = cap / 128;

	  printf ("SPHLFpcqueueFreeSpace() = %zu, units=%zu\n", cap, units);
	}
      else
	rc += 10;
    }

  return rc;
}

#endif

int
main ()
{
  int rc, rc3;
  double clock, nano, rate;
  sphtimer_t tempt, startt, endt, freqt;

  N_PROC_CONF = sysconf (_SC_NPROCESSORS_ONLN);
  if (N_PROC_CONF < 8)
    num_threads = N_PROC_CONF;
  else
    num_threads = 8;

  SAS_IO_INIT			// init the io stuff
    rc = SASJoinRegion ();

  if (rc)
    {
      printf ("SASJoinRegion Error# %d\n", rc);

      return 1;
    }
  else
    {
      printf ("SAS Joined with %d processors\n", N_PROC_CONF);

      num_threads = 2;
      rc = test_pcq_init (4096);

      pcq_list[0] = pcqueue;
      pcq_list[1] = pcqueue;
      test_funclist[0] = test_thread_Producer_fill;
      test_funclist[1] = test_thread_consumer_verify;
      if (rc == 0)
	{
	  p10 = units * 1000000;
	  printf ("start test_thread_Producer | consumer (%p,%zu)\n",
		  pcqueue, units);

	  startt = sphgettimer ();
	  rc +=
	    launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	  endt = sphgettimer ();
	  tempt = endt - startt;
	  clock = tempt;
	  freqt = sphfastcpufreq ();
	  nano = (clock * 1000000000.0) / (double) freqt;
	  nano = nano / p10;
	  rate = p10 / (clock / (double) freqt);

	  printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
		  startt, endt, tempt, freqt);

	  printf
	    ("test_thread_Producer | consumer %zu ave= %6.2fns rate=%10.1f/s\n",
	     p10, nano, rate);
	  printf ("end   test_thread_Producer | consumer (%p,%zu) = %d\n",
		  pcqueue, units, rc);

	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      pcq_list[0] = pcqueue;
	      pcq_list[1] = pcqueue;
	      test_funclist[0] = test_thread_Producer_fastfill;
	      test_funclist[1] = test_thread_consumer_fastverify;
	      p10 = units * 1000000;

	      printf ("start test_thread_Producer | consumer fast (%p,%zu)\n",
		      pcqueue, units);

	      startt = sphgettimer ();
	      rc +=
		launch_test_threads (num_threads, fill_test_parallel_thread,
				     p10);
	      endt = sphgettimer ();
	      tempt = endt - startt;
	      clock = tempt;
	      freqt = sphfastcpufreq ();
	      nano = (clock * 1000000000.0) / (double) freqt;
	      nano = nano / p10;
	      rate = p10 / (clock / (double) freqt);

	      printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
		      startt, endt, tempt, freqt);

	      printf
		("test_thread_Producer | consumer fast %zu ave= %6.2fns rate=%10.1f/s\n",
		 p10, nano, rate);
	      printf
		("end   test_thread_Producer | consumer fast (%p,%zu) = %d\n",
		 pcqueue, units, rc);
	    }
	  else
	    rc += rc3;
	}
    }

  //SASCleanUp();
  printf ("SAS removed\n");
  SASRemove ();
  return rc;
};
