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

#ifndef __SAS_SIM_H
#define __SAS_SIM_H

/**! \file sassim.h
*  \brief Shared Address Space, base runtime
*	for shared and persistent memory for multi-thread/multi-core applications.
*
*   Provides APIs to manage (setup and take-down) the SAS Region and
*   backing store for cooperating processes. The region is a contiguous
*   subrange of the process address space. The exact range varies based
*   on the platform on the platform (as defined in sasconf.h).
*
*   Associated with an active SAS region is SAS Store which is a
*   directory contain backing files which are memory mapped for the
*   active portions of the region.
*   A region is fully qualified by its SAS Store (directory)
*   path. Processes joined to the same SAS story are accessing the
*   same data at the same virtual address.
*
*   \note: For Linux this means memory mapping the pages from the
*   kernel's file cache, and so we are sharing the same real pages
*   across all partipating processes.
*   This enables zero copy sharing and persistence.
*
*   Processes using different SAS Store paths are accessing
*   logically different and disjoint regions. A process can only access
*   one SAS region but there is no limit (other than files system
*   capacity) to the number different SAS regions accessed by
*   different groups of cooperating processes.
*
*   The SAS region address range and backing file store are managed
*   via a power of 2 buddy system. Logically the region is divided into
*   segments and blocks. Segment are a fixed power of 2 size (defined
*   for the platform by sasconf.h) and alignment within the region.
*   Blocks are a variable power of 2 size and have an alignment
*   matching their size or better. Logically blocks are sub-allocated
*   from allocated segments (have a backing file and can be mmaped)
*   which are sub-allocated from the region.
*
*   \todo The current runtime restricts blocks to segment size or
*   smaller. Creating larger blocks would require creating (power of 2) multiple
*   segments (and backing files which have to memory mapped) while
*   holding the global anchor block lock. Future releases may remove
*   this restriction.
*
*   \note Segments and blocks inherit their virtual address (within the
*   process) from the region. The region has a fixed starting address
*   and segments always have a unique offset (and file index) within the
*   region. The segment's file index is used to generate the unique file
*   name within the SAS store directory. Obviously blocks inherit their
*   virtual address from the containing segment which inherits from the
*   region.
*
*   The segment is the granule of backing file allocation and memory
*   mapping. Segment backing files are always extended to the full
*   segment size at creation.  This minimizes OS overhead by minimizing
*   file system and memory mapping operations at runtime. Segments are
*   not allocated (and backing files created) until additional block
*   space is needed (when free block space, of the required size and
*   alignment, in currently allocated segments, is depleted).
*   This is done implicitly under the block allocate call.
*
*   \note SASJoinRegion and related functions (SASTHreadSetUp)
*   establish a sigaction handler for signal SIGSEGV.
*   This handles the case when one process,
*   sharing a named Region allocates a new segment and backing.  Other
*   processes (sharing the same Region) are not immediately aware of
*   this and need not perform any specific actions until or unless that
*   process references data in that segment.  This will generate a
*   SIGSEGV and the SAS sigaction handler will attach this segment by
*   mmapping the associated backing file into the faulting process at
*   the assigned address.
*
*   \note Applications that set their own signal to sigaction handlers
*   for SIGSEGV should be aware of this and plan for proper nesting.
*   Applications should establish their SIGSEGV handlers before
*   joining.  The SASJoinRegion will preserve the the "old action" as
*   part of setting it own SIGSEGV handler.
*
*   \note When the SPHDE runtime handler gets control it will verify
*   that the faulting address is within the SAS Region. If it is not,
*   it is likely a error or something that the application should
*   handle itself. The SPHDE runtime handler  will check if the
*   "old action" was SIG_DFL or has it own signal or sigaction handler.
*   If the application defined it own handler SPHDE will attempt to
*   call that handler.  Otherwise SPHDE runtime handler will print a
*   back trace and exit the effected process (or thread).
*
*   The runtime also sets up shared memory segments (shmat) for SAS locks.
*   SAS locks need to be shared but should not persist across reboot
*   (like memory mapped files do). Different SAS stores must have
*   independent locks (different lock segments).
*   So lock segments are allocated/attached via shmget/shmat using a
*   key generated (ftok) from the SAS Store path.
**/

