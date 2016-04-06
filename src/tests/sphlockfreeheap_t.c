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
#include <string.h>

#include "sastype.h"
#include "sassim.h"
#include "sphlockfreeheap.h"

static const long int prime_cnt = 7;
static int prime_size[7] = {1,13,2,11,3,7,5};

static const long int alloc_cnt = 331;
static void *alloc_track[331];


int
lockfree_align_test (char *x4k)
{
    int rc = 0;
    int status;
    SPHLockFreeHeap_t lfHeap;
    block_size_t lfspace, lfTemp;
    block_size_t aSize, align;
    char *temp0, *temp1, *temp2, *temp3;

    memset (x4k, 0, 4096);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		4096, 16);
    if (lfHeap)
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_align_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_align_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

	aSize = 88;
	align = 128;
	temp0 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp0)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp0);
	    strcpy (temp0, "temp0");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    if ((unsigned long)temp0 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp0, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	    printf("lockfree_align_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	} else {
	    printf("lockfree_align_test SPHLockFreeHeapEmpty(%p) is correct\n",
		lfHeap);
	}

	aSize = 88;
	align = 192;
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (!temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96))
	    {
	    	printf("error lockfree_align_test(%p) lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L);
		rc++;
	    } else {
	    	printf("lockfree_align_test SPHLockFreeHeapAlignAlloc detected invalid aligmnent\n");
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 48;
	align = 64;
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+48))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+48L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 16;
	align = 16;
	temp2 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp2)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+48+16))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+48L+16L));
		rc++;
	    }

	    if (((unsigned long)temp2 > (unsigned long)temp1)
	    &&  ((unsigned long)temp2 < (unsigned long)temp0))
	    {
	    	printf("lockfree_align_test: %p < %p < %p allocation is correct\n",
		temp1, temp2, temp0);
	    } else {
	    	printf("lockfree_align_test: allocations should be temp1 < temp2 < temp0\n");
	    }
	    if ((unsigned long)temp2 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp2, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	if (0 != strcmp(temp2, "temp2"))
	{
	    printf("lockfree_align_test data verify temp2=%s failed\n", temp2);
	    return 10;
	}

	aSize = 16;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp2);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp2, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+48))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+48L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp2, status);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_align_test data verify temp1=%s failed\n", temp2);
	    return 10;
	}

	aSize = 48;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp1);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp1, status);
	    return 10;
	}

	aSize = 511;
	align = 256;
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+512L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 511;
	align = 256;
	temp2 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp2)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L));
		rc++;
	    }
	    if ((unsigned long)temp2 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp2, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 511;
	align = 256;
	temp3 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp3)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp3);
	    strcpy (temp3, "temp3");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L+512L));
		rc++;
	    }
	    if ((unsigned long)temp3 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp3, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_align_test data verify temp1=%s failed\n", temp2);
	    return 10;
	}

	aSize = 511;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp1);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp1, status);
	    return 10;
	}

	if (0 != strcmp(temp2, "temp2"))
	{
	    printf("lockfree_align_test data verify temp2=%s failed\n", temp2);
	    return 10;
	}

	aSize = 511;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp2);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp2, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp2, status);
	    return 10;
	}

	aSize = 256;
	align = 64;
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+512L+256L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 511;
	align = 256;
	temp2 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp2)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+256L+512L));
		rc++;
	    }
	    if ((unsigned long)temp2 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp2, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_align_test data verify temp1=%s failed\n", temp2);
	    return 10;
	}

	aSize = 256;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp1);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld)\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp1, status);
	    return 10;
	}

	aSize = 256;
	align = 512;
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+512L+256L+512L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_align_test data verify temp1=%s failed\n", temp2);
	    return 10;
	}

	aSize = 256;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp1);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp1, status);
	    return 10;
	}

	aSize = 256;
	if (sizeof(long) == sizeof(long long))
	{
	    align = 1024;
	} else {
	    align = 512;
	}
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+512L+256L+512L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_align_test data verify temp1=%s failed\n", temp2);
	    return 10;
	}

	aSize = 256;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp1);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp1, status);
	    return 10;
	}

	if (0 != strcmp(temp2, "temp2"))
	{
	    printf("lockfree_align_test data verify temp2=%s failed\n", temp2);
	    return 10;
	}

	aSize = 511;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp2);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp2, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp2, status);
	    return 10;
	}

	if (0 != strcmp(temp3, "temp3"))
	{
	    printf("lockfree_align_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 511;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp3);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp3, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp3, status);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_align_test data verify temp0=%s failed\n", temp2);
	    return 10;
	}

	aSize = 96;
	status = SPHLockFreeHeapFreeIn (lfHeap, temp0);
	if (!status)
	{
	    printf("lockfree_align_test SPHLockFreeHeapFreeIn(%p,%p) = %d\n",
		lfHeap, temp0, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+0))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (0L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapFreeIn(%p,%p) failed =%d\n",
		lfHeap, lfHeap, temp0, status);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_align_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

	aSize = 96;
	align = 2048;
	temp3 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (!temp3)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp3);

	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+0))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (0L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p != 0\n",
		lfHeap, aSize, align, temp3);
	    return 10;
	}

	aSize = 96;
	if (sizeof(long) == sizeof(long long))
	{
	    align = 1024;
	} else {
	    align = 512;
	}
	temp0 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp0)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp0);
	    strcpy (temp0, "temp0");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    if ((unsigned long)temp0 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp0, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 256;
	if (sizeof(long) == sizeof(long long))
	{
	    align = 1024;
	} else {
	    align = 512;
	}
	temp1 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp1)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+256))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 96L+256L);
		rc++;
	    }
	    if ((unsigned long)temp1 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp1, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 511;
	if (sizeof(long) == sizeof(long long))
	{
	    align = 1024;
	} else {
	    align = 512;
	}
	temp2 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp2)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+256+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+256L+512L));
		rc++;
	    }
	    if ((unsigned long)temp2 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp2, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	/* In 32-bit this test should fail because the alignment is
	   greater then the atomic word size.
	   In 64-bit this test should fail because we are out of room
	   at that alignement */
	aSize = 511;
	align = 1024;
	temp3 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (!temp3)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp3);

	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		       lfHeap, lfspace, lfTemp, (96L+512L+256L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) != 0\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 96;
	align = 2048;
	temp3 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (!temp3)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp3);

	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+256L));
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) != 0\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}

	aSize = 511;
	align = 512;
	temp3 = (char*)SPHLockFreeHeapAlignAlloc (lfHeap, aSize, align);
	if (temp3)
	{
	    printf("lockfree_align_test SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) = %p\n",
		lfHeap, aSize, align, temp3);
	    strcpy (temp3, "temp3");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+96+512+256+512))
	    {
	    	printf("error lockfree_align_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (96L+512L+256L+512L));
		rc++;
	    }
	    if ((unsigned long)temp3 & (align-1))
	    {
	    	printf("error lockfree_align_test(%p) %p not %zu aligned\n",
		lfHeap, temp2, align);
		rc++;
	    }
	} else {
	    printf("error lockfree_align_test(%p)  SPHLockFreeHeapAlignAlloc(%p,%zu,%zu) failed\n",
		lfHeap, lfHeap, aSize, align);
	    return 10;
	}
    } else {
	printf("error lockfree_align_test(%p)  SPHLockFreeHeapInit(%p,%x,4096,16) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP);
	return 10;
    }

    return rc;
}

