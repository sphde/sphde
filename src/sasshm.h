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
 * SAS SYSV IPC Shared Memory
 */
 
#ifndef __SAS_SHM_H
#define __SAS_SHM_H

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif
#ifdef __WORDSIZE_64
#define __SAS_DEFAULT_SHMKEY 0x5341534D
#else
#define __SAS_DEFAULT_SHMKEY 0x5341324D
#endif

typedef int sasshm_t;

extern __C__ int sasshm_id;

extern __C__ int SASAllocateShm( void* addr, long size );

extern __C__ int SASAllocateShmID(key_t key_id, void* addr, long size );

extern __C__ int SASAllocateShmID_clear(key_t key_id, void* addr, long size );

extern __C__ int SASAllocateShmName(char *key_name, void* addr, long size );

extern __C__ int SASAllocateShmNameProj( char* key_name, char proj,
                                        void* addr, long size );

extern __C__ void SASDetachShm( void* addr );

extern __C__ void SASRemoveShm( void );

extern __C__ void SASRemoveShmID(int shm_id );

#endif
