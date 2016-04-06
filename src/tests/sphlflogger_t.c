/*
 * Copyright (c) 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */


#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "sphtimer.h"
#include "sasconf.h"
#include "sastype.h"
#include "sassim.h"
#include "sphlflogger.h"
#include "sphlflogentry.h"

int
lflogger_basic_test (char *x4k)
{
  int rc = 0;
  SPHLFLogger_t lfLog;
  block_size_t lfspace, lf0space, lfTemp;
  block_size_t aSize;;
  char *temp0, *temp1, *temp2, *temp3;

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInit (x4k, 4096);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);
      lfspace = lf0space;

      printf ("lflogger_basic_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 128;
      temp0 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp0)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp0);
	  strcpy (temp0, "temp0");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      if (SPHLFLoggerEmpty (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) false, succeeds\n",
		  lfLog);
	}

      temp1 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp1)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp1);
	  strcpy (temp1, "temp1");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      temp2 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp2)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp2);
	  strcpy (temp2, "temp2");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      temp3 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp3)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp3);
	  strcpy (temp3, "temp3");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf ("lflogger_basic_test data verify temp0=%s failed\n", temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf ("lflogger_basic_test data verify temp1=%s failed\n", temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf ("lflogger_basic_test data verify temp2=%s failed\n", temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf ("lflogger_basic_test data verify temp3=%s failed\n", temp3);
	  return 10;
	}

      printf ("lflogger_basic_test data line 171\n");
      /* in its current state the logger neither empty or full */
      if (SPHLFLoggerEmpty (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	}
      lfspace = SPHLFLoggerFreeSpace (lfLog);

      if (SPHLFLoggerResetIfFullSync (lfLog))
	{
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace == lfTemp)
	    {
	      printf
		("lflogger_basic_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
		 x4k, lfLog, lfTemp);
	    }
	  else
	    {
	      printf
		("lflogger_basic_test SPHLFLoggerResetIfFullSync(%p) changed freespace from %zu tp %zu\n",
		 lfLog, lfspace, lfTemp);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lflogger_basic_test SPHLFLoggerResetIfFullSync(%p) failed\n",
	     lfLog);
	  rc++;
	}

      printf ("lflogger_basic_test data verify success\n");

      /* now fill the logger up */
      while (temp0)
	{
	  temp0 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
	}

      lfspace = SPHLFLoggerFreeSpace (lfLog);
      printf ("lflogger_basic_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lfspace);

      if (!SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) true, succeeds\n",
		  lfLog);
	}
    }
  else
    {
      printf
	("error lflogger_basic_test(%p)  SPHLFLoggerInit(%p, 4096) failed\n",
	 x4k, x4k);
      return 10;
    }

  printf ("lflogger_basic_test data line 235\n");
  if (!SPHLFLoggerResetIfFullSync (lfLog))
    {
      printf
	("lflogger_basic_test(%p)  SPHLFLoggerResetIfFullSync(%p) success\n",
	 x4k, lfLog);
      lfspace = SPHLFLoggerFreeSpace (lfLog);

      if (lf0space == lfspace)
	{
	  printf ("lflogger_basic_test  SPHLFLoggerFreeSpace(%p) = %zu\n",
		  lfLog, lfspace);
	}
      else
	{
	  printf
	    ("lflogger_basic_test  SPHLFLoggerFreeSpace(%p) = %zu != %zu\n",
	     lfLog, lfspace, lf0space);
	}

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 112;
      temp0 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp0)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp0);
	  strcpy (temp0, "temp0");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      if (SPHLFLoggerEmpty (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerEmpty(%p) false, succeeds\n",
		  lfLog);
	}

      temp1 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp1)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp1);
	  strcpy (temp1, "temp1");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      temp2 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp2)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp2);
	  strcpy (temp2, "temp2");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      temp3 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
      if (temp3)
	{
	  printf ("lflogger_basic_test SPHLFLoggerAllocRaw(%p, %zu) = %p\n",
		  lfLog, aSize, temp3);
	  strcpy (temp3, "temp3");
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error lflogger_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 lfLog, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_basic_test  SPHLFLoggerAllocRaw(%p,%zu) failed\n",
	     lfLog, aSize);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf ("lflogger_basic_test data verify temp0=%s failed\n", temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf ("lflogger_basic_test data verify temp1=%s failed\n", temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf ("lflogger_basic_test data verify temp2=%s failed\n", temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf ("lflogger_basic_test data verify temp3=%s failed\n", temp3);
	  return 10;
	}

      printf ("lflogger_basic_test data verify success\n");

      while (temp0)
	{
	  temp1 = temp0;
	  temp0 = (char *) SPHLFLoggerAllocRaw (lfLog, aSize);
	}

      printf ("lflogger_basic_test last @ %p\n", temp1);

      lfspace = SPHLFLoggerFreeSpace (lfLog);
      printf ("lflogger_basic_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lfspace);

      if (!SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	  printf ("lflogger_basic_test SPHLFLoggerFull(%p) true, succeeds\n",
		  lfLog);
	}
    }
  else
    {
      printf
	("error lflogger_basic_test(%p)  SPHLFLoggerResetIfFullSync(%p) failed\n",
	 x4k, x4k);
      return 10;
    }

  return rc;
}

int
lflogger_timestamp_test (char *x4k)
{
  int rc = 0;
  SPHLFLogger_t lfLog;
  block_size_t lfspace, lf0space, lfTemp;
  block_size_t aSize;
  SPHLFLogIterator_t *iter, iter0, iter1;
  SPHLFLoggerHandle_t *handle, handle0, handle1, handle2, handle3;
  SPHLFLoggerHandle_t *handlex, handle4, handle5;
  SPHLFLogHeader_t *entry;
  char *temp0, *temp1, *temp2, *temp3, *tempx;

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInit (x4k, 4096);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);
      lfspace = lf0space;

      printf ("lflogger_timestamp_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_timestamp_test SPHLFLoggerEmpty(%p) failed\n",
		  lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_timestamp_test SPHLFLoggerFull(%p) failed\n",
		  lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 128 - 16;
      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle0);
      if (handle)
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle0, handle);
	  temp0 = handle->next;
	  strcpy (temp0, "temp0");
	  printf ("lflogger_timestamp_test ->next=%p now %s\n", temp0, temp0);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16))
	    {
	      printf
		("error lflogger_timestamp_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle0);
	  return 10;
	}

      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle1);
      if (handle)
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle1, handle);
	  temp1 = handle->next;
	  strcpy (temp1, "temp1");
	  printf ("lflogger_timestamp_test ->next=%p now %s\n", temp1, temp1);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16 + aSize + 16))
	    {
	      printf
		("error lflogger_timestamp_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16 + aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle1);
	  return 10;
	}

      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle2);
      if (handle)
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle2, handle);
	  temp2 = handle->next;
	  strcpy (temp2, "temp2");
	  printf ("lflogger_timestamp_test ->next=%p now %s\n", temp2, temp2);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16 + aSize + 16 + aSize + 16))
	    {
	      printf
		("error lflogger_timestamp_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16 + aSize + 16 + aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle2);
	  return 10;
	}

      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle3);
      if (handle)
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle3, handle);
	  temp3 = handle->next;
	  strcpy (temp3, "temp3");
	  printf ("lflogger_timestamp_test ->next=%p now %s\n", temp3, temp3);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace !=
	      (lfTemp + aSize + 16 + aSize + 16 + aSize + 16 + aSize + 16))
	    {
	      printf
		("error lflogger_timestamp_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp,
		 (aSize + 16 + aSize + 16 + aSize + 16 + aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_timestamp_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle3);
	  return 10;
	}

      if (SPHLFLoggerEmpty (lfLog))
	{
	  printf ("lflogger_timestamp_test SPHLFLoggerEmpty(%p) failed\n",
		  lfLog);
	  rc++;
	}
      else
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerEmpty(%p) false, succeeds\n",
	     lfLog);
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf ("lflogger_timestamp_test data verify temp0=%s failed\n",
		  temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf ("lflogger_timestamp_test data verify temp1=%s failed\n",
		  temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf ("lflogger_timestamp_test data verify temp2=%s failed\n",
		  temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf ("lflogger_timestamp_test data verify temp3=%s failed\n",
		  temp3);
	  return 10;
	}

      printf ("lflogger_basic_test data verify success\n");

      entry = handle0.entry;
      printf ("lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
	      entry, entry->entryID.idUnit, entry->PID, entry->TID,
	      entry->timeStamp);

      entry = handle1.entry;
      printf ("lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
	      entry, entry->entryID.idUnit, entry->PID, entry->TID,
	      entry->timeStamp);

      entry = handle2.entry;
      printf ("lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
	      entry, entry->entryID.idUnit, entry->PID, entry->TID,
	      entry->timeStamp);

      entry = handle3.entry;
      printf ("lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
	      entry, entry->entryID.idUnit, entry->PID, entry->TID,
	      entry->timeStamp);

      if (SPHLFLoggerEntryIsComplete (&handle0))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle0));
	  rc++;
	}

      if (SPHLFLoggerEntryIsComplete (&handle1))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle1));
	  rc++;
	}

      if (SPHLFLoggerEntryIsComplete (&handle2))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle2));
	  rc++;
	}

      if (SPHLFLoggerEntryIsComplete (&handle3))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle3));
	  rc++;
	}

      if (SPHLFLogEntryIsComplete (&handle3))
	{
	  printf ("  SPHLFLogEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLogEntryIsComplete (&handle3));
	  rc++;
	}

      if (SPHLFLoggerEntryIsTimestamped (&handle3))
	{
	  printf ("  SPHLFLoggerEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsTimestamped (&handle3));
	  rc++;
	}

      if (SPHLFLogEntryIsTimestamped (&handle3))
	{
	  printf ("  SPHLFLoggerEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsTimestamped (&handle3));
	  rc++;
	}

      SPHLFLoggerEntryComplete (&handle0);
      SPHLFLoggerEntryComplete (&handle1);
      SPHLFLoggerEntryComplete (&handle2);
      SPHLFLoggerEntryComplete (&handle3);

      printf ("lflogger_timestamp_test data complete\n");

      entry = handle0.entry;
      if (!entry->entryID.detail.valid)
	{
	  printf ("error lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
		  entry, entry->entryID.idUnit, entry->PID, entry->TID,
		  entry->timeStamp);
	  rc++;
	}

      entry = handle1.entry;
      if (!entry->entryID.detail.valid)
	{
	  printf ("error lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
		  entry, entry->entryID.idUnit, entry->PID, entry->TID,
		  entry->timeStamp);
	  rc++;
	}

      entry = handle2.entry;
      if (!entry->entryID.detail.valid)
	{
	  printf ("error lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
		  entry, entry->entryID.idUnit, entry->PID, entry->TID,
		  entry->timeStamp);
	  rc++;
	}

      entry = handle3.entry;
      if (!entry->entryID.detail.valid)
	{
	  printf ("error lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
		  entry, entry->entryID.idUnit, entry->PID, entry->TID,
		  entry->timeStamp);
	  rc++;
	}

      if (SPHLFLoggerEntryIsComplete (&handle0))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) succeeded, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle0));
	}
      else
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsComplete (&handle0));
	  rc++;
	}

      if (SPHLFLoggerEntryIsComplete (&handle1))
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) succeeded, returned %d\n",
		  &handle1, SPHLFLoggerEntryIsComplete (&handle1));
	}
      else
	{
	  printf ("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
		  &handle1, SPHLFLoggerEntryIsComplete (&handle1));
	  rc++;
	}

      if (SPHLFLogEntryIsComplete (&handle2))
	{
	  printf ("  SPHLFLogEntryIsComplete(%p) succeeded, returned %d\n",
		  &handle2, SPHLFLogEntryIsComplete (&handle2));
	}
      else
	{
	  printf ("  SPHLFLogEntryIsComplete(%p) failed, returned %d\n",
		  &handle2, SPHLFLogEntryIsComplete (&handle2));
	  rc++;
	}

      if (SPHLFLogEntryIsComplete (&handle3))
	{
	  printf ("  SPHLFLogEntryIsComplete(%p) succeeded, returned %d\n",
		  &handle3, SPHLFLogEntryIsComplete (&handle3));
	}
      else
	{
	  printf ("  SPHLFLogEntryIsComplete(%p) failed, returned %d\n",
		  &handle3, SPHLFLogEntryIsComplete (&handle3));
	  rc++;
	}

      if (SPHLFLoggerEntryIsTimestamped (&handle0))
	{
	  printf
	    ("  SPHLFLoggerEntryIsTimestamped(%p) succeeded, returned %d\n",
	     &handle0, SPHLFLoggerEntryIsTimestamped (&handle0));
	}
      else
	{
	  printf ("  SPHLFLoggerEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle0, SPHLFLoggerEntryIsTimestamped (&handle0));
	  rc++;
	}

      if (SPHLFLoggerEntryIsTimestamped (&handle1))
	{
	  printf
	    ("  SPHLFLoggerEntryIsTimestamped(%p) succeeded, returned %d\n",
	     &handle1, SPHLFLoggerEntryIsTimestamped (&handle1));
	}
      else
	{
	  printf ("  SPHLFLoggerEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle1, SPHLFLoggerEntryIsTimestamped (&handle1));
	  rc++;
	}

      if (SPHLFLogEntryIsTimestamped (&handle2))
	{
	  printf ("  SPHLFLogEntryIsTimestamped(%p) succeeded, returned %d\n",
		  &handle2, SPHLFLogEntryIsTimestamped (&handle2));
	}
      else
	{
	  printf ("  SPHLFLogEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle2, SPHLFLogEntryIsTimestamped (&handle2));
	  rc++;
	}

      if (SPHLFLogEntryIsTimestamped (&handle3))
	{
	  printf ("  SPHLFLogEntryIsTimestamped(%p) succeeded, returned %d\n",
		  &handle3, SPHLFLogEntryIsTimestamped (&handle3));
	}
      else
	{
	  printf ("  SPHLFLogEntryIsTimestamped(%p) failed, returned %d\n",
		  &handle3, SPHLFLogEntryIsTimestamped (&handle3));
	  rc++;
	}

      printf ("lflogger_timestamp_test entry complete\n");

      iter = SPHLFLoggerCreateIterator (lfLog, &iter0);
      if (iter)
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
		  lfLog, &iter0, iter);
	}
      else
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
		  lfLog, &iter0, iter);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  iter, &handle4, handlex);
	  if (handlex->entry != handle0.entry)
	    {
	      printf
		("  SPHLFLoggerIteratorNext(%p,%p) entry mismatch expected %p, found %p\n",
		 iter, &handle4, handle0.entry, handlex->entry);
	      return (rc + 10);
	    }
	  tempx = (char *) SPHLFLogEntryGetFreePtr (handlex);

	  if (0 != strcmp (tempx, "temp0"))
	    {
	      printf
		("lflogger_timestamp_test iterator verify temp0=%s failed\n",
		 temp0);
	      return 10;
	    }
	  else
	    {
	      printf ("   SPHLFLoggerIteratorNext(%p,%p) found entry %s\n",
		      iter, &handle4, tempx);
	    }
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  iter, &handle4, handlex);
	  rc++;
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  iter, &handle4, handlex);
	  if (handlex->entry != handle1.entry)
	    {
	      printf
		("  SPHLFLoggerIteratorNext(%p,%p) entry mismatch expected %p, found %p\n",
		 iter, &handle4, handle1.entry, handlex->entry);
	      return (rc + 10);
	    }
	  tempx = (char *) SPHLFLogEntryGetFreePtr (handlex);

	  if (0 != strcmp (tempx, "temp1"))
	    {
	      printf
		("lflogger_timestamp_test iterator verify temp1=%s failed\n",
		 temp1);
	      return 10;
	    }
	  else
	    {
	      printf ("   SPHLFLoggerIteratorNext(%p,%p) found entry %s\n",
		      iter, &handle4, tempx);
	      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
		      iter->logger, iter->current, iter->free,
		      iter->start_log, iter->end_log);
	    }
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  iter, &handle4, handlex);
	  rc++;
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  iter, &handle4, handlex);
	  if (handlex->entry != handle2.entry)
	    {
	      printf
		("  SPHLFLoggerIteratorNext(%p,%p) entry mismatch expected %p, found %p\n",
		 iter, &handle4, handle2.entry, handlex->entry);
	      return (rc + 10);
	    }
	  tempx = (char *) SPHLFLogEntryGetFreePtr (handlex);

	  if (0 != strcmp (tempx, "temp2"))
	    {
	      printf
		("lflogger_timestamp_test iterator verify temp2=%s failed\n",
		 temp2);
	      return 10;
	    }
	  else
	    {
	      printf ("   SPHLFLoggerIteratorNext(%p,%p) found entry %s\n",
		      iter, &handle4, tempx);
	      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
		      iter->logger, iter->current, iter->free,
		      iter->start_log, iter->end_log);
	    }
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  iter, &handle4, handlex);
	  rc++;
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  iter, &handle4, handlex);
	  if (handlex->entry != handle3.entry)
	    {
	      printf
		("  SPHLFLoggerIteratorNext(%p,%p) entry mismatch expected %p, found %p\n",
		 iter, &handle4, handle3.entry, handlex->entry);
	      return (rc + 10);
	    }
	  tempx = (char *) SPHLFLogEntryGetFreePtr (handlex);

	  if (0 != strcmp (tempx, "temp3"))
	    {
	      printf
		("lflogger_timestamp_test iterator verify temp3=%s failed\n",
		 temp3);
	      return 10;
	    }
	  else
	    {
	      printf ("   SPHLFLoggerIteratorNext(%p,%p) found entry %s\n",
		      iter, &handle4, tempx);
	      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
		      iter->logger, iter->current, iter->free,
		      iter->start_log, iter->end_log);
	    }
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  iter, &handle4, handlex);
	  rc++;
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  iter, &handle4, handlex);
	  printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
		  iter->logger, iter->current, iter->free, iter->start_log,
		  iter->end_log);

	  rc++;
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p end found\n",
		  iter, &handle4, handlex);
	}

      printf ("lflogger_timestamp_test iterator test complete\n");

      while (handle)
	{
	  entry = handle->entry;
	  handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle3);
	  if (handle)
	    SPHLFLoggerEntryComplete (handle);
	}

      if (!entry->entryID.detail.valid)
	{
	  printf ("error lflogger_timestamp_test entry@%p [%x,%d,%d,%llu]\n",
		  entry, entry->entryID.idUnit, entry->PID, entry->TID,
		  entry->timeStamp);
	  rc++;
	}

      iter = SPHLFLoggerCreateIterator (lfLog, &iter1);
      if (iter)
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
		  lfLog, &iter1, iter);
	}
      else
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
		  lfLog, &iter1, iter);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);
      if (handlex)
	{
	  while (handlex)
	    {
	      handlex = SPHLFLoggerIteratorNext (iter, &handle5);
	    }
	}
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);

      lfspace = SPHLFLoggerFreeSpace (lfLog);
      printf ("lflogger_timestamp_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lfspace);

      if (!SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_timestamp_test SPHLFLoggerFull(%p) failed\n",
		  lfLog);
	  rc++;
	}
      else
	{
	  printf
	    ("lflogger_timestamp_test SPHLFLoggerFull(%p) true, succeeds\n",
	     lfLog);
	}
    }
  else
    {
      printf
	("error lflogger_timestamp_test(%p)  SPHLFLoggerResetAsync(%p) failed\n",
	 x4k, x4k);
      return 10;
    }

  return rc;
}

