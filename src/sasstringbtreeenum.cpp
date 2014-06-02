/*
 * Copyright (c) 2005, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include <string.h>
#include "sasalloc.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sasstringbtree.h"
#include "sasstringbtreepriv.h"
#include "sasstringbtreenodepriv.h"
#include "sasstringbtreeenum.h"

typedef struct __SBEnumeration
{
  SASStringBTree_t tree;
  char *curkey;
  SBTnodePosRef ref;
  long curmod;
  long curcount;
  bool hasmore;
  char entryString[sizeof (void *)];
} __SBEnumeration;

typedef void *SASStringBTreeEnum_t;

SASStringBTreeEnum_t
SASStringBTreeEnumCreate (SASStringBTree_t tree)
{
  __SBEnumeration *stringenum = NULL;
  stringenum = (__SBEnumeration *) malloc (sizeof (__SBEnumeration));
  if (stringenum != NULL)
    {
      stringenum->curmod = SASStringBTreeGetModCount (tree);
      stringenum->curcount = SASStringBTreeGetCurCount (tree);
      if (stringenum->curmod != 0L)
	{
	  stringenum->hasmore = !SASStringBTreeIsEmpty (tree);
	  stringenum->tree = tree;
	  stringenum->ref.node = NULL;
	  stringenum->ref.pos = 0;
	  stringenum->entryString[0] = '\0';
	  stringenum->curkey = &stringenum->entryString[0];
	}
      else
	{
	  free (stringenum);
	  stringenum = NULL;
	}
    }
  else
    {
    }

  return (SASStringBTreeEnum_t) stringenum;
}

SASStringBTreeEnum_t
SASStringBTreeEnumCreateStartAt (SASStringBTree_t tree, char *start_key)
{
  __SBEnumeration *stringenum = NULL;
  char *maxkey;
  size_t startkey_len = strlen (start_key) + 1;

  stringenum =
    (__SBEnumeration *) malloc (sizeof (__SBEnumeration) + startkey_len);
  if (stringenum != NULL)
    {
      SASLock (tree, SasUserLock__WRITE);
      maxkey = SASStringBTreeGetMaxKey (tree);
      if ((maxkey != NULL) && (strcmp (start_key, maxkey) < 0))
	{
	  stringenum->curmod = SASStringBTreeGetModCount (tree);
	  stringenum->curcount = SASStringBTreeGetCurCount (tree);
	  if (stringenum->curmod != 0L)
	    {
	      stringenum->hasmore = (strcmp (start_key, maxkey) < 0);
	      stringenum->tree = tree;
	      stringenum->ref.node = NULL;
	      stringenum->ref.pos = 0;
	      memcpy ((char *) &stringenum->entryString[0],
		      start_key, startkey_len);
	      if (stringenum->entryString[0] != '\0')
		stringenum->entryString[startkey_len - 2] -= 1;
	      stringenum->curkey = &stringenum->entryString[0];
	    }
	  else
	    {
	      free (stringenum);
	      stringenum = NULL;
	    }
	}
      else
	{
	  free (stringenum);
	  stringenum = NULL;
	}
      SASUnlock (tree);
    }

  return (SASStringBTreeEnum_t) stringenum;
}

void
SASStringBTreeEnumDestroy (SASStringBTreeEnum_t stringenum)
{
  free (stringenum);
}

int
SASStringBTreeEnumHasMore (SASStringBTreeEnum_t sbtenum)
{
  __SBEnumeration *stringenum = (__SBEnumeration *) sbtenum;
  return stringenum->hasmore;
}

long
SASStringBTreeEnumCount (SASStringBTreeEnum_t sbtenum)
{
  __SBEnumeration *stringenum = (__SBEnumeration *) sbtenum;
  return stringenum->curcount;
}

char *
SASStringBTreeEnumCurrent (SASStringBTreeEnum_t sbtenum)
{
  __SBEnumeration *stringenum = (__SBEnumeration *) sbtenum;
  return stringenum->curkey;
}

void *
SASStringBTreeEnumNext (SASStringBTreeEnum_t sbtenum)
{
  __SBEnumeration *stringenum = (__SBEnumeration *) sbtenum;
  void *result = NULL;
  bool found = false;
  char *maxkey;


  SASLock (stringenum->tree, SasUserLock__WRITE);
  maxkey = SASStringBTreeGetMaxKey (stringenum->tree);
#if __SASDebugPrint__ > 1
  sas_printf ("SASStringBTreeEnumNext; enum->tree=%p maxkey=%s\n",
                  stringenum->tree, maxkey);
#endif
  if (maxkey != NULL)
    {
      stringenum->hasmore = (strcmp (stringenum->curkey, maxkey) < 0);
      if (stringenum->hasmore)
	{
	  SASStringBTreeNode_t curnode = stringenum->ref.node;
	  long treemod = SASStringBTreeGetModCount (stringenum->tree);

	  if ((curnode != NULL) && (treemod == stringenum->curmod))
	    {
	      short curpos = stringenum->ref.pos;
	      SASStringBTreeNodeHeader *curSBnode =
		(SASStringBTreeNodeHeader *) curnode;
	      if (curpos < curSBnode->count)
		{
		  if (curSBnode->branch[curpos] == NULL)
		    {
		      curpos++;
		      stringenum->ref.pos = curpos;
		      result = curSBnode->vals[curpos];
		      stringenum->curkey = curSBnode->keys[curpos];
		      stringenum->hasmore =
			(strcmp (stringenum->curkey, maxkey) < 0);
		      found = true;
		    }
		  else
		    {
		      found =
			SASStringBTreeNodeSearchGT (curnode,
						    stringenum->curkey,
						    &stringenum->ref);
		      if (found)
			{
			  curnode = stringenum->ref.node;
			  curSBnode = (SASStringBTreeNodeHeader *) curnode;
			  curpos = stringenum->ref.pos;
			  result = curSBnode->vals[curpos];
			  stringenum->curkey = curSBnode->keys[curpos];
			  stringenum->curcount -= 1;
			  stringenum->hasmore =
			    (strcmp (stringenum->curkey, maxkey) < 0);
			}
		    }
		  if (stringenum->hasmore)
		    stringenum->curcount -= 1;
		  else if (found)
		    stringenum->curcount = 1;
		  else
		    stringenum->curcount = 0;
		}
	    }
	  if (!found)
	    {
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext; !found enum->tree=%s\n",
            stringenum->tree, stringenum->curkey);
#endif
	      SASStringBTreeNode_t curnode =
		SASStringBTreeGetRootNode (stringenum->tree);
              if (stringenum->ref.node == NULL)
              {
	          found =
		  SASStringBTreeNodeSearchGE (curnode, stringenum->curkey,
					    &stringenum->ref);
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext; !found curnode=%p SearchGE=%d\n",
                                  curnode, found);
#endif
              } else {
	          found =
		  SASStringBTreeNodeSearchGT (curnode, stringenum->curkey,
					    &stringenum->ref);
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext; !found curnode=%p SearchGT=%d\n",
                                  curnode, found);
#endif
              }
	      if (found)
		{
		  short curpos = stringenum->ref.pos;
		  SASStringBTreeNodeHeader *curSBnode =
		    (SASStringBTreeNodeHeader *) stringenum->ref.node;
		  result = curSBnode->vals[curpos];
		  stringenum->curkey = curSBnode->keys[curpos];
		  stringenum->curmod = treemod;
		  stringenum->curcount =
		    SASStringBTreeGetCurCount (stringenum->tree);
		  stringenum->hasmore =
		    (strcmp (stringenum->curkey, maxkey) < 0);
#if __SASDebugPrint__ > 1
     sas_printf ("SASStringBTreeEnumNext; curpos=%hd node=%p result=%p\n",
                                  curpos, curSBnode, result);
#endif
		}
	      else
		{
		  stringenum->hasmore = false;
		}
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeEnumNext; enum->tree=%p invalid\n",
		  stringenum->tree);
#endif
    }
  SASUnlock (stringenum->tree);
//      sas_printf("{%s,%s,%d}",stringenum->curkey, maxkey, stringenum->hasmore);

  return result;
}

void *
SASStringBTreeEnumNext_nolock (SASStringBTreeEnum_t sbtenum)
{
  __SBEnumeration *stringenum = (__SBEnumeration *) sbtenum;
  void *result = NULL;
  bool found = false;
  char *maxkey;


  maxkey = SASStringBTreeGetMaxKey_nolock (stringenum->tree);
#if __SASDebugPrint__ > 1
  sas_printf ("SASStringBTreeEnumNext_nolock; enum->tree=%p maxkey=%s\n",
                  stringenum->tree, maxkey);
#endif
  if (maxkey != NULL)
    {
      stringenum->hasmore = (strcmp (stringenum->curkey, maxkey) < 0);
      if (stringenum->hasmore)
	{
	  SASStringBTreeNode_t curnode = stringenum->ref.node;
	  long treemod = SASStringBTreeGetModCount_nolock (stringenum->tree);

	  if ((curnode != NULL) && (treemod == stringenum->curmod))
	    {
	      short curpos = stringenum->ref.pos;
	      SASStringBTreeNodeHeader *curSBnode =
		(SASStringBTreeNodeHeader *) curnode;
	      if (curpos < curSBnode->count)
		{
		  if (curSBnode->branch[curpos] == NULL)
		    {
		      curpos++;
		      stringenum->ref.pos = curpos;
		      result = curSBnode->vals[curpos];
		      stringenum->curkey = curSBnode->keys[curpos];
		      stringenum->hasmore =
			(strcmp (stringenum->curkey, maxkey) < 0);
		      found = true;
		    }
		  else
		    {
		      found =
			SASStringBTreeNodeSearchGT (curnode,
						    stringenum->curkey,
						    &stringenum->ref);
		      if (found)
			{
			  curnode = stringenum->ref.node;
			  curSBnode = (SASStringBTreeNodeHeader *) curnode;
			  curpos = stringenum->ref.pos;
			  result = curSBnode->vals[curpos];
			  stringenum->curkey = curSBnode->keys[curpos];
			  stringenum->curcount -= 1;
			  stringenum->hasmore =
			    (strcmp (stringenum->curkey, maxkey) < 0);
			}
		    }
		  if (stringenum->hasmore)
		    stringenum->curcount -= 1;
		  else if (found)
		    stringenum->curcount = 1;
		  else
		    stringenum->curcount = 0;
		}
	    }
	  if (!found)
	    {
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext_nolock; !found enum->tree=%s\n",
            stringenum->tree, stringenum->curkey);
#endif
	      SASStringBTreeNode_t curnode =
		SASStringBTreeGetRootNodeNoLock (stringenum->tree);
              if (stringenum->ref.node == NULL)
              {
	          found =
		  SASStringBTreeNodeSearchGE (curnode, stringenum->curkey,
					    &stringenum->ref);
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext_nolock; !found curnode=%p SearchGE=%d\n",
                                  curnode, found);
#endif
              } else {
	          found =
		  SASStringBTreeNodeSearchGT (curnode, stringenum->curkey,
					    &stringenum->ref);
#if __SASDebugPrint__ > 1
    sas_printf ("SASStringBTreeEnumNext_nolock; !found curnode=%p SearchGT=%d\n",
                                  curnode, found);
#endif
              }
	      if (found)
		{
		  short curpos = stringenum->ref.pos;
		  SASStringBTreeNodeHeader *curSBnode =
		    (SASStringBTreeNodeHeader *) stringenum->ref.node;
		  result = curSBnode->vals[curpos];
		  stringenum->curkey = curSBnode->keys[curpos];
		  stringenum->curmod = treemod;
		  stringenum->curcount =
		    SASStringBTreeGetCurCount (stringenum->tree);
		  stringenum->hasmore =
		    (strcmp (stringenum->curkey, maxkey) < 0);
#if __SASDebugPrint__ > 1
     sas_printf ("SASStringBTreeEnumNext_nolock; curpos=%hd node=%p result=%p\n",
                                  curpos, curSBnode, result);
#endif
		}
	      else
		{
		  stringenum->hasmore = false;
		}
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASStringBTreeEnumNext_nolock; enum->tree=%p invalid\n",
		  stringenum->tree);
#endif
    }
//      sas_printf("{%s,%s,%d}",stringenum->curkey, maxkey, stringenum->hasmore);

  return result;
}
