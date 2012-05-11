/*
 * Copyright (c) 2009, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* #define CHECK_BITV_PRIMATIVES 1 */

#ifdef CHECK_BITV_PRIMATIVES 
#include <bitv_priv.h>

int
bitv_mask_to_start_mrk_test (void)
{
	int err = 0;
	bitv_word msk, res, chk;

	msk = 1UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = bit_zero;
	res = bitv_mask_to_start_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 4096UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 3UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != 2UL) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = (long)bit_zero >> 1;
	res = bitv_mask_to_start_mrk(msk);
	if (res != bit_zero) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 6144UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != 4096UL) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 15UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != 8UL) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = (long)bit_zero >> 3;
	res = bitv_mask_to_start_mrk(msk);
	if (res != bit_zero) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 15*4096UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != 8*4096UL) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 511*2048UL;
	res = bitv_mask_to_start_mrk(msk);
	if (res != 256*2048UL) {
		printf ("error bitv_mask_to_start_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	printf ("bitv_mask_to_start_mrk_test %d errors\n", err);
	return err;
}

int
bitv_mask_to_end_mrk_test (void)
{
	int err = 0;
	bitv_word msk, res, chk;

	msk = 1UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = bit_zero;
	res = bitv_mask_to_end_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 4096UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != msk) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 3UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != 1UL) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = (long)bit_zero >> 1;
	chk = bit_zero >> 1;
	res = bitv_mask_to_end_mrk(msk);
	if (res != chk) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 6144UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != 2048UL) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 15UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != 1UL) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = (long)bit_zero >> 3;
	chk = bit_zero >> 3;
	res = bitv_mask_to_end_mrk(msk);
	if (res != chk) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 15*4096UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != 4096UL) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	msk = 511*2048UL;
	res = bitv_mask_to_end_mrk(msk);
	if (res != 2048UL) {
		printf ("error bitv_mask_to_end_mrk_test(%lx) = %lx\n",
			msk, res);
	}

	printf ("bitv_mask_to_end_mrk_test %d errors\n", err);
	return err;
}

int
bitv_mask_gen_test (void)
{
	int err = 0;
	bitv_word msk;
	size_t sz, rsz;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);

	sz = 3;
	rsz = bitv_round_size(&cb, sz);
	if (rsz != 0x10) {
		printf("error bitv_round_unit(%lx)=%lx\n", sz, rsz);
		err++;
	}

	msk = bitv_size_to_mask(&cb, sz);
	if (msk != bit_zero) {
		printf("error bitv_size_to_mask(%lx)=%lx\n", sz, msk);
		err++;
	}

	sz = 69;
	rsz = bitv_round_size(&cb, sz);
	if (rsz != 0x50) {
		printf("error bitv_round_unit(%lx)=%lx\n", sz, rsz);
		err++;
	}

	msk = bitv_size_to_mask(&cb, sz);
	if (msk != ((long)bit_zero >> 4)) {
		printf("error bitv_size_to_mask(%lx)=%lx\n", sz, msk);
		err++;
	}

	sz = 127;
	rsz = bitv_round_size(&cb, sz);
	if (rsz != 0x80) {
		printf("error bitv_round_unit(%lx)=%lx\n", sz, rsz);
		err++;
	}

	msk = bitv_size_to_mask(&cb, sz);
	if (msk != ((long)bit_zero >> 7)) {
		printf("error bitv_size_to_mask(%lx)=%lx\n", sz, msk);
		err++;
	}

	sz = 128;
	rsz = bitv_round_size(&cb, sz);
	if (rsz != 0x80) {
		printf("error bitv_round_unit(%lx)=%lx\n", sz, rsz);
		err++;
	}

	msk = bitv_size_to_mask(&cb, sz);
	if (msk != ((long)bit_zero >> 7)) {
		printf("error bitv_size_to_mask(%lx)=%lx\n", sz, msk);
		err++;
	}

	sz = 129;
	rsz = bitv_round_size(&cb, sz);
	if (rsz != 0x90) {
		printf("error bitv_round_unit(%lx)=%lx\n", sz, rsz);
		err++;
	}

	msk = bitv_size_to_mask(&cb, sz);
	if (msk != ((long)bit_zero >> 8)) {
		printf("error bitv_size_to_mask(%lx)=%lx\n", sz, msk);
		err++;
	}

	printf ("bitv_mask_gen_test %d errors\n", err);
	return err;
}

