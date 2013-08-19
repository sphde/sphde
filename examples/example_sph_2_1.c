/*
 * Copyright (c) 2003, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Adhemerval Zanella - Example creation.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sphde/sassim.h>
#include <sphde/sassimpleheap.h>
#include <sphde/sasalloc.h>

/*
 * This example shows how to use the SimpleHeap object and how to allocate 
 * chunks of memory within the object.
 * It first creates a simple heap, then allocates a vector to hold references
 * to multiple chunks of memory. 
 *
 * The SimpleHeap object does not contain any facility to associates chunks of
 * memory with other reference (like strings) for further reference, so it
 * is needed to add aditional logic to do so (like the vector of references 
 * used in this example).
 */

const char progName[] = "example_sph_2_1";

const block_size_t kHeapSize  = block__Size1M;
const block_size_t kBlockSize = 16384;

int main()
{
  SASSimpleHeap_t heap;
  void **blockDescriptor;
  size_t blockDescriptorCount;
  int rc, i;

  /* Init a SPHDE region: create and attach the control shared memory 
   * segment and locks. */
  rc = SASJoinRegion();
  if (rc != 0) {
    fprintf(stderr, "%s: error: SASJoinRegion: %d\n", progName, rc);
    exit(EXIT_FAILURE);
  }

  /* Allocate a block of SAS storage for the vector descriptors that point to
   * the heap allocations.  Since we'll be allocating 1MB of SimpleHeap and a
   * SASSimpleHeap_t allocates a control header of size 'heap_offset' use the
   * following to determine the necessary storage for the vector descriptor
   * block:
   * 
   * (1MB - heap_offset)/16KB == 63 entries
   *   32 bits -> 63*sizeof(void*) == 252 bytes 
   *   64 bits -> 63*sizeof(void*) == 504 bytes 
   * So it used a block of 512 to work on both 32 and 64 bits */
  blockDescriptor = SASBlockAlloc(block__Size512);
  printf("%s: SASBlockAlloc(%lu) ", progName, block__Size512);
  if (!blockDescriptor) {
    printf("failed\n");
    exit(EXIT_FAILURE);
  }
  printf("succeed: %p\n", blockDescriptor);

  heap = SASSimpleHeapCreate(kHeapSize);
  printf("%s: SASSimpleHeapCreate(%zu) ", progName, kHeapSize);
  if (!heap) {
    printf("failed\n");
    exit(EXIT_FAILURE);
  }
  printf("succeed: %p\n", heap);

  /* And finally allocates each memory chunk. */
  blockDescriptorCount = (block__Size1M - heap_offset) /
			  (sizeof(void *) + kBlockSize);
  for (i=0; i<blockDescriptorCount; ++i) {
    blockDescriptor[i] = SASSimpleHeapAlloc(heap, kBlockSize);
    if (!blockDescriptor[i]) {
      printf("%s: blockDescriptor[%d] = SASSimpleHeapAlloc(%p, %zu) failed\n",
	     progName, i, heap, kBlockSize);
      exit(EXIT_FAILURE);
    }
    memset(blockDescriptor[i], i, kBlockSize);
  }
  printf("%s: SASSimpleHeapFreeSpace(%p): %zu\n", progName, heap,
         SASSimpleHeapFreeSpace(heap));

  SASSimpleHeapDestroy(heap);
  printf("%s: SASSimpleHeapDestroy(%p)\n", progName, heap);

  SASBlockDealloc(blockDescriptor, block__Size512);
  printf("%s: SASBlockDealloc(%p, %lu)\n", progName, blockDescriptor,
	 block__Size512);

  SASCleanUp();

  return 0;
}
