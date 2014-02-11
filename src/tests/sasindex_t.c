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

#ifdef __SASDebugPrint__
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
#endif

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
sasindex_print_node_uint (SASIndexNode_t node)
{
  int i, n;
  n = SASIndexNodeGetCount (node);
  printf (" node@%p {", node);
  for (i = 0; i <= n; ++i)
    {
      SASIndexNode_t br;
      SASIndexKey_t *key = SASIndexNodeGetKeyIndexed (node, i);
      if (key)
      {
#ifdef __WORDSIZE_64
      printf ("(%lx : %p)", key->data[0],
             (void*) SASIndexNodeGetValIndexed (node, i));
#else
      printf ("(%lx,%lx : %p)", key->data[0], key->data[1],
             (void*) SASIndexNodeGetValIndexed (node, i));
#endif
      }
      if ((br = SASIndexNodeGetBranchIndexed (node, i)))
	     sasindex_print_node_uint (br);
      if (i != n)
	     printf (", ");
    }
  printf ("}");
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

static void
sasindex_print_uint (SASIndex_t index)
{
  SASIndexNode_t root = SASIndexGetRootNode (index);
  if (root)
    {
      printf ("{");
      sasindex_print_node_uint (root);
      printf ("}");
    }
  printf ("\n");
}

static void
sasindex_print_node_int (SASIndexNode_t node)
{
  int i, n;
  signed long long keyval;
  n = SASIndexNodeGetCount (node);
  printf (" node@%p {", node);
  for (i = 0; i <= n; ++i)
    {
      SASIndexNode_t br;
      SASIndexKey_t *key = SASIndexNodeGetKeyIndexed (node, i);
      if (key)
      {
      keyval = SASIndexKeyReturn1stInt64(key);
#ifdef __WORDSIZE_64
      printf ("(%lx (%lld) : %p)", key->data[0], keyval,
             (void*) SASIndexNodeGetValIndexed (node, i));
#else
      printf ("(%lx,%lx (%lld) : %p)", key->data[0], key->data[1],
             keyval, (void*) SASIndexNodeGetValIndexed (node, i));
#endif
      }
      if ((br = SASIndexNodeGetBranchIndexed (node, i)))
             sasindex_print_node_int (br);
      if (i != n)
             printf (", ");
    }
  printf ("}");
}

static void
sasindex_print_node_double (SASIndexNode_t node)
{
  int i, n;
  double keyval;
  n = SASIndexNodeGetCount (node);
  printf (" node@%p {", node);
  for (i = 0; i <= n; ++i)
    {
      SASIndexNode_t br;
      SASIndexKey_t *key = SASIndexNodeGetKeyIndexed (node, i);
      if (key)
      {
        keyval = SASIndexKeyReturn1stDouble(key);
#ifdef __WORDSIZE_64
      printf ("(%lx (%.14a) : %p)", key->data[0], keyval,
             (void*) SASIndexNodeGetValIndexed (node, i));
#else
      printf ("(%lx,%lx (%.14a) : %p)", key->data[0], key->data[1],
             keyval,
             (void*) SASIndexNodeGetValIndexed (node, i));
#endif
      }
      if ((br = SASIndexNodeGetBranchIndexed (node, i)))
             sasindex_print_node_double (br);
      if (i != n)
             printf (", ");
    }
  printf ("}");
}

static void
sasindex_print_int (SASIndex_t index)
{
  SASIndexNode_t root = SASIndexGetRootNode (index);
  if (root)
    {
      printf ("{");
      sasindex_print_node_int (root);
      printf ("}");
    }
  printf ("\n");
}

static void
sasindex_print_double (SASIndex_t index)
{
  SASIndexNode_t root = SASIndexGetRootNode (index);
  if (root)
    {
      printf ("{");
      sasindex_print_node_double (root);
      printf ("}");
    }
  printf ("\n");
}

#define JOIN_EXIT_FAILURE 128
//#define __SASDebugPrint__ 1
static int
sassim_index_test5 ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;

  int i;
  int rc1;
  long modcnt;
  double rawkey, rawkey2;
  double keylist[64];
  void *keyval;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

  keylist[0] = -4.0;
  keylist[1] = -3.0;
  keylist[2] = -2.0;
  keylist[3] = -1.0;
  keylist[4] =  0.0;
  keylist[5] =  1.0;
  keylist[6] =  2.0;
  keylist[7] =  3.0;
  keylist[8] =  4.0;

  for (i = 0; i < 9; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lf) != %lf", &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
          return 1;
        }

      SASSIM_PRINT_MSG ("SASIndexPut (%p, %lf (%.14a), %p)", index, rawkey, rawkey, keyval);
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  keylist[9] =  -27.0;
  keylist[10] = -26.0;
  keylist[11] =  26.0;
  keylist[12] =  27.0;
  keylist[13] =  2147483647.0;
  keylist[14] =  2147483647.5;
  keylist[15] =  2147483647.75;
  keylist[16] = -2147483647.0;
  keylist[17] = -2147483647.5;
  keylist[18] = -2147483647.75;
  /*
  keylist[19] = -1.7976931348623157e+308;
  keylist[20] =  1.7976931348623157e+308;
  */
  for (i = 9; i < 19; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lf) != %lf", &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
          return 1;
        }
      SASSIM_PRINT_MSG ("SASIndexPut (%p, %lf (%.14a), %p)", index, rawkey, rawkey, keyval);
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      double *temp = (double *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lf", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

  keylist[19] = -1.7976931348623157e+308;
  keylist[20] =  1.7976931348623157e+308;
  keylist[21] =  __builtin_inf();
  keylist[22] = -__builtin_inf();
  keylist[23] =  2.2250738585072014e-308;
  keylist[24] = -2.2250738585072014e-308;

  for (i = 19; i < 25; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lg) != %lg", &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lg, %p)", index, rawkey, keyval);
          return 1;
        }
      SASSIM_PRINT_MSG ("SASIndexPut (%p, %lg (%.14a), %p)", index, rawkey, rawkey, keyval);
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      double *temp = (double *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lg", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_test4 ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  SASIndexKey_t ndxkey2;
  SASIndexKey_t ndxkey3;
  int i;
  long modcnt;
  int rc1;
  double rawkey, rawkey2;
  double keylist[64];
  void *keyval, *keyval2;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_double (index);
  rawkey = 115.0;
  keyval = (void *)'s';
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
  if (rawkey != rawkey2)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lf) != %lf", &ndxkey, rawkey, rawkey2);
      return 1;
    }

  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  rawkey = 0.0;
  keyval = (void *)0;
  SASIndexKeyInitDouble (&ndxkey, rawkey);

  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));
  rawkey = -450;
  keyval = (void *)-450;
  SASIndexKeyInitDouble (&ndxkey, rawkey);

  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  rawkey = 300.0;
  keyval = (void *)300;
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  rawkey = 33.0;
  keyval = (void *)'!';
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  if (SASIndexKeyReturn1stDouble(temp1) != -450)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));
  rawkey = -3.25;
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, %lf)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lf) false", index, rawkey);
    }
  rawkey = 115.0;
  SASIndexKeyInitDouble (&ndxkey2, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey2);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, %lf)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lf) true", index, rawkey);
    }
  rawkey = 83.0;
  SASIndexKeyInitDouble (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitDouble (%p, %lf)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lf) false", index, rawkey);
    }
  rawkey = -451;
  SASIndexKeyInitDouble (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitDouble (%p, %lf)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lf) false", index, rawkey);
    }
  rawkey = 0;
  SASIndexKeyInitDouble (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitDouble (%p, %lf)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lf) true", index, rawkey);
    }
  rawkey = -3.25;
  keyval = (void *) SASIndexGet (index, &ndxkey);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lf)=%p", rawkey, keyval);

  rawkey = 115.0;
  keyval = (void *) SASIndexGet (index, &ndxkey2);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lf)=%p", rawkey, keyval);
  rawkey = 0;
  keyval = (void *) SASIndexGet (index, &ndxkey3);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lf)=%p", rawkey, keyval);

  rawkey = 115.0;
  keyval = (void *)'M';
  SASIndexKeyInitDouble (&ndxkey2, rawkey);
  rc1 = SASIndexPut (index, &ndxkey2, keyval);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  keyval2 = (char *) SASIndexReplace (index, &ndxkey2, keyval);
  if (!keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  rawkey = 110.0;
  keyval = (void *)'n';
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  keyval2 = (char *) SASIndexReplace (index, &ndxkey, keyval);
  if (keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %lf, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
#if 0
  rawkey = -3.25;
  SASIndexKeyInitDouble (&ndxkey2, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey2);
  if (keyval != (void *)-3)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
#endif
  rawkey = 's';
  SASIndexKeyInitDouble (&ndxkey2, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey2);
  if (keyval != (void *)'M')
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  rawkey = 0;
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  rawkey = -450;
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)-450)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  rawkey = 300;
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)300)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  rawkey = '!';
  SASIndexKeyInitDouble (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)'!')
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  temp1 = SASIndexGetMinKey (index);
  if (temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%p", temp1);
  temp2 = SASIndexGetMaxKey (index);
  if (temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%p", temp2);
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  rawkey = 7.0;
  keylist[0] = 1.0;
  for (i = 0; i < 60; i++)
    {

      keylist[i] = rawkey;
      rawkey = rawkey - 0.35;
    }
  for (i = 0; i < 30; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lf) != %lf",
                                   &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
          return 1;
        }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

  SASSIM_PRINT_MSG ("SASIndexPrintValue=");
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0)
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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      double *temp = (double *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lf", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  for (i = 30; i < 60; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stDouble (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitDouble/1st (%p, %lf) != %lf",
                                         &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lf, %p)", index, rawkey, keyval);
          return 1;
        }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
      if (!SASIndexEnumHasMore (senum))
        {
          SASSIM_PRINT_ERR ("SASIndexEnumHasMore (%p)", senum);
          return 1;
        }
      else
        {
          double *temp = (double *) SASIndexEnumNext (senum);
          if (!temp)
            {
              SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
              return 1;
            }
          SASSIM_PRINT_MSG ("<%p> %lf", temp, *temp);
        }
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);

  while (SASIndexEnumHasMore (senum))
    {
      double *temp = (double *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lf", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0)
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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }

  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  i = 59;
  while (SASIndexEnumHasMore (senum))
    {
      double *temp = (double *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lf", temp, *temp);
      if (*temp != keylist[i])
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p) sequence error: %lf != %lf",
                              index, *temp, keylist[i]);
          return 1;
        }
      i--;
    }
  SASIndexEnumDestroy (senum);

  for (i = 59; i >= 30; i--)
    {
      rawkey = keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %lf) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }

  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_double (index);
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lf", SASIndexKeyReturn1stDouble(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lf", SASIndexKeyReturn1stDouble(temp2));

  for (i = 0; i < 30; i++)
    {
      rawkey = keylist[i];
      SASIndexKeyInitDouble (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lf) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %lf) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_double (index);
