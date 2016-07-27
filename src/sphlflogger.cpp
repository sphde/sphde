/*
 * Copyright (c) 2010-2014 IBM Corporation.
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
#include "sphlflogger.h"
#include "sphthread.h"

//#undef __SASDebugPrint__

typedef struct SASLFLoggerHeader
{
  SASBlockHeader blockHeader;
  longPtr_t next_free;
  longPtr_t start_log_buf;
  longPtr_t end_log_buf;
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
  void *dummy6;
  freeNode *headerFreeSpace;
} SASLFLoggerHeader;

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

SPHLFLogger_t
SPHLFLoggerInitInternal (void *buf_seg, sas_type_t sasType,
			 block_size_t buf_size,
			 unsigned short stride, unsigned int options)
{
  SASLFLoggerHeader *heapBlock = (SASLFLoggerHeader *) buf_seg;
  char *stackStart = NULL;
  char *stackEnd = NULL;
  longPtr_t remaining;
  longPtr_t round = (longPtr_t) DEFAULT_ALLOC_UNIT;

  if (heapBlock)
    {
      initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType, buf_size, NULL);
    }
#ifdef __SASDebugPrint__
  sas_printf
    ("SPHLFLoggerInitInternal(%p, %ld, %d) sizeof(header)=%zu round=%ld\n",
     buf_seg, buf_size, stride, sizeof (SASLFLoggerHeader), round);
#endif

  stackStart = (char *) heapBlock + DEFAULT_PAGE;
  stackEnd = (char *) heapBlock + buf_size;
  heapBlock->next_free = (longPtr_t) stackStart;
  heapBlock->start_log_buf = (longPtr_t) stackStart;
  heapBlock->end_log_buf = (longPtr_t) stackEnd;
  heapBlock->align_mask = (longPtr_t) DEFAULT_ALIGN_MASK;
  heapBlock->options = options;
  heapBlock->default_entry_stride = (stride + round) & ~round;

  remaining = DEFAULT_PAGE - sizeof (SASLFLoggerHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);
  heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
  heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;

#ifdef __SASDebugPrint__
  sas_printf ("SPHLFLoggerInitInternal() mask=%lx options=%x stride=%d\n",
	      heapBlock->align_mask,
	      heapBlock->options = options, heapBlock->default_entry_stride);
#endif
  return (SPHLFLogger_t) heapBlock;
}

SPHLFLogger_t
SPHLFLoggerInit (void *buf_seg, block_size_t buf_size)
{
  return SPHLFLoggerInitInternal (buf_seg,
				  SAS_RUNTIME_LOCKFREELOGGER,
				  buf_size,
				  NODEFAULT_ENTRY_STRIDE,
				  NODEFAULT_LOG_OPTIONS);
}

SPHLFLogger_t
SPHLFLoggerInitWithStride (void *buf_seg, block_size_t buf_size,
			   unsigned short entry_stride, unsigned int options)
{
  return SPHLFLoggerInitInternal (buf_seg,
				  SAS_RUNTIME_LOCKFREELOGGER,
				  buf_size, entry_stride, options);
}

SPHLFLogger_t
SPHLFLoggerCreate (block_size_t buf_size)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
  if (heapBlock)
    {
      return SPHLFLoggerInit (heapBlock, buf_size);
    }
  else
    return NULL;
}

SPHLFLogger_t
SPHLFCircularLoggerCreate (block_size_t buf_size, unsigned short stride)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) buf_size);
  if (heapBlock)
    {
      return SPHLFLoggerInitWithStride (heapBlock, buf_size,
					stride, SPHLFLOGGER_CIRCULAR);
    }
  else
    return NULL;
}

void *
SPHLFLoggerAllocRaw (SPHLFLogger_t log, block_size_t alloc_size)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  longPtr_t round;
  longPtr_t alloc_round = 0;
  longPtr_t log_entry = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      round = ~headerBlock->align_mask;
      alloc_round = (alloc_size + round) & ~round;
      log_entry = __sync_fetch_and_add (&headerBlock->next_free, alloc_round);

      if (log_entry + alloc_round > headerBlock->end_log_buf)
	{
	  log_entry = 0;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerAllocRaw(%p, %ld) alloc failed\n",
		      log, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerAllocRaw(%p, %ld) type check failed\n",
		  log, alloc_size);
#endif
    }
  return (void *) log_entry;
}

//#define __SASDebugPrint__ (1)

SPHLFLoggerHandle_t *
SPHLFLoggerAllocTimeStamped (SPHLFLogger_t log,
			     int catcode, int subcode,
			     block_size_t alloc_size,
			     SPHLFLoggerHandle_t * handleorg)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  SPHLFLoggerHandle_t *handlespace = handleorg;
  longPtr_t round;
  longPtr_t alloc_round;
  longPtr_t log_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      round = ~headerBlock->align_mask;
      alloc_round = (alloc_size + sizeof (SPHLFLogHeader_t) + round) & ~round;
#ifdef __powerpc__
      log_entry =
	(longPtr_t) sas_fetch_and_add_ptr ((void **) &headerBlock->next_free,
				       alloc_round);
#else
      log_entry = __sync_fetch_and_add (&headerBlock->next_free, alloc_round);
#endif
      if (log_entry + alloc_round <= headerBlock->end_log_buf)
	{
	  SPHLFLogHeader_t *entryPtr = (SPHLFLogHeader_t *) log_entry;
	  sphLogEntry_t entrytemp;
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH0)
	    __builtin_prefetch ((void *) (log_entry));
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH1)
	    __builtin_prefetch ((void *) (log_entry + 128));

	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;

	  entryPtr->timeStamp = sphgettimer ();
	  entryPtr->PID = sphFastGetPID ();
	  entryPtr->TID = sphFastGetTID ();

	  handlespace->entry = entryPtr;
	  handlespace->next =
	    (char *) (log_entry + sizeof (SPHLFLogHeader_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (SPHLFLogHeader_t);
	}
      else
	{
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerAllocTimeStamped(%p, %ld) alloc failed\n",
		      log, alloc_size);
#endif
	  sas_write_barrier ();
	  if (headerBlock->options & SPHLFLOGGER_CIRCULAR)
	    {
	      bool done = 0;
	      longPtr_t old;
	      /* verify that this thread is the first or primary
	         thread to find the wrap condition.  */
	      if ((log_entry <= headerBlock->end_log_buf)
		  && (log_entry + alloc_round >= headerBlock->end_log_buf))
		{
		  do
		    {
		      old = headerBlock->next_free;
		      done =
			__sync_bool_compare_and_swap (&headerBlock->next_free,
						      old,
						      headerBlock->
						      start_log_buf);
		    }
		  while (!done);
		  sas_fetch_and_or (&headerBlock->options,
					      SPHLFLOGGER_CIRCULAR_WRAPED);
#ifdef __SASDebugPrint__
		  sas_printf
		    ("SPHLFLoggerAllocTimeStamped(%p, %ld) wrapping\n", log,
		     alloc_size);
		  sas_printf
		    (" lflog@%p, old=%ld, ->next_free=%ld, ->start_log_buf=%ld, \n",
		     headerBlock, old, headerBlock->next_free,
		     headerBlock->start_log_buf);
#endif
		  handlespace = SPHLFLoggerAllocTimeStamped (log, catcode,
							     subcode,
							     alloc_size,
							     handleorg);
		}
	      else
		{
		  /* secondary threads should just retry until the
		     buffer wraps.  */
		  for (old = headerBlock->next_free;
		       (old + alloc_round) >= log_entry;
		       old = headerBlock->next_free)
		    sas_read_barrier ();

		  do
		    {
		      handlespace = SPHLFLoggerAllocTimeStamped (log, catcode,
								 subcode,
								 alloc_size,
								 handleorg);
		    }
		  while (!handlespace);
		}
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerAllocTimeStamped(%p, %ld) type check failed\n",
		  log, alloc_size);
