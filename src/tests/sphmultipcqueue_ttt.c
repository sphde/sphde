/*
 * sphmultipcqueue_ttt.c
 *
 *  Created on: 2016 May 23
 *      Author: pc@us.ibm.com
 */
/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 *     IBM Corporation, Paul Clarke - port to Multi
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>
#include "sasshm.h"
#include "sasconf.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "saslock.h"
#define SPH_INTERNAL
#include "sphlfentry.h"
#include "sphmultipcqueue.h"
#include "sphthread.h"
#include "sphtimer.h"


typedef struct {
	volatile unsigned int 	threshold;
	volatile unsigned int 	spincnt;
	/** Boolean: Indicates that a thread is waiting. **/
	volatile unsigned int 	waiting;
	volatile unsigned int 	waitcnt;
	volatile unsigned int 	postcnt;
	volatile unsigned int 	remaining;
	volatile unsigned int 	semvalue;
	volatile int 	datavalue;
	/** Boolean: Queue wait lock. **/
	sem_t			qlock;
	} sphPCQSem_t;



static int N_PROC_CONF = 1;
static int num_threads = 1;
static long thread_iterations;


static const int max_threads = 256;
#define MAX_THREADS 256

static SPHMultiPCQueue_t pcqueue;
static SPHMultiPCQueue_t pcqueue2;
static SPHMultiPCQueue_t pcqueue3;
static SPHMultiPCQueue_t consumer_pcq_list[MAX_THREADS];
static SPHMultiPCQueue_t producer_pcq_list[MAX_THREADS];

static block_size_t cap, units, p10, pcq_alloc, pcq_stride;
//static volatile block_size_t pthreshold, cthreshold;

typedef void *(*test_ptr_t) (void *);
typedef int (*test_fill_ptr_t) (SPHMultiPCQueue_t, SPHMultiPCQueue_t, int, long);
static test_fill_ptr_t test_funclist[MAX_THREADS];
static int cpu_list[MAX_THREADS] = {0};
static int cpu_list_len = 0;
static int cpu_max = 0;

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sphmultipcqueue_ttt";

static inline void
sassim_print_error (const char *test, int line, const char *fmt, ...)
{
  va_list args;
  fprintf (stderr, "%s:%s:%i error: ", sassim_prog_name, test, line);
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
}

#define SASSIM_PRINT_ERR(fmt, ...) sassim_print_error(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

static inline void
sassim_print_msg (const char *func, int line, const char *fmt, ...)
{
  va_list args;
  printf ("%s:%s:%i ", sassim_prog_name, func, line);
  va_start (args, fmt);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);
}

#define SASSIM_PRINT_MSG(fmt, ...) sassim_print_msg(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

static void
sassim_dump_block (const char *func, int line, void *blockAddr,
                   unsigned long len)
{
  unsigned int *dumpAddr = (unsigned int *) blockAddr;
  unsigned char *charAddr = (unsigned char *) blockAddr;
  void *tempAddr;
  unsigned char chars[20];
  unsigned char temp;
  unsigned int i, j;
  chars[16] = 0;
  for (i = 0; i < len; i = i + 16)
    {
      tempAddr = dumpAddr;
      for (j = 0; j < 16; j++)
        {
          temp = *charAddr++;
          if ((temp < 32) || (temp > 126))
            temp = '.';
          chars[j] = temp;
        };
      sassim_print_msg (func, line, "%p  %08x %08x %08x %08x <%s>",
                        tempAddr, *dumpAddr, *(dumpAddr + 1),
                        *(dumpAddr + 2), *(dumpAddr + 3), chars);
      dumpAddr += 4;
    }
}
#define SASSIM_DUMP_BLOCK(fmt, ...) sassim_dump_block(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#undef DEBUG_PRINT

