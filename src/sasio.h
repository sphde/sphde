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

 
// stdio for SAS loaded modules
/* This include is used by SAS loaded code to call standard IO functions.
	The implementation of this interface introduces a level of indirection
	the decouples the SAS module from the process local dependences of the
	c runtime for standard IO. 
*/

#ifndef __SJM_sasio_H
#define __SJM_sasio_H

#include <stdlib.h>
#include <stdio.h>

#include "sasiodef.h"

#if 1
#define sas_printf printf
#define sas_sprintf sprintf
#else
#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

extern __C__ int sas_printf (const char *format, ...);

extern __C__ int sas_scanf (const char *format, ...);

extern __C__ int sas_sprintf (char *buffer, const char *format, ...);

extern __C__ int sas_sscanf (const char *buffer, const char *format, ...); 

extern __C__ int sas_fprintf (void *stream, const char *format, ...);

extern __C__ int sas_fscanf (void *stream, const char *format, ...);

extern __C__ FILE * sas_fopen(const char *,const char *);

extern __C__ int sas_fclose(FILE *);

extern __C__ int sas_stat(const char *file_name, struct stat *buf);

extern __C__ int sas_fstat(int file_des, struct  stat *buf);

extern __C__ int sas_lstat(const char *file_name, struct stat *buf);

extern __C__ int sas_ftruncate(int file_des, off_t len);

extern __C__ int sas_truncate(const char *file_name, off_t len);

#undef stdin
#undef stdout
#undef stderr
#define stdin  (sas_io->va_stdin)
#define stdout (sas_io->va_stdout)
#define stderr (sas_io->va_stderr)
#endif

#endif

