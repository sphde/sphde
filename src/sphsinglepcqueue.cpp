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

//#define __SASDebugPrint__ 1

#define sas_printf printf
//#include "sasio.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sasatom.h"
#include "sassim.h"
#include "saslock.h"
#include "sphsinglepcqueue.h"
#include "sphthread.h"

//#undef __SASDebugPrint__

typedef struct SPHPCQueueHeader
{
  SASBlockHeader blockHeader;
  longPtr_t qhead;
  longPtr_t qtail;
  longPtr_t startq;
  longPtr_t endq;
  longPtr_t align_mask;
#ifdef __LP64__
  unsigned int options;
  unsigned short default_entry_stride;
  unsigned short dummy4;
  void *dummy5;
#else
  unsigned int options;
  unsigned short default_entry_stride;
  unsigned short dummy5;
#endif
  freeNode *headerFreeSpace;
} SPHPCQueueHeader;

#ifdef __LP64__
#define HEAP_OFFSET 128
#define DEFAULT_PAGE 256
#else
#define HEAP_OFFSET 64
#define DEFAULT_PAGE 256
#endif
#define DEFAULT_ALIGN_MASK	(~(sizeof(void*) + sizeof(void*) - 1))
#define DEFAULT_ALLOC_UNIT	(sizeof(void*) + sizeof(void*))
#define NODEFAULT_ENTRY_STRIDE (0)
#define NODEFAULT_LOG_OPTIONS (0)

SPHSinglePCQueue_t
SPHSinglePCQueueInitInternal (void *buf_seg, sas_type_t sasType,
			      block_size_t buf_size,
			      unsigned int stride, unsigned int options)
{
  SPHPCQueueHeader *heapBlock = (SPHPCQueueHeader *) buf_seg;
  char *qStart = NULL;
  char *qEnd = NULL;
  longPtr_t remaining;
  longPtr_t round = (longPtr_t) DEFAULT_ALLOC_UNIT;

  if (heapBlock)
    {
      initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType, buf_size, NULL);
    }
#ifdef __SASDebugPrint__
  sas_printf
    ("SPHSinglePCQueueInitInternal(%p, %ld, %d) sizeof(header)=%zu round=%ld\n",
     buf_seg, buf_size, stride, sizeof (SPHPCQueueHeader), round);
#endif
  if (stride != NODEFAULT_ENTRY_STRIDE)
    {
      /* insure stride keep minimal alignment */
      stride = (stride + round) & ~round;
      /* round buf_size to be an integral number of strides */
      buf_size = buf_size - DEFAULT_PAGE;
      buf_size = buf_size / stride;
      buf_size = buf_size * stride;
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueInitInternal() stride=%d, buf_size=%ld\n",
		  stride, buf_size);
#endif
    }

  qStart = (char *) heapBlock + DEFAULT_PAGE;
  qEnd = qStart + buf_size;

#ifdef __SASDebugPrint__
  sas_printf ("SPHSinglePCQueueInitInternal() qStart=%p, qEnd=%p\n",
	      qStart, qEnd);
#endif
  heapBlock->qhead = (longPtr_t) qStart;
  heapBlock->qtail = (longPtr_t) qStart;
  heapBlock->startq = (longPtr_t) qStart;
  heapBlock->endq = (longPtr_t) qEnd;
  heapBlock->align_mask = (longPtr_t) DEFAULT_ALIGN_MASK;
  heapBlock->options = options;
  heapBlock->default_entry_stride = stride;

  remaining = DEFAULT_PAGE - sizeof (SPHPCQueueHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);
  heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
  heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;

#ifdef __SASDebugPrint__
  sas_printf
    ("SPHSinglePCQueueInitInternal() mask=%lx options=%x stride=%d\n",
     heapBlock->align_mask, heapBlock->options =
     options, heapBlock->default_entry_stride);
#endif
  return (SPHSinglePCQueue_t) heapBlock;
}

SPHSinglePCQueue_t
SPHSinglePCQueueInit (void *buf_seg, block_size_t buf_size)
{
  return SPHSinglePCQueueInitInternal (buf_seg,
				       SAS_RUNTIME_PCQUEUE,
				       buf_size,
				       NODEFAULT_ENTRY_STRIDE,
				       NODEFAULT_LOG_OPTIONS);
}

SPHSinglePCQueue_t
SPHSinglePCQueueInitWithStride (void *buf_seg, block_size_t buf_size,
				unsigned short entry_stride,
				unsigned int options)
{
  return SPHSinglePCQueueInitInternal (buf_seg,
				       SAS_RUNTIME_PCQUEUE,
				       buf_size, entry_stride, options);
}