int
bitv_popcountl_one_test (void)
{
	int err = 0;
	bitv_word vec = 0L;
	int rc;

	vec = 512;
	rc = bitv_popcountl_one(vec);
	if (rc == 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}

	vec = 1;
	rc = bitv_popcountl_one(vec);
	if (rc == 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}

	vec = bit_zero;
	rc = bitv_popcountl_one(vec);
	if (rc == 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}

	vec >>= 2;
	rc = bitv_popcountl_one(vec);
	if (rc == 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}


	vec = 512+16;
	rc = bitv_popcountl_one(vec);
	if (rc != 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}
	
	vec = 512-1;
	rc = bitv_popcountl_one(vec);
	if (rc != 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}

	vec = 0;
	rc = bitv_popcountl_one(vec);
	if (rc != 0) {
		printf("error bitv_popcountl_one(%lx)=%zu\n", vec, rc);
		err++;
	}

	printf ("bitv_popcountl_one_test %d errors\n", err);
	return err;
}
#else
#include <bitv.h>
#endif 

int
bitv_aligned_alloc_test (void)
{
	int err = 0;
	bitv_word vec = -1L;
	size_t sz, rsz;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	sz = 3;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 32);
	if (rsz != 0) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 9;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 32);
	if (rsz != 32) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 9;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 16);
	if (rsz != 16) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 69;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 128);
	if (rsz != 128) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 69;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 128);
	if (rsz != 256) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 96;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 128);
	if (rsz != 384) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 69;
	rsz = bitv_aligned_alloc(&cb, &vec, sz, 16);
	if (rsz != 48) 
	{
		printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	if (bits_per_long == 64) {
		sz = 257;
		rsz = bitv_aligned_alloc(&cb, &vec, sz, 512);
		if (rsz != 512) 
		{
			printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n",
				vec, sz, rsz);
			err++;
		}

		sz = 96;
		rsz = bitv_aligned_alloc(&cb, &vec, sz, 128);
		if (rsz != 896) 
		{
			printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n",
				vec, sz, rsz);
			err++;
		}

		sz = 90;
		rsz = bitv_aligned_alloc(&cb, &vec, sz, 32);
		if (rsz != 800) 
		{
			printf("error bitv_aligned_alloc(%lx, %zu)=%zu\n",
				vec, sz, rsz);
			err++;
		}
	}

	printf ("bitv_aligned_alloc_test %d errors\n", err);
	return err;
}

int
bitv_aligned_alloc_marked_test (void)
{
	int err = 0;
	bitv_word vec = -1L;
	bitv_word mrk = 0;
	bitv_word chk;
	size_t sz, rsz, align;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	sz = 3;
	align = 32;
	chk = bit_zero;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 0 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 9;
	chk |= bit_zero >> 2;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 32 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 9;
	align = 16;
	chk |= bit_zero >> 1;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 16 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 69;
	align = 128;
	chk |= bit_zero >> 12;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 128 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 69;
	chk |= bit_zero >> 20;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 256 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 96;
	chk |= bit_zero >> 29;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 384 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	sz = 69;
	align = 16;
	chk |= bit_zero >> 7;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 48 || mrk != chk) 
	{
		printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
		err++;
	}

	if (bits_per_long == 64) {
		sz = 257;
		align = 512;
		chk |= 1UL << 15;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 512 || mrk != chk) 
		{
			printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
			err++;
		}

		sz = 96;
		align = 128;
		chk |= 1UL << 2;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 896 || mrk != chk) 
		{
			printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
			err++;
		}

		sz = 90;
		align = 32;
		chk |= 1UL << 8;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 800 || mrk != chk) 
		{
			printf("error bitv_aligned_alloc_marked(%lx,%lx,%zu,%zu)=%zu\n",
			vec, mrk, sz, align, rsz);
			err++;
		}
	}

	printf ("bitv_aligned_alloc_marked_test %d errors\n", err);
	return err;
}

