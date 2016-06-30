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

static SPHMPMCQ_t pcqueue;
static SPHMPMCQ_t pcqueue2;
static SPHMPMCQ_t pcqueue3;
static SPHMPMCQ_t consumer_pcq_list[MAX_THREADS];
static SPHMPMCQ_t producer_pcq_list[MAX_THREADS];

static block_size_t cap,units,p10,pcq_alloc,pcq_stride;
//static volatile block_size_t pthreshold,cthreshold;

typedef void *(*test_ptr_t)(void *);
typedef int (*test_fill_ptr_t)(SPHMPMCQ_t,SPHMPMCQ_t,int,long);
static test_fill_ptr_t test_funclist[MAX_THREADS];
static int cpu_list[MAX_THREADS] = {0};
static int cpu_list_len = 0;
static int cpu_max = 0;

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

#ifdef DEBUG
#define debug_printf(...) sas_printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

static const char sassim_prog_name[] = __FILE__;

static inline void
sassim_print_error(const char *test, int line, const char *fmt, ...)
{
  va_list args;
  fprintf(stderr,"%s:%s:%i error: ",sassim_prog_name,test,line);
  va_start(args,fmt);
  vfprintf(stderr,fmt,args);
  fprintf(stderr,"\n");
  va_end(args);
}

#define SASSIM_PRINT_ERR(fmt,...) sassim_print_error(__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)

static inline void
sassim_print_msg(const char *func, int line, const char *fmt, ...)
{
  va_list args;
  printf("%s:%s:%i ",sassim_prog_name,func,line);
  va_start(args,fmt);
  vprintf(fmt,args);
  printf("\n");
  va_end(args);
}

#define SASSIM_PRINT_MSG(fmt,...) sassim_print_msg(__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)

static void
sassim_dump_block(const char *func, int line, void *blockAddr,
		  unsigned long len)
{
  unsigned int *dumpAddr = (unsigned int *) blockAddr;
  unsigned char *charAddr = (unsigned char *) blockAddr;
  void *tempAddr;
  unsigned char chars[20];
  unsigned char temp;
  unsigned int i,j;
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
      sassim_print_msg(func,line,"%p  %08x %08x %08x %08x <%s>",
		       tempAddr,*dumpAddr,*(dumpAddr + 1),
		       *(dumpAddr + 2),*(dumpAddr + 3),chars);
      dumpAddr += 4;
    }
}
#define SASSIM_DUMP_BLOCK(fmt,...) sassim_dump_block(__FUNCTION__,__LINE__,fmt,##__VA_ARGS__)

#undef DEBUG_PRINT

int
test_pcq_init(block_size_t pcq_size)
{
	int rc = 0;
	pcq_alloc = pcq_size;
	pcq_stride = 128;

	pcqueue = SPHMPMCQCreateWithStride(pcq_alloc,pcq_stride);
	if (pcqueue) {
		rc = SPHMPMCQSetCachePrefetch(pcqueue,0);
		printf("\nqueueCreate(%zu) success \n",pcq_alloc);

		cap = SPHMPMCQFreeSpace(pcqueue);
		units = cap / 128;

		printf("QFreeSpace() = %zu units=%zu\n",cap,units);
	} else
		rc++;

	pcqueue2 = SPHMPMCQCreateWithStride(pcq_alloc,pcq_stride);
	if (pcqueue2) {
		rc = SPHMPMCQSetCachePrefetch(pcqueue2,0);
		printf("\nqueueCreate(%zu) success \n",pcq_alloc);
	} else
		rc++;

	pcqueue3 = SPHMPMCQCreateWithStride(pcq_alloc,pcq_stride);
	if (pcqueue3) {
		rc = SPHMPMCQSetCachePrefetch(pcqueue3,0);
		printf("\nqueueCreate(%zu) success \n",pcq_alloc);
	} else
		rc++;

	return rc;
}

