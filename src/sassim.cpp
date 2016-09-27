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

//#define __SASDebugPrint__

// test for storage allocation
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
/*
#ifdef __USE_SHM
#include <sys/shm.h>
#else
#include <sys/mman.h>
#endif
#include <sys/stat.h>
*/
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "sasio.h"
#include "freenode.h"
#include "sasanchr.h"
#include "sasalloc.h"
#include "ultree.h"
#include "sassim.h"
#include "saslock.h"
#include "sasconf.h"
#include "sasstname.h"

#define	createOnlyFlags (IPC_EXCL | IPC_CREAT | 0666 )
#define	createFlags (IPC_CREAT | 0666 )
#define	attachFlags ( 0666 )

typedef struct
{
  SASBlockHeader header;
  SASAnchor_t anchors;
} SASAnchorBlock_t;

typedef struct
{
  uLongTreeNode *root;
} roottype;

unsigned long memLow __attribute__ ((visibility ("hidden")));
unsigned long memHigh __attribute__ ((visibility ("hidden")));

key_t sas_key;
int *mem_IDs;
int sasClearOnDealloc = 0;

#ifdef __WORDSIZE_64
#define maxLog2 36
#else
#define maxLog2 20
#endif

unsigned long logTable[maxLog2] = { 0x1000,	// 00 = 004k
  0x2000,			// 01 =   8k
  0x4000,			// 02 =  16k
  0x8000,			// 03 =  32k
  0x10000,			// 04 =  64k
  0x20000,			// 05 = 128k
  0x40000,			// 06 = 256k
  0x80000,			// 07 = 512k
  0x100000,			// 08 =   1m
  0x200000,			// 09 =   2m
  0x400000,			// 10 =   4m
  0x800000,			// 11 =   8m
  0x1000000,			// 12 =  16m
  0x2000000,			// 13 =  32m
  0x4000000,			// 14 =  64m
  0x8000000,			// 15 = 128m
  0x10000000,			// 16 = 256m
  0x20000000,			// 17 = 512m
  0x40000000,			// 18 =   1g
#ifdef __WORDSIZE_64
  0x80000000UL,			// 19 =   2g
  0x100000000UL,		// 20 = 4g
  0x200000000UL,		// 21 = 8g
  0x400000000UL,		// 22 =  16g
  0x800000000UL,		// 23 =  32g
  0x1000000000UL,		// 24 =  64g
  0x2000000000UL,		// 25 = 128g
  0x4000000000UL,		// 26 = 256g
  0x8000000000UL,		// 27 = 512g
  0x10000000000UL,		// 28 = 1t
  0x20000000000UL,		// 29 = 2t
  0x40000000000UL,		// 30 = 4t
  0x80000000000UL,		// 31 = 8t
  0x100000000000UL,		// 32 = 16t
  0x200000000000UL,		// 33 = 32t
  0x400000000000UL,		// 34 = 64t
  0x800000000000UL		// 35 = 128t
#else
  0x80000000			// 19 =   2g
#endif
};

typedef struct
{
#ifdef __WORDSIZE_64
#if defined (__x86_64__) || \
    (defined (__LITTLE_ENDIAN__) && defined (__powerpc64__)) \
    || defined (__aarch64__) || defined (__arm__)
  unsigned long offset:56;
  unsigned int size:8;
#else
  unsigned int size:8;
  unsigned long offset:56;
#endif
#else
#if __BYTE_ORDER == __BIG_ENDIAN
  unsigned int size:8;
  unsigned int offset:24;
#else
  unsigned int offset:24;
  unsigned int size:8;
#endif
#endif
} logNodeType;

typedef union
{
  logNodeType node;
  unsigned long val;
} longNodeType;

static unsigned long
nodeToLong (unsigned long offset, unsigned int size)
{
  longNodeType temp;
  temp.node.offset = offset >> 8;
  temp.node.size = size;
  return temp.val;
}

static unsigned long
longToOffset (unsigned long val)
{
  longNodeType temp;
  temp.val = val;
  return (temp.node.offset) << 8;
}

static unsigned int
longToSize (unsigned long val)
{
  longNodeType temp;
  temp.val = val;
  return temp.node.size;
}

static unsigned int
SizeToLog2 (unsigned long size)
{
  unsigned int result = 0;
  int i;
#ifdef __SASDebugPrint__
  sas_printf ("SizeToLog2 (%lx), logTable[%d]=%lx\n", size, maxLog2 - 1,
	      logTable[maxLog2 - 1]);
#endif
  for (i = (maxLog2 - 1); i >= 0; i--)
    {
      if (size == logTable[i])
	{
	  result = i;
	  break;
	}
    }
#ifdef __SASDebugPrint__
  sas_printf ("SizeToLog2 (%lx) = %d\n", size, result);
#endif
  return result;
}

static void
p2AddUsed (uLongTreeNode ** root, unsigned long size, void *loc)
{
  uLongTreeNode *n;
  unsigned long keys;
  unsigned long offset = ((unsigned long) loc) - memLow;

  keys = nodeToLong (offset, SizeToLog2 (size));
  n = (uLongTreeNode *) SASNearAlloc (root, sizeof (uLongTreeNode));
  n->init (keys, (unsigned long) loc);
  (*root)->insertNode (root, n);
}