#endif
    }
  return handlespace;
}

//#undef __SASDebugPrint__

//#define __SASDebugPrint__ (1)

SPHLFLoggerHandle_t *
SPHLFLoggerAllocStrideTimeStamped (SPHLFLogger_t log,
				   int catcode, int subcode,
				   SPHLFLoggerHandle_t * handleorg)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  SPHLFLoggerHandle_t *handlespace = handleorg;
  longPtr_t alloc_round = 0;
  longPtr_t log_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {				/* for Strided alloc increment is pre rounded */
      alloc_round = headerBlock->default_entry_stride;
#ifdef __powerpc__
      log_entry =
	(longPtr_t) sas_fetch_and_add_ptr ((void **) &headerBlock->next_free,
				       alloc_round);
#else
      log_entry = __sync_fetch_and_add (&headerBlock->next_free, alloc_round);
#endif
      if (log_entry + alloc_round <= headerBlock->end_log_buf)
	{
	  SPHLFLogHeader_t *entryPtr = (SPHLFLogHeader_t *) log_entry;
	  sphLogEntry_t entrytemp;
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH0)
	    __builtin_prefetch ((void *) (log_entry));
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH1)
	    __builtin_prefetch ((void *) (log_entry + 128));

#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerAllocStrideTimeStamped() alloc %ld\n",
		      alloc_round);
