/*
 * sphmultipcqueue_t.c
 *
 *  Created on: Apr 27, 2016
 *      Author: pc
 */

//#define DebugPrint

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <limits.h>
#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <sys/auxv.h>
#include "sassim.h"
#include "sphmultipcqueue.h"
#include "sphthread.h"

#ifdef LONGCHECK
# define ITERATIONS 1000000
#else
# define ITERATIONS 100000
#endif

#define DEFAULT_ITERS 1

#define Q_SIZE 1024
#define Q_STRIDE 128
#define MAX_THREADS 256

static int cpu_list[MAX_THREADS] = {0};
static int cpu_list_len = 0;
static int cpu_max = 0;

#ifndef debug_printf
#ifdef DebugPrint
#define debug_printf(...) printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif
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

static int num_threads = 1;
static const int max_threads = 256;
static long thread_iterations;
static block_size_t cap, units, p10, pcq_alloc, pcq_stride;
static SPHMPMCQ_t pcqueue;
static sphPCQAlignedSem_t pwait, cwait;
static sphLFEntryID_t pcqueue_tmpl;
static SPHMPMCQ_t pcqueue2;
static sphPCQAlignedSem_t pwait2, cwait2;
static sphLFEntryID_t pcqueue2_tmpl;
static SPHMPMCQ_t pcqueue3;
static sphPCQAlignedSem_t pwait3, cwait3;
static sphLFEntryID_t pcqueue3_tmpl;
static SPHMPMCQ_t consumer_pcq_list[MAX_THREADS];
static SPHMPMCQ_t producer_pcq_list[MAX_THREADS];
static sphLFEntryID_t pcqueue_tmpl;
typedef void *(*test_ptr_t) (void *);
typedef int (*test_fill_ptr_t) (SPHMPMCQ_t, SPHMPMCQ_t, int, long);
static test_fill_ptr_t test_funclist[MAX_THREADS];
static int arg_list[MAX_THREADS];

