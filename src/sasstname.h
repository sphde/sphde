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

#ifndef __SAS_STORE_NAME_H
#define __SAS_STORE_NAME_H

#include "sassim.h"

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

extern __C__ char *sasStorePath;

extern __C__ char *SASSegNameIndexed(char *name, sasseg_t segnum);

extern __C__ int SASSegNameExists(char *name);

extern __C__ int SASSegIndexExists (sasseg_t segnum);

extern __C__ int SASSegStoreCreateByName (char *name);

extern __C__ int SASSegStoreCreate (sasseg_t segnum);

extern __C__ int SASSegStoreRemoveByName (char *name);

extern __C__ int SASSegStoreRemove (sasseg_t segnum);

#endif  /* _SAS_STORE_NAME_H */
