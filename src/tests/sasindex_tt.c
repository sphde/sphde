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
#include "sphtimer.h"
#include "sasindexkey.h"
#include "sasindexenum.h"
#include "sasindexpriv.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sasindex_tt";

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

#ifdef __SASDebugPrint__
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
#endif

#define JOIN_EXIT_FAILURE 128
//#define __SASDebugPrint__ 1

#ifdef LONGCHECK
# define LARGE_KEY_COUNT 1000000
#else
# define LARGE_KEY_COUNT 100000
#endif

static int
sassim_index_random ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size1M;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long *keyref;
  unsigned long long keylist[LARGE_KEY_COUNT];
  void *keyval, *keyval2;
  unsigned int prime_rng;

  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
#endif

  prime_rng = 13523;

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    { /* Turns out random does not guarantee unique.  :(
         So using a cycle based on primes.  */
//	  rawkey = nrand48 (seed);
	  rawkey = prime_rng;
	  keylist[i] = rawkey;
	  prime_rng += 17389;
    }

  SASSIM_PRINT_MSG ("\nRandom SASIndexPut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq  = (double)freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif
  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
#endif
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
#endif
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nRandom SASIndexGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = SASIndexGet (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) failed",
        		index, rawkey);
        return 1;
      }
      if (keyval2 != keyval) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) %p != %p",
        		index, rawkey, keyval2, keyval);
        return 1;

      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexGet X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASIndexEnum of Random");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#if 0
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
#endif
#endif
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nRandom SASIndexRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
        return 1;
      } else {
#if __SASDebugPrint__ > 2
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
#endif
      }

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexRemove X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nRandom refill SASIndexPut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut refill X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASIndexEnum of Random");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#if 0
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
#endif
#endif
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }
  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);
  SASIndexEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_random_nolock ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size1M;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long *keyref;
  unsigned long long keylist[LARGE_KEY_COUNT];
  void *keyval, *keyval2;
  unsigned int prime_rng;

  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
#endif

  prime_rng = 13523;

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    { /* Turns out random does not guarantee unique.  :(
         So using a cycle based on primes.  */
//	  rawkey = nrand48 (seed);
	  rawkey = prime_rng;
	  keylist[i] = rawkey;
	  prime_rng += 17389;
    }

  SASSIM_PRINT_MSG ("\nRandom SASIndexPut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq  = (double)freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut_nolock (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif
  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  temp1 = SASIndexGetMinKey_nolock (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
#endif
  temp2 = SASIndexGetMaxKey_nolock (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
#endif
  modcnt = SASIndexGetModCount_nolock (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nRandom SASIndexGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = SASIndexGet_nolock (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) failed",
        		index, rawkey);
        return 1;
      }
      if (keyval2 != keyval) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) %p != %p",
        		index, rawkey, keyval2, keyval);
        return 1;

      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexGet X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASIndexEnum of Random");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#if 0
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
#endif
#endif
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nRandom SASIndexRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = (char *) SASIndexRemove_nolock (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
        return 1;
      } else {
#if __SASDebugPrint__ > 2
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
#endif
      }

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexRemove X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  rc1 = SASIndexIsEmpty_nolock (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nRandom refill SASIndexPut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut_nolock (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut refill X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASIndexEnum of Random");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#if 0
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
#endif
#endif
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }
  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);
  SASIndexEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_sequential ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size1M;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long *keyref;
  unsigned long long keylist[LARGE_KEY_COUNT];
  void *keyval, *keyval2;

  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
#endif
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = i;
	  keylist[i] = rawkey;
    }

  SASSIM_PRINT_MSG ("\nSequential SASIndexPut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq  = (double)freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  temp1 = SASIndexGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
#endif
  temp2 = SASIndexGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
#endif
  modcnt = SASIndexGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nSequential SASIndexGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = SASIndexGet (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) failed",
        		index, rawkey);
        return 1;
      }
      if (keyval2 != keyval) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) %p != %p",
        		index, rawkey, keyval2, keyval);
        return 1;

      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexGet X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASIndexEnum");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nSequential SASIndexRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = (char *) SASIndexRemove (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
        return 1;
      } else {
#if __SASDebugPrint__ > 2
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
#endif
      }

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexRemove X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  rc1 = SASIndexIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nSequential refill SASIndexPut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut refill X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASIndexEnum");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASIndexDestroy (index);

  return 0;
}

