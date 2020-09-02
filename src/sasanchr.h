/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SASANCHOR__H
#define __SASANCHOR__H
//TODO SASSIM is now the default and only supported mode
#define __SASSIM__ 1

# ifdef __SASSIM__
#include "ultree.h"
#include <sys/mman.h>
#include <semaphore.h>
# endif


typedef struct
{
#ifdef __WORDSIZE_64
#if defined (__x86_64__) || \
    (defined (__LITTLE_ENDIAN__) && defined (__powerpc64__)) \
    || defined (__aarch64__) || defined (__arm__) \
    || ((__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) && defined(__mips64))
  unsigned int compactUseList:1;
  unsigned long reserved0:63;
#else
  unsigned long reserved0:63;
  unsigned int compactUseList:1;
#endif
#else
#if __BYTE_ORDER == __ORDER_LITTLE_ENDIAN__
  unsigned int compactUseList:1;
  unsigned int reserved0:31;
#else
  unsigned int reserved0:31;
  unsigned int compactUseList:1;
#endif
#endif
} regionFlags;

typedef struct {
	unsigned long	regionSize;
	void		*finder;
# ifdef __SASSIM__
	uLongTreeNode	*uncommitted;
	uLongTreeNode	*free;
	uLongTreeNode	*used;
	uLongTreeNode	*region;
	uLongTreeNode	*allocated;
#  ifdef __GNUC__
	sem_t           SASSem;
#  else
	msemaphore	SASSem;
#  endif
	regionFlags	rFlags;
# endif
} SASAnchor_t;

#endif

