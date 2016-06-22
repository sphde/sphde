/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 *     IBM Corporation, Paul Clarke   - MPMC implementation
 */

#define __SASDebugPrint__ 1

#define sas_printf printf
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#ifdef __HTM__
#include <htmxlintrin.h>
#endif
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sasatom.h"
#include "sassim.h"
#include "saslock.h"
#define SPH_INTERNAL
#include "sphmultipcqueue.h"
#include "sphthread.h"

#ifdef __SASDebugPrint__
#define debug_printf(...) sas_printf(__VA_ARGS__)
#else
#define debug_printf(...)
#endif

typedef struct SPHMPCQueueHeader
{
	SASBlockHeader blockHeader;
	longPtr_t startq;
	longPtr_t endq;
	longPtr_t align_mask;
	unsigned int options;
	unsigned short default_entry_stride;
	void *dummy6[15]; // ensure qhead in own cacheline
	longPtr_t qhead;
	void * dummy7[15]; // ensure qtail in own cacheline
	longPtr_t qtail;
	freeNode *headerFreeSpace;
} SPHMPCQueueHeader;

#ifdef __LP64__
#define HEAP_OFFSET 128
#if 0
#define DEFAULT_PAGE 256
#else
#define DEFAULT_PAGE 384
#endif
#else
#define HEAP_OFFSET 64
#define DEFAULT_PAGE 256
#endif
#define DEFAULT_ALIGN_MASK	(~(sizeof(void*) + sizeof(void*) - 1))
#define DEFAULT_ALLOC_UNIT	(sizeof(void*) + sizeof(void*))
#define NODEFAULT_ENTRY_STRIDE (0)
#define NODEFAULT_LOG_OPTIONS (0)

static SPHMultiPCQueue_t
SPHMultiPCQueueInitInternal (void *buf_seg, sas_type_t sasType,
			      block_size_t buf_size,
			      unsigned int stride, unsigned int options)
{
	SPHMPCQueueHeader *heapBlock = (SPHMPCQueueHeader *) buf_seg;
	char *qStart = NULL;
	char *qEnd = NULL;
	longPtr_t remaining;
	longPtr_t round = (longPtr_t) DEFAULT_ALLOC_UNIT;

	if (heapBlock)
		initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType, buf_size, NULL);
	debug_printf("SPHMultiPCQueueInitInternal(%p, %ld, %d) sizeof(header)=%zu round=%ld\n",buf_seg, buf_size, stride, sizeof (SPHMPCQueueHeader), round);
	if (stride != NODEFAULT_ENTRY_STRIDE) {
		/* insure stride keep minimal alignment */
		stride = (stride + round) & ~round;
		/* round buf_size to be an integral number of strides */
		buf_size = buf_size - DEFAULT_PAGE;
		buf_size = buf_size / stride;
		buf_size = buf_size * stride;
		debug_printf ("SPHMultiPCQueueInitInternal() stride=%d, buf_size=%ld\n",stride, buf_size);
	}

	qStart = (char *) heapBlock + DEFAULT_PAGE;
	qEnd = qStart + buf_size;

	debug_printf ("SPHMultiPCQueueInitInternal() qStart=%p, qEnd=%p\n",qStart, qEnd);
	debug_printf ("SPHMultiPCQueueInitInternal() offsetof(startq)=%lx, offsetof(endq)=%lx\n",
			__builtin_offsetof(struct SPHMPCQueueHeader, startq),
			__builtin_offsetof(struct SPHMPCQueueHeader, endq));
	debug_printf ("SPHSMultiPCQueueInitInternal() offsetof(qhead)=%lx, offsetof(qtail)=%lx\n",
			__builtin_offsetof(struct SPHMPCQueueHeader, qhead),
			__builtin_offsetof(struct SPHMPCQueueHeader, qtail));

	heapBlock->qhead = (longPtr_t) qStart;
	heapBlock->qtail = (longPtr_t) qStart;
	heapBlock->startq = (longPtr_t) qStart;
	heapBlock->endq = (longPtr_t) qEnd;
	heapBlock->align_mask = (longPtr_t) DEFAULT_ALIGN_MASK;
	heapBlock->options = options;
	heapBlock->default_entry_stride = stride;

	remaining = DEFAULT_PAGE - sizeof (SPHMPCQueueHeader);
	heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
	freeNode_init (heapBlock->headerFreeSpace, remaining);
	heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
	heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;

	debug_printf("SPHMultiPCQueueInitInternal() mask=%lx options=%x stride=%d\n",
			heapBlock->align_mask, options, heapBlock->default_entry_stride);

	return (SPHMultiPCQueue_t) heapBlock;
}

