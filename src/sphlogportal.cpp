/*
 * Copyright (c) 2011-2014 IBM Corporation.
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
#include <stdio.h>
//#include "sasio.h"
#include <stdlib.h>
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
#include "sphlogportal.h"

//#undef __SASDebugPrint__

typedef struct SASLogPortalHeader {
		SASBlockHeader blockHeader;
		SPHLFLogger_t	*log_buff_list;
		longPtr_t		current_logger;
		longPtr_t		next_free;
#ifdef __LP64__
		unsigned int		options;
		unsigned short		list_size;
		unsigned short		dummy3;
		void			*dummy4;
#else
		unsigned int		options;
		unsigned short		list_size;
		unsigned short		dummy4;
#endif
		void			*dummy5;
		void			*dummy6;
		freeNode		*headerFreeSpace;
		} SASLogPortalHeader;


#ifdef __LP64__
#define HEAP_OFFSET 128
#define DEFAULT_PAGE 256
#else
#define HEAP_OFFSET 64
#define DEFAULT_PAGE 256
#endif
#define DEFAULT_ALIGN_MASK	(~(sizeof(void*) + sizeof(void*) - 1))
#define DEFAULT_ALLOC_UNIT	(sizeof(void*) + sizeof(void*))
#define NODEFAULT_LOG_OPTIONS (0)

SPHLogPortal_t
SPHLogPortalInitInternal (void* buf_seg,  sas_type_t sasType,
                         block_size_t buf_size,
                         unsigned int options)
{
	SASLogPortalHeader	*heapBlock = (SASLogPortalHeader*)buf_seg;
    char		*listStart = NULL;
    char		*listEnd = NULL;
    longPtr_t		remaining;
    longPtr_t		entries;


    if ( heapBlock )
    {
	initSOMSASBlock((SASBlockHeader*)heapBlock, sasType,
	                buf_size, NULL);
    }
#ifdef __SASDebugPrint__
    sas_printf("SPHLogPortalInitInternal(%p, %ld, %d) sizeof(header)=%zu\n",
	          buf_seg, buf_size, options,
	          sizeof(SASLogPortalHeader));
#endif

    listStart = (char*) heapBlock + DEFAULT_PAGE;
    listEnd   = (char*) heapBlock + buf_size;
    remaining = (longPtr_t)listEnd - (longPtr_t)listStart;
    entries   = remaining / DEFAULT_ALLOC_UNIT;
    heapBlock->options = options;
    heapBlock->list_size = entries;
    heapBlock->log_buff_list = (SPHLFLogger_t*)listStart;
    heapBlock->current_logger = 0;
    heapBlock->next_free = 0;

    remaining= DEFAULT_PAGE - sizeof(SASLogPortalHeader);
    heapBlock->headerFreeSpace = (freeNode *)&heapBlock[1];
    freeNode_init(heapBlock->headerFreeSpace, remaining);
    heapBlock->blockHeader.baseBlock = (SASBlockHeader*)heapBlock;
    heapBlock->blockHeader.nextBlock = (SASBlockHeader*)heapBlock;

#ifdef __SASDebugPrint__
    sas_printf("SPHLogPortalInitInternal() options=%x size=%d list@=%p\n",
	          heapBlock->options, heapBlock->list_size, heapBlock->log_buff_list);
#endif
    return (SPHLogPortal_t)heapBlock;
}


SPHLogPortal_t
SPHLogPortalInit (void* buf_seg,  block_size_t buf_size)
{
    return SPHLogPortalInitInternal(buf_seg,
    								SAS_RUNTIME_LOGPORTAL,
                                    buf_size,
                                    NODEFAULT_LOG_OPTIONS);
}

SPHLogPortal_t
SPHLogPortalCreate (block_size_t buf_size)
{
    SASBlockHeader	*heapBlock = NULL;

    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)buf_size);
    if ( heapBlock )
    {
	    return SPHLogPortalInit(heapBlock, buf_size);
    } else
	    return NULL;
}

int
SPFLogPortalDestroyNoLock(SPHLogPortal_t portal)
{
	SASBlockHeader *headerBlock = (SASBlockHeader*) portal;
	block_size_t heapSize;
	int rc;

	if (SOMSASCheckBlockSigAndTypeAndSubtype(headerBlock, SAS_RUNTIME_LOGPORTAL)) {
		heapSize = headerBlock->blockSize;
		SASBlockDealloc(portal, heapSize);
		rc = 0;
	} else {
#ifdef __SASDebugPrint__
		sas_printf("SPHLFLoggerDestroy(%p) does not match type/subtype\n", portal);
#endif
		rc = -1;
	}
	return rc;
}

int
SPHLogPortalDestroy(SPHLogPortal_t portal)
{
	SASBlockHeader *headerBlock = (SASBlockHeader*) portal;
	int rc;

	if (SOMSASCheckBlockSigAndTypeAndSubtype(headerBlock, SAS_RUNTIME_LOGPORTAL)) {
		SASLock(portal, SasUserLock__WRITE);
		rc = SPFLogPortalDestroyNoLock(portal);
		SASUnlock(portal);
	} else {
#ifdef __SASDebugPrint__
		sas_printf("SPHLogPortalDestroy(%p) does not match type/subtype\n", portal);
#endif
		rc = -1;
	}
	return rc;
}

SPHLFLogger_t
SPHLogPortalAddLogger (SPHLogPortal_t portal, SPHLFLogger_t log)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	SASBlockHeader	*logBlock = (SASBlockHeader*)log;
	SASBlockHeader	*logPrevBlock;
	longPtr_t index;
	SPHLFLogger_t rc = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		if (SOMSASCheckBlockSigAndType (logBlock,
				SAS_RUNTIME_LOCKFREELOGGER) )
		{
			index = __sync_fetch_and_add (&headerBlock->next_free, 1);
			if (index < headerBlock->list_size)
			{
				/* Insert the Logger in the Portal list */
				headerBlock->log_buff_list[index] = log;
				/* Link this Logger to the Portal as its Base Block
				 * then link this Logger as the Next Block of the
				 * previous Logger in the list.
				 */
				logBlock->baseBlock = (SASBlockHeader*)headerBlock;
				if (index > 0)
				{
					logPrevBlock = (SASBlockHeader*)headerBlock->log_buff_list[index-1];
					logPrevBlock->nextBlock = logBlock;
				}

				rc = log;
			} else {
				if (index > headerBlock->list_size)
					index = __sync_fetch_and_add (&headerBlock->next_free, -1);
			}
	#ifdef __SASDebugPrint__
		} else {
			sas_printf("SPHLogPortalAddLogger(%p, %p) log parm type check failed\n",
					portal, log);
	#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalAddLogger(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

SPHLFLogger_t
SPHLogPortalGetCurrentLogger (SPHLogPortal_t portal)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	longPtr_t index;
	SPHLFLogger_t rc = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		if ( headerBlock->next_free >  headerBlock->current_logger )
		{
			index = headerBlock->current_logger;
			rc = headerBlock->log_buff_list[index];
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalCurrentLogger(%p) next=%lx, current=%lx\n",  portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalCurrentLogger(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

SPHLFLogger_t
SPHLogPortalGetLoggerByIndex (SPHLogPortal_t portal, longPtr_t index)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	SPHLFLogger_t rc = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		if ( (index >= 0) && (index  < headerBlock->next_free) )
		{
			rc = headerBlock->log_buff_list[index];
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalGetLoggerByIndex(%p, %ld) next=%ld, rc=%p\n",
					portal, index,
					headerBlock->next_free, rc);
#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalGetLoggerByIndex(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

int
SPHLogPortalGetCurrentIndex (SPHLogPortal_t portal)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	int rc = -1;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		if ( headerBlock->next_free >  headerBlock->current_logger )
		{
			rc = headerBlock->current_logger;
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalCurrentIndex(%p) next=%ld, current=%ld\n",  portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalCurrentLogger(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

int
SPHLogPortalEmpty (SPHLogPortal_t portal)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		if ( headerBlock->next_free ==  headerBlock->current_logger )
		{
			rc = 1;
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalEmpty(%p) next=%lx, current=%lx\n",  portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalEmpty(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

int
SPHLogPortalEntries (SPHLogPortal_t portal)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	int rc = -1;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		if ( headerBlock->next_free < headerBlock->list_size )
		{
			rc = headerBlock->next_free;
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalEntries(%p) next=%lx, size=%d\n",  portal,
					headerBlock->next_free, headerBlock->list_size);
#endif
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalEntries(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

int
SPHLogPortalCapacity (SPHLogPortal_t portal)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	int rc = 0;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		sas_read_barrier();
		rc = headerBlock->list_size;
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalCapacity(%p) list_size=%hd\n",
					portal, headerBlock->list_size);
#endif
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalCapacity(%p) type check failed\n",
				portal);
#endif
	}
	return rc;
}

SPHLFPortalIterator_t *
SPHLFPortalCreateIterator(SPHLogPortal_t portal,
		SPHLFPortalIterator_t *Iterator_space)
{
	SPHLFPortalIterator_t *iter	= NULL;
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
    SPHLFLogger_t log;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
    		SAS_RUNTIME_LOGPORTAL) )
    {
    	SPHLFLogIterator_t *l_iter = NULL;
    	log = headerBlock->log_buff_list[0];
#ifdef __SASDebugPrint__
    	sas_printf("SPHLFPortalCreateIterator(%p, %p) log[0]= %p\n",
    			portal, Iterator_space, log);
#endif
    	if (log)
    	{
    		l_iter = SPHLFLoggerCreateIterator(log, &Iterator_space->logIter);
    		if (l_iter)
    		{
    			iter = Iterator_space;
    			iter->portal = portal;
    			iter->current = 0;
    			iter->next_free = headerBlock->next_free;
    			iter->capacity = headerBlock->list_size;
    		}
    	}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHLFPortalCreateIterator(%p, %p) type check failed\n",
    			portal, Iterator_space);
