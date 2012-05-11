/*
 * Copyright (c) 2010, 2011 IBM Corporation.
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
#include <stdlib.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sphlockfreeheap.h"
#include <bitv_priv.h>

/* Define a common maximum units for allocation to be the same for
   32-bit and 64-bit systems. so that we can get (mostly the same
   results for both. */
#define MAX_UNITS 32

typedef struct SPHLockFreeHeapHeader
{
  SASBlockHeader blockHeader;
  bitv_cb_t bitv_cb;
  unsigned short vec_cnt;
  bitv_word *alloc_vec;
  bitv_word *endmrk_vec;
  bitv_word vecbuf[8];
} SPHLockFreeHeapHeader;

SPHLockFreeHeap_t
SPHLockFreeHeapInit (void *heap_seg, sas_type_t sasType,
		     block_size_t heap_size, size_t unit_size)
{
  SASBlockHeader *heapBlock = (SASBlockHeader *) heap_seg;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap_seg;

#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapInit(%p,%x,%ld,%zu)\n",
	      heap_seg, sasType, heap_size, unit_size);
  if (sizeof (SPHLockFreeHeapHeader) > heap_offset)
    {
      sas_printf
	("SPHLockFreeHeapInit  sizeof(SPHLockFreeHeapHeader) = %zu\n",
	 sizeof (SPHLockFreeHeapHeader));
    }
#endif
  if (heapBlock)
    {
      bitv_word *endbuf_vec;
      void *endbuf;
      size_t header_size = heap_offset;

      initSOMSASBlock (heapBlock, sasType, heap_size, NULL);
      bitv_init (&heapHdr->bitv_cb, unit_size);

      heapHdr->vec_cnt = (heap_size / bits_per_long) / unit_size;
      heapHdr->alloc_vec = heapHdr->vecbuf;
      heapHdr->endmrk_vec = &heapHdr->vecbuf[heapHdr->vec_cnt];
      endbuf_vec = &heapHdr->vecbuf[(heapHdr->vec_cnt) * 2];
      endbuf = bitv_round_ptr_to_unit (&heapHdr->bitv_cb, endbuf_vec);
      header_size = (unsigned long) endbuf - (unsigned long) heapBlock;
#ifdef __SASDebugPrint__
      sas_printf
	("SPHLockFreeHeapInit alloc=%p, endmrk=%p, endbuf=%p, header_size=%zu\n",
	 heapHdr->alloc_vec, heapHdr->endmrk_vec, endbuf_vec, header_size);
#endif
      for (int i = 0; i < heapHdr->vec_cnt; i++)
	{
	  heapHdr->alloc_vec[i] = (bitv_word) ((long) -1);
	  heapHdr->endmrk_vec[i] = 0;
#ifdef __SASDebugPrint__L2
	  sas_printf ("SPHLockFreeHeapInit [%ld] %lx, %lx\n",
		      i, heapHdr->alloc_vec[i], heapHdr->endmrk_vec[i]);
#endif
	}

      /* Mark space for the header allocated */
      if (bitv_round_unit (&heapHdr->bitv_cb, header_size) <= MAX_UNITS)
	{
	  bitv_alloc (&heapHdr->bitv_cb, heapHdr->alloc_vec, header_size);
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLockFreeHeapInit [0] %lx, %lx\n",
		      heapHdr->alloc_vec[0], heapHdr->endmrk_vec[0]);
#endif
	}
      else
	{
	  if (bitv_round_unit (&heapHdr->bitv_cb, header_size) <=
	      MAX_UNITS * 2)
	    {
	      if (bits_per_long == MAX_UNITS)
		{
		  size_t adj_size = header_size - (unit_size * MAX_UNITS);
		  heapHdr->alloc_vec[0] = 0;
		  bitv_alloc (&heapHdr->bitv_cb, &heapHdr->alloc_vec[1],
			      adj_size);
#ifdef __SASDebugPrint__
		  sas_printf ("SPHLockFreeHeapInit [0] %lx, %lx\n",
			      heapHdr->alloc_vec[0], heapHdr->endmrk_vec[0]);
		  sas_printf ("SPHLockFreeHeapInit [1] %lx, %lx\n",
			      heapHdr->alloc_vec[1], heapHdr->endmrk_vec[1]);
#endif
		}
	      else
		{
		  bitv_alloc (&heapHdr->bitv_cb, heapHdr->alloc_vec,
			      header_size);
#ifdef __SASDebugPrint__
		  sas_printf ("SPHLockFreeHeapInit [0] %lx, %lx\n",
			      heapHdr->alloc_vec[0], heapHdr->endmrk_vec[0]);
#endif
		}
	    }
	  else
	    {
	      heapBlock = (SASBlockHeader *) - 1;
	    }
	}
    }

  return (SPHLockFreeHeap_t) heapBlock;
}

