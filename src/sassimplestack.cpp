/*
 * Copyright (c) 2005, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */


#define __SASDebugPrint__ 1
#define sas_printf printf
#include <stdlib.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sassimplestack.h"

typedef struct SASSimpleStackHeader
{
  SASBlockHeader blockHeader;
  node_size_t next_stack;
  node_size_t start_stack;
  node_size_t end_stack;
  node_size_t align_mask;
  void *dummy4;
  void *dummy5;
  void *dummy6;
  freeNode *headerFreeSpace;
} SASSimpleStackHeader;

#ifdef __WORDSIZE_64
#define HEAP_OFFSET 128
#define DEFAULT_PAGE 4096
#else
#define HEAP_OFFSET 64
#define DEFAULT_PAGE 4096
#endif
#define DEFAULT_ALIGN_MASK	(~(sizeof(void*) + sizeof(void*) - 1));

SASSimpleStack_t
SASSimpleStackInitInternal (void *heap_seg, sas_type_t sasType,
			    block_size_t heap_size)
{
  SASSimpleStackHeader *heapBlock = (SASSimpleStackHeader *) heap_seg;
  char *stackStart = NULL;
  char *stackEnd = NULL;
  node_size_t remaining;

  if (heapBlock)
    {
      initSOMSASBlock ((SASBlockHeader *) heapBlock, sasType,
		       heap_size, NULL);
    }

  stackStart = (char *) heapBlock + DEFAULT_PAGE;
  stackEnd = (char *) heapBlock + heap_size;
  heapBlock->next_stack = (node_size_t) stackStart;
  heapBlock->start_stack = (node_size_t) stackStart;
  heapBlock->end_stack = (node_size_t) stackEnd;
  heapBlock->align_mask = (node_size_t) DEFAULT_ALIGN_MASK;

  remaining = DEFAULT_PAGE - sizeof (SASSimpleStackHeader);
  heapBlock->headerFreeSpace = (freeNode *) & heapBlock[1];
  freeNode_init (heapBlock->headerFreeSpace, remaining);
  heapBlock->blockHeader.baseBlock = (SASBlockHeader *) heapBlock;
  heapBlock->blockHeader.nextBlock = (SASBlockHeader *) heapBlock;

  return (SASSimpleStack_t) heapBlock;
}

SASSimpleStack_t
SASSimpleStackInit (void *heap_seg, block_size_t heap_size)
{
  return SASSimpleStackInitInternal (heap_seg,
				     SAS_RUNTIME_SIMPLESTACK, heap_size);
}

SASSimpleStack_t
SASSimpleStackCreate (block_size_t heap_size)
{
  SASBlockHeader *heapBlock = NULL;

  heapBlock = (SASBlockHeader *) SASBlockAlloc ((long) heap_size);
  if (heapBlock)
    {
      return SASSimpleStackInit (heapBlock, heap_size);
    }
  else
    return NULL;
}

void *
SASSimpleStackAllocNoLock (SASSimpleStack_t heap, block_size_t alloc_size)
{
  SASSimpleStackHeader *headerBlock = (SASSimpleStackHeader *) heap;
  node_size_t round;
  node_size_t stacknext = 0;
  node_size_t stack_alloc = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_SIMPLESTACK))
    {
      round = ~headerBlock->align_mask;
      stacknext = (headerBlock->next_stack + alloc_size + round)
	& headerBlock->align_mask;
      if (stacknext <= headerBlock->end_stack)
	{
	  stack_alloc = headerBlock->next_stack;
	  headerBlock->next_stack = stacknext;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SASSimpleStackAlloc(%p, %zu) alloc failed\n",
		      heap, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackAlloc(%p, %zu) type check failed\n",
		  heap, alloc_size);
#endif
    }
  return (void *) stack_alloc;
}

void *
SASSimpleStackAlloc (SASSimpleStack_t heap, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  void *stack_alloc = NULL;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_SIMPLESTACK))
    {
      SASLock (heap, SasUserLock__WRITE);
      stack_alloc = SASSimpleStackAllocNoLock (heap, alloc_size);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackAlloc(%p, %zu) type check failed\n",
		  heap, alloc_size);
#endif
    }
  return stack_alloc;
}

void *
SASSimpleStackNearAllocNoLock (void *nearObj, block_size_t alloc_size)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  void *stack_alloc = NULL;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) nearHeader,
					  SAS_RUNTIME_SIMPLESTACK))
	    {
	      SASSimpleStack_t nearStack = (SASSimpleStack_t) nearHeader;
	      stack_alloc = SASSimpleStackAllocNoLock (nearStack, alloc_size);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASSimpleStackNearAllocNoLock(%p, %zu) type check failed @%p\n",
		 nearObj, alloc_size, nearHeader);
#endif
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASSimpleStackNearAllocNoLock(%p, %zu) block check failed @%p\n",
	     nearObj, alloc_size, nearHeader);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SASSimpleStackNearAllocNoLock(%p, %zu) find header failed\n",
	 nearObj, alloc_size);