#endif
	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;

	  entryPtr->timeStamp = sphgettimer ();
	  entryPtr->PID = sphFastGetPID ();
	  entryPtr->TID = sphFastGetTID ();

	  handlespace->entry = entryPtr;
	  handlespace->next =
	    (char *) (log_entry + sizeof (SPHLFLogHeader_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (SPHLFLogHeader_t);
	}
      else
	{
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHLFLoggerAllocStrideTimeStamped(%p, %ld) alloc failed opt=%x\n",
	     log, alloc_round, headerBlock->options);
#endif
	  sas_write_barrier ();
	  if (headerBlock->options & SPHLFLOGGER_CIRCULAR)
	    {
	      bool done = 0;
	      longPtr_t old;
	      /* verify that this thread is the first or primary
	         thread to find the wrap condition.  */
	      if ((log_entry <= headerBlock->end_log_buf)
		  && (log_entry + alloc_round >= headerBlock->end_log_buf))
		{
		  do
		    {
		      old = headerBlock->next_free;
		      done =
			__sync_bool_compare_and_swap (&headerBlock->next_free,
						      old,
						      headerBlock->
						      start_log_buf);
		    }
		  while (!done);
		  sas_fetch_and_or (&headerBlock->options,
					      SPHLFLOGGER_CIRCULAR_WRAPED);
#ifdef __SASDebugPrint__
		  sas_printf
		    ("SPHLFLoggerAllocStrideTimeStamped(%p, ...) wrapping\n",
		     log);
		  sas_printf
		    (" lflog@%p, old=%ld, ->next_free=%ld, ->start_log_buf=%ld, \n",
		     headerBlock, old, headerBlock->next_free,
		     headerBlock->start_log_buf);
#endif
		  handlespace =
		    SPHLFLoggerAllocStrideTimeStamped (log, catcode, subcode,
						       handleorg);
		}
	      else
		{
		  /* secondary threads should just retry until the
		     buffer wraps.  */
		  for (old = headerBlock->next_free;
		       (old + alloc_round) >= log_entry;
		       old = headerBlock->next_free)
		    sas_read_barrier ();

		  do
		    {
		      handlespace =
			SPHLFLoggerAllocStrideTimeStamped (log, catcode,
							   subcode,
							   handleorg);
		    }
		  while (!handlespace);
		}
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLFLoggerAllocStrideTimeStamped(%p, %ld) type check failed\n",
	 log, alloc_round);
#endif
    }
  return handlespace;
}

//#undef __SASDebugPrint__

