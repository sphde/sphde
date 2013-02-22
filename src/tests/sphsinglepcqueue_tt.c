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


// Control/Test Program (_main) for SASSIM shared memory on AIX and
// Linux

//#include <iostream>
//using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasshm.h"
#include "sasalloc.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "saslock.h"
#include "sphlfentry.h"
#include "sphsinglepcqueue.h"
#include "sphthread.h"
#include "sphtimer.h"

int
echo (char *fileName)
{
  int rc = 0;
  int c = 0;
  FILE *fp;
  fp = fopen (fileName, "r");

  if (fp > 0)
    {
      while (c != EOF)
	{
	  c = fgetc (fp);
	  if (c != EOF)
	    putchar (c);
	}
      fclose (fp);
    }
  else
    {
      perror ("Open failed ..\n");
      rc = errno;
    }
  return rc;
}

void
fdisplayProcMaps (__pid_t pid)
{
  char buff[32];
  sprintf (buff, "/proc/%d/maps", pid);
  echo (buff);
}

void
displayProcMaps (char *pid)
{
  char buff[32];
  sprintf (buff, "/proc/%s/maps", pid);
  echo (buff);
}

void
displayMyProcMaps ()
{
  __pid_t pid;
  pid = getpid ();
  fdisplayProcMaps (pid);
}

int
lfPCQentry_fasttest (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1 = 0;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      array[0] = val1;
      array[1] = val2;
      array[2] = val3;
      SPHLFEntryComplete (&handle0);
    }
  else
    {
      rc1 = 1;
    }

  return (rc1);
}

int
lfPCQentry_fastverify (SPHSinglePCQueue_t pcqueue, int val1, int val2,
		       int val3)
{
  int rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handle, handle0;
  int *array;

  handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
  if (handle)
    {
      array = (int *) SPHLFEntryGetFreePtr (&handle0);
      rc1 = (array[0] != val1);
      rc2 = (array[1] != val2);
      rc3 = (array[2] != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_fastverify:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
	     tv1, tv2, tv3, val1, val2, val3);
	}

      if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	{
	  rc4 = 0;
	}
      else
	{
	  printf
	    ("lfPCQentry_fastverify:: SPHSinglePCQueueFreeNextEntry() = failed\n");
	  rc4 = 1;
	}
    }
  else
    {
      return 10;
    }

  return (rc1 | rc2 | rc3 | rc4);
}

int
lfPCQentry_test (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
  if (handle)
    {
      rc1 = SPHLFEntryAddInt (&handle0, val1);
      rc2 = SPHLFEntryAddInt (&handle0, val2);
      rc3 = SPHLFEntryAddInt (&handle0, val3);
      SPHSinglePCQueueEntryComplete (&handle0);
    }
  else
    {
      return -1;
    }

  return (rc1 | rc2 | rc3);
}

int
lfPCQentry_verify (SPHSinglePCQueue_t pcqueue, int val1, int val2, int val3)
{
  int rc1, rc2, rc3, rc4;
  int tv1, tv2, tv3;
  SPHLFEntryHandle_t *handle, handle0;

  handle = SPHSinglePCQueueGetNextComplete (pcqueue, &handle0);
  if (handle)
    {
      tv1 = SPHLFEntryGetNextInt (&handle0);
      tv2 = SPHLFEntryGetNextInt (&handle0);
      tv3 = SPHLFEntryGetNextInt (&handle0);
      rc1 = (tv1 != val1);
      rc2 = (tv2 != val2);
      rc3 = (tv3 != val3);

      if (rc1 | rc2 | rc3)
	{
	  printf
	    ("lfPCQentry_verify:: SPHLFEntryGetNextInt() = %d,%d,%d should be %d,%d,%d\n",
	     tv1, tv2, tv3, val1, val2, val3);
	}

      if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	{
	  rc4 = 0;
	}
      else
	{
	  printf
	    ("lfPCQentry_verify:: SPHSinglePCQueueFreeNextEntry() = failed\n");
	  rc4 = 1;
	}
    }
  else
    {
      return 10;
    }

  return (rc1 | rc2 | rc3 | rc4);
}