int
lockfree_basic_test (char *x4k)
{
    int rc = 0;
    int status;
    SPHLockFreeHeap_t lfHeap;
    block_size_t lfspace, lfTemp;
    block_size_t aSize;;
    char *temp0, *temp1, *temp2, *temp3;

    memset (x4k, 0, 4096);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		4096, 16);
    if (lfHeap)
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_basic_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

	aSize = 96;
	temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp0)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp0);
	    strcpy (temp0, "temp0");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, lfHeap, aSize);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp0=%s failed\n", temp0);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) succeeds\n",
		lfHeap);
	}

	aSize = 128;
	temp1 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp1)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, (aSize)+96);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp0=%s failed\n", temp0);
	    return 10;
	}

	aSize = 496;
	temp2 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp2)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96+128))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, (aSize)+96+128);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp0=%s failed\n", temp0);
	    return 10;
	}
#if 1
	aSize = 512;
	temp3 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (!temp3)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp3);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+496+96+128))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, 496L+96L+128L);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) exceeds mask limit\n",
		lfHeap, aSize);
	    return 10;
	}
#endif
#if 0
	aSize = 511;
	temp3 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (!temp3)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp3);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+496+96+128))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, 496+96+128);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) exceeds mask limit\n",
		lfHeap, aSize);
	    return 10;
	}
