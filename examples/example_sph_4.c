/*
 * Copyright (c) 2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steve Munroe - Example creation.
 *     IBM Corporation, Rajalakshmi S- Example creation.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sphde/sassim.h>
#include <sphde/sassimpleheap.h>
#include <sphde/sascompoundheap.h>
#include <sphde/sphcontext.h>
#include <sphde/sasalloc.h>
#include <sphde/sasindex.h>
#include <sphde/saslock.h>
#include <sphde/sasstringbtree.h>
#include <sphde/sasstdio.h>
#include <sphde/sasindexkey.h>
#include <sphde/sasindexenum.h>
#include <sphde/sasindexpriv.h>
 /*
 * This example shows how to use the set up different projects and how to add user data
 * within the project. The SPHContext_t object is used to create an association between a
 * person info (stored in a structure) and a shared memory address.
 * This adds persistence by using the SphContext to retrieve the references to
 * the allocated memory over multiples runs of the program.
 *
 * Each person record has its own SPHContext_t allocated from a SASCompoundHeap_t
 * and resolves to struct record which contains the
 * name, address, phone number birthday etc of the persons.
 *
 * This examples also shows how to add multiple SASStringBTree_t and SASIndex_t
 * searches over the Person records to allow search on phone number birthday, etc.
 */

const char progName[] = "example_sph_4";
SASCompoundHeap_t compoundHeap;
void
Update_Project (SPHContext_t context, int count, void *lastobj,
		SASStringBTree_t btree, SASIndex_t index);

void Print_Project (char *name, int count);

#define NTEXTMAX 64
struct emp_info
{
  char name[NTEXTMAX];
  char address[NTEXTMAX];
  long phone;
  char birthday[NTEXTMAX];
} emp_info_t;

/* Sample person information */
char names[20][20] = { "John", "James", "Jose", "Alice", "Tony",
  "David", "Joseph", "Paul", "Mark", "Mary",
  "Nancy", "Philip", "Frank", "Thomas", "Charls",
  "Laura", "Sarah", "King", "Harris", "Lisa"
};

char address[20][20] = {
  "Alabama", "Alaska", "Arkansas", "California", "Delaware",
  "Florida ", "Georgia ", "Idaho   ", "Illinois", "Indiana ",
  "Iowa   ", "Kentucky", "Louisiana ", "Maine  ", "Maryland",
  "Massachusetts", "Michigan", "Mississippi", "Missouri", "Nevada"
};

long phone[20] = { 12345, 67890, 45345, 45456, 67765,
  89879, 32432, 246767, 676557, 45435,
  68493, 43859, 458943, 4305983, 450943,
  457843, 64543, 67767, 65867, 34234
};

char bday[20][20] = {
  "01/01/1986", "02/02/1987", "03/03/1988", "04/04/1989", "05/05/1990",
  "05/06/1991", "06/01/1992", "07/04/1993", "07/07/1994", "09/08/1986",
  "09/08/1991", "10/08/1992", "11/03/1993", "12/04/1994", "01/05/1986",
  "02/06/1987", "03/07/1988", "05/04/1988", "08/05/1989", "08/07/1981"
};
char indexes[2][10] = { "sasindex", "sasstring" };

/* This function creates the shared heap */
SASSimpleHeap_t *
CreateHeap ()
{
  SASSimpleHeap_t simpleHeap;
  unsigned long blockSize = block__Size64K;
  compoundHeap = SASCompoundHeapCreate (blockSize);
  if (!compoundHeap)
    {
      fprintf (stderr, "SASCompoundHeapCreate(%ld)", blockSize);
      SASCleanUp ();
      exit (EXIT_FAILURE);
    }
  simpleHeap = SASCompoundHeapAlloc (compoundHeap);
  if (!simpleHeap)
    {
      fprintf (stderr, "SASCompoundHeapAlloc(%p)", compoundHeap);
      SASCleanUp ();
      exit (EXIT_FAILURE);
    }
  return simpleHeap;
}

