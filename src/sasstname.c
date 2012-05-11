/*
 * Copyright (c) 2003, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "sasconf.h"
#include "sassim.h"
#include "sasio.h"

#define sas_printf printf

char *sasStorePath = NULL;

char *
SASSegNameIndexed (char *name, sasseg_t segnum)
{
  if (sasStorePath != NULL)
    sas_sprintf (name, "%s/SAS%05lX.DAT", sasStorePath, segnum);
  else
    sas_sprintf (name, "SAS%05lX.DAT", segnum);

#ifdef __SASDebugPrint__
  sas_printf ("SASSegNameIndexed (%ld) = <%s>\n", segnum, name);
#endif

  return name;
}

int
SASSegNameExists (char *name)
{
  int rc, result;
  struct stat stat_buf;

#ifdef __SASDebugPrint__
  sas_printf ("SASSegNameExists(%s)\n", name);
  errno = 0;
#endif

  rc = stat (name, &stat_buf);
#ifdef __SASDebugPrint__
  sas_printf ("SASSegNameExists stat %d, %s\n", rc, strerror (errno));
#endif
  if (rc == -1)
    {
#ifdef __SASDebugPrint__
      sas_printf ("SASSegNameExists; %s\n", strerror (errno));
#endif
      result = 0;
    }
  else
    {
      result = 1;
    }
  return result;
}

int
SASSegIndexExists (sasseg_t segnum)
{
  char name[STORE_NAME_SIZE];

#ifdef __SASDebugPrint__
  sas_printf ("SASSegIndexExists(%ld)\n", segnum);
#endif
  SASSegNameIndexed (&name[0], segnum);
  return (SASSegNameExists (name));
}

int
SASSegStoreCreateByName (char *name)
{
  int rc, fd;
#ifdef __SASDebugPrint__
  sas_printf ("SASSegStoreCreateByName(%s)\n", name);
#endif

  rc = open (name, (O_CREAT | O_EXCL | O_RDWR), (0766));
  if (rc != -1)
    {
      fd = rc;
      rc = ftruncate (fd, SegmentSize);
      if (rc == 0)
	{
#ifdef __SASDebugPrint__
	  sas_printf ("SASSegStoreCreateByName created %s\n", name);
#endif
	  rc = close (fd);
	  if (rc != 0)
	    {
	      sas_printf ("SASSegStoreCreateByName close failed; %s\n",
			  strerror (errno));
	      sas_printf (" Store Name %s\n", name);
	    }
	}
      else
	{
	  sas_printf ("SASSegStoreCreateByName truncate failed with %s\n",
		      strerror (errno));
	  sas_printf (" Store Name %s\n", name);
	}
    }
  else
    {
#if 0
      sas_printf ("SASSegStoreCreateByName open failed with %s\n",
		  strerror (errno));
#else
      sas_printf ("SASSegStoreCreateByName open failed with %d\n", errno);
#endif
      sas_printf (" Store Name %s\n", name);
    }

  return rc;
}

int
SASSegStoreCreate (sasseg_t segnum)
{
  int rc;
  char name[STORE_NAME_SIZE];
#ifdef __SASDebugPrint__
  sas_printf ("SASSegStoreCreate(%ld)\n", segnum);
#endif

  SASSegNameIndexed (&name[0], segnum);

  rc = SASSegStoreCreateByName (name);

  return rc;
}

int
SASSegStoreRemoveByName (char *name)
{
  int rc;

#ifdef __SASDebugPrint__
  sas_printf ("SASSegStoreRemoveByName(%s)\n", name);
#endif
  rc = remove (name);
  if (rc == -1)
    {
      sas_printf ("SASSegStoreRemoveByName remove failed %s\n",
		  strerror (errno));
      sas_printf (" Store Name %s\n", name);
    }

  return rc;
}

int
SASSegStoreRemove (sasseg_t segnum)
{
  int rc;
  char name[STORE_NAME_SIZE];

#ifdef __SASDebugPrint__
  sas_printf ("SASSegStoreRemove(%ld)\n", segnum);
#endif

  SASSegNameIndexed (&name[0], segnum);

  rc = SASSegStoreRemoveByName (name);

  return rc;
}