#endif
	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp0=%s failed\n", temp0);
	    return 10;
	}
	
	aSize = 496;
	temp3 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp3)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp3);
	    strcpy (temp3, "temp3");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96+128+496))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, (aSize)+96+128+496);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) exceeds mask limit\n",
		lfHeap, aSize);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp0=%s failed\n", temp0);
	    return 10;
	}

	aSize = 96;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp0, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp0, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+128+496+496))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (128L+496L+496L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp0, aSize, status);
	    return 10;
	}

	if (0 != strcmp(temp2, "temp2"))
	{
	    printf("lockfree_basic_test data verify temp2=%s failed\n", temp2);
	    return 10;
	}

	aSize = 496;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp2, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp2, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+128+496))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (128L+496L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp2, aSize, status);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) succeeds\n",
		lfHeap);
	}

	aSize = 256;
	temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp0)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp0);
	    strcpy (temp0, "temp0");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+128+496))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	aSize = 352;
	temp2 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp2)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+256+128+496))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, (aSize)+256+128+496);
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	if (0 != strcmp(temp3, "temp3"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 496;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp3, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp3, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+256+128+352))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (256L+128L+352L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp3, aSize, status);
	    return 10;
	}

	if (0 != strcmp(temp2, "temp2"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 352;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp2, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp2, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+256+128))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (256L+128L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp2, aSize, status);
	    return 10;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 256;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp1, aSize);
	if (status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp1, aSize, status);
	    printf("lockfree_basic_test detected size > allocated size\n");
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp1, aSize, status);
	    rc++;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 512;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp1, aSize);
	if (status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp1, aSize, status);
	    printf("lockfree_basic_test detected size >= max size\n");
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp1, aSize, status);
	    rc++;
	}

	if (0 != strcmp(temp1, "temp1"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 128;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp1, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp1, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+256))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (256L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp1, aSize, status);
	    return 10;
	}

	if (0 != strcmp(temp0, "temp0"))
	{
	    printf("lockfree_basic_test data verify temp3=%s failed\n", temp2);
	    return 10;
	}

	aSize = 0;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp0, aSize);
	if (!status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp0, aSize, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+0))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (0L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp0, aSize, status);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) true\n",
		lfHeap);
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

        if (!SPHLockFreeHeapFull (lfHeap))
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFull(%p) false\n",
		lfHeap);
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapFull(%p) failed\n",
		lfHeap);
	    rc++;
	}

	aSize = 0;
	status = SPHLockFreeHeapFreeChk (lfHeap, 
		temp1, aSize);
	if (status)
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk(%p,%p,%zu) = %d\n",
		lfHeap, temp1, aSize, status);
	    printf("lockfree_basic_test SPHLockFreeHeapFreeChk detected double free\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+0))
	    {
	    	printf("error lockfree_basic_test(%p)  lfspace (%zu != (%zu+%ld))\n",
		lfHeap, lfspace, lfTemp, (0L));
		rc++;
	    }
	} else {
	    printf("error lockfree_basic_test(%p)  SPHLockFreeHeapFreeChk(%p,%p,%zu) failed =%d\n",
		lfHeap, lfHeap, temp1, aSize, status);
	    return 10;
	}

	lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	while ( lfTemp > 0 )
	{
	    if (lfTemp > 256)
		aSize = 256;
	    else
		aSize = lfTemp;

	    temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	    if (temp0)
	    {
		printf("lockfree_basic_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
			lfHeap, aSize, temp0);
	    } else {
		printf("error lockfree_basic_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
			x4k, lfHeap, aSize);
		return 10;
	    }
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	}

        if (SPHLockFreeHeapFull (lfHeap))
	{
	    printf("lockfree_basic_test SPHLockFreeHeapFull(%p) true\n",
		lfHeap);
	} else {
	    printf("lockfree_basic_test SPHLockFreeHeapFull(%p) failed\n",
		lfHeap);
	    rc++;
	}
    } else {
	printf("error lockfree_basic_test(%p)  SPHLockFreeHeapInit(%p,%x,4096,16) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP);
	return 10;
    }

    return rc;
}