SPHLockFreeHeap_t
SPHLockFreeHeapCreate (block_size_t heap_size, block_size_t unit_size)
{
  SASBlockHeader *heapBlock = NULL;
  SPHLockFreeHeap_t result = NULL;

  /* Both the heap size and unit size must be a power od 2 */
  if (bitv_popcountl_one (heap_size) && bitv_popcountl_one (unit_size))
    {
      if (heap_size > (bits_per_long * unit_size))
	{
	  heapBlock =
	    (SASBlockHeader *) SASBlockAlloc ((unsigned long) heap_size);
	  if (heapBlock)
	    {
	      result = SPHLockFreeHeapInit (heapBlock,
					    SAS_RUNTIME_LOCKFREEHEAP,
					    heap_size, unit_size);
	      if (result == (SPHLockFreeHeap_t) (-1L))
		{		/* The init failed, The header size required to
				   cover the allocated size exceeded MAX_UNITS. */
		  SASBlockDealloc ((void *) heapBlock,
				   (unsigned long) heap_size);
		  result = NULL;
		}
	    }
	  else
	    result = NULL;
	}
    }

  return result;
}

void *
SPHLockFreeHeapAllocNoLock (SPHLockFreeHeap_t heap, size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  bitv_word result = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long unit_shift = heapHdr->bitv_cb.alloc_shift;
      block_size_t maxAlloc;
#if 0
      maxAlloc = bits_per_long << (unit_shift - 1);
#else
      maxAlloc = MAX_UNITS << unit_shift;
#endif
      if (alloc_size < maxAlloc)
	{
	  ssize_t offset = -1;
	  long cnt = heapHdr->vec_cnt;

	  for (long i = 0; i < cnt; i++)
	    {
	      if (heapHdr->alloc_vec[i] != 0)
		{
#ifdef __SASDebugPrint__
		  sas_printf ("SPHLockFreeHeapAlloc:[%ld] %lx %lx\n",
			      i, heapHdr->alloc_vec[i],
			      heapHdr->endmrk_vec[i]);
#endif
		  offset = bitv_alloc_marked (&heapHdr->bitv_cb,
					      &heapHdr->alloc_vec[i],
					      &heapHdr->endmrk_vec[i],
					      alloc_size);
		  if (offset != -1)
		    {
		      bitv_word byte_off;
		      byte_off = ((i * bits_per_long) << unit_shift) + offset;
		      result = (bitv_word) heap + byte_off;
#ifdef __SASDebugPrint__
		      sas_printf ("SPHLockFreeHeapAlloc:[%ld,%zu]->%lx\n",
				  i, offset, result);
#endif
		      break;
		    }
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf ("SPHLockFreeHeapAlloc(%p, %zu) range check failed\n",
		      heap, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLockFreeHeapAlloc(%p, %zu) type check failed\n",
		  heap, alloc_size);
#endif
    }
  return (void *) result;
}

void *
SPHLockFreeHeapAlloc (SPHLockFreeHeap_t heap, block_size_t alloc_size)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapAlloc(%p, %ld)\n", heap, alloc_size);
#endif
  return SPHLockFreeHeapAllocNoLock (heap, alloc_size);
}

void *
SPHLockFreeHeapAlignAllocNoLock (SPHLockFreeHeap_t heap,
				 block_size_t alloc_size,
				 block_size_t alignment)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  bitv_word result = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long unit_shift = heapHdr->bitv_cb.alloc_shift;
      block_size_t maxAlloc, maxAlign;

#if 0
      maxAlloc = bits_per_long << (unit_shift - 1);
      maxAlign = bits_per_long << (unit_shift);
#else
      maxAlloc = MAX_UNITS << unit_shift;
      maxAlign = bits_per_long << (unit_shift);
#endif
      if ((alloc_size < maxAlloc)
	  && bitv_popcountl_one (alignment) && (alignment <= (maxAlign)))
	{
	  ssize_t offset = -1;
	  long cnt = heapHdr->vec_cnt;

	  for (long i = 0; i < cnt; i++)
	    {
	      if (heapHdr->alloc_vec[i] != 0)
		{
#ifdef __SASDebugPrint__
		  sas_printf
		    ("SPHLockFreeHeapAlignAllocNoLock:[%ld] %lx, %lx\n", i,
		     heapHdr->alloc_vec[i], heapHdr->endmrk_vec[i]);
#endif
		  offset = bitv_aligned_alloc_marked (&heapHdr->bitv_cb,
						      &heapHdr->alloc_vec[i],
						      &heapHdr->endmrk_vec[i],
						      alloc_size, alignment);
		  if (offset != -1)
		    {
		      bitv_word byte_off;
		      byte_off = ((i * bits_per_long) << unit_shift) + offset;
		      result = (bitv_word) heap + byte_off;
#ifdef __SASDebugPrint__
		      sas_printf
			("SPHLockFreeHeapAlignAllocNoLock:[%ld,%zu]->%lx\n",
			 i, offset, result);
#endif
		      break;
		    }
		}
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SPHLockFreeHeapAlignAllocNoLock(%p,%ld,%ld) range check failed\n",
	     heap, alloc_size, alignment);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapAlignAllocNoLock(%p,%ld,%ld) type check failed\n",
	 heap, alloc_size, alignment);
#endif
    }
  return (void *) result;
}

