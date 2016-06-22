/*
 * Copyright (c) 2003-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "sasio.h"
#include "sasmsync.h"

static inline void*
pageAlignStart (void *startAddr)
{
	unsigned long pageSize = getpagesize();
	unsigned long pageRnd  = pageSize - 1;
	unsigned long pageMask = ~pageRnd;
	unsigned long addr = (unsigned long)startAddr;
	return (void*)(addr & pageMask);
}

static inline size_t
pageAlignLen (void *startAddr, size_t size)
{
	unsigned long pageSize = getpagesize();
	unsigned long pageRnd  = pageSize - 1;
	unsigned long pageMask = ~pageRnd;
	unsigned long addr = (unsigned long)startAddr;
	unsigned long endAddr = (addr + size + pageRnd)
				& pageMask;
	addr &= pageMask;
	return (size_t)(endAddr - addr);
}


int  sasMsyncRelease(void *startAddr, size_t size)
{
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);
	return madvise(alignAddr, alignSize, MADV_DONTNEED);
}


int  sasMsyncPurge(void *startAddr, size_t size, int asyncBool)
{
	int rc;
	int flags;
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);
	if (asyncBool)
		flags = (MS_ASYNC | MS_INVALIDATE);
	else
		flags = (MS_SYNC | MS_INVALIDATE);
		
	rc = msync(alignAddr, alignSize, flags);
	if (rc)
	  return rc;

	return madvise(alignAddr, alignSize, MADV_DONTNEED);
};


int  sasMsyncBring(void *startAddr, size_t size)
{
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);
		
	return madvise(alignAddr, alignSize, MADV_WILLNEED);
}


int  sasMsyncSequential(void *startAddr, size_t size)
{
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);
		
	return madvise(alignAddr, alignSize, MADV_SEQUENTIAL);
}


int  sasMsyncRandom(void *startAddr, size_t size)
{
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);
		
	return madvise(alignAddr, alignSize, MADV_RANDOM);
}


int  sasMsyncWrite(void *startAddr, size_t size, int asyncBool)
{
	int flags;
	void 	*alignAddr = pageAlignStart(startAddr);
	size_t	alignSize = pageAlignLen (startAddr, size);

	if (asyncBool)
		flags = (MS_ASYNC);
	else
		flags = (MS_SYNC);
		
	return msync(alignAddr, alignSize, flags);
}
