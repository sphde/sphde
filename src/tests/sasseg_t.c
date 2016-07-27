/*
 * sasseg_t.c
 *
 *  Created on: Mar 27, 2016
 *      Author: sjmunroe
 */
/*
 * Copyright (c) 2016 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "sasshm.h"
#include "sasalloc.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "sassimpleheap.h"
#include "sassimplestack.h"
#include "sassimplespace.h"
#include "sascompoundheap.h"
#include "sasstringbtree.h"
#include "sasstringbtreeenum.h"
#include "sasindexkey.h"
#include "sasindex.h"
#include "sasindexenum.h"
#include "sphcontext.h"
#include "sphlflogentry.h"
#include "sphlflogger.h"
#include "sphthread.h"
#include "sphtimer.h"

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sasseg_t";

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

#define FORK_EXIT_FAILURE 127
#define JOIN_EXIT_FAILURE 32
#define WAIT_EXIT_FAILURE 16

static void
relax_ptr (void **ref)
{
	while (*ref == NULL)
	{
		usleep (1000);
	}
}

pid_t child_1, child_2;

typedef struct {
	long		child_status;
	char		*block1;
	char		*block2;
	char		*block3;
	char		*badBlock;
} seg_test_t;


static struct sigaction testoldSigSegV;

void
TESTSigSegvHandler (int signal, siginfo_t * info, void *context)
{
	seg_test_t *shared_block;

	/* If we got to here the runtime detected that the sigsegv was
	 * not within the SAS Region range.  So the SAS signal handler
	 * correctly passed control to the next outter sigaction.  */

	shared_block = (seg_test_t*)getSASFinder ();

	if (shared_block)
	{
		/* Decrement the child_status as this is OK, The SAS runtime
		 * correctly handled this bogus address ref.  */
		sas_fetch_and_add (&shared_block->child_status, -1);
	}
	fprintf (stdout, "Child1 TESTSigSegvHandler\n");

	exit (1);
}

void
TESTEnableSigSegv (void)
{
#if 1
  struct sigaction sa;
  /* register signal handler via sigaction.
     Remove SA_ONESHOT for the real thing.  */
  sa.sa_flags = SA_SIGINFO | SA_RESTART /* | SA_ONESHOT */ ;
  sa.sa_sigaction = &TESTSigSegvHandler;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGSEGV, &sa, &testoldSigSegV);
#endif
}

void
TESTDisableSigSegv (void)
{
#if 1
  sigaction (SIGSEGV, &testoldSigSegV, NULL);
#endif
}

int
test_child_1 (void)
{
	seg_test_t *shared_block;
	char 		*new_block;
	int rc = 0;


	/* This is the child process */
	fprintf (stdout, "Child1 process\n");
	/* This child registers own sigaction to demonstrate the
	 * nesting of SIGSEGV sigaction handlers.  */
	TESTEnableSigSegv();
	sleep (1);

	if ((rc = SASJoinRegion ()))
	{
		SASSIM_PRINT_ERR ("SASJoinRegion: %i", rc);

		return (JOIN_EXIT_FAILURE);
	}

	printf ("Child1 __SAS_BASE_ADDRESS=%lx\n", __SAS_BASE_ADDRESS);
	printf ("Child1 RegionSize        =%lx\n", RegionSize);
	printf ("Child1 SegmentSize       =%lx\n", SegmentSize);

	shared_block = (seg_test_t*)getSASFinder ();
	printf ("Child1 shared_block      =%p\n", shared_block);
	/* the parent pre-incremented the error status in case the child
	 * or child Join fails. Since we got to this point the child is
	 * running and successfully Joined, post-decrement to clear the
	 * error.  */
	sas_fetch_and_add (&shared_block->child_status,
			-(JOIN_EXIT_FAILURE));

	new_block = SASBlockAlloc (SegmentSize);
	shared_block->block1 = strcpy (new_block, "new_block 1");
	printf ("Child1 shared_block.blk1 =%p\n", shared_block->block1);

	relax_ptr ((void**)&shared_block->block3);
	printf ("Child1 shared_block.blk3 =%p\n", shared_block->block3);
	printf ("Child1 shared_block.blk3 =<%s>\n", shared_block->block3);

	relax_ptr ((void**)&shared_block->badBlock);

	/* Pre-increment the child_status before a ref to bogus address
	 * causes a SIGSEGV.
	 *
	 * The following test will verify that the runtime detects the
	 * SIGSEGV address is outside the Region it manages, and
	 * successfully up-calls to the applications sigaction handler.
	 * For this test the applications sigaction handler will
	 * post-decrement the child status to clear the error.  */
	sas_fetch_and_add (&shared_block->child_status, 1);
#if 1
	*shared_block->badBlock = 1;
#else
	printf ("Child1 shared_block.blk3 =%p\n", shared_block->badBlock);
	printf ("Child1 shared_block.blk3 =<%s>\n", shared_block->badBlock);
#endif
	fprintf (stdout, "Child1 process exit\n");

	SASCleanUp ();

	TESTDisableSigSegv();
	return (rc);
}