static void
p2RemUsed (uLongTreeNode ** root, unsigned long size, void *loc)
{
  uLongTreeNode *n;
  uLongTreeNode **nn;
  unsigned long keys;
  unsigned long k, l;
  unsigned long offset = ((long) loc) - memLow;

  keys = nodeToLong (offset, SizeToLog2 (size));
  nn = (*root)->searchEqualOrNextNode (root, keys);
  if (nn)
    {
      n = *nn;
      if (n)
	{
	  k = n->getKey ();
	  l = n->getInfo ();
	  if ((k == keys) && (l == (unsigned long) loc))
	    {
	      n = n->removeNode (nn);
	      SASNearDealloc (n, sizeof (uLongTreeNode));
	    }
	  else
	    {
	      sas_printf ("!SAS integrity check failed\n");
	      sas_printf ("  returning %p size %p\n", loc, (void *) size);
	    }
	}
    }
  else
    {
      sas_printf ("!SAS integrity check failed\n");
      sas_printf ("  returning %p size %p\n", loc, (void *) size);
    }
}

static void *
p2Alloc (uLongTreeNode ** root, unsigned long size)
{
  uLongTreeNode *n;
  uLongTreeNode **nn;
  unsigned long k, l, m;
  unsigned long keys;
  unsigned long result = 0;
  unsigned int j;
  unsigned int ui, uj;

#ifdef __SASDebugPrint__
  sas_printf ("p2Alloc (%p, %lu)\n", root, size);
#endif
  keys = nodeToLong (0, SizeToLog2 (size));

  nn = (*root)->searchEqualOrNextNode (root, keys);
  if (nn)
    {
      j = longToSize (keys);
      n = *nn;
      if (n)
	{
	  k = n->getKey ();
	  l = n->getInfo ();
#ifdef __SASDebugPrint__
	  sas_printf ("Found->%p-%p\n", (void *) k, (void *) l);
#endif
	  n = n->removeNode (nn);
	  if (n)
	    {
	      ui = longToOffset (k);
	      uj = longToSize (k);
	      result = l;	// nodeToLong(ui, j);
	      if (j == uj)
		{
		  SASNearDealloc (n, sizeof (uLongTreeNode));
		}
	      else
		{
#ifdef __SASDebugPrint__
		  sas_printf ("Split");
#endif
		  while (j < uj)
		    {
		      uj--;
		      if (!n)
			n = (uLongTreeNode *) SASNearAlloc (root,
							    sizeof
							    (uLongTreeNode));
		      k = nodeToLong (ui + logTable[uj], uj);
		      m = l + logTable[uj];
#ifdef __SASDebugPrint__
		      sas_printf (" ->%p-%p", (void *) k, (void *) m);
#endif
		      n->init (k, m);
		      (*root)->insertNode (root, n);
		      n = NULL;
		    }
#ifdef __SASDebugPrint__
		  sas_printf ("\n");
#endif
		}
	    }
	  else
	    {
	      sas_printf ("p2Alloc:: <n->removeNode (nn); == NULL>!\n");
	    };
	}
      else
	{
	  sas_printf ("p2Alloc:: <n = *nn; == NULL>!\n");
	}
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("p2Alloc::nothing big enough!\n");
#endif
    }

#ifdef __SASDebugPrint__
  sas_printf ("\n  result = %p\n", (void *) result);
#endif
  return ((void *) result);
}


static void
p2Dealloc (uLongTreeNode ** root, unsigned long size, void *loc)
{
  uLongTreeNode *n;
  uLongTreeNode **nn;
  unsigned long k;
  unsigned long val = (unsigned long) loc;
  unsigned long keys;
  int flag;
  unsigned int ui, uj;

  keys = nodeToLong ((val - memLow), SizeToLog2 (size));
  n = NULL;
#ifdef __SASDebugPrint__
  sas_printf ("%p", (void *) keys);
#endif

  do
    {
      uj = longToSize (keys);
      ui = longToOffset (keys);
      k = nodeToLong ((ui ^ logTable[uj]), uj);
      nn = (*root)->searchEqualOrNextNode (root, k);
      if (nn && (uj < (maxLog2)))
	{
	  n = *nn;
	  if (k == n->getKey ())
	    {
#ifdef __SASDebugPrint__
	      sas_printf ("&%p", (void *) k);
#endif
	      n = n->removeNode (nn);
	      SASNearDealloc (n, sizeof (uLongTreeNode));
	      n = NULL;
	      if (keys > k)
		{
		  ui = longToOffset (k);
		  val = val - logTable[uj];
		};
	      uj++;
	      keys = nodeToLong (ui, uj);
#ifdef __SASDebugPrint__
	      sas_printf ("->%p,%p ", (void *) keys, (void *) val);
#endif
	      flag = (uj < maxLog2);
	    }
	  else
	    {
	      n = NULL;
	      flag = 0;
	    };
	}
      else
	{
	  flag = 0;
	};
    }
  while (flag);

  if (!n)
    n = (uLongTreeNode *) SASNearAlloc (root, sizeof (uLongTreeNode));

  n->init (keys, val);
  (*root)->insertNode (root, n);
#ifdef __SASDebugPrint__
  sas_printf ("\n");
#endif
}