int lfPCQentry_pingtest(SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1 = 0;
	int rc2, rc3, rc4;
	SPHLFEntryDirect_t handle;
	int *array;

	handle = SPHMultiPCQueueAllocStrideDirectTM(pqueue);
	if (handle) {
		array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		array[0] = val1;
		array[1] = val2;
		array[2] = val3;
		SPHLFEntryDirectComplete(handle, entry_tmp, 1, 2);

		handle = SPHMultiPCQueueGetNextCompleteDirectTM(cqueue);
		if (handle) {
			array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			rc1 = (array[0] != val1);
			rc2 = (array[1] != val2);
			rc3 = (array[2] != val3);

			if (rc1 | rc2 | rc3) {
				printf(
						"lfPCQentry_pingtest:: SPHLFEntryDirectGetFreePtr(%p) = %d,%d,%d should be %d,%d,%d\n",
						handle, array[0], array[1], array[2], val1, val2, val3);
				array = (int *) SPHLFEntryDirectGetFreePtr(handle);
				rc1 = (array[0] != val1);
				rc2 = (array[1] != val2);
				rc3 = (array[2] != val3);

				if (rc1 | rc2 | rc3) {
					SASSIM_DUMP_BLOCK(cqueue, 4096);
				}
			}

			if (SPHMultiPCQueueFreeEntryDirect(cqueue, handle)) {
				rc4 = 0;
			} else {
				printf(
						"lfPCQentry_pingtest:: SPHMultiPCQueueFreeEntryDirect() = failed\n");
				rc4 = 1;
			}
		} else {
			return (10);
		}

		rc1 = rc1 | rc2 | rc3 | rc4;
	} else {
		rc1 = 1;
	}

	return (rc1);
}

int
lfPCQentry_pongtest(SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1, rc2, rc3, rc4;
	SPHLFEntryDirect_t handle, handle2;
	int *array, *array2;

	handle = SPHMultiPCQueueGetNextCompleteDirectTM(pqueue);
	if (handle) {
		array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		rc1 = (array[0] != val1);
		rc2 = (array[1] != val2);
		rc3 = (array[2] != val3);

		if (rc1 | rc2 | rc3) {
			printf(
					"lfPCQentry_pongtest:: SPHLFEntryDirectGetFreePtr(%p) = %d,%d,%d should be %d,%d,%d\n",
					handle, array[0], array[1], array[2], val1, val2, val3);
			array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			rc1 = (array[0] != val1);
			rc2 = (array[1] != val2);
			rc3 = (array[2] != val3);

			if (rc1 | rc2 | rc3) {
				SASSIM_DUMP_BLOCK(pqueue, 4096);
			}
		}

		handle2 = SPHMultiPCQueueAllocStrideDirectTM(cqueue);
		if (handle2) {
			array2 = (int *) SPHLFEntryDirectGetFreePtr(handle2);
			array2[0] = array[0];
			array2[1] = array[1];
			array2[2] = array[2];
			SPHLFEntryDirectComplete(handle2, entry_tmp, 1, 2);
		} else {
			rc4 = 1;
		}

		if (SPHMultiPCQueueFreeEntryDirect(pqueue, handle)) {
			rc4 = 0;
		} else {
			printf(
					"lfPCQentry_pongtest:: SPHMultiPCQueueFreeEntryDirect() = failed\n");
			rc4 = 1;
		}
	} else {
		return (10);
	}

	return (rc1 | rc2 | rc3 | rc4);
}

int lfPCQentry_pingpause(SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1 = 0;
	int rc2, rc3, rc4;
	SPHLFEntryDirect_t handle;
	int *array;

	handle = SPHMultiPCQueueAllocStrideDirectTM(pqueue);
	if (handle) {
		array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		array[0] = val1;
		array[1] = val2;
		array[2] = val3;
		SPHLFEntryDirectComplete(handle, entry_tmp, 1, 2);

		handle = SPHMultiPCQueueGetNextCompleteDirectTM(cqueue);
		if (handle) {
			array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			rc1 = (array[0] != val1);
			rc2 = (array[1] != val2);
			rc3 = (array[2] != val3);

			if (rc1 | rc2 | rc3) {
				printf(
						"lfPCQentry_pingtest:: SPHLFEntryDirectGetFreePtr(%p) = %d,%d,%d should be %d,%d,%d\n",
						handle, array[0], array[1], array[2], val1, val2, val3);
				array = (int *) SPHLFEntryDirectGetFreePtr(handle);
				rc1 = (array[0] != val1);
				rc2 = (array[1] != val2);
				rc3 = (array[2] != val3);

				if (rc1 | rc2 | rc3) {
					SASSIM_DUMP_BLOCK(cqueue, 4096);
				}
			}

			if (SPHMultiPCQueueFreeEntryDirect(cqueue, handle)) {
				rc4 = 0;
			} else {
				printf(
						"lfPCQentry_pingtest:: SPHMultiPCQueueFreeEntryDirect() = failed\n");
				rc4 = 1;
			}
		} else {
			return (10);
		}

		rc1 = rc1 | rc2 | rc3 | rc4;
	} else {
		rc1 = 1;
	}

	return (rc1);
}

