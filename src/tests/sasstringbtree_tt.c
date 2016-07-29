/*
 * Copyright (c) 2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe     - initial API and implementation
 *     IBM corporation, Adhemerval Zanella - tests organization
 *     IBM Corporation, Steven Munroe     - Tests for nolock APIs
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
#include "sasstringbtreeenum.h"
#include "sasstringbtreepriv.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

//#define __SASDebugPrint__ 1

static const char sassim_prog_name[] = "sasstringBTree_tt";

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
static inline void
sasbtree_print_node (SASStringBTreeNode_t node)
{
  int i, n;
  n = SASStringBTreeNodeGetCount (node);
  printf (" node@%p {", node);
  for (i = 0; i <= n; ++i)
    {
      SASStringBTreeNode_t br;
      const char *key = SASStringBTreeNodeGetKeyIndexed (node, i);
      if (key)
	{
	  printf ("(%s : %s)", SASStringBTreeNodeGetKeyIndexed (node, i),
		  (char *) SASStringBTreeNodeGetValIndexed (node, i));
	}
      if ((br = SASStringBTreeNodeGetBranchIndexed (node, i)))
	sasbtree_print_node (br);
      if (i != n)
	printf (", ");
    }
  printf ("}");
}

static inline void
sasbtree_print (SASStringBTree_t btree)
{
  SASStringBTreeNode_t root = SASStringBTreeGetRootNode (btree);
  if (root)
    {
      printf ("{");
      sasbtree_print_node (root);
      printf ("}");
    }
  printf ("\n");
}
#endif

#define JOIN_EXIT_FAILURE 128

#ifdef LONGCHECK
# define LARGE_KEY_COUNT 1000000
#else
# define LARGE_KEY_COUNT 100000
#endif

static int
sassim_index_random ()
{
  SASStringBTree_t index;
  unsigned long blockSize = block__Size1M;
  SASStringBTreeEnum_t senum;
  char *temp1;
  char *temp2;
  int i, l;
  long modcnt;
  int rc1;
  unsigned long long rawkey;
  char *keyref, *keyref2;
  char *keylist[LARGE_KEY_COUNT];
  char *keybuf;
  void *keyval, *keyval2;
  unsigned int prime_rng;

  sphtimer_t tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  keybuf = malloc (LARGE_KEY_COUNT * 32);

  index = SASStringBTreeCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d",
		    SASStringBTreeIsEmpty (index));
  sasbtree_print (index);
#endif

  prime_rng = 13523;

  keyref = keybuf;
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {				/* Turns out random does not guarantee unique.  :(
				   So using a cycle based on primes.  */
//        rawkey = nrand48 (seed);
      rawkey = prime_rng;
      l = snprintf (keyref, 32, "%016llx", rawkey);
      if (!l)
	{
	  SASSIM_PRINT_MSG ("snprintf (@%p, %016llx) success", keyref,
			    rawkey);
	}
      keylist[i] = keyref;
      keyref += 32;
      prime_rng += 17389;
    }

  SASSIM_PRINT_MSG ("\nRandom SASStringBTreePut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq = (double) freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %llx, %p)", index, rawkey,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
#endif
    }
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif
  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
		    startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG ("\nSASStringBTreePut X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  temp1 = SASStringBTreeGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%llx",
		    SASStringBTreeKeyReturn1stUInt64 (temp1));
#endif
  temp2 = SASStringBTreeGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%llx",
		    SASStringBTreeKeyReturn1stUInt64 (temp2));
#endif
  modcnt = SASStringBTreeGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nRandom SASStringBTreeGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = SASStringBTreeGet (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %llx) failed",
			    index, rawkey);
	  return 1;
	}
      if (keyval2 != keyval)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %llx) %p != %p",
			    index, rawkey, keyval2, keyval);
	  return 1;

	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeGet X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum of Random");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#if 0
  if (*keyref != 0ULL)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%llx expected 0",
			senum, *keyref);
      return 1;
    }