unsigned long
getMemLow ()
{
  return memLow;
}

unsigned long
getMemHigh ()
{
  return memHigh;
}

void
setSASmemrange (unsigned long low, unsigned long high)
{
    memLow = low;
    memHigh = high;
}

static int
initSASSem (SASAnchor_t * anchor)
{
  int rc = 0;

#ifdef __GNUC__
  rc = sem_init (&anchor->SASSem, 1, 1);
  if (rc != 0)
    sas_printf ("initSASSem: sem_init failed: %s\n", strerror (errno));
#else
  msemaphore *tempSem = msem_init (&(anchor->SASSem), MSEM_UNLOCKED);
  if (!tempSem)
    {
      sas_printf ("initSASSem: msem_init failed: %s\n", strerror (errno));
      rc = 1;
    }
#endif
  return rc;
}

static void
seizeSASSem (SASAnchor_t * anchor)
{
  int rc;
#ifdef __GNUC__
  rc = sem_wait (&anchor->SASSem);
  if (rc != 0)
    sas_printf ("seizeSASSem: sem_wait failed: %s\n", strerror (errno));
#else
  rc = msem_lock (&(anchor->SASSem), 0);
  if (rc != 0)
    sas_printf ("seizeSASSem: msem_lock failed %s\n", strerror (errno));
#endif
}

static void
releaseSASSem (SASAnchor_t * anchor)
{
  int rc;
#ifdef __GNUC__
  rc = sem_post (&anchor->SASSem);
  if (rc != 0)
    sas_printf ("releaseSASSem: sem_post failed: %s\n", strerror (errno));
#else
  rc = msem_unlock (&(anchor->SASSem), 0);
  if (rc != 0)
    sas_printf ("releaseSASSem: msem_unlock failed: %s\n", strerror (errno));
#endif
}

static void
destroySASSem (SASAnchor_t * anchor)
{
  int rc;
#ifdef __GNUC__
  rc = sem_destroy (&anchor->SASSem);
  if (rc != 0)
    sas_printf ("destroySASSem: sem_destroy failed: %s\n", strerror (errno));
#else
  rc = msem_remove (&(anchor->SASSem), 0);
  if (rc != 0)
    sas_printf ("destroySASSem: msem_remove failed: %s\n", strerror (errno));
#endif
}

static void
initRegion ()
{
  SASBlockHeader *anchorBlock;
  uLongTreeNode **nn;
  char *blockHeap;
  SASAnchor_t *anchor;
  unsigned long keys;

  memLow = (unsigned long) __SAS_BASE_ADDRESS;
  memHigh = memLow + RegionSize;
  anchorBlock = (SASBlockHeader *) memLow;

#ifdef __SASDebugPrint__
  sas_printf ("initRegion anchor@%p\n\n", anchorBlock);
#endif
  blockHeap = (char *) anchorBlock;
  blockHeap = blockHeap + block__Size4K;
  initSOMSASBlock (anchorBlock, SAS_RUNTIME_ANCHOR, block__Size1M, blockHeap);
  anchorBlock->special = anchorBlock + 1;
  anchor = (SASAnchor_t *) anchorBlock->special;
  anchor->regionSize = RegionSize;
  anchor->finder = NULL;
  anchor->uncommitted = NULL;
  anchor->free = NULL;
  anchor->used = NULL;
  anchor->region = NULL;
  anchor->allocated = NULL;

#ifdef __SASDebugPrint__
  sas_printf ("initRegion uncommitted %lx\n", SegmentSize);
#endif
  nn = &(anchor->uncommitted);

  keys = nodeToLong (0, SizeToLog2 (SegmentSize));
  anchor->uncommitted->insertNode (nn, keys, (unsigned long) anchorBlock);

#ifdef __SASDebugPrint__
  sas_printf ("initRegion used %lx\n", block__Size1M);
#endif
  p2Alloc (nn, block__Size1M);

  nn = &(anchor->used);
  p2AddUsed (nn, block__Size1M, anchorBlock);

#ifdef __SASDebugPrint__
  sas_printf ("initRegion region %lx\n", RegionSize);
#endif
  nn = &(anchor->region);
  keys = nodeToLong (0, SizeToLog2 (RegionSize));
  anchor->region->insertNode (nn, keys, (unsigned long) anchorBlock);
#ifdef __SASDebugPrint__
  sas_printf ("initRegion allocate %lx\n", SegmentSize);
#endif
  p2Alloc (nn, SegmentSize);
  nn = &(anchor->allocated);
  p2AddUsed (nn, SegmentSize, anchorBlock);

  initSASSem (anchor);
}