static inline void
sasindex_add_element (SASIndex_t index, long k, const char *v)
{
  SASIndexKey_t key;
  int rc;

  SASIndexKeyInitRef (&key, (void *) k);
  rc = SASIndexPut (index, &key, (void *) v);
  if (rc == 0)
    {
      fprintf (stderr, "%s: SASIndexPut(%p, %ld, %s) failed\n", progName,
	       index, k, v);
      exit (EXIT_FAILURE);
    }
}

/* Functions to print project info */
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
      printf ("(%d : %s)", (char) key->data[0],
	      (char *) SASIndexNodeGetValIndexed (node, i));
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

int
main ()
{
  SPHContext_t context, context1;
  SASIndexKey_t key;
  SASSimpleHeap_t simpleHeap;
  SASStringBTree_t btree;
  SASIndex_t index;
  int i, rc;
  char *tmp_name;

  /* Init a SPHDE region: create and attach the control shared memory
   * segment and locks. */
  rc = SASJoinRegion ();

  if (rc != 0)
    {
      fprintf (stderr, "%s: error: SASJoinRegion: %d\n", progName, rc);
      exit (EXIT_FAILURE);
    }
  /* The Context memory block containing the key/pair associations. If
   * it does not exist, create one. */
  context = getProjectContextByName ("Project_B");
  if (context == NULL)
    {
      /* Setup first project */
      context = SPHSetupProjectContext ("Project_A");
      if (!context)
	{
	  fprintf (stderr, "SPHSetupProjectContext () failed");
	  SASCleanUp ();
	  exit (EXIT_FAILURE);
	}
      printf ("Created project (Project_A) at %p\n", context);

      /* Setup second project */
      context1 = SPHSetupProjectContext ("Project_B");
      if (!context1)
	{
	  fprintf (stderr, "SPHSetupProjectContext () failed");
	  SASCleanUp ();
	  exit (EXIT_FAILURE);
	}
      printf ("Created project (Project_B) at %p\n", context1);

      /* Add indexes info to context */
      /* This will be retrieved later for searching */
      index = SASIndexCreate (block__Size64K);
      if (index == 0)
	{
	  fprintf (stderr, "%s: SASIndexCreate(%li) failed\n", progName,
		   block__Size64K);
	  exit (EXIT_FAILURE);
	}
      rc = SPHContextAddName (context, indexes[0], (SASIndex_t *) index);
      if (!rc)
	{
	  fprintf (stderr, "SPHContextAddName() failed: %i\n", rc);
	  SASCleanUp ();
	  exit (1);
	}
      btree = SASStringBTreeCreate (block__Size64K);
      if (btree == 0)
	{
	  fprintf (stderr, "%s: error: SASStringBTreeCreate(%li) failed\n",
		   progName, block__Size64K);
	  exit (EXIT_FAILURE);
	}
      rc =
	SPHContextAddName (context, indexes[1], (SASStringBTree_t *) btree);
      if (!rc)
	{
	  fprintf (stderr, "SPHContextAddName() failed: %i\n", rc);
	  SASCleanUp ();
	  exit (1);
	}
      /* Allocate a new simple heap from compound heap */
      simpleHeap = CreateHeap ();
      /* Fill in project information */
      Update_Project (context, 0, simpleHeap, btree, index);
      /* 
       * Allocate a new simple heap from same compound heap.
       * This  insures that ProjectA and ProjectB are separated
       */
      simpleHeap = SASCompoundHeapNearAlloc (compoundHeap);
      /* Fill in project information */
      Update_Project (context1, 10, simpleHeap, btree, index);
    }

  /* Examples to access persistent data */
  /* First retrieve the indexes from project context */
  context = getProjectContextByName ("Project_A");
  index = SPHContextFindByName (context, indexes[0]);
  sasindex_print (index);
  btree = SPHContextFindByName (context, indexes[1]);
  sasbtree_print (btree);
  /* Using SASIndexGet() to search project info */
  printf ("Name\tPhone\tAddress\t\tBirthday\n");
  printf ("---------------------------------------\n");
  for (i = 0; i < 20; i++)
    {
      SASIndexKeyInitRef (&key, (void *) phone[i]);
      tmp_name = (char *) SASIndexGet (index, &key);
      printf ("%s\t%ld\t%s\t%s\n", tmp_name, phone[i],
	      (char *) SASStringBTreeGet (btree, tmp_name),
	      (char *) SASStringBTreeGet (btree,
              (char *) SASStringBTreeGet (btree, tmp_name)));
    }
  /* print project info using SPHContextFindByName/SPHContextFindByAddr */
  Print_Project ("Project_A", 0);
  Print_Project ("Project_B", 10);

  SASCleanUp ();

  return 0;
}

