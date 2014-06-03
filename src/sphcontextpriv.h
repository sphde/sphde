
/*
 * Copyright (c) 2012-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * sphcontextpriv.h
 * Created on: Apr 21, 2012
 *      Author: sjmunroe
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef SPHCONTEXTPRIV_H_
#define SPHCONTEXTPRIV_H_

#include <sphcontext.h>

/** \brief internal use only
*
*/
extern __C__ void *
SPHContextAlloc (SPHContext_t heap, block_size_t alloc_size);

/** \brief internal use only
*
*/
extern __C__ int
SPHContextFree (SPHContext_t heap,
		void *free_block,
		block_size_t alloc_size);

/** \brief internal use only
*
*/
extern __C__ void*
SPHContextNearAlloc(void *nearObj, long allocSize);

/** \brief internal use only
*
*/
extern __C__ void
SPHContextNearDealloc(void *memAddr, long allocSize);

#endif /* SPHCONTEXTPRIV_H_ */