int
SASAttachSegByName (void *baseAddr, unsigned long size,
		    int segIndex, char *name, int flags)
{
  void *BaseAddr1 = NULL;
  int fd;
  int rc = 0;

#ifdef __SASDebugPrint__
  sas_printf ("SASAttachSegByName (%p,%lx,%s,%o)\n",
	      baseAddr, size, name, flags);
#endif
#ifdef __USE_SHM
  sas_key = ftok (name, 'R');
  if (sas_key != (key_t) - 1)
    {
      mem_ID = shmget (sas_key, size, flags);
      if (mem_ID != -1)
	{
#ifdef __SASDebugPrint__
	  sas_printf ("   shmget(%x,%lx,%o)\n", sas_key, size, flags);
#endif
	  BaseAddr1 = shmat (mem_ID, baseAddr, 0);
	  if (((int) BaseAddr1) != -1)
	    {
#ifdef __SASDebugPrint__
	      sas_printf ("   shmat(%x,%p,%o)\n", mem_ID, baseAddr, 0);
#endif
	      mem_IDs[segIndex] = mem_ID;
	      rc = 0;
	    }
	  else
	    {
	      sas_printf ("SASAttachSegByName:shmat failed! %s\n",
			  strerror (errno));
	      rc = 1;
	    }
	}
      else
	{
	  sas_printf ("SASAttachSegByName:shmget failed! %s\n",
		      strerror (errno));
	  sas_printf ("   shmget(%x,%x,%o)\n", sas_key, size, flags);
	  rc = 2;
	}
    }
  else
    {
      sas_printf ("SASAttachSegByName:ftok failed! %s:\n", strerror (errno));
      rc = 3;
    }
#else
  fd = open (name, O_RDWR);
  if (fd != -1)
    {
      BaseAddr1 = mmap (baseAddr, size,
			(PROT_READ | PROT_WRITE),
			(MAP_SHARED | MAP_FIXED), fd, 0);
      if ((long) BaseAddr1 == -1)
	{
	  sas_printf ("SASAttachSegByName:mmap failed! %s:\n",
		      strerror (errno));
	  rc = 2;
	}
      else
	mem_IDs[segIndex] = 1;
      close (fd);
    }
  else
    {
      sas_printf ("SASAttachSegByName:open failed! %s:\n", strerror (errno));
      rc = 3;
    }
#endif
  return rc;
}

int
SASDetachSegByAddr (void *segAddr, unsigned long size)
{
  unsigned long regionOffset = ((unsigned long) segAddr - memLow);
  int segIndex = regionOffset / size;
  int rc = 0;

#ifdef __SASDebugPrint__
  sas_printf ("SASDetachSegByAddr(%p)\n", segAddr);
#endif
#ifdef __USE_SHM
  if (shmdt ((char *) segAddr))
    {
      sas_printf ("SASDetachSegByAddr :shmdt Error %s\n", strerror (errno));
      rc = 3;
    }
#else
  if (munmap (segAddr, size))
    {
      sas_printf ("SASDetachSegByAddr :munmap Error %s\n", strerror (errno));
      rc = 3;
    }
#endif
  mem_IDs[segIndex] = 0;

  return rc;
}

int
SASRemoveSegByAddr (void *segAddr, unsigned long size)
{
  unsigned long regionOffset = ((unsigned long) segAddr - memLow);
  int segIndex = regionOffset / size;
  char name[STORE_NAME_SIZE];
#ifdef __USE_SHM
  struct shmid_ds shmid_buf;
#endif
  int rc;
#ifdef __SASDebugPrint__
  sas_printf ("SASRemoveSegByAddr(%p) -> %d\n", segAddr, segIndex);
#endif
  rc = SASDetachSegByAddr (segAddr, size);

#ifdef __USE_SHM
  if (shmctl (mem_IDs[segIndex], IPC_RMID, &shmid_buf))
    {
      sas_printf ("SASRemoveSegByAddr :shmctl Error %s\n", strerror (errno));
      rc = 2;
    }
#endif
  SASSegNameIndexed (&name[0], segIndex);
  rc = SASSegStoreRemoveByName (name);
  if (rc)
    {
      sas_printf ("SASRemoveSegByAddr, SASSegStoreRemoveByName failed\n");
      rc = 1;
    }

  return rc;
}

int
SASCreateSegByAddr (void *segAddr, unsigned long size)
{
  unsigned long regionOffset = ((unsigned long) segAddr - memLow);
  int segIndex = regionOffset / size;
  char name[STORE_NAME_SIZE];
  int rc;

#ifdef __SASDebugPrint__
  sas_printf ("SASCreateSegByAddr(%p) -> %d\n", segAddr, segIndex);
#endif
  SASSegNameIndexed (&name[0], segIndex);
  rc = SASSegStoreCreateByName (name);
  if (rc)
    {
      sas_printf ("SASCreateSegByAddr, SASSegStoreCreateByName failed\n");
      rc = 1;
    }
  else
    {
      rc = SASAttachSegByName (segAddr, size,
			       segIndex, name, createOnlyFlags);
    }

  return rc;
}