#endif
#endif
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %llx", keyref, *keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nRandom SASStringBTreeRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = (char *) SASStringBTreeRemove (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
	  return 1;
	}
      else
	{
#if __SASDebugPrint__ > 2
	  SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
#endif
	}

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeRemove X %ld ave= %6.2fns rate=%10.1f/s\n", p10, nano,
     rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  rc1 = SASStringBTreeIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nRandom refill SASStringBTreePut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %llx, %p)", index, rawkey,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut refill X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum of Random");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#if 0
  if (*keyref != 0ULL)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
#endif
#endif
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
	  return 1;
	}
      i++;
    }
  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);
  SASStringBTreeEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASStringBTreeDestroy (index);
  free (keybuf);

  return 0;
}

#if 1
static int
sassim_index_random_nolock ()
{
  SASStringBTree_t index;
  unsigned long blockSize = block__Size1M;
  SASStringBTreeEnum_t senum;
  char *temp1;
  char *temp2;
  int i, l;
  long modcnt;
  int rc1;
  unsigned long long rawkey;
  char *keyref, *keyref2;
  char *keylist[LARGE_KEY_COUNT];
  char *keybuf;
  void *keyval, *keyval2;
  unsigned int prime_rng;

  sphtimer_t tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  keybuf = malloc (LARGE_KEY_COUNT * 32);

  index = SASStringBTreeCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d",
		    SASStringBTreeIsEmpty (index));
  sasbtree_print (index);
#endif

  prime_rng = 13523;

  keyref = keybuf;
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {				/* Turns out random does not guarantee unique.  :(
				   So using a cycle based on primes.  */
//        rawkey = nrand48 (seed);
      rawkey = prime_rng;
      l = snprintf (keyref, 32, "%016llx", rawkey);
      if (!l)
	{
	  SASSIM_PRINT_MSG ("snprintf (@%p, %016llx) success", keyref,
			    rawkey);
	}
      keylist[i] = keyref;
      keyref += 32;
      prime_rng += 17389;
    }

  SASSIM_PRINT_MSG ("\nRandom SASStringBTreePut_nolock");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq = (double) freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut_nolock (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
#endif
    }
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif
  endt = sphgettimer ();
  tempt = endt - startt;
  clock = tempt;
  nano = (clock * 1000000000.0) / freq;
  nano = nano / p10;
  rate = p10 / (clock / freq);

  SASSIM_PRINT_MSG ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld",
		    startt, endt, tempt, freqt);

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  temp1 = SASStringBTreeGetMinKey_nolock (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", (temp1));
#endif
  temp2 = SASStringBTreeGetMaxKey_nolock (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", (temp2));
#endif
  modcnt = SASStringBTreeGetModCount_nolock (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nRandom SASStringBTreeGet_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = SASStringBTreeGet_nolock (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) failed",
			    index, keyref);
	  return 1;
	}
      if (keyval2 != keyval)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) %p != %p",
			    index, keyref, keyval2, keyval);
	  return 1;

	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeGet_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum of Random");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#if 0
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
#endif
#endif
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeEnum_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nRandom SASStringBTreeRemove_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = (char *) SASStringBTreeRemove_nolock (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
	  return 1;
	}
      else
	{
#if __SASDebugPrint__ > 2
	  SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
#endif
	}

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeRemove_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  rc1 = SASStringBTreeIsEmpty_nolock (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nRandom refill SASStringBTreePut_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut_nolock (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut_nolock refill X %ld ave= %6.2fns rate=%10.1f/s\n",
     p10, nano, rate);
#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum_nolock of Random");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#if 0
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
#endif
#endif
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
	  return 1;
	}
      i++;
    }
  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeEnum_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);
  SASStringBTreeEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASStringBTreeDestroy (index);
  free (keybuf);

  return 0;
}
#endif
static int
sassim_index_sequential ()
{
  SASStringBTree_t index;
  unsigned long blockSize = block__Size1M;
  SASStringBTreeEnum_t senum;
  int i, l;
  long modcnt;
  int rc1;
  unsigned long long rawkey;
  char *temp1, *temp2;
  char *keyref, *keyref2;
  char *keylist[LARGE_KEY_COUNT];
  char *keybuf;
  void *keyval, *keyval2;

  sphtimer_t tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  keybuf = malloc (LARGE_KEY_COUNT * 32);

  index = SASStringBTreeCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d",
		    SASStringBTreeIsEmpty (index));
  sasbtree_print (index);
