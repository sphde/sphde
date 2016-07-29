/*
 * Copyright (c) 2009, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe     - initial API and implementation
 *     IBM corporation, Adhemerval Zanella - tests organization
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasshm.h"
#include "sasalloc.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "sassimpleheap.h"
#include "sassimplestack.h"
#include "sassimplespace.h"
#include "sascompoundheap.h"
#include "sasstringbtree.h"
#include "sasstringbtreeenum.h"
#include "sasindexkey.h"
#include "sasindex.h"
#include "sasindexenum.h"
#include "sphcontext.h"
#include "sphlflogentry.h"
#include "sphlflogger.h"
#include "sphthread.h"
#include "sphtimer.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sassim_t";

static inline void
sassim_print_error (const char *test, int line, const char *fmt, ...)
{
  va_list args;
  fprintf (stderr, "%s:%s:%i error: ", sassim_prog_name, test, line);
  va_start (args, fmt);
  vfprintf (stderr, fmt, args);
  fprintf (stderr, "\n");
  va_end (args);
}

#define SASSIM_PRINT_ERR(fmt, ...) sassim_print_error(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

static inline void
sassim_print_msg (const char *func, int line, const char *fmt, ...)
{
  va_list args;
  printf ("%s:%s:%i ", sassim_prog_name, func, line);
  va_start (args, fmt);
  vprintf (fmt, args);
  printf ("\n");
  va_end (args);
}

#define SASSIM_PRINT_MSG(fmt, ...) sassim_print_msg(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

static void
sassim_dump_block (const char *func, int line, void *blockAddr,
		   unsigned long len)
{
  unsigned int *dumpAddr = (unsigned int *) blockAddr;
  unsigned char *charAddr = (unsigned char *) blockAddr;
  void *tempAddr;
  unsigned char chars[20];
  unsigned char temp;
  unsigned int i, j;
  chars[16] = 0;
  for (i = 0; i < len; i = i + 16)
    {
      tempAddr = dumpAddr;
      for (j = 0; j < 16; j++)
	{
	  temp = *charAddr++;
	  if ((temp < 32) || (temp > 126))
	    temp = '.';
	  chars[j] = temp;
	};
      sassim_print_msg (func, line, "%p  %08x %08x %08x %08x <%s>",
			tempAddr, *dumpAddr, *(dumpAddr + 1),
			*(dumpAddr + 2), *(dumpAddr + 3), chars);
      dumpAddr += 4;
    }
}
#define SASSIM_DUMP_BLOCK(fmt, ...) sassim_dump_block(__FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#define JOIN_EXIT_FAILURE 128

static int
sassim_sasseg_test ()
{
  char fileName[STORE_NAME_SIZE];
  int rc;

  SASSegNameIndexed (fileName, 0);
  if (!SASSegNameExists (fileName))
    return 1;

  // The SAS segments should not exit prior this call
  if (SASSegIndexExists (16))
    {
      SASSIM_PRINT_ERR ("SASSegIndexExists(16) already exists");
      return 1;
    }
  if ((rc = SASSegStoreCreate (16)))
    {
      SASSIM_PRINT_ERR ("SASSegStoreCreate(16): %i", rc);
      return 1;
    }
  if (!(rc = SASSegIndexExists (16)))
    {
      SASSIM_PRINT_ERR ("SASSegIndexExists(16): %i", rc);
      return 1;
    }
  if (SASSegStoreRemove (16))
    {
      SASSIM_PRINT_ERR ("SASSegStoreRemove(16): %i", rc);
      return 1;
    }
  return 0;
}

static int
sassim_msync_test ()
{
  void *baseAddr1 = NULL;
  unsigned long blockSize = block__Size1M;
  char *page1_ptr;
  char *pageX_ptr;
  int rc;

  baseAddr1 = (char *) SASBlockAlloc (blockSize);
  if (!baseAddr1)
    {
      SASSIM_PRINT_ERR ("SASBlockAlloc(%i)", blockSize);
      return 1;
    }
  page1_ptr = (char *) baseAddr1;
  memset (baseAddr1, 0, blockSize);
  sprintf ((char *) baseAddr1, "SAS Page 0@%p", baseAddr1);
  for (pageX_ptr = (page1_ptr + 4096); pageX_ptr < (page1_ptr + blockSize);
       pageX_ptr += 4096)
    {
      long page_num = (long) (pageX_ptr - page1_ptr) / 4096;
      sprintf (pageX_ptr, "SAS Page %ld@%p", page_num, pageX_ptr);
    } SASSIM_DUMP_BLOCK (baseAddr1, 32);
  SASSIM_PRINT_MSG ("sasMsyncWrite (%p, 4096, %d)", page1_ptr, SAS_SYNC);
  if ((rc = sasMsyncWrite (page1_ptr, 4096, SAS_SYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncWrite(%p, 4096, SAS_SYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasMsyncPurge (%p, 4096, %d)", page1_ptr, SAS_SYNC);
  if ((rc = sasMsyncPurge (page1_ptr, 4096, SAS_SYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncPurge(%p, 4096, SAS_SYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasMsyncBring (%p, 4096)", page1_ptr);
  if ((rc = sasMsyncBring (page1_ptr, 4096)))
    {
      SASSIM_PRINT_ERR ("sasMsyncBring(%p, 4096): %i", page1_ptr, rc);
      return 1;
    }
  SASSIM_DUMP_BLOCK (page1_ptr + blockSize - 4096, 32);
  SASSIM_PRINT_MSG ("sasMsyncPurge (%p, 4096, %d)", page1_ptr, SAS_SYNC);
  if ((rc = sasMsyncPurge (page1_ptr, 4096, SAS_SYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncPurge(%p, 4096, SAS_SYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasMsyncSequential (%p, 4096)", page1_ptr);
  if ((rc = sasMsyncSequential (page1_ptr, 4096)))
    {
      SASSIM_PRINT_ERR ("sasMsyncSequential(%p, 4096): %i", page1_ptr, rc);
      return 1;
    }
  SASSIM_DUMP_BLOCK (page1_ptr + blockSize - 4096, 32);
  SASSIM_PRINT_MSG ("sasMsyncPurge (%p, 4096, %d)", page1_ptr, SAS_SYNC);
  if ((rc = sasMsyncPurge (page1_ptr, 4096, SAS_SYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncPurge(%p, 4096, SAS_SYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasMsyncRandom (%p, 4096)", page1_ptr);
  if ((rc = sasMsyncRandom (page1_ptr, 4096)))
    {
      SASSIM_PRINT_ERR ("sasMsyncRandom(%p, 4096): %i", page1_ptr, rc);
      return 1;
    }
  SASSIM_DUMP_BLOCK (page1_ptr + blockSize - 4096, 32);
  SASSIM_PRINT_MSG ("sasMsyncWrite (%p, %ld, %d)", page1_ptr, blockSize,
		    SAS_ASYNC);
  if ((rc = sasMsyncWrite (page1_ptr, blockSize, SAS_ASYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncWrite(%p, 4096, SAS_ASYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasMsyncPurge (%p, %ld, %d)", page1_ptr, blockSize,
		    SAS_ASYNC);
  if ((rc = sasMsyncPurge (page1_ptr, blockSize, SAS_ASYNC)))
    {
      SASSIM_PRINT_ERR ("sasMsyncPurge(%p, 4096, SAS_ASYNC): %i", page1_ptr,
			rc);
      return 1;
    }
  SASSIM_DUMP_BLOCK (page1_ptr + (blockSize / 2), 32);
  SASSIM_PRINT_MSG ("sasMsyncBring (%p, %ld)", page1_ptr, blockSize);
  if ((rc = sasMsyncBring (page1_ptr, blockSize)))
    {
      SASSIM_PRINT_ERR ("sasMsyncBring(%p, 4096): %i", page1_ptr, rc);
      return 1;
    }
  SASSIM_DUMP_BLOCK (baseAddr1, 32);
  SASSIM_DUMP_BLOCK (page1_ptr + blockSize - 4096, 32);
  SASBlockDealloc (baseAddr1, blockSize);
  return 0;
}

static int
sassim_heap_test1 ()
{
  SASSimpleHeap_t simpleHeap, simpleHeap2;
  unsigned long blockSize = block__Size64K;
  char *temp1, *temp2, *temp3, *temp4, *temp5, *temp6;
  unsigned long rSize;
  simpleHeap = SASSimpleHeapCreate (blockSize);

  if (!simpleHeap)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapCreate(%i)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapCreate (%ld) success", blockSize);
  SASSIM_PRINT_MSG ("sizeof(FreeNode) = %zu", sizeof (FreeNode));
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_PRINT_MSG ("SASSimpleHeapEmpty(simpleHeap) = %i",
		    SASSimpleHeapEmpty (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp1 = (char *) SASSimpleHeapAlloc (simpleHeap, 12);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(simpleHeap, 12)");
      return 1;
    }
  sprintf (temp1, "heap-%2d    ", 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  simpleHeap2 = SASSimpleHeapNearFind (temp1);
  SASSIM_PRINT_MSG ("SASSimpleHeapNearFind(%p) = %p", temp1, simpleHeap2);
  SASSIM_PRINT_MSG ("SASSimpleHeapEmpty(%p) = %d",
		    SASSimpleHeapEmpty (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp2 = (char *) SASSimpleHeapAlloc (simpleHeap, 20);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(simpleHeap, 20)");
      return 1;
    }
  sprintf (temp2, "heap-%2d         ", 20);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 20);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp3 = (char *) SASSimpleHeapAlloc (simpleHeap, 8);
  if (!temp3)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(simpleHeap, 8)");
      return 1;
    }
  sprintf (temp3, "heap-%d", 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp4 = (char *) SASNearAlloc (temp1, 8);
  if (!temp4)
    {
      SASSIM_PRINT_ERR ("SASNearAlloc(%p, %d)", temp1, 8);
      return 1;
    }
  sprintf (temp4, "near-%d", 8);
  SASSIM_PRINT_MSG ("SASNearAlloc (%p, %d) success", temp1, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp5 = (char *) SASNearAlloc (temp3, 12);
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASNearAlloc(%p, %d)", temp3, 12);
      return 1;
    }
  sprintf (temp5, "near-%d", 12);
  SASSIM_PRINT_MSG ("SASNearAlloc (%p, %d) success", temp3, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  rSize = SASSimpleHeapFreeSpace (simpleHeap);
  temp6 = (char *) SASSimpleHeapAlloc (simpleHeap, rSize);
  if (!temp6)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(%p, %d)", simpleHeap, rSize);
      return 1;
    }
  sprintf (temp6, "block-%ld", rSize);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %ld) success", simpleHeap,
		    rSize);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  if (SASSimpleHeapFree (simpleHeap, temp6, rSize))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %ld) failed", simpleHeap,
			temp6, rSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  if (SASSimpleHeapFree (simpleHeap, temp3, 8))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d) failed", simpleHeap,
			temp3, 8);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  if (SASSimpleHeapFree (simpleHeap, temp1, 12))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d) failed", simpleHeap,
			temp1, 12);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  SASNearDealloc (temp5, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  if (SASSimpleHeapFree (simpleHeap, temp2, 20))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d) failes", simpleHeap,
			temp2, 20);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  SASNearDealloc (temp4, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (simpleHeap));
  SASSIM_PRINT_MSG ("SASSimpleHeapEmpty(%p) = %d",
		    SASSimpleHeapEmpty (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  if (SASSimpleHeapDestroy (simpleHeap))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapDestroy(%p) failed", simpleHeap);
      return 1;
    }
  SASSIM_DUMP_BLOCK (simpleHeap, 144);
  return 0;
}

static int
sassim_heap_test2 ()
{
  SASSimpleHeap_t simpleHeap =
    (char *) __SAS_TEMP_ADDRESS + __SAS_SHMAP_MAX + __SAS_SHMAP_MAX;
  char *temp1, *temp2, *temp3, *temp4, *temp5;
  sasshm_t lock_mem = 0;
  key_t test_heap = 4321;

  lock_mem = SASAllocateShmID (test_heap, simpleHeap, __SAS_SHMAP_MAX);
  if (lock_mem == -1)
    {
      SASSIM_PRINT_ERR ("SASAllocateShmID(%d, %p, __SAS_SHMAP_MAX)",
			test_heap, simpleHeap);
      return 1;
    }
  memset (simpleHeap, 0, __SAS_SHMAP_MAX);
  simpleHeap =
    SASSimpleHeapInit (simpleHeap, SAS_RUNTIME_LOCKTABLE, __SAS_SHMAP_MAX);
  SASSIM_PRINT_MSG ("SASSimpleHeapInit (%p, %ld) success", simpleHeap,
		    __SAS_SHMAP_MAX);
  SASSIM_PRINT_MSG ("sizeof(FreeNode) = %zu", sizeof (FreeNode));
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp1 = (char *) SASSimpleHeapAllocNoLock (simpleHeap, 12);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAllocNoLock(%p, %d)", simpleHeap, 12);
      return 1;
    }
  sprintf (temp1, "heap-%2d    ", 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp2 = (char *) SASSimpleHeapAllocNoLock (simpleHeap, 20);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAllocNoLock(%p, %d)", simpleHeap, 20);
      return 1;
    }
  sprintf (temp2, "heap-%2d         ", 20);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 20);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp3 = (char *) SASSimpleHeapAllocNoLock (simpleHeap, 8);
  if (!temp3)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAllocNoLock(%p, %d)", simpleHeap, 8);
      return 1;
    }
  sprintf (temp3, "heap-%d", 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", simpleHeap, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp4 = (char *) SASSimpleHeapNearAlloc (temp1, 8);
  if (!temp4)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p, %d)", temp1, 8);
      return 1;
    }
  sprintf (temp4, "near-%d", 8);
  SASSIM_PRINT_MSG ("SASNearAlloc (%p, %d) success", temp1, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  temp5 = (char *) SASSimpleHeapNearAlloc (temp3, 12);
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p, %d)", temp3, 12);
      return 1;
    }
  sprintf (temp5, "near-%d", 12);
  SASSIM_PRINT_MSG ("SASNearAlloc (%p, %d) success", temp3, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  if (SASSimpleHeapFreeNoLock (simpleHeap, temp1, 12))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d)", simpleHeap, temp1,
			12);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  if (SASSimpleHeapFreeNoLock (simpleHeap, temp2, 20))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d)", simpleHeap, temp2,
			20);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  if (SASSimpleHeapFreeNoLock (simpleHeap, temp3, 8))
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapFree (%p, %p, %d)", simpleHeap, temp3,
			8);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  SASSimpleHeapNearDealloc (temp4, 8);
  SASSIM_PRINT_MSG ("SASNearDealloc (%p, %d) success", temp4, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  SASSimpleHeapNearDealloc (temp5, 12);
  SASSIM_PRINT_MSG ("SASNearDealloc (%p, %d) success", temp5, 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpaceNoLock (simpleHeap));
  SASSIM_DUMP_BLOCK (simpleHeap, 128);
  SASDetachShm (simpleHeap);
  SASRemoveShmID (lock_mem);
  return 0;
}

static int
sassim_space_test1 ()
{
  SASSimpleSpace_t simpleSpace, simpleSpace2;
  unsigned long spaceSize = 48000;
  char *temp1;

  simpleSpace = SASSimpleSpaceCreate (spaceSize);
  if (!simpleSpace)
    {
      SASSIM_PRINT_ERR ("SASSimpleSpaceCreate(%u)", spaceSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleSpaceCreate (%d) success", spaceSize);
  SASSIM_PRINT_MSG ("SASSimpleSpaceFreeSpace() = %zu",
		    SASSimpleSpaceFreeSpace (simpleSpace));
  temp1 = (char *) SASSimpleSpaceToAddr (simpleSpace);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASSimpleSpaceToAddr(%p)", simpleSpace);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleSpaceToAddr (%p) = %p", simpleSpace, temp1);
  simpleSpace2 = (char *) SASSimpleSpaceFromAddr (temp1);
  if (!simpleSpace2)
    {
      SASSIM_PRINT_ERR ("SASSimpleSpaceFromAddr(%p)", temp1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASSimpleSpaceFromAddr (%p) = %p", temp1, simpleSpace2);
  SASSIM_DUMP_BLOCK (simpleSpace, 128);
  if (SASSimpleSpaceDestroy (simpleSpace))
    {
      SASSIM_PRINT_ERR ("SASSimpleSpaceDestroy(%p)", simpleSpace);
      return 1;
    }
  SASSIM_DUMP_BLOCK (simpleSpace, 144);
  return 0;
}

int
main ()
{
  int rc;
  int failures = 0;

  if ((rc = SASJoinRegion ()))
    {
      SASSIM_PRINT_ERR ("SASJoinRegion: %i", rc);
      exit (JOIN_EXIT_FAILURE);
    }
  printf ("__SAS_BASE_ADDRESS=%lx\n", __SAS_BASE_ADDRESS);
  printf ("RegionSize        =%lx\n", RegionSize);
  printf ("SegmentSize       =%lx\n", SegmentSize);
  printf ("__SAS_TEMP_ADDRESS=%lx\n", __SAS_TEMP_ADDRESS);
  printf ("__SAS_TEMP_FREE   =%lx\n", __SAS_TEMP_FREE);

  failures += sassim_sasseg_test ();

  failures += sassim_msync_test ();

  failures += sassim_heap_test1 ();

  failures += sassim_heap_test2 ();

  failures += sassim_space_test1 ();

  SASRemove ();

  return failures;
}