void *
SPHLockFreeHeapAlignAlloc (SPHLockFreeHeap_t heap,
			   block_size_t alloc_size, block_size_t alignment)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapAlignAlloc(%p, %ld, %ld)\n",
	      heap, alloc_size, alignment);
#endif
  return SPHLockFreeHeapAlignAllocNoLock (heap, alloc_size, alignment);
}

int
SPHLockFreeHeapNotContainedNoLock (SPHLockFreeHeap_t heap, void *block)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t allocBlock = (block_size_t) block;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      block_size_t heap_size = headerBlock->blockSize;
      block_size_t heap_start = (block_size_t) heap;
      block_size_t heap_end = heap_start + heap_size;
      if ((allocBlock < heap_start) || (allocBlock >= heap_end))
	rc = -2;
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf
	("SPHLockFreeHeapNotContained(%p, ...) does not match type/subtype\n",
	 heap);
#endif
    }
  return rc;
}

int
SPHLockFreeHeapFreeInNoLock (SPHLockFreeHeap_t heap,
			     void *free_block, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  block_size_t heapSize;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long unit_shift = heapHdr->bitv_cb.alloc_shift;

      heapSize = headerBlock->blockSize;
      if (!
	  (((block_size_t) free_block ^ (block_size_t) heap) &
	   ~(heapSize - 1)))
	{
	  block_size_t offset = (block_size_t) free_block & (heapSize - 1);
	  block_size_t index = offset >> unit_shift;
	  block_size_t word_index = index / bits_per_long;
	  block_size_t cnt = heapHdr->vec_cnt;
	  block_size_t maxAlloc;
	  block_size_t allocated = 1;

	  if (alloc_size > 0)
	    {			/* only perform this check if alloc_size non-zero */
	      maxAlloc = MAX_UNITS << unit_shift;

	      allocated = bitv_allocated_size_chk (&heapHdr->bitv_cb,
						   &heapHdr->
						   alloc_vec[word_index],
						   &heapHdr->
						   endmrk_vec[word_index],
						   offset, alloc_size);
#ifdef __SASDebugPrint__
	      sas_printf
		("SPHLockFreeHeapFree: size=%ld, allocated=%ld max=%ld\n",
		 alloc_size, allocated, maxAlloc);
#endif
	      if ((alloc_size > allocated) || (alloc_size >= maxAlloc))
		{		/* alloc_size range check fail */
		  allocated = 0;
		}
	    }
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHLockFreeHeapFree: offset=%lx, index=%ld, word=%ld\n", offset,
	     index, word_index);
#endif
	  if ((word_index < cnt) && (allocated != 0))
	    {
	      if (bitv_free_marked (&heapHdr->bitv_cb,
				    &heapHdr->alloc_vec[word_index],
				    &heapHdr->endmrk_vec[word_index], offset))
		rc = -4;
	    }
	  else
	    {
	      rc = -3;
	    }
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLockFreeHeapFree(%p, %p, %ld) range check failed\n",
		      heap, free_block, alloc_size);