#endif
  keyref = keybuf;
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      rawkey = i;
      l = snprintf (keyref, 32, "%016llx", rawkey);
      if (!l)
	{
	  SASSIM_PRINT_MSG ("snprintf (@%p, %016llx)->%s success", keyref,
			    rawkey, keyref);
	}
      keylist[i] = keyref;
      keyref += 32;
    }

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreePut");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq = (double) freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = (char *) keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreePut X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  temp1 = SASStringBTreeGetMinKey (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%llx",
		    SASStringBTreeKeyReturn1stUInt64 (temp1));
#endif
  temp2 = SASStringBTreeGetMaxKey (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%llx",
		    SASStringBTreeKeyReturn1stUInt64 (temp2));
#endif
  modcnt = SASStringBTreeGetModCount (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeGet");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = SASStringBTreeGet (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) failed",
			    index, keyref);
	  return 1;
	}
      if (keyval2 != keyval)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) %p != %p",
			    index, keyref, keyval2, keyval);
	  return 1;

	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeGet X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeEnum");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeRemove");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = (char *) SASStringBTreeRemove (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
	  return 1;
	}
      else
	{
#if __SASDebugPrint__ > 2
	  SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
#endif
	}

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeRemove X %ld ave= %6.2fns rate=%10.1f/s\n", p10, nano,
     rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  rc1 = SASStringBTreeIsEmpty (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nSequential refill SASStringBTreePut");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut refill X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeEnum");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG ("\nSASStringBTreeEnum X %ld ave= %6.2fns rate=%10.1f/s\n",
		    p10, nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASStringBTreeDestroy (index);
  free (keybuf);

  return 0;
}

#if 1
static int
sassim_index_sequential_nolock ()
{
  SASStringBTree_t index;
  unsigned long blockSize = block__Size1M;
  SASStringBTreeEnum_t senum;
  int i, l;
  long modcnt;
  int rc1;
  unsigned long long rawkey;
  char *temp1, *temp2;
  char *keyref, *keyref2;
  char *keylist[LARGE_KEY_COUNT];
  char *keybuf;
  void *keyval, *keyval2;

  sphtimer_t tempt, startt, endt, freqt;
  double clock, nano, rate, freq;
  unsigned long int p10;

  keybuf = malloc (LARGE_KEY_COUNT * 32);

  index = SASStringBTreeCreate (blockSize);
  if (!index)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeCreate(%zu)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeCreate (%lu) success", blockSize);
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#ifdef __SASDebugPrint__
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d",
		    SASStringBTreeIsEmpty (index));
  sasbtree_print (index);
#endif
  keyref = keybuf;
  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      rawkey = i;
      l = snprintf (keyref, 32, "%016llx", rawkey);
      if (!l)
	{
	  SASSIM_PRINT_MSG ("snprintf (@%p, %016llx)->%s success", keyref,
			    rawkey, keyref);
	}
      keylist[i] = keyref;
      keyref += 32;
    }

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreePut_nolock");

  /* Set up high perf timers */
  p10 = LARGE_KEY_COUNT;
  freqt = sphfastcpufreq ();
  freq = (double) freqt;

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut_nolock (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  temp1 = SASStringBTreeGetMinKey_nolock (index);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMinKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey(%p)", temp1);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMinKey=%s", temp1);
#endif
  temp2 = SASStringBTreeGetMaxKey_nolock (index);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetMaxKey (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=(%p)", temp2);
  SASSIM_PRINT_MSG ("SASStringBTreeGetMaxKey=%s", temp2);
#endif
  modcnt = SASStringBTreeGetModCount_nolock (index);
  if (modcnt == 0L)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeGetModCount (%p)", index);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeGetModCount=%ld", modcnt);
