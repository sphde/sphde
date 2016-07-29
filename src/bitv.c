/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

/* Change Activity: */
/* End Change Activity */
//#define PRINT_DEBUG 1
#include <bitv_priv.h>
#include <stdio.h>
#include "sasatom.h"

void
bitv_init (bitv_cb_t * cb, size_t unit)
{
/* Assumes:
	unit is nonzero
   verifies:
	unit is a power of 2
*/
  if (bitv_popcountl_one (unit))
    {
      cb->alloc_unit = unit;
      cb->alloc_mask = unit - 1;
      cb->alloc_shift = __builtin_ctzl (unit);
    }
  else
    {
      printf ("error bitv_init(unit=%zu) must be power of 2\n", unit);
    }
}

/* Need a fast estimate to decide if it is worth our while to scan this
   bitv_word again. */
#ifdef _ARCH_PWR5
/* popcount is good when hardware instructions can be inlined */
#define BITV_SPACE_EST(__vec,__units) (__builtin_popcountl(__vec) >= __units)
#else
/* Otherwise use a crude estimate that avoids the out of line runtime
   call and associated register spillage.

   First verify that the bitv_word is not zero, then count the leading
   and trailing zeros to compute the number of bits between and
   compare this to the number of bits we need to allocate */
#define BITV_SPACE_EST(__vec,__units) (__vec && \
(((bits_per_long - __builtin_clzl(__vec)) - __builtin_ctzl(__vec)) >= __units))
#endif

ssize_t
bitv_alloc (bitv_cb_t * cb, bitv_word * bvec, size_t size)
{
/* Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	unit is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
  ssize_t result = -1;
  size_t alloc_units = bitv_round_unit (cb, size);
  bitv_word req_msk0 = bitv_size_to_mask (cb, size);
  bitv_word cur_msk = *bvec;
#ifdef PRINT_DEBUG
  printf ("alloc_units=%ld, req_msk=%lx, cur_msk=%lx\n",
	  alloc_units, req_msk0, cur_msk);
#endif
  while (cur_msk != 0UL)
    {
      bitv_word req_msk;
      int i;
      int start = __builtin_clzl (cur_msk);
      if (start > (bits_per_long - alloc_units))
	return -1;

      req_msk = req_msk0 >> start;

      for (i = start; i <= (bits_per_long - alloc_units); i++)
	{
	  if ((cur_msk & req_msk) == req_msk)
	    {
	      bitv_word new_msk;
	      new_msk = cur_msk & ~req_msk;
	      ;
	      if (sas_compare_and_swap ((long int*)bvec, cur_msk, new_msk))
		{		/* cur_msk was still valid and update successful */
#ifdef PRINT_DEBUG
		  printf ("i=%ld, req_msk=%lx\n", i, req_msk);
#endif
		  result = i << cb->alloc_shift;
		  cur_msk = 0UL;
		}
	      else
		{
		  cur_msk = *bvec;
		  if (BITV_SPACE_EST (cur_msk, alloc_units))
		    {		/* might still have room, retry */
		    }
		  else
		    {		/* out of space */
		      cur_msk = 0UL;
		    }
		}
	      break;
	    }
	  else
	    {
	      req_msk >>= 1;
	    }
	}
    }

  return result;
}

ssize_t
bitv_alloc_marked (bitv_cb_t * cb, bitv_word * bvec, bitv_word * endvec,
		   size_t size)
{
/* Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	unit is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
  ssize_t result = -1;
  size_t alloc_units = bitv_round_unit (cb, size);
  bitv_word req_msk0 = bitv_size_to_mask (cb, size);
  bitv_word cur_msk = *bvec;
  long retry_cnt = 0;
#ifdef PRINT_DEBUG
  printf ("alloc_units=%ld, req_msk=%lx, cur_msk=%lx\n",
	  alloc_units, req_msk0, cur_msk);
#endif
  while (cur_msk != 0UL)
    {
      bitv_word req_msk;
      long i;
      long start_bit = __builtin_clzl (cur_msk);
      long end_bit = (bits_per_long - alloc_units);

      if (start_bit > end_bit)
	return -1;

      req_msk = req_msk0 >> start_bit;

      for (i = start_bit; i <= end_bit; i++)
	{
	  if ((cur_msk & req_msk) == req_msk)
	    {
	      bitv_word new_msk;
	      new_msk = cur_msk & ~req_msk;
	      ;
	      if (__sync_bool_compare_and_swap (bvec, cur_msk, new_msk))
		{		/* cur_msk was still valid and update successful */
		  bitv_word end_mrk, chk_mrk __attribute__((unused));
		  end_mrk = bitv_mask_to_end_mrk (req_msk);
		  chk_mrk = sas_fetch_and_or_long (endvec, end_mrk);
#ifdef PRINT_DEBUG
		  printf ("i=%ld, req_msk=%lx, end_mrk=%lx\n",
			  i, req_msk, end_mrk);
		  if (chk_mrk & req_msk)
		    {
		      printf ("bitv_alloc_marked: error prior end-mark inconsistent "
			"with unit vector free space\n");
		    }
#endif
		  result = i << cb->alloc_shift;
		  cur_msk = 0UL;
		}
	      else
		{
		  cur_msk = *bvec;
		  if (BITV_SPACE_EST (cur_msk, alloc_units))
		    {		/* might still have room, retry */
		      if (retry_cnt > 0)
			return -1;
		    }
		  else
		    {		/* out of space */
		      cur_msk = 0UL;
		    }
		  retry_cnt++;
		}
	      break;
	    }
	  else
	    {
	      req_msk >>= 1;
	      if (i == end_bit)
		cur_msk = 0UL;
	    }
	}
    }

  return result;
}