#endif
    }
  return stack_alloc;
}

void *
SASSimpleStackNearAlloc (void *nearObj, block_size_t alloc_size)
{
  SASBlockHeader *nearHeader = SASFindHeader (nearObj);
  void *stack_alloc = NULL;

  if (nearHeader != NULL)
    {
      if (SOMSASCheckBlockSig (nearHeader))
	{
	  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) nearHeader,
					  SAS_RUNTIME_SIMPLESTACK))
	    {
	      SASSimpleStack_t nearStack = (SASSimpleStack_t) nearHeader;
	      SASLock (nearHeader, SasUserLock__WRITE);
	      stack_alloc = SASSimpleStackAllocNoLock (nearStack, alloc_size);
	      SASUnlock (nearHeader);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SASSimpleStackNearAlloc(%p, %zu) type check failed @%p\n",
		 nearObj, alloc_size, nearHeader);
#endif
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SASSimpleStackNearAlloc(%p, %zu) block check failed @%p\n",
	     nearObj, alloc_size, nearHeader);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackNearAlloc(%p, %zu) find header failed\n",
		  nearObj, alloc_size);
#endif
    }
  return stack_alloc;
}

int
SASSimpleStackDeallocNoLock (SASSimpleStack_t heap, void *stack_pointer)
{
  SASSimpleStackHeader *headerBlock = (SASSimpleStackHeader *) heap;
  node_size_t new_sp = (node_size_t) stack_pointer;
  int rc = -1;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_SIMPLESTACK))
    {
      if ((new_sp >= headerBlock->start_stack)
	  && (new_sp <= headerBlock->end_stack))
	{
	  headerBlock->next_stack = new_sp;
	  rc = 0;
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SASSimpleStackDeallocNoLock(%p, %p) contained check failed\n",
	     heap, stack_pointer);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackDeallocNoLock(%p, %p) type check failed\n",
		  heap, stack_pointer);
#endif
    }
  return rc;
}

int
SASSimpleStackDealloc (SASSimpleStack_t heap, void *stack_pointer)
{
  SASSimpleStackHeader *headerBlock = (SASSimpleStackHeader *) heap;
  node_size_t new_sp = (node_size_t) stack_pointer;
  int rc = -1;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_RUNTIME_SIMPLESTACK))
    {
      SASLock (headerBlock, SasUserLock__WRITE);
      if ((new_sp >= headerBlock->start_stack)
	  && (new_sp <= headerBlock->end_stack))
	{
	  new_sp &= headerBlock->align_mask;
	  if (new_sp == (node_size_t) stack_pointer)
	    {
	      headerBlock->next_stack = new_sp;
	      rc = 0;
	    }
	  else
	    {
	      rc = -3;
#ifdef __SASDebugPrint__
	      sas_printf
		("SASSimpleStackDealloc(%p, %p) alignment check failed\n",
		 heap, stack_pointer);
#endif
	    }
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SASSimpleStackDealloc(%p, %p) contained check failed\n", heap,
	     stack_pointer);
#endif
	}
      SASUnlock (headerBlock);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackDealloc(%p, %p) type check failed\n",
		  heap, stack_pointer);
#endif
    }
  return rc;
}

block_size_t
SASSimpleStackFreeSpaceNoLock (SASSimpleStack_t heap)
{
  SASSimpleStackHeader *headerBlock = (SASSimpleStackHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_SIMPLESTACK_TYPE))
    {
      heapFree = headerBlock->end_stack - headerBlock->next_stack;
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackFreeSpace(%p) does not match type/subtype\n",
		  heap);
#endif
    }
  return heapFree;
}

block_size_t
SASSimpleStackFreeSpace (SASSimpleStack_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType ((SASBlockHeader *) headerBlock,
				  SAS_SIMPLESTACK_TYPE))
    {
      SASLock (heap, SasUserLock__WRITE);
      heapFree = SASSimpleStackFreeSpaceNoLock (heap);
      SASUnlock (heap);
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASSimpleStackFreeSpace(%p) does not match type/subtype\n",
		  heap);
#endif
    }
  return heapFree;
}

int
SASSimpleStackDestroyNoLock (SASSimpleStack_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_SIMPLESTACK))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (heap, heapSize);
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASSimpleStackDestroy(%p) does not match type/subtype\n",
		  heap);
#endif
      rc = -1;
    }
  return rc;
}

int
SASSimpleStackDestroy (SASSimpleStack_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_SIMPLESTACK))
    {
      SASLock (heap, SasUserLock__WRITE);
      rc = SASSimpleStackDestroyNoLock (heap);
      SASUnlock (heap);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASSimpleStackDestroy(%p) does not match type/subtype\n",
		  heap);
#endif
      rc = -1;
    }
  return rc;
}