#include <stdlib.h>

/** \brief SAS segment ID.
*
*	Internal typedef that identifies a segment (process address range
*	and backing file name) within the SAS Store (directory).
*/
typedef unsigned long sasseg_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief SAS clear on block deallocate flag.
*
*	Internal flag that requests the SAS runtime to clear blocks as they
*	are deallocated.
*/
extern __C__ int sasClearOnDealloc;

/** \brief Get the Region's lowest memory address.
*
*	With getMemHigh() defines the Region (starting process address and extent).
*
*	@return the address value for the lowest extent of the SAS Region.
*/
extern __C__ unsigned long getMemLow();

/** \brief Get the Region's highest memory address.
*
*	With getMemLow() defines the Region (starting process address and extent).
*
*	@return the address value for the highest extent (+1) of the SAS Region.
*/
extern __C__ unsigned long getMemHigh();

/** \brief Set the Region's low/high memory address for testing
*
*   @param low defines the regions starting address.
*   @param high defines the regions ending address.
*/
extern __C__ void
setSASmemrange (unsigned long low, unsigned long high);

/** \brief Join this process to a SAS Region.
*
*   Join this process to the SAS Region based on the anchor segment
*   in the SAS Store (either the default '.' directory or the path
*   specified in the environment variable SASSTOREPATH).
*   The anchor segment contains the anchor block including the indexes
*   that manage segments and (free and allocated) block space within the region.
*
*   If the backing file for the anchor segment does not exist in the SAS
*   Store directory, the runtime creates a "SegmentSize" backing file
*   and mmaps that file at SAS memLow. The lowest part of this segment
*   is initialed as the SAS anchor block.
*
*   Otherwise the the runtime mmaps the anchor segment at memLow and
*   proceeds to bring up the rest of the SAS environment.
*   This includes mmaping additional segments from the SAS Store.
*
*   The join function also checks for, and if needed initializes,
*   the matching lock segments.
*   The lock segments are allocated/attached via shmget/shmat using a
*   key generated from the SAS Store path. This insures that applications
*   using different SAS stores also have independent lock segments.
*
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int SASJoinRegion();

/** \brief Join this process to a named SAS Region.
*
*   Join this process to the SAS Region based on the anchor segment
*   backing file in the named SAS Store.
*   The anchor segment contains the anchor block including the indexes
*   that manage segments and (free and allocated) block space within the region.
*
*   If the backing file for the anchor segment does not exist in the SAS
*   Store directory, the runtime creates a "SegmentSize" backing file
*   and mmaps that file at SAS memLow. The lowest part of this segment
*   is initialed as the SAS anchor block.
*
*   Otherwise the the runtime mmaps the anchor segment at memLow and
*   proceeds to bring up the rest of the SAS environment.
*   This includes mmaping additional segments from the SAS Store.
*
*   The join function also checks for, and if needed initializes,
*   the matching lock segments.
*   The lock segments are allocated/attached via shmat using a key generated
*   from the SAS Store path. This insures that applications using different
*   SAS stores also have independent lock segments.
*
*   Finally the runtime sets up as sigsegv handler. This allows the SAS
*   runtime to automatically attach segments created by other processes
*   in this SAS store.
*
*   @param store_name C string containing the path to this SAS store directory.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int SASJoinRegionByName (const char * store_name);

/** \brief Allocate a block of memory within SAS Storage.
*
*	Blocks are allocated within the SAS region.
*	Blocks must be a power of 2 in size (256 bytes >= and <= segmentSize).
*	Allocated blocks always have alignment equal to their size or better.
*
*	If the required space is not found within a segment backed by existing files
*	in the SAS store, a new segment backing file is created in the SAS store
*	directory and mmaped into the region and added to region free space.
*   This automatically extends the SAS Store as needed to satisfy block
*   allocate requests.
*
*   @param blockSize size of the block to be allocated.
*	@return the address of the start of the allocated block,
*	or NULL if the allocation fails.
*/
extern __C__ void *SASBlockAlloc (unsigned long blockSize);