ssize_t
bitv_aligned_alloc (bitv_cb_t * cb, bitv_word * bvec, size_t size,
		    size_t align)
{
/* Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	align is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
  ssize_t result = -1;
  size_t alloc_units = bitv_round_unit (cb, size);
  size_t align_shift = __builtin_ffsl (align >> cb->alloc_shift);
  size_t align_units = bitv_round_unit (cb, align);
  bitv_word req_msk0 = bitv_size_to_mask (cb, size);
  bitv_word cur_msk = *bvec;
  if (align != (1 << (align_shift + cb->alloc_shift - 1)))
    {
#ifdef PRINT_DEBUG
      printf ("error align value must be a power of 2\n");
      printf ("bitv_aligned_alloc(%p,%lx,%ld,%ld)\n", cb, *bvec, size, align);
      printf
	("alloc_units=%ld, align_units=%ld, align_shift=%ld, req_msk=%lx, cur_msk=%lx\n",
	 alloc_units, align_units, align_shift, req_msk0, cur_msk);
#endif
      return -1;
    }

  while (cur_msk != 0UL)
    {
      bitv_word req_msk;
      int i;
      int start = __builtin_clzl (cur_msk);

      start = (start + align_units - 1) & (~(align_units - 1));
      if (start > (bits_per_long - alloc_units))
	return -1;

      req_msk = req_msk0 >> start;

      for (i = start; i <= (bits_per_long - alloc_units); i += align_units)
	{
	  if ((cur_msk & req_msk) == req_msk)
	    {
	      bitv_word new_msk;
	      new_msk = cur_msk & ~req_msk;
	      ;
	      if (__sync_bool_compare_and_swap (bvec, cur_msk, new_msk))
		{		/* cur_msk was still valid and update successful */
#ifdef PRINT_DEBUG
		  printf ("i=%ld, req_msk=%lx\n", i, req_msk);
#endif
		  result = i << cb->alloc_shift;
		  cur_msk = 0UL;
		}
	      else
		{
		  cur_msk = *bvec;
		  if (BITV_SPACE_EST (cur_msk, alloc_units))
		    {		/* might still have room, retry */
		    }
		  else
		    {		/* out of space */
		      cur_msk = 0UL;
		    }
		}
	      break;
	    }
	  else
	    {
	      req_msk >>= align_units;
	      if (req_msk == 0UL)
		cur_msk = 0UL;
	    }
	}
    }

  return result;
}

ssize_t
bitv_aligned_alloc_marked (bitv_cb_t * cb, bitv_word * bvec,
			   bitv_word * endvec, size_t size, size_t align)
{
/* Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	align is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
  ssize_t result = -1;
  size_t alloc_units = bitv_round_unit (cb, size);
  size_t align_shift = __builtin_ffsl (align >> cb->alloc_shift);
  size_t align_units = bitv_round_unit (cb, align);
  bitv_word req_msk0 = bitv_size_to_mask (cb, size);
  bitv_word cur_msk = *bvec;
  if (align != (1 << (align_shift + cb->alloc_shift - 1)))
    {
#ifdef PRINT_DEBUG
      printf ("error align value must be a power of 2\n");
      printf ("bitv_aligned_alloc(%p,%lx,%ld,%ld)\n", cb, *bvec, size, align);
      printf
	("alloc_units=%ld, align_units=%ld, align_shift=%ld, req_msk=%lx, cur_msk=%lx\n",
	 alloc_units, align_units, align_shift, req_msk0, cur_msk);
#endif
      return -1;
    }

  while (cur_msk != 0UL)
    {
      bitv_word req_msk;
      int i;
      int start = __builtin_clzl (cur_msk);

      start = (start + align_units - 1) & (~(align_units - 1));
      if (start > (bits_per_long - alloc_units))
	return -1;

      req_msk = req_msk0 >> start;
#ifdef PRINT_DEBUG
      printf ("start=%ld, alloc_units=%ld, req_msk=%lx\n",
	      start, alloc_units, req_msk);
#endif

      for (i = start; i <= (bits_per_long - alloc_units); i += align_units)
	{
#ifdef PRINT_DEBUG
	  printf ("i=%ld, cur_msk=%lx, req_msk=%lx\n", i, cur_msk, req_msk);
#endif
	  if ((cur_msk & req_msk) == req_msk)
	    {
	      bitv_word new_msk;
	      new_msk = cur_msk & ~req_msk;
	      ;
	      if (__sync_bool_compare_and_swap (bvec, cur_msk, new_msk))
		{		/* cur_msk was still valid and update successful */
		  bitv_word end_mrk, chk_mrk __attribute__((unused));
		  end_mrk = bitv_mask_to_end_mrk (req_msk);
		  chk_mrk = sas_fetch_and_or_long (endvec, end_mrk);
#ifdef PRINT_DEBUG
		  printf ("i=%ld, req_msk=%lx, end_mrk=%lx\n",
			  i, req_msk, end_mrk);
		  if (chk_mrk & req_msk)
		    {
		      printf
			("bitv_alloc_marked: error prior end-mark inconsistent with unit vector free space\n");
		    }
#endif
		  result = i << cb->alloc_shift;
		  cur_msk = 0UL;
		}
	      else
		{
		  cur_msk = *bvec;
		  if (BITV_SPACE_EST (cur_msk, alloc_units))
		    {		/* might still have room, retry */
		    }
		  else
		    {		/* out of space */
		      cur_msk = 0UL;
		    }
		}
	      break;
	    }
	  else
	    {
	      if (i < (bits_per_long - alloc_units))
		req_msk >>= align_units;
	      else
		cur_msk = 0UL;
	    }
	}
    }

  return result;
}