#endif
	}
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf
	("SPHLockFreeHeapFree(%p, ...) does not match type/subtype\n", heap);
#endif
    }
  return rc;
}

#if 0
int
SPHLockFreeHeapFreeNoLock (SPHLockFreeHeap_t heap,
			   void *free_block, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  block_size_t heapSize;
  int rc = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long unit_shift = heapHdr->bitv_cb.alloc_shift;

      heapSize = headerBlock->blockSize;
      if (!
	  (((block_size_t) free_block ^ (block_size_t) heap) &
	   ~(heapSize - 1)))
	{
	  block_size_t offset = (block_size_t) free_block & (heapSize - 1);
	  block_size_t index = offset >> unit_shift;
	  block_size_t word_index = index / bits_per_long;
	  long cnt = heapHdr->vec_cnt;
	  block_size_t maxAlloc;
	  block_size_t allocated = 1;

	  if (alloc_size > 0)
	    {			/* only perform this check if alloc_size non-zero */
	      maxAlloc = bits_per_long << (unit_shift - 1);
	      allocated = bitv_allocated_size_chk (&heapHdr->bitv_cb,
						   &heapHdr->
						   alloc_vec[word_index],
						   &heapHdr->
						   endmrk_vec[word_index],
						   offset, alloc_size);
#ifdef __SASDebugPrint__
	      sas_printf
		("SPHLockFreeHeapFree: size=%ld, allocated=%ld max=%ld\n",
		 alloc_size, allocated, maxAlloc);
#endif
	      if ((alloc_size > allocated) || (alloc_size >= maxAlloc))
		{		/* alloc_size range check fail */
		  allocated = 0;
		}
	    }
#ifdef __SASDebugPrint__
	  sas_printf
	    ("SPHLockFreeHeapFree: offset=%lx, index=%ld, word=%ld\n", offset,
	     index, word_index);
#endif
	  if ((word_index < cnt) && (allocated != 0))
	    {
	      if (bitv_free_marked (&heapHdr->bitv_cb,
				    &heapHdr->alloc_vec[word_index],
				    &heapHdr->endmrk_vec[word_index], offset))
		rc = -4;
	    }
	  else
	    {
	      rc = -3;
	    }
	}
      else
	{
	  rc = -2;
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLockFreeHeapFree(%p, %p, %d) range check failed\n",
		      heap, free_block, alloc_size);
#endif
	}
    }
  else
    {
      rc = -1;
#ifdef __SASDebugPrint__
      sas_printf
	("SPHLockFreeHeapFree(%p, ...) does not match type/subtype\n", heap);
#endif
    }
  return rc;
}
#endif
int
SPHLockFreeHeapFreeChk (SPHLockFreeHeap_t heap,
			void *free_block, block_size_t alloc_size)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFreeChk(%p, %p, %ld)\n",
	      heap, free_block, alloc_size);
#endif
  return SPHLockFreeHeapFreeInNoLock (heap, free_block, alloc_size);
}

int
SPHLockFreeHeapFree (SPHLockFreeHeap_t heap, void *free_block)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFree(%p, %p)\n", heap, free_block);
#endif
  return SPHLockFreeHeapFreeInNoLock (heap, free_block, 0);
}

int
SPHLockFreeHeapFreeNearChk (void *free_block, block_size_t alloc_size)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFreeNearChk(%p)\n", free_block);
#endif
  SASBlockHeader *headerBlock;
  int result = -5;

  headerBlock = (SASBlockHeader *) SASFindHeader (free_block);
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFreeNearChk(%p) header = %p\n",
	      free_block, headerBlock);
#endif
  if (headerBlock)
    {
      result = SPHLockFreeHeapFreeInNoLock (headerBlock,
					    free_block, alloc_size);
    }
  return result;
}