SPHSinglePCQueue_t
SPHSinglePCQueueCreate (block_size_t buf_size)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
  if (heapBlock)
    {
      return SPHSinglePCQueueInit (heapBlock, buf_size);
    }
  else
    return NULL;
}

SPHSinglePCQueue_t
SPHSinglePCQueueCreateWithStride (block_size_t buf_size,
				  unsigned short stride)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
  if (heapBlock)
    {
      return SPHSinglePCQueueInitWithStride (heapBlock, buf_size,
					     stride, SPHSPCQUEUE_CIRCULAR);
    }
  else
    return NULL;
}

int
SPHSinglePCQueueEmpty (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
      sas_read_barrier ();
      if (headerBlock->qhead == headerBlock->qtail)
	{
	  rc = 1;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHSinglePCQueueEmpty(%p) next=%lx, end=%lx\n", queue,
		      headerBlock->qhead, headerBlock->endq);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHSinglePCQueueEmpty(%p) type check failed\n", log);
#endif
    }
  return rc;
}

int
SPHSinglePCQueueFull (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  longPtr_t head;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
      sas_read_barrier ();
      head = headerBlock->qhead + headerBlock->default_entry_stride;
      if (head >= headerBlock->endq)
	head = headerBlock->startq;

      if (head == headerBlock->qtail)
	{
	  rc = 1;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHSinglePCQueueEmpty(%p) next=%lx, end=%lx\n", queue,
		      headerBlock->qhead, headerBlock->endq);
#endif
	}
    }
  else
    {
      rc = 1;
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueFull(%p) type check failed\n", log);
#endif
    }
  return rc;
}

block_size_t
SPHSinglePCQueueFreeSpace (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  longPtr_t head, temp;
  block_size_t rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
#ifdef __SASDebugPrint__
      sas_printf
	("SPHSinglePCQueueFreeSpace(%p) head=%lx, tail=%lx, start=%lx, end=%lx\n",
	 queue, headerBlock->qhead, headerBlock->qtail, headerBlock->startq,
	 headerBlock->endq);
#endif
      sas_read_barrier ();
      head = headerBlock->qhead + headerBlock->default_entry_stride;
      if (head >= headerBlock->endq)
	head = headerBlock->startq;

      if (head > headerBlock->qtail)
	{
	  temp = (headerBlock->endq - head)
	    + (headerBlock->qtail - headerBlock->startq);
	  rc = (block_size_t) temp;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHSinglePCQueueFreeSpace(%p) head=%lx, temp=%lx\n",
		      queue, head, temp);
#endif
	}
      else
	{
	  if (head < headerBlock->qtail)
	    {
	      temp = (headerBlock->qtail - head);
	      rc = (block_size_t) temp;
#ifdef __SASDebugPrint__
	      sas_printf
		("SPHSinglePCQueueFreeSpace(%p) head=%lx, temp=%lx\n", queue,
		 head, temp);
#endif
	    }
	  else
	    {
	      rc = 0;		/* queue is full */
#ifdef __SASDebugPrint__
	      sas_printf
		("SPHSinglePCQueueFreeSpace(%p) head=%lx, tail=%lx, rc=%lx\n",
		 queue, headerBlock->qhead, headerBlock->qtail, rc);
#endif
	    }
	}

    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueFreeSpace(%p) type check failed\n", queue);
#endif
    }
  return rc;
}

void *
SPHSinglePCQueueAllocRaw (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  longPtr_t head;
  longPtr_t queue_entry = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
      head = headerBlock->qhead + headerBlock->default_entry_stride;
      if (head >= headerBlock->endq)
	head = headerBlock->startq;

      if (head != headerBlock->qtail)
	{			/* Not full so safe to allocate */
	  queue_entry = headerBlock->qhead;
	  /* allocate the entry */
	  headerBlock->qhead = head;
	  /* Write barrier to insure that the consumer sees the qhead
	   * update change before the producer fills in the entry */
	  sas_write_barrier ();
#ifdef __SASDebugPrint__
	  sas_printf ("SPHSinglePCQueueAllocRaw(%p) alloc failed\n", queue);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHSinglePCQueueAllocRaw(%p) type check failed\n", queue);
#endif
    }
  return (void *) queue_entry;
}