int
SASAttachSegByAddr (void *segAddr, unsigned long size)
{
  unsigned long regionOffset = ((unsigned long) segAddr - memLow);
  int segIndex = regionOffset / size;
  char name[STORE_NAME_SIZE];
  int rc;

#ifdef __SASDebugPrint__
  sas_printf ("SASAttachSegByAddr(%p) -> %d\n", segAddr, segIndex);
#endif
  SASSegNameIndexed (&name[0], segIndex);
  rc = SASAttachSegByName (segAddr, size, segIndex, name, attachFlags);
  return rc;
}

void
SASAttachAllocatedAddr (void *segAddr)
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.allocated;
  void *blockAddr;
  void *segBase;
  unsigned long blockSize;
  unsigned long blockMask = ~(SegmentSize - 1);
  unsigned long keys = 0;

  segBase = (void *) ((unsigned long) segAddr & blockMask);
#ifdef __SASDebugPrint__
  sas_printf ("SASAttachAllocatedAddr @ <%p, %p>\n", segAddr, segBase);
#endif

  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize = logTable[longToSize (keys)];
	  if (segBase == blockAddr)
	    {
#ifdef __SASDebugPrint__
	      sas_printf ("SASAttachAllocatedAddr segment <%p, %lx>\n",
			  blockAddr, blockSize);
#endif
	      if (SASAttachSegByAddr (segBase, blockSize))
		sas_printf ("SASAttachAllocatedAddr:%s for %p:\n",
			    "SASAttachSegByAddr failed", blockAddr);
	      break;
	    }
	}
    }
  while (n);
}

void
SASAttachAllocatedSegs ()
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.allocated;
  void *blockAddr;
  unsigned long blockSize;
  unsigned long keys = 0;

  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize = logTable[longToSize (keys)];
	  if (cnt != 0)
	    {
#ifdef __SASDebugPrint__
	      sas_printf ("SASAttachAllocatedSegs segment <%p, %lx>\n",
			  blockAddr, blockSize);
#endif
	      if (SASAttachSegByAddr (blockAddr, blockSize))
		sas_printf ("SASAttachAllocatedSegs:%s for %p:\n",
			    "SASAttachSegByAddr failed", blockAddr);
	    }
	  cnt++;
	};
    }
  while (n);
}

void
SASDetachAllocatedSegs ()
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.allocated;
  void *blockAddr;
  unsigned long keys = 0;

  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr = (void *) n->getInfo ();
	  keys = n->getKey ();
	  if (cnt != 0)
	    {
#ifdef __SASDebugPrint__
	      unsigned long blockSize = logTable[longToSize (keys)];
	      sas_printf ("SASDetachAllocatedSegs segment <%p, %lx>\n",
			  blockAddr, blockSize);
#endif
	      SASDetachSegByAddr (blockAddr, SegmentSize);
	    }
	  cnt++;
	};
    }
  while (n);
}

void
SASRemoveAllocatedSegs ()
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.allocated;
  void *blockAddr;
  unsigned long keys = 0;

  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr = (void *) n->getInfo ();
	  keys = n->getKey ();
	  if (cnt > 0)
	    {			/* What if segments are not contiguous? */
	      if (SASRemoveSegByAddr (blockAddr, SegmentSize))
		sas_printf ("SASRemoveAllocatedSegs:%s Error %s\n",
			    "SASRemoveSegByAddr", strerror (errno));
	    }
	  cnt++;
	};
    }
  while (n);
}

void *
SASBlockAllocNoLock (unsigned long blockSize)
{
  void *temp = NULL;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode **nn = &(anchor->anchors.free);
  uLongTreeNode **uu = &(anchor->anchors.used);

#ifdef __SASDebugPrint__
  sas_printf ("SASBlockAlloc (%lx)\n", blockSize);
#endif

  if (anchor->anchors.free)
    {
      temp = p2Alloc (nn, blockSize);
    }
  if (!temp)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASBlockAlloc no free space for %lx\n", blockSize);
#endif
      nn = &(anchor->anchors.uncommitted);
      temp = p2Alloc (nn, blockSize);
      if (temp)
	{
	  p2AddUsed (uu, blockSize, temp);
	}
      else
	{
	  void *segAddr;
#ifdef __SASDebugPrint__
	  sas_printf ("SASBlockAlloc no uncommitted %lx\n", blockSize);
#endif
	  nn = &(anchor->anchors.region);
	  segAddr = p2Alloc (nn, SegmentSize);
	  if (segAddr)
	    {
	      if (!SASCreateSegByAddr (segAddr, SegmentSize))
		{
		  uu = &(anchor->anchors.allocated);
		  p2AddUsed (uu, SegmentSize, segAddr);
		  nn = &(anchor->anchors.uncommitted);
		  p2Dealloc (nn, SegmentSize, segAddr);
		  temp = SASBlockAllocNoLock (blockSize);
		}
	      else
		{
		  sas_printf ("SASBlockAlloc attach failed %lx\n", blockSize);
		}
	    }
	  else
	    {
	      sas_printf ("SASBlockAlloc no region %lx\n", blockSize);
	    }
	}
    }
  else
    {
      p2AddUsed (uu, blockSize, temp);
    }
  return temp;
}

