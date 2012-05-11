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
#include "sasstringbtree.h"
#include "sasstringbtreeenum.h"
#include "sasindexkey.h"
#include "sasindex.h"
#include "sasindexenum.h"
#include <sphcontextpriv.h>

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x)  #x
#define LINE_STRING  STRINGIZE(__LINE__)

static const char sassim_prog_name[] = "sphcontext_t";

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
#else
#define SASSIM_DUMP_BLOCK(fmt, ...)
#endif

#define JOIN_EXIT_FAILURE 128
//#define __SASDebugPrint__ 1

static int
sassim_context_test1 ()
{
  SPHContext_t context;
  block_size_t blockSize = block__Size64K;
  block_size_t freeSpace, freeSpace0;
  char str[64];
  char str3[64];
  char str4[64];
  char *txtstr;
  char *txtstr0;
  char *txtstr1;
  char *txtstr2;
  char *txtstr3;
  int rc;

  context = SPHContextCreate (blockSize);
  if (!context) {
    SASSIM_PRINT_ERR ("SPHContextCreate (%zu)", blockSize);
    return 1;
  }
  SASSIM_PRINT_MSG ("SPHContextCreate (%ld) success", blockSize);
  freeSpace = SPHContextFreeSpace (context);
  freeSpace0 = freeSpace;
  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", freeSpace);
  SASSIM_DUMP_BLOCK (context, 128+32);
  txtstr3 = (char *) SPHContextAlloc (context, 16);
  if (!txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextAlloc (%p, %d) failed", context, 16);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextAlloc (%p, %d) == %p", context, 16, txtstr3);
      freeSpace = SPHContextFreeSpace (context);
      if (freeSpace == (freeSpace0-16))
        {
          SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", freeSpace);
        } else {
          SASSIM_PRINT_ERR ("SPHContextFreeSpace miss-match %ld vs %ld", freeSpace, (freeSpace0-16));
          return 1;
        }
    }

  txtstr2 = (char *) SPHContextAlloc (context, 32);
  if (!txtstr2)
    {
      SASSIM_PRINT_ERR ("SPHContextAlloc (%p, %d) failed", context, 32);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextAlloc (%p, %d) == %p", context, 32, txtstr2);
      freeSpace = SPHContextFreeSpace (context);
      if (freeSpace == (freeSpace0-(16+32)))
        {
          SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", freeSpace);
        } else {
          SASSIM_PRINT_ERR ("SPHContextFreeSpace miss-match %ld vs %ld", freeSpace, (freeSpace0-(16+32)));
          return 1;
        }
    }

  rc = SPHContextFree (context, txtstr2, 32);
  if (rc)
    {
      SASSIM_PRINT_ERR ("SPHContextFree (%p, %p, %d) failed", context, txtstr2, 32);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFree (%p, %p, %d) == %d", context, txtstr2, 32, rc);
      freeSpace = SPHContextFreeSpace (context);
      if (freeSpace == (freeSpace0-(16)))
        {
          SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", freeSpace);
        } else {
          SASSIM_PRINT_ERR ("SPHContextFreeSpace miss-match %ld vs %ld", freeSpace, (freeSpace0-(16)));
          return 1;
        }
    }

  rc = SPHContextFree (context, txtstr3, 16);
  if (rc)
    {
      SASSIM_PRINT_ERR ("SPHContextFree (%p, %p, %d) failed", context, txtstr3, 16);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFree (%p, %p, %d) == %d", context, txtstr3, 16, rc);
      freeSpace = SPHContextFreeSpace (context);
      if (freeSpace == (freeSpace0))
        {
          SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", freeSpace);
        } else {
          SASSIM_PRINT_ERR ("SPHContextFreeSpace miss-match %ld vs %ld", freeSpace, (freeSpace0));
          return 1;
        }
    }

  memset (str, 0, 64);
  txtstr = strcpy (str, "aLongContextName");
  txtstr0 = txtstr;
  rc = SPHContextAddName (context, txtstr, txtstr);
  if (!rc)
    {
      SASSIM_PRINT_ERR ("SPHContextAddName (%p, %s, %p) failed", context,
              txtstr, txtstr);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) success", context,
          txtstr, txtstr);

  txtstr2 = (char *) SPHContextFindByAddr (context, txtstr);
  if (!txtstr2)
    {
       SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) failed", context,
               txtstr);
       return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextFindByAddr (%p, %p) == %s@%p", context,
          txtstr, txtstr2, txtstr2);

  txtstr3 = (char *) SPHContextFindByName (context, str);
  if (!txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByName (%p, %s) failed", context, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByName (%p, %s) == %p", context, str,
             txtstr3);
      if (txtstr0 != txtstr3)
        {
          SASSIM_PRINT_ERR ("SPHContextFindByName value miss-match %p vs %p",
        		  txtstr0, txtstr3);
          return 1;
        }
    }
  
  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", SPHContextFreeSpace (context));
  SASSIM_DUMP_BLOCK (context, 128+48);
  memset (str3, 0, 64);
  txtstr = strcpy (str3, "aLongerrrrrrContextName");
  txtstr1 = txtstr;
  rc = SPHContextAddName (context, txtstr, txtstr);
  if (!rc)
    {
      SASSIM_PRINT_ERR ("SPHContextAddName (%p, %s, %p) failed", context,
              txtstr, txtstr);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) success", context,
          txtstr, txtstr);

  txtstr2 = (char *) SPHContextFindByAddr (context, txtstr);
  if (!txtstr2)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) failed", context,
              txtstr);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextFindByAddr (%p, %p) == %s", context,
        txtstr, txtstr2);

  txtstr3 = (char *) SPHContextFindByName (context, str3);
  if (!txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByName (%p, %s) failed", context, str3);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByName (%p, %s) == %p", context, str3,
             txtstr3);
      if (txtstr1 != txtstr3)
        {
          SASSIM_PRINT_ERR ("SPHContextFindByName value miss-match %p vs %p",
        		  txtstr1, txtstr3);
          return 1;
        }
    }

  txtstr3 = (char *) SPHContextFindByName (context, str);
  if (!txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByName (%p, %s) failed", context, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByName (%p, %s) == %p", context, str,
             txtstr3);
      if (txtstr0 != txtstr3)
        {
          SASSIM_PRINT_ERR ("SPHContextFindByName value miss-match %p vs %p",
        		  txtstr0, txtstr3);
          return 1;
        }
    }

  txtstr2 = (char *) SPHContextFindByAddr (context, txtstr0);
  if (!txtstr2)
    {
       SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) failed", context,
               txtstr0);
       return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByAddr (%p, %p) == %s@%p", context,
    		  txtstr0, txtstr2, txtstr2);
      if (strlen(txtstr2) != strlen(str))
        {
          SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) Name length miss-match",
        		  context, txtstr0);
          return 1;
        }
    }

  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", SPHContextFreeSpace (context));
  SASSIM_DUMP_BLOCK (context, 128+80);
  memset (str4, 0, 64);
  txtstr = strcpy (str4, "anotherContextName");
  txtstr3 = (char *) SPHContextFindByName (context, str4);
  if (txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByName (%p, %s) failed", context, str);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextFindByName (%p, %s) == %p", context, str,
        txtstr3);

  txtstr2 = (char *) SPHContextFindByAddr (context, str4);
  if (txtstr2)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) failed", context,
        txtstr);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextFindByAddr (%p, %p) == %s", context,
        txtstr, txtstr2);

  rc = SPHContextAddName (context, txtstr, txtstr);
  if (!rc)
    {
      SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) failed", context,
        txtstr, txtstr);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) success", context,
        txtstr, txtstr);

  txtstr2 = (char *) SPHContextFindByAddr (context, txtstr);
  if (!txtstr2)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByAddr (%p, %p) failed", context,
        txtstr);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByAddr (%p, %p) == %s", context,
    	        txtstr, txtstr2);
      if (strcmp(txtstr, txtstr2) != 0)
        {
          SASSIM_PRINT_ERR ("SPHContextFindByName value miss-match %p vs %p",
            		  txtstr, txtstr2);
          return 1;
        }
    }

  txtstr3 = (char *) SPHContextFindByName (context, str4);
  if (!txtstr3)
    {
      SASSIM_PRINT_ERR ("SPHContextFindByName (%p, %s) failed", context, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextFindByName (%p, %s) == %p", context,
    		  txtstr, txtstr3);
          if (txtstr != txtstr3)
            {
              SASSIM_PRINT_ERR ("SPHContextFindByName value miss-match %p vs %p",
                		  txtstr, txtstr3);
              return 1;
            }
        }

  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", SPHContextFreeSpace (context));
  SASSIM_DUMP_BLOCK (context, 128 + 96);

  rc = SPHContextRemoveByName (context, str3);
  if (rc)
    {
      SASSIM_PRINT_ERR ("SPHContextRemoveByName (%p, %s) failed rc=%d",
        context, str3, rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextRemoveByName (%p, %s) success", context,
        str3);
  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", SPHContextFreeSpace (context));
  SASSIM_DUMP_BLOCK (context, 128 + 96);
  rc = SPHContextRemoveByAddr (context, str4);
  if (rc)
    {
      SASSIM_PRINT_ERR ("SPHContextRemoveByAddr (%p, %p) failed rc=%d",
        context, str4, rc);
      return 1;
    }
  SASSIM_PRINT_MSG ("SPHContextRemoveByAddr (%p, %p) success ", context,
        str4);
  SASSIM_PRINT_MSG ("SPHContextFreeSpace() = %zu", SPHContextFreeSpace (context));
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  /* Now test for duplicate names */
  txtstr = str;

  rc = SPHContextAddName (context, txtstr, str);
  if (rc)
    {
      SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) failed", context,
        txtstr, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) duplicate key (%d)",
    		  context, txtstr, str, rc);
    }
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  /* Now test for values with multiple names */
  memset (str4, 0, 64);
  txtstr = strcpy (str4, "anotherContextName2");

  rc = SPHContextAddName (context, txtstr, str);
  if (rc)
    {
      SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) failed", context,
        txtstr, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextAddName (%p, %s, %p) duplicate key (%d)",
    		  context, txtstr, str, rc);
    }
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  /* Now test renaming a value */
  memset (str4, 0, 64);
  txtstr = strcpy (str4, "anotherContextName3");

  rc = SPHContextRename (context, str, txtstr, str4);
  if (rc)
    {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s, %p) failed (%d)", context,
        str, txtstr, str4, rc);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s %p) detected value miss-match (%d)",
    		  context, str, txtstr, str4, rc);
    }
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  rc = SPHContextRename (context, txtstr1, txtstr, str4);
  if (rc)
    {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s, %p) failed (%d)", context,
        txtstr1, txtstr, str4, rc);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s %p) detected invalid oldname (%d)",
    		  context, txtstr1, txtstr, str4, rc);
    }
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  rc = SPHContextRename (context, str, txtstr, str);
  if (!rc)
    {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s, %p) failed", context,
        str, txtstr, str);
      return 1;
    } else {
      SASSIM_PRINT_MSG ("SPHContextRename (%p, %s, %s %p) success (%d)",
    		  context, str, txtstr, str, rc);
    }
  SASSIM_DUMP_BLOCK (context, 128 + 64);

  rc = SPHContextDestroy (context);
  if (rc)
    {
      SASSIM_PRINT_MSG ("SPHContextDestroy (%p) failed", context);
      return 1;
    }

  return 0;
}