int
lfPCQentry_pongpause(SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1, rc2, rc3, rc4;
	SPHLFEntryDirect_t handle, handle2;
	int *array, *array2;

	handle = SPHMultiPCQueueGetNextCompleteDirectTM(pqueue);
	if (handle) {
		array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		rc1 = (array[0] != val1);
		rc2 = (array[1] != val2);
		rc3 = (array[2] != val3);

		if (rc1 | rc2 | rc3) {
			printf(
					"lfPCQentry_pongtest:: SPHLFEntryDirectGetFreePtr(%p) = %d,%d,%d should be %d,%d,%d\n",
					handle, array[0], array[1], array[2], val1, val2, val3);
			array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			rc1 = (array[0] != val1);
			rc2 = (array[1] != val2);
			rc3 = (array[2] != val3);

			if (rc1 | rc2 | rc3) {
				SASSIM_DUMP_BLOCK(pqueue, 4096);
			}
		}

		handle2 = SPHMultiPCQueueAllocStrideDirectTM(cqueue);
		if (handle2) {
			array2 = (int *) SPHLFEntryDirectGetFreePtr(handle2);
			array2[0] = array[0];
			array2[1] = array[1];
			array2[2] = array[2];
			SPHLFEntryDirectComplete(handle2, entry_tmp, 1, 2);
		} else {
			rc4 = 1;
		}

		if (SPHMultiPCQueueFreeEntryDirect(pqueue, handle)) {
			rc4 = 0;
		} else {
			printf(
					"lfPCQentry_pongtest:: SPHMultiPCQueueFreeEntryDirect() = failed\n");
			rc4 = 1;
		}
	} else {
		return (10);
	}

	return (rc1 | rc2 | rc3 | rc4);
}

int
lfPCQentry_fastertest (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3)
{
  int rc1 = 0;
  SPHLFEntryDirect_t handle;
  int *array;

  handle = SPHMultiPCQueueAllocStrideDirectTM (pqueue);
  if (handle)
    {
      array = (int *) SPHLFEntryDirectGetFreePtr (handle);
      array[0] = val1;
      array[1] = val2;
      array[2] = val3;
      SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2);
    }
  else
    {
      rc1 = 1;
    }

  return (rc1);
}