int
test_child_2 (void)
{
	seg_test_t *shared_block;
	char 		*new_block;
	int rc = 0;

	/* This is the child process */
	fprintf (stdout, "Child process\n");
	sleep (1);

	if ((rc = SASJoinRegion ()))
	{
		SASSIM_PRINT_ERR ("SASJoinRegion: %i", rc);

		return (JOIN_EXIT_FAILURE);
	}

	printf ("Child2 __SAS_BASE_ADDRESS=%lx\n", __SAS_BASE_ADDRESS);
	printf ("Child2 RegionSize        =%lx\n", RegionSize);
	printf ("Child2 SegmentSize       =%lx\n", SegmentSize);

	shared_block = (seg_test_t*)getSASFinder ();
	printf ("Child2 shared_block      =%p\n", shared_block);
	/* the parent pre-incremented the error status in case the child
	 * or child Join fails. Since we got to this point the child is
	 * running and successfully Joined, post-decrement to clear the
	 * error.  */
	sas_fetch_and_add (&shared_block->child_status,
			-(JOIN_EXIT_FAILURE));

	new_block = SASBlockAlloc (SegmentSize);
	shared_block->block2 = strcpy (new_block, "new_block 2");
	printf ("Child2 shared_block.blk2 =%p\n", shared_block->block2);

	relax_ptr ((void**)&shared_block->block3);
	printf ("Child2 shared_block.blk3 =%p\n", shared_block->block3);
	printf ("Child2 shared_block.blk3 =<%s>\n", shared_block->block3);

	fprintf (stdout, "Child2 process exit\n");

	SASCleanUp ();

	return (rc);
}

int
test_parent (void)
{
	seg_test_t	*shared_block;
	char 			*new_block3;

	int rc;
	int failures = 0;
	int wait_stat1, wait_stat2;

	fprintf (stdout, "Parent process\n");

	if ((rc = SASJoinRegion ()))
	{
		SASSIM_PRINT_ERR ("SASJoinRegion: %i", rc);
		failures = (JOIN_EXIT_FAILURE);
	}

	printf ("Parent __SAS_BASE_ADDRESS=%lx\n", __SAS_BASE_ADDRESS);
	printf ("Parent RegionSize        =%lx\n", RegionSize);
	printf ("Parent SegmentSize       =%lx\n", SegmentSize);

	shared_block = (seg_test_t*)SASBlockAlloc (4096);
	shared_block->child_status = 0;

	/* The parent pre-increments this error status in case the child
	 * or child Join fails. Once the child is running and
	 * successfully Joined, it will post-decrement to clear the
	 * error.  */
	sas_fetch_and_add (&shared_block->child_status,
			(JOIN_EXIT_FAILURE));
	sas_fetch_and_add (&shared_block->child_status,
			(JOIN_EXIT_FAILURE));

	setSASFinder (shared_block);
	printf ("Parent share_block       =%p\n", shared_block);

	relax_ptr ((void**)&shared_block->block1);
	printf ("Parent shared_block.blk1 =%p\n", shared_block->block1);
	printf ("Parent shared_block.blk1 =<%s>\n", shared_block->block1);

	relax_ptr ((void**)&shared_block->block2);
	printf ("Parent shared_block.blk2 =%p\n", shared_block->block2);
	printf ("Parent shared_block.blk2 =<%s>\n", shared_block->block2);

	new_block3 = SASBlockAlloc (SegmentSize);
	shared_block->block3 = strcpy (new_block3, "new_block 3");
	printf ("Parent shared_block.blk3 =%p\n", shared_block->block3);

	shared_block->badBlock = (char *)(__SAS_BASE_ADDRESS - (64 * 1024));

	fprintf (stdout, "Parent process wait for child\n");
	if (waitpid (child_1, &wait_stat1, 0) != child_1)
	{
		perror ("waitpid1 Failed");
		failures += WAIT_EXIT_FAILURE;
	}
	if (waitpid (child_2, &wait_stat2, 0) != child_2)
	{
		perror ("waitpid2 Failed");
		failures += WAIT_EXIT_FAILURE;
	}

	failures += shared_block->child_status;
	printf("SAS removed\n");
	SASRemove();

	fprintf (stdout, "Parent process wait complete\n");

	return (failures);
}

int
main ()
{
  int failures = 0;

  child_1 = fork ();
  if (child_1 == (pid_t)0)
  {
	/* This is the 1st child process */
	  failures = test_child_1 ();

	  return failures;
  } else if (child_1 > (pid_t)0)
  {
	  /* This is the Parent process */
	  child_2 = fork ();
	  if (child_2 == (pid_t)0)
	  {
		/* This is the 2nd child process */
		  failures = test_child_2 ();

		  return failures;
	  } else if (child_2 > (pid_t)0)
	  {
		  /* This is the Parent process */
		  failures = test_parent();
	  } else {
		  /* the 2nd fork failed */
		  perror ("fork Failed");
		  failures += FORK_EXIT_FAILURE;
	  }
  } else {
	  /* the 1st fork failed */
	  perror ("fork Failed");
	  failures += FORK_EXIT_FAILURE;
  }

  return failures;
}
