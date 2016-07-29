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

#ifndef __SJM_sasiodef_H
#define __SJM_sasiodef_H
// stdio SAS linkage structure
/* This header contains the linkage structures the bind the SAS IO functions
	back into the main programs standard IO library. The Main program must use
	the sasstdio.h include and SAS_IO_INIT macro to allocate the structure and
	initial pointers to standard IO functions. The function in the SAS IO
	library will use these pointer to call back into the main programs
	standard IO library.
*/


#include <stdarg.h>
#include <stdio.h>
#if 0
#include <sys/stat.h>

typedef int (*va_file_t)(void *, const char *, va_list);

typedef struct {
		int (*va_print)(const char *, va_list);
		int (*va_scan)(const char *, va_list);
		int (*va_sprint)(char *, const char *, va_list);
		int (*va_sscan)(const char *, const char *, va_list);
		va_file_t va_fprint;
		va_file_t va_fscan;
		void *va_stdin;
		void *va_stdout;
		void *va_stderr;
		FILE *(*fopen)(const char *,const char *);
		int (*fclose)(FILE *);
		int (*fstat)(int, struct stat *);
		int (*lstat)(const char *, struct stat *);
		int (*stat)(const char *, struct stat *);
		int (*ftruncate)(int, off_t);
		int (*truncate)(const char *, off_t);
	} sas_io_t;

// This pointer should be in coerced copy-on-write SAS memory
#ifndef __SAS__
extern sas_io_t	*sas_io;
#endif
#endif
#endif

