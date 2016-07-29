/*
 * sphsinglepcqueue_ttt.c
 *
 *  Created on: Jun 21, 2012
 *      Author: sjmunroe
 */
/*
 * Copyright (c) 2012, 2014, 2015 IBM Corporation.
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
#include <semaphore.h>
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

#ifdef LONGCHECK
# define ITERATIONS 10000000
#else
# define ITERATIONS 1000000
#endif

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


typedef union {
	/** Logger Entry header as a Unit **/
	char		unit[128] __attribute__ ((aligned (128)));
	/** Logger Entry header in detail **/
	sphPCQSem_t	sem;
	} sphPCQAlignedSem_t;

static int N_PROC_CONF = 1;
static int num_threads = 1;
static long thread_iterations;


static const int max_threads = 256;
#define MAX_THREADS 256

static SPHSinglePCQueue_t pcqueue;
static sphPCQAlignedSem_t pwait, cwait;
static SPHSinglePCQueue_t pcqueue2;
static sphPCQAlignedSem_t pwait2, cwait2;
static SPHSinglePCQueue_t pcqueue3;
static sphPCQAlignedSem_t pwait3, cwait3;
static SPHSinglePCQueue_t consumer_pcq_list[MAX_THREADS];
static SPHSinglePCQueue_t producer_pcq_list[MAX_THREADS];
static sphPCQAlignedSem_t *consumer_sem_list[MAX_THREADS];
static sphPCQAlignedSem_t *producer_sem_list[MAX_THREADS];

static block_size_t cap, units, p10, pcq_alloc, pcq_stride;
//static volatile block_size_t pthreshold, cthreshold;

typedef void *(*test_ptr_t) (void *);
typedef int (*test_fill_ptr_t) (SPHSinglePCQueue_t, SPHSinglePCQueue_t, int, long);
static test_fill_ptr_t test_funclist[MAX_THREADS];

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sphsinglepcqueue_ttt";

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
#if 0
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
#endif
static void
sem_data_init (sphPCQAlignedSem_t *wait, unsigned int spinlimit, unsigned int threshld)
{
    wait->sem.datavalue = 0;
    wait->sem.remaining = 0;
    wait->sem.waiting = 0;
    wait->sem.semvalue = 0;

    wait->sem.waitcnt = 0;
    wait->sem.postcnt = 0;
    wait->sem.spincnt = spinlimit;
    wait->sem.threshold = threshld;
}

int
sem_wait_if_empty (sphPCQAlignedSem_t *wait,
		SPHSinglePCQueue_t pcqueue)
{
	int rc1 = 0;
	int spinner = wait->sem.spincnt;

    while (SPHSinglePCQueueEmpty (pcqueue))
	{
      if (spinner)
      {
    	  spinner--;
      } else {
		  wait->sem.waiting = 1;
		  wait->sem.waitcnt += 1;
		  if (sem_wait (&wait->sem.qlock))
		  {
			  perror ("sem_wait_if_empty sem_wait failed");
			  rc1++;
		  }
      }
	}

	return rc1;
}

int
sem_wait_if_full (sphPCQAlignedSem_t *wait,
		SPHSinglePCQueue_t pcqueue)
{
	int rc1 = 0;
	int spinner = wait->sem.spincnt;

    while (SPHSinglePCQueueFull (pcqueue))
	{
        if (spinner)
        {
      	  spinner--;
        } else {
		  wait->sem.waiting = 1;
		  wait->sem.waitcnt += 1;
		  if (sem_wait (&wait->sem.qlock))
		  {
			  perror ("sem_wait_if_full sem_wait failed");
			  rc1++;
		  }
        }
	}

	return rc1;
}

int
sem_post_if_waiting (sphPCQAlignedSem_t *wait,
		SPHSinglePCQueue_t pcqueue, int value)
{
	int rc1 = 0;
	int semval;

	wait->sem.datavalue = value;
	sem_getvalue(&wait->sem.qlock, &semval);
    if (semval <= 0)
      {
		 block_size_t remaining = SPHSinglePCQueueFreeSpace (pcqueue);
		 wait->sem.remaining = remaining;
		 wait->sem.semvalue = semval;
		 wait->sem.waiting = 0;
		 wait->sem.postcnt += 1;
		 if (sem_post (&wait->sem.qlock))
			{
			  perror ("sem_post_if_waiting sem_post failed");
			  rc1++;
			}
      }
	return rc1;
}