void *
SASBlockAlloc (unsigned long blockSize)
{
  void *temp = NULL;
#ifdef __SASDebugPrint__
  sas_printf ("SASBlockAlloc (%lx)\n", blockSize);
#endif
  if (blockSize <= SegmentSize)
    {
      SASSeize ();
      temp = SASBlockAllocNoLock (blockSize);
      SASRelease ();
    }
  else
    {
      sas_printf ("SASBlockAlloc blocksize exceeds segment size\n");
    }
  return temp;
}

void
SASBlockDealloc (void *blockAddr, unsigned long blockSize)
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode **nn = &(anchor->anchors.free);
  uLongTreeNode **uu = &(anchor->anchors.used);

  SASSeize ();
  uu = &(anchor->anchors.used);
  p2RemUsed (uu, blockSize, blockAddr);

//  initialize block to zeros, needed only in windows/AIX/OS2
  if (blockSize > 4096)
    memset (blockAddr, 0, 4096);
  else
    memset (blockAddr, 0, blockSize);

  if (sasClearOnDealloc)
    {
      SASRelease ();
      if (blockSize > 4096)
	memset ((char *) blockAddr + 4096, 0, blockSize - 4096);
      SASSeize ();
    }
  nn = &(anchor->anchors.free);
  p2Dealloc (nn, blockSize, blockAddr);
  SASRelease ();
}

int
SASAttachAnchorSeg (void *regionBase, size_t regionSize, size_t segmentSize)
{
  char storeName[STORE_NAME_SIZE];
  int rc = 0;
#ifdef __SASDebugPrint__
  sas_printf ("SASAttachAnchorSeg(%p, %zu, %zu)\n",
	      regionBase, regionSize, segmentSize);
#endif
  if (SASSegIndexExists (0))
    {
      SASSegNameIndexed (storeName, 0);
#ifdef __SASDebugPrint__
      sas_printf ("SASAttachAnchorSeg Anchor file <%s>\n", storeName);
#endif
      rc = SASAttachSegByName ((char *) regionBase, segmentSize,
			       0, storeName, attachFlags);
      if (!rc)
	{
	  memLow = (unsigned long) regionBase;
	  memHigh = memLow + regionSize;
	}
    }
  else
    rc = 1;

  return rc;
}

int
SASCreateAnchorSeg (void *regionBase, size_t regionSize, size_t segmentSize)
{
  char storeName[STORE_NAME_SIZE];
  int rc = 0;
#ifdef __SASDebugPrint__
  sas_printf ("SASCreateAnchorSeg(%p, %zu, %zu)\n",
	      regionBase, regionSize, segmentSize);
#endif
  if (SASSegStoreCreate (0))
    {
      sas_printf ("SASCreateAnchorSeg Failed\n");
      rc = 1;
    }
  else
    {
      SASSegNameIndexed (storeName, 0);
#ifdef __SASDebugPrint__
      sas_printf ("SASCreateAnchorSeg Anchor file <%s>\n", storeName);
#endif
      rc = SASAttachSegByName ((char *) regionBase, segmentSize,
			       0, storeName, createFlags);
      if (!rc)
	{
	  memLow = (unsigned long) regionBase;
	  memHigh = memLow + regionSize;
	}
    }

  return rc;
}



/* Obtain a backtrace and print it to stdout. */
static void
print_trace (void)
{
  void *array[32];
  size_t size;
  char **strings;
  size_t i;

  size = backtrace (array, 32);
  strings = backtrace_symbols (array, size);
  sas_printf ("Obtained %zd stack frames.\n", size);

  for (i = 0; i < size; i++)
    sas_printf ("%s\n", strings[i]);

  free (strings);
}

void SASDisableSigSegv ();

static struct sigaction oldSigSegV;

void
SASSigSegvHandler(int signal, siginfo_t * info, void *context)
{
#ifdef __SASDebugPrint__
	sas_printf ("Signal received: %d\n", signal);
	sas_printf ("si_signo=%d si_code=%d\n", info->si_signo, info->si_code);
#endif
	if (signal == SIGSEGV) {
		unsigned long reg_low = (unsigned long) memLow;
		unsigned long reg_high = (unsigned long) memHigh;
		unsigned long segv_addr = (unsigned long) info->si_addr;
		if ((segv_addr >= reg_low) && (segv_addr < reg_high))
		{
			SASAttachAllocatedAddr(info->si_addr);
		} else {
			if (oldSigSegV.sa_handler != SIG_DFL)
			{
				/* assume the app has its own signal handler.
				 * If so call it and hope it handles the situation */
				if (oldSigSegV.sa_flags & SA_SIGINFO)
				{
					(*oldSigSegV.sa_sigaction)(signal, info, context);
				} else {
					(*oldSigSegV.sa_handler)(signal);
				}
			} else {
				/* Else print an helpful message include back trace
				 * then exit.  We can not continue because the sigsegv
				 * condition persists and we would end up in a
				 * recurring signal loop.
				 */
				void SASDisableSigSegv();
				sas_printf("SIGSEGV@%p\n", info->si_addr);
				print_trace();
				exit (1);
			}
		}
	}
}

