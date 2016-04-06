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

#ifndef _BITV_H
#define _BITV_H
#include <unistd.h>

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

typedef unsigned long bitv_word;

typedef struct bitv_cb_t {
		size_t		alloc_unit;
		size_t		alloc_shift;
		bitv_word	alloc_mask;
		} bitv_cb_t;

static const size_t bits_per_long = sizeof(long)*8;

#if __INT_MAX__ == __LONG_MAX__
static const bitv_word bit_zero = 0x80000000;
#else
static const bitv_word bit_zero = 0x8000000000000000;
#endif

/* 
   Initialize the bit vector unit control block for size. This fixes
   the relationship between power of 2 units of allocation and size
   in bytes.

   Assumes:
	unit is nonzero
   verifies:
	unit is a power of 2
*/
extern __C__ void
bitv_init (bitv_cb_t *cb, size_t unit);

/* 
   Allocate size (rounded up to alloc_units) from the bit vector.

   Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	unit is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
extern __C__ ssize_t
bitv_alloc(bitv_cb_t *cb, bitv_word *bvec, size_t size);

/* 
   Allocate size (rounded up to alloc_units) from the bit vector.
   The resulting allocation is aligned to boundary specified.

   Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	align is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
extern __C__ ssize_t
bitv_aligned_alloc(bitv_cb_t *cb, bitv_word *bvec, size_t size, size_t align);

/* 
   Deallcates size (rounded up to alloc_units) from the bit vector
   stating unit offset specified by at.

   Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	That size fits within a bitv_word
   returns:
	0 if the deallocation was successful from this bitv_word
	or nonzero if something failed
*/
extern __C__ int
bitv_dealloc (bitv_cb_t *cb, bitv_word *bvec, bitv_word at, size_t size);

/* 
   Allocate size (rounded up to alloc_units) from the bit vector (*bvec)
   then mark the last unit of the allocation (*endvec).

   Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	unit is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
extern __C__ ssize_t
bitv_alloc_marked(bitv_cb_t *cb, bitv_word *bvec, bitv_word *endvec, size_t size);

/* 
   Allocate size (rounded up to alloc_units) from the bit vector (*bvec)
   then mark the last unit of the allocation (*endvec). The resulting
   allocation is aligned to boundary specified.

   Assumes:
	cb has been initialized by bitv_init()
	size is greater than 0
   verifies:
	unit is a power of 2
   returns:
	-1 if the allocation failed for this bitv_word
	or offset to the allocation, relative to the storage area
	represented by this bitv_word
*/
extern __C__ ssize_t
bitv_aligned_alloc_marked(bitv_cb_t *cb, bitv_word *bvec, bitv_word *endvec, size_t size, size_t align);

/*
   Calculate the free space for the bit vector based on the bit
   population count and the associated unit size.
*/
extern __C__ int
bitv_free_marked (bitv_cb_t *cb, bitv_word *bvec, bitv_word *endvec,
	bitv_word at);

/* 
   Use the allocation address/offset and the location of the
   corresponding end mark to compute the size/allocation units then
   clear the end mark before deaalocating the corresponding units from
   the bit vector.

   Assumes:
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
extern __C__ size_t
bitv_free_space (bitv_cb_t *cb, bitv_word *bvec);

#endif /* _BITV_H */
