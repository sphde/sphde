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

#ifndef _BITV_PRIV_H
#define _BITV_PRIV_H

#include <bitv.h>

static inline bitv_word
bitv_round_mask (bitv_cb_t *cb)
{
/* Assumes:
	cb has been initialized by bitvector_init()
   Returns:
	the alloc_mask.
*/
	return (cb->alloc_mask);
}

static inline void*
bitv_round_ptr_to_unit (bitv_cb_t *cb, void *ptr)
{
/* Assumes:
	cb has been initialized by bitvector_init()
   Returns:
	the ptr value rounded up to the unit size alignment
*/
	bitv_word word_ptr = (bitv_word)ptr;
	return (void*)((word_ptr + cb->alloc_mask) & (~(cb->alloc_mask)));
}

static inline bitv_word
bitv_mask_to_end_mrk (bitv_word msk)
{
/* Assumes:
	msk is the allocation mask before insertion
   Returns:
	a single 1 bit in the location of the last 1 bit in the msk.
*/
	return (msk & (~(msk<<1)));
}

static inline bitv_word
bitv_mask_to_start_mrk (bitv_word msk)
{
/* Assumes:
	msk is the allocation mask before insertion
   Returns:
	a single 1 bit in the location of the 1st 1 bit in the msk.
*/
	return (msk & (~(msk>>1)));
}

static inline int
bitv_popcountl_one (bitv_word vec)
{
/* return true if the bitwise population count is exactly 1.
   Effectively verifies that size values are an exact power of 2.
   We use this instead of __builtin_popcount because only the
   newest processors implement hardware popcnt but most implement
   count leading zeros. */
	return (vec && (vec == (bit_zero >> __builtin_clzl(vec))));
}

static inline size_t
bitv_round_unit (bitv_cb_t *cb, size_t size)
{
/* Assumes:
	cb has been initialized by bitvector_init()
*/
	bitv_word result = (size + cb->alloc_mask) >> cb->alloc_shift;
	return result;
}

static inline size_t
bitv_round_size (bitv_cb_t *cb, size_t size)
{
/* Assumes:
	cb has been initialized by bitvector_init()
	size is greater than 0
*/
	bitv_word msk = cb->alloc_mask;
	bitv_word result = (size + msk) & (~msk);
	return result;
}

static inline bitv_word
bitv_size_to_mask (bitv_cb_t *cb, size_t size)
{
/* Assumes:
	cb has been initialized by bitvector_init()
	size is greater than 0
*/
	long mask = (long) bit_zero;
	size_t cnt = bitv_round_unit(cb, size) - 1;

	return (bitv_word)(mask >>= cnt);
}

static inline bitv_word
bitv_units_to_mask (size_t units)
{
/* Assumes:
	cb has been initialized by bitvector_init()
	size is greater than 0
*/
	long mask = (long) bit_zero;
	size_t cnt = units - 1;

	return (bitv_word)(mask >>= cnt);
}

static inline size_t
bitv_allocated_size (bitv_cb_t *cb, bitv_word *bvec, bitv_word *endvec, bitv_word at)
{
/* Assumes:
	cb has been initialized by bitv_init()
	bvec and endvec has been set by bitv_alloc_marked or
	bitv_aligned_alloc_marked
   verifies:
	That an end mark is set between the starting offset and the
	end of end makr vector.
   returns:
	Non-zero allocation size in bytes 0 if the deallocation was successful from this bitv_word
	or zero of  no allocation
*/
	size_t alloc_units = 0;
	bitv_word offset = (at >> cb->alloc_shift) & (bits_per_long - 1);
	bitv_word endmsk = *endvec;
	bitv_word endmrk = endmsk << offset;

	if (endmrk) {
		int units = __builtin_clzl(endmrk);

		alloc_units = units + 1;
	}

	return (alloc_units << cb->alloc_shift);
}

static inline size_t
bitv_allocated_size_chk (bitv_cb_t *cb, bitv_word *bvec, bitv_word *endvec, bitv_word at, size_t alloc_size)
{
/* Assumes:
	cb has been initialized by bitv_init()
	bvec and endvec has been set by bitv_alloc_marked or
	bitv_aligned_alloc_marked
   verifies:
	That an end mark is set between the starting offset and the
	end of end makr vector.
   returns:
	Non-zero allocation size in bytes 0 if the deallocation was successful from this bitv_word
	or zero of  no allocation
*/
	size_t alloc_units = 0;
	bitv_word offset = (at >> cb->alloc_shift) & (bits_per_long - 1);
	bitv_word endmsk = *endvec;
	bitv_word endmrk = endmsk << offset;
	size_t chk_size = bitv_round_unit(cb, alloc_size);

	if (endmrk) {
		size_t units = __builtin_clzl(endmrk) + 1;

		if (chk_size == units)
			alloc_units = (units << cb->alloc_shift);
	}

	return alloc_units;
}

#endif /* _BITV_PRIV_H */