int
SPHLockFreeHeapFreeNear (void *free_block)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFreeNear(%p)\n", free_block);
#endif
  SASBlockHeader *headerBlock;
  int result = -5;

  headerBlock = (SASBlockHeader *) SASFindHeader (free_block);
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFreeNear(%p) header = %p\n",
	      free_block, headerBlock);
#endif
  if (headerBlock)
    {
      result = SPHLockFreeHeapFreeInNoLock (headerBlock, free_block, 0);
    }
  return result;
}

int
SPHLockFreeHeapFreeIn (SPHLockFreeHeap_t heap, void *free_block)
{
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapFree(%p, %p)\n", heap, free_block);
#endif
  return SPHLockFreeHeapFreeInNoLock (heap, free_block, 0);
}

SPHLockFreeHeap_t
SPHLockFreeHeapNearFind (void *nearObj)
{
  SPHLockFreeHeap_t result = NULL;
  SASBlockHeader *headerBlock;

  headerBlock = (SASBlockHeader *) SASFindHeader (nearObj);
  if (headerBlock)
    {
      if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
	{
	  result = (SPHLockFreeHeap_t) headerBlock;
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SPHLockFreeHeapNearFind(%p) doesn't match type/subtype\n",
	     nearObj);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SPHLockFreeHeapNearFind(%p) header not found\n", nearObj);
#endif
    }
  return result;
}

void *
SPHLockFreeHeapNearAllocNoLock (void *nearObj, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock;
  bitv_word result = 0;

  headerBlock = (SASBlockHeader *) SASFindHeader (nearObj);
#ifdef __SASDebugPrint__
  sas_printf ("SPHLockFreeHeapNearAllocNoLock(%p, %ld) header = %p\n",
	      nearObj, alloc_size, headerBlock);
#endif
  if (headerBlock)
    {
      if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
	{
#if 1
	  return (SPHLockFreeHeapAllocNoLock (headerBlock, alloc_size));
#else
	  SPHLockFreeHeapHeader *heapHdr =
	    (SPHLockFreeHeapHeader *) headerBlock;
	  long unit_shift = heapHdr->bitv_cb.alloc_shift;
	  block_size_t maxAlloc;

#if 0
	  maxAlloc = bits_per_long << (unit_shift - 1);
#else
	  maxAlloc = MAX_UNITS << unit_shift;
#endif
	  if (alloc_size < maxAlloc)
	    {
	      ssize_t offset = -1;
	      long i;
	      long cnt = heapHdr->vec_cnt;

	      for (i = 0; i < cnt; i++)
		{
		  if (heapHdr->alloc_vec[i] != 0)
		    {
		      offset = bitv_alloc_marked (&heapHdr->bitv_cb,
						  &heapHdr->alloc_vec[i],
						  &heapHdr->endmrk_vec[i],
						  alloc_size);

		      if (offset != -1)
			{
			  offset = (i << unit_shift) + offset;
			  result = (bitv_word) heapHdr + offset;
			  break;
			}
		    }
		}
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SPHLockFreeHeapNearAllocNoLock(%p, %ld) range check failed\n",
		 nearObj, alloc_size);
#endif
	    }
#endif
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SPHLockFreeHeapNearAllocNoLock(%p, %ld) doesn't match type/subtype\n",
	     nearObj, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapNearAllocNoLock(%p, %ld) header not found\n",
	 nearObj, alloc_size);
#endif
    }
  return (void *) result;
}

void *
SPHLockFreeHeapNearAlloc (void *nearObj, long allocSize)
{
  return SPHLockFreeHeapNearAllocNoLock (nearObj, allocSize);
}


#if 0
/* Would be used for interal allocations but is currently not used */
void
SPHLockFreeHeapNearDeallocNoLock (void *nearObj, block_size_t alloc_size)
{
  SASBlockHeader *headerBlock;
  block_size_t heapSize;
  freeNode *free_node = (freeNode *) nearObj;


  headerBlock = (SASBlockHeader *) SASFindHeader (nearObj);
  if (headerBlock)
    {
      if (SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE))
	{
	  free_node->init (alloc_size);
	  heapSize = headerBlock->blockSize - heap_offset;
	  if ((alloc_size < heapSize)
	      && ((unsigned long) nearObj >=
		  ((unsigned long) headerBlock + heap_offset)))
	    {
	      free_node->deallocSpace (&headerBlock->blockFreeSpace,
				       alloc_size);
#ifdef __SASDebugPrint__
	    }
	  else
	    {
	      sas_printf
		("SPHLockFreeHeapNearDeallocNoLock(%p, %ld) range check failed\n",
		 nearObj, alloc_size);
#endif
	    }
#ifdef __SASDebugPrint__
	}
      else
	{
	  sas_printf
	    ("SPHLockFreeHeapNearDeallocNoLock(%p, %ld) doesn't match type/subtype\n",
	     nearObj, alloc_size);
#endif
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapNearDeallocNoLock(%p, %ld) header not found\n",
	 nearObj, alloc_size);
#endif
    }
}