SPHMultiPCQueue_t
SPHMultiPCQueueInit (void *buf_seg, block_size_t buf_size)
{
	debug_printf("%s NEW\n",__FUNCTION__);
	return SPHMultiPCQueueInitInternal (buf_seg,SAS_RUNTIME_PCQUEUE_TM,buf_size,NODEFAULT_ENTRY_STRIDE,NODEFAULT_LOG_OPTIONS);
}

SPHMultiPCQueue_t
SPHMultiPCQueueInitWithStride (void *buf_seg, block_size_t buf_size,
				unsigned short entry_stride,
				unsigned int options)
{
	debug_printf("%s NEW\n",__FUNCTION__);
	return SPHMultiPCQueueInitInternal (buf_seg,
			SAS_RUNTIME_PCQUEUE_TM,
			buf_size, entry_stride, options);
}

SPHMultiPCQueue_t
SPHMultiPCQueueCreate (block_size_t buf_size)
{
	  SASBlockHeader *heapBlock = NULL;

	  debug_printf("%s\n",__FUNCTION__);
	  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
	  if (heapBlock) {
		  return SPHMultiPCQueueInit (heapBlock, buf_size);
	  } else
		  return NULL;
}

SPHMultiPCQueue_t
SPHMultiPCQueueCreateWithStride (block_size_t buf_size,
				  unsigned short stride)
{
	SASBlockHeader *heapBlock = NULL;

	debug_printf("%-6d: %s\n",sphdeGetTID(),__FUNCTION__);
	heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
	if (heapBlock) {
		return SPHMultiPCQueueInitWithStride (heapBlock, buf_size,
						     stride, SPHSPCQUEUE_CIRCULAR);
	} else
		return NULL;
}

int
SPHMultiPCQueueGetStride (SPHMultiPCQueue_t queue)
{
	printf("%-6d: %s\n",sphdeGetTID(),__FUNCTION__);
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
	                                  SAS_RUNTIME_PCQUEUE_TM))
		rc = headerBlock->default_entry_stride;
	else {
		rc = -1;
		debug_printf ("SPHMultiPCQueueGetStride(%p) type check failed\n", queue);
	}
	return rc;
}

int
SPHMultiPCQueueGetEntries (SPHMultiPCQueue_t queue)
{
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	debug_printf("%-6d: %s NEW\n",sphdeGetTID(),__FUNCTION__);
	return (headerBlock->endq - headerBlock->startq) / headerBlock->default_entry_stride;
}

int
SPHMultiPCQueueResetAsync (SPHMultiPCQueue_t queue)
{
	debug_printf("%-6d: %s\n",sphdeGetTID(),__FUNCTION__);
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,SAS_RUNTIME_PCQUEUE_TM)) {
		sas_read_barrier ();
		headerBlock->qhead = headerBlock->startq;
		headerBlock->qtail = headerBlock->startq;
	} else {
		rc = 1;
		debug_printf ("SPHMultiPCQueueResetAsync(%p) type check failed\n", queue);
	}
	return rc;
}