int
lfPCQentry_fasterverify (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue, int val1, int val2,
		       int val3)
{
  int rc1, rc2, rc3, rc4;
  SPHLFEntryDirect_t handle;
  int *array;

#if 0
  handle = SPHMultiPCQueueGetNextCompleteDirectTM (cqueue);
  if (!handle)
    {
      while (!handle)
        {
          handle = SPHMultiPCQueueGetNextEntryDirectTM (cqueue);
        }
      while (!SPHLFEntryDirectIsComplete(handle))
        {
#if __powerpc__
          sas_read_barrier ();
#else
          __arch_pause();
#endif
        }
    }
#else
  handle = SPHMultiPCQueueGetNextCompleteDirectTM (cqueue);
#endif
  if (handle)
    {
      array = (int *) SPHLFEntryDirectGetFreePtr (handle);
      rc1 = (array[0] != val1);
      rc2 = (array[1] != val2);
      rc3 = (array[2] != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_fasterverify:: SPHLFEntryDirectGetFreePtr(%p) = %d,%d,%d should be %d,%d,%d\n",
	     handle, array[0], array[1], array[2] ,val1, val2, val3);
          array = (int *) SPHLFEntryDirectGetFreePtr (handle);
          rc1 = (array[0] != val1);
          rc2 = (array[1] != val2);
          rc3 = (array[2] != val3);

          if (rc1 | rc2 | rc3)
           {
	     SASSIM_DUMP_BLOCK (cqueue, 4096);
           }
	}

      if (SPHMultiPCQueueFreeEntryDirect (cqueue, handle))
	{
	  rc4 = 0;
	}
      else
	{
	  printf
	    ("lfPCQentry_fasterverify:: SPHMultiPCQueueFreeEntry() = failed\n");
	  rc4 = 1;
	}
    }
  else
    {
      return (10);
    }

  return (rc1 | rc2 | rc3 | rc4);
}
//#undef DEBUG_PRINT
int test_thread_Producer_pinger(SPHMultiPCQueue_t pqueue,
		SPHMultiPCQueue_t cqueue, int thread_ID, long iterations) {
	int rc, rtn = 0;
	long i;
	sphLFEntryID_t entry_tmp;
#ifdef DEBUG_PRINT
	char number[128];
	sprintf (number, "test_thread_Producer_pinger(%p, %d, %ld)\n", pcqueue,
			thread_ID, iterations);
	puts (number);
#endif

	entry_tmp = SPHMultiPCQueueGetEntryTemplate(pqueue);
	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_pingtest(pqueue, cqueue, entry_tmp, i, 0x01234567,
				0xdeadbeef);
		if (!rc) {
		} else {
			printf(
					"test_thread_Producer_pinger SPHMultiPCQueueAllocDirectTM (%p) failed\n",
					pcqueue);
			rtn++;
			break;
		}
	}

	return (rtn);
}

int
test_thread_consumer_ponger (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
                                 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
	sphLFEntryID_t entry_tmp;
#ifdef DEBUG_PRINT
	char number[128];
	sprintf (number, "test_thread_Producer_ponger(%p, %d, %ld)\n", pcqueue,
			thread_ID, iterations);
	puts (number);
#endif

	entry_tmp = SPHMultiPCQueueGetEntryTemplate(pqueue);
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_pongtest (pqueue, cqueue, entry_tmp, i, 0x01234567, 0xdeadbeef);
      if (!rc)
        {
        }
      else
        {
          printf
            ("lfPCQentry_pongtest SPHMultiPCQueueGetNextCompleteDirectTM (%p) failed\n",
             pcqueue);
          rtn++;
          break;
        }
    }

  return (rtn);
}

int test_thread_Producer_pingpause(SPHMultiPCQueue_t pqueue,
		SPHMultiPCQueue_t cqueue, int thread_ID, long iterations) {
	int rc, rtn = 0;
	long i;
	sphLFEntryID_t entry_tmp;
#ifdef DEBUG_PRINT
	char number[128];
	sprintf (number, "test_thread_Producer_pingpause(%p, %d, %ld)\n", pcqueue,
			thread_ID, iterations);
	puts (number);
#endif

	entry_tmp = SPHMultiPCQueueGetEntryTemplate(pqueue);
	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_pingpause(pqueue, cqueue, entry_tmp, i, 0x01234567,
				0xdeadbeef);
		if (!rc) {
		} else {
			printf(
					"test_thread_Producer_pingpause SPHMultiPCQueueAllocDirectTM (%p) failed\n",
					pcqueue);
			rtn++;
			break;
		}
	}

	return (rtn);
}

int
test_thread_consumer_pongpause (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
                                 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
	sphLFEntryID_t entry_tmp;
#ifdef DEBUG_PRINT
	char number[128];
	sprintf (number, "test_thread_Producer_ponger(%p, %d, %ld)\n", pcqueue,
			thread_ID, iterations);
	puts (number);
#endif

	entry_tmp = SPHMultiPCQueueGetEntryTemplate(pqueue);
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_pongpause (pqueue, cqueue, entry_tmp, i, 0x01234567, 0xdeadbeef);
      if (!rc)
        {
        }
      else
        {
          printf
            ("lfPCQentry_pongtest SPHMultiPCQueueGetNextCompleteDirectTM (%p) failed\n",
             pcqueue);
          rtn++;
          break;
        }
    }

  return (rtn);
}

