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

#ifndef __SJM_sasstdio_H
#define __SJM_sasstdio_H

/* This header contains the linkage structures the bind the sas IO functions
	back into the main programs standard IO library. The Main program must use
	the SAS_IO_INIT macro to allocate the structure and initial pointers to
	standard IO functions. The function in the SAS IO library will use these
	pointer to call back into the main programs standard IO library.
*/

#include <stdio.h>

#include "sasiodef.h"

#if 1
#define SAS_IO_INIT
#else

int sas_vscanf(const char *format, va_list arglist)
{
    void *args[16];
    int arg_count = 0;
    int ret = 0;
    const char	*next;

    for (next = format; *next; next++)
	{
	if (*next == '%')
	    {
	    if (next[1] == '%')
		{
		next++;
		} else {
		    args[arg_count] = va_arg(arglist,void*);
		    if (arg_count < 16)
			{
			arg_count++;
			};
		    };
	    };
	};

	switch (arg_count) {
	    case 1:
		ret = scanf(format, args[0]);
		break;
	    case 2:
		ret = scanf(format, args[0], args[1]);
		break;
	    case 3:
		ret = scanf(format, args[0], args[1], args[2]);
		break;
	    case 4:
		ret = scanf(format, args[0], args[1], args[2], args[3]);
		break;
	    case 5:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4]);
		break;
	    case 6:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5]);
		break;
	    case 7:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6]);
		break;
	    case 8:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7]);
		break;
	    case 9:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8]);
		break;
	    case 10:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9]);
		break;
	    case 11:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10]);
		break;
	    case 12:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10], args[11]);
		break;
	    case 13:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10], args[11], args[12]);
		break;
	    case 14:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10], args[11], args[12], args[13]);
		break;
	    case 15:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10], args[11], args[12], args[13], args[14]);
		break;
	    case 16:
		ret = scanf(format, args[0], args[1], args[2], args[3], args[4],
			    args[5], args[6], args[7], args[8], args[9],
			    args[10], args[11], args[12], args[13], args[14],
			    args[15]);
		break;
	    default :
		ret = 0;
	    };

	    return ret;
};

int sas_vfscanf(FILE *stream, const char *format, va_list arglist)
{
    void *args[16];
    int arg_count = 0;
    int ret = 0;
    const char	*next;

    for (next = format; *next; next++)
	{
	if (*next == '%')
	    {
	    if (next[1] == '%')
		{
		next++;
		} else {
		    args[arg_count] = va_arg(arglist,void*);
		    if (arg_count < 16)
			{
			arg_count++;
			};
		    };
	    };
	};

	switch (arg_count) {
	    case 1:
		ret = fscanf(stream, format, args[0]);
		break;
	    case 2:
		ret = fscanf(stream, format, args[0], args[1]);
		break;
	    case 3:
		ret = fscanf(stream, format, args[0], args[1], args[2]);
		break;
	    case 4:
		ret = fscanf(stream, format, args[0], args[1], args[2], args[3]);
		break;
	    case 5:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4]);
		break;
	    case 6:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5]);
		break;
	    case 7:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6]);
		break;
	    case 8:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7]);
		break;
	    case 9:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8]);
		break;
	    case 10:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9]);
		break;
	    case 11:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10]);
		break;
	    case 12:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11]);
		break;
	    case 13:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12]);
		break;
	    case 14:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13]);
		break;
	    case 15:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13], args[14]);
		break;
	    case 16:
		ret = fscanf(stream, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13], args[14],
			     args[15]);
		break;
	    default :
		ret = 0;
	    };

	    return ret;
};
int sas_vsscanf(const char *buffer, const char *format, va_list arglist)
{
    void *args[16];
    int arg_count = 0;
    int ret = 0;
    const char	*next;

    for (next = format; *next; next++)
	{
	if (*next == '%')
	    {
	    if (next[1] == '%')
		{
		next++;
		} else {
		    args[arg_count] = va_arg(arglist,void*);
		    if (arg_count < 16)
			{
			arg_count++;
			};
		    };
	    };
	};

	switch (arg_count) {
	    case 1:
		ret = sscanf(buffer, format, args[0]);
		break;
	    case 2:
		ret = sscanf(buffer, format, args[0], args[1]);
		break;
	    case 3:
		ret = sscanf(buffer, format, args[0], args[1], args[2]);
		break;
	    case 4:
		ret = sscanf(buffer, format, args[0], args[1], args[2], args[3]);
		break;
	    case 5:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4]);
		break;
	    case 6:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5]);
		break;
	    case 7:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6]);
		break;
	    case 8:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7]);
		break;
	    case 9:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8]);
		break;
	    case 10:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9]);
		break;
	    case 11:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10]);
		break;
	    case 12:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11]);
		break;
	    case 13:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12]);
		break;
	    case 14:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13]);
		break;
	    case 15:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13], args[14]);
		break;
	    case 16:
		ret = sscanf(buffer, format,
			     args[0], args[1], args[2], args[3], args[4],
			     args[5], args[6], args[7], args[8], args[9],
			     args[10], args[11], args[12], args[13], args[14],
			     args[15]);
		break;
	    default :
		ret = 0;
	    };

	    return ret;
};

