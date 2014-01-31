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
#include <stdarg.h>
#include "sasalloc.h"
#include "sasstdio.h"
#include "sassim.h"
#include "sasstringbtreepriv.h"
#include "sasstringbtreeenum.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sasstringbtree_t";

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

static inline void
sasbtree_print_node(SASStringBTreeNode_t node)
{
  int i, n;
  n = SASStringBTreeNodeGetCount(node);
  printf (" node@%p {", node);
  for (i = 0; i <= n; ++i)
    {
      SASStringBTreeNode_t br;
      const char *key = SASStringBTreeNodeGetKeyIndexed(node, i);
      if (key)
        {
          printf("(%s : %s)", SASStringBTreeNodeGetKeyIndexed(node, i),
	  (char*)SASStringBTreeNodeGetValIndexed(node, i));
        }
      if ((br = SASStringBTreeNodeGetBranchIndexed(node, i)))
	sasbtree_print_node(br);
      if (i != n)
	printf(", ");
    }
  printf ("}");
}

static inline void
sasbtree_print(SASStringBTree_t btree)
{
  SASStringBTreeNode_t root = SASStringBTreeGetRootNode(btree);
  if (root)
    {
      printf("{");
      sasbtree_print_node(root);
      printf("}");
    }
  printf("\n");
}

#define JOIN_EXIT_FAILURE 128
//#define __SASDebugPrint__ 1
static int
sassim_btree_test_split ()
{
  SASStringBTree_t stringBTree;
  unsigned long blockSize = block__Size64K;
  SASStringBTreeEnum_t senum;
  char *rawkey, *keyval, *strptr;
  char *temp1, *temp2;
  int i, rc1;
  long modcnt;
  char chr, str[128], keylist[128];
  char str2[4] = " ";

  stringBTree = SASStringBTreeCreate (blockSize);
  if (!stringBTree)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu", SASStringBTreeFreeSpace (stringBTree));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (stringBTree, 128);
#endif
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(stringBTree) = %d", SASStringBTreeIsEmpty (stringBTree));

  memset (str, '\0', 128);
  strptr = &str[0];
  i = 0;
  for (chr = '1'; chr <= 0x7d; chr++)
    {
      keylist[i] = chr;
      *strptr = chr;
      rc1 = SASStringBTreePut (stringBTree, strptr, strptr);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %s)", stringBTree, strptr, strptr);
        return 1;
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
#endif
     strptr++;
     i++;
    }
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);

  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=(%s)", temp1);
  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=(%s)", temp2);
  modcnt = SASStringBTreeGetModCount (stringBTree);
  if (!modcnt)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);

  senum = SASStringBTreeEnumCreate (stringBTree);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", stringBTree);

  i = 0;
  while (SASStringBTreeEnumHasMore (senum))
    {
      char *temp = (char *) SASStringBTreeEnumNext (senum);
      if (!temp)
        {
          break;
        }
        if (keylist[i] != *temp)
        {
            SASSIM_PRINT_ERR ("SASStringBTreeEnumNext: misscompare %c != %c",
                        keylist[i] != *temp);
            return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %c", temp, *temp);
      i++;
    }
  SASStringBTreeEnumDestroy (senum);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu", SASStringBTreeFreeSpace (stringBTree));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (stringBTree, 128);
#endif
  for (chr = 0x31; chr <= 0x60; chr++)
    {
      str2[0] = chr;
      keyval = (char *) SASStringBTreeRemove (stringBTree, str2);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %c", stringBTree, str2, *keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %c", stringBTree, str2, *keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
#endif
    }
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);

  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
  modcnt = SASStringBTreeGetModCount (stringBTree);
  if (modcnt == 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);

  senum = SASStringBTreeEnumCreate (stringBTree);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", stringBTree);

  keyval = (char *) SASStringBTreeEnumNext (senum);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
  SASSIM_PRINT_MSG ("<%p> %c", keyval, *keyval);
  if (*keyval != 'a')
  {
    SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%c expected 'a'",
                senum, *keyval);
    return 1;
  }
  rawkey = keyval;
  while (SASStringBTreeEnumHasMore (senum))
    {
      keyval = (char *) SASStringBTreeEnumNext (senum);
      if (!keyval)
        {
          SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %c", keyval, *keyval);
      if (rawkey < keyval)
          rawkey = keyval;
      else
      {
          SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %c=%c",
                          senum, rawkey, *keyval);
          return 1;
      }
    }
  SASStringBTreeEnumDestroy (senum);
  for (chr = 0x61; chr <= 0x7d; chr++)
    {
      str2[0] = chr;
      keyval = (char *) SASStringBTreeRemove (stringBTree, str2);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %c", stringBTree, str2, *keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %c", stringBTree, str2, *keyval);
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
#endif
    }

  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);

  rc1 = SASStringBTreeIsEmpty (stringBTree);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeIsEmpty(stringBTree) = %d", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(stringBTree) = %d", rc1);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu", SASStringBTreeFreeSpace (stringBTree));

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (stringBTree, 128);
#endif
  SASStringBTreeDestroy (stringBTree);

  return 0;
}