int
test_thread_Producer_fasterfill (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
			       int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
  sphLFEntryID_t entry_tmp;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_Producer_fasterfill(%p, %d, %ld)\n", pcqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  entry_tmp = SPHMultiPCQueueGetEntryTemplate(pqueue);
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_fastertest (pqueue, cqueue, entry_tmp,
    		  i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_Producer_fasterfill SPHMultiPCQueueDirect (%p) failed\n",
	     pcqueue);
	  rtn++;
	  break;
	}
    }

  return (rtn);
}

int
test_thread_consumer_fasterverify (SPHMultiPCQueue_t pqueue, SPHMultiPCQueue_t cqueue,
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
      rc = lfPCQentry_fasterverify (pqueue, cqueue, i, 0x01234567, 0xdeadbeef);
      if (!rc)
        {
        }
      else
        {
          printf
            ("test_thread_consumer_verify SPHMultiPCQueueGetNextCompleteTM (%p) failed\n",
             pcqueue);
          rtn++;
          break;
        }
    }

  return (rtn);
}

static void *
fill_test_parallel_thread (void *arg)
{
  SPHMultiPCQueue_t pqueue, cqueue;
  test_fill_ptr_t test_func;
  long result = 0;
  int tn = (int) (long int) arg;

  pqueue = producer_pcq_list[tn];
  cqueue = consumer_pcq_list[tn];
  test_func = test_funclist[tn];

  if (cpu_list_len > tn) {
	  int rc;
	  cpu_set_t *cset = CPU_ALLOC(cpu_max);
	  int size = CPU_ALLOC_SIZE(cpu_max);
	  CPU_ZERO_S(size,cset);
	  CPU_SET_S(cpu_list[tn],size,cset);
	  printf("%-6d: %s binding thread %d to %d\n",sphFastGetTID(),__FUNCTION__,tn,cpu_list[tn]);
	  rc = sched_setaffinity(sphFastGetTID(),size,cset);
	  printf("%-6d: %s sched_setaffinity=%d\n",sphFastGetTID(),__FUNCTION__,rc);
  }
  SASThreadSetUp ();
#ifdef DEBUG_PRINT
  char number[128];
  pid_t tid = sphFastGetTID ();
  sprintf (number, "ltt(%d, %d, @%p, @%p): begin", tn, tid, pqueue, cqueue);
  puts (number);
#endif

  result += (*test_func) (pqueue, cqueue, tn, thread_iterations);

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
  pthread_t th[max_threads];
  long thread_result;
  int result = 0;

#ifdef DEBUG_PRINT
  int pid = getpid ();
  int tid = sphdeGetTID ();
  printf ("creating thread from pid/tid = %d/%d\n", pid, tid);
#endif
  thread_iterations = iterations / t_cnt;

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

  pcqueue = SPHMultiPCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue)
    {
      rc = SPHMultiPCQueueSetCachePrefetch (pcqueue, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);

      cap = SPHMultiPCQueueFreeSpace (pcqueue);

      units = cap / 128;

      printf ("SPHMultiPCQueueFreeSpace() = %zu units=%zu\n", cap, units);
    }
  else
    rc++;

  pcqueue2 = SPHMultiPCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue2)
    {
      rc = SPHMultiPCQueueSetCachePrefetch (pcqueue2, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);
    }
  else
    rc++;

  pcqueue3 = SPHMultiPCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue3)
    {
      rc = SPHMultiPCQueueSetCachePrefetch (pcqueue3, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);
    }
  else
    rc++;

  return rc;
}

