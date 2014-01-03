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

//#define __SASDebugPrint__ 2
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
#include "sasindex.h"
#include "sasindexpriv.h"
#include "sasindexnodepriv.h"
#include "sasindexenum.h"

typedef struct __IDXEnumeration
{
  SASIndex_t tree;
  SASIndexKey_t *curkey;
  __IDXnodePosRef ref;
  long curmod;
  bool hasmore;
  SASIndexKey_t entryIndex;
} __IDXEnumeration;

typedef void *SASIndexEnum_t;

SASIndexEnum_t
SASIndexEnumCreate (SASIndex_t tree)
{
  __IDXEnumeration *indexenum = NULL;
  indexenum = (__IDXEnumeration *) malloc (sizeof (__IDXEnumeration));
  if (indexenum != NULL)
    {
      indexenum->curmod = SASIndexGetModCount (tree);
      if (indexenum->curmod != 0L)
	{
	  indexenum->hasmore = !SASIndexIsEmpty (tree);
	  indexenum->tree = tree;
	  indexenum->ref.node = NULL;
	  indexenum->ref.pos = 0;
	  SASIndexKeyInitRef (&indexenum->entryIndex, NULL);
	  indexenum->curkey = &indexenum->entryIndex;
	}
      else
	{
	  free (indexenum);
	  indexenum = NULL;
	}
    }

  return (SASIndexEnum_t) indexenum;
}

void
SASIndexEnumDestroy (SASIndexEnum_t indexenum)
{
  free (indexenum);
}

int
SASIndexEnumHasMore (SASIndexEnum_t idxenum)
{
  __IDXEnumeration *indexenum = (__IDXEnumeration *) idxenum;
  return indexenum->hasmore;
}

void *
SASIndexEnumNext (SASIndexEnum_t idxenum)
{
  __IDXEnumeration *indexenum = (__IDXEnumeration *) idxenum;
  void *result = NULL;
  bool found = false;
  SASIndexKey_t *maxkey;


  SASLock (indexenum->tree, SasUserLock__WRITE);
  maxkey = SASIndexGetMaxKey (indexenum->tree);
#if __SASDebugPrint__ > 1
  sas_printf ("SASIndexEnumNext; enum->tree=%p maxkey=%p\n",
		  indexenum->tree, maxkey);
#endif
  if (maxkey != NULL)
    {
      indexenum->hasmore =
	(SASIndexKeyCompare (indexenum->curkey, maxkey) < 0);
      if (indexenum->hasmore)
	{
	  SASIndexNode_t curnode = indexenum->ref.node;
	  long treemod = SASIndexGetModCount (indexenum->tree);

	  if ((curnode != NULL) && (treemod == indexenum->curmod))
	    {
	      short curpos = indexenum->ref.pos;
	      SASIndexNodeHeader *curSBnode = (SASIndexNodeHeader *) curnode;
	      if (curpos < curSBnode->count)
		{
		  if (curSBnode->branch[curpos] == NULL)
		    {
		      curpos++;
		      indexenum->ref.pos = curpos;
		      result = curSBnode->vals[curpos];
		      indexenum->curkey = curSBnode->keys[curpos];
		      indexenum->hasmore =
			(SASIndexKeyCompare (indexenum->curkey, maxkey) < 0);
		      found = true;
		    }
		  else
		    {
		      found =
			SASIndexNodeSearchGT (curnode, indexenum->curkey,
					      &indexenum->ref);
		      if (found)
			{
			  curnode = indexenum->ref.node;
			  curSBnode = (SASIndexNodeHeader *) curnode;
			  curpos = indexenum->ref.pos;
			  result = curSBnode->vals[curpos];
			  indexenum->curkey = curSBnode->keys[curpos];
			  indexenum->hasmore =
			    (SASIndexKeyCompare (indexenum->curkey, maxkey) <
			     0);
			}
		    }
		}
	    }
	  if (!found)
	    {
#if __SASDebugPrint__ > 1
		  sas_printf ("SASIndexEnumNext; !found enum->tree=%p curkey=%p=%ix\n",
				  indexenum->tree, indexenum->curkey, indexenum->curkey->data[0]);
#endif
	      SASIndexNode_t curnode = SASIndexGetRootNode (indexenum->tree);
	      if (indexenum->ref.node == NULL)
	      {
		      found = SASIndexNodeSearchGE (curnode, indexenum->curkey,
					      &indexenum->ref);
#if __SASDebugPrint__ > 1
		      sas_printf ("SASIndexEnumNext; !found curnode=%p SearchGE=%d\n",
				  curnode, found);
#endif
	      } else {
	          found = SASIndexNodeSearchGT (curnode, indexenum->curkey,
				      &indexenum->ref);
#if __SASDebugPrint__ > 1
	          sas_printf ("SASIndexEnumNext; !found curnode=%p SearchGT=%d\n",
				  curnode, found);
#endif
	      }
	      if (found)
		{
		  short curpos = indexenum->ref.pos;
		  SASIndexNodeHeader *curSBnode =
		    (SASIndexNodeHeader *) indexenum->ref.node;
		  result = curSBnode->vals[curpos];
		  indexenum->curkey = curSBnode->keys[curpos];
		  indexenum->curmod = treemod;
		  indexenum->hasmore =
		    (SASIndexKeyCompare (indexenum->curkey, maxkey) < 0);
#if __SASDebugPrint__ > 1
		  sas_printf ("SASIndexEnumNext; curpos=%hd node=%p result=%p\n",
				  curpos, curSBnode, result);
#endif
		}
	      else
		{
		  indexenum->hasmore = false;
		}
	    }
	}
#ifdef __SASDebugPrint__
    }
  else
    {
      sas_printf ("SASIndexEnumNext; enum->tree=%p invalid\n",
		  indexenum->tree);
#endif
    }
  SASUnlock (indexenum->tree);
//      sas_printf("{%s,%s,%d}",indexenum->curkey, maxkey, indexenum->hasmore);

  return result;
}
