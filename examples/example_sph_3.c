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

#include <sphde/sassim.h>
#include <sphde/saslock.h>
#include <sphde/sasalloc.h>
#include <sphde/sasstringbtree.h>

static const char progName[] = "example_sph_3";

/*
 * This example shows some SAS B-Tree usage as element insertion, remotion,
 * replace and how to walk over a SAS B-Tree and prints its internal
 * elements.
 */

static inline void
sasbtree_print_node (SASStringBTreeNode_t node)
{
  int i, n;
  n = SASStringBTreeNodeGetCount (node);
  for (i = 0; i <= n; ++i)
    {
      SASStringBTreeNode_t br;
      const char *key = SASStringBTreeNodeGetKeyIndexed (node, i);
      if (!key)
	continue;
      printf ("(%s : %s)", key,
	      (char *) SASStringBTreeNodeGetValIndexed (node, i));
      if ((br = SASStringBTreeNodeGetBranchIndexed (node, i)))
	sasbtree_print_node (br);
      if (i != n)
	printf (", ");
    }
}

static inline void
sasbtree_print (SASStringBTree_t btree)
{
  SASStringBTreeNode_t root;

  SASLock (btree, SasUserLock__READ);
  root = SASStringBTreeGetRootNodeNoLock (btree);
  if (root)
    {
      printf ("{");
      sasbtree_print_node (root);
      printf ("}");
    }
  SASUnlock (btree);
  printf ("\n");
}

static inline void
sasbtree_add_element (SASStringBTree_t btree, const char *k, const char *v)
{
  int rc = SASStringBTreePut (btree, (char *) k, (void *) v);
  if (rc == 0)
    {
      fprintf (stderr, "%s: error: SASStringBTreePut(%p, %s, %s) failed\n",
	       progName, btree, k, v);
      exit (EXIT_FAILURE);
    }
}

static inline void
sasbtree_remove_element (SASStringBTree_t btree, const char *k)
{
  void *rc = SASStringBTreeRemove (btree, (char *) k);
  if (rc == 0)
    {
      fprintf (stderr, "%s: error: SASStringBTreeRemove(%p, %s) failed\n",
	       progName, btree, k);
      exit (EXIT_FAILURE);
    }
}

static inline void
sasbtree_replace_element (SASStringBTree_t btree, const char *k,
			  const char *v)
{
  void *rc = SASStringBTreeReplace (btree, (char *) k, (char *) v);
  if (rc == 0)
    {
      fprintf (stderr,
	       "%s: error: SASStringBTreeReplace(%p, %s, %s) failed\n",
	       progName, btree, k, v);
      exit (EXIT_FAILURE);
    }
}

int
main ()
{
  SASStringBTree_t btree;
  int rc;

  rc = SASJoinRegion ();
  if (rc != 0)
    {
      fprintf (stderr, "%s: error: SASJoinRegion failed: %d\n", progName, rc);
      exit (EXIT_FAILURE);
    }

  btree = SASStringBTreeCreate (block__Size256K);
  if (btree == 0)
    {
      fprintf (stderr, "%s: error: SASStringBTreeCreate(%li) failed\n",
	       progName, block__Size256K);
      exit (EXIT_FAILURE);
    }

  sasbtree_add_element (btree, "a", "a");
  sasbtree_add_element (btree, "b", "b");
  sasbtree_add_element (btree, "c", "c");
  sasbtree_add_element (btree, "d", "d");
  sasbtree_add_element (btree, "e", "e");

  sasbtree_print (btree);

  sasbtree_remove_element (btree, "a");
  sasbtree_remove_element (btree, "c");

  sasbtree_print (btree);

  sasbtree_replace_element (btree, "b", "2");
  sasbtree_replace_element (btree, "d", "4");
  sasbtree_replace_element (btree, "e", "5");

  sasbtree_print (btree);

  SASCleanUp ();

  return 0;
}