int
test_pcq_reset()
{
	int rc = 0;

	rc = SPHMPMCQResetAsync(pcqueue);
	if (!rc) {
		printf("\nQResetAsync(%p) success \n",pcqueue);

		cap = SPHMPMCQFreeSpace(pcqueue);
		units = cap / 128;

		printf("QFreeSpace() = %zu units=%zu\n",cap,units);
	} else
		rc++;

	rc = SPHMPMCQResetAsync(pcqueue2);
	if (!rc) {
		printf("\nQResetAsync(%p) success \n",pcqueue2);
	} else
		rc++;

	rc = SPHMPMCQResetAsync(pcqueue3);
	if (!rc) {
		printf("\nQResetAsync(%p) success \n",pcqueue3);
	} else
		rc++;

	return rc;
}

int lfPCQentry_pingtest(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1 = 0;
	int rc2,rc3,rc4;
	SPHLFEntryDirect_t handle;
	int *array;

	while (!(handle = SPHMPMCQAllocStrideDirectTM(pqueue)))
		sched_yield();

	array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	array[0] = val1;
	array[1] = val2;
	array[2] = val3;
	SPHLFEntryDirectComplete(handle,entry_tmp,1,2);

	while (!(handle = SPHMPMCQGetNextCompleteDirectTM(cqueue)))
		sched_yield();

	array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	rc1 = (array[0] != val1);
	rc2 = (array[1] != val2);
	rc3 = (array[2] != val3);
		
	if (rc1 | rc2 | rc3) {
		printf("%s: verify(%p) = %d,%d,%d should be %d,%d,%d\n",__FUNCTION__,
		       handle,array[0],array[1],array[2],val1,val2,val3);
		SASSIM_DUMP_BLOCK(cqueue,4096);
	}

	if (SPHMPMCQFreeEntryDirect(cqueue,handle)) {
		rc4 = 0;
	} else {
		printf("%s: free failed\n",__FUNCTION__);
		rc4 = 1;
	}

	return rc1 | rc2 | rc3 | rc4;
}

int
lfPCQentry_pongtest(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3) {
	int rc1,rc2,rc3,rc4;
	SPHLFEntryDirect_t handle,handle2;
	int *array,*array2;

	while (!(handle = SPHMPMCQGetNextCompleteDirectTM(pqueue)))
		sched_yield();

	array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	rc1 = (array[0] != val1);
	rc2 = (array[1] != val2);
	rc3 = (array[2] != val3);

	if (rc1 | rc2 | rc3) {
		printf("%s: verify(%p) = %d,%d,%d should be %d,%d,%d\n",__FUNCTION__,
		       handle,array[0],array[1],array[2],val1,val2,val3);
		SASSIM_DUMP_BLOCK(pqueue,4096);
	}

	while (!(handle2 = SPHMPMCQAllocStrideDirectTM(cqueue)))
		sched_yield();

	array2 = (int *) SPHLFEntryDirectGetFreePtr(handle2);
	array2[0] = array[0];
	array2[1] = array[1];
	array2[2] = array[2];
	SPHLFEntryDirectComplete(handle2,entry_tmp,1,2);

	if (SPHMPMCQFreeEntryDirect(pqueue,handle)) {
		rc4 = 0;
	} else {
		printf("%s: free failed\n",__FUNCTION__);
		rc4 = 1;
	}

	return rc1 | rc2 | rc3 | rc4;
}

int test_thread_Producer_pinger(SPHMPMCQ_t pqueue,
		SPHMPMCQ_t cqueue,int thread_ID,long iterations) {
	int rc,rtn = 0;
	long i;
	sphLFEntryID_t entry_tmp;
	debug_printf("%s(%p, %d, %ld)\n",__FUNCTION__,pcqueue,
		     thread_ID,iterations);

	entry_tmp = SPHMPMCQGetEntryTemplate(pqueue);
	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_pingtest(pqueue,cqueue,entry_tmp,i,0x01234567,
					 0xdeadbeef);
		if (rc) {
			printf("%s alloc/send(%p) failed\n",__FUNCTION__,pcqueue);
			rtn++;
			break;
		}
	}
	return rtn;
}