/* Function to update  project info */
void
Update_Project (SPHContext_t context, int count, void *lastobj,
		SASStringBTree_t btree, SASIndex_t index)
{
  int i, rc;
  SASSimpleHeap_t simpleHeap;
  struct emp_info *emp_info_t[count];
  for (i = count; i < count + 10; i++)
    {
      /* 
       * Allocate a data block from "the" SASSimpleHeap_t
       * that contains the address lastdata. This ensures
       * objects from same project are grouped together
       */
      emp_info_t[i] =
	(struct emp_info *) SASSimpleHeapNearAlloc (lastobj,
						sizeof (struct emp_info));
      if (emp_info_t[i] == NULL)
	{
          /* 
          * Expand:Allocate a data block from same SASCompoundHeap_t
          * but a different SASSimpleHeap_t, that contains the address lastdata
          */
	  simpleHeap = SASCompoundHeapNearAlloc (lastobj);
	  if (!simpleHeap)
	    {
	      fprintf (stderr, "SASCompoundHeapNearAlloc(%p)", compoundHeap);
	      SASCleanUp ();
	      exit (EXIT_FAILURE);
	    }
          lastobj = simpleHeap;
	  emp_info_t[i] =
	    (struct emp_info *) SASSimpleHeapNearAlloc (simpleHeap,
						    sizeof (struct emp_info));
	  if (emp_info_t[i] == NULL)
	    {
	      fprintf (stderr, "SASSimpleHeapAlloc() failed\n");
	      SASCleanUp ();
	      exit (1);
	    }
	}
      strcpy (emp_info_t[i]->name, names[i]);
      strcpy (emp_info_t[i]->address, address[i]);
      emp_info_t[i]->phone = phone[i];
      strcpy (emp_info_t[i]->birthday, bday[i]);
      rc = SPHContextAddName (context, names[i], emp_info_t[i]);
      if (!rc)
	{
	  fprintf (stderr, "SPHContextAddName() failed: %i\n", rc);
	  SASCleanUp ();
	  exit (1);
	}
      sasindex_add_element (index, phone[i], names[i]);
      sasbtree_add_element (btree, names[i], address[i]);
      sasbtree_add_element (btree, address[i], bday[i]);

    }
}

/* Function to print project info */
void
Print_Project (char *name, int count)
{
  struct emp_info *emp_info_tmp;
  int i;
  SPHContext_t context = getProjectContextByName (name);

  printf ("\nAccessing Project '%s'\n", name);
  printf ("------------------------------\n");
  printf ("Name\tPhone\tAddress\t\tBirthday\n");
  printf ("---------------------------------------\n");
  /* Switch to next project */
  for (i = count; i < count + 10; i++)
    {
      emp_info_tmp =
	(struct emp_info *) SPHContextFindByName (context, names[i]);
      printf ("%s\t%ld\t%s\t%s\n",
	      (char *) SPHContextFindByAddr (context, emp_info_tmp),
	      emp_info_tmp->phone, emp_info_tmp->address,
	      emp_info_tmp->birthday);
    }
}