int
bitv_alloc_test (void)
{
	int err = 0;
	bitv_word vec = -1L;
	size_t sz, rsz;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	printf("bitv_alloc_test vec = %lx\n", vec);
	sz = 3;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 0) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 9;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 16) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 69;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 32) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 127;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 112) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 128;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 240) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	sz = 129;
	rsz = bitv_alloc(&cb, &vec, sz);
	if (rsz != 368) {
		printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
		err++;
	}

	if (bits_per_long == 64) {
		sz = 256;
		rsz = bitv_alloc(&cb, &vec, sz);
		if (rsz != 512) {
			printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
			err++;
		}
	
		sz = 257;
		rsz = bitv_alloc(&cb, &vec, sz);
		if (rsz != -1) {
			printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
			err++;
		}
	
		sz = 256;
		rsz = bitv_alloc(&cb, &vec, sz);
		if (rsz != 768) {
			printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
			err++;
		}
	} else {
		sz = 128;
		rsz = bitv_alloc(&cb, &vec, sz);
		if (rsz != -1) {
			printf("error bitv_alloc(%lx, %zu)=%zu\n", vec, sz, rsz);
			err++;
		}
	}

	printf ("bitv_alloc_test %d errors\n", err);
	return err;
}

int
bitv_alloc_marked_test (void)
{
	int err = 0;
	bitv_word vec = -1L;
	bitv_word mrk = 0;
	bitv_word chk;
	size_t sz, rsz;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	printf("bitv_alloc_marked_test vec = %lx mrk = %lx\n", vec, mrk);
	sz = 3;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if ((rsz != 0) || (mrk != bit_zero)) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	sz = 9;
	chk = (long)bit_zero>>1;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if ((rsz != 16) || (mrk != chk)) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	sz = 69;
	chk |= bit_zero >> 6;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if ((rsz != 32) || (mrk != chk)) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	sz = 127;
	chk |= bit_zero >> 14;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if (rsz != 112 || mrk != chk) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	sz = 128;
	chk |= bit_zero >> 22;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if (rsz != 240 || mrk != chk) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	sz = 129;
	chk |= bit_zero >> 31;
	rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
	if (rsz != 368 || mrk != chk) {
		printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
			vec, mrk, sz, rsz);
		err++;
	}

	if (bits_per_long == 64) {
		sz = 256;
		chk |= 1UL << 16;
		rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
		if (rsz != 512 || mrk != chk) {
			printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
				vec, mrk, sz, rsz);
			err++;
		}
	
		sz = 257;
		rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
		if (rsz != -1 || mrk != chk) {
			printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
				vec, mrk, sz, rsz);
			err++;
		}
	
		sz = 256;
		chk |= 1UL;
		rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
		if (rsz != 768 || mrk != chk) {
			printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
				vec, mrk, sz, rsz);
			err++;
		}
	} else {
		sz = 128;
		rsz = bitv_alloc_marked(&cb, &vec, &mrk, sz);
		if (rsz != -1 || mrk != chk) {
			printf("error bitv_alloc_marked(%lx, %lx, %zu)=%zu\n",
				vec, mrk, sz, rsz);
			err++;
		}
	}

	printf ("bitv_alloc_marked_test %d errors\n", err);
	return err;
}

int
bitv_dealloc_test (void)
{
	int err = 0;
	bitv_word vec = 0L;
	bitv_word at, tmp;
	size_t sz;
	int rc;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	printf("bitv_dealloc_test vec = %lx\n", vec);
	sz = 3;
	at = 512;
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if (rc != 0) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if (rc == 0) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}

	sz = 127;
	tmp = vec;
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if (rc == 0) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}
	at = 512 + 16;
	vec = tmp;
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if (rc != 0) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}

	sz = 65;
	at = 960;
	tmp = vec;
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if ((rc == 0) || (tmp != vec)) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}
	sz = 64;
	rc = bitv_dealloc(&cb, &vec, at, sz);
	if (rc != 0) {
		printf("error bitv_dealloc(%lx, %zu)=%d\n", vec, sz, rc);
		err++;
	}


	printf ("bitv_dealloc_test %d errors, vec=%lx\n", err, vec);
	return err;
}

