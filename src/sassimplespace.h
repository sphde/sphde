/*
 * Copyright (c) 2005-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SAS_SIMPLE_SPACE_H
#define __SAS_SIMPLE_SPACE_H

#include "sastype.h"

/**! \file sassimplespace.h
*  \brief Shared Address Space Simple Space.
*   Allocate a SAS block as one contiguous space.
*
**/

/** \brief Handle to SAS Simple Space.
*
*	The type is SAS_RUNTIME_SIMPLESPACE
*/
typedef void *SASSimpleSpace_t; 

/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Initialize a shared storage block as a simple space.
*
*	Initialize the control blocks within the specified storage
*	block as a Simple Space.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_SIMPLESPACE.
*
*	@param heap_block a block of allocated SAS storage.
*	@param block_size power of two size of the heap to be initialized.
*	@param space_size size of the simple space within the block.
*	@return a handle to the initialized SASSimpleSpace_t
*/
extern __C__ SASSimpleSpace_t 
SASSimpleSpaceInit (void *heap_block, block_size_t block_size, block_size_t space_size);

/** \brief Allocate a SAS block large enough to contain the requested
*   SAS Simple Space.
*
*	Initialize the control blocks within the specified storage
*	block as a Simple Space.
*	The storage block must be power of two in size.
*	The type should be SAS_RUNTIME_SIMPLESPACE.
*
*	@param space_size size of the simple space within the block.
*	@return a handle to the created SASSimpleSpace_t.
*/
extern __C__ SASSimpleSpace_t 
SASSimpleSpaceCreate (block_size_t space_size);

/** \brief Obtain the starting byte address of the simple space within
*   the SASSimpleSpace_t block.
*
*	Each SASSimpleSpace_t starts with a control block.
*	The Simple Space data starts after this control block.
*
*	@param space Handle of a SAS Simple Space.
*	@return address of contained simple space data.
*/
extern __C__ void* 
SASSimpleSpaceToAddr (SASSimpleSpace_t space);

/** \brief Obtain the SASSimpleSpace_t handle from a contained space address.
*
*	Find the associated SASSimpleSpace_t control block and return its
*	address as a SASSimpleSpace_t handle.
*
*	@param space Address within the range of the contained simple space.
*	@return a handle to the created SASSimpleSpace_t.
*/
extern __C__ SASSimpleSpace_t 
SASSimpleSpaceFromAddr (void *space);

/** \brief Destroy a SASSimpleSpace_t and free the shared storage block.
*
*	The sas_type_t must be SAS_RUNTIME_SIMPLESPACE.
*	Destroy holds an exclusive write while clearing the control blocks
*	and freeing the SAS block.
*
*	@param space handle of the SASSimpleSpace_t to be destroyed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int 
SASSimpleSpaceDestroy (SASSimpleSpace_t space);

/** \brief Return the block free space not occupied by control blocks
*   and the simple space.
*
*	@param space Handle of a SAS Simple Space.
*	@return the size of the remaining free space.
*/
extern __C__ block_size_t 
SASSimpleSpaceFreeSpace (SASSimpleSpace_t space);

/** \brief Destroy a SASSimpleSpace_t and free the shared storage block.
*
*	The sas_type_t must be SAS_RUNTIME_SIMPLESPACE.
*
*	@param space handle of the SASSimpleSpace_t to be destroyed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int 
SASSimpleSpaceDestroyNoLock (SASSimpleSpace_t space);

#endif /* __SAS_SIMPLE_SPACE_H */
