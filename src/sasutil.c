/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - Unix command line invocation.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>

#include "sasconf.h"
#include "sassim.h"


/*!
 * \page sasutil control program for libsphde project
 *
 * \section sec1 SYNOPSIS
 * sasutil [-h] [-v] [-p path] \<command\> [\<args\>]
 *
 * \section sec2 DESCRIPTION
 * The sasutil utility is used to control de libsphde share memory segments,
 * showing its internal details (memory state/dump, lock/semaphores details,
 * etc.), memory maps and reseting its internal state.
 *
 * \section sec3 OPTIONS
 * <pre>
 * <b>-v</b>
 *     Prints the sasutil version.
 * </pre>
 *
 * <pre>
 * <b>-h</b>
 *     Prints the synopsis and a list of the available commands.
 * </pre>
 *
 * <pre>
 * <b>-p</b>
 *     Path to wherever libsphde segment is located. This also can be
 *     controlled by setting SASSTOREPATH environment variable. If not path is
 *     given, will use the current path.
 * </pre>
 *
 * \section sec4 COMMANDS
 * The available commands are: stat, detail, reset, dump, remove, list, path,
 * and map. 
 *
 * <pre>
 * <b>stat</b>
 *     Shows the overall memory statistics: total in use, total free, total
 *     uncommited, total region free, total region used, and anchor free space.
 * </pre>
 *
 * <pre>
 * <b>detail</b>
 *     Shows detailed information, dividing each statistic from <b>stat</b>
 *     command in memory block information.
 * </pre>
 *
 * <pre>
 * <b>reset</b>
 *     Resets the anchor semaphore (used to access the control shared memory
 *     segment), the allocated segments and all internal locks. This command
 *     is useful to reset the SPHDE state after an process finished with
 *     unexpected error (like a SEGFAULT).
 * </pre>
 *
 * <pre>
 * <b>dump \<address\:size\></b>
 *     Dumps a list of \<memory:size\> where \<memory\> is a memory address in
 *     hexadecimal and \<size\> is decimal. The output is similar to
 *     <b>hexdump</b> utility called with '-C' option.
 * </pre>
 *
 * <pre>
 * <b>remove</b>
 *     Removed the allocated segments, internal locks and anchor semaphores from
 *     control shared memory segment).
 * </pre>
 *
 * <pre>
 * <b>list</b>
 *     List and dump in use shared memory blocks.
 * </pre>
 *
 * <pre>
 * <b>path</b>
 *     Shows the store path used (as indicated by SASSTOREPATH).
 * </pre>
 *
 * <pre>
 * <b>map \<pid\> </b>
 *     Shows the memory map (/proc/\<pid\>/maps) for PIDs indicates as aguments.
 *     Default is current process.
 * </pre>
 *
 * \section sec5 ENVIRONMENT VARIABLES
 * The sasutil command accepts te following environment variables:
 *
 * <pre>
 * <i>SASSTOREPATH</i>
 *     Defines the path to search for the store segment to attach and analyze.
 * </pre>
 *
 * \section sec6 AUTHOR
 * Written by Steven Munroe (initial API and implementation) and Adhemerval
 * Zanella (bugfixes and API improvements).
 *
 * \section sec7 REPORTING BUG
 * Report a bug to munroesj@us.ibm.com
 *
 * \section sec8 COPYRIGHT
 *
 * Copyright (c) 2009, 2011 IBM Corporation. All rights reserved. This program
 * and the accompanying materials are made available under the terms of the
 * Eclipse Public License v1.0 which accompanies this distribution, and is
 * available at http://www.eclipse.org/legal/epl-v10.html .
 *
 * \section sec9 SEE ALSO
 */

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))
#define MAX_ADDR_LIST  8192
#define MAX_LINE_LEN   512

#ifdef __WORDSIZE_64
#define POINT_PRINT_FMT  "0x%016lx"
#define POINT_CAST_FMT   long unsigned int
#else
#define POINT_PRINT_FMT  "0x%08x"
#define POINT_CAST_FMT   unsigned int
#endif


static void sasutil_stat_cmd(int, char **);
static void sasutil_detail_cmd(int, char **);
static void sasutil_reset_cmd(int, char **);
static void sasutil_dump_cmd(int, char **);
static void sasutil_remove_cmd(int, char **);
static void sasutil_list_cmd(int, char **);
static void sasutil_path_cmd(int, char **);
static void sasutil_map_cmd(int, char **);


typedef void (*cmd_func_t)(int, char **);

const char sasutil_prog_name[] =
  "sasutil";
const char sasutil_prog_version[] =
  "0.2";