int
lflogger_wrap_test (char *x4k)
{
  int rc = 0;
  SPHLFLogger_t lfLog;
  block_size_t lfspace, lf0space, lfTemp, lfentries, lfi;
  block_size_t aSize;
  SPHLFLoggerHandle_t *handle, handle0;
  SPHLFLoggerHandle_t *handlex, handle4, handle5;
  int entry_cat, entry_sub, entry_num;
  SPHLFLogIterator_t *iter, iter0;
  char *temp0;

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInitWithStride (x4k, 4096, 128, SPHLFLOGGER_CIRCULAR);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);
      lfspace = lf0space;
      lfentries = lfspace / 128;

      printf ("lflogger_wrap_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu, %zu\n",
	      x4k, lfLog, lf0space, lfentries);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_wrap_test SPHLFLoggerEmpty(%p) failed\n", lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_wrap_test SPHLFLoggerFull(%p) failed\n", lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 128 - 16;
      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle0);
      if (handle)
	{
	  printf
	    ("lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle0, handle->entry);
	  temp0 = handle->next;
	  strcpy (temp0, "temp0");
	  printf ("lflogger_wrap_test ->next=%p now %s\n", temp0, temp0);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16))
	    {
	      printf
		("error lflogger_wrap_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16));
	      rc++;
	    }
	  SPHLFLogEntryComplete (handle);
	  printf ("lflogger_wrap_test @%p->id=%x\n",
		  handle->entry, handle->entry->entryID.idUnit);
	}
      else
	{
	  printf
	    ("error lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle0);
	  return 10;
	}
      lf0space = SPHLFLoggerFreeSpace (lfLog);

      printf ("lflogger_wrap_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      for (lfi = 0; lfi < (lfentries / 2); lfi++)
	{
	  handle = SPHLFLoggerAllocTimeStamped (lfLog,
						1, lfi, aSize, &handle0);
	  if (handle)
	    {
	      SPHLFLogEntryComplete (handle);
	      printf
		("lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,1,%zu,%zu,%p) = %p\n",
		 lfLog, lfi, aSize, &handle0, handle->entry);
	      temp0 = handle->next;
	    }
	  else
	    {
	      printf
		("error lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,1,%zu,%zu,%p) failed\n",
		 lfLog, lfi, aSize, &handle0);
	      rc++;
	    }
	}
      lf0space = SPHLFLoggerFreeSpace (lfLog);

      printf ("lflogger_wrap_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (SPHLFLoggerWrapped (lfLog))
	{
	  printf
	    ("error lflogger_wrap_test SPHLFLoggerWrapped(%p) reports wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	  rc++;
	}
      else
	{
	  printf
	    ("success lflogger_wrap_test SPHLFLoggerWrapped(%p) reports not wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	}

      iter = SPHLFLoggerCreateIterator (lfLog, &iter0);
      if (iter)
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
		  lfLog, &iter0, iter);
	}
      else
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
		  lfLog, &iter0, iter);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  lfLog, &handle4, handlex);
	  printf ("   @%p->id=%x\n",
		  handlex->entry, handlex->entry->entryID.idUnit);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  lfLog, &handle4, handlex);
	  return (rc + 10);
	}

      entry_cat = SPHLFLogEntryCategory (handlex);
      entry_sub = SPHLFLogEntrySubcat (handlex);
      if ((entry_cat == 0) && (entry_sub == 0))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, handlex);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  lfLog, &handle4, handlex);
	  printf ("   @%p->id=%x\n",
		  handlex->entry, handlex->entry->entryID.idUnit);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  lfLog, &handle4, handlex);
	  return (rc + 10);
	}

      entry_cat = SPHLFLogEntryCategory (handlex);
      entry_sub = SPHLFLogEntrySubcat (handlex);
      if ((entry_cat == 1) && (entry_sub == 0))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, handlex);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	  return (rc + 10);
	}