/** \brief Deallocate a block of memory within SAS Storage.
*
*	Blocks within the SAS Region are freed by adding the block range
*	to the free list. Adjacent free blocks of equal size and
*	appropriate alignment are combined into larger free blocks.
*
*   @param blockAddr address of start of the block to be deallocated.
*   @param blockSize size of the block to be deallocated.
*/
extern __C__ void SASBlockDealloc (void *blockAddr, unsigned long blockSize);

/** \brief Return lists of currently free segment block addresses and sizes.
*
*	Returns the addresses and sizes of power of 2 sized segment blocks
*	in the regions free list.
*	It is possible for the free list to cover combined segment blocks of
*	(power of 2) multiple segments.
*	Blocks on the regions free list will not have backing files in the SAS store directory.
*
*   @param blockAddr address of an array or addresses to be filled in.
*   @param blockSize address of an array of block size values to be filled in.
*   @param count address of a int to be filled in with the actual count of entries.
*
*   \todo should change this API to pass the max array size as a parm and return the actual count.
*/
extern __C__ void SASListFreeRegion (	void **blockAddr, 
                                            unsigned long *blockSize,
                                            int *count);

/** \brief Return lists of currently allocated segment block addresses and sizes.
*
*	Returns the addresses and sizes of power of 2 sized segment blocks currently
*	allocated within the region.
*	It is possible for the allocated list to cover combined segment blocks of
*	(power of 2) multiple segments.
*	Blocks on the regions allocated list always represent
*	segments with a backing file in the SAS Store directory.
*
*   @param blockAddr address of an array or addresses to be filled in.
*   @param blockSize address of an array of block size values to be filled in.
*   @param count address of a int to be filled in with the actual count of entries.
*
*   \todo should change this API to pass the max array size as a parm and return the actual count.
*/
extern __C__ void SASListAllocatedRegion (void **blockAddr, 
                                                   unsigned long *blockSize, 
                                                   int *count);

/** \brief Return lists of currently allocated block addresses and sizes.
*
*	Returns the addresses and sizes of power of 2 sized blocks currently
*	allocated within the region.
*	Blocks on the allocated list are a sub range of existing
*	segments with a backing file in the SAS Store directory.
*
*   @param blockAddr address of an array or addresses to be filled in.
*   @param blockSize address of an array of block size values to be filled in.
*   @param count address of a int to be filled in with the actual count of entries.
*
*   \todo should change this API to pass the max array size as a parm and return the actual count.
*/
extern __C__ void SASListInUseMem (void **blockAddr, unsigned long *blockSize,
                                           int *count);

/** \brief Return lists of freed block addresses and sizes.
*
*	Returns the addresses and sizes of power of 2 sized blocks currently
*	freed within the region.
*	Blocks on the freed list are a sub range of existing
*	segments with a backing file in the SAS Store directory.
*	It is possible for the free list to cover combined blocks of
*	multiple (power of 2) segments.
*
*   @param blockAddr address of an array or addresses to be filled in.
*   @param blockSize address of an array of block size values to be filled in.
*   @param count address of a int to be filled in with the actual count of entries.
*
*   \todo should change this API to pass the max array size as a parm and return the actual count.
*/
extern __C__ void SASListFreeMem (void **blockAddr, unsigned long *blockSize,
                                        int *count);