const char sasutil_usage_string[] = 
  "[-h] [-v] [-p path] <command> [<args>]\n";

const char *storepath = NULL;

struct sasutils_commands_t 
{
  const char *name;
  const char *desc;
  cmd_func_t func;
} sasutil_commands[] = 
{
  { "stat",   "show memory statistics (total in use, free, etc.)",            sasutil_stat_cmd   },
  { "detail", "show memory (default), locks (-l) or semaphores (-s) details", sasutil_detail_cmd },
  { "reset",  "resets semaphotes, locks and control structures",              sasutil_reset_cmd  },
  { "dump",   "dump the selected shared memory in hexadecimal",               sasutil_dump_cmd   },
  { "remove", "remove the shared memory segment",                             sasutil_remove_cmd },
  { "list",   "list in use memory segments",                                  sasutil_list_cmd   },
  { "path",   "show current path used",                                       sasutil_path_cmd   },
  { "map",    "display process memory maps",                                  sasutil_map_cmd    }
};

static void
sasutil_usage()
{
  printf("usage: %s %s\n", sasutil_prog_name, sasutil_usage_string);
  exit(EXIT_SUCCESS);
}

static void
sasutil_help(const char *optstr, ...)
{
  int i;

  if (optstr) {
    va_list ap;
    va_start(ap, optstr);
    vprintf(optstr, ap);
    va_end(ap);
  }

  printf("usage: %s %s\n", sasutil_prog_name, sasutil_usage_string);

  printf("The available commands are:\n");
  for (i=0; i<ARRAY_SIZE(sasutil_commands); ++i) {
    printf("  %-10s %s\n", sasutil_commands[i].name, sasutil_commands[i].desc);
  }
  exit(EXIT_SUCCESS);
}

static void
sasutil_version()
{
  printf("%s version %s\n", sasutil_prog_name, sasutil_prog_version);
  exit(EXIT_SUCCESS);
}

static void
sasutil_fatal_error(const char *optstr, ...)
{
  va_list ap;

  fprintf(stderr, "%s: error: ", sasutil_prog_name);

  va_start(ap, optstr);
  vfprintf(stderr, optstr, ap);
  va_end(ap);

  exit(EXIT_FAILURE);
}


/* utilitary functions */

static int
sasutil_get_uint(const char *str, unsigned long *value, int base)
{
  char *endptr;
  unsigned long ret = strtoll(str, &endptr, base);
  if ((*endptr != '\0') || (errno == ERANGE)) {
    return -1;
  }
  *value = ret;
  return 0;
}

static void
sasutil_join_region(const char *storepath)
{
  const char *funcName;
  int rc;

  if (storepath) {
    rc = SASJoinRegionByName ((char*)storepath);
    funcName = "SASJoinRegionByName";
  } else {
    rc = SASJoinRegion();
    funcName = "SASJoinRegion";
  }

  if (rc) {
    sasutil_fatal_error("%s failed: %i\n", funcName, rc);
  }
}

static void
sasutil_cleanup()
{
  SASCleanUp();
}

/* sasutils commands functions */

static void
sasutil_stat_cmd(int argc, char *argv[])
{
  void *addrList[MAX_ADDR_LIST];
  unsigned long sizeList[MAX_ADDR_LIST];
  int count;
  unsigned long tUsed, tFree, tUncom, tUsedReg, tFreeReg;
  unsigned int anchorFree;
  int i;

  sasutil_join_region(storepath);

  SASListInUseMem (addrList, sizeList, &count);
  tUsed = 0L;
  for (i = 0; i < count; ++i) {
    tUsed = tUsed + sizeList[i];
  };

  SASListFreeMem (addrList, sizeList, &count);
  tFree = 0L;
  for (i = 0; i < count; ++i) {
    tFree = tFree + sizeList[i];
  };

  SASListUncommittedMem (addrList, sizeList, &count);
  tUncom = 0;
  for ( i = 0; i < count; i++) {
    tUncom = tUncom + sizeList[i];
  };

  SASListFreeRegion (addrList, sizeList, &count);
  tFreeReg = 0;
  for ( i = 0; i < count; i++) {
    tFreeReg = tFreeReg + sizeList[i];
  };

  SASListAllocatedRegion (addrList, sizeList, &count);
  tUsedReg = 0;
  for ( i = 0; i < count; i++) {
    tUsedReg = tUsedReg + sizeList[i];
  };

  anchorFree = SASAnchorFreeSpace();

  printf ("Total in use      %ldKB\n", (tUsed/1024));
  printf ("Total free        %ldKB\n", (tFree/1024));
  printf ("Total Uncommitted %ldKB\n", (tUncom/1024));
  printf ("Total Region free %ldKB\n", (tFreeReg/1024));
  printf ("Total Region used %ldKB\n", (tUsedReg/1024));
  printf ("Anchor Free Space %d\n",    (anchorFree));

  sasutil_cleanup();
}