int
lockfree_near_test (char *x4k)
{
    int rc = 0;
    int status;
    SPHLockFreeHeap_t lfHeap;
    block_size_t lfspace, lfTemp;
    block_size_t aSize;;
    char *temp0, *temp1, *temp2, *temp3;

    memset (x4k, 0, 4096);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		4096, 16);
    if (lfHeap)
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_near_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_near_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

	aSize = 96;
	temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
	if (temp0)
	{
	    printf("lockfree_near_test SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp0);
	    strcpy (temp0, "temp0");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_near_test  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	    printf("lockfree_near_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	} else {
	    printf("lockfree_near_test SPHLockFreeHeapEmpty(%p) succeeds\n",
		lfHeap);
	}

	aSize = 128;
	temp1 = (char*)SPHLockFreeHeapNearAlloc (lfHeap, aSize);
	if (temp1)
	{
	    printf("lockfree_near_test SPHLockFreeHeapNearAlloc(%p, %zu) = %p\n",
		lfHeap, aSize, temp1);
	    strcpy (temp1, "temp1");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_near_test SPHLockFreeHeapNearAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	aSize = 192;
	temp2 = (char*)SPHLockFreeHeapNearAlloc (temp0, aSize);
	if (temp2)
	{
	    printf("lockfree_near_test SPHLockFreeHeapNearAlloc(%p, %zu) = %p\n",
		temp0, aSize, temp2);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96+128))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_near_test SPHLockFreeHeapNearAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	aSize = 64;
	temp3 = (char*)SPHLockFreeHeapNearAlloc (&temp0, aSize);
	if (temp3)
	{
	    printf("expect NULL; lockfree_near_test SPHLockFreeHeapNearAlloc(%p, %zu) = %p\n",
		&temp0, aSize, temp3);
	    return 10;
	} else {
	    printf("Success lockfree_near_test SPHLockFreeHeapNearAlloc(%p,%zu)==NULL\n",
		&temp0, aSize);
	}

	aSize = 96;
	temp3 = (char*)SPHLockFreeHeapNearAlloc ((temp0+4096), aSize);
	if (temp3)
	{
	    printf("lockfree_near_test SPHLockFreeHeapNearAlloc(%p, %zu) = %p\n",
		(temp0+4096), aSize, temp3);
	    strcpy (temp2, "temp2");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize+96+128+192))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("error lockfree_near_test SPHLockFreeHeapNearAlloc(%p,%zu) failed\n",
		lfHeap, aSize);
	    return 10;
	}

	aSize = 96+128+96;
	status = SPHLockFreeHeapFreeNear (temp2);
	if (!status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNear(%p) = %d\n",
		temp2, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("Error lockfree_near_test  SPHLockFreeHeapFreeNear(%p) failed =%d\n",
		temp2, status);
	    return 10;
	}

	aSize = 96+128+96;
	status = SPHLockFreeHeapFreeNear (temp2);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNear(%p) = %d\n",
		temp2, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNear detected double free\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    temp2 = NULL;
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNear(%p) failed =%d\n",
		temp2, status);
	    return 10;
	}

	aSize = 128+96;
	status = SPHLockFreeHeapFreeNear (temp0);
	if (!status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNear(%p) = %d\n",
		temp0, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNear(%p) failed =%d\n",
		temp0, status);
	    return 10;
	}

	aSize = 128+96;
	status = SPHLockFreeHeapFreeNear (temp0);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNear(%p) = %d\n",
		temp0, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNear detected double free\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    temp0 = NULL;
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNear(%p) failed =%d\n",
		temp0, status);
	    return 10;
	}

	aSize = 128+96;
	status = SPHLockFreeHeapFreeNear (&temp0);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNear(%p) = %d\n",
		&temp0, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNear detected invalid free block\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    temp0 = NULL;
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNear(%p) failed =%d\n",
		&temp0, status);
	    return 10;
	}

	aSize = 128+96;
	status = SPHLockFreeHeapFreeNearChk (temp1, 196);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 196) = %d\n",
		temp1, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNearChk detected size error\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("Error lockfree_near_test(%p)  SPHLockFreeHeapFreeNearChk(%p, 196) failed =%d\n",
		lfHeap, temp1, status);
	    return 10;
	}

	aSize = 128+96;
	status = SPHLockFreeHeapFreeNearChk (temp1, 96);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 96) = %d\n",
		temp1, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNearChk detected size error\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("Error lockfree_near_test(%p)  SPHLockFreeHeapFreeNearChk(%p, 96) failed =%d\n",
		lfHeap, temp1, status);
	    return 10;
	}

	aSize = 96;
	status = SPHLockFreeHeapFreeNearChk (temp1, 128);
	if (!status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) = %d\n",
		temp1, status);
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) failed =%d\n",
		temp1, status);
	    return 10;
	}

	aSize = 96;
	status = SPHLockFreeHeapFreeNearChk (temp1, 128);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) = %d\n",
		temp1, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNearChk detected double free\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    temp1 = NULL;
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) failed =%d\n",
		temp1, status);
	    return 10;
	}

	aSize = 96;
	status = SPHLockFreeHeapFreeNearChk (&temp1, 128);
	if (status)
	{
	    printf("lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) = %d\n",
		&temp0, status);
	    printf("Success lockfree_near_test SPHLockFreeHeapFreeNearChk detected invalid free block\n");
	    lfTemp = SPHLockFreeHeapFreeSpace (lfHeap);
	    if (lfspace != (lfTemp+aSize))
	    {
	    	printf("error lockfree_near_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		lfHeap, lfspace, lfTemp, aSize);
		rc++;
	    }
	    temp1 = NULL;
	} else {
	    printf("Error lockfree_near_test SPHLockFreeHeapFreeNearChk(%p, 128) failed =%d\n",
		&temp0, status);
	    return 10;
	}

    } else {
	printf("error lockfree_near_test(%p)  SPHLockFreeHeapInit(%p,%x,4096,16) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP);
	return 10;
    }

    return rc;
}

