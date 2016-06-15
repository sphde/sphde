#ifndef __SPH_MAIN_DOX_H
#define __SPH_MAIN_DOX_H

/*
 * Copyright (c) 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe
 *     IBM Corporation, Adhemerval Zanella
 */

/** \mainpage Shared Persistent Heap Data Environment
* \brief SPHDE is composed of two major software layers: The Shared Address
* Space (SAS) layer provides the basic services for a shared address space and
* transparent, persistent storage.  The Shared Persistent Heap (SPH) layer
* organizes blocks of SAS storage into useful functions for storing and
* retrieving data.
*
*  \authors Steven Munroe, Ryan S. Arnold, et al.
*
*  \section Rationale
*
*  The I/O subsystems, memory subsystems, and supporting APIs of current
*  operating systems were designed for an environment of scarcity (limited
*  real memory and virtual address spaces). Modern applications have to
*  deal with increasingly complex data structures whose data needs to be
*  shared among process instances and persist to storage while still being
*  constrained by these APIs and subsystems.
*
*  Scarcity forces trade-offs between simplicity and efficiency and
*  complicates programming tasks.  This is especially true if these data
*  structures contain internal references (pointers). Traditional file
*  systems and relational databases simply don't handle internal reference
*  persistence well.  Data persistence to storage in these systems is not
*  transparent and requires additional layers of software to capture the
*  relationships represented by these references.  This adds even more
*  overhead and complexity to programming tasks.
*
*  The availability of 64-bit commodity processors, cheap high-density
*  DRAM, and commodity operating systems effectively eliminate the
*  original scarcity. The standard POSIX shared memory APIs (e.g., shmat,
*  shmdt,shmctl, et al.) enable the exploitation of abundant memory, but
*  are still not simple to use. A (relatively thin) API layer is needed to
*  manage shared memory access across a large shared address space in a
*  simple and coherent way.
*
*  Creating such an API is the goal of this project. The primary function
*  is to manage backing files and memory map them into the application.
*  For Linux, this allows data to be shared directly in the real pages of
*  the kernel's file cache. Since the files are always mapped at the same
*  virtual address, internal C pointers can be maintained for both
*  inter-process sharing and transparent persistent storage. This easily
*  supports zero-copy sharing and operate-in-place persistence.
*
*  \section sasSection SAS Overview
*  The SAS layer manages a region of process address space to provide:
*   \li Sharing and transparent storage persistence of data within that
*   region by providing and managing backing file space and memory mapping
*   the backing files for the process.
*   \li Sharing of data within that region between cooperating processes.
*   \li Locking primitives to provide synchronization of use/update
*   between cooperating processes and threads.
*   \li APIs for allocating/deallocating blocks of shared/persistent
*   data.
*   \li Further tools for managing blocks as data structures (heaps,
*   stacks, indexes, etc.).
*
*  Some additional SAS definitions:
*
*  For now, the size and virtual address range of the region is fixed for
*  each platform (somewhere beyond TASK_UNMAPPED_BASE and below the main
*  stack). Blocks are allocated in power-of-2 size and alignment from
*  within the region.
*
*  Backing storage (files) are allocated in power-of-2 sizes called
*  segments. Segments must be smaller in size than the region and
*  usually larger than blocks.  Segments don't necessarily have anything
*  to do with any notion of hardware segmentation, but it may be useful if
*  the size/alignment of SAS segments match the underlying hardware.
*
*  A simple example of using SPHDE: \code
#include <sassim.h> int main () { int rc;

  rc = SASJoinRegion();
  if (rc)
    return 1;

  char *block;

  // allocation a block of SPHDE data ...
  block = (char*)SASBlockAlloc (4096);

  // code accessing data in SPHDE region goes here.

  SASCleanUp();
  return 0;
}
*  \endcode
*
*  \note The application must first "join" a region before allocating or
*  accessing any blocks. If the region does not exist (has no backing
*  files), a "join" will create the initial segment (backing file) and
*  initialize the anchor block.
*
*  By default the region's path name is either "." or the current
*  directory.  The region name can be overridden by setting the
*  "SASSTOREPATH" environment variable, or using the SASJoinRegionByName()
*  function.  The "region" name is the path to a directory where the
*  SAS/SPH backing files will be created.
*
*  Processes that join with the same region name will share the region and
*  all of its (allocated) storage. All processes that share a region can
*  allocate, deallocate, reference and update blocks (normal C pointer
*  semantics) within the region.
*
*  \note To insure that segments of memory allocated by one process, can
*  be accessed by any other process sharing a SAS Region, the SPHDE
*  runtime establishes a sigaction handler for SIGSEGV within each
*  process and thread.  This is enabled as part of the Join process
*  and remains enabled until the process leaves Region by calling
*  SASCleanup.  Applications that use signal/sigaction handlers should
*  be aware of this.
*
*  Processes that use a different region name are independent from any
*  other region and only see the region that they have joined.  There is
*  no limit to the number of independent regions per process other than
*  those imposed by the file system. Normal file access rules apply. Any
*  process that does not have read/write authority to the region's
*  directory or files can not join the region or access the data.
*
*  Once a block is allocated it is backed with file storage, and
*  implicitly mmaped for transparent storage persistence and sharing
*  between processes. The block will always be mapped at the same virtual
*  address each time it is loaded and for each sharing process. This
*  allows complex pointer based data structures to be stored, persisted,
*  and shared without additional effort (the pointers are context
*  independent).
*
*  \subsection lockSection Lock Manager Overview
*  A lock manager is provides so utilities and cooperating processes can
*  synchronize their activities. Locks are "keyed" by virtual addresses which
*  are normally the address of some interesting shared data structure or
"  utility" object.
*
*  \code
#include <sassim.h>
#include <sasalloc.h>
#include <saslock.h>
int main ()
{
  int rc;

  rc = SASJoinRegion();

  if (rc)
    return 1;

  char *block;
  // primitive way to remember the root ptr
  block = getSASFinder();

  while (...)
  {
      SASLock(block, SasUserLock__WRITE);
      // synchronized access to "block"
      SASUnlock(block);
  }

  SASCleanUp();
  return 0;
}
*  \endcode
*
*  Currently the lock manager supports (shared) SasUserLock__READ and
*  (exclusive) SasUserLock__WRITE locks. The intent is to add other lock
*  types as needed. For example, the index utility could use an "INTENT"
*  lock (which allows multiple READ locks but is exclusive with other
*  INTENT and WRITE lock) which is upgradeable to a WRITE lock.

*  \note Notice the function getSASFinder(); The SAS runtime starts with a
*  special "anchor" block at a fixed location. The anchor block contains
*  the indexes used to track free space and allocations. It also contains
*  the "finder" pointer. The setSASFinder() allows t application to save a
*  pointer to any structure of its choice. The next time that the
*  application starts, it can use getSASFinder() to obtain the original
*  pointer and navigation from there.

*  \section sphsection SPH Utility Objects Overview
*  A simple pointer is sufficient to anchor a linked list or quad­tree
*  etc., but not very user friendly. SPHDE provides a number of utility
*  objects. Utility objects use blocks of storage to provide higher level
*  functions. An application could create a SASStringBTree_t or
*  SPHContext_t and store its pointer in the finder as in the following
*  example:
*
*  \code
#include <sassim.h>
#include <sasalloc.h>
#include <sphcontext.h>
#include <saslock.h>
int main ()
{
  int rc;
  SPHContext_t finderContext;

  rc = SASJoinRegion();
  if (rc)
    return 1;

  char *block;

  block = getSASFinder();
  if (block)
    finderContext = (SPHContext_t)block;
  else
    { // first time logic, only executes once.
      finderContext = SPHContextCreate(block__Size16M);
      setSASFinder(finderContext);
    }
  // create and provide a "name" for a new image
  block = (char*)SASBlockAlloc (block__Size4M);

  SPHContextAddName(finderContext, "my_image_buffer", block);

  SASLock(block, SasUserLock__WRITE);
  // should lock the new block because is now visible to others
     ...
  SASUnlock(block);
  ...
  // look up an old image by "name"
  block = SPHContextFindByName(finderContext, "my_old_image");
  if(block)
    {
      SASLock(block, SasUserLock__WRITE);
      ...
      SASUnlock(block);
    }

  SASCleanUp();
  return 0;

}
*
*  \endcode
*
*  \note A general purpose solution is provided by the SPHSetupProjectContext API
*  defined in sphcontext.h.
*
*  Utility objects like Context, Index, and StringBTree can be used to
*  create directories or index large arrays of data structures for search.
*  The Context is a combination of a StringBTree and a Index. It allows
*  any object (address) to have a symbolic name (or names). It also
*  provides the reverse mapping, from address to name(s).
*
*  \subsection SASRuntime Shared Address Space Runtime
*  - sassim.h SAS base Runtime
*  - saslock.h SAS Shared Address Locks
*  - sasatom.h SAS Atomic operation
*  - sasmsync.h SAS Memory Advice API
*
*  \subsection SASUtility General Utility Objects
*  - sassimplespace.h SAS Simple Space
*  - sassimplestack.h SAS Simple Stack
*  - sassimpleheap.h SAS Simple Heap
*  - sascompoundheap.h SAS Compound Heap; expanding heap of Simple Heaps
*  - sphcompoundpcqheap.h SAS Compound Heap extension; expanding heap of PCQueues
*
*  \subsection SASIndex Maps, indexes and Contexts
*  - sphcontext.h SPH Context
*  - sasstringbtree.h SAS String BTree
*  - sasstringbtreeenum.h String BTree Enum for accessing Contexts and String BTrees
*  - sasindex.h Binary Index BTree
*  - sasindexenum.h Binary BTree Index Enum for accessing Index BTrees
*
*  \note These (Context, Index, and StringBTree) utility objects are
*  implemented on (as a subclass of) the SASCompoundHeap_t utility object.
*  A SASCompoundHeap_t is a heap of SASSimpleHeap_t heaps.  The Btree
*  nodes are SASSimpleHeap_t, which are normally page size allocations.
*  This framework implicitly manages locality of reference, minimizing the
*  number of pages/cache-lines touched during any specific search.
*
*  \subsection SPHMultiCore Shared Persistent Heap, Multi-core Optimized Utilities
*  - sphlockfreeheap.h Lock Free Heap
*  - sphlflogger.h Lock Free Logger
*  - sphlogportal.h Lock Free Portal/Multiplexer for Loggers
*  - sphlflogentry.h Logger Entry access API
*  - sphdirectpcqueue.h Lock Free Direct Single Producer / Single Consumer Queue
*  - sphsinglepcqueue.h Lock Free Single Producer / Single Consumer Queue
*  - sphlfentry.h SPH Entry access API
*
*  \subsection SPH MultiCore Utility functions
*  - sphtimer.h APIs to access the high resolution timer and timer frequency
*  - sphthread.h APIs to identify processes and threads
**/

#endif /* __SPH_MAIN_DOX_H  */
