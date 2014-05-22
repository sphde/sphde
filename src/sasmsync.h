/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SAS_MSYNC_H
#define __SAS_MSYNC_H

/**! \file sasmsync.h
*  \brief API to manage the resources of the Shared Address Space.
*
*  Manage the real memory and persistent state of the backing files
*  associated with the SAS Region.
*
**/

/** \brief SAS msync asynchronous option.  **/
#define SAS_ASYNC 1
/** \brief SAS msync synchronous option.  **/
#define SAS_SYNC 0

/** \brief ignore this macro behind the curtain **/
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Write a range of pages to persistent storage and inform the
*   kernel that those pages can be removed from real memory.
*
*   Write (msync) any changed pages in range then mark all pages in range
*   MADV_DONTNEED.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@param asyncBool Flag for asynchronous action if true.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncPurge(void *startAddr, size_t size, int asyncBool);

/** \brief Inform the kernel that those pages can be removed from
*   real memory.
*
*   Mark all pages in range MADV_DONTNEED.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncRelease(void *startAddr, size_t size);

/** \brief Inform the kernel that those pages will be needed soon.
*
*   Mark all pages in range MADV_WILLNEED.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncBring(void *startAddr, size_t size);

/** \brief Inform the kernel that those pages will be needed soon
*   and will be accessed in sequential order.
*
*   Mark all pages in range MADV_SEQUENTIAL.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncSequential(void *startAddr, size_t size);

/** \brief Inform the kernel that those pages will be needed soon
*   and will be accessed in random order.
*
*   Mark all pages in range MADV_RANDOM.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncRandom(void *startAddr, size_t size);

/** \brief Write a range of pages to persistent storage.
*
*   Write (msync) any changed pages in range.
*
*	@param startAddr starting address of the purge range.
*	@param size of the purge range.
*	@param asyncBool Flag for asynchronous action if true.
*	@return Zero on success and ERRNO otherwise.
*/
extern __C__ int sasMsyncWrite(void *startAddr, size_t size, int asyncBool);

#endif