int
lockfree_large_test (char *x4k)
{
    int rc = 0;
    SPHLockFreeHeap_t lfHeap;
    block_size_t lfspace, i;
    block_size_t aSize, hSize, uSize;
    unsigned long int align_mask;
    char *temp0;

#if 0
    /*  this test need full sphde environment with SPHLockFreeHeapCreate */
    hSize = 1536;
    uSize = 16;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
                hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap == (SPHLockFreeHeap_t)(-1L)))
    {
        printf("lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) heap too large\n",
                x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);

    } else {
        printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) = %p failed\n",
                x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize, lfHeap);
        return 10;
    }
#endif 
    hSize = 1024;
    uSize = 16;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
                hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
        lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
        
        printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
                x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
        {
        } else {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

    } else {
        printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
                x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
        return 10;
    }

    hSize = 8192;
    uSize = 16;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    hSize += hSize;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    hSize += hSize;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    hSize += hSize;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap == (SPHLockFreeHeap_t)(-1L)))
    {
	printf("lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) heap too large\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) = %p failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize, lfHeap);
	return 10;
    }

    memset (x4k, 0, hSize);
    uSize = 1024;

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    memset (x4k, 0, hSize);
    uSize = 2048;

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	printf("lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) unit too large for heap\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);

    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) = %p failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize, lfHeap);
	return 10;
    }

    memset (x4k, 0, hSize);
    uSize = 32;

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

        aSize = uSize;
	align_mask = (uSize - 1);
        for (i = 0; i < (lfspace / uSize); i++)
        {
                temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
                if (temp0)
                {
                        if ((i % 100) == 0)
                                printf("lockfree_large_test[%zu] SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
                                i, lfHeap, aSize, temp0);

			if (((unsigned long int)temp0 & align_mask) != 0UL) {
                                printf("lockfree_large_test[%zu] %p not aligned to unit size\n",
                                i, temp0);
                        	rc++;
			}
                } else {
                        printf("error lockfree_large_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
                                lfHeap, lfHeap, aSize);
                        rc++;
                }
        }

        if (!SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) false\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

        if (SPHLockFreeHeapFull (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) true\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) failed\n",
                lfHeap);
            rc++;
        }
    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    memset (x4k, 0, hSize);
    uSize = 256;

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

        aSize = uSize;
	align_mask = (uSize - 1);
        for (i = 0; i < (lfspace / uSize); i++)
        {
                temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
                if (temp0)
                {
                        if ((i % 100) == 0)
                                printf("lockfree_large_test[%zu] SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
                                i, lfHeap, aSize, temp0);

			if (((unsigned long int)temp0 & align_mask) != 0UL) {
                                printf("lockfree_large_test[%zu] %p not aligned to unit size\n",
                                i, temp0);
                        	rc++;
			}
                } else {
                        printf("error lockfree_large_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
                                lfHeap, lfHeap, aSize);
                        rc++;
                }
        }

        if (!SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) false\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

        if (SPHLockFreeHeapFull (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) true\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) failed\n",
                lfHeap);
            rc++;
        }
    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    memset (x4k, 0, hSize);
    uSize = 1024;

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_large_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
	} else {
	    printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

        aSize = uSize;
	align_mask = (uSize - 1);
        for (i = 0; i < (lfspace / uSize); i++)
        {
                temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
                if (temp0)
                {
                        if ((i % 10) == 0)
                                printf("lockfree_large_test[%zu] SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
                                i, lfHeap, aSize, temp0);

			if (((unsigned long int)temp0 & align_mask) != 0UL) {
                                printf("lockfree_large_test[%zu] %p not aligned to unit size\n",
                                i, temp0);
                        	rc++;
			}
                } else {
                        printf("error lockfree_large_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
                                lfHeap, lfHeap, aSize);
                        rc++;
                }
        }

        if (!SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) false\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

        if (SPHLockFreeHeapFull (lfHeap))
        {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) true\n",
                lfHeap);
        } else {
            printf("lockfree_large_test SPHLockFreeHeapFull(%p) failed\n",
                lfHeap);
            rc++;
        }
    } else {
	printf("error lockfree_large_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    return rc;
}

int
lockfree_cycle_test (char *x4k)
{
    int rc = 0;
    long int i, j;
    SPHLockFreeHeap_t lfHeap;
    block_size_t lfspace;
    block_size_t aSize, hSize, uSize;
    unsigned long int align_mask;
    char *temp0;

    hSize = (32*1024);
    uSize = 16;
    memset (x4k, 0, hSize);

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    if ((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_cycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
            printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
	} else {
	    printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}

	for (i = 0; i < alloc_cnt; i++)
	{
		alloc_track[i] = NULL;
	}
	j = 0;

	align_mask = (uSize - 1);
        for (i = 0; i < alloc_cnt; i++)
        {
		if (j >= prime_cnt)
			j = 0;

		aSize = prime_size[j++] * uSize;
                temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
                if (temp0)
                {
#if 0
                        if ((i % 10) == 0)
                                printf("lockfree_cycle_test[%zu] SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
                                i, lfHeap, aSize, temp0);
#endif
			if (((unsigned long int)temp0 & align_mask) != 0UL) {
                                printf("lockfree_cycle_test[%ld] %p not aligned to unit size\n",
                                i, temp0);
                        	rc++;
			}

			alloc_track[i] = temp0;
                } else {
                        printf("error lockfree_cycle_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
                                lfHeap, lfHeap, aSize);
                        rc++;
                }

        }

	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_cycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (!SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) false\n",
                lfHeap);
        } else {
            printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

	j = 0;
        for (i = 0; i < alloc_cnt; i++)
        {
		if (j >= prime_cnt)
			j = 0;

		aSize = prime_size[j++] * uSize;
		temp0 = (char*)alloc_track[i];
                if(SPHLockFreeHeapFreeNearChk (temp0, aSize))
                {
                        printf("error lockfree_cycle_test(%p)  SPHLockFreeHeapFreeNearChk(%p,%zu) failed\n",
                                lfHeap, temp0, aSize);
                        rc++;
                } else {
#if 0
                        if ((i % 10) == 0)
                                printf("lockfree_cycle_test[%zu] SPHLockFreeHeapFreeNearChk(%p, %zu) = %p\n",
                                i, temp0, aSize, temp0);
#endif
		}

        }

	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_cycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
        } else {
            printf("lockfree_cycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }
    } else {
	printf("error lockfree_cycle_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    return rc;
}

int
lockfree_recycle_test (char *x4k, char *y4k)
{
    int rc = 0;
    long int i, j, j2, k;
    SPHLockFreeHeap_t lfHeap, lf2Heap;
    block_size_t lfspace;
    block_size_t aSize, hSize, uSize;
    unsigned long int align_mask;
    char *temp0, *temp1;

    hSize = (32*1024);
    uSize = 16;
    memset (x4k, 0, hSize);
    memset (y4k, 0, (64*1024));

    lfHeap = SPHLockFreeHeapInit (x4k , SAS_RUNTIME_LOCKFREEHEAP, 
		hSize, uSize);
    lf2Heap = SPHLockFreeHeapInit (y4k , SAS_RUNTIME_LOCKFREEHEAP, 
		(64*1024), 32);
    if (((lfHeap != NULL) && (lfHeap != (SPHLockFreeHeap_t)(-1L)))
    &&  ((lf2Heap != NULL) && (lf2Heap != (SPHLockFreeHeap_t)(-1L))))
    {
	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
	{
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
	} else {
	    printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
		lfHeap);
	    rc++;
	}
	lfspace = SPHLockFreeHeapFreeSpace (lf2Heap);
	
	printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		y4k, lf2Heap, lfspace);

        if (SPHLockFreeHeapEmpty (lf2Heap))
	{
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lf2Heap);
	} else {
	    printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
		lf2Heap);
	    rc++;
	}

	for (i = 0; i < alloc_cnt; i++)
	{
		alloc_track[i] = NULL;
	}
	j = 0; j2 = 0;
	i = 0;

	align_mask = (uSize - 1);
        for (k = 0; k < (alloc_cnt * 49); k++)
        {
		if (j >= prime_cnt)
			j = 0;
		if (i >= alloc_cnt)
			i = 0;

		temp0 = (char*)alloc_track[i];
		if (k >= alloc_cnt) {
			if (j2 >= prime_cnt)
				j2 = 0;

			aSize = prime_size[j2++] * uSize;
			if (temp0) {
				if(SPHLockFreeHeapFreeNearChk (temp0, aSize))
				{
					printf("error lockfree_recycle_test(%p)  SPHLockFreeHeapFreeNearChk(%p,%zu) failed\n",
						lfHeap, temp0, aSize);
					printf("lockfree_recycle_test (i,j,j2,k)  %ld,%ld,%ld,%ld\n",
						i,j,j2,k);
					rc++;
				} else {
					alloc_track[i] = NULL;
#if 0
					if ((i % 10) == 0)
						printf("lockfree_recycle_test[%zu] SPHLockFreeHeapFreeNearChk(%p, %zu) = %p\n",
						i, temp0, aSize, temp0);
#endif
				}
			}
		}
		aSize = prime_size[j++] * uSize;
                temp0 = (char*)SPHLockFreeHeapAlloc (lfHeap, aSize);
                if (temp0)
                {
#if 0
                        if ((i % 10) == 0)
                                printf("lockfree_recycle_test[%zu] SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
                                i, lfHeap, aSize, temp0);
#endif
			if (((unsigned long int)temp0 & align_mask) != 0UL) {
                                printf("lockfree_recycle_test[%ld] %p not aligned to unit size\n",
                                i, temp0);
                        	rc++;
			}

			alloc_track[i] = temp0;
                } else {
                        printf("warn lockfree_recycle_test(%p)  SPHLockFreeHeapAlloc(%p,%zu) failed\n",
                                lfHeap, lfHeap, aSize);
#if 0
			printf("lockfree_recycle_test (i,j,j2,k)  %zu,%zu,%zu,%zu\n",
					i,j,j2,k);
#endif
			lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
			
			printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
				x4k, lfHeap, lfspace);
	
			temp1 = (char*)SPHLockFreeHeapAlloc (lf2Heap, aSize);
			if (temp1)
			{
	#if 1
				printf("lockfree_recycle_test[%ld] 2nd SPHLockFreeHeapAlloc(%p, %zu) = %p\n",
				i, lf2Heap, aSize, temp1);
	#endif
				if (((unsigned long int)temp1 & align_mask) != 0UL) {
					printf("lockfree_recycle_test[%ld] %p not aligned to unit size\n",
					i, temp1);
					rc++;
				}
	
				alloc_track[i] = temp1;
			} else {
				printf("lockfree_recycle_test(%p) 2nd SPHLockFreeHeapAlloc(%p,%zu) failed\n",
					lf2Heap, lfHeap, aSize);
#if 0
				printf("lockfree_recycle_test (i,j,j2,k)  %zu,%zu,%zu,%zu\n",
						i,j,j2,k);
#endif
				lfspace = SPHLockFreeHeapFreeSpace (lf2Heap);
				
				printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
					y4k, lf2Heap, lfspace);
				rc++;
			}
                }
		i++;
        }

	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_recycle_test(%p) mid SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (!SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) false\n",
                lfHeap);
        } else {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

	lfspace = SPHLockFreeHeapFreeSpace (lf2Heap);
	
	printf("lockfree_recycle_test(%p) mid SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		y4k, lf2Heap, lfspace);
#if 0
        if (!SPHLockFreeHeapEmpty (lf2Heap))
        {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) false\n",
                lf2Heap);
        } else {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lf2Heap);
            rc++;
        }