#if defined(__HTM__)
int test0(unsigned int iters) {
	SPHMPMCQ_t pcq;
	SPHLFEntryDirect_t send_handle[Q_SIZE/Q_STRIDE],recv_handle[Q_SIZE/Q_STRIDE];
	sphLFEntryID_t tmpl;
	unsigned int entries;
	unsigned long space;
	unsigned int iter,i,empty,full;

	pcq = SPHMPMCQCreateWithStride(Q_SIZE,Q_STRIDE);
	if (!pcq) {
		fprintf(stderr,"SPHMPMCQCreateWithStride Error\n");
		return 1;
	}

	empty = SPHMPMCQIsEmpty(pcq);
	debug_printf("Q is empty = %d\n",empty);
	if (!empty) return 1;

	entries = SPHMPMCQGetEntries(pcq);
	debug_printf("Q entries = %d\n",entries);
	space = (unsigned long) SPHMPMCQFreeSpace(pcq);
	debug_printf("Q space = %lu\n",space);
	if (entries * Q_STRIDE != space) return 1;

	tmpl = SPHMPMCQGetEntryTemplate(pcq);
	if (!tmpl) {
		fprintf(stderr,"SPHMPMCQGetEntryTemplate Error\n");
		return 1;
	}

	for (iter = 0; iter < iters; iter++) {
		for (i = 0; i < entries; i++) {
			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) return 1;

			send_handle[i] = SPHMPMCQAllocStrideDirectHTM(pcq);
			if (!send_handle[i]) {
				fprintf(stderr,"SPHMPMCQAllocStrideDirectHTM Error\n");
				return 1;
			}
			debug_printf("ALLOC %p\n",send_handle[i]);

			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;
		}
		space = (unsigned long) SPHMPMCQFreeSpace(pcq);
		debug_printf("Q space = %lu\n",space);
		if (space) return 1;

		full = SPHMPMCQIsFull(pcq);
		debug_printf("Q is full = %d\n",full);
		if (!full) return 1;

		for (i = 0; i < entries; i++) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			debug_printf("MARK  %p\n",send_handle[i]);
			SPHLFEntryDirectComplete(send_handle[i],tmpl,0,0); // always succeeds

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (!full) return 1;
		}

		for (i = 0; i < entries; i++) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			recv_handle[i] = SPHMPMCQGetNextCompleteDirectHTM(pcq);
			if (!recv_handle[i]) {
				fprintf(stderr,"SPHMPMCQGetNextDirectCompleteHTM Error\n");
				return 1;
			}
			debug_printf("GET   %p\n",recv_handle[i]);

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) return 1;
		}

		for (i = 0; i < entries; i++) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			debug_printf("FREE  %p\n",recv_handle[i]);
			if (!SPHMPMCQFreeEntryDirect(pcq,recv_handle[i]))  {
				fprintf(stderr,"SPHMPMCQFreeEntryDirect Error\n");
				return 1;
			}
			debug_printf("Q space = %lu\n",SPHMPMCQFreeSpace(pcq));

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) return 1;
		}
		space = (unsigned long) SPHMPMCQFreeSpace(pcq);
		debug_printf("Q space = %lu\n",space);
		if (entries * Q_STRIDE != space) return 1;

		empty = SPHMPMCQIsEmpty(pcq);
		debug_printf("Q is empty = %d\n",empty);
		if (empty) return 1;

		for (i = 0; i < entries; i++) {
			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) return 1;

			send_handle[i] = SPHMPMCQAllocStrideDirectHTM(pcq);
			if (!send_handle[i]) {
				fprintf(stderr,"SPHMPMCQAllocStrideDirectHTM Error\n");
				return 1;
			}
			debug_printf("ALLOC %p\n",send_handle[i]);

			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;
		}

		/* mark in reverse order */
		for (i = entries; i > 0; i--) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			debug_printf("MARK  %p\n",send_handle[i-1]);
			SPHLFEntryDirectComplete(send_handle[i-1],tmpl,0,0); // always succeeds

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (!full) return 1;
		}

		for (i = 0; i < entries; i++) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			recv_handle[i] = SPHMPMCQGetNextCompleteDirectHTM(pcq);
			if (!recv_handle[i]) {
				fprintf(stderr,"SPHMPMCQGetNextCompleteDirectHTM Error\n");
				return 1;
			}
			debug_printf("GET   %p\n",recv_handle[i]);

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) return 1;
		}

		/* free in reverse order */
		for (i = entries; i > 0; i--) {
			empty = SPHMPMCQIsEmpty(pcq);
			debug_printf("Q is empty = %d\n",empty);
			if (empty) return 1;

			debug_printf("FREE  %p\n",recv_handle[i-1]);
			if (!SPHMPMCQFreeEntryDirect(pcq,recv_handle[i-1]))  {
				fprintf(stderr,"SPHMPMCQFreeEntryDirect Error\n");
				return 1;
			}
			debug_printf("Q space = %lu\n",SPHMPMCQFreeSpace(pcq));

			full = SPHMPMCQIsFull(pcq);
			debug_printf("Q is full = %d\n",full);
			if (full) {
				/* implementation-dependent, so ignore result */
			}
		}

		space = (unsigned long) SPHMPMCQFreeSpace(pcq);
		debug_printf("Q space = %lu\n",space);
		if (entries * Q_STRIDE != space) return 1;

		empty = SPHMPMCQIsEmpty(pcq);
		debug_printf("Q is empty = %d\n",empty);
		if (!empty) return 1;

		full = SPHMPMCQIsFull(pcq);
		debug_printf("Q is full = %d\n",full);
		if (full) return 1;
	}

	if (SPHMPMCQDestroy(pcq) != 0)  {
		fprintf(stderr,"SPHMPMCQDestroy Error\n");
		return 1;
	}
	return 0;
}
#endif