static int
sassim_btree_test1 ()
{
  SASStringBTree_t stringBTree;
  SASStringBTreeEnum_t senum;
  unsigned long blockSize = block__Size64K;
  char *temp1, *temp2, *temp3, *temp4, *temp5, *temp6;
  char str[64];
  char str2[4] = " ";
  char *strptr;
  char chr;
  int rc1;
  long modcnt;

  stringBTree = SASStringBTreeCreate (blockSize);
  if (!stringBTree)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%u)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (stringBTree));
  SASSIM_DUMP_BLOCK (stringBTree, 128);
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(stringBTree) = %d",
		    SASStringBTreeIsEmpty (stringBTree));
  rc1 = SASStringBTreePut (stringBTree, (char *) "s", (void *) "s");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree, "s",
			"s");
      return 1;
    }
  rc1 = SASStringBTreePut (stringBTree, (char *) "j", (void *) "j");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree, "j",
			"j");
      return 1;
    }
  rc1 = SASStringBTreePut (stringBTree, (char *) "m", (void *) "m");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree, "m",
			"m");
      return 1;
    }
  rc1 = SASStringBTreePut (stringBTree, (char *) "~", (void *) "~");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree, "~",
			"~");
      return 1;
    }
  rc1 = SASStringBTreePut (stringBTree, (char *) "!", (void *) "!");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree, "!",
			"!");
      return 1;
    }
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(stringBTree) = %d",
		    SASStringBTreeIsEmpty (stringBTree));
  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey(%p)", stringBTree);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
      if (strcmp(temp1, "!") != 0)
      {
          SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey(%p) expected <%s> was %s@%p",
        		  stringBTree, "!", temp1, temp1);
      }
    }


  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
  rc1 = SASStringBTreeContainsKey (stringBTree, (char *) "s");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeContainsKey(%p, %s): %d", stringBTree,
			"s", rc1);
      return 1;
    }
  rc1 = SASStringBTreeContainsKey (stringBTree, (char *) "S");
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeContainsKey(%p, %s): %d", stringBTree,
			"S", rc1);
      return 1;
    }
  temp3 = (char *) SASStringBTreeGet (stringBTree, (char *) "s");
  if (!temp3)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGet(%p, %s): %p", stringBTree, "s",
			temp3);
      return 1;
    }
  temp4 = (char *) SASStringBTreeGet (stringBTree, (char *) "S");
  if (temp4)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGet(%p, %s): %p", stringBTree, "S",
			temp4);
      return 1;
    }
  rc1 = SASStringBTreePut (stringBTree, (char *) "m", (void *) "M");
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s): %d", stringBTree,
			"m", "M", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 =
    (char *) SASStringBTreeReplace (stringBTree, (char *) "m", (void *) "M");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeReplace(%p, %s, %s)", stringBTree,
			"m", "M");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeReplace(m, M) = <%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 =
    (char *) SASStringBTreeReplace (stringBTree, (char *) "n", (void *) "N");
  if (temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeReplace(%p, %s, %s): %p", stringBTree,
			"n", "N", temp5);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeReplace(n,N)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 = (char *) SASStringBTreeRemove (stringBTree, (char *) "s");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeRemove(%p, %s)", stringBTree, "s");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeRemove(s)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 = (char *) SASStringBTreeRemove (stringBTree, (char *) "j");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeRemove(%p, %s)", stringBTree, "j");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeRemove(j)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 = (char *) SASStringBTreeRemove (stringBTree, (char *) "m");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeRemove(%p, %s)", stringBTree, "m");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeRemove(m)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 = (char *) SASStringBTreeRemove (stringBTree, (char *) "~");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeRemove(%p, %s)", stringBTree, "~");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeRemove(~)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp5 = (char *) SASStringBTreeRemove (stringBTree, (char *) "!");
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeRemove(%p, %s)", stringBTree, "!");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeRemove(!)=<%s>", temp5);
  SASSIM_PRINT_MSG ("sasbtree_print=");
  sasbtree_print (stringBTree);
  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (temp1)
    {
      SASSIM_PRINT_ERR
	("SASStringBTreeGetMinKey(%p) returned on a empty btree",
	 stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (temp2)
    {
      SASSIM_PRINT_ERR
	("SASStringBTreeGetMaxKey(%p) returned on a empty btree",
	 stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
  memset (str, '\0', 64);
  strptr = &str[33];
  for (chr = 'a'; chr <= 'z'; chr++)
    {
      *strptr = chr;
      rc1 = SASStringBTreePut (stringBTree, strptr, strptr);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree,
			    strptr, strptr);
	  return 1;
	}
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
      strptr++;
    }
  modcnt = SASStringBTreeGetModCount (stringBTree);
  if (!modcnt)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
  senum = SASStringBTreeEnumCreate (stringBTree);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate(%p)", stringBTree);
      return 1;
    }
  printf ("SASStringBTreeEnumCreate(%p)", stringBTree);
  printf (" SASStringBTreeEnumNext(%p)   =<", senum);
  while (SASStringBTreeEnumHasMore (senum))
    {
      char *temp = (char *) SASStringBTreeEnumNext (senum);
      if (!temp)
	{
	  break;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  printf (">");
  SASStringBTreeEnumDestroy (senum);
  senum = SASStringBTreeEnumCreate (stringBTree);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate(%p)", stringBTree);
      return 1;
    }
  printf ("SASStringBTreeEnumCreate(%p)", stringBTree);
  strptr = &str[1];
  for (chr = 'A'; chr <= 'Z'; chr++)
    {
      *strptr = chr;
      rc1 = SASStringBTreePut (stringBTree, strptr, strptr);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut(%p, %s, %s)", stringBTree,
			    strptr, strptr);
	  return 1;
	}
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
      if (SASStringBTreeEnumHasMore (senum))
	{
	  char *temp = (char *) SASStringBTreeEnumNext (senum);
	  if (!temp)
	    {
	      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)", senum);
	      return 1;
	    }
	  SASSIM_PRINT_MSG ("SASStringBTreeEnumNext(%p)=%c", senum, *temp);
	  strptr++;
	}
      printf (" SASStringBTreeEnumNext(%p)   =<", senum);
      while (SASStringBTreeEnumHasMore (senum))
	{
	  char *temp = (char *) SASStringBTreeEnumNext (senum);
	  if (!temp)
	    {
	      break;
	    }
	  SASSIM_PRINT_MSG ("%c", *temp);
	}
      printf (">");
    }
  SASStringBTreeEnumDestroy (senum);
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (stringBTree));
  modcnt = SASStringBTreeGetModCount (stringBTree);
  if (!modcnt)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
  temp1 = SASStringBTreeGetMinKey (stringBTree);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
  temp2 = SASStringBTreeGetMaxKey (stringBTree);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey(%p)", stringBTree);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
  senum = SASStringBTreeEnumCreate (stringBTree);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate(%p)", stringBTree);
      return 1;
    }
  printf ("SASStringBTreeEnumCreate(%p)", stringBTree);
  printf (" SASStringBTreeEnumNext(%p)   =<", senum);
  while (SASStringBTreeEnumHasMore (senum))
    {
      char *temp = (char *) SASStringBTreeEnumNext (senum);
      if (!temp)
	{
	  break;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  printf (">");
  SASStringBTreeEnumDestroy (senum);
  senum = SASStringBTreeEnumCreateStartAt (stringBTree, (char*)"m");
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreateStartAt(%p)", stringBTree);
      return 1;
    }
  printf ("SASStringBTreeEnumCreate(%p)", stringBTree);
  temp6 = SASStringBTreeEnumCurrent(senum);
  printf (" SASStringBTreeEnumNext(%p) %c  =<", senum, *temp6);
  while (SASStringBTreeEnumHasMore (senum))
    {
      char *temp = (char *) SASStringBTreeEnumNext (senum);
      if (!temp)
	{
	  break;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  printf (">");
  SASStringBTreeEnumDestroy (senum);
#if 1
  for (chr = 'a'; chr <= 'z'; chr++)
    {
      str2[0] = chr;
      temp6 = (char *) SASStringBTreeRemove (stringBTree, str2);
      if (!temp6)
	{
	  break;
	}
      SASSIM_PRINT_MSG ("SASStringBTreeRemove(%s)= %c", str2, *temp6);
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
    }
  for (chr = 'A'; chr <= 'Z'; chr++)
    {
      str2[0] = chr;
      temp6 = (char *) SASStringBTreeRemove (stringBTree, str2);
      if (!temp6)
	{
	  break;
	}
      SASSIM_PRINT_MSG ("SASStringBTreeRemove(%s)= %c", str2, *temp6);
      SASSIM_PRINT_MSG ("sasbtree_print=");
      sasbtree_print (stringBTree);
    }

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (stringBTree));
  SASSIM_DUMP_BLOCK (stringBTree, 128);
#endif
  SASStringBTreeDestroy (stringBTree);
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

  failures += sassim_btree_test1 ();
  failures += sassim_btree_test_split();

  //SASCleanUp();
  printf("SAS removed\n");
  SASRemove();

  return failures;
}