static int
sassim_index_sequential_nolock ()
{
  SASIndex_t index;
  unsigned long blockSize = block__Size1M;
  SASIndexEnum_t senum;
  SASIndexKey_t *temp1;
  SASIndexKey_t *temp2;
  SASIndexKey_t ndxkey;
  int i;
  long modcnt;
  int rc1;
  unsigned long long rawkey, rawkey2;
  unsigned long long *keyref;
  unsigned long long keylist[LARGE_KEY_COUNT];
  void *keyval, *keyval2;

  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  index = SASIndexCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASIndexCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", SASIndexIsEmpty (index));
  sasindex_print_uint (index);
#endif
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = i;
	  keylist[i] = rawkey;
    }

  SASSIM_PRINT_MSG ("\nSequential SASIndexPut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq  = (double)freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut_nolock (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  temp1 = SASIndexGetMinKey_nolock (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASIndexGetMinKey=%llx", SASIndexKeyReturn1stUInt64(temp1));
#endif
  temp2 = SASIndexGetMaxKey_nolock (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASIndexGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASIndexGetMaxKey=%llx", SASIndexKeyReturn1stUInt64(temp2));
#endif
  modcnt = SASIndexGetModCount_nolock (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASIndexGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nSequential SASIndexGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = SASIndexGet_nolock (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) failed",
        		index, rawkey);
        return 1;
      }
      if (keyval2 != keyval) {
        SASSIM_PRINT_ERR ("SASIndexGet (%p, %llx) %p != %p",
        		index, rawkey, keyval2, keyval);
        return 1;

      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexGet X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASIndexEnum");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nSequential SASIndexRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      keyval2 = (char *) SASIndexRemove_nolock (index, &ndxkey);
      if (!keyval2) {
        SASSIM_PRINT_ERR ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
        return 1;
      } else {
#if __SASDebugPrint__ > 2
        SASSIM_PRINT_MSG ("SASIndexRemove (%p, %llx) = %p", index, rawkey, keyval2);
#endif
      }

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexRemove X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  rc1 = SASIndexIsEmpty_nolock (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASIndexIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASIndexIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASIndexFreeSpace() = %zu", SASIndexFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nSequential refill SASIndexPut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
	  rawkey = keylist[i];
	  keyval = &keylist[i];
      SASIndexKeyInitUInt64 (&ndxkey, rawkey);
      rc1 = SASIndexPut_nolock (index, &ndxkey, keyval);
      if (!rc1) {
        SASSIM_PRINT_ERR ("SASIndexPut (%p, %llx, %p)", index, rawkey, keyval);
        return 1;
      }
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASIndexPrint=");
      sasindex_print_uint (index);
#endif
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexPut refill X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASIndexPrint=");
  sasindex_print_uint (index);
#endif

  senum = SASIndexEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASIndexEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASIndexEnum");

  startt = sphgettimer ();

  keyref = (unsigned long long *) SASIndexEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
  if (*keyref != 0ULL)
  {
    SASSIM_PRINT_ERR ("SASIndexEnumNext(%p)=%llx expected 0",
    		senum, *keyref);
    return 1;
  }
  rawkey2 = *keyref;
  i = 1;

  while (SASIndexEnumHasMore (senum))
    {
      keyref = (unsigned long long *) SASIndexEnumNext_nolock (senum);
      if (!keyref)
        {
          SASSIM_PRINT_ERR ("SASIndexEnumNext (%p)", senum);
          return 1;
	    }

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (rawkey2 < *keyref)
    	  rawkey2 = *keyref;
      else
      {
          SASSIM_PRINT_ERR ("SASIndexEnumNext(%p) ordering %llx>=%llx",
        		  senum, rawkey2, *keyref);
          return 1;
      }
      i++;
    }

  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
      startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASIndexEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
      p10, nano, rate);

  if ( i != LARGE_KEY_COUNT)
  {
	  SASSIM_PRINT_ERR ("SASIndexEnum interations count %d expected %d",
			  i, LARGE_KEY_COUNT);
	  return 1;
#ifdef SPH_TIMERTEST_VERIFY
  } else {
	  SASSIM_PRINT_MSG ("SASIndexEnum interations %d", i);
#endif
  }
  SASIndexEnumDestroy (senum);

#if __SASDebugPrint__ > 2
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
  failures += sassim_index_sequential ();
#endif
#if 1
  failures += sassim_index_sequential_nolock ();
#endif
#if 1
  failures += sassim_index_random ();
#endif
#if 1
  failures += sassim_index_random_nolock ();
#endif
  //SASCleanUp();
  printf("SAS removed\n");
  SASRemove();

  return failures;
}