SPHLFEntryHandle_t *
SPHSinglePCQueueAllocStrideEntry (SPHSinglePCQueue_t queue,
				  int catcode, int subcode,
				  SPHLFEntryHandle_t * handleorg)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  SPHLFEntryHandle_t *handlespace = handleorg;
  longPtr_t alloc_round = 0;
  longPtr_t head;
  longPtr_t queue_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {				/* for Strided alloc increment is pre rounded */
      SPHLFEntryHeader_t *entryPtr;
      sphLFEntry_t entrytemp;

      queue_entry = headerBlock->qhead;
      entryPtr = (SPHLFEntryHeader_t *) queue_entry;

      head = queue_entry + headerBlock->default_entry_stride;
      alloc_round = headerBlock->default_entry_stride;
      if (head >= headerBlock->endq)
	head = headerBlock->startq;

      if ((head != headerBlock->qtail)
	  && !entryPtr->entryID.detail.valid
	  && !entryPtr->entryID.detail.allocated)
	{
	  /* Not full so safe to allocate */
	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 0;
	  entrytemp.detail.allocated = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;
	  /* allocate the entry */
	  headerBlock->qhead = head;
	  /* Write barrier to insure that the consumer sees the qhead
	   * update and entry detail change before the producer fills
	   * in the entry */
	  sas_write_barrier ();

	  handlespace->entry = entryPtr;
	  handlespace->next = (char *) (queue_entry + sizeof (sphLFEntry_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (sphLFEntry_t);
	}
      else
	{
	  queue_entry = 0;
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) alloc failed opt=%x\n",
	     queue, alloc_round, headerBlock->options);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) type check failed\n",
	 queue, alloc_round);
#endif
    }
  return handlespace;
}

SPHLFEntryHandle_t *
SPHSinglePCQueueAllocStrideTimeStamped (SPHSinglePCQueue_t queue,
					int catcode, int subcode,
					SPHLFEntryHandle_t * handleorg)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  SPHLFEntryHandle_t *handlespace = handleorg;
  longPtr_t alloc_round = 0;
  longPtr_t head;
  longPtr_t queue_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {				/* for Strided alloc increment is pre rounded */
      SPHLFEntryHeader_t *entryPtr;
      sphLFEntry_t entrytemp;

      queue_entry = headerBlock->qhead;
      entryPtr = (SPHLFEntryHeader_t *) queue_entry;

      head = queue_entry + headerBlock->default_entry_stride;
      alloc_round = headerBlock->default_entry_stride;
      if (head >= headerBlock->endq)
	head = headerBlock->startq;

      if ((head != headerBlock->qtail)
	  && !entryPtr->entryID.detail.valid
	  && !entryPtr->entryID.detail.allocated)
	{
	  /* Not full so safe to allocate */
	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 1;
	  entrytemp.detail.allocated = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;
	  /* allocate the entry */
	  headerBlock->qhead = head;
	  /* Write barrier to insure that the consumer sees the qhead
	   * update and entry detail change before the producer fills
	   * in the entry */
	  sas_write_barrier ();

	  entryPtr->timeStamp = sphgettimer ();
	  entryPtr->PID = sphFastGetPID ();
	  entryPtr->TID = sphFastGetTID ();

	  handlespace->entry = entryPtr;
	  handlespace->next =
	    (char *) (queue_entry + sizeof (SPHLFEntryHeader_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (SPHLFEntryHeader_t);
	}
      else
	{
	  queue_entry = 0;
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) alloc failed opt=%x\n",
	     queue, alloc_round, headerBlock->options);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) type check failed\n",
	 queue, alloc_round);
#endif
    }
  return handlespace;
}

int
SPHSinglePCQueueEntryComplete (SPHLFEntryHandle_t * handlespace)
{
  int rc = 0;
  SPHLFEntryHeader_t *entryPtr = handlespace->entry;
  sphLFEntry_t entrytemp;

  /* A barrier to insure entry updates are complete before we mark
   * the entry as valid.  */
  sas_read_barrier ();
  entrytemp.idUnit = entryPtr->entryID.idUnit;
  if (entrytemp.detail.allocated)
    {
      entrytemp.detail.valid = 1;
      entryPtr->entryID.idUnit = entrytemp.idUnit;
      rc = 1;
    }

  return rc;
}

int
SPHSinglePCQueueEntryIsComplete (SPHLFEntryHandle_t * handlespace)
{
  SPHLFEntryHeader_t *entryPtr = handlespace->entry;

  return (entryPtr->entryID.detail.valid == 1);
}