void
SPHLockFreeHeapNearDealloc (void *memAddr, long allocSize)
{
  SPHLockFreeHeapNearDeallocNoLock (memAddr, allocSize);
}
#endif

block_size_t
SPHLockFreeHeapFreeSpaceNoLock (SPHLockFreeHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  block_size_t heapFree = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long i;
      long cnt = heapHdr->vec_cnt;

      for (i = 0; i < cnt; i++)
	{
#ifdef __SASDebugPrint__
	  sas_printf ("SPHLockFreeHeapFreeSpace(%p) alloc_vec[%ld]=%lx \n",
		      heap, i, heapHdr->alloc_vec[i]);
#endif
	  if (heapHdr->alloc_vec[i] != 0)
	    {
	      heapFree += bitv_free_space (&heapHdr->bitv_cb,
					   &heapHdr->alloc_vec[i]);
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapFreeSpace(%p) does not match type/subtype\n", heap);
#endif
    }
  return heapFree;
}

block_size_t
SPHLockFreeHeapFreeSpace (SPHLockFreeHeap_t heap)
{
  return SPHLockFreeHeapFreeSpaceNoLock (heap);
}

int
SPHLockFreeHeapEmptyNoLock (SPHLockFreeHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  bitv_word accum_vec = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long i;
      long cnt = heapHdr->vec_cnt;

      for (i = 0; i < cnt; i++)
	{
	  accum_vec |= heapHdr->endmrk_vec[i];
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapEmptyNoLock(%p) does not match type/subtype\n",
	 heap);
#endif
    }
  return (accum_vec == 0);
}

int
SPHLockFreeHeapEmpty (SPHLockFreeHeap_t heap)
{
  return SPHLockFreeHeapEmptyNoLock (heap);
}

int
SPHLockFreeHeapFullNoLock (SPHLockFreeHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  SPHLockFreeHeapHeader *heapHdr = (SPHLockFreeHeapHeader *) heap;
  bitv_word accum_vec = 0;

  if (SOMSASCheckBlockSigAndType (headerBlock, SAS_RUNTIME_LOCKFREEHEAP))
    {
      long i;
      long cnt = heapHdr->vec_cnt;

      for (i = 0; i < cnt; i++)
	{
	  accum_vec |= heapHdr->alloc_vec[i];
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf
	("SPHLockFreeHeapFullNoLock(%p) does not match type/subtype\n", heap);
#endif
    }
  return (accum_vec == 0);
}

int
SPHLockFreeHeapFull (SPHLockFreeHeap_t heap)
{
  return SPHLockFreeHeapFullNoLock (heap);
}

int
SPHLockFreeHeapDestroyNoLock (SPHLockFreeHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  block_size_t heapSize;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREEHEAP))
    {
      heapSize = headerBlock->blockSize;
      SASBlockDealloc (heap, heapSize);
      rc = 0;
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLockFreeHeapDestroy(%p) does not match type/subtype\n",
		  heap);
#endif
      rc = -1;
    }
  return rc;
}

int
SPHLockFreeHeapDestroy (SPHLockFreeHeap_t heap)
{
  SASBlockHeader *headerBlock = (SASBlockHeader *) heap;
  int rc;

  if (SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock,
					    SAS_RUNTIME_LOCKFREEHEAP))
    {
      SASLock (heap, SasUserLock__WRITE);
      rc = SPHLockFreeHeapDestroyNoLock (heap);
      SASUnlock (heap);
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SPHLockFreeHeapDestroy(%p) does not match type/subtype\n",
		  heap);
#endif
      rc = -1;
    }
  return rc;
}