int
test_thread_consumer_ponger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			    int thread_ID, long iterations)
{
	int rc,rtn = 0;
	long i;
	sphLFEntryID_t entry_tmp;
	debug_printf("%s(%p, %d, %ld)\n",__FUNCTION__,pcqueue,thread_ID,iterations);

	entry_tmp = SPHMPMCQGetEntryTemplate(pqueue);
	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_pongtest(pqueue,cqueue,entry_tmp,i,0x01234567,0xdeadbeef);
		if (rc) {
			printf("%s verify(%p) failed\n",__FUNCTION__,pcqueue);
			rtn++;
			break;
		}
	}
	return rtn;
}

int
lfPCQentry_fastertest(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		sphLFEntryID_t entry_tmp, int val1, int val2, int val3)
{
	int rc1 = 0;
	SPHLFEntryDirect_t handle;
	int *array;

	while (!(handle = SPHMPMCQAllocStrideDirectTM(pqueue)))
		sched_yield();

	array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	array[0] = val1;
	array[1] = val2;
	array[2] = val3;
	SPHLFEntryDirectComplete(handle,entry_tmp,1,2);

	return rc1;
}

int
lfPCQentry_fasterverify(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue, int val1, int val2,
		       int val3)
{
	int rc1,rc2,rc3,rc4;
	SPHLFEntryDirect_t handle;
	int *array;

	while (!(handle = SPHMPMCQGetNextCompleteDirectTM(cqueue)))
		sched_yield();

	array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	rc1 = (array[0] != val1);
	rc2 = (array[1] != val2);
	rc3 = (array[2] != val3);

	if (rc1 | rc2 | rc3) {
		printf("%s:: verify(%p) = %d,%d,%d should be %d,%d,%d\n",__FUNCTION__,
		       handle,array[0],array[1],array[2] ,val1,val2,val3);
		SASSIM_DUMP_BLOCK(cqueue,4096);
	}

	if (SPHMPMCQFreeEntryDirect(cqueue,handle))
		rc4 = 0;
	else {
		printf("%s: free failed\n",__FUNCTION__);
		rc4 = 1;
	}

	return rc1 | rc2 | rc3 | rc4;
}

int
test_thread_Producer_fasterfill(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			       int thread_ID, long iterations)
{
	int rc,rtn = 0;
	long i;
	sphLFEntryID_t entry_tmp;

	debug_printf("%s(%p, %d, %ld)\n",__FUNCTION__,pcqueue,thread_ID,iterations);

	entry_tmp = SPHMPMCQGetEntryTemplate(pqueue);
	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_fastertest(pqueue,cqueue,entry_tmp,
					    i,0x01234567,0xdeadbeef);
		if (rc) {
			printf("%s alloc/send(%p) failed\n",__FUNCTION__,pcqueue);
			rtn++;
			break;
		}
	}
	return rtn;
}

int
test_thread_consumer_fasterverify(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
				  int thread_ID, long iterations)
{
	int rc,rtn = 0;
	long i;

	debug_printf("%s(%p, %d, %ld)\n",pcqueue,thread_ID,iterations);

	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_fasterverify(pqueue,cqueue,i,0x01234567,0xdeadbeef);
		if (rc) {
			printf("%s verify(%p) failed\n",__FUNCTION__,pcqueue);
			rtn++;
			break;
		}
	}
	return rtn;
}

static void *
fill_test_parallel_thread(void *arg)
{
	SPHMPMCQ_t pqueue,cqueue;
	test_fill_ptr_t test_func;
	long result = 0;
	int tn = (int)(long int) arg;

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
	SASThreadSetUp();
	debug_printf("ltt(%d, %d, @%p, @%p): begin",tn,tid,pqueue,cqueue);

	result += (*test_func)(pqueue,cqueue,tn,thread_iterations);

	debug_printf("ltt(%d, %d): end",tn,tid);

	SASThreadCleanUp();

	return (void *) result;
}

