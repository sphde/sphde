/*
 * Copyright (c) 2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe     - initial API and implementation
 */

#ifndef _SASAllocPriv_H
#define _SASAllocPriv_H

#include "sasalloc.h"

extern unsigned long memLow __attribute__ ((visibility ("hidden")));
extern unsigned long memHigh __attribute__ ((visibility ("hidden")));

static inline unsigned long
getfastMemLow ()
{
  return memLow;
}

static inline unsigned long
getfastMemHigh ()
{
  return memHigh;
}

#endif /* _SASAllocPriv_H */
