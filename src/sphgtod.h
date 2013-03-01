#ifndef __SPH_GTOD_H_
#define __SPH_GTOD_H_

/*
 * Copyright (c) 2012 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Ryan S Arnold - New API.
 */

#include <sys/time.h>

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/*!
 * \brief Return the timebase converted to gettimeofday struct timeval.
 *
 * Returns a fast emulation of the values returned by gettimeofday computed
 * from a queried machine timebase register value.
 *
 * @return The timebase converted to gettimeofday struct timeval.
 */

extern __C__ int
sphgtod (struct timeval *tv, struct timezone *tz);

#endif /* __SPH_GTOD_H */