static void
sasutil_detail_cmd(int argc, char *argv[])
{
  void *addrList[MAX_ADDR_LIST];
  unsigned long sizeList[MAX_ADDR_LIST];
  int count;
  unsigned long tUsed, tFree, tUncom, tUsedReg, tFreeReg;
  int i;

  sasutil_join_region(storepath);

  SASListInUseMem (addrList, sizeList, &count);
  printf ("Memory in use:\n");
  tUsed = 0L;
  for (i = 0; i < count; ++i) {
    tUsed = tUsed + sizeList[i];
    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
  }
  printf ("Total in use:      %ldKB\n\n", (tUsed/1024));

  SASListFreeMem (addrList, sizeList, &count);
  printf ("Memory free:\n");
  tFree = 0L;
  for (i = 0; i < count; ++i) {
    tFree = tFree + sizeList[i];
    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
  }
  printf ("Total free:        %ldKB\n\n", (tFree/1024));

  SASListUncommittedMem (addrList, sizeList, &count);
  printf ("Memory uncommitted:\n");
  tUncom = 0;
  for (i = 0; i < count; ++i) {
    tUncom = tUncom + sizeList[i];
    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
  }
  printf ("Total Uncommitted: %ldKB\n\n", (tUncom/1024));

  SASListFreeRegion (addrList, sizeList, &count);
  printf ("Region free:\n");
  tFreeReg = 0;
  for (i = 0; i < count; ++i) {
    tFreeReg = tFreeReg + sizeList[i];
    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
  }
  printf ("Total Region free: %ldKB\n\n", (tFreeReg/1024));

  SASListAllocatedRegion (addrList, sizeList, &count);
  printf ("Region allocated:\n");
  tUsedReg = 0;
  for (i = 0; i < count; ++i) {
    tUsedReg = tUsedReg + sizeList[i];
    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
  }
  printf ("Total Region used: %ldKB\n", (tUsedReg/1024));

  sasutil_cleanup();
}


static void
sasutil_reset_cmd(int argc, char *argv[])
{
  sasutil_join_region(storepath);
  SASResetSem();
  SASReset();
  sasutil_cleanup();
}


static void
sasutil_dump_block(void *blockAddr, unsigned long len)
{
  unsigned int  *dumpAddr = (unsigned int*)blockAddr;
  unsigned char *charAddr = (unsigned char*)blockAddr;
  void          *tempAddr;
  unsigned char chars[20];
  unsigned char temp;
  unsigned int  i, j;

  chars[16] = 0;
  for (i = 0; i<len; i=i+16) {
    tempAddr = dumpAddr;
    for (j=0; j<16; j++) {
      temp = *charAddr++;
      if ( (temp < 32) || (temp > 126) )
	temp = '.';
      chars[j] = temp;
    };

      printf ("%p  %08x %08x %08x %08x <%s>\n", tempAddr, *dumpAddr,
	      *(dumpAddr+1), *(dumpAddr+2), *(dumpAddr+3), chars);
      dumpAddr += 4;
  }
}

static void
sasutil_dump_cmd(int argc, char *argv[])
{
  int i;

  sasutil_join_region(storepath);

  printf("Dumping memory blocks:\n");
  for (i=0; i<argc; ++i) {
    char *memoryAddrStr;
    void* memoryAddr;
    char *memoryLenStr;
    unsigned long memoryLen;
    unsigned long value;

    memoryAddrStr = strtok(argv[i], ":");
    if (sasutil_get_uint(memoryAddrStr, &value, 16)) {
      fprintf(stderr, "  Error: invalid argument: %s\n", memoryAddrStr);
      exit(EXIT_FAILURE);
    }
    memoryAddr = (void*)value;

    memoryLenStr = strtok(NULL, ":");
    if (sasutil_get_uint(memoryLenStr, &memoryLen, 10)) {
      fprintf(stderr, "  Error: invalid argument: %s\n", memoryAddrStr);
      exit(EXIT_FAILURE);
    }

    printf("\nStart address " POINT_PRINT_FMT " - size %lu\n", (POINT_CAST_FMT)value, memoryLen);
    sasutil_dump_block(memoryAddr, memoryLen);
  }
  sasutil_cleanup();
}


static void
sasutil_remove_cmd(int argc, char *argv[])
{
  sasutil_join_region(storepath);
  SASRemove();
}


