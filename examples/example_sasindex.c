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

#include <sphde/sassim.h>
#include <sphde/saslock.h>
#include <sphde/sasalloc.h>
#include <sphde/sasindex.h>
#include <sphde/sasindexenum.h>

static const char progName[] = "example_sasindex";

/*
 * This example shows some SAS Index usage and how to prints its keys and
 * values. Two ways of printing the SAS index are presented:
 * 1. Using the SASIndexNode functions to access the SASIndex_t index 
 * as a tree structure.
 * 2. Using the SASIndexEnum functions to provide a simpler API to access
 * the SASIndex.
 */

static inline void
sasindex_add_element (SASIndex_t index, int k, const char *v)
{
  SASIndexKey_t key;
  int rc;

  SASIndexKeyInitRef (&key, (void*) k);
  rc = SASIndexPut (index, &key, (void*) v);
  if (rc == 0)
    {
      fprintf (stderr, "%s: SASIndexPut(%p, %c, %s) failed\n", progName, 
	       index, k, v);
      exit (EXIT_FAILURE);
    }
  printf("%s: SASIndexPut (%c, %s)\n", progName, k, v);
}

static inline void
sasindex_print_node (SASIndexNode_t node)
{
  int i, n;

  n = SASIndexNodeGetCount (node);
  for (i = 0; i <= n; ++i)
    {
      SASIndexNode_t br;
      SASIndexKey_t *key;

      key = SASIndexNodeGetKeyIndexed (node, i);
      if (!key)
        continue;
      printf ("(%c : %s)", (char)key->data[0],
	     (char*) SASIndexNodeGetValIndexed (node, i));
      if ((br = SASIndexNodeGetBranchIndexed (node, i)))
	sasindex_print_node (br);
      if (i != n)
	printf (", ");
    }
}

static inline void
sasindex_print (SASIndex_t index)
{
  SASIndexNode_t root;

  root = SASIndexGetRootNode (index);
  if (root)
    {
      printf ("%s: {", progName);
      sasindex_print_node (root);
      printf ("}");
    }
  else
    {
      printf ("%s: { empty }", progName);
    }
  printf ("\n");
}

static inline void
sasindex_print_enum (SASIndex_t index)
{
  SASIndexEnum_t ndxenum;

  ndxenum = SASIndexEnumCreate (index);
  printf ("%s: ", progName);
  if (!ndxenum)
    {
      printf ("{ empty }\n");
      return;
    }
  printf ("{");
  while (SASIndexEnumHasMore (ndxenum))
    {
      void *val = SASIndexEnumNext (ndxenum);
      if (!val)
	break;
      printf ("(%s), ", (char*) val);
    }
  SASIndexEnumDestroy (ndxenum);

  printf ("}\n");
}

int
main (int argc, char *argv[])
{
  SASIndex_t index;
  int rc;

  rc = SASJoinRegion();
  if (rc != 0)
    {
      fprintf (stderr, "%s: SASJoinRegion failed: %d\n", progName, rc);
      exit (EXIT_FAILURE);
    }

  index = SASIndexCreate (block__Size256K);
  if (index == 0)
    {
      fprintf (stderr, "%s: SASIndexCreate(%li) failed\n", progName,
	       block__Size256K);
      exit (EXIT_FAILURE);
    }

  sasindex_add_element (index, 'a', "1");
  sasindex_add_element (index, 'b', "2");
  sasindex_add_element (index, 'c', "3");

  sasindex_print (index);
  sasindex_print_enum (index);

  SASCleanUp ();

  return 0;
}
