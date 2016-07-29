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
	void		*reserved1;
# endif
} SASAnchor_t;

#endif