#define NTEXTMAX 128

static int
sassim_context_test2 ()
{
  SPHContext_t context;
  SPHContext_t context0, context1, context2, context3, context4;
  char ntext[NTEXTMAX];
  int rc;

  context = getCurrentProjectContext();
  if (context)
  {
      SASSIM_PRINT_MSG ("getCurrentProjectContext () expected NULL, got %p",
    		  context);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project");
  context = SPHSetupProjectContext(ntext);
  if (context)
  {
      SASSIM_PRINT_MSG ("SPHSetupProjectContext (%s) returned %p", ntext,
    		  context);
      context0 = (SPHContext_t)getSASFinder();
      SASSIM_PRINT_MSG ("SPHSetupProjectContext() root context @%p", context0);
      context1 = getCurrentProjectContext();
      if (context == context1)
      {
          SASSIM_PRINT_MSG ("getCurrentProjectContext() returns %p matches @%p",
        		  context1, context);
      } else {
    	  SASSIM_PRINT_ERR ("getCurrentProjectContext () failed");
          return 1;
      }
  } else {
	  SASSIM_PRINT_ERR ("SPHSetupProjectContext (%s) failed", ntext);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project_two");
  context = SPHSetupProjectContext(ntext);
  if (context)
  {
      SASSIM_PRINT_MSG ("SPHSetupProjectContext (%s) returned %p", ntext,
    		  context);
      context2 = getCurrentProjectContext();
      if (context == context2)
      {
          SASSIM_PRINT_MSG ("getCurrentProjectContext() returns %p matches @%p",
        		  context2, context);
      } else {
    	  SASSIM_PRINT_ERR ("getCurrentProjectContext () failed");
          return 1;
      }
      context = (SPHContext_t)getSASFinder();
      if (context != context0)
      {
    	  SASSIM_PRINT_ERR ("getSASFinder() root context @%p changed!", context);
    	  return 1;
      }
  } else {
	  SASSIM_PRINT_ERR ("SPHSetupProjectContext (%s) failed", ntext);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project_three");
  context3 = SPHSetupAltProjectContext(ntext);
  if (context3)
  {
      SASSIM_PRINT_MSG ("SPHSetupAltProjectContext (%s) returned %p", ntext,
    		  context3);
      context = getCurrentProjectContext();
      if (context == context3)
      {
    	  SASSIM_PRINT_ERR ("getCurrentProjectContext () should not change! %p==%p",
    			  context, context3);
          return 1;
      }
  } else {
	  SASSIM_PRINT_ERR ("SPHSetupAltProjectContext (%s) failed", ntext);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project");
  context4 = getProjectContextByName(ntext);
  if (context4)
  {
      SASSIM_PRINT_MSG ("getProjectContextByName (%s) returned %p", ntext,
    		  context4);
      if (context1 != context4)
      {
    	  SASSIM_PRINT_ERR ("getProjectContextByName () should find original %s !", ntext);
          return 1;
      }
  } else {
	  SASSIM_PRINT_ERR ("getProjectContextByName (%s) failed", ntext);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project");
  rc = SPHDestroyProjectContext(ntext);
  if (!rc)
  {
      SASSIM_PRINT_MSG ("SPHDestroyProjectContext (%s) returned %d", ntext,
    		  rc);
      context = getCurrentProjectContext();
      if (context == context2)
      {
          SASSIM_PRINT_MSG ("getCurrentProjectContext() returns %p matches @%p unchanged",
        		  context2, context);
      } else {
    	  SASSIM_PRINT_ERR ("getCurrentProjectContext () failed");
          return 1;
      }
  } else {
	  SASSIM_PRINT_ERR ("SPHDestroyProjectContext (%s) failed with %d", ntext, rc);
      return 1;
  }

  snprintf(ntext, NTEXTMAX, "My_project_two");
  context = SPHRemoveProjectContext(ntext);
  if (context)
  {
      SASSIM_PRINT_MSG ("SPHRemoveProjectContext (%s) returned %p", ntext,
    		  context);
      if (context == context2)
      {
          SASSIM_PRINT_MSG ("SPHRemoveProjectContext () return value matches original",
        		  ntext, context);
      } else {
          SASSIM_PRINT_ERR ("SPHRemoveProjectContext () return %p != %p",
        		  context, context2);
      }

      context4 = getCurrentProjectContext();
      if (!context4)
      {
          SASSIM_PRINT_MSG ("getCurrentProjectContext() returns NULL, correct");
      } else {
    	  SASSIM_PRINT_ERR ("getCurrentProjectContext () returns %p, incorrect", context4);
          return 1;
      }

      rc = SPHContextDestroy(context);
      if (!rc)
      {
          SASSIM_PRINT_MSG ("SPHRemoveProjectContext() returns SPHContext_t, correct");
      } else {
          SASSIM_PRINT_MSG ("SPHRemoveProjectContext() returns incorrect SAStype");
      }
  } else {
	  SASSIM_PRINT_ERR ("SPHSetupProjectContext (%s) failed", ntext);
      return 1;
  }

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

  failures += sassim_context_test1 ();

  failures += sassim_context_test2 ();

  //SASCleanUp();
  printf("SAS removed\n");
  SASRemove();

  return failures;
}