static void
sasutil_list_cmd(int argc, char *argv[])
{
  void *addrList[MAX_ADDR_LIST];
  unsigned long sizeList[MAX_ADDR_LIST];
  int count;
  unsigned long tUsed;
  int i;

  static union {
    char sasChar[4];
    int  sasInt;
  } sasTag = { 
    { 'S', 'A', 'S', ' ' } 
  };


  sasutil_join_region(storepath);

  printf ("Listing in use memory:\n");
  SASListInUseMem (addrList, sizeList, &count);

  tUsed = 0L;
  for (i = 0; i < count; ++i) {
    tUsed = tUsed + sizeList[i];

    printf ("%03d: %p - %ldKB\n", i, addrList[i], (sizeList[i]/1024));
    if (i != 0) {
      int *tmpptr = (int*)addrList[i];
      if (*tmpptr == sasTag.sasInt) {
	printf(" %s\n", (char*)addrList[i]);
      } else {
	unsigned long *temp = (unsigned long *)addrList[i];
	unsigned long count = sizeList[i]/ sizeof(unsigned long);
	unsigned long i;

	printf("starting scan @ %p, %ld\n",temp, count);
	for (i = 0; i < count; ++i) {
	  if (*temp) 
	    printf ("scaned %lx @ %p\n",*temp,temp);
	  temp++;
	};
	printf("scan complete @%p\n",temp);
      }
    }
  }
  printf ("Total in use:      %ldKB\n", (tUsed/1024));

  sasutil_cleanup();
}


static void
sasutil_path_cmd(int argc, char *argv[])
{
  const char *default_path = "defaults to .";
  const char *sas_path;

  sas_path = getenv("SASSTOREPATH");
  if (sas_path != NULL)
    default_path = sas_path;
  printf ("SASSTOREPATH %s\n", default_path);
}


static const char*
sasutil_extract_name(const char *cmdline)
{
  const char *slash;
  if (!cmdline || !*cmdline)
    return NULL;
  slash = cmdline + strlen(cmdline);

  while (cmdline <= slash && (*slash != '/'))
    slash--;

  if (slash >= cmdline)
    return slash + 1;
  return cmdline;
}

static const char*
sasutil_get_proc_name(pid_t pid)
{
  char path[MAXPATHLEN];
  static char procname[MAX_LINE_LEN];
  FILE *arq;

  procname[0] = '\0';

  snprintf(path, MAXPATHLEN, "/proc/%d/cmdline", pid);
  arq = fopen(path, "r");
  if (!arq) {
     path[0] = '\0';
  } else {
     int ret = fread(procname, MAX_LINE_LEN, 1, arq);
     fclose(arq);
     if (ret >= 1)
       return sasutil_extract_name(procname);
  }
  return procname;
}

static void
sasutil_map_cmd(int argc, char *argv[])
{
  FILE *arq;
  char path[MAXPATHLEN];
  char line[MAX_LINE_LEN];
  int i;

  for (i=0; i<argc; ++i) {
    pid_t pid;
    unsigned long uintpid;
    if (sasutil_get_uint(argv[i], &uintpid, 10)) {
      fprintf(stderr, "  Error: invalid argument: %s\n", optarg);
      exit(EXIT_FAILURE);
    }
    pid = uintpid;

    printf("Displaying memory maps for PID %d (%s)\n", (int)pid, 
 	   sasutil_get_proc_name(pid));
    snprintf(path, MAXPATHLEN, "/proc/%d/maps", pid);
    arq = fopen(path, "r");
    if (!arq) {
      fprintf(stderr, "  Error: could not open %s map file\n", path);
      exit(0);
    }
    while (fgets(line, MAX_LINE_LEN, arq)) {
      printf("%s", line);
    }
  }
}



int
main(int argc, char *argv[])
{
  int opt, i;
  const char *cmd;

  while ((opt = getopt(argc, argv, "hvp:")) != -1) {
    switch (opt) {
    case 'h':
      sasutil_help(NULL);
    case 'v':
      sasutil_version();
    case 'p':
      storepath = optarg;
      break;
    default:
      sasutil_help("invalid options: %c\n", opt);
    }
  }

  if (optind >= argc) {
    sasutil_usage();
  }
  cmd = argv[optind];

  for (i=0; i<ARRAY_SIZE(sasutil_commands); ++i) {
    if (strcmp(cmd, sasutil_commands[i].name) == 0) {
      /* The +1 is to not pass the command itself */
      sasutil_commands[i].func(argc - (optind + 1), argv + (optind + 1));

      exit(EXIT_SUCCESS);
    }
  }
  sasutil_help("invalid command: %s\n", cmd);


  return 0;
}