void
SASEnableSigSegv ()
{
#if 1
  struct sigaction sa;
  /* register signal handler via sigaction.  
     Remove SA_ONESHOT for the real thing.  */
  sa.sa_flags = SA_SIGINFO | SA_RESTART /* | SA_ONESHOT */ ;
  sa.sa_sigaction = &SASSigSegvHandler;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGSEGV, &sa, &oldSigSegV);
#endif
}

void
SASDisableSigSegv ()
{
#if 1
  int rc;
  rc = sigaction (SIGSEGV, &oldSigSegV, NULL);
  if (rc)
  {
	  perror ("SASDisableSigSegv Failed");
  }
#endif
}

int
SASJoinRegionByName (const char *store_name)
{
  int segs = (int) (RegionSize / SegmentSize);
  size_t memIDsize = (RegionSize / SegmentSize) * sizeof (*mem_IDs);
  int rc = 1;
  int i;

  if (store_name != NULL)
    {
      int pathlen, malloclen;
      pathlen = strlen (store_name);
      malloclen = (pathlen + 2 + 8) & ~(7);
      sasStorePath = (char *) malloc (malloclen);
      sasStorePath = strcpy (sasStorePath, store_name);
      if (sasStorePath[pathlen - 1] == '/')
	sasStorePath[pathlen - 1] = 0;
#ifdef __SASDebugPrint__
      sas_printf ("SASJoinRegionByName (%s)\n", sasStorePath);
#endif
    }
  else
    {				/* Store Name is required for this API */
      return (3);
    }
  mem_IDs = (int *) malloc (memIDsize);
  if (mem_IDs == NULL)
    {
      return (2);
    }
  else
    {
      for (i = 0; i < segs; i++)
	mem_IDs[i] = 0;
    }
  rc = SASAttachAnchorSeg ((char *) __SAS_BASE_ADDRESS,
			   RegionSize, SegmentSize);
  if (rc)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASJoinRegion no anchor found\n");
#endif
      rc = SASCreateAnchorSeg ((char *) __SAS_BASE_ADDRESS,
			       RegionSize, SegmentSize);
      if (!rc)
	{
	  int pgsize = getpagesize ();
#ifdef __SASDebugPrint__
	  sas_printf ("SASJoinRegion anchor created\n");
#endif
	  initRegion ();
	  mmap ((char *) getMemHigh (), pgsize,
		(PROT_READ | PROT_WRITE),
		(MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED), -1, 0);
	};
    }
  else
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASJoinRegion joined existing region\n");
#endif
      SASAttachAllocatedSegs ();
      /* Place guard page to protect Region from main stack. */
      mmap ((char *) getMemHigh (), 4096,
	    (PROT_READ | PROT_WRITE),
	    (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED), -1, 0);
    }

  SASLockInit ();
  SASEnableSigSegv ();

  return rc;
}

int
SASJoinRegion ()
{
  char storeName[STORE_NAME_SIZE];
  int rc = 1;
  char *sas_path;

  sas_path = getenv ("SASSTOREPATH");
  if (sas_path != NULL)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASSTOREPATH <%s>\n", sas_path);
#endif
      rc = SASJoinRegionByName (sas_path);
    }
  else
    {
      sas_path = getcwd (storeName, STORE_NAME_SIZE);
#ifdef __SASDebugPrint__
      sas_printf ("SASSTOREPATH defaults to . <%s>\n", sas_path);
#endif
      rc = SASJoinRegionByName (sas_path);
    }
  return rc;
}

void
SASListFreeRegion (void **blockAddr, unsigned long *blockSize, int *count)
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.region;
  unsigned long keys = 0;

  SASSeize ();
  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr[cnt] = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize[cnt] = logTable[longToSize (keys)];
	  cnt++;
	};
    }
  while (n);
  *count = cnt;
  SASRelease ();
}

void
SASListAllocatedRegion (void **blockAddr,
			unsigned long *blockSize, int *count)
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.allocated;
  unsigned long keys = 0;

  SASSeize ();
  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr[cnt] = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize[cnt] = logTable[longToSize (keys)];
	  cnt++;
	};
    }
  while (n);
  *count = cnt;

  SASRelease ();
}

void
SASListInUseMem (void **blockAddr, unsigned long *blockSize, int *count)
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.used;
  unsigned long keys = 0;

  SASSeize ();
  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr[cnt] = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize[cnt] = logTable[longToSize (keys)];
	  cnt++;
	}
    }
  while (n);
  *count = cnt;
  SASRelease ();
}

void
SASListUncommittedMem (void **blockAddr, unsigned long *blockSize, int *count)
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.uncommitted;
  unsigned long keys = 0;

  SASSeize ();
  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr[cnt] = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize[cnt] = logTable[longToSize (keys)];
	  cnt++;
	};
    }
  while (n);
  *count = cnt;
  SASRelease ();
}

