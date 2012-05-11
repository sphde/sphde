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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include "sasshm.h"
#include "sasstname.h"
#include "sasio.h"
#include "saslock.h"
#include "sasmlock.h"
#include "sassimpleheap.h"

int    SasLockOwner = 0;
static sasshm_t	SasLockMemID = -1;
static SasMasterLock *ml = 0;

const static unsigned int kMasterLockSize = 256;

void
SASLockReset (void)
{
    if ( ml == 0 ) {
	fprintf(stderr, "SASLockReset: locks not initialized: exiting\n");
	return;
    }
    
    void *lock_addr = (void *)__SAS_TEMP_ADDRESS;
    SASBlockHeader *shm_locks = (SASBlockHeader*)lock_addr;

    ml = new (shm_locks) SasMasterLock(kMasterLockSize);
    setSASBlockSpecial (lock_addr, ml);
    SasLockOwner = 1;
}

void
SASLockInit (void)
{
    char *lock_addr = (char *)__SAS_TEMP_ADDRESS;
#if 0
    SasLockMemID = SASAllocateShmID (__SAS_LOCK_SHMKEY, lock_addr, 
                                     __SAS_SHMAP_MAX /* SegmentSize */ );
#else
    SasLockMemID = SASAllocateShmNameProj(sasStorePath, 'L',
                                          lock_addr, __SAS_SHMAP_MAX);
#endif
    if ( SasLockMemID != -1 )
    {
    	if (errno != EEXIST)
	{
	    SASBlockHeader *shm_locks = (SASBlockHeader*)SASSimpleHeapInit( lock_addr, 
	    						SAS_RUNTIME_LOCKTABLE, 
							__SAS_SHMAP_MAX	/* SegmentSize */ );
					
	    ml = new (shm_locks) SasMasterLock(kMasterLockSize);
	    setSASBlockSpecial (lock_addr, ml);
	    SasLockOwner = 1;
	} else {
	    ml = (SasMasterLock*) getSASBlockSpecial (lock_addr); 
	}
    }
}

void
SASLock(vm_address_t addr,
	sas_userlock_request_t lockT)
{
    ml->lock(addr, lockT);
}	
	
void
SASUnlock(vm_address_t addr)
{
    ml->unlock(addr);
}
	
void
SASLockPrintDetailedStats(void)
{
    ml->printDetailedStats();
}
	
void
SASLockPrintHighLevelStats(void)
{
    ml->printHighLevelStats();
}

void
SASLockDetach (void)
{
    char *lock_addr = (char *)__SAS_TEMP_ADDRESS;
    
    SASDetachShm( lock_addr );
}

void
SASLockRemove (void)
{
    char *lock_addr = (char *)__SAS_TEMP_ADDRESS;
    
    SASDetachShm( lock_addr );
    SASRemoveShmID( SasLockMemID );
}

void
SASPrintLockSemStats(void)
{
    int i;

    for (i=0; i<199; i++)
    {
	//SASPrintSemStats_ID(sasLockSemaphore, i);
    }
}
