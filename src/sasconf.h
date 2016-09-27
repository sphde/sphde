/*
 * Copyright (c) 2003-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SASCONF__H
#define __SASCONF__H

#include <sys/param.h>

#if defined(__powerpc64__) || defined(__x86_64__)
# define __BIGREGION__ 1
#else
# undef __BIGREGION__
#endif

/* maximum length (including trailing null) of the path
   and file name for the segment stores.  */
#define STORE_NAME_SIZE MAXPATHLEN

/*
 * For each platform establish the VA range (base address 
 * and region size) for the shared region. This should a range
 * of virtual address that are not normally allocated to the 
 * kernel or the process. For Linux this is usually between
 * the start of anonymous memory and the process stack.
 *
 * Next establish the maximum allocation (segment) size for shared 
 * segment units with the shared region and the maximum size of
 * segment store files.
 */
#ifdef __powerpc__
# ifdef __powerpc64__
#  define	__WORDSIZE_64
#  ifdef __BIGREGION__
#   define	__SAS_BASE_ADDRESS	0x80000000000L
#   define	RegionSize  		0x40000000000L	/* 4TB */
#   define	SegmentSize		0x00010000000L	/* 256MB */
#  else
#   define	__SAS_BASE_ADDRESS	0x0a000000000L
#   define	RegionSize  		0x04000000000L	/* 256GB */
#   define	SegmentSize		0x00010000000L	/* 256MB */
#  endif
#  define	__SAS_SHMAP_MAX	0x1000000L	/* 16MB */
# else
#  ifdef __BIGREGION__
#   define	__SAS_BASE_ADDRESS	0x80000000UL
#   define	RegionSize		0x40000000UL	/* 1GB */
#   define	SegmentSize		0x01000000UL	/*  16MB */
#   define	__SAS_SHMAP_MAX		0x1000000UL
#  else
#   define	__SAS_BASE_ADDRESS	0x60000000UL
#   define	RegionSize		0x10000000UL	/* 256MB */
#   define	SegmentSize		0x01000000UL	/*  16MB */
#  endif
# endif
#endif

#ifdef __x86_64__
#  define	__WORDSIZE_64
#  define	__SAS_BASE_ADDRESS	0x400000000000L
#  define	RegionSize  		0x200000000000L	/* 32TB */
# define	SegmentSize		0x000010000000L	/* 256MB */
#  define	__SAS_SHMAP_MAX		0x000001000000L	/* 16MB */
#endif

#ifdef __s390x__
# define     __WORDSIZE_64
# define     __SAS_BASE_ADDRESS      0x20000000000L   /* 2TB */
# define     RegionSize              0x10000000000L   /* 1TB */
# define     SegmentSize             0x00000400000L   /* 4MB */
# define     __SAS_SHMAP_MAX         0x00000400000L   /* 4MB */
#endif

#ifdef __aarch64__
# define     __WORDSIZE_64
# define     __SAS_BASE_ADDRESS      0x4000000000L   /* 512GB */
# define     RegionSize              0x2000000000L   /* 128GB */
# define     SegmentSize             0x0010000000L   /* 256MB */
# define     __SAS_SHMAP_MAX         0x0001000000L   /* 16MB */
#endif

#ifdef __arm__
# define     __SAS_BASE_ADDRESS      0x60000000UL
# define     RegionSize              0x20000000UL    /* 512MB */
# define     SegmentSize             0x01000000UL    /*  16MB */
# define     __SAS_SHMAP_MAX         0x01000000UL    /*  16MB */
#endif

/* 
 * If the platform is not recognized above, select some resonable default.
 */
#ifndef __SAS_BASE_ADDRESS
# ifdef __GNUC__
#  define __SAS_BASE_ADDRESS	0x60000000UL
# else
#  define __SAS_BASE_ADDRESS	0x40000000UL
# endif
# define	RegionSize	0x20000000UL	/* 512MB */
# define	SegmentSize	0x00400000UL	/*   4MB */
#endif


#define __SAS_TEMP_ADDRESS (__SAS_BASE_ADDRESS + RegionSize + SegmentSize)

#define __SAS_TEMP_FREE    (__SAS_BASE_ADDRESS + RegionSize + 4*SegmentSize)


#ifndef __SAS_SHMAP_MAX
# define __SAS_SHMAP_MAX SegmentSize
#endif

#ifndef __WORDSIZE_64
# ifndef __WORDSIZE_32
#  define __WORDSIZE_32
# endif
#endif

#endif

