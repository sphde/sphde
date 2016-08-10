/*
 * Copyright (c) 2011 IBM Corporation.
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
#include <pthread.h>
#include "sasshm.h"
#include "sasconf.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "saslock.h"
#ifdef __SOMTEST__
#include "somdef.h"
#include "somcls.h"
#endif
#include "sphcontext.h"
#include "sphlflogentry.h"
#include "sphlflogger.h"
#include "sphthread.h"
#include "sphtimer.h"

#ifdef LONGCHECK
# define ITERATIONS 100000000
#else
# define ITERATIONS 10000000
#endif

static int N_PROC_CONF = 1;
static int num_threads = 1;
static long thread_iterations;

static const int max_threads = 256;

static	SPHLFLogger_t logger;
static	SPHLFLogger_t log_lst[256];

static	block_size_t cap, units, p10, log_alloc;

typedef void* (*test_ptr_t)(void*);
typedef int (*test_fill_ptr_t)(SPHLFLogger_t, int, long);
static test_fill_ptr_t test_func;

//#define DEBUG_PRINT 1
static void *
fill_test_parallel_thread (void *arg)
{
	SPHLFLogger_t logger;
    long result = 0;
    int tn = (int) (long int) arg;
#ifdef DEBUG_PRINT
    pid_t tid = sphFastGetTID();
    char number[128];
#endif

    logger = log_lst[tn];

    SASThreadSetUp ();
#ifdef DEBUG_PRINT
    sprintf(number, "ltt(%d, %d, @%p): begin", tn, tid, logger);
    puts (number);
#endif

    result += (*test_func)(logger, tn, thread_iterations);

#ifdef DEBUG_PRINT
    sprintf(number, "ltt(%d, %d): end", tn, tid);
    puts (number);
#endif
    SASThreadCleanUp ();

    return (void*)result;
}

static void *
fill_test_thread (void *arg)
{
    int tn = (int) (long int) arg;
    long result = 0;
#ifdef DEBUG_PRINT
    pid_t tid = sphFastGetTID();
    char number[128];
#endif

    SASThreadSetUp ();
#ifdef DEBUG_PRINT
    sprintf(number, "ltt(%d, %d): begin", tn, tid);
    puts (number);
#endif

    result += (*test_func)(logger, tn, thread_iterations);

#ifdef DEBUG_PRINT
    sprintf(number, "ltt(%d, %d): end", tn, tid);
    puts (number);
#endif
    SASThreadCleanUp ();

    return (void*)result;
}

static int 
launch_test_threads (int t_cnt, test_ptr_t test_f,
			int iterations)
{
  long int n;
  pthread_t th[max_threads];
  long thread_result;
  int result = 0;
#ifdef DEBUG_PRINT
  int tid;
  int pid;

  pid = getpid();
  tid = sphdeGetTID();
#endif

  thread_iterations = iterations / t_cnt;
#ifdef DEBUG_PRINT
  printf("creating thread from pid/tid = %d/%d\n", 
	pid, tid);
#endif

  for (n = 0; n < t_cnt; ++n)
	{
		void *arg;
		arg = (void*)n;
		if (pthread_create (&th[n], NULL, test_f, arg) != 0)
		{
			puts ("create failed");
			exit (1);
		}
	}
#ifdef DEBUG_PRINT
  puts("after create");
#endif

  for (n = 0; n < t_cnt; ++n)
    if (pthread_join (th[n], (void**)&thread_result) != 0)
      {
	puts ("join failed");
	exit (2);
      } else {
	result += thread_result;
      }

#ifdef DEBUG_PRINT
  puts("after join");
#endif
  return result;
}

int echo (char * fileName)
{
    int	rc = 0;
    int	c = 0;
    FILE *fp;
    fp = fopen(fileName, "r");

    if (fp > 0)
    {
	while ( c != EOF )
	{
	    c = fgetc (fp);
	    if ( c != EOF )
		putchar(c);
	}
	fclose(fp);
    } else {
	perror("Open failed ..\n");
	rc = errno;
    }
    return rc;
}

void fdisplayProcMaps (__pid_t pid)
{	
    char buff[32];
    sprintf(buff, "/proc/%d/maps", pid);
    echo (buff);
}

void displayProcMaps (char* pid)
{	
    char buff[32];
    sprintf(buff, "/proc/%s/maps", pid);
    echo (buff);
}

void displayMyProcMaps ()
{	
    __pid_t pid;
    pid = getpid();
    fdisplayProcMaps(pid);
}

int
lflogentry_test_handle (SPHLFLogger_t log, 
			SPHLFLoggerHandle_t *handle_in,
			int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             handle_in);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(handle, val1);
	    rc2 = SPHLFlogEntryAddInt(handle, val2);
	    rc3 = SPHLFlogEntryAddInt(handle, val3);
	    SPHLFLogEntryComplete(handle);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_test (SPHLFLogger_t log, int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle, handle0;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(&handle0, val1);
	    rc2 = SPHLFlogEntryAddInt(&handle0, val2);
	    rc3 = SPHLFlogEntryAddInt(&handle0, val3);
	    SPHLFLogEntryComplete(&handle0);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_test_strong (SPHLFLogger_t log, int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle, handle0;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(&handle0, val1);
	    rc2 = SPHLFlogEntryAddInt(&handle0, val2);
	    rc3 = SPHLFlogEntryAddInt(&handle0, val3);
	    SPHLFLogEntryStrongComplete(&handle0);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_test_weak (SPHLFLogger_t log, int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle, handle0;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(&handle0, val1);
	    rc2 = SPHLFlogEntryAddInt(&handle0, val2);
	    rc3 = SPHLFlogEntryAddInt(&handle0, val3);
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_fasttest (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetest (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStamped (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetest_strong (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStamped (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryStrongComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetest_weak (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStamped (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetest_nolock (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStampedNoLock (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}


int
test_log_cleanup(void)
{
	int rc, rtn = 0;	
					
	rc = SPHLFLoggerDestroy( logger );
	printf("SPHLFLoggerDestroy(%p) = %d\n",
			logger, rc);

	logger = NULL;

	return rtn;
}

int
test_thread_log_verify(SPHLFLogger_t logger)
{
	int	tempn_i, temp0_i, temp1_i;
	unsigned int temp2_i;
	SPHLFLoggerHandle_t *handlex, handle5;
	sphpid16_t entry_pid, entry_tid;
	SPHLFLogIterator_t *iter, iter0;
	sphtimer_t entry_timestamp;
	int rtn = 0;	
	long i;

	int		t_PID[max_threads];
	int		t_TID[max_threads];
	int		t_seq[max_threads];
	sphtimer_t	t_tts[max_threads];

#ifdef SPH_TIMERTEST_VERIFY
	for (i = 0; i < num_threads; i++)
	{
		t_PID[i] = -1;
		t_TID[i] = -1;
		t_seq[i] = -1;
		t_tts[i] = 0LL;
	}

	printf("\ntest_thread_log_verify() verify log contents\n");
	iter = SPHLFLoggerCreateIterator(logger, &iter0);
	if (iter)
	{
		printf("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
				logger, &iter0, iter);
	} else {
		printf("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
				logger, &iter0, iter);
		return (rtn + 10);
	}

	do
	{
		handlex = SPHLFLoggerIteratorNext (iter, &handle5);
		if (handlex)
		{
			entry_pid = SPHLFLogEntryPID(handlex);
			entry_tid = SPHLFLogEntryTID(handlex);
			entry_timestamp = SPHLFLogEntryTimeStamp (handlex);

			temp0_i = SPHLFlogEntryGetNextInt(handlex);
			temp1_i = SPHLFlogEntryGetNextInt(handlex);
			temp2_i = SPHLFlogEntryGetNextInt(handlex);
			if ((temp1_i < num_threads)
			&&  (temp2_i == 0xdeadbeef))
			{
				tempn_i = t_seq[temp1_i];
				if (tempn_i == -1)
				{
					t_PID[temp1_i] = entry_pid;
					t_TID[temp1_i] = entry_tid;
					t_seq[temp1_i] = temp0_i;
					t_tts[temp1_i] = entry_timestamp;
				} else {
					if ((temp0_i != (tempn_i + 1))
					||  (t_PID[temp1_i] != entry_pid)
					||  (t_TID[temp1_i] != entry_tid)
					||  (entry_timestamp < t_tts[temp1_i]))
					{
						printf("  SPHLFLoggerIteratorNext() seq mismatch  %d,%d,%x, %llu,%llu\n",
						   temp0_i, temp1_i, temp2_i,
						   t_tts[temp1_i], entry_timestamp);
						rtn++;
					}
					t_seq[temp1_i] = temp0_i;
					t_tts[temp1_i] = entry_timestamp;
				}
			} else {
				printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x\n",
						   temp0_i, temp1_i, temp2_i);
				rtn++;
			}
		}
	} while (handlex);
	
	printf("test_thread_log_verify() verify log complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */
	return rtn;
}