SPHLFLoggerHandle_t *
SPHLFLoggerAllocTimeStampedNoLock (SPHLFLogger_t log,
				   int catcode, int subcode,
				   block_size_t alloc_size,
				   SPHLFLoggerHandle_t * handleorg)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  SPHLFLoggerHandle_t *handlespace = handleorg;
  longPtr_t round;
  longPtr_t alloc_round;
  longPtr_t log_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      round = ~headerBlock->align_mask;
      alloc_round = (alloc_size + sizeof (SPHLFLogHeader_t) + round) & ~round;
      //      log_entry = __sync_fetch_and_add(&headerBlock->next_free, alloc_round);
      log_entry = headerBlock->next_free;
      headerBlock->next_free += alloc_round;
      if (log_entry + alloc_round <= headerBlock->end_log_buf)
	{
	  SPHLFLogHeader_t *entryPtr = (SPHLFLogHeader_t *) log_entry;
	  sphLogEntry_t entrytemp;
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH0)
	    __builtin_prefetch ((void *) (log_entry));
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH1)
	    __builtin_prefetch ((void *) (log_entry + 128));

	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;

	  entryPtr->timeStamp = sphgettimer ();
	  entryPtr->PID = sphFastGetPID ();
	  entryPtr->TID = sphFastGetTID ();

	  handlespace->entry = entryPtr;
	  handlespace->next =
	    (char *) (log_entry + sizeof (SPHLFLogHeader_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (SPHLFLogHeader_t);
	}
      else
	{
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHLFLoggerAllocTimeStampedNoLock(%p, %ld) alloc failed\n", log,
	     alloc_size);
#endif
	  //          atomic_write_barrier();
	  if (headerBlock->options & SPHLFLOGGER_CIRCULAR)
	    {
	      headerBlock->next_free = headerBlock->start_log_buf;
	      headerBlock->options |= SPHLFLOGGER_CIRCULAR_WRAPED;
	      handlespace = SPHLFLoggerAllocTimeStampedNoLock (log, catcode,
							       subcode,
							       alloc_size,
							       handleorg);
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLFLoggerAllocTimeStampedNoLock(%p, %ld) type check failed\n",
	 log, alloc_size);
#endif
    }
  return handlespace;
}

SPHLFLoggerHandle_t *
SPHLFLoggerAllocStrideTimeStampedNoLock (SPHLFLogger_t log,
					 int catcode, int subcode,
					 SPHLFLoggerHandle_t * handleorg)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  SPHLFLoggerHandle_t *handlespace = handleorg;
  longPtr_t alloc_round = 0;
  longPtr_t log_entry;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {				/* for Strided alloc increment is pre rounded */
      alloc_round = headerBlock->default_entry_stride;
      //      log_entry = __sync_fetch_and_add(&headerBlock->next_free, alloc_round);
      log_entry = headerBlock->next_free;
      headerBlock->next_free += alloc_round;
      if (log_entry + alloc_round <= headerBlock->end_log_buf)
	{
	  SPHLFLogHeader_t *entryPtr = (SPHLFLogHeader_t *) log_entry;
	  sphLogEntry_t entrytemp;
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH0)
	    __builtin_prefetch ((void *) (log_entry));
	  if (headerBlock->options & SPHLFLOGGER_CACHE_PREFETCH1)
	    __builtin_prefetch ((void *) (log_entry + 128));

#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerAllocStrideTimeStamped() alloc %ld\n",
		      alloc_round);
#endif
	  entrytemp.detail.valid = 0;
	  entrytemp.detail.timestamped = 1;
	  entrytemp.detail.__reserved = 0;
	  entrytemp.detail.category = catcode;
	  entrytemp.detail.subcat = subcode;
	  entrytemp.detail.len = (alloc_round / DEFAULT_ALLOC_UNIT);
	  entryPtr->entryID.idUnit = entrytemp.idUnit;

	  entryPtr->timeStamp = sphgettimer ();
	  entryPtr->PID = sphFastGetPID ();
	  entryPtr->TID = sphFastGetTID ();

	  handlespace->entry = entryPtr;
	  handlespace->next =
	    (char *) (log_entry + sizeof (SPHLFLogHeader_t));
	  handlespace->total_size = alloc_round;
	  handlespace->remaining = alloc_round - sizeof (SPHLFLogHeader_t);
	}
      else
	{
	  handlespace = NULL;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHLFLoggerAllocStrideTimeStampedNoLock(%p, %ld) alloc failed opt=%x\n",
	     log, alloc_round, headerBlock->options);
