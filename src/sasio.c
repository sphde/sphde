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

// SAS IO implimentation
/*
	This module can be linked with SAS shared libraries to support standard C
	IO.
*/
#if 0

#include "sasio.h"
#include "sasiodef.h"
#ifdef __SAS__
#include "sasStuff.H"
#endif

// next line should be removed when integrated with SAS ...
#ifndef __SAS__
sas_io_t	*sas_io;
#endif

int sas_truncate(const char *file_name, off_t len)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->truncate)(file_name, len);
#else
  return (*sas_io->truncate)(file_name, len);
#endif
}

int sas_ftruncate(int file_des, off_t len)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->ftruncate)(file_des, len);
#else
  return (*sas_io->ftruncate)(file_des, len);
#endif
}

int sas_stat(const char *file_name, struct stat *buf)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->stat)(file_name, buf);
#else
  return (*sas_io->stat)(file_name, buf);
#endif
}

int sas_fstat(int file_des,struct  stat *buf)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->fstat)(file_des, buf);
#else
  return (*sas_io->fstat)(file_des, buf);
#endif
}

int sas_lstat(const char *file_name, struct stat *buf)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->lstat)(file_name, buf);
#else
  return (*sas_io->lstat)(file_name, buf);
#endif
}

FILE *sas_fopen(const char * Path,const char * Type)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->fopen)(Path,Type);
#else
  return (*sas_io->fopen)(Path,Type);
#endif
}

int sas_fclose(FILE * Stream)
{
#ifdef __SAS__
  task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
  return (taskAnchor->sas_io->fclose)(Stream);
#else
  return (*sas_io->fclose)(Stream);
#endif
}

int sas_printf (const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_print)(format, pa);
#else
	count = (*sas_io->va_print)(format, pa);
#endif
	va_end(pa);
	return count;
};


int sas_scanf (const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_scan)(format, pa);
#else
	count = (*sas_io->va_scan)(format, pa);
#endif
	va_end(pa);
	return count;
};

int sas_sprintf (char *buffer, const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_sprint)(buffer,format, pa);
#else
	count = (*sas_io->va_sprint)(buffer, format, pa);
#endif
	va_end(pa);
	return count;
};

int sas_sscanf (const char *buffer, const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_sscan)(buffer, format, pa);
#else
	count = (*sas_io->va_sscan)(buffer, format, pa);
#endif
	va_end(pa);
	return count;
};

int sas_fprintf (void *stream, const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_fprint)(stream, format, pa);

#else
	count = (*sas_io->va_fprint)(stream, format, pa);
#endif
	va_end(pa);
	return count;
};

int sas_fscanf (void *stream, const char *format, ...)
{	va_list pa;
	int	count;
	va_start(pa, format);
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	count = (taskAnchor->sas_io->va_fscan)(stream, format, pa);
#else
	count = (*sas_io->va_fscan)(stream, format, pa);
#endif
	va_end(pa);
	return count;
};

#endif