int
sem_post_if_waiting_low_threshold (sphPCQAlignedSem_t *wait,
		SPHSinglePCQueue_t pcqueue, int value)
{
	int rc1 = 0;
	int semval;

	wait->sem.datavalue = value;
	sem_getvalue(&wait->sem.qlock, &semval);
    if (semval <= 0)
      {
		 block_size_t remaining = SPHSinglePCQueueFreeSpace (pcqueue);
		 wait->sem.remaining = remaining;
		 wait->sem.semvalue = semval;
		 if (remaining <= wait->sem.threshold)
		   {
			  wait->sem.waiting = 0;
			  wait->sem.postcnt += 1;
			  if (sem_post (&wait->sem.qlock))
				{
				  perror ("sem_post_if_waiting sem_post failed");
				  rc1++;
				}
			}
      }
	return rc1;
}

int
sem_post_if_waiting_high_threshold (sphPCQAlignedSem_t *wait,
		SPHSinglePCQueue_t pcqueue, int value)
{
	int rc1 = 0;
	int semval;

	wait->sem.datavalue = value;
	sem_getvalue(&wait->sem.qlock, &semval);
    if (semval <= 0)
      {
		 block_size_t remaining = SPHSinglePCQueueFreeSpace (pcqueue);
		 wait->sem.remaining = remaining;
		 wait->sem.semvalue = semval;
		 if (remaining >= wait->sem.threshold)
		   {
			  wait->sem.waiting = 0;
			  wait->sem.postcnt += 1;
			  if (sem_post (&wait->sem.qlock))
				{
				  perror ("sem_post_if_waiting sem_post failed");
				  rc1++;
				}
			}
      }
	return rc1;
}

int
lfPCQentry_waittest (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
		sphPCQAlignedSem_t *pwait, sphPCQAlignedSem_t *cwait,
		int val1, int val2, int val3)
{
  int rc1 = 0;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
  if (!handle)
    {
#if 0
      while (SPHSinglePCQueueFull (pqueue))
		{
		  pwait->sem.waiting = 1;
		  pwait->sem.waitcnt += 1;
		  if (sem_wait (&pwait->sem.qlock))
		  {
			  perror ("lfPCQentry_waittest sem_wait failed");
			  rc1++;
		  }
		}
#else
      sem_wait_if_full (pwait, pqueue);
#endif
      handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
    }
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      array[0] = val1;
      array[1] = val2;
      array[2] = val3;
      SPHLFEntryComplete (&handle0);

      sem_post_if_waiting_low_threshold (cwait, pqueue, val1);
    }
  else
    {
      rc1 = 1;
    }

  return (rc1);
}

int
lfPCQentry_waittransfer (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
		sphPCQAlignedSem_t *pwait, sphPCQAlignedSem_t *cwait,
		sphPCQAlignedSem_t *pwait2, sphPCQAlignedSem_t *cwait2,
		int val1, int val2, int val3)
{
  int rc1, rc2, rc3, rc4;
  SPHLFEntryHandle_t *handlec, *handlep, handle0, handle1;
  int *array, *array1 = 0;

  rc4 = 0;
  handlec = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
  if (!handlec)
    {
      sem_wait_if_empty (cwait, cqueue);

      handlec = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
    }
  if (handlec)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      rc1 = (array[0] != val1);
      rc2 = (array[1] != val2);
      rc3 = (array[2] != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_waittransfer:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
	    		array[0], array[1], array[2], val1, val2, val3);
	}

      handlep = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle1);
      if (!handlep)
        {
          sem_wait_if_full (pwait2, pqueue);

          handlep = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle1);
        }
      if (handlep)
        {
          array1 = (int *) SPHLFEntryGetFreePtr (&handle1);
          array1[0] = array[0];
          array1[1] = array[1];
          array1[2] = array[2];
          SPHLFEntryComplete (&handle1);

          sem_post_if_waiting_low_threshold (cwait2, pqueue, val1);
        }

      if (SPHSinglePCQueueFreeNextEntry (cqueue))
		{
          sem_post_if_waiting_high_threshold (pwait, cqueue, array1[0]);
		}
      else
		{
		  printf
			("lfPCQentry_waittransfer:: SPHSinglePCQueueFreeNextEntry() = failed\n");
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
lfPCQentry_waittransfercopy (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
		sphPCQAlignedSem_t *pwait, sphPCQAlignedSem_t *cwait,
		sphPCQAlignedSem_t *pwait2, sphPCQAlignedSem_t *cwait2,
		int val1, int val2, int val3)
{
  int rc0, rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handlec, *handlep, handle0, handle1;
  int *array, *array1;

  rc4 = 0;
  handlec = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
  if (!handlec)
    {
      sem_wait_if_empty (cwait, cqueue);

      handlec = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
    }
  if (handlec)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      tv1 = array[0];
      tv2 = array[1];
      tv3 = array[2];
      rc1 = (tv1 != val1);
      rc2 = (tv2 != val2);
      rc3 = (tv3 != val3);

      if (rc1 | rc2 | rc3)
		{
		  printf
			("lfPCQentry_waittransfercopy:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
			 tv1, tv2, tv3, val1, val2, val3);
		}

      if (SPHSinglePCQueueFreeNextEntry (cqueue))
		{
          sem_post_if_waiting_high_threshold (pwait, cqueue, tv1);
		}
      else
		{
		  printf
			("lfPCQentry_waittransfercopy:: SPHSinglePCQueueFreeNextEntry() = failed\n");
		  rc4 = 1;
		}

      rc0 = 0;
      handlep = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle1);
      if (!handlep)
        {
          sem_wait_if_full (pwait2, pqueue);

          handlep = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle1);
        }
      if (handlep)
        {
          array1 = (int *) SPHLFEntryGetFreePtr (&handle1);
          array1[0] = tv1;
          array1[1] = tv2;
          array1[2] = tv3;
          SPHLFEntryComplete (&handle1);

          sem_post_if_waiting_low_threshold (cwait2, pqueue, val1);
        }
      else
        {
          rc0 = 1;
        }
    }
  else
    {
      return 10;
    }

  return (rc0 | rc1 | rc2 | rc3 | rc4);
}