int
SPHMultiPCQueueEmpty (SPHMultiPCQueue_t queue)
{
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	debug_printf("%-6d: %s NEW\n",sphdeGetTID(),__FUNCTION__);

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
					  SAS_RUNTIME_PCQUEUE_TM))
	{
		sas_read_barrier ();
		longPtr_t temp = headerBlock->qtail;
		SPHLFEntryHeader_t *entryPtr = (SPHLFEntryHeader_t *)temp;
		if (!entryPtr->entryID.detail.valid)
			rc = 1;
	} else {
		debug_printf ("%s(%p) type check failed\n", __FUNCTION__,queue);
	}
	return rc;
}

int
SPHMultiPCQueueFull (SPHMultiPCQueue_t queue)
{
	printf("%s NEW\n",__FUNCTION__);
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,SAS_RUNTIME_PCQUEUE_TM)) {
		sas_read_barrier ();
		longPtr_t temp = headerBlock->qhead;
		SPHLFEntryHeader_t *entryPtr = (SPHLFEntryHeader_t *)temp;
		if (entryPtr->entryID.detail.allocated || entryPtr->entryID.detail.valid)
			rc = 1;
	} else {
		rc = 1;
		debug_printf ("%s(%p) type check failed\n", __FUNCTION__,queue);
	}
	return rc;
}

block_size_t
SPHMultiPCQueueFreeSpace (SPHMultiPCQueue_t queue)
{
	debug_printf("%s NEW\n",__FUNCTION__);
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	block_size_t rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock, SAS_RUNTIME_PCQUEUE_TM)) {
		sas_read_barrier ();
		unsigned short stride = headerBlock->default_entry_stride;
		longPtr_t qlo = headerBlock->startq;
		longPtr_t qhi = headerBlock->endq;
		longPtr_t tail = headerBlock->qtail;
		longPtr_t temp = headerBlock->qhead;
		do {
			SPHLFEntryHeader_t *entryPtr = (SPHLFEntryHeader_t *)temp;
			if (entryPtr->entryID.detail.allocated || entryPtr->entryID.detail.valid) break;
			rc += stride;
			temp += stride;
			if (temp >= qhi) temp = qlo;
		} while (temp != tail);
	} else {
		debug_printf("%s(%p) type check failed\n",__FUNCTION__, queue);
	}
	debug_printf("%s=%lu\n",__FUNCTION__,rc);
	return rc;
}

sphLFEntryID_t
SPHMultiPCQueueGetEntryTemplate (SPHMultiPCQueue_t queue)
{
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	longPtr_t alloc_round = 0;
	sphLFEntry_t entrytemp;

	entrytemp.idUnit = 0;
	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock, SAS_RUNTIME_PCQUEUE_TM)) {
		/* for Strided alloc increment is pre rounded */
		alloc_round = headerBlock->default_entry_stride;
		/* initialize common entry details.  */
		entrytemp.detail.valid = 0;
		entrytemp.detail.timestamped = 0;
		entrytemp.detail.allocated = 1;
		entrytemp.detail.__reserved = 0;
		entrytemp.detail.category = 0;
		entrytemp.detail.subcat = 0;
		entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);

	} else {
		debug_printf("%s(%p, %ld) type check failed\n", __FUNCTION__,queue, alloc_round);
	}
	return (entrytemp.idUnit);
}

static inline SPHLFEntryHeader_t *
SPHMultiPCQueueAdvanceHead(SPHLFEntryHeader_t **head_p, sphLFEntryID_t idAlloc, sphLFEntryID_t idFree, unsigned short len, longPtr_t qlo, longPtr_t qhi) {
	SPHLFEntryHeader_t *entryPtr = *head_p;
	if (entryPtr->entryID.idUnit == idFree) {
		longPtr_t new_head = ((longPtr_t)entryPtr) + len;
		if (new_head >= qhi)
			new_head = qlo;
		entryPtr->entryID.idUnit = idAlloc;
		*(longPtr_t *)head_p = new_head;
		return entryPtr;
	}
	return 0;
}