int
test_pcq_init (block_size_t pcq_size)
{
	int rc = 0;
	pcq_alloc = pcq_size;
	pcq_stride = 128;

	pcqueue = SPHMPMCQCreateWithStride (pcq_alloc, pcq_stride);
	if (pcqueue) {
		pcqueue_tmpl = SPHMPMCQGetEntryTemplate(pcqueue);
		rc = SPHMPMCQSetCachePrefetch (pcqueue, 0);
		debug_printf("%-6d: SPHMPMCQCreateWithStride (%zu) success\n",sphdeGetTID(),pcq_alloc);

		cap = SPHMPMCQFreeSpace (pcqueue);

		units = cap / 128;

		debug_printf("%-6d: SPHMPMCQFreeSpace() = %zu units=%zu\n", sphdeGetTID(), cap, units);

		memset (pwait.unit, 0, 128);
		memset (cwait.unit, 0, 128);
		rc -= sem_init (&pwait.sem.qlock, 0, 0);
		rc -= sem_init (&cwait.sem.qlock, 0, 0);
	} else
		rc++;

	pcqueue2 = SPHMPMCQCreateWithStride (pcq_alloc, pcq_stride);
	if (pcqueue2) {
		pcqueue2_tmpl = SPHMPMCQGetEntryTemplate(pcqueue2);
		rc = SPHMPMCQSetCachePrefetch (pcqueue2, 0);
		debug_printf("%-6d: SPHMPMCQSetCachePrefetch (%zu) success\n", sphdeGetTID(), pcq_alloc);

		memset (pwait2.unit, 0, 128);
		memset (cwait2.unit, 0, 128);
		rc -= sem_init (&pwait2.sem.qlock, 0, 0);
		rc -= sem_init (&cwait2.sem.qlock, 0, 0);
	} else
		rc++;

	pcqueue3 = SPHMPMCQCreateWithStride (pcq_alloc, pcq_stride);
	if (pcqueue3) {
		pcqueue3_tmpl = SPHMPMCQGetEntryTemplate(pcqueue3);
		rc = SPHMPMCQSetCachePrefetch (pcqueue3, 0);
		debug_printf("%-6d: SPHMPMCQSetCachePrefetch (%zu) success\n", sphdeGetTID(), pcq_alloc);

		memset (pwait3.unit, 0, 128);
		memset (cwait3.unit, 0, 128);
		rc -= sem_init (&pwait3.sem.qlock, 0, 0);
		rc -= sem_init (&cwait3.sem.qlock, 0, 0);
	} else
		rc++;

	return rc;
}

#if defined(__HTM__)
int
lfPCQentry_test (SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue, sphLFEntryID_t qtmpl, int val1, int val2, int val3)
{
	SPHLFEntryDirect_t *handle;

	while ((handle = SPHMPMCQAllocStrideDirectHTM(pqueue)) == 0)
		sched_yield();

	int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,val1,val2,val3);
	array[0] = val1;
	array[1] = val2;
	array[2] = val3;
	SPHLFEntryDirectComplete (handle,qtmpl,0,0);

	return 0;
}

int
lfPCQentry_verify (SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue, int val1, int val2, int val3)
{
	int rc1, rc2, rc3, rc4 = 0;
	SPHLFEntryHandle_t *handle;

	debug_printf("%s(%p,%x,%x,%x)\n",__FUNCTION__,cqueue,val1,val2,val3);

	while ((handle = SPHMPMCQGetNextCompleteDirectHTM (cqueue)) == 0)
		sched_yield();

	int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
	debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,val1,val2,val3);
	rc1 = (array[0] != val1);
	rc2 = (array[1] != val2);
	rc3 = (array[2] != val3);
	if (rc1 | rc2 | rc3)
		printf("%s:: verify %x,%x,%x should be %x,%x,%x\n",__FUNCTION__,
		       array[0], array[1], array[2], val1, val2, val3);

	/* invalidate buffer contents */
	array[0] = array[1] = array[2] = 0x2020202;

	if (!SPHMPMCQFreeEntryDirect(cqueue,handle)) {
		printf("%s:: SPHMPMCQFreeNextEntryDirect() = fail\n",
		       __FUNCTION__);
		rc4 = 1;
	}

	return (rc1 | rc2 | rc3 | rc4);
}
#endif

int
lfPCQsend(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue, sphLFEntryID_t qtmpl, int val1, int val2, int val3)
{
	SPHLFEntryDirect_t *handle;

	handle = SPHSPMCQAllocStrideDirect(pqueue);
	if (!handle) {
		debug_printf("%s(%p) full\n",__FUNCTION__,pqueue);
		while (SPHMPMCQIsFull(pqueue))
			sched_yield ();
		handle = SPHSPMCQAllocStrideDirect(pqueue);
		debug_printf("%s(%p) retry handle=%p\n",__FUNCTION__,pqueue,handle);
	}
	if (handle) {
		int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,0x111111,0x1234567,0xdeadbeef);
		array[0] = val1;
		array[1] = val2;
		array[2] = val3;
		SPHLFEntryDirectComplete(handle,qtmpl,0,0);
	} else {
		printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
		return 1;
	}
	debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,val1,val2,val3);
	return 0;
}