void SASIOINIT()
{
    static sas_io_t	io; /* allocate storage in main */
#ifndef __SAS__
    sas_io = &io; /* set linkage pointer to the SAS IO structure */
    sas_io->va_print = vprintf; /* set printf function pointer */
    sas_io->va_scan  = sas_vscanf; /* set scanf function pointer */
    sas_io->va_sprint= vsprintf; /* set sprintf function pointer */
    sas_io->va_sscan = sas_vsscanf; /* set sscanf function pointer */
    sas_io->va_fprint= (va_file_t)vfprintf; /* set fprintf function pointer */
    sas_io->va_fscan = (va_file_t)sas_vfscanf; /* set fscanf function pointer */

    sas_io->va_stdin = stdin; /* set pointer to FILE stdin */
    sas_io->va_stdout= stdout; /* set pointer to FILE stdout */
    sas_io->va_stderr= stderr; /* set pointer to FILE stderr */
    
    sas_io->fstat    = fstat; /* set pointer to fstat */
    sas_io->lstat    = lstat; /* set pointer to lstat */
    sas_io->stat     = stat; /* set pointer to stat */
    
    sas_io->ftruncate= ftruncate; /* set pointer to ftruncate */
    sas_io->truncate = truncate; /* set pointer to truncate */
#else
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;	
    taskAnchor->sas_io = &io; /* set linkage pointer to the SAS IO structure */
    taskAnchor->sas_io->va_print = vprintf; /* set printf function pointer */
    taskAnchor->sas_io->va_scan  = sas_vscanf; /* set scanf function pointer */
    taskAnchor->sas_io->va_sprint= vsprintf; /* set sprintf function pointer */
    taskAnchor->sas_io->va_sscan = sas_vsscanf; /* set sscanf function pointer */
    taskAnchor->sas_io->va_fprint= (va_file_t)vfprintf; /* set fprintf function pointer */
    taskAnchor->sas_io->va_fscan = (va_file_t)sas_vfscanf; /* set fscanf function pointer */

    taskAnchor->sas_io->va_stdin = stdin; /* set pointer to FILE stdin */
    taskAnchor->sas_io->va_stdout= stdout; /* set pointer to FILE stdout */
    taskAnchor->sas_io->va_stderr= stderr; /* set pointer to FILE stderr */
    
    taskAnchor->sas_io->fstat    = fstat; /* set pointer to fstat */
    taskAnchor->sas_io->lstat    = lstat; /* set pointer to lstat */
    taskAnchor->sas_io->stat     = stat; /* set pointer to stat */
    
    taskAnchor->sas_io->ftruncate= ftruncate; /* set pointer to ftruncate */
    taskAnchor->sas_io->truncate = truncate; /* set pointer to truncate */

#endif /* __SAS__*/
};

#define SAS_IO_INIT SASIOINIT();
#endif

#endif