#if 1
      if (handlex)
	{
	  while (handlex)
	    {
	      entry_cat = SPHLFLogEntryCategory (handlex);
	      entry_sub = SPHLFLogEntrySubcat (handlex);
	      handlex = SPHLFLoggerIteratorNext (iter, &handle5);
	    }
	}
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);

      if ((entry_cat == 1) && (entry_sub == 14))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, &handle5);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	  return (rc + 10);
	}
#endif
      for (lfi = (lfentries / 2); lfi < lfentries; lfi++)
	{
	  handle = SPHLFLoggerAllocTimeStamped (lfLog,
						1, lfi, aSize, &handle0);
	  if (handle)
	    {
	      SPHLFLogEntryComplete (handle);
	      printf
		("lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,1,%zu,%zu,%p) = %p\n",
		 lfLog, lfi, aSize, &handle0, handle->entry);
	      temp0 = handle->next;
	    }
	  else
	    {
	      printf
		("error lflogger_wrap_test SPHLFLoggerAllocTimeStamped(%p,1,%zu,%zu,%p) failed\n",
		 lfLog, lfi, aSize, &handle0);
	      rc++;
	    }
	}
      lf0space = SPHLFLoggerFreeSpace (lfLog);

      printf ("lflogger_wrap_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (!SPHLFLoggerWrapped (lfLog))
	{
	  printf
	    ("error lflogger_wrap_test SPHLFLoggerWrapped(%p) reports wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	  rc++;
	}
      else
	{
	  printf
	    ("success lflogger_wrap_test SPHLFLoggerWrapped(%p) reports wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	}

      iter = SPHLFLoggerCreateIterator (lfLog, &iter0);
      if (iter)
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
		  lfLog, &iter0, iter);
	}
      else
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
		  lfLog, &iter0, iter);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  lfLog, &handle4, handlex);
	  printf ("   @%p->id=%x\n",
		  handlex->entry, handlex->entry->entryID.idUnit);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  lfLog, &handle4, handlex);
	  return (rc + 10);
	}

      entry_cat = SPHLFLogEntryCategory (handlex);
      entry_sub = SPHLFLogEntrySubcat (handlex);
      if ((entry_cat == 1) && (entry_sub == 0))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, handlex);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	  return (rc + 10);
	}