int
lfPCQentry_waitverify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
		sphPCQAlignedSem_t *pwait, sphPCQAlignedSem_t *cwait,
		int val1, int val2, int val3)
{
  int rc1, rc2, rc3, rc4;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  rc4 = 0;
  handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
  if (!handle)
    {
#if 0
      while (SPHSinglePCQueueEmpty (cqueue))
		{
		  cwait->sem.waiting = 1;
		  cwait->sem.waitcnt += 1;
		  if (sem_wait (&cwait->sem.qlock))
		  {
			  perror ("lfPCQentry_waitverify sem_wait failed");
			  rc4++;
		  }
		}
#else
      sem_wait_if_empty (cwait, cqueue);
#endif
      handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
    }
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      rc1 = (array[0] != val1);
      rc2 = (array[1] != val2);
      rc3 = (array[2] != val3);

      if (rc1 | rc2 | rc3)
		{
		  printf ("lfPCQentry_fastverify:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
			 array[0], array[1], array[2], val1, val2, val3);
		}

      if (SPHSinglePCQueueFreeNextEntry (cqueue))
		{
          sem_post_if_waiting_high_threshold (pwait, cqueue, val1);
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
#undef DEBUG_PRINT

int
lfPCQentry_fasttest (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue, int val1, int val2, int val3)
{
  int rc1 = 0;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
  if (!handle)
    {
      while (SPHSinglePCQueueFull (pqueue))
	{
#if 1
	  sched_yield ();
#endif
	}
      handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
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
lfPCQentry_fastverify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue, int val1, int val2,
                       int val3)
{
  int rc1, rc2, rc3, rc4;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
  if (!handle)
    {
#if 1
      while (SPHSinglePCQueueEmpty (pcqueue))
        {
          sched_yield ();
        }
      handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
#else
      while (&handle0 != SPHSinglePCQueueGetNextComplete (cqueue, &handle0))
        {
        }
      handle = &handle0;
#endif
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
            ("lfPCQentry_fastverify:: SPHLFEntryGetFreePtr() = %d,%d,%d should be %d,%d,%d\n",
             array[0], array[1], array[2] ,val1, val2, val3);
        }

      if (SPHSinglePCQueueFreeNextEntry (cqueue))
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
lfPCQentry_test (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
  if (!handle)
    {
#ifdef DEBUG_PRINT
      char number[128];
      sprintf (number, "lfPCQentry_test(%p, %d) SPHSinglePCQueueFull\n",
	       pcqueue, val1);
      puts (number);
#endif
      while (SPHSinglePCQueueFull (pqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueAllocStrideEntry (pqueue, 1, 2, &handle0);
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
lfPCQentry_verify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
  if (!handle)
    {
#ifdef DEBUG_PRINT
      char number[128];
      sprintf (number, "lfPCQentry_verify(%p, %d) SPHSinglePCQueueEmpty\n",
	       pcqueue, val1);
      puts (number);
#endif
      while (SPHSinglePCQueueEmpty (cqueue))
	{
	  sched_yield ();
	}
      handle = SPHSinglePCQueueGetNextComplete (cqueue, &handle0);
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

      if (SPHSinglePCQueueFreeNextEntry (cqueue))
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
test_thread_Producer_fill (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
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
      rc = lfPCQentry_test (pqueue, cqueue, i, 0x01234567, 0xdeadbeef);
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
test_thread_consumer_verify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
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
      rc = lfPCQentry_verify (pqueue, cqueue, i, 0x01234567, 0xdeadbeef);
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
test_thread_Producer_waitfill (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
			       int thread_ID, long iterations)
{
  int rc, rtn = 0;
  int pqueue_empty;
  long i;
  sphPCQAlignedSem_t *pwait, *cwait;
#ifdef DEBUG_PRINT
  char number[512];
  char *numptr;
  int len;
  sprintf (number, "test_thread_Producer_waitfill(%p, %p, %d, %ld)\n",
		  pqueue, cqueue,
	   thread_ID, iterations);
  puts (number);
#endif

  pwait = producer_sem_list[thread_ID];
  cwait = consumer_sem_list[thread_ID];
#ifdef DEBUG_PRINT
  sprintf (number, "test_thread_Producer_waitfill() sem_wait @%p @%p\n", pwait, cwait);
  puts (number);
#endif
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_waittest (pqueue, cqueue, pwait, cwait, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
			("test_thread_Producer_waitfill SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
			 pqueue);
	  rtn++;
	  break;
	}
    }

  pwait->sem.threshold = 0;
  cwait->sem.threshold = cap;
  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
#ifdef DEBUG_PRINT
  len = sprintf (number, "test_thread_Producer_waitfill() csem.waiting %d empty %d\n",
		  cwait->sem.waiting, pqueue_empty);
  numptr = &number[len];
#endif

  while (!pqueue_empty)
   {

#ifdef DEBUG_PRINT
//  len = sprintf (numptr, "test_thread_Producer_waitfill() not empty csem.waiting %d\n", cwait->sem.waiting);
//  numptr += len;
#endif

	  sem_post_if_waiting (cwait, pqueue, iterations);
	  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
   }

#ifdef DEBUG_PRINT
  sprintf (numptr, "test_thread_Producer_waitfill() exiting\n",
		  cwait->sem.waiting, SPHSinglePCQueueEmpty (pqueue));
  puts (number); fflush(stdout);
#endif
//  sleep(1);
  return rtn;
}

int
test_thread_consumer_waitverify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
  sphPCQAlignedSem_t *pwait, *cwait;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_consumer_waitverify(%p, %p, %d, %ld)\n",
		  pqueue, cqueue,
		  thread_ID, iterations);
  puts (number);
#endif

  pwait = producer_sem_list[thread_ID];
  cwait = consumer_sem_list[thread_ID];
#ifdef DEBUG_PRINT
  sprintf (number, "test_thread_consumer_waitverify() sem_wait @%p @%p\n", pwait, cwait);
  puts (number);
#endif
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_waitverify (pqueue, cqueue, pwait, cwait, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_consumer_waitverify SPHSinglePCQueueGetNextComplete (%p) failed\n",
	     cqueue);
	  rtn++;
	  break;
	}
    }

#ifdef DEBUG_PRINT
  sprintf (number, "test_thread_consumer_waitverify() psem.waiting %d empty %d\n",
		  pwait->sem.waiting, SPHSinglePCQueueEmpty (pqueue));
  puts (number);
  sprintf (number, "test_thread_consumer_waitverify() csem.waiting %d empty %d\n",
		  cwait->sem.waiting, SPHSinglePCQueueEmpty (cqueue));
  puts (number); fflush(stdout);
#endif
  return rtn;
}

int
test_thread_consumer_waittransfer (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  int pqueue_empty;
  long i;
  sphPCQAlignedSem_t *pwait, *cwait;
  sphPCQAlignedSem_t *pwait2, *cwait2;
#ifdef DEBUG_PRINT
  char number[512];
  char *numptr;
  int len;
  sprintf (number, "test_thread_consumer_waittransfer(%p, %p, %d, %ld)\n",
		  pqueue, cqueue,
		  thread_ID, iterations);
  puts (number);
#endif

  pwait = producer_sem_list[thread_ID];
  cwait = consumer_sem_list[thread_ID];
  pwait2 = producer_sem_list[thread_ID+1];
  cwait2 = consumer_sem_list[thread_ID+1];
#ifdef DEBUG_PRINT
  sprintf (number, "test_thread_consumer_waittransfer() sem_wait @%p @%p  sem_wait2 @%p @%p\n",
		  pwait, cwait, pwait2, cwait2);
  puts (number);
#endif
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_waittransfer (pqueue, cqueue, pwait, cwait, pwait2, cwait2, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_consumer_waittransfer SPHSinglePCQueueGetNextComplete (%p) failed\n",
	     cqueue);
	  rtn++;
	  break;
	}
    }

  pwait2->sem.threshold = 0;
  cwait2->sem.threshold = cap;
  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
#ifdef DEBUG_PRINT
  len = sprintf (number, "test_thread_consumer_waittransfer() csem.waiting %d empty %d\n",
		  cwait2->sem.waiting, pqueue_empty);
  numptr = &number[len];
#endif

  while (!pqueue_empty)
   {

#ifdef DEBUG_PRINT
//  len = sprintf (numptr, "test_thread_consumer_waittransfer() not empty csem.waiting %d\n", cwait2->sem.waiting);
//  numptr += len;
#endif

	  sem_post_if_waiting (cwait2, pqueue, iterations);
	  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
   }

#ifdef DEBUG_PRINT
  sprintf (numptr, "test_thread_consumer_waittransfer() exiting\n",
		  cwait2->sem.waiting, SPHSinglePCQueueEmpty (pqueue));
  puts (number); fflush(stdout);
#endif
  return rtn;
}

int
test_thread_consumer_waittransfercopy (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
				 int thread_ID, long iterations)
{
  int rc, rtn = 0;
  int pqueue_empty;
  long i;
  sphPCQAlignedSem_t *pwait, *cwait;
  sphPCQAlignedSem_t *pwait2, *cwait2;
#ifdef DEBUG_PRINT
  char number[512];
  char *numptr;
  int len;
  sprintf (number, "test_thread_consumer_waittransfercopy(%p, %p, %d, %ld)\n",
		  pqueue, cqueue,
		  thread_ID, iterations);
  puts (number);
#endif

  pwait = producer_sem_list[thread_ID];
  cwait = consumer_sem_list[thread_ID];
  pwait2 = producer_sem_list[thread_ID+1];
  cwait2 = consumer_sem_list[thread_ID+1];
#ifdef DEBUG_PRINT
  sprintf (number, "test_thread_consumer_waittransfercopy() sem_wait @%p @%p  sem_wait2 @%p @%p\n",
		  pwait, cwait, pwait2, cwait2);
  puts (number);
#endif
  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_waittransfercopy (pqueue, cqueue, pwait, cwait, pwait2, cwait2, i, 0x01234567, 0xdeadbeef);
      if (!rc)
	{
	}
      else
	{
	  printf
	    ("test_thread_consumer_waittransfercopy SPHSinglePCQueueGetNextComplete (%p) failed\n",
	     cqueue);
	  rtn++;
	  break;
	}
    }

  pwait2->sem.threshold = 0;
  cwait2->sem.threshold = cap;
  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
#ifdef DEBUG_PRINT
  len = sprintf (number, "test_thread_consumer_waittransfercopy() csem.waiting %d empty %d\n",
		  cwait2->sem.waiting, pqueue_empty);
  numptr = &number[len];
#endif

  while (!pqueue_empty)
   {

#ifdef DEBUG_PRINT
//  len = sprintf (numptr, "test_thread_consumer_waittransfercopy() not empty csem.waiting %d\n", cwait2->sem.waiting);
//  numptr += len;
#endif

	  sem_post_if_waiting (cwait2, pqueue, iterations);
	  pqueue_empty = SPHSinglePCQueueEmpty (pqueue);
   }

#ifdef DEBUG_PRINT
  sprintf (numptr, "test_thread_consumer_waittransfercopy() exiting\n",
		  cwait2->sem.waiting, SPHSinglePCQueueEmpty (pqueue));
  puts (number); fflush(stdout);
#endif
  return rtn;
}
//#undef DEBUG_PRINT

int
test_thread_Producer_fastfill (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
                               int thread_ID, long iterations)
{
  int rc, rtn = 0;
  long i;
#ifdef DEBUG_PRINT
  char number[128];
  sprintf (number, "test_thread_Producer_fastfill(%p, %d, %ld)\n", pcqueue,
           thread_ID, iterations);
  puts (number);
#endif

  for (i = 0; i < iterations; i++)
    {
      rc = lfPCQentry_fasttest (pqueue, cqueue, i, 0x01234567, 0xdeadbeef);
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
test_thread_consumer_fastverify (SPHSinglePCQueue_t pqueue, SPHSinglePCQueue_t cqueue,
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
      rc = lfPCQentry_fastverify (pqueue, cqueue, i, 0x01234567, 0xdeadbeef);
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
  SPHSinglePCQueue_t pqueue, cqueue;
  test_fill_ptr_t test_func;
  long result = 0;
  int tn = (int) (long int) arg;

  pqueue = producer_pcq_list[tn];
  cqueue = consumer_pcq_list[tn];
  test_func = test_funclist[tn];

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

  pcqueue = SPHSinglePCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue)
    {
      rc = SPHSinglePCQueueSetCachePrefetch (pcqueue, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);

      cap = SPHSinglePCQueueFreeSpace (pcqueue);

      units = cap / 128;

      printf ("SPHSinglePCQueueFreeSpace() = %zu units=%zu\n", cap, units);

      memset (pwait.unit, 0, 128);
      memset (cwait.unit, 0, 128);
      rc -= sem_init (&pwait.sem.qlock, 0, 0);
      rc -= sem_init (&cwait.sem.qlock, 0, 0);
    }
  else
    rc++;

  pcqueue2 = SPHSinglePCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue2)
    {
      rc = SPHSinglePCQueueSetCachePrefetch (pcqueue2, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);

      memset (pwait2.unit, 0, 128);
      memset (cwait2.unit, 0, 128);
      rc -= sem_init (&pwait2.sem.qlock, 0, 0);
      rc -= sem_init (&cwait2.sem.qlock, 0, 0);
    }
  else
    rc++;

  pcqueue3 = SPHSinglePCQueueCreateWithStride (pcq_alloc, pcq_stride);
  if (pcqueue3)
    {
      rc = SPHSinglePCQueueSetCachePrefetch (pcqueue3, 0);
      printf ("\nSPHLFpcqueueCreate (%zu) success \n", pcq_alloc);

      memset (pwait3.unit, 0, 128);
      memset (cwait3.unit, 0, 128);
      rc -= sem_init (&pwait3.sem.qlock, 0, 0);
      rc -= sem_init (&cwait3.sem.qlock, 0, 0);
    }
  else
    rc++;

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

      memset (pwait.unit, 0, 128);
      memset (cwait.unit, 0, 128);
      rc -= sem_init (&pwait.sem.qlock, 0, 0);
      rc -= sem_init (&cwait.sem.qlock, 0, 0);
    }
  else
    rc++;

  rc = SPHSinglePCQueueResetAsync (pcqueue2);
  if (!rc)
    {
      printf ("\nSPHSinglePCQueueResetAsync (%p) success \n", pcqueue2);

      memset (pwait2.unit, 0, 128);
      memset (cwait2.unit, 0, 128);
      rc -= sem_init (&pwait2.sem.qlock, 0, 0);
      rc -= sem_init (&cwait2.sem.qlock, 0, 0);
    }
  else
    rc++;

  rc = SPHSinglePCQueueResetAsync (pcqueue3);
  if (!rc)
	{
      printf ("\nSPHSinglePCQueueResetAsync (%p) success \n", pcqueue3);

      memset (pwait3.unit, 0, 128);
      memset (cwait3.unit, 0, 128);
      rc -= sem_init (&pwait3.sem.qlock, 0, 0);
      rc -= sem_init (&cwait3.sem.qlock, 0, 0);
	    }
	  else
    rc++;

  return rc;
}

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

      return (1);
	}
      else
	{
      printf ("SAS Joined with %d processors\n", N_PROC_CONF);

      num_threads = 2;
      rc = test_pcq_init (4096);

      producer_pcq_list[0] = pcqueue;
      consumer_pcq_list[1] = pcqueue;
      test_funclist[0] = test_thread_Producer_fill;
      test_funclist[1] = test_thread_consumer_verify;
      if (rc == 0)
	{
#if 1
	  p10 = units * ITERATIONS;
	  printf ("start 1 test_thread_Producer | consumer (%p,%zu)\n",
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
	  printf ("end 1   test_thread_Producer | consumer (%p,%zu) = %d\n",
		  pcqueue, units, rc);

#endif
#if 1
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
    {
	      producer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      test_funclist[0] = test_thread_Producer_fastfill;
	      test_funclist[1] = test_thread_consumer_fastverify;
	      p10 = units * ITERATIONS;

	      printf ("start 2 test_thread_Producer | consumer fast (%p,%zu)\n",
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
		("end 2   test_thread_Producer | consumer fast (%p,%zu) = %d\n",
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
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;

	      sem_data_init(&pwait, 0, 0);
	      sem_data_init(&cwait, 0, cap);

	      p10 = units * ITERATIONS;

	      printf ("start 3 test_thread_Producer | consumer wait (%p,%zu)\n",
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
		("test_thread_Producer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		 p10, nano, rate);
	      printf
		("end 3   test_thread_Producer | consumer wait (%p,%zu) = %d\n",
		 pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);
    }
  else
	    rc += rc3;

	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;

	      sem_data_init(&pwait, 0, cap / 2);
	      sem_data_init(&cwait, 0, cap / 2);

	      p10 = units * ITERATIONS;

	      printf ("start 4 test_thread_Producer | consumer wait (%p,%zu)\n",
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
		("test_thread_Producer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		 p10, nano, rate);
	      printf
		("end 4   test_thread_Producer | consumer wait (%p,%zu) = %d\n",
		 pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);
    }
  else
	    rc += rc3;
#endif
#if 1
      num_threads = 3;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfer;
	      test_funclist[2] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;

	      sem_data_init(&pwait, 0, 0);
	      sem_data_init(&cwait, 0, cap);
	      sem_data_init(&pwait2, 0, 0);
	      sem_data_init(&cwait2, 0, cap);

	      p10 = units * ITERATIONS;

	      printf ("start 5 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		          p10, nano, rate);
	      printf
		    ("end 5   test_thread_Producer | transfer | consumer wait (%p,%zu) = %d\n",
		          pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);
    }
	  else
	    rc += rc3;

      num_threads = 3;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfer;
	      test_funclist[2] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;

	      sem_data_init(&pwait, 0, cap / 2);
	      sem_data_init(&cwait, 0, cap / 2);
	      sem_data_init(&pwait2, 0, cap / 2);
	      sem_data_init(&cwait2, 0, cap / 2);

	      p10 = units * ITERATIONS;

	      printf ("start 6 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		          p10, nano, rate);
	      printf
		    ("end 6   test_thread_Producer | transfer | consumer wait (%p,%zu) = %d\n",
		          pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);
	    }
	  else
	    rc += rc3;

      num_threads = 4;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue3;
	      producer_pcq_list[3] = pcqueue3;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[3] = pcqueue3;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfer;
	      test_funclist[2] = test_thread_consumer_waittransfer;
	      test_funclist[3] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      producer_sem_list[3] = &pwait3;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;
	      consumer_sem_list[3] = &cwait3;

	      sem_data_init(&pwait, 0, cap / 2);
	      sem_data_init(&cwait, 0, cap / 2);
	      sem_data_init(&pwait2, 0, cap / 2);
	      sem_data_init(&cwait2, 0, cap / 2);
	      sem_data_init(&pwait3, 0, cap / 2);
	      sem_data_init(&cwait3, 0, cap / 2);

	      p10 = units * ITERATIONS;

	      printf ("start 7 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		          p10, nano, rate);
	      printf
		    ("end 7   test_thread_Producer | transfer | consumer wait (%p,%zu) = %d\n",
		          pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);

	      printf
		("Producer3 wait/post %d/%d consumer3 wait/post %d/%d\n",
				  pwait3.sem.waitcnt,
				  pwait3.sem.postcnt,
				  cwait3.sem.waitcnt,
				  cwait3.sem.postcnt);
	}
      else
	    rc += rc3;

      num_threads = 4;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue3;
	      producer_pcq_list[3] = pcqueue3;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[3] = pcqueue3;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfer;
	      test_funclist[2] = test_thread_consumer_waittransfer;
	      test_funclist[3] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      producer_sem_list[3] = &pwait3;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;
	      consumer_sem_list[3] = &cwait3;
	      sem_data_init(&pwait, 2, cap / 2);
	      sem_data_init(&cwait, 2, cap / 2);
	      sem_data_init(&pwait2, 2, cap / 2);
	      sem_data_init(&cwait2, 2, cap / 2);
	      sem_data_init(&pwait3, 2, cap / 2);
	      sem_data_init(&cwait3, 2, cap / 2);
	      p10 = units * ITERATIONS;

	      printf ("start 8 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		          p10, nano, rate);
	      printf
		    ("end 8   test_thread_Producer | transfer | consumer wait (%p,%zu) = %d\n",
		          pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);

	      printf
		("Producer3 wait/post %d/%d consumer3 wait/post %d/%d\n",
				  pwait3.sem.waitcnt,
				  pwait3.sem.postcnt,
				  cwait3.sem.waitcnt,
				  cwait3.sem.postcnt);
    }
  else
	    rc += rc3;

      num_threads = 4;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	{
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue3;
	      producer_pcq_list[3] = pcqueue3;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[3] = pcqueue3;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfer;
	      test_funclist[2] = test_thread_consumer_waittransfer;
	      test_funclist[3] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      producer_sem_list[3] = &pwait3;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;
	      consumer_sem_list[3] = &cwait3;

	      sem_data_init(&pwait, 5, cap / 2);
	      sem_data_init(&cwait, 5, cap / 2);
	      sem_data_init(&pwait2, 5, cap / 2);
	      sem_data_init(&cwait2, 5, cap / 2);
	      sem_data_init(&pwait3, 5, cap / 2);
	      sem_data_init(&cwait3, 5, cap / 2);
	  p10 = units * ITERATIONS;

	      printf ("start 9 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfer | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
	     p10, nano, rate);
	      printf
		    ("end 9   test_thread_Producer | transfer | consumer wait (%p,%zu) = %d\n",
		  pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);

	      printf
		("Producer3 wait/post %d/%d consumer3 wait/post %d/%d\n",
				  pwait3.sem.waitcnt,
				  pwait3.sem.postcnt,
				  cwait3.sem.waitcnt,
				  cwait3.sem.postcnt);
	    }
	  else
	    rc += rc3;

      num_threads = 4;
	  rc3 = test_pcq_reset ();
	  if (rc3 == 0)
	    {
	      producer_pcq_list[0] = pcqueue;
	      producer_pcq_list[1] = pcqueue2;
	      producer_pcq_list[2] = pcqueue3;
	      producer_pcq_list[3] = pcqueue3;
	      consumer_pcq_list[0] = pcqueue;
	      consumer_pcq_list[1] = pcqueue;
	      consumer_pcq_list[2] = pcqueue2;
	      consumer_pcq_list[3] = pcqueue3;
	      test_funclist[0] = test_thread_Producer_waitfill;
	      test_funclist[1] = test_thread_consumer_waittransfercopy;
	      test_funclist[2] = test_thread_consumer_waittransfercopy;
	      test_funclist[3] = test_thread_consumer_waitverify;
	      producer_sem_list[0] = &pwait;
	      producer_sem_list[1] = &pwait;
	      producer_sem_list[2] = &pwait2;
	      producer_sem_list[3] = &pwait3;
	      consumer_sem_list[0] = &cwait;
	      consumer_sem_list[1] = &cwait;
	      consumer_sem_list[2] = &cwait2;
	      consumer_sem_list[3] = &cwait3;

	      sem_data_init(&pwait, 2, cap / 2);
	      sem_data_init(&cwait, 2, cap / 2);
	      sem_data_init(&pwait2, 2, cap / 2);
	      sem_data_init(&cwait2, 2, cap / 2);
	      sem_data_init(&pwait3, 2, cap / 2);
	      sem_data_init(&cwait3, 2, cap / 2);

	      p10 = units * ITERATIONS;

	      printf ("start 10 test_thread_Producer | transfer | consumer wait (%p,%zu)\n",
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
		    ("test_thread_Producer | transfercopy | consumer wait %zu ave= %6.2fns rate=%10.1f/s\n",
		 p10, nano, rate);
	      printf
		    ("end 10   test_thread_Producer | transfercopy | consumer wait (%p,%zu) = %d\n",
		 pcqueue, units, rc);
	      printf
		("Producer wait/post %d/%d consumer wait/post %d/%d\n",
			      pwait.sem.waitcnt,
			      pwait.sem.postcnt,
			      cwait.sem.waitcnt,
			      cwait.sem.postcnt);

	      printf
		("Producer2 wait/post %d/%d consumer2 wait/post %d/%d\n",
				  pwait2.sem.waitcnt,
				  pwait2.sem.postcnt,
				  cwait2.sem.waitcnt,
				  cwait2.sem.postcnt);

	      printf
		("Producer3 wait/post %d/%d consumer3 wait/post %d/%d\n",
				  pwait3.sem.waitcnt,
				  pwait3.sem.postcnt,
				  cwait3.sem.waitcnt,
				  cwait3.sem.postcnt);
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