int
lfPCQreceive(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue, int val1, int val2, int val3)
{
	int rc;
	SPHLFEntryHandle_t *handle;
	debug_printf("%s(%p,%x,%x,%x)\n",__FUNCTION__,cqueue,val1,val2,val3);

	handle = SPHMPSCQGetNextCompleteDirect(cqueue);
	if (!handle) {
		debug_printf("%s(%p) empty\n",__FUNCTION__,cqueue);
		while (SPHMPMCQIsEmpty(cqueue))
			sched_yield();
		handle = SPHMPSCQGetNextCompleteDirect(cqueue);
		debug_printf("%s(%p) retry handle=%p\n",__FUNCTION__,cqueue,handle);
	}
	if (handle) {
		int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
		debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,val1,val2,val3);
		rc = (array[0] != val1) + (array[1] != val2) + (array[2] != val3);
		if (rc)
			printf("%s:: verify %x,%x,%x "
			       "should be %x,%x,%x\n",__FUNCTION__,
			       array[0], array[1], array[2], val1, val2, val3);

		/* invalidate buffer contents */
		array[0] = array[1] = array[2] = 0x2020202;

		if ((rc += !SPHMPMCQFreeEntryDirect(cqueue,handle))) {
			printf("%s: SPHMPMCQFreeNextEntryDirect() fail\n",__FUNCTION__);
		}
	} else {
		rc = 1;
	}
	return rc;
}

