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
#include <sphde/sphcontext.h>
#include <sphde/sasalloc.h>

/*
 * This example is similar to 'example_sph_2_1": it uses a SimpleHeap object
 * and allocates chunks on memory within the object. The difference is that it
 * now adds persistence by using the SphContext to retrieve the references to
 * the allocated memory over multiples runs of the program.
 *
 * The SPHContext_t object is used to create an association between a string
 * and a shared memory address (in this case, the vector descriptor). Then the
 * association is used to retrieve the created shared heap and check its
 * contents.
 */

const char progName[] = "example_sph_2_2";

/* Identifier used to retried block descriptor address */
const char blockName[] = "blockDescriptor";

const block_size_t kHeapSize  = block__Size1M;
const block_size_t kBlockSize = 16384;


/*
 * This function creates the shared heap with multiple chunks of memory
 * inside. */
void **
createHeap(size_t blockDescriptorCount)
{
  SASSimpleHeap_t heap;
  void **blockDescriptor;
  int i;

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

  for (i=0; i<blockDescriptorCount; ++i) {
    blockDescriptor[i] = SASSimpleHeapAlloc(heap, kBlockSize);
    if (!blockDescriptor[i]) {
      printf("%s: blockDescriptor[%d] = SASSimpleHeapAlloc(%p, %zu) failed\n",
	     progName, i, heap, kBlockSize);
      exit(EXIT_FAILURE);
    }
    // this will be used to check value across multiple program invocations
    memset(blockDescriptor[i], i, kBlockSize);
  }
  printf("%s: SASSimpleHeapFreeSpace(%p): %zu\n", progName, heap, 
	 SASSimpleHeapFreeSpace(heap));

  return blockDescriptor;
}

int main()
{
  SPHContext_t finderContext;
  void **blockDescriptor;
  size_t blockDescriptorCount;
  char *block;
  int rc, i;

  /* Init a SPHDE region: create and attach the control shared memory 
   * segment and locks. */
  rc = SASJoinRegion();
  if (rc != 0) {
    fprintf(stderr, "%s: error: SASJoinRegion: %d\n", progName, rc);
    exit(EXIT_FAILURE);
  }

  /* The Context memory block containing the key/pair associations. If
   * it does not exist, create one. */
  block = getSASFinder();
  if (block != NULL) {
    finderContext = (SPHContext_t)block;
  } else {
    finderContext = SPHContextCreate(block__Size256K);
    printf("%s: SPHContextCreate(%lu): %p\n", progName, block__Size256K,
	   finderContext);
    setSASFinder(finderContext);
  }
  printf("%s: SPHContext_t: %p\n", progName, finderContext);

  /* Find the block descriptor using a string as reference. */
  blockDescriptorCount = (block__Size1M - heap_offset) /
			  (sizeof(void *) + kBlockSize);
  blockDescriptor = (void **)SPHContextFindByName(finderContext, blockName);
  printf("%s: SPHContextFindByName(%p, %s): %p\n", progName, finderContext,
	 blockName, blockDescriptor);
  if (!blockDescriptor) {
    blockDescriptor = createHeap(blockDescriptorCount);
    SPHContextAddName(finderContext, blockName, blockDescriptor);
  }

  /* Finally check the memory chunk values. */
  for (i=0; i<blockDescriptorCount; ++i) {
    char *ptr = blockDescriptor[i];
    if (*ptr != i) {
      fprintf(stderr, "%s: error: \n", progName);
    }
  }

  SASCleanUp();

  return 0;
}