#endif
//                      atomic_write_barrier();
	  if (headerBlock->options & SPHLFLOGGER_CIRCULAR)
	    {
	      headerBlock->next_free = headerBlock->start_log_buf;
	      headerBlock->options |= SPHLFLOGGER_CIRCULAR_WRAPED;
	      handlespace = SPHLFLoggerAllocStrideTimeStampedNoLock (log,
								     catcode,
								     subcode,
								     handleorg);
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLFLoggerAllocStrideTimeStampedNoLock(%p, %ld) type check failed\n",
	 log, alloc_round);
#endif
    }
  return handlespace;
}

int
SPHLFLoggerEntryComplete (SPHLFLoggerHandle_t * handlespace)
{
  int rc = 0;
  SPHLFLogHeader_t *entryPtr = handlespace->entry;
  sphLogEntry_t entrytemp;

  sas_read_barrier ();
  entrytemp.idUnit = entryPtr->entryID.idUnit;
  entrytemp.detail.valid = 1;
  entryPtr->entryID.idUnit = entrytemp.idUnit;

  return rc;
}

int
SPHLFLoggerEntryIsComplete (SPHLFLoggerHandle_t * handlespace)
{
  SPHLFLogHeader_t *entryPtr = handlespace->entry;

  return (entryPtr->entryID.detail.valid == 1);
}

int
SPHLFLoggerEntryIsTimestamped (SPHLFLoggerHandle_t * handlespace)
{
  SPHLFLogHeader_t *entryPtr = handlespace->entry;

  return ((entryPtr->entryID.detail.valid == 1)
	  && (entryPtr->entryID.detail.timestamped == 1));
}

//#define __SASDebugPrint__ 1
SPHLFLoggerHandle_t *
SPHLFLoggerIteratorNext (SPHLFLogIterator_t * iterator,
			 SPHLFLoggerHandle_t * handleorg)
{
  SPHLFLogger_t log = iterator->logger;
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  SPHLFLoggerHandle_t *handle = NULL;

#ifdef __SASDebugPrint__
  sas_printf ("SPHLFLoggerIteratorNext(%p, %p) entry\n", iterator, handleorg);
  sas_printf ("  log=%p headerBlock=%p\n", log, headerBlock);
#endif
  /* Verify we are pointing to a valid logger before we poke around
     its insides.  */
  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      unsigned int log_options = iterator->options;
      longPtr_t lPtr = iterator->current;
      longPtr_t eLen, prev;
      SPHLFLogHeader_t *entryPtr;

      entryPtr = (SPHLFLogHeader_t *) lPtr;

#ifdef __SASDebugPrint__
      sas_printf ("  lPtr=%lx entryPtr=%p end=%lx \n",
	lPtr, entryPtr, iterator->end_log);
#endif

      if ((lPtr < iterator->end_log) && (entryPtr->entryID.detail.valid))
	{
	  eLen = (entryPtr->entryID.detail.len * DEFAULT_ALLOC_UNIT);

	  handleorg->entry = entryPtr;
	  handleorg->total_size = eLen;

	  if (entryPtr->entryID.detail.timestamped)
	    {
	      handleorg->next = (char *) (lPtr + sizeof (SPHLFLogHeader_t));
	      handleorg->remaining = eLen - sizeof (SPHLFLogHeader_t);
	    }
	  else
	    {
	      handleorg->next = (char *) (lPtr + sizeof (sphLogEntry_t));
	      handleorg->remaining = eLen - sizeof (sphLogEntry_t);
	    }
	  prev = lPtr;
	  lPtr += eLen;

#ifdef __SASDebugPrint__
	  sas_printf ("  lPtr=%lx eLen=%ld valid=%d\n", lPtr, eLen,
	    entryPtr->entryID.detail.valid);
#endif
	  if ((log_options & SPHLFLOGGER_CIRCULAR)
	      && (log_options & SPHLFLOGGER_CIRCULAR_WRAPED))
	    {
	      if (lPtr >= iterator->end_log)
		{
		  iterator->current = iterator->start_log;
		  handle = handleorg;
		}
	      else if ((prev != iterator->free)
		       || ((log_options & SPHLFLOGGER_CIRCULAR_NOTFIRST)
			   != SPHLFLOGGER_CIRCULAR_NOTFIRST))
		{
		  iterator->current = lPtr;
		  handle = handleorg;
		}
	    }
	  else
	    {
	      if (lPtr <= iterator->free)
		{
		  iterator->current = lPtr;
		  handle = handleorg;
		}
	    }
	  if (handle)
	    iterator->options |= SPHLFLOGGER_CIRCULAR_NOTFIRST;
#ifdef __SASDebugPrint__
	  sas_printf ("  handle=%p entry%p current=%lx\n",
		      handle, handleorg->entry, iterator->current);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerIteratorNext(%p, %p) type check failed\n",
		  iterator, handleorg);
#endif
    }

  return handle;
}