int
test_unit (block_size_t aSize)
{
  int rtn = 0;
  int i, rc;
  SPHSinglePCQueue_t pcqueue;
  block_size_t cap, units;
#ifdef SPH_TIMERTEST_VERIFY
  int j, k;
  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate;
  long int p10;
#endif

  pcqueue = SPHSinglePCQueueCreateWithStride (4096, aSize);
  if ((pcqueue != NULL))
    {
      printf ("\nSPHSinglePCQueueCreateWithStride (%d, %zu) success \n", 4096,
	      aSize);

      cap = SPHSinglePCQueueFreeSpace (pcqueue);

      units = cap / aSize;

      printf ("SPHSinglePCQueueFreeSpace() = %zu units=%zu\n", cap, units);

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_test (pcqueue, i, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	}

      printf ("SPHSinglePCQueueFreeSpace() = %zu\n",
	      SPHSinglePCQueueFreeSpace (pcqueue));

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("test_unit SPHSinglePCQueueEmpty(%p) failed\n", pcqueue);
	  rtn++;
	}
      else
	{
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("test_unit SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_verify (pcqueue, i, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueGetNextComplete (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	}

      /* in its current state the queue is not full and is empty */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	}
      else
	{
	  printf ("test_unit SPHSinglePCQueueEmpty(%p) failed\n", pcqueue);
	  rtn++;
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("test_unit SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      printf ("test_unit() verify PCQueue complete\n\n");

#ifdef SPH_TIMERTEST_VERIFY
      j = 0;
      k = 0;
      p10 = 10000000;

      startt = sphgettimer ();
      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_test (pcqueue, k, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	  k++;
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("test_unit SPHSinglePCQueueEmpty(%p) failed\n", pcqueue);
	  rtn++;
	}
      else
	{
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("test_unit SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      for (i = 0; i < p10; i++)
	{
	  rc = lfPCQentry_verify (pcqueue, j, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueGetNextComplete (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	  j++;

	  rc = lfPCQentry_test (pcqueue, k, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	  k++;
	}

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_verify (pcqueue, j, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueGetNextComplete (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	  j++;
	}

      /* in its current state the queue is not full and is empty */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	}
      else
	{
	  printf ("test_unit SPHSinglePCQueueEmpty(%p) failed\n", pcqueue);
	  rtn++;
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("test_unit SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      endt = sphgettimer ();
      tempt = endt - startt;
      clock = tempt;
      freqt = sphfastcpufreq ();
      nano = (clock * 1000000000.0) / (double) freqt;
      nano = nano / p10;
      rate = p10 / (clock / (double) freqt);

      printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
	      startt, endt, tempt, freqt);

      printf ("test_unit X %ld ave= %6.2fns rate=%10.1f/s\n",
	      p10, nano, rate);

      printf ("test_unit() timed PCQueue complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */

      printf ("\nSPHSinglePCQueueDestroy (%p) \n", pcqueue);
      SPHSinglePCQueueDestroy (pcqueue);
    }
  else
    {
      printf ("SPHSinglePCQueueCreateWithStride (%d, %zu) failed \n", 4096,
	      aSize);
      rtn++;
    }

  return rtn;
}

int
test_unitfast (block_size_t aSize)
{
  int rtn = 0;
  int i, rc;
  SPHSinglePCQueue_t pcqueue;
  block_size_t cap, units;
#ifdef SPH_TIMERTEST_VERIFY
  int j, k;
  sphtimer_t	tempt, startt, endt, freqt;
  double clock, nano, rate;
  long int p10;
#endif

  pcqueue = SPHSinglePCQueueCreateWithStride (4096, aSize);
  if ((pcqueue != NULL))
    {
      printf ("\nSPHSinglePCQueueCreateWithStride (%d, %zu) success \n", 4096,
	      aSize);

      cap = SPHSinglePCQueueFreeSpace (pcqueue);

      units = cap / aSize;

      printf ("SPHSinglePCQueueFreeSpace() = %zu units=%zu\n", cap, units);

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_fasttest (pcqueue, i, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	}

      printf ("SPHSinglePCQueueFreeSpace() = %zu\n",
	      SPHSinglePCQueueFreeSpace (pcqueue));

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("test_unitfast SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rtn++;
	}
      else
	{
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("test_unitfast SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_fastverify (pcqueue, i, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf ("SPHSinglePCQueueGetNextComplete (%p) failed\n",
		      pcqueue);
	      rtn++;
	      break;
	    }
	}

      /* in its current state the queue is not full and is empty */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	}
      else
	{
	  printf ("test_unitfast SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rtn++;
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("test_unitfast SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      printf ("test_unitfast() verify PCQueue complete\n\n");

#ifdef SPH_TIMERTEST_VERIFY
      j = 0;
      k = 0;
      p10 = 10000000;

      startt = sphgettimer ();
      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_fasttest (pcqueue, k, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf
		("test_unitfast() SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		 pcqueue);
	      rtn++;
	      break;
	    }
	  k++;
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("test_unitfast SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rtn++;
	}
      else
	{
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("test_unitfast SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      for (i = 0; i < p10; i++)
	{
	  rc = lfPCQentry_fastverify (pcqueue, j, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf
		("test_unitfast() SPHSinglePCQueueGetNextComplete (%p) failed\n",
		 pcqueue);
	      rtn++;
	      break;
	    }
	  j++;

	  rc = lfPCQentry_fasttest (pcqueue, k, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf
		("test_unitfast() SPHSinglePCQueueAllocStrideEntry (%p) failed\n",
		 pcqueue);
	      rtn++;
	      break;
	    }
	  k++;
	}

      for (i = 0; i < units; i++)
	{
	  rc = lfPCQentry_fastverify (pcqueue, j, 0x12345678, 0xdeadbeef);
	  if (!rc)
	    {
	    }
	  else
	    {
	      printf
		("test_unitfast() SPHSinglePCQueueGetNextComplete (%p) failed\n",
		 pcqueue);
	      rtn++;
	      break;
	    }
	  j++;
	}

      /* in its current state the queue is not full and is empty */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	}
      else
	{
	  printf ("test_unitfast SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rtn++;
	}
      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("test_unitfast SPHSinglePCQueueFull(%p) failed\n", pcqueue);
	  rtn++;
	}

      endt = sphgettimer ();
      tempt = endt - startt;
      clock = tempt;
      freqt = sphfastcpufreq ();
      nano = (clock * 1000000000.0) / (double) freqt;
      nano = nano / p10;
      rate = p10 / (clock / (double) freqt);

      printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
	      startt, endt, tempt, freqt);

      printf ("test_unitfast X %ld ave= %6.2fns rate=%10.1f/s\n",
	      p10, nano, rate);

      printf ("test_unitfast() timed PCQueue complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */

      printf ("\nSPHSinglePCQueueDestroy (%p) \n", pcqueue);
      SPHSinglePCQueueDestroy (pcqueue);
    }
  else
    {
      printf ("SPHSinglePCQueueCreateWithStride (%d, %zu) failed \n", 4096,
	      aSize);
      rtn++;
    }

  return rtn;
}

int
main ()
{
  int rc;

  SAS_IO_INIT			// init the io stuff
    rc = SASJoinRegion ();

  if (rc)
    {
      printf ("SASJoinRegion Error# %d\n", rc);

      return 1;
    }
  else
    {
      printf ("SAS Joined\n");

      rc += test_unit (128);
      rc += test_unit (3 * 128);

      rc += test_unitfast (128);
      rc += test_unitfast (3 * 128);
    }

  //SASCleanUp();
  printf ("SAS removed\n");
  SASRemove ();
  return rc;
};