#if defined(__HTM__)
int
test_thread_Producer_fill (SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQentry_test(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_thread_pinger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQentry_test(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
		if (lfPCQentry_verify(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_SPMC_pinger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQsend(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
		if (lfPCQentry_verify(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_SPMC_ponger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQentry_verify(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
		if (lfPCQsend(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_MPSC_pinger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQentry_test(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
		if (lfPCQreceive(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_MPSC_ponger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQreceive(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
		if (lfPCQentry_test(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}
#endif

int
test_SPSC_pinger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQsend(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
		if (lfPCQreceive(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_SPSC_ponger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQreceive(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
		if (lfPCQsend(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

#if defined(__HTM__)
int
test_thread_ponger(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
		   int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0;
	long i;

	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__, pqueue,
	   thread_ID, iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		if (lfPCQentry_verify(pqueue,cqueue,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s verify (%p) failed\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
		if (lfPCQentry_test(pqueue,cqueue,qtmpl,0x111111,0x01234567,0xdeadbeef) != 0) {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_thread_consumer_verify (SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			     int thread_ID, long iterations)
{
	int rc, rtn = 0;
	long i;
	debug_printf ("%s(%p, %d, %ld)\n",__FUNCTION__,pqueue,
			thread_ID, iterations);

	for (i = 0; i < iterations; i++) {
		rc = lfPCQentry_verify (pqueue, cqueue, 0x111111, 0x01234567, 0xdeadbeef);
		if (!rc) {
		} else {
			printf("%s verify (%p) fail\n",
					__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}
#endif

int
test_single_producer_fill(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			  int thread_ID, long iterations)
{
	sphLFEntryID_t qtmpl;
	int rtn = 0,i;

	debug_printf("%s(%p, %d, %ld)\n",__FUNCTION__,pqueue,thread_ID,iterations);

	qtmpl = SPHMPMCQGetEntryTemplate(pqueue);

	for (i = 0; i < iterations; i++) {
		SPHLFEntryDirect_t *handle;

		handle = SPHSPMCQAllocStrideDirect(pqueue);
		if (!handle) {
			debug_printf("%s(%p) full\n",__FUNCTION__,pqueue);
			while (SPHMPMCQIsFull(pqueue))
				sched_yield ();
			handle = SPHSPMCQAllocStrideDirect(pqueue);
			debug_printf("%s(%p) retry handle=%p\n",__FUNCTION__,pqueue,handle);
		}
		if (handle) {
			int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,0x111111,0x1234567,0xdeadbeef);
			array[0] = 0x111111;
			array[1] = 0x1234567;
			array[2] = 0xdeadbeef;
			SPHLFEntryDirectComplete(handle,qtmpl,0,0);
		} else {
			printf("%s alloc (%p) failed\n",__FUNCTION__,pqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

int
test_single_consumer_verify(SPHMPMCQ_t pqueue, SPHMPMCQ_t cqueue,
			    int thread_ID, long iterations)
{
	int rc, rtn = 0;
	int val1 = 0x111111, val2 = 0x1234567, val3 = 0xdeadbeef;
	long i;
	debug_printf("%s(%p, %d, %ld)\n",__FUNCTION__,cqueue,thread_ID,iterations);

	for (i = 0; i < iterations; i++) {
		SPHLFEntryHandle_t *handle;
		debug_printf("%s(%p,%x,%x,%x)\n",__FUNCTION__,cqueue,val1,val2,val3);

		handle = SPHMPSCQGetNextCompleteDirect(cqueue);
		if (!handle) {
			debug_printf("%s(%p) empty\n",__FUNCTION__,cqueue);
			while (SPHMPMCQIsEmpty(cqueue))
				sched_yield();
			handle = SPHMPSCQGetNextCompleteDirect(cqueue);
			debug_printf("%s(%p) retry handle=%p\n",__FUNCTION__,cqueue,handle);
		}
		if (handle) {
			int *array = (int *) SPHLFEntryDirectGetFreePtr(handle);
			debug_printf("%s %p = [ %x %x %x ]\n",__FUNCTION__,array,val1,val2,val3);
			rc = (array[0] != val1) + (array[1] != val2) + (array[2] != val3);
			if (rc)
				printf("%s:: verify %x,%x,%x "
				       "should be %x,%x,%x\n",__FUNCTION__,
				       array[0], array[1], array[2], val1, val2, val3);

			/* invalidate buffer contents */
			array[0] = array[1] = array[2] = 0x2020202;

			if ((rc += !SPHMPMCQFreeEntryDirect(cqueue,handle))) {
				printf("%s: SPHMPMCQFreeNextEntryDirect() fail\n",__FUNCTION__);
			}
		} else {
			rc = 1;
		}

		if (rc) {
			printf("%s verify (%p) fail\n",__FUNCTION__,cqueue);
			rtn++;
			break;
		}
	}
	debug_printf ("%s END\n",__FUNCTION__);
	return rtn;
}

static void *
fill_test_parallel_thread (void *arg)
{
	SPHMPMCQ_t pqueue, cqueue;
	test_fill_ptr_t test_func;
	long result = 0;
	int tn = (int) (long int) arg;

	pqueue = producer_pcq_list[tn];
	cqueue = consumer_pcq_list[tn];
	test_func = test_funclist[tn];

	if (cpu_list_len > tn) {
		int rc;
		cpu_set_t *cset = CPU_ALLOC(cpu_max+1);
		int size = CPU_ALLOC_SIZE(cpu_max+1);
		CPU_ZERO_S(size,cset);
		CPU_SET_S(cpu_list[tn],size,cset);
		rc = sched_setaffinity(sphFastGetTID(),size,cset);
		if (rc == 0) {
			printf("%6d: sched_setaffinity(thread %d to CPU %d)\n",
				sphFastGetTID(),tn,cpu_list[tn]);
		} else {
			char buf[1024];
			char *msg = strerror_r(errno,buf,sizeof buf);
			if (!msg) msg = buf;
			fprintf(stderr,"%6d: sched_setaffinity(thread %d to CPU %d) %s\n",
				sphFastGetTID(),tn,cpu_list[tn],msg);
		}
		CPU_FREE(cset);
	}

	SASThreadSetUp ();
	debug_printf ("ltt(%d, %d, @%p, @%p): begin\n", tn, sphFastGetTID(),
			pqueue, cqueue);

	result += (*test_func) (pqueue, cqueue, tn, arg_list[tn]);

	debug_printf ("ltt(%d, %d): end\n", tn, sphFastGetTID());
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

	debug_printf ("creating threads from pid/tid = %d/%d\n", getpid(), sphdeGetTID());

	thread_iterations = iterations / t_cnt;

	for (n = 0; n < t_cnt; ++n) {
		void *arg;
		arg = (void *) n;
		if (pthread_create (&th[n], NULL, test_f, arg) != 0) {
			puts ("create failed");
			exit (1);
		}
	}
	debug_printf ("after creates\n");

	for (n = 0; n < t_cnt; ++n)
		if (pthread_join (th[n], (void **) &thread_result) != 0) {
			puts ("join failed");
			exit (2);
		} else {
			result += thread_result;
		}
	debug_printf ("after joins\n");

	return result;
}

void compute_stats(unsigned long count, sphtimer_t delta, double *per, double *rate)
{
	double freq = sphfastcpufreq();
	double deltat = delta / freq;
	*per = deltat * 1000000000.0 / count;
	*rate = count / deltat;
}

int
main(int argc, char *argv[]) {
	unsigned long hwcap2 = getauxval(AT_HWCAP2);
#ifdef __s390x__
	if ((hwcap2 & HWCAP_S390_TE) == 0) {
#else
	if ((hwcap2 & PPC_FEATURE2_HAS_HTM) == 0) {
#endif
		fprintf(stderr,"MPMCQ requires Hardware Transactional Memory support.\n");
		exit(0);
	}

	double nano, rate;
	sphtimer_t startt, endt;
	int rc = 0;
	int test_id,i;
	int num_producers, num_consumers;

	for (i = 1; i < argc; i++) {
		cpu_list[i-1] = strtol(argv[i],0,0);
		if (cpu_list[i-1] > cpu_max) cpu_max = cpu_list[i-1];
		cpu_list_len++;
	}

	rc = SASJoinRegion();
	if (rc) {
		fprintf(stderr,"SASJoinRegion Error# %d\n",rc);
		return rc;
	}
	debug_printf("SAS Joined\n");

#if defined(__HTM__)
	unsigned long iters = DEFAULT_ITERS;
	printf("START 0 test0(%lu)\n",iters);
	if (argc > 1) {
		iters = strtoul(argv[1],0,0);
	}
	debug_printf("%s: iterations=%lu\n",__FUNCTION__,iters);

	rc = test0(iters);
	printf("END   0 test0 = %d\n",rc);
#endif

	rc = test_pcq_init (4096);

	p10 = units * ITERATIONS;
#if defined(__HTM__)
#if 1
	block_size_t remainder;

	test_id = 1;

	num_threads = 2;
	num_producers = 1;
	num_consumers = num_threads - num_producers;
	remainder = p10;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = 0;
		test_funclist[i] = test_thread_Producer_fill;
		arg_list[i] = p10 / num_producers;
		remainder -= p10 / num_producers;
	}
	arg_list[i-1] += remainder;

	remainder = p10;
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = 0;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_consumer_verify;
		arg_list[i] = p10 / num_consumers;
		remainder -= p10 / num_consumers;
	}
	arg_list[i-1] += remainder;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d producers | %d consumers (%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | %d consumers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test thread %d producers | %d consumers (%p,%zu) = %d\n",
	       test_id,num_producers,num_consumers,pcqueue,units,rc);
#endif
#if 1
	test_id = 2;

	num_threads = 2;
	num_consumers = 1; // SINGLE CONSUMER!
	num_producers = num_threads - num_consumers;
	remainder = p10;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = 0;
		test_funclist[i] = test_thread_Producer_fill;
		arg_list[i] = p10 / num_producers;
		remainder -= p10 / num_producers;
	}
	arg_list[i-1] += remainder;

	producer_pcq_list[i] = 0;
	consumer_pcq_list[i] = pcqueue;
	test_funclist[i] = test_single_consumer_verify;
	arg_list[i] = p10;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test %d producers | single consumer (%p,%zu)\n",
	       test_id,num_producers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | single consumer %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,p10,nano,rate);
	printf("END   %u   test %d producers | single consumer (%p,%zu) = %d\n",
	       test_id,num_producers,pcqueue, units, rc);
#endif
#if 1
	test_id = 3;

	num_threads = 2;
	num_producers = 1; // SINGLE PRODUCER!
	num_consumers = num_threads - num_producers;

	producer_pcq_list[0] = pcqueue;
	consumer_pcq_list[0] = 0;
	test_funclist[0] = test_single_producer_fill;
	arg_list[0] = p10;
	i = 1;

	remainder = p10;
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = 0;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_consumer_verify;
		arg_list[i] = p10 / num_consumers;
		remainder -= p10 / num_consumers;
	}
	arg_list[i-1] += remainder;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test single producer | %d consumers (%p,%zu)\n",
		test_id,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread single producer | %d consumers %zu ave= %6.2fns rate=%10.1f/s\n",
		num_consumers,p10,nano,rate);
	printf("END   %u   test single producer | %d consumers (%p,%zu) = %d\n",
		test_id,num_consumers,pcqueue,units,rc);
#endif
#endif
#if 1
	test_id = 4;

	num_producers = 1; // SINGLE PRODUCER!
	num_consumers = 1; // SINGLE CONSUMER!
	num_threads = num_producers + num_consumers;

	producer_pcq_list[0] = pcqueue;
	consumer_pcq_list[0] = 0;
	test_funclist[0] = test_single_producer_fill;
	arg_list[0] = p10;

	producer_pcq_list[1] = 0;
	consumer_pcq_list[1] = pcqueue;
	test_funclist[1] = test_single_consumer_verify;
	arg_list[1] = p10;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test single producers | single consumer (%p,%zu)\n",
		test_id,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test single producer | single consumer %zu ave= %6.2fns rate=%10.1f/s\n",
		     p10, nano, rate);
	printf("END   %u   test single producer | single consumer (%p,%zu) = %d\n",
		test_id,pcqueue,units,rc);
#endif
#if defined(__HTM__)
#if 1
	test_id = 5;

	num_threads = 8;
	num_producers = 4;
	num_consumers = num_threads - num_producers;
	remainder = p10;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = 0;
		test_funclist[i] = test_thread_Producer_fill;
		arg_list[i] = p10 / num_producers;
		remainder -= p10 / num_producers;
	}
	arg_list[i-1] += remainder;

	remainder = p10;
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = 0;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_consumer_verify;
		arg_list[i] = p10 / num_consumers;
		remainder -= p10 / num_consumers;
	}
	arg_list[i-1] += remainder;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d producers | %d consumers (%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | %d consumers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test thread %d producers | %d consumers (%p,%zu) = %d\n",
	       test_id,num_producers,num_consumers,pcqueue,units,rc);
#endif
#if 1
	test_id = 6;

	num_threads = 5;
	num_consumers = 1; // SINGLE CONSUMER!
	num_producers = num_threads - num_consumers;
	remainder = p10;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = 0;
		test_funclist[i] = test_thread_Producer_fill;
		arg_list[i] = p10 / num_producers;
		remainder -= p10 / num_producers;
	}
	arg_list[i-1] += remainder;

	producer_pcq_list[i] = 0;
	consumer_pcq_list[i] = pcqueue;
	test_funclist[i] = test_thread_consumer_verify;
	arg_list[i] = p10;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test %d producers | %d consumers (%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | %d consumer %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test %d producers | single consumer (%p,%zu) = %d\n",
	       test_id,num_producers,pcqueue, units, rc);
#endif
#if 1
	test_id = 7;

	num_threads = 5;
	num_consumers = 1; // SINGLE CONSUMER!
	num_producers = num_threads - num_consumers;
	remainder = p10;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = 0;
		test_funclist[i] = test_thread_Producer_fill;
		arg_list[i] = p10 / num_producers;
		remainder -= p10 / num_producers;
	}
	arg_list[i-1] += remainder;

	producer_pcq_list[i] = 0;
	consumer_pcq_list[i] = pcqueue;
	test_funclist[i] = test_single_consumer_verify;
	arg_list[i] = p10;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test %d producers | single consumer (%p,%zu)\n",
	       test_id,num_producers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | single consumer %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,p10,nano,rate);
	printf("END   %u   test %d producers | single consumer (%p,%zu) = %d\n",
	       test_id,num_producers,pcqueue, units, rc);
#endif
#if 1
	test_id = 8;

	num_threads = 5;
	num_producers = 1; // SINGLE PRODUCER!
	num_consumers = num_threads - num_producers;

	producer_pcq_list[0] = pcqueue;
	consumer_pcq_list[0] = 0;
	test_funclist[0] = test_thread_Producer_fill;
	arg_list[0] = p10;
	i = 1;

	remainder = p10;
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = 0;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_consumer_verify;
		arg_list[i] = p10 / num_consumers;
		remainder -= p10 / num_consumers;
	}
	arg_list[i-1] += remainder;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test %d producers | %d consumers (%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d producers | %d consumers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test %d producer | %d consumers (%p,%zu) = %d\n",
	       test_id,num_producers,num_consumers,pcqueue,units,rc);
#endif
#if 1
	test_id = 9;

	num_threads = 5;
	num_producers = 1; // SINGLE PRODUCER!
	num_consumers = num_threads - num_producers;

	producer_pcq_list[0] = pcqueue;
	consumer_pcq_list[0] = 0;
	test_funclist[0] = test_single_producer_fill;
	arg_list[0] = p10;
	i = 1;

	remainder = p10;
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = 0;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_consumer_verify;
		arg_list[i] = p10 / num_consumers;
		remainder -= p10 / num_consumers;
	}
	arg_list[i-1] += remainder;

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test single producer | %d consumers (%p,%zu)\n",
		test_id,num_consumers,pcqueue,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread single producer | %d consumers %zu ave= %6.2fns rate=%10.1f/s\n",
		num_consumers,p10,nano,rate);
	printf("END   %u   test single producer | %d consumers (%p,%zu) = %d\n",
		test_id,num_consumers,pcqueue,units,rc);
#endif
#if 1
	test_id = 10;

	num_producers = 1;
	num_consumers = num_producers;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_thread_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d pingers | %d pongers (%p,%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d pingers | %d pongers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test thread %d pingers | %d pongers (%p,%p,%zu) = %d\n",
	       test_id,num_producers,num_consumers,pcqueue,pcqueue2,units,rc);
#endif
#if 1
	test_id = 11;

	num_producers = 1;
	num_consumers = num_producers;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_SPMC_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread single pinger | %d pongers (%p,%p,%zu)\n",
	       test_id,num_consumers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread single pinger | %d pongers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_consumers,p10,nano,rate);
	printf("END   %u   test thread single pinger | %d pongers (%p,%p,%zu) = %d\n",
	       test_id,num_consumers,pcqueue,pcqueue2,units,rc);
#endif
#if 1
	test_id = 12;

	num_producers = 1;
	num_consumers = num_producers;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_thread_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_MPSC_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d pingers | single ponger (%p,%p,%zu)\n",
	       test_id,num_producers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d pingers | single ponger %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,p10,nano,rate);
	printf("END   %u   test thread %d pingers | single ponger (%p,%p,%zu) = %d\n",
	       test_id,num_producers,pcqueue,pcqueue2,units,rc);
#endif
#endif
#if 1
	test_id = 13;

	num_producers = 1;
	num_consumers = num_producers;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_SPSC_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_SPSC_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread single pinger | single ponger (%p,%p,%zu)\n",
	       test_id,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread single pinger | single ponger %zu ave= %6.2fns rate=%10.1f/s\n",
	       p10,nano,rate);
	printf("END   %u   test thread single pinger | single ponger (%p,%p,%zu) = %d\n",
	       test_id,pcqueue,pcqueue2,units,rc);
#endif
#if defined(__HTM__)
#if 0
	test_id = 14;

	num_producers = 1;
	num_consumers = 4;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_SPSC_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread single pinger | %d pongers (%p,%p,%zu)\n",
	       test_id,num_consumers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread single pinger | %d pongers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_consumers,p10,nano,rate);
	printf("END   %u   test thread single pinger | %d pongers (%p,%p,%zu) = %d\n",
	       test_id,num_consumers,pcqueue,pcqueue2,units,rc);
#endif
#if 0
	test_id = 15;

	num_producers = 4;
	num_consumers = 1;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_thread_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_SPSC_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d pingers | single pongers (%p,%p,%zu)\n",
	       test_id,num_producers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d pingers | single ponger %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,p10,nano,rate);
	printf("END   %u   test thread %d pingers | single ponger (%p,%p,%zu) = %d\n",
	       test_id,num_producers,pcqueue,pcqueue2,units,rc);
#endif
#if 0
	test_id = 16;

	num_producers = 4;
	num_consumers = 4;
	num_threads = num_producers + num_consumers;
	for (i = 0; i < num_producers; i++) {
		producer_pcq_list[i] = pcqueue;
		consumer_pcq_list[i] = pcqueue2;
		test_funclist[i] = test_thread_pinger;
		arg_list[i] = p10;
	}
	for (; i < num_threads; i++) {
		producer_pcq_list[i] = pcqueue2;
		consumer_pcq_list[i] = pcqueue;
		test_funclist[i] = test_thread_ponger;
		arg_list[i] = p10;
	}

	for (i = 0; i < num_threads; i++) {
		debug_printf("%d: arg %d func %p p %p c %p\n",i,arg_list[i],test_funclist[i],producer_pcq_list[i],consumer_pcq_list[i]);
	}

	printf("START %u test thread %d pingers | %d pongers (%p,%p,%zu)\n",
	       test_id,num_producers,num_consumers,pcqueue,pcqueue2,units);

	startt = sphgettimer ();
	rc += launch_test_threads (num_threads, fill_test_parallel_thread, p10);
	endt = sphgettimer ();

	compute_stats(p10,endt-startt,&nano,&rate);

	printf("test thread %d pingers | %d pongers %zu ave= %6.2fns rate=%10.1f/s\n",
	       num_producers,num_consumers,p10,nano,rate);
	printf("END   %u   test thread %d pingers | %d pongers (%p,%p,%zu) = %d\n",
	       test_id,num_producers,num_consumers,pcqueue,pcqueue2,units,rc);
#endif
#endif
	SASRemove();
	return rc;
}