SPHLFLogIterator_t *
SPHLFLoggerCreateIterator (SPHLFLogger_t log,
			   SPHLFLogIterator_t * Iterator_space)
{
  SPHLFLogIterator_t *iter = NULL;
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      unsigned int log_options = headerBlock->options;
      iter = Iterator_space;
      iter->logger = log;
      iter->free = headerBlock->next_free;
      iter->start_log = headerBlock->start_log_buf;
      iter->end_log = headerBlock->end_log_buf;
      if ((log_options & SPHLFLOGGER_CIRCULAR)
	  && (log_options & SPHLFLOGGER_CIRCULAR_WRAPED))
	{
	  iter->current = headerBlock->next_free;
	}
      else
	{
	  iter->current = headerBlock->start_log_buf;
	}
      iter->options = log_options;

#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerCreateIterator(%p, %p) type check failed\n",
		  log, Iterator_space);
#endif
    }

  return iter;
}

//#undef __SASDebugPrint__
int
SPHLFLoggerWrapped (SPHLFLogger_t log)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      sas_read_barrier ();
      if ((headerBlock->options & SPHLFLOGGER_CIRCULAR)
	  && (headerBlock->options & SPHLFLOGGER_CIRCULAR_WRAPED))
	{
	  rc = 1;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerWrapped(%p) next=%lx, end=%lx\n", log,
		      headerBlock->next_free, headerBlock->end_log_buf);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerWrapped(%p) type check failed\n", log);
#endif
    }
  return rc;
}

int
SPHLFLoggerEmpty (SPHLFLogger_t log)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      sas_read_barrier ();
      if (headerBlock->next_free == headerBlock->start_log_buf)
	{
	  rc = 1;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerEmpty(%p) next=%lx, end=%lx\n", log,
		      headerBlock->next_free, headerBlock->end_log_buf);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLFLoggerEmpty(%p) type check failed\n", log);
#endif
    }
  return rc;
}

block_size_t
SPHLFLoggerFreeSpace (SPHLFLogger_t log)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  block_size_t freespace = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      sas_read_barrier ();
      if (headerBlock->next_free < headerBlock->end_log_buf)
	{
	  freespace = headerBlock->end_log_buf - headerBlock->next_free;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerFreeSpace(%p) next=%lx, end=%lx\n", log,
		      headerBlock->next_free, headerBlock->end_log_buf);
#endif
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerFreeSpace(%p) type check failed\n", log);
#endif
    }
  return freespace;
}

int
SPHLFLoggerFull (SPHLFLogger_t log)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      sas_read_barrier ();
      if (headerBlock->next_free >= headerBlock->end_log_buf)
	{
	  rc = 1;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLFLoggerEmpty(%p) next=%lx, end=%lx\n", log,
		      headerBlock->next_free, headerBlock->end_log_buf);