#endif
	j = j2;
        for (i = 0; i < alloc_cnt; i++)
        {
		if (j >= prime_cnt)
			j = 0;

		aSize = prime_size[j++] * uSize;
		temp0 = (char*)alloc_track[i];
		if (temp0) {
			if(SPHLockFreeHeapFreeNearChk (temp0, aSize))
			{
				printf("error lockfree_recycle_test(%p)  SPHLockFreeHeapFreeNearChk(%p,%zu) failed\n",
					lfHeap, temp0, aSize);
				rc++;
			} else {
#if 0
				if ((i % 10) == 0)
					printf("lockfree_recycle_test[%zu] SPHLockFreeHeapFreeNearChk(%p, %zu) = %p\n",
					i, temp0, aSize, temp0);
#endif
			}
		}
        }

	lfspace = SPHLockFreeHeapFreeSpace (lfHeap);
	
	printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		x4k, lfHeap, lfspace);

        if (SPHLockFreeHeapEmpty (lfHeap))
        {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lfHeap);
        } else {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lfHeap);
            rc++;
        }

	lfspace = SPHLockFreeHeapFreeSpace (lf2Heap);
	
	printf("lockfree_recycle_test(%p)  SPHLockFreeHeapFreeSpace(%p) = %zu\n",
		y4k, lf2Heap, lfspace);

        if (SPHLockFreeHeapEmpty (lf2Heap))
        {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) true\n",
                lf2Heap);
        } else {
            printf("lockfree_recycle_test SPHLockFreeHeapEmpty(%p) failed\n",
                lf2Heap);
            rc++;
        }
    } else {
	printf("error lockfree_recycle_test(%p)  SPHLockFreeHeapInit(%p,%x,%zu,%zu) failed\n",
		x4k, x4k, SAS_RUNTIME_LOCKFREEHEAP, hSize, uSize);
	return 10;
    }

    return rc;
}