#endif
    }

	return iter;
}

SPHLFLoggerHandle_t*
SPHLFPortalIteratorNext (SPHLFPortalIterator_t *iterator,
                         SPHLFLoggerHandle_t *handlespace)
{
	SPHLFLoggerHandle_t *handle = NULL;
	SPHLogPortal_t portal = iterator->portal;
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
    SPHLFLogger_t log, log0;

    if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
    		SAS_RUNTIME_LOGPORTAL) )
    {   /* iterator refs a valid portal.  */
    	log = iterator->logIter.logger;
#ifdef __SASDebugPrint__
    	sas_printf("SPHLFPortalIteratorNext(%p, %p) logger= %p\n",
    			iterator, handlespace, log);
#endif
    	if (log)
    	{   /* The iterator should point to a logger.  */
    		handle = SPHLFLoggerIteratorNext(&iterator->logIter, handlespace);
    		if (!handle)
    		{   /* implies we at then end of the current logger. */
    			longPtr_t cur = iterator->current;
    			longPtr_t nxt = cur + 1;
    			log0 = headerBlock->log_buff_list[cur];
    			if (log0 && (nxt <= iterator->next_free)
    			&& (nxt < iterator->capacity))
    			{   /* nxt is within the allocated list.  */
    				log = headerBlock->log_buff_list[nxt];
    				if (log)
    				{   /* and the list entry was filled in. */
    			    	SPHLFLogIterator_t *l_iter;
    		    		l_iter = SPHLFLoggerCreateIterator(log, &iterator->logIter);
    		    		if (l_iter)
    		    		{   /* and that pointer to a valid logger.  */
    		    			iterator->current = nxt;
    		        		handle = SPHLFLoggerIteratorNext(l_iter, handlespace);
    		    		}
    				}
    			}
    		}
    	}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHLFPortalIteratorNext(%p, %p) type check failed\n",
    			iterator, handlespace);