int
test_thread_log_fill(SPHLFLogger_t logger,
					int thread_ID, long iterations)
{
	int rc, rtn = 0;	
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_test (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("SPHLFLoggerAllocTimeStamped (%p) failed\n", 
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_thread_log_fill_strong(SPHLFLogger_t logger,
					int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_test_strong (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_thread_log_fill_weak(SPHLFLogger_t logger,
					int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_test_weak (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_log_init(block_size_t log_size)
{
	int rc = 0;	
	//log_alloc = (4*1024*1024);
	log_alloc = log_size;
	
	logger = SPHLFLoggerCreate (log_alloc);
	if ( logger )
	{
		rc = SPHLFLoggerPrefetch(logger);
		printf("\nSPHLFLoggerCreate (%zu) success \n", log_alloc);

		cap = SPHLFLoggerFreeSpace( logger );
			
		units = cap / 128;
			
		printf("SPHLFLoggerFreeSpace() = %zu units=%zu\n",
			cap, units);
	} else	rc = 10;

	return rc;
}

int
test_log_reset()
{
	int rc = 0;

	rc = SPHLFLoggerResetAsync (logger);
	if ( !rc )
	{
		printf("\nSPHLFLoggerResetAsync (%p) success \n", logger);

		cap = SPHLFLoggerFreeSpace( logger );

		units = cap / 128;

		printf("SPHLFLoggerFreeSpace() = %zu units=%zu\n",
			cap, units);
	} else	rc = 10;

	return rc;
}

int
test_thread_wraplog_verify(SPHLFLogger_t logger)
{
	int	tempn_i, temp0_i, temp1_i;
	unsigned int temp2_i;
	SPHLFLoggerHandle_t *handlex, handle5;
	sphpid16_t entry_pid, entry_tid;
	SPHLFLogIterator_t *iter, iter0;
	int	*tarray;
	sphtimer_t entry_timestamp;
	int rtn = 0;	
	long i;

	int		t_PID[max_threads];
	int		t_TID[max_threads];
	int		t_seq[max_threads];
	sphtimer_t	t_tts[max_threads];

#ifdef SPH_TIMERTEST_VERIFY
	for (i = 0; i < num_threads; i++)
	{
		t_PID[i] = -1;
		t_TID[i] = -1;
		t_seq[i] = -1;
		t_tts[i] = 0LL;
	}

	printf("\ntest_thread_wraplog_verify() verify log contents\n");
	iter = SPHLFLoggerCreateIterator(logger, &iter0);
	if (iter)
	{
		printf("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
				logger, &iter0, iter);
	} else {
		printf("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
				logger, &iter0, iter);
		return (rtn + 10);
	}

	do
	{
		handlex = SPHLFLoggerIteratorNext (iter, &handle5);
		if (handlex)
		{
			entry_pid = SPHLFLogEntryPID(handlex);
			entry_tid = SPHLFLogEntryTID(handlex);
			entry_timestamp = SPHLFLogEntryTimeStamp (handlex);

			tarray = (int*)SPHLFLogEntryGetFreePtr(handlex);
			temp0_i = tarray[0];
			temp1_i = tarray[1];
			temp2_i = tarray[2];
			if ((temp1_i < num_threads)
			&&  (temp2_i == 0xdeadbeef))
			{
				tempn_i = t_seq[temp1_i];
				if (tempn_i == -1)
				{
					t_PID[temp1_i] = entry_pid;
					t_TID[temp1_i] = entry_tid;
					t_seq[temp1_i] = temp0_i;
					t_tts[temp1_i] = entry_timestamp;
				} else {
					if ((temp0_i != (tempn_i + 1))
					||  (t_PID[temp1_i] != entry_pid)
					||  (t_TID[temp1_i] != entry_tid)
					||  (entry_timestamp < t_tts[temp1_i]))
					{
						printf("  SPHLFLoggerIteratorNext() seq mismatch  %d,%d,%x, %llu,%llu\n",
							   temp0_i, temp1_i, temp2_i,
							   t_tts[temp1_i], entry_timestamp);
						rtn++;
					}
					t_seq[temp1_i] = temp0_i;
					t_tts[temp1_i] = entry_timestamp;
				}
			} else {
				printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x\n",
						temp0_i, temp1_i, temp2_i);
				rtn++;
			}
		}
	} while (handlex);
	
	printf("test_thread_wraplog_verify() verify log complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */

	return rtn;
}

int
test_thread_wraplog_fill(SPHLFLogger_t logger,
						int thread_ID, long iterations)
{
	int rc, rtn = 0;	
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_stridetest (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("test_thread_wraplog_fill thread=%d, iteration=%ld\n",
					thread_ID, i);
			printf("SPHLFLoggerAllocStrideTimeStamped (%p) failed\n",
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_thread_wraplog_fill_strong(SPHLFLogger_t logger,
						int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_stridetest_strong (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("test_thread_wraplog_fill_strong thread=%d, iteration=%ld\n",
					thread_ID, i);
			printf("SPHLFLoggerAllocStrideTimeStamped (%p) failed\n",
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_thread_wraplog_fill_nolock(SPHLFLogger_t logger,
						int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_stridetest_nolock (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("test_thread_wraplog_fill_nolock thread=%d, iteration=%ld\n",
				thread_ID, i);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_thread_wraplog_fill_weak(SPHLFLogger_t logger,
						int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;

	for ( i = 0; i < iterations; i++ )
	{
		rc = lflogentry_stridetest_weak (logger, i, thread_ID, 0xdeadbeef);
		if ( !rc )
		{
		} else {
			printf("test_thread_wraplog_fill_weak thread=%d, iteration=%ld\n",
					thread_ID, i);
			printf("SPHLFLoggerAllocStrideTimeStamped (%p) failed\n",
				logger);
			rtn++;
			break;
		}
	}

	return rtn;
}

int
test_wraplog_init(block_size_t log_size)
{
	int rc = 0;	
	//log_alloc = (4*1024*1024);
	log_alloc = log_size;
	

	logger = SPHLFCircularLoggerCreate (log_alloc, 128);
	if ( logger )
	{
		rc = SPHLFLoggerPrefetch(logger);
		printf("\nSPHLFCirgularLoggerCreate (%zu) success \n", log_alloc);
		
		cap = SPHLFLoggerFreeSpace( logger );
		units = cap / 128;
		
		printf("SPHLFLoggerFreeSpace() = %zu, units=%zu\n",
		       cap, units);
	} else	rc = 10;

	return rc;
}

int
test_wraplog_reset()
{
	int rc = 0;


	rc = SPHLFLoggerResetAsync (logger);
	if ( !rc )
	{
		printf("\nSPHLFLoggerResetAsync (%p) success \n", logger);

		cap = SPHLFLoggerFreeSpace( logger );
		units = cap / 128;

		printf("SPHLFLoggerFreeSpace() = %zu, units=%zu\n",
		       cap, units);
	} else	rc = 10;

	return rc;
}

int
test_thread_wraplog_parallel_verify(void)
{

	int rtn = 0;
	int i;

	for (i = 0; i < num_threads; i++)
	{
		rtn += test_thread_wraplog_verify( log_lst[i] );
	}

	return rtn;
}

int
test_log_parallel_cleanup(void)
{
	int rc, rtn = 0;
	int i;

	for (i = 0; i < num_threads; i++)
	{
		rc = SPHLFLoggerDestroy( log_lst[i] );
		printf("SPHLFLoggerDestroy(%p) = %d\n",
				log_lst[i], rc);

		log_lst[i] = NULL;
	}

	return rtn;
}

int
test_wraplog_parallel_init(block_size_t log_size)
{
	int rc = 0;
	int i;
	//log_alloc = (4*1024*1024);
	log_alloc = log_size;

	for (i = 0; i < num_threads; i++)
	{
		log_lst[i] = SPHLFCircularLoggerCreate (log_alloc, 128);
		if ( log_lst[i] )
		{
			rc = SPHLFLoggerPrefetch(log_lst[i]);
			printf("\nSPHLFCirgularLoggerCreate (%zu) success \n", log_alloc);

			cap = SPHLFLoggerFreeSpace( log_lst[i] );
			units = cap / 128;

			printf("SPHLFLoggerFreeSpace() = %zu, units=%zu\n",
				   cap, units);
		} else	rc += 10;
	}

	return rc;
}

int
test_wraplog_parallel_reset()
{
	int rc = 0;
	int i;

	for (i = 0; i < num_threads; i++)
	{
		rc = SPHLFLoggerResetAsync (log_lst[i]);
		if ( !rc )
		{
			printf("\nSPHLFLoggerResetAsync (%p) success \n", log_lst[i]);

			cap = SPHLFLoggerFreeSpace( log_lst[i] );
			units = cap / 128;

			printf("SPHLFLoggerFreeSpace() = %zu, units=%zu\n",
				   cap, units);
		} else	rc += 10;
	}

	return rc;
}


int main ()
{
    int	rc, rc2, rc3;
    double clock, nano, rate;
    sphtimer_t	tempt, startt, endt, freqt;

	N_PROC_CONF = sysconf(_SC_NPROCESSORS_ONLN);
	if (N_PROC_CONF < 8)
		num_threads = N_PROC_CONF;
	else
		num_threads = 8;

    SAS_IO_INIT		// init the io stuff

    rc = SASJoinRegion();

    if (rc)
    {
		printf("SASJoinRegion Error# %d\n", rc);

		return 1;
    } else {
		printf("SAS Joined with %d processors\n", 
			N_PROC_CONF);
	
		rc = test_log_init(SegmentSize);
		if (rc == 0)
		{
			p10 = units;
			test_func = test_thread_log_fill;
			printf("start test_thread_log_fill (%p,%zu)\n",
				logger, units);

			startt = sphgettimer();
			rc += launch_test_threads (num_threads, fill_test_thread, p10);
			endt = sphgettimer();
			tempt = endt -startt;
			clock = tempt;
			freqt = sphfastcpufreq();
			nano = (clock * 1000000000.0) / (double)freqt;
			nano = nano / p10;
			rate = p10 / (clock / (double)freqt);
				
			printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
				startt, endt, tempt, freqt);
	
			printf ("lflogentry_test X %zu ave= %6.2fns rate=%10.1f/s\n",
				p10, nano, rate);
			printf("end   test_thread_log_fill (%p,%zu) = %d\n",
				logger, units, rc);
#ifdef SPH_TIMERTEST_VERIFY
			rc+= test_thread_log_verify(logger);
#endif

			rc3 = test_log_reset();
			if (rc3 == 0)
			{
				p10 = units;
				test_func = test_thread_log_fill_strong;
				printf("start test_thread_log_fill_strong (%p,%zu)\n",
					logger, units);

				startt = sphgettimer();
				rc += launch_test_threads (num_threads, fill_test_thread, p10);
				endt = sphgettimer();
				tempt = endt -startt;
				clock = tempt;
				freqt = sphfastcpufreq();
				nano = (clock * 1000000000.0) / (double)freqt;
				nano = nano / p10;
				rate = p10 / (clock / (double)freqt);

				printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
					startt, endt, tempt, freqt);

				printf ("lflogentry_test_strong X %zu ave= %6.2fns rate=%10.1f/s\n",
					p10, nano, rate);
				printf("end   test_thread_log_fill_strong (%p,%zu) = %d\n",
					logger, units, rc);
	#ifdef SPH_TIMERTEST_VERIFY
				rc+= test_thread_log_verify(logger);
	#endif
			}

			rc3 = test_log_reset();
			if (rc3 == 0)
			{
				p10 = units;
				test_func = test_thread_log_fill_weak;
				printf("start test_thread_log_fill_weak (%p,%zu)\n",
					logger, units);

				startt = sphgettimer();
				rc += launch_test_threads (num_threads, fill_test_thread, p10);
				endt = sphgettimer();
				tempt = endt -startt;
				clock = tempt;
				freqt = sphfastcpufreq();
				nano = (clock * 1000000000.0) / (double)freqt;
				nano = nano / p10;
				rate = p10 / (clock / (double)freqt);

				printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
					startt, endt, tempt, freqt);

				printf ("lflogentry_test_weak X %zu ave= %6.2fns rate=%10.1f/s\n",
					p10, nano, rate);
				printf("end   test_thread_log_fill_weak (%p,%zu) = %d\n",
					logger, units, rc);
	#ifdef SPH_TIMERTEST_VERIFY
				rc+= test_thread_log_verify(logger);
	#endif
			}


			rc += test_log_cleanup();
		}
	
		rc2 = test_wraplog_init(SegmentSize);
		if (rc2 == 0)
		{
			p10 = ITERATIONS;
			test_func = test_thread_wraplog_fill;
			printf("start test_thread_wraplog_fill (%p,%zu)\n",
				logger, units);

			startt = sphgettimer();
			rc += launch_test_threads (num_threads, fill_test_thread, p10);
			endt = sphgettimer();
			tempt = endt -startt;
			clock = tempt;
			freqt = sphfastcpufreq();
			nano = (clock * 1000000000.0) / (double)freqt;
			nano = nano / p10;
			rate = p10 / (clock / (double)freqt);
				
			printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
				startt, endt, tempt, freqt);
	
			printf ("lflogentry_test X %zu ave= %6.2fns rate=%10.1f/s\n",
				p10, nano, rate);
			printf("end   test_thread_wraplog_fill (%p,%zu) = %d\n",
				logger, units, rc);

#ifdef SPH_TIMERTEST_VERIFY
			rc+= test_thread_wraplog_verify(logger);
#endif

			rc3 = test_wraplog_reset();
			if (rc3 == 0)
			{
				p10 = ITERATIONS;
				test_func = test_thread_wraplog_fill_strong;
				printf("start test_thread_wraplog_fill_strong (%p,%zu)\n",
					logger, units);

				startt = sphgettimer();
				rc += launch_test_threads (num_threads, fill_test_thread, p10);
				endt = sphgettimer();
				tempt = endt -startt;
				clock = tempt;
				freqt = sphfastcpufreq();
				nano = (clock * 1000000000.0) / (double)freqt;
				nano = nano / p10;
				rate = p10 / (clock / (double)freqt);

				printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
					startt, endt, tempt, freqt);

				printf ("lflogentry_test_strong X %zu ave= %6.2fns rate=%10.1f/s\n",
					p10, nano, rate);
				printf("end   test_thread_wraplog_fill_strong (%p,%zu) = %d\n",
					logger, units, rc);

	#ifdef SPH_TIMERTEST_VERIFY
				rc+= test_thread_wraplog_verify(logger);
	#endif
			}

			rc3 = test_wraplog_reset();
			if (rc3 == 0)
			{
				p10 = ITERATIONS;
				test_func = test_thread_wraplog_fill_weak;
				printf("start test_thread_wraplog_fill_weak (%p,%zu)\n",
					logger, units);

				startt = sphgettimer();
				rc += launch_test_threads (num_threads, fill_test_thread, p10);
				endt = sphgettimer();
				tempt = endt -startt;
				clock = tempt;
				freqt = sphfastcpufreq();
				nano = (clock * 1000000000.0) / (double)freqt;
				nano = nano / p10;
				rate = p10 / (clock / (double)freqt);

				printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
					startt, endt, tempt, freqt);

				printf ("lflogentry_test_weak X %zu ave= %6.2fns rate=%10.1f/s\n",
					p10, nano, rate);
				printf("end   test_thread_wraplog_fill_weak (%p,%zu) = %d\n",
					logger, units, rc);

	#ifdef SPH_TIMERTEST_VERIFY
				rc+= test_thread_wraplog_verify(logger);
	#endif
			}

			rc += test_log_cleanup();

			rc2 = test_wraplog_parallel_init(SegmentSize);
			if (rc2 == 0)
			{
				p10 = ITERATIONS;
				test_func = test_thread_wraplog_fill;
				printf("start test_thread_wraplog_parallel_fill (%p,%zu)\n",
					logger, units);

				startt = sphgettimer();
				rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
				endt = sphgettimer();
				tempt = endt -startt;
				clock = tempt;
				freqt = sphfastcpufreq();
				nano = (clock * 1000000000.0) / (double)freqt;
				nano = nano / p10;
				rate = p10 / (clock / (double)freqt);

				printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
					startt, endt, tempt, freqt);

				printf ("lflogentry_test X %zu ave= %6.2fns rate=%10.1f/s\n",
					p10, nano, rate);
				printf("end   test_thread_wraplog_parallel_fill (%p,%zu) = %d\n",
					logger, units, rc);

	#ifdef SPH_TIMERTEST_VERIFY
				rc+= test_thread_wraplog_parallel_verify();
	#endif

				rc3 = test_wraplog_parallel_reset();
				if (rc3 == 0)
				{
					p10 = ITERATIONS;
					test_func = test_thread_wraplog_fill_strong;
					printf("start test_thread_wraplog_fill_parallel_strong (%p,%zu)\n",
						logger, units);

					startt = sphgettimer();
					rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
					endt = sphgettimer();
					tempt = endt -startt;
					clock = tempt;
					freqt = sphfastcpufreq();
					nano = (clock * 1000000000.0) / (double)freqt;
					nano = nano / p10;
					rate = p10 / (clock / (double)freqt);

					printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
						startt, endt, tempt, freqt);

					printf ("lflogentry_test_strong X %zu ave= %6.2fns rate=%10.1f/s\n",
						p10, nano, rate);
					printf("end   test_thread_wraplog_fill_parallel_strong (%p,%zu) = %d\n",
						logger, units, rc);

		#ifdef SPH_TIMERTEST_VERIFY
					rc+= test_thread_wraplog_parallel_verify();
		#endif
				}

				rc3 = test_wraplog_parallel_reset();
				if (rc3 == 0)
				{
					p10 = ITERATIONS;
					test_func = test_thread_wraplog_fill_nolock;
					printf("start test_thread_wraplog_fill_parallel_nolock (%p,%zu)\n",
						logger, units);

					startt = sphgettimer();
					rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
					endt = sphgettimer();
					tempt = endt -startt;
					clock = tempt;
					freqt = sphfastcpufreq();
					nano = (clock * 1000000000.0) / (double)freqt;
					nano = nano / p10;
					rate = p10 / (clock / (double)freqt);

					printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
						startt, endt, tempt, freqt);

					printf ("lflogentry_test_nolock X %zu ave= %6.2fns rate=%10.1f/s\n",
						p10, nano, rate);
					printf("end   test_thread_wraplog_fill_parallel_nolock (%p,%zu) = %d\n",
						logger, units, rc);

		#ifdef SPH_TIMERTEST_VERIFY
					rc+= test_thread_wraplog_parallel_verify();
		#endif
				}

				rc += test_log_parallel_cleanup();
			}
		}
		else rc += rc2;
    }

    //SASCleanUp();
    printf("SAS removed\n");
    SASRemove();
    return rc;
};