#endif
    }

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_test3 ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  SASIndexKey_t ndxkey2;
  SASIndexKey_t ndxkey3;
  int i;
  long modcnt;
  int rc1;
  signed long long rawkey, rawkey2;
  signed long long keylist[64];
  void *keyval, *keyval2;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_int (index);
  rawkey = -3;
  keyval = (void *)-3;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rawkey2 = SASIndexKeyReturn1stInt64 (&ndxkey);
  if (rawkey != rawkey2)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitInt64/1st (%p, %lld) != %lld",
                                              &ndxkey, rawkey, rawkey2);
      return 1;
    }
  rc1 = SASIndexPut (index, &ndxkey, keyval);

  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));

  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_int (index);
  rawkey = 's';
  keyval = (void *)'s';
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rawkey2 = SASIndexKeyReturn1stInt64 (&ndxkey);
  if (rawkey != rawkey2)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitInt64/1st (%p, %lld) != %lld",
                                                  &ndxkey, rawkey, rawkey2);
      return 1;
    }

  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  rawkey = 0;
  keyval = (void *)0;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));
  rawkey = -450;
  keyval = (void *)-450;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  rawkey = 300;
  keyval = (void *)300;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  rawkey = '!';
  keyval = (void *)'!';
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));
  rawkey = -3;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, %lld)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lld) true", index, rawkey);
    }
  rawkey = 's';
  SASIndexKeyInitInt64 (&ndxkey2, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey2);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, %lld)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lld) true", index, rawkey);
    }
  rawkey = 'S';
  SASIndexKeyInitInt64 (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitInt64 (%p, %lld)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lld) false", index, rawkey);
    }
  rawkey = -451;
  SASIndexKeyInitInt64 (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitInt64 (%p, %lld)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lld) false", index, rawkey);
    }
  rawkey = 0;
  SASIndexKeyInitInt64 (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitInt64 (%p, %lld)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %lld) true", index, rawkey);
    }
  rawkey = -3;
  keyval = (void *) SASIndexGet (index, &ndxkey);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lld)=%p", rawkey, keyval);

  rawkey = 's';
  keyval = (void *) SASIndexGet (index, &ndxkey2);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lld)=%p", rawkey, keyval);
  rawkey = 0;
  keyval = (void *) SASIndexGet (index, &ndxkey3);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%lld)=%p", rawkey, keyval);

  rawkey = 's';
  keyval = (void *)'M';
  SASIndexKeyInitInt64 (&ndxkey2, rawkey);
  rc1 = SASIndexPut (index, &ndxkey2, keyval);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  keyval2 = (char *) SASIndexReplace (index, &ndxkey2, keyval);
  if (!keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  rawkey = 'n';
  keyval = (void *)'n';
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  keyval2 = (char *) SASIndexReplace (index, &ndxkey, keyval);
  if (keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %lld, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  rawkey = -3;
  SASIndexKeyInitInt64 (&ndxkey2, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey2);
  if (keyval != (void *)-3)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  rawkey = 's';
  SASIndexKeyInitInt64 (&ndxkey2, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey2);
  if (keyval != (void *)'M')
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  rawkey = 0;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  rawkey = -450;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)-450)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  rawkey = 300;
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)300)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  rawkey = '!';
  SASIndexKeyInitInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (keyval != (void *)'!')
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  temp1 = SASIndexGetMinKey (index);
  if (temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%p", temp1);
  temp2 = SASIndexGetMaxKey (index);
  if (temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%p", temp2);
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  rawkey = 30L;
  for (i = 0; i < 60; i++)
    {
      keylist[i] = rawkey;
      rawkey = rawkey - 1;
    }
  for (i = 0; i < 30; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stInt64 (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitInt64/1st (%p, %lld) != %lld",
                                           &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
          return 1;
        }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_int (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      signed long long *temp = (signed long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lld", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  for (i = 30; i < 60; i++)
    {
      rawkey = keylist[i];
      keyval = &keylist[i];
      SASIndexKeyInitInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stInt64 (&ndxkey);
      if (rawkey != rawkey2)
        {
          SASSIM_PRINT_ERR ("SASIndexKeyInitInt64/1st (%p, %lld) != %lld",
                                     &ndxkey, rawkey, rawkey2);
          return 1;
        }
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1)
        {
          SASSIM_PRINT_ERR ("SASIndexPut (%p, %lld, %p)", index, rawkey, keyval);
          return 1;
        }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_int (index);
#endif
      if (!SASIndexEnumHasMore (senum))
        {
          SASSIM_PRINT_ERR ("SASIndexEnumHasMore (%p)", senum);
          return 1;
        }
      else
        {
          signed long long *temp = (signed long long *) SASIndexEnumNext (senum);
          if (!temp)
            {
              SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
              return 1;
            }
          SASSIM_PRINT_MSG ("<%p> %lld", temp, *temp);
        }
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);

  while (SASIndexEnumHasMore (senum))
    {
      signed long long *temp = (signed long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lld", temp, *temp);
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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }

  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  i = 59;
  while (SASIndexEnumHasMore (senum))
    {
      signed long long *temp = (signed long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
        }
      SASSIM_PRINT_MSG ("<%p> %lld", temp, *temp);
      if (*temp != keylist[i])
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p) sequence error: %lld != %lld",
                              index, *temp, keylist[i]);
          return 1;
        }
      i--;
    }
  SASIndexEnumDestroy (senum);

  for (i = 0; i < 30; i++)
    {
      rawkey = keylist[i];
      SASIndexKeyInitInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %lld) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_int (index);
#endif
    }

  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_int (index);
  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%lld", SASIndexKeyReturn1stInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%lld", SASIndexKeyReturn1stInt64(temp2));
  for (i = 30; i < 60; i++)
    {
      rawkey = keylist[i];
      SASIndexKeyInitInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %lld) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %lld) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_int (index);
