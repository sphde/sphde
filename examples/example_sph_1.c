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
#include <stdlib.h>
#include <unistd.h>

#include <sphde/sassim.h>
#include <sphde/sphcontext.h>
#include <sphde/sasalloc.h>

/*
 * This is a simple example to show how to use a Shared Persistent Heap (SPH)
 * across multiple executions.
 * The program handles multiple pairs of strings, similar to a dictionary. The
 * following commands show its functionality with examples:
 *
 * # associates the string 'test1' with default key ('defaultBufferKey')
 * ./example_sph_1 -w "test1"
 *
 * # reads the written value on default key
 * ./example_sph_1 -r
 *
 * # associates the string 'test2' with key 'key2'
 * ./example_sph_1 -b key2 -w "test2"
 *
 * # reads the value for the key 'test2'
 * ./example_sph_1 -b key2 -r
 */

const char progName[]       = "example_sph_1";
const char defaultKeyName[] = "defaultBufferKey";

void
usage(void)
{
  printf("Usage: %s [-b buffer_name] [-w string_to_write] [-r]\n", progName);
}

int
main (int argc, char *argv[])
{
  int rc;
  char *block;
  SPHContext_t finderContext;
  int actWrite = 0, actRead = 0;
  const char *bufferKeyName = defaultKeyName;
  const char *actToWrite = NULL;
  int opt;

  while ((opt = getopt(argc, argv, "b:w:rh")) != -1) {
    switch (opt) {
      case 'b':
        bufferKeyName = optarg;
        break;
      case 'w':
        actWrite = 1;
        actToWrite = optarg;
        break;
      case 'r':
        actRead = 1;
        break;
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
      default:
        usage();
        exit(EXIT_FAILURE);
    }
  }

  if (!actWrite && !actRead) {
    fprintf(stderr, "Error: '-w' of '-r' must be used\n");
    exit(EXIT_FAILURE);
  }

  if (actWrite && actRead) {
    fprintf(stderr, "Error: '-w' and '-r' can not be use at same time\n");
    exit(EXIT_FAILURE);
  }
 
  /* Init a SPHDE region: create and attach the control shared memory 
   * segment and locks. */
  rc = SASJoinRegion();
  if (rc != 0) {
    fprintf(stderr, "error: SASJoinRegion: %d\n", rc);
    exit(EXIT_FAILURE);
  }

  /* The Context memory block containing the key/pair associations. If
   * it does not exist, create one. */
  block = getSASFinder();
  if (block != NULL) {
    finderContext = (SPHContext_t)block;
  } else {
    finderContext = SPHContextCreate(block__Size256K);
    setSASFinder(finderContext);
  }

  /* Try to get the block by a name, if it does not exists creates it */
  block = (char*)SPHContextFindByName(finderContext, bufferKeyName);
  if (!block) {
    block = (char*)SASBlockAlloc(block__Size4K);
    if (block == NULL) {
      fprintf(stderr, "error: SASBlockAlloc\n");
      SASCleanUp();
      exit(EXIT_FAILURE);
    }

    rc = SPHContextAddName(finderContext, bufferKeyName, block);
    if (!rc) {
      fprintf(stderr, "SPHContextAddName(%p, %s, %p) failed: %i\n",
        finderContext, bufferKeyName, block, rc);
      SASCleanUp();
      exit(1);
    }
  }

  if (actRead) {
    printf("Reading block '%s' : %s\n", bufferKeyName, block);
  } else {
    printf("Writing block '%s' : %s\n", bufferKeyName, block);
    snprintf(block, block__Size256K, "%s", actToWrite);
  }

  SASCleanUp();

  return 0;
}