SPHLFEntryDirect_t
SPHMultiPCQueueAllocStrideDirectTM (SPHMultiPCQueue_t queue)
{
	volatile SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	SPHLFEntryHeader_t *entryPtr = 0;

	debug_printf("%-6d: %s NEW\n",sphdeGetTID(),__FUNCTION__);
	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,SAS_RUNTIME_PCQUEUE_TM)) {
		sphLFEntry_t entrytemp;
		const sphLFEntry_t entryfree = {0};
		const unsigned short stride = headerBlock->default_entry_stride;
		const longPtr_t qlo = headerBlock->startq;
		const longPtr_t qhi = headerBlock->endq;
#ifdef __HTM__
		TM_buff_type TM_buff;
#endif

		entrytemp.detail.valid = 0;
		entrytemp.detail.timestamped = 0;
		entrytemp.detail.allocated = 1;
		entrytemp.detail.__reserved = 0;
		entrytemp.detail.category = 0;
		entrytemp.detail.subcat = 0;
		entrytemp.detail.len = (stride / DEFAULT_ALLOC_UNIT);

#ifdef __HTM__
		if (__TM_begin (TM_buff) == _HTM_TBEGIN_STARTED) {
			/* Transaction State Initiated. */
			entryPtr = SPHMultiPCQueueAdvanceHead((SPHLFEntryHeader_t **)&headerBlock->qhead,entrytemp.idUnit,entryfree.idUnit,stride,qlo,qhi);
			__TM_end ();
		} else {
			if (__TM_is_failure_persistent (TM_buff)) {
				/* revert to GNU TM */
#endif
				__transaction_atomic {
					entryPtr = SPHMultiPCQueueAdvanceHead((SPHLFEntryHeader_t **)&headerBlock->qhead,entrytemp.idUnit,entryfree.idUnit,stride,qlo,qhi);
				}
#ifdef __HTM__
			}
		}
#endif
	} else {
		debug_printf("%-6d: %s(%p) type check failed\n",sphdeGetTID(),__FUNCTION__,queue);
	}
	debug_printf("%-6d: %s = %p\n",sphdeGetTID(),__FUNCTION__,entryPtr);
	return entryPtr;
}

static inline SPHLFEntryHeader_t *
SPHMultiPCQueueAdvanceTail(SPHLFEntryHeader_t **tail_p, unsigned short len, longPtr_t qlo, longPtr_t qhi) {
	SPHLFEntryHeader_t *entryPtr = *tail_p;
	sphLFEntry_t entrytemp;
	entrytemp.idUnit = entryPtr->entryID.idUnit;
	if (entrytemp.detail.allocated && entrytemp.detail.valid) {
		longPtr_t new_tail = ((longPtr_t)entryPtr) + len;
		if (new_tail >= qhi)
			new_tail = qlo;
		entrytemp.detail.allocated = 0;
		entryPtr->entryID.idUnit = entrytemp.idUnit;
		*(longPtr_t *)tail_p = new_tail;
		return entryPtr;
	}
	return 0;
}

SPHLFEntryDirect_t
SPHMultiPCQueueGetNextCompleteDirectTM (SPHMultiPCQueue_t queue)
{
	volatile SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	SPHLFEntryHeader_t *entryPtr = 0;

	debug_printf("%-6d: %s NEW\n",sphdeGetTID(),__FUNCTION__);

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,SAS_RUNTIME_PCQUEUE_TM)) {
		const unsigned short stride = headerBlock->default_entry_stride;
		const longPtr_t qlo = headerBlock->startq;
		const longPtr_t qhi = headerBlock->endq;
#ifdef __HTM__
		TM_buff_type TM_buff;
		if (__TM_begin (TM_buff) == _HTM_TBEGIN_STARTED) {
			/* Transaction State Initiated. */
			entryPtr = SPHMultiPCQueueAdvanceTail((SPHLFEntryHeader_t **)&headerBlock->qtail,stride,qlo,qhi);
			__TM_end ();
		} else {
			if (__TM_is_failure_persistent (TM_buff)) {
				/* revert to GNU TM */
#endif
				__transaction_atomic {
					entryPtr = SPHMultiPCQueueAdvanceTail((SPHLFEntryHeader_t **)&headerBlock->qtail,stride,qlo,qhi);
				}
#ifdef __HTM__
			}
		}
#endif
	} else {
		debug_printf("%s(%p) type check failed\n",__FUNCTION__,queue);
	}
	debug_printf("%-6d: %s = %p\n",sphdeGetTID(),__FUNCTION__,entryPtr);
	return ((SPHLFEntryDirect_t)entryPtr);
}

