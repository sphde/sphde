/*
 * Copyright (c) 2009, 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe     - initial API and implementation
 *     IBM Corporation, Steven Munroe     - Refactor and expanded
 *     										CompoundHeap tests.
 *
 * sascompoundheap_t.c
 *
 *  Created on: May 28, 2016
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasshm.h"
#include "sasalloc.h"
#include "sasstdio.h"
//#include "sasmsync.h"
//#include "sasstname.h"
#include "sassim.h"
#include "sassimpleheap.h"
#include "sassimplestack.h"
#include "sassimplespace.h"
#include "sphcompoundpcqheap.h"
#include "sascompoundheap.h"
//#include "sasstringbtree.h"
//#include "sasstringbtreeenum.h"
//#include "sasindexkey.h"
//#include "sasindex.h"
//#include "sasindexenum.h"
//#include "sphcontext.h"
//#include "sphlflogentry.h"
//#include "sphlflogger.h"
#include "sphthread.h"
#include "sphtimer.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

#ifdef LONGCHECK
# define LOOPMAX 23040
#else
# define LOOPMAX 2304
#endif
static const char sassim_prog_name[] = "sascompoundheap_t";

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

#define JOIN_EXIT_FAILURE 128

static int
sassim_compound_heap_test1 ()
{
  SASCompoundHeap_t compoundHeap =
    (char *) __SAS_TEMP_ADDRESS + __SAS_SHMAP_MAX + __SAS_SHMAP_MAX;
  SASSimpleHeap_t temp1;
  SASSimpleHeap_t temp2;
  SASSimpleHeap_t simpleHeap2;
  char *temp3, *temp4, *temp5, *temp6;
  sasshm_t lock_mem = 0;
  key_t test_heap = 1234;

  lock_mem = SASAllocateShmID (test_heap, compoundHeap, __SAS_SHMAP_MAX);
  if (lock_mem == -1)
    {
      SASSIM_PRINT_ERR ("SASAllocateShmID(%d, %p, __SAS_SHMAP_MAX)",
			test_heap, compoundHeap);
      return 1;
    }
  memset (compoundHeap, 0, __SAS_SHMAP_MAX);
  compoundHeap =
    SASCompoundHeapInit (compoundHeap, __SAS_SHMAP_MAX, block__Size512, 0);
  SASSIM_PRINT_MSG ("SASCompoundHeapInit (%p, %ld) success", compoundHeap,
		    __SAS_SHMAP_MAX);
  SASSIM_PRINT_MSG ("sizeof(FreeNode) = %zu", sizeof (FreeNode));
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  SASSIM_PRINT_MSG ("SASCompoundHeapAllocSize() = %zu",
		    SASCompoundHeapAllocSize (compoundHeap));
  SASSIM_DUMP_BLOCK (compoundHeap, 160);
  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASCompoundHeapAlloc (%p) = %p", compoundHeap, temp1);
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));
  SASSIM_PRINT_MSG ("SASSimpleHeapEmpty(%p) = %i", temp1,
		    SASSimpleHeapEmpty (temp1));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp3 = (char *) SASSimpleHeapAlloc (temp1, 8);
  if (!temp3)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(%p, %d)", temp1, 8);
      return 1;
    }
  sprintf (temp3, "heap_%d", 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", temp1, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));
  simpleHeap2 = SASSimpleHeapNearFind (temp3);
  SASSIM_PRINT_MSG ("SASSimpleHeapNearFind(%p) = %p", temp3, simpleHeap2);
  SASSIM_PRINT_MSG ("SASSimpleHeapEmpty(%p) = %i", temp1,
		    SASSimpleHeapEmpty (temp1));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp4 = (char *) SASSimpleHeapNearAlloc (temp1, 8);
  if (!temp4)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p, %d)", temp1, 8);
      return 1;
    }
  sprintf (temp4, "near_%d", 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapNearAlloc (%p, %d) = %p", temp1, 8, temp4);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp2 = SASCompoundHeapNearAlloc (temp4);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("SASCompoundHeapNearAlloc (%p) = %p", temp4, temp2);
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp5 = (char *) SASSimpleHeapAlloc (temp2, 12);
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(%p, %d)", temp2, 12);
      return 1;
    }
  sprintf (temp5, "heap_%d", 12);
  SASSIM_PRINT_MSG ("SASSimpleHeapAlloc (%p, %d) success", temp2, 8);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp6 = (char *) SASSimpleHeapNearAlloc (temp2, 20);
  if (!temp6)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p, %d)", temp2, 20);
      return 1;
    }
  sprintf (temp6, "near_%d", 20);
  SASSIM_PRINT_MSG ("SASSimpleHeapNearAlloc (%p, %d) = %p", temp2, 8, temp6);
  SASSIM_PRINT_MSG ("SASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  SASCompoundHeapFree (compoundHeap, temp1);
  SASSIM_PRINT_MSG ("SASCompoundHeapFree (%p, %p) successful", compoundHeap,
		    temp1);
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  SASCompoundHeapNearDealloc (temp6);
  SASSIM_PRINT_MSG ("SASCompoundHeapNearDealloc (%p) successful", temp6);
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  SASSIM_PRINT_MSG ("SASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  SASDetachShm (compoundHeap);
  SASRemoveShmID (lock_mem);
  return 0;
}

static int
sassim_compound_heap_test2 ()
{
  SASCompoundHeap_t compoundHeap;
  SASCompoundHeap_t expandHeap;
  SASSimpleHeap_t temp1;
  SASSimpleHeap_t temp2, simpleHeap2;
  unsigned long blockSize = block__Size64K;
  char *temp3, *temp4, *temp5, *temp6;
  SASSimpleHeap_t temp7;
  int i;

  compoundHeap = SASCompoundHeapCreate (blockSize);
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapInit (%ld) success", blockSize);
  SASSIM_PRINT_MSG ("\n\tsizeof(FreeNode) = %zu", sizeof (FreeNode));
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p", compoundHeap, temp1);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapEmpty(%p) = %d", temp1,
		    SASSimpleHeapEmpty (temp1));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp3 = (char *) SASSimpleHeapAlloc (temp1, 8);
  if (!temp3)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(%p, %d)", temp1, 8);
      return 1;
    }
  sprintf (temp3, "heap_%d", 8);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapAlloc (%p, %d) success", temp1, 8);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));
  simpleHeap2 = SASSimpleHeapNearFind (temp3);
  if (!simpleHeap2)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearFind(%p)", temp3);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapNearFind(%p) = %p", temp3, simpleHeap2);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapEmpty(%p) = %d", temp1,
		    SASSimpleHeapEmpty (temp1));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp4 = (char *) SASSimpleHeapNearAlloc (temp1, 8);
  if (!temp4)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p %d)", temp1, 8);
      return 1;
    }
  sprintf (temp4, "near_%d", 8);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapNearAlloc (%p, %d) = %p", temp1, 8, temp4);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp1));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  temp2 = SASCompoundHeapNearAlloc (temp4);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p", temp4, temp2);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
#endif
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp5 = (char *) SASSimpleHeapAlloc (temp2, 12);
  if (!temp5)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapAlloc(%p, %d)", temp2, 12);
      return 1;
    }
  sprintf (temp5, "heap_%d", 12);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapAlloc (%p, %d) success", temp2, 12);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp6 = (char *) SASSimpleHeapNearAlloc (temp2, 20);
  if (!temp6)
    {
      SASSIM_PRINT_ERR ("SASSimpleHeapNearAlloc(%p)", temp2, 20);
      return 1;
    }
  sprintf (temp6, "near_%d", 20);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapNearAlloc (%p, %d) = %p", temp2, 20, temp6);
  SASSIM_PRINT_MSG ("\n\tSASSimpleHeapFreeSpace() = %zu",
		    SASSimpleHeapFreeSpace (temp2));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp7 = SASCompoundHeapNearAlloc (compoundHeap);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #1", compoundHeap,
		    temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  SASCompoundHeapNearDealloc (temp6);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearDealloc (%p)", temp6);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  SASCompoundHeapFree (compoundHeap, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFree (%p, %p)", compoundHeap, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp2);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp2);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #2", temp2, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp4);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #3", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp5);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp5);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #4", temp5, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp6);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp6);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #5", temp6, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp7);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp7);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #6", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp4);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #7", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #8", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #9", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  temp6 = (char *) temp7;
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #10", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp4);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #11", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp4);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #12", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp6);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp6);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #13", temp6, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp7 = SASCompoundHeapNearAlloc (temp4);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapNearAlloc(%p)", temp4);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearAlloc (%p) = %p #14", temp4, temp7);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  SASCompoundHeapFree (compoundHeap, temp1);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFree (%p, %p) successful ", compoundHeap,
		    temp1);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (temp1, 128);
#endif
  SASCompoundHeapNearDealloc (temp6);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapNearDealloc (%p) successful ", temp6);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (temp2, 128);
#endif
  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #15", compoundHeap,
		    temp1);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  temp2 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp2)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #16", compoundHeap,
		    temp2);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  for (i = 17; i < 23; ++i)
    {
      temp7 = SASCompoundHeapAlloc (compoundHeap);
      if (!temp7)
	{
	  SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	  return 1;
	}
      SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #%d", compoundHeap,
			temp7, i);
      SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
			SASCompoundHeapFreeSpace (compoundHeap));
    }
  expandHeap = SASCompoundHeapExpandCreate (compoundHeap);
  if (!expandHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapExpandCreate(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapExpandCreate (%p) success", expandHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu", expandHeap,
		    SASCompoundHeapFreeSpace (expandHeap));
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu", compoundHeap,
		    SASCompoundHeapFreeSpace (compoundHeap));
#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (expandHeap, 128);
#endif
  expandHeap = SASCompoundHeapExpandCreate (compoundHeap);
  if (!expandHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapExpandCreate(%p)", compoundHeap);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapExpandCreate (%p) success ", expandHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (expandHeap));
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu", compoundHeap,
		    SASCompoundHeapFreeSpace (compoundHeap));

#ifdef DEBUG_PRINT
  SASSIM_DUMP_BLOCK (compoundHeap, 128);
  SASSIM_DUMP_BLOCK (expandHeap, 128);
#endif
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    SASCompoundHeapFreeSpace (compoundHeap));
  for (i = 23; i < 40; ++i)
    {
      temp7 = SASCompoundHeapAlloc (compoundHeap);
      if (!temp7)
	{
	  SASSIM_PRINT_ERR ("\n\tSASCompoundHeapAlloc(%p)", compoundHeap);
	  return 1;
	}
      SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAlloc (%p) = %p #%d", compoundHeap,
			temp7, i);
      SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
			SASCompoundHeapFreeSpace (compoundHeap));
    }
  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}

static int
sassim_compound_heap_test3 ()
{
  SASCompoundHeap_t compoundHeap;
  SASSimpleHeap_t temp1;
  unsigned long blockSize = block__Size64K;
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0;
  int i;

  compoundHeap = SASCompoundHeapCreate (blockSize);
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + alloc_size;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                      compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=0; i < LOOPMAX; i++)
  {
	  temp1 = SASCompoundHeapAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}

static int
sassim_compound_heap_test3B ()
{
  SASCompoundHeap_t compoundHeap;
  SASSimpleHeap_t temp1, temp2;
  unsigned long blockSize = block__Size64K;
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0, temp_free;
  int i;

//  compoundHeap = SASCompoundHeapCreate (blockSize);
  compoundHeap = SASCompoundHeapCreatePageSize (blockSize, (1024));
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + 4096;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                      compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=0; i < LOOPMAX; i++)
  {
	  temp1 = SASCompoundHeapAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp2 = SASCompoundHeapAlloc (compoundHeap);

  temp_free = SASCompoundHeapFreeSpace (compoundHeap);

  temp_b0 = temp_free + alloc_size;
  if (temp_b0 != cur_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, cur_free, temp_free, alloc_size);
      return 1;
  }

  temp_b0 = (block_size_t)temp1 + alloc_size;
  if (temp_b0 != (block_size_t)temp2)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p) %p != %p + %zu",
                       compoundHeap, temp2, temp1, alloc_size);
      return 1;
  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}

static int
sassim_compound_heap_test3C ()
{
  SASCompoundHeap_t compoundHeap;
  SASSimpleHeap_t temp1, temp2;
  unsigned long blockSize = block__Size64K;
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0, temp_free;
  int i;

//  compoundHeap = SASCompoundHeapCreate (blockSize);
  compoundHeap = SASCompoundHeapCreatePageSize (blockSize, (512));
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + 4096;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                      compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=0; i < LOOPMAX; i++)
  {
	  temp1 = SASCompoundHeapAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp2 = SASCompoundHeapAlloc (compoundHeap);

  temp_free = SASCompoundHeapFreeSpace (compoundHeap);

  temp_b0 = temp_free + alloc_size;
  if (temp_b0 != cur_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, cur_free, temp_free, alloc_size);
      return 1;
  }

  temp_b0 = (block_size_t)temp1 + alloc_size;
  if (temp_b0 != (block_size_t)temp2)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p) %p != %p + %zu",
                       compoundHeap, temp2, temp1, alloc_size);
      return 1;
  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}
/* Only build / run these test for 64-bit machines.  */
#ifdef __LP64__
static int
sassim_compound_heap_test4 ()
{
  SASCompoundHeap_t compoundHeap;
  SASSimpleHeap_t temp1;
  unsigned long blockSize = SegmentSize;
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0;
  int i;

  compoundHeap = SASCompoundHeapCreatePageSize (blockSize, (64 * 1024));
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + alloc_size;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                       compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=1; i < LOOPMAX /*100000*/; i++)
  {
	  temp1 = SASCompoundHeapAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}
