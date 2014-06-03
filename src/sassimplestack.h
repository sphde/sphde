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

#ifndef __SAS_SIMPLE_STACK_H
#define __SAS_SIMPLE_STACK_H

#include "sastype.h"


/**! \file sassimplestack.h
*  \brief Shared Address Space Simple Stack.
*  Allocate a SAS block as stack.
*
**/

/** \brief Handle to SAS Simple Stack.
*
*	The type is SAS_RUNTIME_SIMPLESTACK
*/
typedef void *SASSimpleStack_t; 

/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Initialize a shared storage block as a simple stack.
*
*	Initialize the control blocks within the specified storage
*	block as a Simple Stack.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_SIMPLESTACK.
*
*	@param heap_block a block of allocated SAS storage.
*	@param block_size power of two size of the heap to be initialized.
*	@return a handle to the initialized SASSimpleStack_t
*/
extern __C__ SASSimpleStack_t 
SASSimpleStackInit (void *heap_block, block_size_t block_size);

/** \brief Allocate a SAS block  as a simple stack.
*
*	Initialize the control blocks within the specified storage
*	block as a Simple Stack.
*	The storage block must be power of two in size.
*	The type should be SAS_RUNTIME_SIMPLESPACE.
*
*	@param stack_size size of the simple space within the block.
*	@return a handle to the created SASSimpleStack_t.
*/
extern __C__ SASSimpleStack_t 
SASSimpleStackCreate (block_size_t stack_size);

/** \brief Destroy a SASSimpleStack_t and free the shared storage block.
*
*	The sas_type_t must be SAS_RUNTIME_SIMPLESTACK.
*	Destroy holds an exclusive write while clearing the control blocks
*	and freeing the SAS block.
*
*	@param stack handle of the SASSimpleSpace_t to be destroyed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int 
SASSimpleStackDestroy (SASSimpleStack_t stack);

/** \brief Return the block free space not currently allocated to the stack.
*
*	@param stack Handle of a SAS Simple Stack.
*	@return the size of the remaining free space.
*/
extern __C__ block_size_t 
SASSimpleStackFreeSpace (SASSimpleStack_t stack);

/** \brief Atomically allocate a sub range of a SAS Simple Stack
*   as a push down stack.
*
*	@param stack Handle of a SAS Simple Stack.
*	@param alloc_size size of the simple space within the block.
*	@return Address of the allocated space or NULL if the stack is full.
*/
extern __C__ void *
SASSimpleStackAlloc (SASSimpleStack_t stack, block_size_t alloc_size);

/** \brief Atomically allocate a sub range of a SAS Simple Stack
*   as a push down stack.
*
*	Find the associated SASSimpleStack_t control block and use it to
*	allocate stack space.
*
*	@param near_stack Handle of a SAS Simple Stack.
*	@param alloc_size size of the simple space within the block.
*	@return Address of the allocated space or NULL if the stack is full.
*/
extern __C__ void *
SASSimpleStackNearAlloc (void *near_stack, block_size_t alloc_size);

/** \brief Atomically deallocate a sub range of a SAS Simple Stack
*   by reseting the stack top.
*
*	@param stack Handle of a SAS Simple Stack.
*	@param stack_pointer The new stack top.
*	@return  a 0 value indicates success, otherwise failure.
*/
extern __C__ int
SASSimpleStackDealloc (SASSimpleStack_t stack, void* stack_pointer);

/** \brief Destroy a SASSimpleStack_t and free the shared storage block.
*
*	The sas_type_t must be SAS_RUNTIME_SIMPLESTACK.
*
*	@param stack handle of the SASSimpleSpace_t to be destroyed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int 
SASSimpleStackDestroyNoLock (SASSimpleStack_t stack);

/** \brief Return the block free space not currently allocated to the stack.
*
*	@param stack Handle of a SAS Simple Stack.
*	@return the size of the remaining free space.
*/
extern __C__ block_size_t 
SASSimpleStackFreeSpaceNoLock (SASSimpleStack_t stack);

/** \brief Allocate a sub range of a SAS Simple Stack as s push down stack.
*
*	@param stack Handle of a SAS Simple Stack.
*	@param alloc_size size of the simple space within the block.
*	@return Address of the allocated space or NULL if the stack is full.
*/
extern __C__ void *
SASSimpleStackAllocNoLock (SASSimpleStack_t stack, block_size_t alloc_size);

/** \brief Allocate a sub range of a SAS Simple Stack as a push down stack.
*
*	Find the associated SASSimpleStack_t control block and use it to
*	allocate stack space.
*
*	@param near_stack Handle of a SAS Simple Stack.
*	@param alloc_size size of the simple space within the block.
*	@return Address of the allocated space or NULL if the stack is full.
*/
extern __C__ void *
SASSimpleStackNearAllocNoLock (void *near_stack, block_size_t alloc_size);

/** \brief Deallocate a sub range of a SAS Simple Stack
*   by reseting the stack top.
*
*	@param stack Handle of a SAS Simple Stack.
*	@param stack_pointer The new stack top.
*	@return  a 0 value indicates success, otherwise failure.
*/
extern __C__ int
SASSimpleStackDeallocNoLock (SASSimpleStack_t stack, void* stack_pointer);

#endif /* __SAS_SIMPLE_STACK_H */