static int
bitv_dealloc_internal (bitv_word * bvec, bitv_word offset, size_t alloc_units)
{
/* Assumes:
	alloc_units is greater than 0
   verifies:
	That alloc_units fit within a bitv_word
   returns:
	0 if the deallocation was successful from this bitv_word
	or nonzero if something failed
*/
  bitv_word req_msk = bitv_units_to_mask (alloc_units);
  bitv_word prv_msk;
  /* Insure the allocated units + offset fit within the long vector */
  if ((alloc_units + offset > bits_per_long))
    return -1;

  req_msk >>= offset;
  prv_msk = sas_fetch_and_or_long (bvec, req_msk);
  /* Insure that we are deallocating units that were previously
     allocated. If so the logical and of the original unit mask
     and the requested mask should be 0s. */
  prv_msk &= req_msk;
#ifdef PRINT_DEBUG
  printf ("alloc_units=%ld, offset=%ld, req_msk=%lx rc=%d\n",
	  alloc_units, offset, req_msk, (prv_msk != 0));
#endif
  return (prv_msk != 0);
}

int
bitv_dealloc (bitv_cb_t * cb, bitv_word * bvec, bitv_word at, size_t size)
{
/* Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	That size fits within a bitv_word
   returns:
	0 if the deallocation was successful from this bitv_word
	or nonzero if something failed
*/
  size_t alloc_units = bitv_round_unit (cb, size);
  bitv_word offset = (at >> cb->alloc_shift) & (bits_per_long - 1);
#ifdef PRINT_DEBUG
  printf ("bitv_dealloc(%lx, %lx. %ld)\n", *bvec, at, size);
#endif
  return bitv_dealloc_internal (bvec, offset, alloc_units);
}

size_t
bitv_free_space (bitv_cb_t * cb, bitv_word * bvec)
{
  return (size_t) __builtin_popcountl (*bvec) << cb->alloc_shift;
}

int
bitv_free_marked (bitv_cb_t * cb, bitv_word * bvec, bitv_word * endvec,
		  bitv_word at)
{
/* Assumes:
	cb has been initialized by bitv_init()
	bvec and endvec has been set by bitv_alloc_marked or
	bitv_aligned_alloc_marked
   verifies:
	That an end mark is set between the starting offset and the
	end of end makr vector.
   returns:
	0 if the deallocation was successful from this bitv_word
	or nonzero if something failed
*/
  int rc = -1;
  size_t alloc_units;
  bitv_word offset = (at >> cb->alloc_shift) & (bits_per_long - 1);
  bitv_word endmsk = *endvec;
  bitv_word endmrk = endmsk << offset;
  bitv_word bgnmrk = *bvec << offset;

#ifdef PRINT_DEBUG
  printf ("bitv_free_marked(%lx, %lx. %ld)\n", *bvec, *endvec, at);
  printf (" offset=%ld, endmsk=%lx, endmrk=%lx, bgnmrk=%lx\n",
	  offset, endmsk, endmrk, bgnmrk);
#endif
  /* verify that the endmark exists and that offset is the start
     of an allocated range */
  if (endmrk && !(bgnmrk & bit_zero))
    {
      int units = __builtin_clzl (endmrk);
      bitv_word chkmsk __attribute__((unused));

      alloc_units = units + 1;
      endmrk = bit_zero >> (offset + units);

      chkmsk = sas_fetch_and_and_long (endvec, ~endmrk);

#ifdef PRINT_DEBUG
      printf (" units=%ld, endmrk=%lx, chkmsk=%lx\n", units, endmrk, chkmsk);
      if (!(chkmsk & endmrk))
	{
	  printf
	    ("bitv_free_marked: error prior end-mark inconsistent with unit vector free space\n");
	}
#endif
      rc = bitv_dealloc_internal (bvec, offset, alloc_units);
    }

  return rc;
}