#endif
    }

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);
  return 0;
}

static int
sassim_index_test_split ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long *keyref;
  unsigned long long keylist[128];
  void *keyval;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
  rawkey = 1L;
  keylist[0] = 0L;
  for (i = 1; i < 65; i++)
    {
	  keylist[i] = rawkey;
      rawkey = rawkey + rawkey;
    }
  for (i = 65; i < 128; i++)
    {
	  keylist[i] = keylist[i-63] + 1ULL;
    }

  for (i = 0; i < 65; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stUInt64 (&ndxkey);
      if (rawkey != rawkey2)
    	{
    	  SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64/1st (%p, %llx) != %llx", &ndxkey, rawkey, rawkey2);
    	  return 1;
    	}
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  i = 0;
  while (SASIndexEnumHasMore (senum))
    {
      unsigned long long *temp = (unsigned long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      else
      {
    	if (keylist[i] != *temp)
    	{
            SASSIM_PRINT_ERR ("SASIndexEnumNext: misscompare %llx != %llx",
            		keylist[i] != *temp);
            return 1;
    	}
      }
      SASSIM_PRINT_MSG ("<%p> %llx", temp, *temp);
      i++;
    }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

  for (i = 65; i < 128; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stUInt64 (&ndxkey);
      if (rawkey != rawkey2)
    	{
    	  SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64/1st (%p, %llx) != %llx", &ndxkey, rawkey, rawkey2);
    	  return 1;
    	}
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);

  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
    }
  SASIndexEnumDestroy (senum);

  for (i = 0; i < 65; i++)
    {
	  rawkey = keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%p", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%p", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);

  if (*keyref != 3ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
    }
  SASIndexEnumDestroy (senum);

  for (i = 65; i < 128; i++)
    {
	  rawkey = keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_test2 ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size64K;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  SASIndexKey_t ndxkey2;
  SASIndexKey_t ndxkey3;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long keylist[64];
  void *keyval, *keyval2;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
  rawkey = 's';
  keyval = (void *)'s';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  rawkey2 = SASIndexKeyReturn1stUInt64 (&ndxkey);
  if (rawkey != rawkey2)
	{
	  SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64/1st (%p, %llx) != %llx", &ndxkey, rawkey, rawkey2);
	  return 1;
	}

  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
  rawkey = 'j';
  keyval = (void *)'j';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
  rawkey = 'm';
  keyval = (void *)'m';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
  rawkey = '~';
  keyval = (void *)'~';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
  rawkey = '!';
  keyval = (void *)'!';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  rc1 = SASIndexPut (index, &ndxkey, keyval);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
  rawkey = 's';
  SASIndexKeyInitUInt64 (&ndxkey2, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey2);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexContainsKey (%p, %llx)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %llx) true", index, rawkey);
    }
  rawkey = 'S';
  SASIndexKeyInitUInt64 (&ndxkey3, rawkey);
  rc1 = SASIndexContainsKey (index, &ndxkey3);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64 (%p, %llx)", index, rawkey);
      return 1;
    }
  else
    {
      SASSIM_PRINT_MSG ("SASIndexContainsKey (%p, %llx) false", index, rawkey);
    }
  rawkey = 's';
  keyval = (void *) SASIndexGet (index, &ndxkey2);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%llx)=%p", rawkey, keyval);
  rawkey = 'S';
  keyval = (void *) SASIndexGet (index, &ndxkey3);
  if (keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexGet (%p, %p)", index, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGet(%llx)=%p", rawkey, keyval);

  rawkey = 'm';
  keyval = (void *)'M';
  SASIndexKeyInitUInt64 (&ndxkey2, rawkey);
  rc1 = SASIndexPut (index, &ndxkey2, keyval);
  if (rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  keyval2 = (char *) SASIndexReplace (index, &ndxkey2, keyval);
  if (!keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = 'n';
  keyval = (void *)'n';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  keyval2 = (char *) SASIndexReplace (index, &ndxkey, keyval);
  if (keyval2)
    {
      SASSIM_PRINT_ERR ("SASIndexReplace (%p, %llx, %p)", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexReplace(%p)=<%p>", keyval, keyval2);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = 's';
  keyval = (void *)'s';
  SASIndexKeyInitUInt64 (&ndxkey2, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey2);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = 'j';
  keyval = (void *)'j';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = 'm';
  keyval = (void *)'m';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = '~';
  keyval = (void *)'~';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  rawkey = '!';
  keyval = (void *)'!';
  SASIndexKeyInitUInt64 (&ndxkey, rawkey);
  keyval = (char *) SASIndexRemove (index, &ndxkey);
  if (!keyval)
    {
      SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) == %p", index, rawkey, keyval);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexRemove(s)=<%p>", keyval);
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  temp1 = SASIndexGetMinKey (index);
  if (temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%p", temp1);
  temp2 = SASIndexGetMaxKey (index);
  if (temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%p", temp2);
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

  rawkey = 1L;
  keylist[0] = 0L;
  for (i = 0; i < 64; i++)
    {
	  keylist[i] = rawkey;
      rawkey = rawkey + rawkey;
    }

  for (i = 0; i < 32; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stUInt64 (&ndxkey);
      if (rawkey != rawkey2)
    	{
    	  SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64/1st (%p, %llx) != %llx", &ndxkey, rawkey, rawkey2);
    	  return 1;
    	}
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  while (SASIndexEnumHasMore (senum))
    {
      unsigned long long *temp = (unsigned long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      SASSIM_PRINT_MSG ("<%p> %llx", temp, *temp);
    }
  SASIndexEnumDestroy (senum);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  for (i = 32; i < 63; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rawkey2 = SASIndexKeyReturn1stUInt64 (&ndxkey);
      if (rawkey != rawkey2)
    	{
    	  SASSIM_PRINT_ERR ("SASIndexKeyInitUInt64/1st (%p, %llx) != %llx", &ndxkey, rawkey, rawkey2);
    	  return 1;
    	}
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
      if (!SASIndexEnumHasMore (senum))
        {
          SASSIM_PRINT_ERR ("SASIndexEnumHasMore (%p)", senum);
          return 1;
		}
      else
        {
		  unsigned long long *temp = (unsigned long long *) SASIndexEnumNext (senum);
		  if (!temp)
			{
			  SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
			  return 1;
			}
		  SASSIM_PRINT_MSG ("<%p> %llx", temp, *temp);
        }
    }
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  while (SASIndexEnumHasMore (senum))
    {
      unsigned long long *temp = (unsigned long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      SASSIM_PRINT_MSG ("<%p> %llx", temp, *temp);
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
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }

  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);
  i = 0;
  while (SASIndexEnumHasMore (senum))
    {
      unsigned long long *temp = (unsigned long long *) SASIndexEnumNext (senum);
      if (!temp)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }
      SASSIM_PRINT_MSG ("<%p> %llx", temp, *temp);
      if (*temp != keylist[i])
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p) sequence error: %llx != #llx",
        		  index, *temp, keylist[i]);
          return 1;
        }
      i++;
    }
  SASIndexEnumDestroy (senum);

  for (i = 0; i < 32; i++)
    {
	  rawkey = keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);

  for (i = 32; i < 63; i++)
    {
	  rawkey = keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
        return 1;
      } else {
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval);
      }

#ifdef __SASDebugPrint__
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

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
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
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

#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif

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
#if 1
  failures += sassim_index_test1 ();
#endif
#if 1
  failures += sassim_index_test2 ();
#endif
#if 1
  failures += sassim_index_test_split ();
#endif
  // for int64 keys
#if 1
  failures += sassim_index_test3 ();
#endif
  // for double keys
#if 1
  failures += sassim_index_test4 ();
#endif
#if 1
  failures += sassim_index_test5 ();
#endif
  //SASCleanUp();
  printf("SAS removed\n");
  SASRemove();

  return failures;
}