#if 1
      while (handlex)
	{
	  entry_num = entry_sub;
	  handlex = SPHLFLoggerIteratorNext (iter, &handle5);
	  if (handlex)
	    {
	      entry_cat = SPHLFLogEntryCategory (handlex);
	      entry_sub = SPHLFLogEntrySubcat (handlex);
	      if (entry_num + 1 != entry_sub)
		{
		  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
			  iter, handlex);
		  printf ("  SPHLFLogEntryCategory(%p) = %d]\n",
			  handlex, entry_cat);
		  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n",
			  handlex, entry_sub);
		  rc++;
		}
	    }
	}
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);

      if ((entry_cat == 1) && (entry_sub == 29))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, &handle5);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	  return (rc + 10);
	}
#endif

      for (lfi = 0; lfi < lfentries; lfi++)
	{
	  handle = SPHLFLoggerAllocStrideTimeStamped (lfLog,
						      1234, lfi, &handle0);
	  if (handle)
	    {
	      SPHLFLogEntryComplete (handle);
	      printf
		("lflogger_wrap_test SPHLFLoggerAllocStrideTimeStamped(%p,1234,%zu,%p) = %p\n",
		 lfLog, lfi, &handle0, handle);
	      temp0 = handle->next;
	    }
	  else
	    {
	      printf
		("error lflogger_wrap_test SPHLFLoggerAllocStrideTimeStamped(%p,1234,%zu,%p) failed\n",
		 lfLog, lfi, &handle0);
	      rc++;
	    }
	}

      if (!SPHLFLoggerWrapped (lfLog))
	{
	  printf
	    ("error lflogger_wrap_test SPHLFLoggerWrapped(%p) reports wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	  rc++;
	}
      else
	{
	  printf
	    ("success lflogger_wrap_test SPHLFLoggerWrapped(%p) reports wrapped (%zu, %p)\n",
	     lfLog, aSize, &handle0);
	}

      iter = SPHLFLoggerCreateIterator (lfLog, &iter0);
      if (iter)
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
		  lfLog, &iter0, iter);
	}
      else
	{
	  printf ("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
		  lfLog, &iter0, iter);
	  return (rc + 10);
	}

      handlex = SPHLFLoggerIteratorNext (iter, &handle4);
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);
      if (handlex)
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
		  lfLog, &handle4, handlex);
	  printf ("   @%p->id=%x\n",
		  handlex->entry, handlex->entry->entryID.idUnit);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
		  lfLog, &handle4, handlex);
	  return (rc + 10);
	}

      entry_cat = SPHLFLogEntryCategory (handlex);
      entry_sub = SPHLFLogEntrySubcat (handlex);
      if ((entry_cat == 1234) && (entry_sub == 0))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, handlex);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", handlex, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", handlex, entry_sub);
	  return (rc + 10);
	}
#if 1
      while (handlex)
	{
	  entry_num = entry_sub;
	  handlex = SPHLFLoggerIteratorNext (iter, &handle5);
	  if (handlex)
	    {
	      entry_cat = SPHLFLogEntryCategory (handlex);
	      entry_sub = SPHLFLogEntrySubcat (handlex);
	      if (entry_num + 1 != entry_sub)
		{
		  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
			  iter, handlex);
		  printf ("  SPHLFLogEntryCategory(%p) = %d]\n",
			  handlex, entry_cat);
		  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n",
			  handlex, entry_sub);
		  rc++;
		}
	    }
	}
      printf ("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
	      iter->logger, iter->current, iter->free, iter->start_log,
	      iter->end_log);

      if ((entry_cat == 1234) && (entry_sub == 29))
	{
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	}
      else
	{
	  printf ("  SPHLFLoggerIteratorNext(%p,%p) = cat mismatch\n",
		  iter, &handle5);
	  printf ("  SPHLFLogEntryCategory(%p) = %d]\n", &handle5, entry_cat);
	  printf ("  SPHLFLogEntrySubcat(%p) = %d]\n", &handle5, entry_sub);
	  return (rc + 10);
	}
#endif
    }
  else
    {
      printf
	("error lflogger_wrap_test(%p)  SPHLFLoggerResetAsync(%p) failed\n",
	 x4k, x4k);
      return 10;
    }

  return rc;
}

int
lflogentry_test_handle (SPHLFLogger_t log,
			SPHLFLoggerHandle_t * handle_in,
			int val1, int val2, int val3)
{
  int rc1, rc2, rc3;
  SPHLFLoggerHandle_t *handle;


  handle = SPHLFLoggerAllocTimeStamped (log, 123, 76, 112, handle_in);
  if (handle)
    {
      rc1 = SPHLFlogEntryAddInt (handle, val1);
      rc2 = SPHLFlogEntryAddInt (handle, val2);
      rc3 = SPHLFlogEntryAddInt (handle, val3);
      SPHLFLogEntryComplete (handle);
    }
  else
    {
      return -1;
    }

  return (rc1 | rc2 | rc3);
}



int
lflogentry_test_struct (SPHLFLogger_t log, int val1, int val2, int val3)
{
  struct data_s1
  {
    sphtimer_t f0;
    int f1;
    int f2;
    int f3;
  };

  SPHLFLoggerHandle_t *handle, handle0;
  struct data_s1 *sptr;

  handle = SPHLFLoggerAllocTimeStamped (log, 123, 76, 112, &handle0);
  if (handle)
    {
      sptr = (struct data_s1 *) SPHLFlogEntryAllocStruct (&handle0,
							  sizeof (struct
								  data_s1),
							  __alignof__ (struct
								       data_s1));
      if (sptr)
	{
	  sptr->f0 = sphgettimer ();
	  sptr->f1 = val1;
	  sptr->f2 = val2;
	  sptr->f3 = val3;
	  SPHLFLogEntryComplete (&handle0);
	}
      else
	{
	  return -1;
	}
    }
  else
    {
      return -1;
    }

  return 0;
}

