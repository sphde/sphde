/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */


/*
 * SAS SYSV IPC Shared Memory support
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasio.h"
#include "sasshm.h"

#include <sys/ipc.h>
#include <sys/shm.h>

key_t shm_key;
int sasshm_id;

static int createOnlyFlags = (IPC_EXCL | IPC_CREAT | 0666);
static int createFlags = (IPC_CREAT | 0666);

void
SASRemoveShmID (int shm_id)
{
  if (shmctl (shm_id, IPC_RMID, NULL) == -1)
    {
      sas_printf ("SASRemoveShmID(%d) shmctl failed;%s\n",
		  shm_id, strerror (errno));
    }
}

void
SASRemoveShm (void)
{
  if (shmctl (sasshm_id, IPC_RMID, NULL) == -1)
    {
      sas_printf ("SASRemoveShm semctl failed;%s\n", strerror (errno));
    }
}

void
SASDetachShm (void *shm_addr)
{
  if (shmdt (shm_addr) == -1)
    {
      sas_printf ("SASDetachShm(%p) semdt failed;%s\n",
		  shm_addr, strerror (errno));
    }
}

int
SASAllocateShmID (key_t key_id, void *addr, long size)
{
  int shm_id;
  int exist = 0;
  char *shm_addr;

  shm_id = shmget (key_id, size, createOnlyFlags);
  if (shm_id == -1)
    {
      if (errno == EEXIST)
	{
	  exist = errno;
	  shm_id = shmget (key_id, size, createFlags);
	}
    }

  if (shm_id == -1)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASAllocateShmID(%x, %p, %ld); shmget failed;%s\n",
		  key_id, addr, size, strerror (errno));
#endif
      return (-1);
    }

  shm_addr = shmat (shm_id, addr, 0);
#ifdef __SASDebugPrint__
  sas_printf ("SASAllocateShmID(%x, %p, %ld); shmat = %p\n",
	      key_id, addr, size, shm_addr);
#endif
  if ((long) shm_addr == -1L)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASAllocateShmID(%x, %p, %ld); shmat failed;%s\n",
		  key_id, addr, size, strerror (errno));
#endif
      return (-1);
    }

#ifdef __SASDebugPrint__
  sas_printf ("SASAllocateShmID(%x, %p, %ld); allocated ID = %d\n",
	      key_id, addr, size, shm_id);
#endif
  errno = exist;
  return (shm_id);
}

int
SASAllocateShmName (char *key_name, void *addr, long size)
{
  key_t shm_key;
  int shm_id = 0;
  int proj_id;
  /* force unique keys between 32-/64-bit modes */
#ifdef __WORDSIZE_64
  proj_id = 'R';
#else
  proj_id = 'r';
#endif
  sas_printf ("SASAllocateShmName(%s, %p, %ld); projID = %d\n",
	      key_name, addr, size, proj_id);
  shm_key = ftok (key_name, proj_id);
  if (shm_key != (key_t) - 1)
    {
      shm_id = SASAllocateShmID (shm_key, addr, size);
    }

  return (shm_id);
}

int
SASAllocateShmNameProj (char *key_name, char proj, void *addr, long size)
{
  key_t shm_key;
  int shm_id = 0;
  char lower = ('a' - 'A');
  int proj_id;
  /* force unique keys between 32-/64-bit modes */
#ifdef __WORDSIZE_64
  proj_id = proj;
#else
  proj_id = (proj + lower);
#endif
  shm_key = ftok (key_name, (proj_id + lower));
#ifdef __SASDebugPrint__
  sas_printf ("SASAllocateShmNameProj(%s, %x, %p, %ld); projID = %d\n",
	      key_name, proj, addr, size, proj_id);
#endif
  if (shm_key != (key_t) - 1)
    {
      shm_id = SASAllocateShmID (shm_key, addr, size);
    }

  return (shm_id);
}

int
SASAllocateShm (void *addr, long size)
{
  key_t shm_key = __SAS_DEFAULT_SHMKEY;
  sasshm_id = SASAllocateShmID (shm_key, addr, size);
  return sasshm_id;
}