#endif
    }

	return handle;
}

SPHLFLoggerHandle_t*
SPHLogPortalAllocStrideTimeStamped (SPHLogPortal_t portal,
                             int catcode, int subcode,
                             SPHLFLoggerHandle_t *handleorg)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	SPHLFLogger_t log;
	longPtr_t log_index;
	SPHLFLoggerHandle_t *result = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		if ( headerBlock->next_free > headerBlock->current_logger )
		{
			log_index = headerBlock->current_logger;
			log = headerBlock->log_buff_list[log_index];
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalAllocStrideTimeStamped(%p) next=%ld, current=%ld\n",  portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
			result = SPHLFLoggerAllocStrideTimeStamped (log, catcode, subcode, handleorg);
			if (!result && SPHLFLoggerFull(log) && ((log_index+1) < headerBlock->next_free))
			{
				bool done = 0;
				/* The Logger entry allocation failed and we verified
				 * that the current Logger was Full so we also check of
				 * the next Logger entry is allocated. If this is all
				 * true, we want exactly one thread to bump the current
				 * pointer to the next list entry. */

				done = __sync_bool_compare_and_swap (&headerBlock->current_logger,
						                             log_index,
	                                                 (log_index+1));
				if (done)
				{
					/* The current logger was successfully adjusted by
					 * this thread. So retry directly with the next
					 * logger entry. */
					log = headerBlock->log_buff_list[log_index+1];
					result = SPHLFLoggerAllocStrideTimeStamped (log, catcode, subcode, handleorg);
				}

				if (!result)
				{
					/* Either The current thread lost the race to
					 * update current or the next logger is full too.
					 * Safest to return recursively rechecking all the
					 * boundaries. */
					sas_read_barrier();
					result = SPHLogPortalAllocStrideTimeStamped (portal,
							catcode, subcode, handleorg);
				}
			}
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalAllocStrideTimeStamped(%p,%d,%d,%p) type check failed\n",
				portal, catcode, subcode, handleorg);