SPHLFEntryHandle_t *
SPHSinglePCQueueGetNextComplete (SPHSinglePCQueue_t queue,
				 SPHLFEntryHandle_t * handleorg)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  SPHLFEntryHandle_t *handlespace = handleorg;
  longPtr_t alloc_round;
  longPtr_t queue_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {				/* for Strided alloc increment is pre rounded */
      SPHLFEntryHeader_t *entryPtr;
      sphLFEntry_t entrytemp;

      sas_read_barrier ();
      queue_entry = headerBlock->qtail;
      entryPtr = (SPHLFEntryHeader_t *) queue_entry;
      entrytemp.idUnit = entryPtr->entryID.idUnit;

      if (headerBlock->qhead != queue_entry
	  && entrytemp.detail.valid && entrytemp.detail.allocated)
	{
	  alloc_round = entrytemp.detail.len * DEFAULT_ALLOC_UNIT;
	  handlespace->entry = entryPtr;
	  handlespace->total_size = alloc_round;
	  if (entrytemp.detail.timestamped)
	    {
	      handlespace->next =
		(char *) (queue_entry + sizeof (SPHLFEntryHeader_t));
	      handlespace->remaining =
		alloc_round - sizeof (SPHLFEntryHeader_t);
	    }
	  else
	    {
	      handlespace->next =
		(char *) (queue_entry + sizeof (sphLFEntry_t));
	      handlespace->remaining = alloc_round - sizeof (sphLFEntry_t);
	    }
	}
      else
	{
	  handlespace = NULL;
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) type check failed\n",
	 queue, alloc_round);
#endif
    }
  return handlespace;
}


int
SPHSinglePCQueueFreeNextEntry (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  longPtr_t queue_entry, tail;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {				/* for Strided alloc increment is pre rounded */
      SPHLFEntryHeader_t *entryPtr;
      sphLFEntry_t entrytemp;

      queue_entry = headerBlock->qtail;
      entryPtr = (SPHLFEntryHeader_t *) queue_entry;
      entrytemp.idUnit = entryPtr->entryID.idUnit;

      tail = queue_entry + headerBlock->default_entry_stride;
      if (tail >= headerBlock->endq)
	tail = headerBlock->startq;

      if (headerBlock->qhead != queue_entry
	  && entrytemp.detail.valid && entrytemp.detail.allocated)
	{
	  /* Mark the entry unallocated and bump the queue tail pointer.
	   * This must be safe as only the consumer thread modifies
	   * the queue tail.
	   */
	  entryPtr->entryID.idUnit = 0;
	  headerBlock->qtail = tail;
	  sas_write_barrier ();
	  rc = 1;
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHSinglePCQueueAllocStrideTimeStamped(%p, %ld) type check failed\n",
	 queue, alloc_round);
#endif
    }
  return rc;
}

int
SPHSinglePCQueueSetCachePrefetch (SPHSinglePCQueue_t queue, int prefetch)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  unsigned int prefetch_opt = 0;
  unsigned int temp = (SPHSPCQUEUE_CACHE_PREFETCH0
		       | SPHSPCQUEUE_CACHE_PREFETCH1);
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
      if (prefetch > 1)
	{
	  prefetch_opt = SPHSPCQUEUE_CACHE_PREFETCH1;
	  if (prefetch > 2)
	    prefetch_opt |= SPHSPCQUEUE_CACHE_PREFETCH0;
	}
      else
	{
	  if (prefetch == 1)
	    prefetch_opt = SPHSPCQUEUE_CACHE_PREFETCH0;
	}

      temp = __sync_fetch_and_and (&headerBlock->options, temp);
      temp = __sync_fetch_and_or (&headerBlock->options, prefetch_opt);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueSetCachePrefetch(%p) type check failed\n",
		  log);
#endif
      rc = 1;
    }
  return rc;
}

int
SPHSinglePCQueueSetCachePrefetch (SPHSinglePCQueue_t queue)
{
  SPHPCQueueHeader *headerBlock = (SPHPCQueueHeader *) queue;
  block_size_t logsize, pagesz, i;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_PCQUEUE))
    {
      logsize = headerBlock->blockHeader.blockSize;
      pagesz = getpagesize ();
      volatile char *touchptr = (char *) queue;
      char touch = 0;

      for (i = pagesz; i < logsize; i += pagesz)
	{
	  touchptr += pagesz;
	  touch += *touchptr;
#if 0
	  printf ("SPHSinglePCQueueSetCachePrefetch@%p\n", touchptr);
#endif
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueSetCachePrefetch(%p) type check failed\n",
		  log);
#endif
      rc = 1;
    }
  return rc;
}

int
SPHSinglePCQueueDestroyNoLock (SPHSinglePCQueue_t queue)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) queue;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, SAS_RUNTIME_PCQUEUE))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (queue, heapSize);
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueDestroy(%p) does not match type/subtype\n",
		  queue);
#endif
      rc = -1;
    }
  return rc;
}

int
SPHSinglePCQueueDestroy (SPHSinglePCQueue_t queue)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) queue;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, SAS_RUNTIME_PCQUEUE))
    {
      SASLock (queue, SasUserLock__WRITE);
      rc = SPHSinglePCQueueDestroyNoLock (queue);
      SASUnlock (queue);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHSinglePCQueueDestroy(%p) does not match type/subtype\n",
		  queue);
#endif
      rc = -1;
    }
  return rc;
}
