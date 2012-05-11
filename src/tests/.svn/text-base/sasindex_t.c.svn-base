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
#include "sasindexkey.h"
#include "sasindexenum.h"
#include "sasindexpriv.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sasindex_t";

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

static void
sasindex_print_node (SASIndexNode_t node)
{
  int i, n;
  n = SASIndexNodeGetCount (node);
  for (i = 0; i <= n; ++i)
    {
      SASIndexNode_t br;
      SASIndexKey_t *key = SASIndexNodeGetKeyIndexed (node, i);
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

static void
sasindex_print (SASIndex_t index)
{
  SASIndexNode_t root = SASIndexGetRootNode (index);
  if (root)
    {
      printf ("{");
      sasindex_print_node (root);
      printf ("}");
    }
  printf ("\n");
}

#define JOIN_EXIT_FAILURE 128
//#define __SASDebugPrint__ 1


static int
sassim_index_test1 ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  SASIndexKey_t ndxkey2;
  SASIndexKey_t ndxkey3;
  char str[64];
  char str2[4] = " ";
  char *txtstr;
  char chr;
  int i;
  int rc1;
  long modcnt;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
  SASSIM_DUMP_BLOCK (index, 128);
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print (index);
  SASIndexKeyInitRef (&ndxkey, (void *) 's');
  rc1 = SASIndexPut (index, &ndxkey, (void *) "s");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, 's', "s");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);
  SASIndexKeyInitRef (&ndxkey, (void *) 'j');
  rc1 = SASIndexPut (index, &ndxkey, (void *) "j");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, 'j', "j");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);
  SASIndexKeyInitRef (&ndxkey, (void *) 'm');
  rc1 = SASIndexPut (index, &ndxkey, (void *) "m");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, 'm', "m");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);
  SASIndexKeyInitRef (&ndxkey, (void *) '~');
  rc1 = SASIndexPut (index, &ndxkey, (void *) "~");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, '~', "~");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);
  SASIndexKeyInitRef (&ndxkey, (void *) '!');
  rc1 = SASIndexPut (index, &ndxkey, (void *) "!");
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, '!', "!");
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lx", temp1->data[0]);
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lx", temp2->data[0]);
  SASIndexKeyInitRef (&ndxkey2, (void *) 's');
  rc1 = SASIndexContainsKey (index, &ndxkey2);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, 's')", index);
      return 1;
    }
  SASIndexKeyInitRef (&ndxkey3, (void *) 'S');
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitRef (%p, 'S')", index);
      return 1;
    }
  txtstr = (char *) SASIndexGet (index, &ndxkey2);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, 's')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(s)=%s", txtstr);
  txtstr = (char *) SASIndexGet (index, &ndxkey3);
  if (txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, 'S')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(S)=%p", txtstr);

  SASIndexKeyInitRef (&ndxkey2, (void *) 'm');
  rc1 = SASIndexPut (index, &ndxkey2, (void *) "M");
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, 'm', \"M\")", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  txtstr = (char *) SASIndexReplace (index, &ndxkey2, (void *) "M");
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, 'm', \"M\")", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(m,M)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey, (void *) 'n');
  txtstr = (char *) SASIndexReplace (index, &ndxkey, (void *) "N");
  if (txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, 'n', \"N\")", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(n,N)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey2, (void *) 's');
  txtstr = (char *) SASIndexRemove (index, &ndxkey2);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, 's')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey, (void *) 'j');
  txtstr = (char *) SASIndexRemove (index, &ndxkey);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, 'j')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(j)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey, (void *) 'm');
  txtstr = (char *) SASIndexRemove (index, &ndxkey);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, 'm')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(m)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey, (void *) '~');
  txtstr = (char *) SASIndexRemove (index, &ndxkey);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, '~')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(~)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  SASIndexKeyInitRef (&ndxkey, (void *) '!');
  txtstr = (char *) SASIndexRemove (index, &ndxkey);
  if (!txtstr)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, '!')", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(!)=<%s>", txtstr);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print (index);

  temp1 = SASIndexGetMinKey (index);
  if (temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%s", temp1);
  temp2 = SASIndexGetMaxKey (index);
  if (temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lx", temp2);

  for (i = 0; i < 64; i++)
    str[i] = 0;
  txtstr = &str[33];
  for (chr = 'a'; chr <= 'z'; chr++)
    {
      long key = chr;
      *txtstr = chr;
      SASIndexKeyInitRef (&ndxkey, (void *) key);
      rc1 = SASIndexPut (index, &ndxkey, txtstr);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, key, txtstr);
        return 1;
      }
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print (index);
      txtstr++;
    }
  SASSIM_PRINT_MSG ("SASIndexPrintValue=");
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lx", temp1->data[0]);
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lx", temp2->data[0]);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      char *temp = (char *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  SASIndexEnumDestroy (senum);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  txtstr = &str[1];
  for (chr = 'A'; chr <= 'Z'; chr++)
    {
      char *temp;
      long key = chr;
      *txtstr = chr;
      SASIndexKeyInitRef (&ndxkey, (void *) key);
      rc1 = SASIndexPut (index, &ndxkey, txtstr);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %c, %s)", index, key, txtstr);
        return 1;
      }
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print (index);
      if (!SASIndexEnumHasMore (senum))
        {
          SASSIM_PRINT_ERR ("SASIndexEnumHasMore (%p)", senum);
          return 1;
	}
      temp = (char *) SASIndexEnumNext (senum);
      if (!temp)
	{
	}
      SASSIM_PRINT_MSG ("SASIndexEnumNext(%p)=%c", senum, *temp);
      txtstr++;
    }

  while (SASIndexEnumHasMore (senum))
    {
      char *temp = (char *) SASIndexEnumNext (senum);
      if (!temp)
	{
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lx", temp1->data[0]);
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lx", temp2->data[0]);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }

  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      char *temp = (char *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	}
      SASSIM_PRINT_MSG ("%c", *temp);
    }
  SASIndexEnumDestroy (senum);

  for (chr = 'a'; chr <= 'z'; chr++)
    {
      long key = chr;
      str2[0] = chr;
      SASIndexKeyInitRef (&ndxkey, (void *) key);
      txtstr = (char *) SASIndexRemove (index, &ndxkey);
      if (!txtstr)
	{
          SASSIM_PRINT_ERR ("SASIndexRemove (%p, %c)", senum, chr);
          return 1;
	}
      SASSIM_PRINT_MSG ("SASIndexRemove(%s)= %c", str2, *txtstr);
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print (index);
    }

  for (chr = 'A'; chr <= 'Z'; chr++)
    {
      long key = chr;
      str2[0] = chr;
      SASIndexKeyInitRef (&ndxkey, (void *) key);
      txtstr = (char *) SASIndexRemove (index, &ndxkey);
      if (!txtstr)
	{
          SASSIM_PRINT_ERR ("SASIndexRemove (%p, %c)", senum, chr);
          return 1;
	}
      SASSIM_PRINT_MSG ("SASIndexRemove(%s)= %c", str2, *txtstr);
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print (index);
    }
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));

  SASSIM_DUMP_BLOCK (index, 128);

  SASIndexDestroy (index);

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

  failures += sassim_index_test1 ();

  //SASCleanUp();
  printf("SAS removed\n");
  SASRemove();

  return failures;
}
