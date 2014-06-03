/*
 * Copyright (c) 2007-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 *     sjm 2010-09-05       Added cached procID and accessor function 
 *                          Added get function for /proc/<PID/cmdline 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syscall.h>
#include "sphthread.h"

__thread pid_t threadID = 0;
pid_t procID = 0;

static char sph_cmdLine[256];

static void
sphdeGetCmdLine_internal (pid_t pid)
{
  char procFile[32];
  int fd, cnt;

  snprintf (procFile, sizeof (procFile), "/proc/%d/cmdline", pid);

  fd = open (procFile, O_RDONLY);
  if (fd != -1)
    {
      cnt = read (fd, sph_cmdLine, sizeof (sph_cmdLine));
      if (cnt > 0)
	{
	  if (cnt == sizeof (sph_cmdLine))
	    cnt--;
	  sph_cmdLine[cnt] = 0;
	}
    }
  else
    {
      sph_cmdLine[0] = 0;
    }
}

pid_t
sphdeGetTID (void)
{
  pid_t tid = threadID;
  if (!threadID)
    {
      tid = syscall (SYS_gettid);
      if (tid == -1)
	{
	  printf ("gettid failed %s\n", strerror (errno));
	}
      else
	{
	  threadID = tid;
	}
    }
  return tid;
}

pid_t
sphdeGetPID (void)
{
  pid_t pid = procID;
  if (!procID)
    {
      procID = getpid ();
      pid = procID;
      sphdeGetCmdLine_internal (pid);
    }
  return pid;
}

char *
sphdeGetCmdLine (void)
{
  pid_t pid = procID;
  if (!procID)
    {
      procID = getpid ();
      pid = procID;
      sphdeGetCmdLine_internal (pid);
    }
  return sph_cmdLine;
}