#endif
	}
	return result;
}

SPHLFLoggerHandle_t*
SPHLogPortalAllocTimeStamped (SPHLogPortal_t portal,
                             int catcode, int subcode,
                             block_size_t alloc_size,
                             SPHLFLoggerHandle_t *handleorg)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	SPHLFLogger_t log;
	longPtr_t log_index;
	SPHLFLoggerHandle_t *result = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		if ( headerBlock->next_free > headerBlock->current_logger )
		{
			log_index = headerBlock->current_logger;
			log = headerBlock->log_buff_list[log_index];
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalAllocTimeStamped(%p) next=%ld, current=%ld\n",
					portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
			result = SPHLFLoggerAllocTimeStamped (log, catcode, subcode, alloc_size, handleorg);
			if (!result && SPHLFLoggerFull(log) && ((log_index+1) < headerBlock->next_free))
			{
				bool done = 0;
				/* The Logger entry allocation failed and we verified
				 * that the current Logger was Full so we also check of
				 * the next Logger entry is allocated. If this is all
				 * true, we want exactly one thread to bump the current
				 * pointer to the next list entry. */

				done = __sync_bool_compare_and_swap (&headerBlock->current_logger,
						                             log_index,
	                                                 (log_index+1));
				if (done)
				{
					/* The current logger was successfully adjusted by
					 * this thread. So retry directly with the next
					 * logger entry. */
					log = headerBlock->log_buff_list[log_index+1];
					result = SPHLFLoggerAllocTimeStamped (log, catcode, subcode, alloc_size, handleorg);
				}

				if (!result)
				{
					/* Either The current thread lost the race to
					 * update current or the next logger is full too.
					 * Safest to return recursively rechecking all the
					 * boundaries. */
					sas_read_barrier();
					result = SPHLogPortalAllocTimeStamped (portal,
							catcode, subcode, alloc_size, handleorg);
				}
			}
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalAllocTimeStamped(%p,%d,%d,%ld,%p) type check failed\n",
				portal, catcode, subcode, alloc_size, handleorg);
#endif
	}
	return result;
}

void *
SPHLogPortalAllocRaw (SPHLogPortal_t portal,
                         block_size_t alloc_size)
{
	SASLogPortalHeader	*headerBlock = (SASLogPortalHeader*)portal;
	SPHLFLogger_t log;
	longPtr_t log_index;
	void *result = NULL;

	if (SOMSASCheckBlockSigAndType ((SASBlockHeader*)headerBlock,
			SAS_RUNTIME_LOGPORTAL) )
	{
		if ( headerBlock->next_free > headerBlock->current_logger )
		{
			log_index = headerBlock->current_logger;
			log = headerBlock->log_buff_list[log_index];
#ifdef __SASDebugPrint__
			sas_printf("SPHLogPortalAllocRaw(%p) next=%ld, current=%ld\n",
					portal,
					headerBlock->next_free, headerBlock->current_logger);
#endif
			result = SPHLFLoggerAllocRaw (log, alloc_size);
			if (!result && SPHLFLoggerFull(log) && ((log_index+1) < headerBlock->next_free))
			{
				bool done = 0;
				/* The Logger entry allocation failed and we verified
				 * that the current Logger was Full so we also check of
				 * the next Logger entry is allocated. If this is all
				 * true, we want exactly one thread to bump the current
				 * pointer to the next list entry. */

				done = __sync_bool_compare_and_swap (&headerBlock->current_logger,
						                             log_index,
	                                                 (log_index+1));
				if (done)
				{
					/* The current logger was successfully adjusted by
					 * this thread. So retry directly with the next
					 * logger entry. */
					log = headerBlock->log_buff_list[log_index+1];
					result = SPHLFLoggerAllocRaw (log, alloc_size);
				}

				if (!result)
				{
					/* Either The current thread lost the race to
					 * update current or the next logger is full too.
					 * Safest to return recursively rechecking all the
					 * boundaries. */
					sas_read_barrier();
					result = SPHLogPortalAllocRaw (portal, alloc_size);
				}
			}
		}
#ifdef __SASDebugPrint__
	} else {
		sas_printf("SPHLogPortalAllocRaw(%p,%ld) type check failed\n",
				portal, alloc_size);
#endif
	}
	return result;
}