#endif
	}
    }
  else
    {
      rc = 1;
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerFull(%p) type check failed\n", log);
#endif
    }
  return rc;
}

int
SPHLFLoggerResetIfFullSync (SPHLFLogger_t log)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) log;
  SASLFLoggerHeader *logheader = (SASLFLoggerHeader *) log;
  longPtr_t next;
  int rc = 0;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREELOGGER))
    {
      bool done = 0;
      do
	{
	  sas_read_barrier ();
	  next = logheader->next_free;
	  if ((next + 128) >= logheader->end_log_buf)
	    {
	      done = __sync_bool_compare_and_swap (&logheader->next_free,
						   next,
						   logheader->start_log_buf);
	      if (done)
		logheader->options &= SPHLFLOGGER_CIRCULAR_RESETMASK;
	    }
	  else
	    {
	      done = 1;
	      rc = 1;
	    }
	}
      while (!done);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerResetAsync(%p) does not match type/subtype\n",
		  log);
#endif
      rc = -1;
    }
  return rc;
}

int
SPHLFLoggerResetAsync (SPHLFLogger_t log)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) log;
  SASLFLoggerHeader *logheader = (SASLFLoggerHeader *) log;
  int rc = 0;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREELOGGER))
    {
      logheader->next_free = logheader->start_log_buf;
      logheader->options &= SPHLFLOGGER_CIRCULAR_RESETMASK;
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerResetAsync(%p) does not match type/subtype\n",
		  log);
#endif
      rc = -1;
    }
  return rc;
}

int
SPHLFLoggerPrefetch (SPHLFLogger_t log)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  block_size_t logsize, pagesz, i;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      logsize = headerBlock->blockHeader.blockSize;
      pagesz = getpagesize ();
      volatile char *touchptr = (char *) log;
      char touch = 0;

      for (i = pagesz; i < logsize; i += pagesz)
	{
	  touchptr += pagesz;
	  touch += *touchptr;
#if 0
	  printf ("SPHLFLoggerPrefetch@%p\n", touchptr);
#endif
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerPrefetch(%p) type check failed\n", log);
#endif
      rc = 1;
    }
  return rc;
}

int
SPHLFLoggerSetCachePrefetch (SPHLFLogger_t log, int prefetch)
{
  SASLFLoggerHeader *headerBlock = (SASLFLoggerHeader *) log;
  unsigned int prefetch_opt = 0;
  unsigned int temp = (SPHLFLOGGER_CACHE_PREFETCH0
		       | SPHLFLOGGER_CACHE_PREFETCH1);
  int rc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_LOCKFREELOGGER))
    {
      if (prefetch > 1)
	{
	  prefetch_opt = SPHLFLOGGER_CACHE_PREFETCH1;
	  if (prefetch > 2)
	    prefetch_opt |= SPHLFLOGGER_CACHE_PREFETCH0;
	}
      else
	{
	  if (prefetch == 1)
	    prefetch_opt = SPHLFLOGGER_CACHE_PREFETCH0;
	}
      temp = sas_fetch_and_and (&headerBlock->options, temp);
      temp = sas_fetch_and_or (&headerBlock->options, prefetch_opt);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerPrefetch(%p) type check failed\n", log);
#endif
      rc = 1;
    }
  return rc;
}

int
SPHLFLoggerDestroyNoLock (SPHLFLogger_t log)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) log;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREELOGGER))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (log, heapSize);
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerDestroy(%p) does not match type/subtype\n",
		  log);
#endif
      rc = -1;
    }
  return rc;
}

int
SPHLFLoggerDestroy (SPHLFLogger_t log)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) log;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREELOGGER))
    {
      SASLock (log, SasUserLock__WRITE);
      rc = SPHLFLoggerDestroyNoLock (log);
      SASUnlock (log);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLFLoggerDestroy(%p) does not match type/subtype\n",
		  log);
#endif
      rc = -1;
    }
  return rc;
}