#endif

static int
sassim_compound_heap_test4B ()
{
  SASCompoundHeap_t compoundHeap;
  SASSimpleHeap_t temp1;
  unsigned long blockSize = (64 * 1024);
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0;
  int i;

  compoundHeap = SASCompoundHeapCreatePageSize (blockSize, (8 * 1024));
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + alloc_size;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                       compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SASCompoundHeapAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=1; i < LOOPMAX /*100000*/; i++)
  {
	  temp1 = SASCompoundHeapAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SASCompoundHeapAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}

static int
sassim_compound_heap_test5 ()
{
  SASCompoundHeap_t compoundHeap;
  SPHSinglePCQueue_t temp1, temp2;
  unsigned long blockSize = block__Size64K;
  SPHSinglePCQueue_t temp7;
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t cur_free, cur_free2, cur_alloc;
  block_size_t temp_b0;
  int i;

  compoundHeap = SASCompoundHeapCreate (blockSize);
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) success", blockSize);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  temp_b0 = init_free + alloc_size;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                       compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SPHCompoundPCQAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }
  /* at 75% load factor, allocate enough entries to allocate 2048 blocks */
  for (i=0; i < LOOPMAX; i++)
  {
	  temp2 = SPHCompoundPCQAlloc (compoundHeap);
	  if (!temp2)
	    {
	      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  cur_free2  = cur_free;

  temp7 = SPHCompoundPCQNearAlloc (temp1);
  if (!temp7)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQNearAlloc(%p)", temp1);
      return 1;
    }

  temp_b0 = (block_size_t)temp1 ^ (block_size_t)temp7;
  temp_b0 = temp_b0 & (-blockSize);
  if (temp_b0)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQNearAlloc(%p) = %p not near (%zu)",
                       temp1, temp7, temp_b0);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSPHCompoundPCQNearAlloc(%p) = %p",
		  temp1, temp7);

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);

  if (cur_free != (cur_free2 - alloc_size))
  {
      SASSIM_PRINT_ERR ("SPHCompoundPCQNearAlloc(%p) free is %zu not %zu",
                       temp1, cur_free, (cur_free2 - alloc_size));
      return 1;
  }

  SPHCompoundPCQFree (compoundHeap, temp1);

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu after free",
		    cur_free);

  if (cur_free != cur_free2)
  {
      SASSIM_PRINT_ERR ("SPHCompoundPCQFree(%p) free is %zu not %zu",
                       temp1, cur_free, cur_free2);
      return 1;
  }

  SASCompoundHeapDestroy (compoundHeap);
  return 0;
}
/* Only build / run these test for 64-bit machines.  */
#ifdef __LP64__
static int
sassim_compound_heap_test6 ()
{
  SASCompoundHeap_t compoundHeap, smallcompoundHeap, largecompoundHeap;
  SPHSinglePCQueue_t temp1;
  unsigned long blockSize_l = SegmentSize;
  unsigned long blockSize_s = (1024 * 1024);
  unsigned long blockSize = (16 * 1024 * 1024);
  block_size_t init_alloc, init_free, alloc_size;
  block_size_t init_alloc_l, init_free_l, alloc_size_l;
  block_size_t init_alloc_s, init_free_s, alloc_size_s;
  block_size_t cur_free, cur_alloc;
  block_size_t temp_b0;
  int i;

  if (blockSize > SegmentSize)
    blockSize = SegmentSize;
  compoundHeap = SASCompoundHeapCreate (blockSize);
  if (!compoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreate(%d)", blockSize);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreate (%ld) = %p success",
		  blockSize, compoundHeap);
  init_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc);
  init_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free);
  alloc_size = SASCompoundHeapAllocSize (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size);
  /* set load factor to 100%/  */
  SASCompoundHeapSetLoadFactor (compoundHeap, 100);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (compoundHeap));

  largecompoundHeap = SASCompoundHeapCreatePageSize (blockSize_l, (64 * 1024));
  if (!largecompoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreatePageSize(%d)", blockSize_l);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreatePageSize (%ld) = %p success",
		  blockSize_l, largecompoundHeap);
  init_alloc_l = SASCompoundHeapAllocSpace (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc_l);
  init_free_l = SASCompoundHeapFreeSpace (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free_l);
  alloc_size_l = SASCompoundHeapAllocSize (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size_l);
  /* set load factor to 100%/  */
  SASCompoundHeapSetLoadFactor (largecompoundHeap, 100);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (largecompoundHeap));

  smallcompoundHeap = SASCompoundHeapCreatePageSize (blockSize_s, (1024));
  if (!smallcompoundHeap)
    {
      SASSIM_PRINT_ERR ("SASCompoundHeapCreatePageSize(%d)", blockSize_s);
      return 1;
    }
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapCreatePageSize (%ld) = %p success",
		  blockSize_s, smallcompoundHeap);
  init_alloc_s = SASCompoundHeapAllocSpace (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace() = %zu",
		    init_alloc_s);
  init_free_s = SASCompoundHeapFreeSpace (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    init_free_s);
  alloc_size_s = SASCompoundHeapAllocSize (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSize() = %zu",
            alloc_size_s);
  /* set load factor to 100%/  */
  SASCompoundHeapSetLoadFactor (smallcompoundHeap, 100);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapGetLoadFactor() = %zu",
		    SASCompoundHeapGetLoadFactor (smallcompoundHeap));

  temp_b0 = init_free + alloc_size;
  if (temp_b0 != init_alloc)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
                       compoundHeap, init_alloc, init_free, alloc_size);
      return 1;
  }

  temp1 = SPHCompoundPCQAlloc (compoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", compoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size;
  if (temp_b0 != init_free)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
                       compoundHeap, init_free, cur_free, alloc_size);
      return 1;
  }

  for (i=1; i < 3000 /*100000*/; i++)
  {
	  temp1 = SPHCompoundPCQAlloc (compoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", compoundHeap);
	      return 1;
	    }

  }

  temp_b0 = init_free_l + alloc_size_l;
  if (temp_b0 != init_alloc_l)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
    		  largecompoundHeap, init_alloc_l, init_free_l, alloc_size_l);
      return 1;
  }

  temp1 = SPHCompoundPCQAlloc (largecompoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", largecompoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size_l;
  if (temp_b0 != init_free_l)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
    		  largecompoundHeap, init_free_l, cur_free, alloc_size_l);
      return 1;
  }

  for (i=1; i < 3000 /*100000*/; i++)
  {
	  temp1 = SPHCompoundPCQAlloc (largecompoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", largecompoundHeap);
	      return 1;
	    }

  }

  temp_b0 = init_free_s + 4096;
  if (temp_b0 != init_alloc_s)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapAllocSize(%p) %zu != %zu + %zu",
    		  smallcompoundHeap, init_alloc_s, init_free_s, alloc_size_s);
      return 1;
  }

  temp1 = SPHCompoundPCQAlloc (smallcompoundHeap);
  if (!temp1)
    {
      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", smallcompoundHeap);
      return 1;
    }

  cur_free = SASCompoundHeapFreeSpace (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace() = %zu",
		    cur_free);
  temp_b0 = cur_free + alloc_size_s;
  if (temp_b0 != init_free_s)
  {
      SASSIM_PRINT_ERR ("SASCompoundHeapFreeSpace(%p) %zu != %zu + %zu",
    		  smallcompoundHeap, init_free_s, cur_free, alloc_size_s);
      return 1;
  }

  for (i=1; i < 3000 /*100000*/; i++)
  {
	  temp1 = SPHCompoundPCQAlloc (smallcompoundHeap);
	  if (!temp1)
	    {
	      SASSIM_PRINT_ERR ("SPHCompoundPCQAlloc(%p)", smallcompoundHeap);
	      return 1;
	    }

  }

  cur_alloc = SASCompoundHeapAllocSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace(%p) = %zu",
		  compoundHeap, cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (compoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu",
		  compoundHeap, cur_free);

  SASCompoundHeapDestroy (compoundHeap);

  cur_alloc = SASCompoundHeapAllocSpace (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace(%p) = %zu",
		  largecompoundHeap, cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (largecompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu",
		  largecompoundHeap, cur_free);

  SASCompoundHeapDestroy (largecompoundHeap);

  cur_alloc = SASCompoundHeapAllocSpace (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapAllocSpace(%p) = %zu",
		  smallcompoundHeap, cur_alloc);
  cur_free = SASCompoundHeapFreeSpace (smallcompoundHeap);
  SASSIM_PRINT_MSG ("\n\tSASCompoundHeapFreeSpace(%p) = %zu",
		  smallcompoundHeap, cur_free);

  SASCompoundHeapDestroy (smallcompoundHeap);
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
  failures += sassim_compound_heap_test1 ();
#endif

#if 1
  failures += sassim_compound_heap_test2 ();
#endif
  failures += sassim_compound_heap_test3 ();
  failures += sassim_compound_heap_test3B ();
  failures += sassim_compound_heap_test3C ();
#ifdef __LP64__
  failures += sassim_compound_heap_test4 ();
#endif
  failures += sassim_compound_heap_test4B ();
  failures += sassim_compound_heap_test5 ();
#ifdef __LP64__
  failures += sassim_compound_heap_test6 ();
#endif

  SASRemove ();

  return failures;
}