static int
launch_test_threads(int t_cnt, test_ptr_t test_f, int iterations)
{
	long int n;
	pthread_t th[max_threads];
	long thread_result;
	int result = 0;

	printf("creating thread from pid/tid = %d/%d\n",getpid(),sphdeGetTID());

	thread_iterations = iterations / t_cnt;

	for (n = 0; n < t_cnt; ++n) {
		void *arg;
		arg = (void *) n;
		if (pthread_create(&th[n],NULL,test_f,arg) != 0) {
			puts("create failed");
			exit(1);
		}
	}
	debug_printf("after create");

	for (n = 0; n < t_cnt; ++n)
		if (pthread_join(th[n],(void **) &thread_result) != 0) {
			puts("join failed");
			exit(2);
		} else {
			result += thread_result;
		}
	debug_printf("after join");

	return result;
}

int
main(int argc, char *argv[])
{
	int rc,rc3;
	double clock,nano,rate;
	sphtimer_t tempt,startt,endt,freqt;
	int i;

	for (i = 1; i < argc; i++) {
		cpu_list[i-1] = strtol(argv[i],0,0);
		if (cpu_list[i-1] > cpu_max) cpu_max = cpu_list[i-1];
		cpu_list_len++;
	}

	N_PROC_CONF = sysconf(_SC_NPROCESSORS_ONLN);
	if (N_PROC_CONF < 8)
		num_threads = N_PROC_CONF;
	else
		num_threads = 8;

	SAS_IO_INIT;	// init the io stuff
	rc = SASJoinRegion();

	if (rc) {
		printf("SASJoinRegion Error# %d\n",rc);
		return(1);
	}

	printf("SAS Joined with %d processors\n",N_PROC_CONF);

	num_threads = 2;
	rc = test_pcq_init(4096);
	if (rc == 0) {
		rc3 = test_pcq_reset();
		if (rc3 == 0) {
			producer_pcq_list[0] = pcqueue;
			consumer_pcq_list[1] = pcqueue;
			test_funclist[0] = test_thread_Producer_fasterfill;
			test_funclist[1] = test_thread_consumer_fasterverify;
			p10 = units * 5000000;

			printf("start 2a test_thread_Producer | consumer fast (%p,%zu)\n",
			       pcqueue,units);

			startt = sphgettimer();
			rc += launch_test_threads(num_threads,fill_test_parallel_thread,p10);
			endt = sphgettimer();
			tempt = endt - startt;
			clock = tempt;
			freqt = sphfastcpufreq();
			nano = (clock * 1000000000.0) / (double) freqt;
			nano = nano / p10;
			rate = p10 / (clock / (double) freqt);

			printf("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			       startt,endt,tempt,freqt);

			printf("test_thread_Producer | consumer faster %zu ave= %6.2fns rate=%10.1f/s\n",
			       p10,nano,rate);
			printf("end 2a  test_thread_Producer | consumer faster (%p,%zu) = %d\n",
			       pcqueue,units,rc);
		} else
			rc += rc3;

		rc3 = test_pcq_reset();
		if (rc3 == 0) {
			producer_pcq_list[0] = pcqueue;
			producer_pcq_list[1] = pcqueue;
			consumer_pcq_list[0] = pcqueue2;
			consumer_pcq_list[1] = pcqueue2;
			test_funclist[0] = test_thread_Producer_pinger;
			test_funclist[1] = test_thread_consumer_ponger;
			p10 = units * 5000000;

			printf("start 3a test_thread_Producer | consumer ping/pong (%p,%zu)\n",
				pcqueue,units);

			startt = sphgettimer();
			rc += launch_test_threads(num_threads,fill_test_parallel_thread,p10);
			endt = sphgettimer();
			tempt = endt - startt;
			clock = tempt;
			freqt = sphfastcpufreq();
			nano = (clock * 1000000000.0) / (double) freqt;
			nano = nano / p10;
			rate = p10 / (clock / (double) freqt);

			printf("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			       startt,endt,tempt,freqt);

			printf("test_thread_Producer | consumer %zu ave= %6.2fns rate=%10.1f/s\n",
			       p10,nano,rate);
			printf("end 3a  test_thread_Producer | consumer ping/pong (%p,%zu) = %d\n",
			       pcqueue,units,rc);
		} else
			rc += rc3;
	}
	fflush(stdout);
	//SASCleanUp();
	printf("SAS removed\n");
	SASRemove();
	return rc;
}
