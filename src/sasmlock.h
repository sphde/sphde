/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Maynard Johnson - initial API and implementation
 */

#ifndef _SASMASTERLOCK_H
#define _SASMASTERLOCK_H

/*
*	
* DESCRIPTION:
* This file contains the SasMasterLock class which is intended to be
* used by the TPC-C benchmark.  The user should create one and only one
* SasMasterLock object and it can be used to facilitate inter-task and
* inter-thread locking of user objects.
*
* ASSOCIATED CLASSES    FILENAMES
*       SasLockList     SasLockList.H
*       SasUserLock     SasUserLock.H
*
* USAGE:
*	-One task creates the SasMasterLock object (at this time,
*       size must be 256) and puts it in SAS storage
*	-Tasks and threads may now use the lock() and unlock() methods on
*	the MasterLock object (assuming they have addressability
*	to the MasterLock) to synchronize access to an object.
*       For example:
*               (1) lock(myObjectAddr, SasUserLock__WRITE);
*               (2) // do something with myObject...
*               (3) unlock(myObjectAddr);
*
*       NOTE:  A thread may obtain multiple write locks or read locks
*       on a given object.  However, if a read lock is obtained first,
*       attempting to obtain a write lock afterwards will cause the
*       thread to hang.  Conversely, if a write lock is obtained first
*       it is possible for the thread to later obtain a read lock.
*
*       -This class also enables collection and printing of lock table
*       statistics, such as distribution info (i.e., load factor) and
*       usage.  The printHighLevelStats() method prints load factor
*       info and other totals.  printDetailedStats() prints info
*       on each table entry containing one or more locks, and also
*       prints out how many times each lock was locked (usage).
*       In order to collect and print this detailed information, the
*       user must compile with "collectstats" defined.
*
* DESTRUCTION
*	-If the MasterLock object is newed-up by the user, then it
*       is the responsibility of the user to delete it.  This is
*       probably the only way that TPC-C would create a MasterLock.
*
*/

#include "sasalloc.h"          // for SASBlockHeader
#include "sasulock.h"       
#include "saslockl.h"       // for sas_userlock_request_t

#define UPPER_BYTES_MASK  0x000000FF 
#ifdef __WORDSIZE_64
#define __SAS_LOCK_SHMKEY 0x4c4f434b
#else
#define __SAS_LOCK_SHMKEY 0x4c4f324b
#endif


class SasMasterLock {
public:
  SasMasterLock(unsigned int size);
  ~SasMasterLock(void);

  void * operator new( size_t size, SASBlockHeader * blockHdr );
  void operator delete( void *deadObject );

  void lock(vm_address_t addr, sas_userlock_request_t lockT);
  void unlock(vm_address_t addr);
  void printDetailedStats(void);
  void printHighLevelStats(void);
    
private:
  //unsigned int eyecatcher;
  unsigned int tableSize;
  SasLockList<SasUserLock,vm_address_t> ** slots;
  //unsigned int eyecatcher2;
  
  long hash(void * kk);
  void initHashTable(void);

  //--------------------------------------------------------------------
  // Disallow default CTOR, copy CTOR, and assignment operator.
  // A compilation error will occur if the client code requires the use
  // of the disallowed functions.
  //--------------------------------------------------------------------
  SasMasterLock(void);                          // default ctor
  SasMasterLock(const SasMasterLock &);         // default copy ctor
  SasMasterLock & operator=(const SasMasterLock &);// assignment operator

};



#endif /* _SasMasterLock_H */