/* Fake out the SAS region range checking for these tests.
*/

void
setmemrange (unsigned long low, unsigned long high)
{
    unsigned long	ak, bk;

    setSASmemrange (low, high);

    ak = getMemLow();
    bk = getMemHigh();
    printf ("getMemLow()=%lu, getMemHigh()=%lu\n",
            ak, bk);
}

#define stack_block_size 65536 
#define stack_block_mask (stack_block_size - 1)
#define stack_buff_size (stack_block_size + stack_block_size)

int main(int argc, char *argv[])
{
    int rc = 0;
    unsigned long	a4k, b4k, c4k;
    char        a[stack_buff_size], b[stack_buff_size];
    char *source_address, *dest_address;

    memset (a, 0, stack_buff_size);
    memset (b, 0, stack_buff_size);
#if 0
    a4k = (unsigned long)(a+stack_block_mask) & stack_block_mask;
    b4k = (unsigned long)(b+stack_block_mask) & stack_block_mask;
    a4k = (stack_block_mask - a4k) & stack_block_mask;
    b4k = (stack_block_mask - b4k) & stack_block_mask;

    source_address=a+a4k;
    dest_address=b+b4k;
#else
    a4k = (unsigned long)(a+stack_block_mask) & ~stack_block_mask;
    b4k = (unsigned long)(b+stack_block_mask) & ~stack_block_mask;
    if (a4k > b4k)
    {
	c4k = a4k;
	a4k = b4k;
	b4k = c4k;
    }

    source_address=(char*)a4k;
    dest_address=(char*)b4k;
#endif
    printf ("source_address=%p, dest_address=%p\n",
            source_address, dest_address);
#if 1
    rc += lockfree_basic_test(source_address);
#endif
#if 1
    rc += lockfree_align_test(source_address);
#endif
#if 1
    setmemrange (a4k, b4k+stack_block_size);

    rc += lockfree_near_test (source_address);
#endif
#if 1
    setmemrange (a4k, b4k+stack_block_size);
    rc += lockfree_large_test (source_address);
#endif
#if 0
    setmemrange (a4k, b4k+stack_block_size);
    rc += lockfree_cycle_test (source_address);
#endif
#if 0
    setmemrange (a4k, b4k+stack_block_size);
    rc += lockfree_recycle_test (source_address, dest_address);
#endif
    return rc;
}