void
SASListFreeMem (void **blockAddr, unsigned long *blockSize, int *count)
{
  int cnt = 0;
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  uLongTreeNode *n;
  uLongTreeNode *u = anchor->anchors.free;
  unsigned long keys = 0;

  SASSeize ();
  do
    {
      n = u->searchNextNode (u, keys);
      if (n)
	{
	  blockAddr[cnt] = (void *) n->getInfo ();
	  keys = n->getKey ();
	  blockSize[cnt] = logTable[longToSize (keys)];
	  cnt++;
	};
    }
  while (n);

  *count = cnt;
  SASRelease ();
}

unsigned int
SASAnchorFreeSpace ()
{
  SASBlockHeader *anchorBlock = (SASBlockHeader *) memLow;
  unsigned int temp;
  SASSeize ();
  temp = freeNode_freeSpaceTotal (anchorBlock->blockFreeSpace);
  SASRelease ();

  return temp;
}

void
SASSeize ()
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  seizeSASSem (&anchor->anchors);
}

void
SASRelease ()
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  releaseSASSem (&anchor->anchors);
}

void
SASResetSem ()
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
  destroySASSem (&anchor->anchors);
  initSASSem (&anchor->anchors);
}

void
SASReset ()
{
  SASSeize ();
  SASRemoveAllocatedSegs ();
  initRegion ();
  SASLockReset ();
  SASRelease ();
}

void
SASRemove ()
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
#ifdef __SASDebugPrint__
  sas_printf ("SASRemove()\n");
#endif
  SASRemoveAllocatedSegs ();
  if (SASRemoveSegByAddr (anchor, SegmentSize))
    sas_printf ("SASRemove: SASRemoveSegByAddr failed\n");
  SASLockReset ();
  SASLockRemove ();
  destroySASSem (&anchor->anchors);
  free (mem_IDs);
}

static __thread struct sigaction oldThreadSigSegV;

void
SASThreadDisableSigSegv ()
{
#if 1
  int rc;
  rc = sigaction (SIGSEGV, &oldThreadSigSegV, NULL);
  if (rc)
  {
	  perror ("SASThreadDisableSigSegv Failed");
  }
#endif
}

void
SASThreadSigSegvHandler(int signal, siginfo_t * info, void *context)
{
#ifdef __SASDebugPrint__
	sas_printf ("Signal received: %d\n", signal);
	sas_printf ("si_signo=%d si_code=%d\n", info->si_signo, info->si_code);
#endif
	if (signal == SIGSEGV)
	{
		unsigned long reg_low = (unsigned long) memLow;
		unsigned long reg_high = (unsigned long) memHigh;
		unsigned long segv_addr = (unsigned long) info->si_addr;
		if ((segv_addr >= reg_low) && (segv_addr < reg_high)) {
			SASAttachAllocatedAddr(info->si_addr);
		} else {
			if (oldThreadSigSegV.sa_handler != SIG_DFL)
			{
				/* assume the app has its own signal handler.
				 * If so call it and hope it handles the situation */
				if (oldThreadSigSegV.sa_flags & SA_SIGINFO)
				{
					(*oldThreadSigSegV.sa_sigaction)(signal, info, context);
				} else {
					(*oldThreadSigSegV.sa_handler)(signal);
				}
			} else {
				/* Else print an helpful message include back trace
				 * then exit.  We can not continue because the sigsegv
				 * condition persists and we would end up in a
				 * recurring signal loop.
				 */
				void SASThreadDisableSigSegv();
				sas_printf("SIGSEGV@%p\n", info->si_addr);
				print_trace();
				pthread_exit ((void*)1);
			}
		}
	}
}

void
SASThreadEnableSigSegv ()
{
#if 1
  struct sigaction sa;
  /* register signal handler via sigaction.
     Remove SA_ONESHOT for the real thing.  */
  sa.sa_flags = SA_SIGINFO | SA_RESTART /* | SA_ONESHOT */ ;
  sa.sa_sigaction = &SASThreadSigSegvHandler;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGSEGV, &sa, &oldThreadSigSegV);
#endif
}

void
SASThreadSetUp ()
{
  SASThreadEnableSigSegv ();
}

void
SASThreadCleanUp ()
{
  SASThreadDisableSigSegv ();
}

void
SASCleanUp ()
{
  SASAnchorBlock_t *anchor = (SASAnchorBlock_t *) memLow;
#ifdef __SASDebugPrint__
  sas_printf ("SASCleanUp()\n");
#endif

  SASDetachAllocatedSegs ();
  if (SASDetachSegByAddr (anchor, SegmentSize))
    {
      sas_printf ("SASCleanUp: SASDetachSegByAddr failed\n");
    }
  SASLockDetach ();
  SASDisableSigSegv ();
  munmap ((char *) getMemHigh (), 4096);

  free (mem_IDs);
  mem_IDs = NULL;
  free (sasStorePath);
  sasStorePath = NULL;
}