int
bitv_free_marked_test (void)
{
	int err = 0;
	int rc;
	bitv_word vec = -1L;
	bitv_word mrk = 0;
	bitv_word chk;
	size_t sz, rsz, align;
	size_t of0, of1, of2, of3, of4, of5, of6, of7, of8, of9;
	size_t free, free_prev, delta, expect;
	bitv_cb_t cb;
	
	bitv_init(&cb, 16);
	
	sz = 3;
	align = 32;
	chk = bit_zero;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 0 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of0 = rsz;

	sz = 9;
	chk |= bit_zero >> 2;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 32 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of1 = rsz;

	sz = 9;
	align = 16;
	chk |= bit_zero >> 1;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 16 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of2 = rsz;

	sz = 69;
	align = 128;
	chk |= bit_zero >> 12;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 128 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of3 = rsz;

	sz = 69;
	chk |= bit_zero >> 20;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 256 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of4 = rsz;

	sz = 96;
	chk |= bit_zero >> 29;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 384 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of5 = rsz;

	sz = 69;
	align = 16;
	chk |= bit_zero >> 7;
	rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
	if (rsz != 48 || mrk != chk) 
	{
		printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
			sz, align, rsz);
		return 10;
	}
	of6 = rsz;

	if (bits_per_long == 64) {
		sz = 257;
		align = 512;
		chk |= 1UL << 15;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 512 || mrk != chk) 
		{
			printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
				sz, align, rsz);
			return 10;
		}
		of7 = rsz;

		sz = 96;
		align = 128;
		chk |= 1UL << 2;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 896 || mrk != chk) 
		{
			printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
				sz, align, rsz);
			return 10;
		}
		of8 = rsz;

		sz = 90;
		align = 32;
		chk |= 1UL << 8;
		rsz = bitv_aligned_alloc_marked(&cb, &vec, &mrk, sz, align);
		if (rsz != 800 || mrk != chk) 
		{
			printf("error bitv_free_marked_test, aligned_alloc(%zu,%zu) failed%zu\n",
				sz, align, rsz);
			return 10;
		}
		of9 = rsz;
		
		free_prev = bitv_free_space(&cb, &vec);
		expect = 96;
		rc = bitv_free_marked (&cb, &vec, &mrk, of9);
		if (rc)
		{
			printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
			err++;
		} else {
			free = bitv_free_space(&cb, &vec);
			delta = free - free_prev;
			if (delta != expect)
			{
				printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
				err++;
			}
			free_prev = free;
		}

		expect = 96;
		rc = bitv_free_marked (&cb, &vec, &mrk, of8);
		if (rc)
		{
			printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
			err++;
		} else {
			free = bitv_free_space(&cb, &vec);
			delta = free - free_prev;
			if (delta != expect)
			{
				printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
				err++;
			}
			free_prev = free;
		}

		expect = 272;
		rc = bitv_free_marked (&cb, &vec, &mrk, of7);
		if (rc)
		{
			printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
			err++;
		} else {
			free = bitv_free_space(&cb, &vec);
			delta = free - free_prev;
			if (delta != expect)
			{
				printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
				err++;
			}
			free_prev = free;
		}
	}

	free_prev = bitv_free_space(&cb, &vec);
	expect = 80;
	rc = bitv_free_marked (&cb, &vec, &mrk, of6);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected delta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	expect = 96;
	rc = bitv_free_marked (&cb, &vec, &mrk, of5);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected delta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	expect = 80;
	rc = bitv_free_marked (&cb, &vec, &mrk, of4);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	rc = bitv_free_marked (&cb, &vec, &mrk, of3);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	expect = 16;
	rc = bitv_free_marked (&cb, &vec, &mrk, of2);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	rc = bitv_free_marked (&cb, &vec, &mrk, of1);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	rc = bitv_free_marked (&cb, &vec, &mrk, of0);
	if (rc)
	{
		printf("error bitv_free_marked(%lx)=%d\n",
			vec, rc);
		err++;
	} else {
		free = bitv_free_space(&cb, &vec);
		delta = free - free_prev;
		if (delta != expect)
		{
			printf("error bitv_free_marked(%lx) expected detta %zu != %zu\n",
				vec, expect, delta);
			err++;
		}
		free_prev = free;
	}

	printf ("bitv_free_marked_test %d errors\n", err);
	return err;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	bitv_cb_t cb;

	bitv_init(&cb, 16);
	printf("bits_per_long = %zu\n", bits_per_long);
	printf("bit_zero = %lx\n", bit_zero);
	printf("alloc_unit = %zu\n", cb.alloc_unit);
	printf("alloc_mask = %lu\n", cb.alloc_mask);
	printf("alloc_shift = %zu\n", cb.alloc_shift);
#ifdef CHECK_BITV_PRIMATIVES
	rc =+ bitv_mask_gen_test();
	rc =+ bitv_popcountl_one_test();
	rc =+ bitv_mask_to_end_mrk_test();
	rc =+ bitv_mask_to_start_mrk_test();
#endif
	rc =+ bitv_alloc_test();

	rc =+ bitv_dealloc_test();

	rc =+ bitv_aligned_alloc_test();

	rc =+ bitv_alloc_marked_test();

	rc =+ bitv_aligned_alloc_marked_test();

	rc =+ bitv_free_marked_test();
	
	return (rc);
}