int
lflogentry_test (SPHLFLogger_t log, int val1, int val2, int val3)
{
  int rc1, rc2, rc3;
  SPHLFLoggerHandle_t *handle, handle0;


  handle = SPHLFLoggerAllocTimeStamped (log, 123, 76, 112, &handle0);
  if (handle)
    {
      rc1 = SPHLFlogEntryAddInt (&handle0, val1);
      rc2 = SPHLFlogEntryAddInt (&handle0, val2);
      rc3 = SPHLFlogEntryAddInt (&handle0, val3);
      SPHLFLogEntryComplete (&handle0);
    }
  else
    {
      return -1;
    }

  return (rc1 | rc2 | rc3);
}

int
lflogger_dataadd_test (char *x4k)
{
  int rc = 0;
  SPHLFLogger_t lfLog;
  block_size_t lfspace, lf0space, lfTemp;
  block_size_t aSize;
  SPHLFLoggerHandle_t *handle, handle0, handle1, handle2;
  char *temp0, *temp1, *temp2 = NULL, *temp3 = NULL;
  int len0, len1, len2 = 0, len3 = 0;
  unsigned long adjust;
  int i;

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInit (x4k, 4096);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);
      lfspace = lf0space;

      printf ("lflogger_dataadd_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_dataadd_test SPHLFLoggerEmpty(%p) failed\n",
		  lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_dataadd_test SPHLFLoggerFull(%p) failed\n",
		  lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 128 - 16;
      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle0);
      if (handle)
	{
	  printf
	    ("lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle0, handle);
	  temp0 = handle->next;
	  len0 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp0, len0);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16))
	    {
	      printf
		("error lflogger_dataadd_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle0);
	  return 10;
	}

      printf ("lflogger_dataadd_test for SPHLFlogEntryAddChar()\n");
      if (SPHLFlogEntryAddChar (&handle0, 'a'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'a') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'b'))
	{

	  printf ("error SPHLFlogEntryAddChar(%p, 'b') failed\n", handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'c'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'c') failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if ((len0 - len3) != (3 * sizeof (char)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (short int) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddShort()\n");

      if (SPHLFlogEntryAddShort (&handle0, 1234))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 1234) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf
		("error SPHLFlogEntryAddShort alignment (%p,%lx) failed\n",
		 temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddShort (&handle0, 2345))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 2345) failed\n", handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddShort (&handle0, 3456))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 3456) failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (short int);

      if ((len0 - len3) != (3 * sizeof (short int)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (int) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddInt()\n");

      if (SPHLFlogEntryAddInt (&handle0, 123456))
	{
	  printf ("error SPHLFlogEntryAddInt(%p, 123456) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf ("error SPHLFlogEntryAddInt alignment (%p,%lx) failed\n",
		      temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddInt (&handle0, 234567))
	{
	  printf ("error SPHLFlogEntryAddInt(%p, 234567) failed\n", handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddInt (&handle0, 345678))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 345678) failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (int);

      if ((len0 - len3) != (3 * sizeof (int)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (long) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddLong()\n");

      if (SPHLFlogEntryAddLong (&handle0, 1234567L))
	{
	  printf ("error SPHLFlogEntryAddLong(%p, 1234567L) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf
		("error SPHLFlogEntryAddLong alignment (%p,%lx) failed\n",
		 temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddLong (&handle0, 2345678L))
	{
	  printf ("error SPHLFlogEntryAddLong(%p, 2345678L) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      printf ("lflogger_dataadd_test for SPHLFlogEntryAddPtr()\n");

      if (SPHLFlogEntryAddPtr (&handle0, &handle0))
	{
	  printf ("error SPHLFlogEntryAddPtr(%p, %p) failed\n",
		  handle, &handle0);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (void *);

      if ((len0 - len3) != (3 * sizeof (void *)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (long long) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddLongLong()\n");

      if (SPHLFlogEntryAddLongLong (&handle0, 12345678LL))
	{
	  printf ("error SPHLFlogEntryAddLongLong(%p, 12345678LL) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf
		("error SPHLFlogEntryAddLongLong alignment (%p,%lx) failed\n",
		 temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddLongLong (&handle0, 23456789LL))
	{
	  printf ("error SPHLFlogEntryAddLongLong(%p, 23456789LL) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddLongLong (&handle0, 34567890LL))
	{
	  printf ("error SPHLFlogEntryAddLongLong(%p, 34567890LL) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (long long);

      if ((len0 - len3) != (3 * sizeof (long long)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (float) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddFloat()\n");

      if (SPHLFlogEntryAddFloat (&handle0, 123.456))
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, 123.456) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf
		("error SPHLFlogEntryAddFloat alignment (%p,%lx) failed\n",
		 temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddFloat (&handle0, 234.567))
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, 234.567) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddFloat (&handle0, 345.678))
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, 345.678) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (float);

      if ((len0 - len3) != (3 * sizeof (float)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

      temp0 = temp3;
      len0 = len3;
      adjust = __alignof__ (double) - 1;
      printf ("lflogger_dataadd_test for SPHLFlogEntryAddDouble()\n");

      if (SPHLFlogEntryAddDouble (&handle0, 123.456))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 123.456) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	  if (((unsigned long) temp1) & adjust)
	    {
	      printf
		("error SPHLFlogEntryAddDouble alignment (%p,%lx) failed\n",
		 temp1, adjust);
	      rc++;
	    }
	}

      if (SPHLFlogEntryAddDouble (&handle0, 234.567))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 234.567) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if (SPHLFlogEntryAddDouble (&handle0, 345.678))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 345.678) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
      len0 = len1 + sizeof (double);

      if ((len0 - len3) != (3 * sizeof (double)))
	{
	  printf ("error lflogger_dataadd_test remainder calc = %d\n",
		  (len0 - len3));
	  rc++;

	}

#ifdef __WORDSIZE_32
      if (SPHLFlogEntryAddDouble (&handle0, 345.678))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 345.678) failed\n",
		  handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
#else
      if (SPHLFlogEntryAddDouble (&handle0, 345.678))
	{
	  printf ("success SPHLFlogEntryAddDouble(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}
#endif

      if (SPHLFlogEntryAddFloat (&handle0, 345.678))
	{
	  printf ("success SPHLFlogEntryAddFloat(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddLongLong (&handle0, 34567890LL))
	{
	  printf ("success SPHLFlogEntryAddLongLong(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddLongLong(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddPtr (&handle0, &handle0))
	{
	  printf ("success SPHLFlogEntryAddPtr(%p, %p) full detected\n",
		  handle, &handle0);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddPtr(%p, %p) should failed\n",
		  handle, &handle0);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddLong (&handle0, 3456789L))
	{
	  printf ("success SPHLFlogEntryAddLong(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddLong(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddInt (&handle0, 345678))
	{
	  printf ("success SPHLFlogEntryAddInt(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddInt(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddShort (&handle0, 3456))
	{
	  printf ("success SPHLFlogEntryAddShort(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddShort(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'c'))
	{
	  printf ("success SPHLFlogEntryAddChar(%p, val) full detected\n",
		  handle);
	}
      else
	{
	  printf ("error SPHLFlogEntryAddChar(%p, val) should failed\n",
		  handle);
	  rc++;

	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle0);
      if (handle)
	{
	  printf
	    ("lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle0, handle);
	  temp0 = handle->next;
	  len0 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp0, len0);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16 + aSize + 16))
	    {
	      printf
		("error lflogger_dataadd_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16 + aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle0);
	  return 10;
	}

      printf ("lflogger_dataadd_test for SPHLFlogEntryAddString()\n");
      if (SPHLFlogEntryAddString (&handle0, (char *) "0123456789"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, " "0123456789"
		  ") failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp1, len1);
	}

      if ((len0 - len1) != (11 * sizeof (char)))
	{
	  printf ("error SPHLFlogEntryAddString(%p, ) length missmatch\n",
		  handle);
	  rc++;
	}

      if (SPHLFlogEntryAddString
	  (&handle0, (char *) "abcdefghijklmnopqrstuvwxyz"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, "
		  "abcdefghijklmnopqrstuvwxyz" ") failed\n", handle);
	  rc++;
	}
      else
	{
	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if ((len1 - len2) != (27 * sizeof (char)))
	{
	  printf ("error SPHLFlogEntryAddString(%p, ) length missmatch\n",
		  handle);
	  rc++;
	}

      if (SPHLFlogEntryAddString
	  (&handle0, (char *) "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, "
		  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ") failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if ((len2 - len3) != (27 * sizeof (char)))
	{
	  printf ("error SPHLFlogEntryAddString(%p, ) length missmatch\n",
		  handle);
	  rc++;
	}

      if (SPHLFlogEntryAddString
	  (&handle0, (char *) "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, "
		  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ") failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if ((len2 - len3) != ((27 * 2) * sizeof (char)))
	{
	  printf ("error SPHLFlogEntryAddString(%p, ) length missmatch\n",
		  handle);
	  rc++;
	}

      if (SPHLFlogEntryAddString
	  (&handle0, (char *) "ABCDEFGHIJKLMNOPQRSTUVWXYZ"))
	{
	  printf ("success SPHLFlogEntryAddString(%p, "
		  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ") reported overflow\n",
		  handle);

	  temp2 = handle->next;
	  len2 = handle->remaining;
	}
      else
	{
	  printf ("error SPHLFlogEntryAddString(%p, "
		  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" ") failed\n", handle);
	  rc++;

	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if ((temp2 == temp3) && (len2 == len3))
	{
	  printf
	    ("verified SPHLFlogEntryAddString(%p, ) handle did not change\n",
	     handle);
	}
      else
	{
	  printf
	    ("error SPHLFlogEntryAddString(%p, ) handle changed after failed add\n",
	     handle);
	  rc++;
	}

      if (SPHLFlogEntryAddString (&handle0, (char *) "01234567890123456789"))
	{
	  printf ("success SPHLFlogEntryAddString(%p, " "01234567890123456789"
		  ") reported overflow\n", handle);

	  temp2 = handle->next;
	  len2 = handle->remaining;
	}
      else
	{
	  printf ("error SPHLFlogEntryAddString(%p, " "01234567890123456789"
		  ") failed\n", handle);
	  rc++;

	  temp2 = handle->next;
	  len2 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp2, len2);
	}

      if ((temp2 == temp3) && (len2 == len3))
	{
	  printf
	    ("verified SPHLFlogEntryAddString(%p, ) handle did not change\n",
	     handle);
	}
      else
	{
	  printf
	    ("error SPHLFlogEntryAddString(%p, ) handle changed after failed add\n",
	     handle);
	  rc++;
	}

      printf
	("lflogger_dataadd_test for SPHLFlogEntryAddString() fill the entry\n");
      if (SPHLFlogEntryAddString (&handle0, (char *) "0123456789012345678"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, " "0123456789012345678"
		  ") failed\n", handle);
	  rc++;
	}
      else
	{
	  temp3 = handle->next;
	  len3 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp3, len3);
	}

      if (len3 == 0)
	{
	  printf ("verified SPHLFlogEntryAddString(%p, ) entry full\n",
		  handle);
	}
      else
	{
	  printf
	    ("error SPHLFlogEntryAddString(%p, ) handle remaining should be 0\n",
	     handle);
	  rc++;
	}


      printf ("lflogger_dataadd_test for lflogentry_test()\n");
      if (lflogentry_test (lfLog, 123, 456, 789))
	{
	  printf ("error lflogentry_test() failed\n");
	  rc++;
	}
      else
	{
	  printf ("success lflogentry_test() complete\n");
	}

      printf ("lflogger_dataadd_test for lflogentry_test_handle()\n");
      if (lflogentry_test_handle (lfLog, &handle1, 123, 456, 789))
	{
	  printf ("error lflogentry_test() failed\n");
	  rc++;
	}
      else
	{
	}

      for (i = 0; i < 18; i++)
	if (lflogentry_test (lfLog, 123, 456, 789))
	  {
	    printf ("error lflogentry_test() failed\n");
	    rc++;
	  }
	else
	  {
	  }

      if (lflogentry_test_handle (lfLog, &handle2, 123, 456, 789))
	{
	  printf ("error lflogentry_test() failed\n");
	  rc++;
	}
      else
	{
	}

      printf ("handle1 ts=%lld, handle2 ts=%lld \n",
	      handle1.entry->timeStamp, handle2.entry->timeStamp);
    }
  else
    {
      printf ("error lflogger_dataadd_test(%p)  SPHLFLoggerInit(%p) failed\n",
	      x4k, x4k);
      return 10;
    }

  return rc;
}

int
lflogger_dataget_test (char *x4k)
{
  int rc = 0;
  SPHLFLogger_t lfLog;
  block_size_t lfspace, lf0space, lfTemp;
  block_size_t aSize;
  SPHLFLoggerHandle_t *handle, handle0, handle1;
  char *temp0, *temp1;
  int len0, len1;
  char valc;
  short int valsi;
  int vali;
  long long valll;
  void *valp;
  float valf, testf;
  double vald, testd;
  char *valstr;

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInit (x4k, 4096);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);
      lfspace = lf0space;

      printf ("lflogger_dataget_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	      x4k, lfLog, lf0space);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_dataget_test SPHLFLoggerEmpty(%p) failed\n",
		  lfLog);
	  rc++;
	}

      if (SPHLFLoggerFull (lfLog))
	{
	  printf ("lflogger_dataget_test SPHLFLoggerFull(%p) failed\n",
		  lfLog);
	  rc++;
	}
      else
	{
	}

      aSize = 128 - 16;
      handle = SPHLFLoggerAllocTimeStamped (lfLog, 0, 0, aSize, &handle0);
      if (handle)
	{
	  printf
	    ("lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) = %p\n",
	     lfLog, aSize, &handle0, handle);
	  temp0 = handle->next;
	  len0 = handle->remaining;
	  printf ("lflogger_dataadd_test ->next=%p, len=%d\n", temp0, len0);
	  lfTemp = SPHLFLoggerFreeSpace (lfLog);
	  if (lfspace != (lfTemp + aSize + 16))
	    {
	      printf
		("error lflogger_dataadd_test  lfspace (%zu != (%zu+%zu))\n",
		 lfspace, lfTemp, (aSize + 16));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lflogger_dataadd_test SPHLFLoggerAllocTimeStamped(%p,0,0,%zu,%p) failed\n",
	     lfLog, aSize, &handle0);
	  return 10;
	}

      handle1 = handle0;

      printf ("lflogger_dataadd_test for SPHLFlogEntryAddChar()\n");
      if (SPHLFlogEntryAddChar (&handle0, 'a'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'a') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddShort()\n");
      if (SPHLFlogEntryAddShort (&handle0, 1234))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 1234) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddShort (&handle0, 12345))
	{
	  printf ("error SPHLFlogEntryAddShort(%p, 12345) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'b'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'b') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddInt()\n");
      if (SPHLFlogEntryAddInt (&handle0, 123456))
	{
	  printf ("error SPHLFlogEntryAddInt(%p, 123456) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddInt (&handle0, 1234567))
	{
	  printf ("error SPHLFlogEntryAddInt(%p, 1234567) failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'c'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'c') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddLongLong()\n");
      if (SPHLFlogEntryAddLongLong (&handle0, 1234567890LL))
	{
	  printf ("error SPHLFlogEntryAddLongLong(%p, 1234567890LL) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddLongLong (&handle0, 12345678901LL))
	{
	  printf
	    ("error SPHLFlogEntryAddLongLong(%p, 12345678901LL) failed\n",
	     handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'd'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'd') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddPtr()\n");
      if (SPHLFlogEntryAddPtr (&handle0, &handle0))
	{
	  printf ("error SPHLFlogEntryAddPtr(%p, %p) failed\n",
		  handle, &handle0);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddPtr (&handle0, &handle1))
	{
	  printf ("error SPHLFlogEntryAddPtr(%p, %p) failed\n",
		  &handle0, &handle1);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'e'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'd') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddFloat()\n");
      if (SPHLFlogEntryAddFloat (&handle0, 123.456))
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, 123.456) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddFloat (&handle0, 1234.567))
	{
	  printf ("error SPHLFlogEntryAddFloat(%p, 1234.567) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddChar (&handle0, 'f'))
	{
	  printf ("error SPHLFlogEntryAddChar(%p, 'd') failed\n", handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddDouble()\n");
      if (SPHLFlogEntryAddDouble (&handle0, 1234.567890))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 1234.567890) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      if (SPHLFlogEntryAddDouble (&handle0, 12345.678901))
	{
	  printf ("error SPHLFlogEntryAddDouble(%p, 12345.678901) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryAddString()\n");
      if (SPHLFlogEntryAddString (&handle0, (char *) "mnopqrstuvwxyz"))
	{
	  printf ("error SPHLFlogEntryAddString(%p, mnopqrstuvwxyz) failed\n",
		  handle);
	  rc++;

	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}
      else
	{
	  temp1 = handle->next;
	  len1 = handle->remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextChar()\n");
      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'a')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextShort()\n");
      valsi = SPHLFlogEntryGetNextShort (&handle1);
      if (valsi == 1234)
	{
	  printf ("success SPHLFlogEntryGetNextShort(%p) returned %hd\n",
		  &handle1, valsi);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextShort(%p) returned %hd\n",
		  &handle1, valsi);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valsi = SPHLFlogEntryGetNextShort (&handle1);
      if (valsi == 12345)
	{
	  printf ("success SPHLFlogEntryGetNextShort(%p) returned %hd\n",
		  &handle1, valsi);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextShort(%p) returned %hd\n",
		  &handle1, valsi);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'b')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextInt()\n");
      vali = SPHLFlogEntryGetNextInt (&handle1);
      if (vali == 123456)
	{
	  printf ("success SPHLFlogEntryGetNextInt(%p) returned %d\n",
		  &handle1, vali);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextInt(%p) returned %d\n",
		  &handle1, vali);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      vali = SPHLFlogEntryGetNextInt (&handle1);
      if (vali == 1234567)
	{
	  printf ("success SPHLFlogEntryGetNextInt(%p) returned %d\n",
		  &handle1, vali);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextInt(%p) returned %d\n",
		  &handle1, vali);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'c')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextLongLong()\n");
      valll = SPHLFlogEntryGetNextLongLong (&handle1);
      if (valll == 1234567890LL)
	{
	  printf ("success SPHLFlogEntryGetNextLongLong(%p) returned %lld\n",
		  &handle1, valll);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextLongLong(%p) returned %lld\n",
		  &handle1, valll);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valll = SPHLFlogEntryGetNextLongLong (&handle1);
      if (valll == 12345678901LL)
	{
	  printf ("success SPHLFlogEntryGetNextLongLong(%p) returned %lld\n",
		  &handle1, valll);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextLongLong(%p) returned %lld\n",
		  &handle1, valll);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'd')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextPtr()\n");
      valp = SPHLFlogEntryGetNextPtr (&handle1);
      if (valp == &handle0)
	{
	  printf ("success SPHLFlogEntryGetNextPtr(%p) returned %p\n",
		  &handle1, valp);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextPtr(%p) returned %p\n",
		  &handle1, valp);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valp = SPHLFlogEntryGetNextPtr (&handle1);
      if (valp == &handle1)
	{
	  printf ("success SPHLFlogEntryGetNextPtr(%p) returned %p\n",
		  &handle1, valp);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextPtr(%p) returned %p\n",
		  &handle1, valp);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'e')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextFloat()\n");
      testf = 123.456;
      valf = SPHLFlogEntryGetNextFloat (&handle1);
      if (valf == testf)
	{
	  printf ("success SPHLFlogEntryGetNextFloat(%p) returned %f\n",
		  &handle1, valf);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextFloat(%p) returned %f\n",
		  &handle1, valf);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      testf = 1234.567;
      valf = SPHLFlogEntryGetNextFloat (&handle1);
      if (valf == testf)
	{
	  printf ("success SPHLFlogEntryGetNextFloat(%p) returned %f\n",
		  &handle1, valf);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextFloat(%p) returned %f\n",
		  &handle1, valf);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      valc = SPHLFlogEntryGetNextChar (&handle1);
      if (valc == 'f')
	{
	  printf ("success SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextChar(%p) returned %c\n",
		  &handle1, valc);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextDouble()\n");
      testd = 1234.567890;
      vald = SPHLFlogEntryGetNextDouble (&handle1);
      if (vald == testd)
	{
	  printf ("success SPHLFlogEntryGetNextDouble(%p) returned %f\n",
		  &handle1, vald);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextDouble(%p) returned %f\n",
		  &handle1, vald);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      testd = 12345.678901;
      vald = SPHLFlogEntryGetNextDouble (&handle1);
      if (vald == testd)
	{
	  printf ("success SPHLFlogEntryGetNextDouble(%p) returned %f\n",
		  &handle1, vald);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextDouble(%p) returned %f\n",
		  &handle1, vald);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

      printf ("lflogger_dataget_test for SPHLFlogEntryGetNextString()\n");
      valstr = SPHLFlogEntryGetNextString (&handle1);
      if (!strcmp (valstr, "mnopqrstuvwxyz"))
	{
	  printf ("success SPHLFlogEntryGetNextString(%p) returned %s\n",
		  &handle1, valstr);
	}
      else
	{
	  printf ("error SPHLFlogEntryGetNextString(%p) returned %s\n",
		  &handle1, valstr);
	  rc++;

	  temp1 = handle1.next;
	  len1 = handle1.remaining;
	  printf ("lflogger_dataget_test ->next=%p, len=%d\n", temp1, len1);
	}

    }
  else
    {
      printf ("error lflogger_dataget_test(%p)  SPHLFLoggerInit(%p) failed\n",
	      x4k, x4k);
      return 10;
    }

  return rc;
}

int
lflogger_struct_alloc_test (char *x4k)
{
  int rc = 0;
  int rc0;
  SPHLFLogger_t lfLog;
  block_size_t lf0space;
  char c_temp;

  unsigned long align_test;

  struct data_s1
  {
    sphtimer_t f0;
    int f1;
    int f2;
    int f3;
  };

  SPHLFLoggerHandle_t *handle, handle0, handle1;
  struct data_s1 *sptr;

  printf ("lflogger_struct_alloc_test(%p)  struct size=%zu, align=%zu\n",
	  x4k, sizeof (struct data_s1), __alignof__ (struct data_s1));

  memset (x4k, 0, 4096);

  lfLog = SPHLFLoggerInit (x4k, 4096);
  if (lfLog)
    {
      lf0space = SPHLFLoggerFreeSpace (lfLog);

      printf
	("lflogger_struct_alloc_test(%p)  SPHLFLoggerFreeSpace(%p) = %zu\n",
	 x4k, lfLog, lf0space);

      if (SPHLFLoggerEmpty (lfLog))
	{
	}
      else
	{
	  printf ("lflogger_struct_alloc_test SPHLFLoggerEmpty(%p) failed\n",
		  lfLog);
	  rc++;
	}

      handle = SPHLFLoggerAllocTimeStamped (lfLog, 123, 76, 112, &handle0);

      handle1 = handle0;	//copy handle for rescan later

      if (handle)
	{
	  rc0 = SPHLFlogEntryAddChar (&handle0, 'A');
	  if (rc0)
	    {
	      printf
		("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryAddChar(%p,) failed\n",
		 x4k, handle);
	      rc++;
	    }
	  else
	    {
	      align_test = (unsigned long) SPHLFLogEntryGetFreePtr (&handle0);
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("lflogger_struct_alloc_test(%p)  SPHLFLogEntryGetFreePtr miss aligned %lx\n",
		     x4k, align_test);
		}
	    }

	  sptr = (struct data_s1 *) SPHLFlogEntryAllocStruct (&handle0,
							      sizeof (struct
								      data_s1),
							      __alignof__
							      (struct
							       data_s1));
	  if (sptr)
	    {
	      align_test = (unsigned long) sptr;
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryAllocStruct(%p) alignment failed\n",
		     x4k, sptr);
		  rc++;
		}
	      printf
		("lflogger_struct_alloc_test SPHLFlogEntryAllocStruct(%p) returned %p\n",
		 handle, sptr);

	      sptr->f0 = sphgettimer ();
	      sptr->f1 = 1;
	      sptr->f2 = 2;
	      sptr->f3 = 3;
	    }
	  else
	    {
	      rc++;
	    }

	  rc0 = SPHLFlogEntryAddChar (&handle0, 'B');
	  if (rc0)
	    {
	      printf
		("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryAddChar(%p,) failed\n",
		 x4k, handle);
	      rc++;
	    }
	  else
	    {
	      align_test = (unsigned long) SPHLFLogEntryGetFreePtr (&handle0);
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("lflogger_struct_alloc_test(%p)  SPHLFLogEntryGetFreePtr miss aligned %lx\n",
		     x4k, align_test);
		}
	    }

	  sptr =
	    (struct data_s1 *) SPHLOGENTRYALLOCSTRUCT (handle,
						       struct data_s1);
	  if (sptr)
	    {
	      align_test = (unsigned long) sptr;
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("error lflogger_struct_alloc_test(%p)  SPHLOGENTRYALLOCSTRUCT(%p) alignment failed\n",
		     x4k, sptr);
		  rc++;
		}
	      printf
		("lflogger_struct_alloc_test SPHLOGENTRYALLOCSTRUCT(%p) returned %p\n",
		 handle, sptr);

	      sptr->f0 = sphgettimer ();
	      sptr->f1 = 11;
	      sptr->f2 = 22;
	      sptr->f3 = 33;
	      SPHLFLogEntryComplete (&handle0);
	    }
	  else
	    {
	      rc++;
	    }

	  handle = &handle1;	// set up for rescan
	  c_temp = SPHLFlogEntryGetNextChar (handle);
	  if (c_temp == 'A')
	    {
	      align_test = (unsigned long) SPHLFLogEntryGetFreePtr (handle);
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("lflogger_struct_alloc_test(%p) after SPHLFlogEntryGetNextChar miss aligned %lx\n",
		     x4k, align_test);
		}

	    }
	  else
	    {
	      printf
		("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryGetNextChar(%p) returned %c\n",
		 x4k, handle, c_temp);
	      rc++;
	    }

	  sptr = (struct data_s1 *) SPHLFlogEntryGetStructPtr (handle,
							       sizeof (struct
								       data_s1),
							       __alignof__
							       (struct
								data_s1));
	  if (sptr)
	    {
	      align_test = (unsigned long) sptr;
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryGetStructPtr(%p) alignment failed\n",
		     x4k, sptr);
		  rc++;
		}
	      printf
		("lflogger_struct_alloc_test SPHLFlogEntryGetStructPtr(%p) returned %p\n",
		 handle, sptr);

	      if ((sptr->f1 == 1) && (sptr->f2 == 2) && (sptr->f3 == 3))
		{
		  printf
		    ("lflogger_struct_alloc_test <%llx,%d,%d,%d> correct\n",
		     sptr->f0, sptr->f1, sptr->f2, sptr->f3);
		}
	      else
		{
		  printf
		    ("lflogger_struct_alloc_test <%llx,%d,%d,%d> incorrect\n",
		     sptr->f0, sptr->f1, sptr->f2, sptr->f3);
		  rc++;
		}
	    }
	  else
	    {
	      rc++;
	    }

	  c_temp = SPHLFlogEntryGetNextChar (handle);
	  if (c_temp == 'B')
	    {
	      align_test = (unsigned long) SPHLFLogEntryGetFreePtr (handle);
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("lflogger_struct_alloc_test(%p) after SPHLFlogEntryGetNextChar miss aligned %lx\n",
		     x4k, align_test);
		}

	    }
	  else
	    {
	      printf
		("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryGetNextChar(%p) returned %c\n",
		 x4k, handle, c_temp);
	      rc++;
	    }

	  sptr =
	    (struct data_s1 *) SPHLOGENTRYGETSTRUCTPTR (handle,
							struct data_s1);
	  if (sptr)
	    {
	      align_test = (unsigned long) sptr;
	      if ((align_test % __alignof__ (struct data_s1)) != 0)
		{
		  printf
		    ("error lflogger_struct_alloc_test(%p)  SPHLFlogEntryGetStructPtr(%p) alignment failed\n",
		     x4k, sptr);
		  rc++;
		}
	      printf
		("lflogger_struct_alloc_test SPHLFlogEntryGetStructPtr(%p) returned %p\n",
		 handle, sptr);

	      if ((sptr->f1 == 11) && (sptr->f2 == 22) && (sptr->f3 == 33))
		{
		  printf
		    ("lflogger_struct_alloc_test <%llx,%d,%d,%d> correct\n",
		     sptr->f0, sptr->f1, sptr->f2, sptr->f3);
		}
	      else
		{
		  printf
		    ("lflogger_struct_alloc_test <%llx,%d,%d,%d> incorrect\n",
		     sptr->f0, sptr->f1, sptr->f2, sptr->f3);
		  rc++;
		}
	    }
	  else
	    {
	      rc++;
	    }
	}
      else
	{
	  rc++;
	}

    }
  else
    {
      printf
	("error lflogger_struct_alloc_test(%p)  SPHLFLoggerInit(%p) failed\n",
	 x4k, x4k);
      rc = 10;
    }
  return rc;
}

/* Fake out the SAS region range checking for these tests.
*/

void
setmemrange (unsigned long low, unsigned long high)
{
  unsigned long ak, bk;

  setSASmemrange (low, high);

  ak = getMemLow ();
  bk = getMemHigh ();
  printf ("getMemLow()=%lx, getMemHigh()=%lx\n", ak, bk);
}

#define stack_block_size 65536
#define stack_block_mask (stack_block_size - 1)
#define stack_buff_size (stack_block_size + stack_block_size)

int
main (int argc, char *argv[])
{
  int rc = 0;
  unsigned long a4k, b4k;
  char a[stack_buff_size], b[stack_buff_size];
  char *source_address, *dest_address;

  memset (a, 0, stack_buff_size);
  memset (b, 0, stack_buff_size);
  a4k = (unsigned long) (a + stack_block_mask) & ~stack_block_mask;
  b4k = (unsigned long) (b + stack_block_mask) & ~stack_block_mask;

  source_address = (char *) a4k;
  dest_address = (char *) b4k;

  printf ("source_address=%p, dest_address=%p\n",
	  source_address, dest_address);
#if 1
  rc += lflogger_basic_test (source_address);
#endif
#if 1
  rc += lflogger_timestamp_test (source_address);
#endif
#if 1
  rc += lflogger_wrap_test (source_address);
#endif
#if 1
  rc += lflogger_dataadd_test (source_address);
#endif
#if 1
  rc += lflogger_dataget_test (source_address);
#endif
#if 1
  rc += lflogger_struct_alloc_test (source_address);
#endif
#if 1
  setmemrange (a4k, b4k + stack_block_size);

  rc += lflogger_dataadd_test (source_address);
#endif
  return rc;
}