#endif
  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeGet_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = SASStringBTreeGet_nolock (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) failed",
			    index, keyref);
	  return 1;
	}
      if (keyval2 != keyval)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeGet (%p, %s) %p != %p",
			    index, keyref, keyval2, keyval);
	  return 1;

	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeGet_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeEnum_nolock");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeEnum_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeRemove_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      keyval2 = (char *) SASStringBTreeRemove_nolock (index, keyref);
      if (!keyval2)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
	  return 1;
	}
      else
	{
#if __SASDebugPrint__ > 2
	  SASSIM_PRINT_MSG ("SASStringBTreeRemove (%p, %s) = %p", index,
			    keyref, keyval2);
#endif
	}

#if __SASDebugPrint__ > 2
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeRemove_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  rc1 = SASStringBTreeIsEmpty_nolock (index);
  if (!rc1)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeIsEmpty(index) = %d", rc1);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("SASStringBTreeIsEmpty(index) = %d", rc1);

  SASSIM_PRINT_MSG ("SASStringBTreeFreeSpace() = %zu",
		    SASStringBTreeFreeSpace (index));
#endif
  SASSIM_PRINT_MSG ("\nSequential refill SASStringBTreePut_nolock");

  startt = sphgettimer ();

  for (i = 0; i < LARGE_KEY_COUNT; i++)
    {
      keyref = keylist[i];
      keyval = keylist[i];
      rc1 = SASStringBTreePut_nolock (index, keyref, keyval);
      if (!rc1)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreePut (%p, %s, %p)", index, keyref,
			    keyval);
	  return 1;
	}
#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("SASStringBTreePrint=");
      sasbtree_print (index);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreePut_nolock refill X %ld ave= %6.2fns rate=%10.1f/s\n",
     p10, nano, rate);

#ifdef __SASDebugPrint__
  SASSIM_PRINT_MSG ("SASStringBTreePrint=");
  sasbtree_print (index);
#endif

  senum = SASStringBTreeEnumCreate (index);
  if (!senum)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumCreate (%p)", index);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASStringBTreeEnumCreate(%p)", index);

  SASSIM_PRINT_MSG ("\nSequential SASStringBTreeEnum_nolock");

  startt = sphgettimer ();

  keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
  if (!keyref)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
      return 1;
    }
#ifdef SPH_TIMERTEST_VERIFY
  SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
  if (strcmp (keyref, "0000000000000000") != 0)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p)=%s expected 0",
			senum, keyref);
      return 1;
    }
  keyref2 = keyref;
  i = 1;

  while (SASStringBTreeEnumHasMore (senum))
    {
      keyref = (char *) SASStringBTreeEnumNext_nolock (senum);
      if (!keyref)
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext (%p)", senum);
	  return 1;
	}

#if __SASDebugPrint__ > 1
      SASSIM_PRINT_MSG ("<%p> %s", keyref, keyref);
#endif
      if (strcmp (keyref2, keyref) < 0)
	keyref2 = keyref;
      else
	{
	  SASSIM_PRINT_ERR ("SASStringBTreeEnumNext(%p) ordering %s>=%s",
			    senum, keyref2, keyref);
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

  SASSIM_PRINT_MSG
    ("\nSASStringBTreeEnum_nolock X %ld ave= %6.2fns rate=%10.1f/s\n", p10,
     nano, rate);

  if (i != LARGE_KEY_COUNT)
    {
      SASSIM_PRINT_ERR ("SASStringBTreeEnum interations count %d expected %d",
			i, LARGE_KEY_COUNT);
      return 1;
#ifdef SPH_TIMERTEST_VERIFY
    }
  else
    {
      SASSIM_PRINT_MSG ("SASStringBTreeEnum interations %d", i);
#endif
    }
  SASStringBTreeEnumDestroy (senum);

#if __SASDebugPrint__ > 2
  SASSIM_DUMP_BLOCK (index, 128);
#endif
  SASStringBTreeDestroy (index);
  free (keybuf);

  return 0;
}
#endif
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
  printf ("SAS removed\n");
  SASRemove ();

  return failures;
}