int
test_pcq_reset ()
{
  int rc = 0;

  rc = SPHMultiPCQueueResetAsync (pcqueue);
  if (!rc)
    {
      printf ("\nSPHMultiPCQueueResetAsync (%p) success \n", pcqueue);

      cap = SPHMultiPCQueueFreeSpace (pcqueue);

      units = cap / 128;

      printf ("SPHMultiPCQueueFreeSpace() = %zu units=%zu\n", cap, units);
    }
  else
    rc++;

  rc = SPHMultiPCQueueResetAsync (pcqueue2);
  if (!rc)
    {
      printf ("\nSPHMultiPCQueueResetAsync (%p) success \n", pcqueue2);
    }
  else
    rc++;

  rc = SPHMultiPCQueueResetAsync (pcqueue3);
  if (!rc)
	{
      printf ("\nSPHMultiPCQueueResetAsync (%p) success \n", pcqueue3);
	    }
	  else
    rc++;

  return rc;
}

int
main (int argc, char *argv[])
{
  int rc, rc3;
  double clock, nano, rate;
  sphtimer_t tempt, startt, endt, freqt;
  int i;

  for (i = 1; i < argc; i++) {
	  cpu_list[i-1] = strtol(argv[i],0,0);
	  if (cpu_list[i-1] > cpu_max) cpu_max = cpu_list[i-1];
	  cpu_list_len++;
  }

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

      return (1);
	}
      else
	{
      printf ("SAS Joined with %d processors\n", N_PROC_CONF);

      num_threads = 2;
      rc = test_pcq_init (4096);
      if (rc == 0)
	{
#if 1
          rc3 = test_pcq_reset ();
          if (rc3 == 0)
    {
              producer_pcq_list[0] = pcqueue;
              consumer_pcq_list[1] = pcqueue;
              test_funclist[0] = test_thread_Producer_fasterfill;
              test_funclist[1] = test_thread_consumer_fasterverify;
              p10 = units * 10000000;
              p10 = units * 1000000;

              printf ("start 2a test_thread_Producer | consumer fast (%p,%zu)\n",
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
                ("test_thread_Producer | consumer faster %zu ave= %6.2fns rate=%10.1f/s\n",
                 p10, nano, rate);
              printf
                ("end 2a  test_thread_Producer | consumer faster (%p,%zu) = %d\n",
                 pcqueue, units, rc);
        }
      else
            rc += rc3;
#endif
#if 1
          rc3 = test_pcq_reset ();
          if (rc3 == 0)
    {
              producer_pcq_list[0] = pcqueue;
              producer_pcq_list[1] = pcqueue;
              consumer_pcq_list[0] = pcqueue2;
              consumer_pcq_list[1] = pcqueue2;
              test_funclist[0] = test_thread_Producer_pinger;
              test_funclist[1] = test_thread_consumer_ponger;
              p10 = units * 10000000;
              p10 = units * 1000000;

              printf ("start 3a test_thread_Producer | consumer ping/pong (%p,%zu)\n",
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
                ("test_thread_Producer | consumer %zu ave= %6.2fns rate=%10.1f/s\n",
                 p10, nano, rate);
              printf
                ("end 3a  test_thread_Producer | consumer ping/pong (%p,%zu) = %d\n",
                 pcqueue, units, rc);
        }
      else
            rc += rc3;
#endif
#if 0
          rc3 = test_pcq_reset ();
          if (rc3 == 0)
    {
              producer_pcq_list[0] = pcqueue;
              producer_pcq_list[1] = pcqueue;
              consumer_pcq_list[0] = pcqueue2;
              consumer_pcq_list[1] = pcqueue2;
              test_funclist[0] = test_thread_Producer_pingpause;
              test_funclist[1] = test_thread_consumer_pongpause;
              p10 = units * 10000000;
              p10 = units * 100000;

              printf ("start 3b test_thread_Producer | consumer ping/pong (%p,%zu)\n",
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
                ("test_thread_Producer | consumer %zu ave= %6.2fns rate=%10.1f/s\n",
                 p10, nano, rate);
              printf
                ("end 3b  test_thread_Producer | consumer ping/pong (%p,%zu) = %d\n",
                 pcqueue, units, rc);
        }
      else
            rc += rc3;
#endif
	}
    }
  fflush(stdout);
  //SASCleanUp();
  printf ("SAS removed\n");
  SASRemove ();
  return (rc);
};