/** \brief Return lists of currently uncommitted block addresses and sizes.
*
*	Returns the addresses and sizes of power of 2 sized blocks
*	uncommitted blocks within the region.
*	Uncommitted blocks are a sub range of existing segments with a
*	backing file in the SAS Store directory, but have never been allocated.
*	Newly created segments start as uncommitted storage and can be
*	assumed to be initialized to zero.
*
*   @param blockAddr address of an array or addresses to be filled in.
*   @param blockSize address of an array of block size values to be filled in.
*   @param count address of a int to be filled in with the actual count of entries.
*
*   \todo should change this API to pass the max array size as a parm and return the actual count.
*/
extern __C__ void SASListUncommittedMem (void **blockAddr, unsigned long *blockSize,
                                                       int *count);

/** \brief Get the current number of free bytes within the Anchor blocks
*   internal heap.
*
*	\note Low values can prevent managing large or fragmented regions.
*	In this case consider increasing the anchor block size.
*
*	@return the number of free bytes remaining in the anchor blocks heap.
*/
extern __C__ unsigned int SASAnchorFreeSpace();

/** \brief Reset the SAS Region back to initial state.
*
*	\warning SASReset will result in the immediate lose of user data within the SAS Region.
*
*	Unmap any segments and delete backing files beyond the anchor segment.
*	Reinitialize the anchor block and lock segments to initial region state.
*
*/
extern __C__ void SASReset();

/** \brief Seize the anchor block master lock.
*
*   Used to serialize access to the anchor block and internal data structures.
*
*/
extern __C__ void SASSeize();

/** \brief Release the anchor block master lock.
*
*/
extern __C__ void SASRelease();

/** \brief Reset the internal Semaphores used by Seize/Release the SAS lock manager.
*
*	\warning Should not be called when multiple cooperating processes or
*	threads are accessing the SAS region. Should only be used to recover
*	from an application crash.
*
*	Reset internal semaphore to their initial state.
*
*/
extern __C__ void SASResetSem();

/** \brief Process wide cleanup for the SAS runtime.
+*
+*	\warning Do not attempt to use SAS/SPH APIs or access the SAS region after this call.
+*
+*	\warning Threaded applications should pthread_join their child threads before this call.
+*
+*	Unmap all SAS segments and detach the shmem segments used for locks.
+*   The runtime disables the sigsegv handler for the main thread.
+*
+*/
extern __C__ void SASCleanUp ();

/** \brief Thread specific setup for secondary thread accessing the SAS region.
*
*	\warning Each secondary thread of a SAS/SPH application should call
*	this function before accessing data within the SAS Region.
*
*   The runtime sets up a sigsegv handler for this thread. This allows the SAS
*   runtime to automatically attach SAS segments created by other processes.
*
*/
extern __C__ void SASThreadSetUp ();

/** \brief Thread specific cleanup for secondary thread accessing the SAS region.
*
*   For secondary threads that called SASThreadSetUp(), SASThreadCleanUp()
*   should be called before thread exit.
*   The runtime disables the sigsegv handler for this thread.
*
*/
extern __C__ void SASThreadCleanUp ();

/** \brief Remove the SAS Region.
*
*	\warning SASRemove will result in the immediate lose of user data within the SAS Region.
*
*	Unmap all segments and delete all backing files. Remove the shmem segments used for locks.
*
*/
extern __C__ void SASRemove();

/** \brief Sets the SAS finder pointer to a SAS block or utility object.

*	\warning The finder address is a shared SAS resource and must point to
*	a block or utility object within the SAS region.
*
*	Stores the region wide finder address within the anchor block.
*
*	\note the finder is normally a SPHContext containing name / address
*	associations for application data and other utility objects with the
*	SAS region. This helps the application find it's data and data
*	created by cooperating applications during startup. While setting
*	the finder to a context is normal it can be any data structure or
*	other utility object useful to the application.
*
*/
extern __C__ void setSASFinder (void *);

/** \brief Get the SAS finder address.
*
*	@return the SAS finder address from the anchor block.
*
*/
extern __C__ void *getSASFinder();

#endif /* __SAS_SIM_H */