int
SPHMultiPCQueueEntryDirectIsComplete (SPHLFEntryDirect_t directHandle)
{
	SPHLFEntryHeader_t  *entryPtr = (SPHLFEntryHeader_t*)directHandle;

	return (entryPtr->entryID.detail.valid == 1);
}

int
SPHMultiPCQueueFreeEntryDirect (SPHMultiPCQueue_t queue,
                                     SPHLFEntryDirect_t entry)
{
	debug_printf("%-6d: %s(%p,%p) NEW\n",sphdeGetTID(),__FUNCTION__,queue,entry);
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
	                                  SAS_RUNTIME_PCQUEUE_TM))
	{
		SPHLFEntryHeader_t *entryPtr =
				(SPHLFEntryHeader_t *) entry;

		/* Mark the entry unallocated */
		entryPtr->entryID.idUnit = 0;
		rc = 1;
	} else {
		debug_printf("SPHMultiPCQueueFreeNextEntry(%p) "
				"type check failed\n",queue);
	}
	debug_printf("%-6d: %s(%p,%p) END\n",sphdeGetTID(),__FUNCTION__,queue,entry);
	return rc;
}

int
SPHMultiPCQueueSetCachePrefetch (SPHMultiPCQueue_t queue, int prefetch)
{
	printf("%s unimplemented\n",__FUNCTION__);
	return 0;
}

int
SPHMultiPCQueuePrefetch (SPHMultiPCQueue_t queue)
{
	SPHMPCQueueHeader *headerBlock = (SPHMPCQueueHeader *) queue;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE)) {
		block_size_t logsize = headerBlock->blockHeader.blockSize;
		block_size_t pagesz = getpagesize ();
		volatile char *touchptr = (char *) queue;
		char touch = 0;
		for (block_size_t i = pagesz; i < logsize; i += pagesz) {
			touchptr += pagesz;
			touch += *touchptr;
		}
	} else {
		debug_printf("%s(%p) type check failed\n",__FUNCTION__,queue);
		rc = 1;
	}
	return rc;
}

static int
SPHMultiPCQueueDestroyNoLock (SPHMultiPCQueue_t queue)
{
	SASBlockHeader *headerBlock = (SASBlockHeader *) queue;
	block_size_t heapSize;
	int rc;

	if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, SAS_RUNTIME_PCQUEUE_TM)) {
		heapSize = headerBlock->blockSize;
		SASBlockDealloc (queue, heapSize);
		rc = 0;
	} else {
		debug_printf ("%s(%p) does not match type/subtype\n", __FUNCTION__,queue);
		rc = -1;
	}
	return rc;
}


int
SPHMultiPCQueueDestroy (SPHMultiPCQueue_t queue)
{
	SASBlockHeader *headerBlock = (SASBlockHeader *) queue;
	int rc;

	debug_printf("%s\n",__FUNCTION__);
	if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, SAS_RUNTIME_PCQUEUE_TM)) {
		SASLock (queue, SasUserLock__WRITE);
		rc = SPHMultiPCQueueDestroyNoLock (queue);
		SASUnlock (queue);
	} else {
		debug_printf ("%s(%p) does not match type/subtype\n", __FUNCTION__,queue);
		rc = -1;
	}
	return rc;
}
