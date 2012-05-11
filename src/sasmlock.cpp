/*
 * Copyright (c) 1995, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, M.P. Johnson - initial API and implementation
 */

/*
*
* DESCRIPTION:
*	See SasMasterLock.H
*/

#include "saslock.h"
#include "sasmlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include <iostream>
using namespace std;

#include "saslockl.cpp"

// This table is used to compute the hash value
static char XOMA1HASHTABLE[256] = {
  /* first set up the T table */
  '\1',  '\127',  '\61',  '\14',
  '\260',  '\262',  '\146',  '\246',
  '\171',  '\301',  '\6',  '\124',
  '\371',  '\346',  '\54',  '\243',

  '\16',  '\305',  '\325',  '\265',
  '\241',  '\125',  '\332',  '\120',
  '\100',  '\357',  '\30',  '\342',
  '\354',  '\216',  '\46',  '\310',

  '\156',  '\261',  '\150',  '\147',
  '\215',  '\375',  '\377',  '\62',
  '\115',  '\145',  '\121',  '\22',
  '\55',  '\140',  '\37',  '\336',

  '\31',  '\153',  '\276',  '\106',
  '\126',  '\355',  '\360',  '\42',
  '\110',  '\362',  '\24',  '\326',
  '\364',  '\343',  '\225',  '\353',

  '\141',  '\352',  '\71',  '\26',
  '\74',  '\372',  '\122',  '\257',
  '\320',  '\5',  '\177',  '\307',
  '\157',  '\76',  '\207',  '\370',

  '\256',  '\251',  '\323',  '\72',
  '\102',  '\232',  '\152',  '\303',
  '\365',  '\253',  '\21',  '\273',
  '\266',  '\263',  '\0',  '\363',

  '\204',  '\70',  '\224',  '\113',
  '\200',  '\205',  '\236',  '\144',
  '\202',  '\176',  '\133',  '\15',
  '\231',  '\366',  '\330',  '\333',

  '\167',  '\104',  '\337',  '\116',
  '\123',  '\130',  '\311',  '\143',
  '\172',  '\13',  '\134',  '\40',
  '\210',  '\162',  '\64',  '\12',

  '\212',  '\36',  '\60',  '\267',
  '\234',  '\43',  '\75',  '\32',
  '\217',  '\112',  '\373',  '\136',
  '\201',  '\242',  '\77',  '\230',

  '\252',  '\7',  '\163',  '\247',
  '\361',  '\316',  '\3',  '\226',
  '\67',  '\73',  '\227',  '\334',
  '\132',  '\65',  '\27',  '\203',

  '\175',  '\255',  '\17',  '\356',
  '\117',  '\137',  '\131',  '\20',
  '\151',  '\211',  '\341',  '\340',
  '\331',  '\240',  '\45',  '\173',

  '\166',  '\111',  '\2',  '\235',
  '\56',  '\164',  '\11',  '\221',
  '\206',  '\344',  '\317',  '\324',
  '\312',  '\327',  '\105',  '\345',

  '\33',  '\274',  '\103',  '\174',
  '\250',  '\374',  '\52',  '\4',
  '\35',  '\154',  '\25',  '\367',
  '\23',  '\315',  '\47',  '\313',

  '\351',  '\50',  '\272',  '\223',
  '\306',  '\300',  '\233',  '\41',
  '\244',  '\277',  '\142',  '\314',
  '\245',  '\264',  '\165',  '\114',

  '\214',  '\44',  '\322',  '\254',
  '\51',  '\66',  '\237',  '\10',
  '\271',  '\350',  '\161',  '\304',
  '\347',  '\57',  '\222',  '\170',

  '\63',  '\101',  '\34',  '\220',
  '\376',  '\335',  '\135',  '\275',
  '\302',  '\213',  '\160',  '\53',
  '\107',  '\155',  '\270',  '\321'
};

SasMasterLock::SasMasterLock(unsigned int size)
{
   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock CTOR" << endl;
   #endif

   if (size != 256)
      throw "ERROR -- only size = 256 is currently supported.";
      
   //eyecatcher = 0x6D6C636B;
   tableSize = size;
      
   //eyecatcher2 = 0x6B636C6D;
   initHashTable();
}

SasMasterLock::~SasMasterLock(void)
{
   #ifdef __SOMDebugPrint__
   cout << "Destroying master lock table..." << endl;
   #endif

   // Call DTOR for each SasUserLock in SasLockList and
   // then deallocate the storage used for that SasUserLock

   long allocSize = sizeof(SasLockList<SasUserLock, vm_address_t>);

   unsigned int loop;
   for (loop = 0; loop < tableSize; ++loop)
    {
      delete slots[loop];
    }

   // Deallocate storage used for slots (hash table)
   allocSize = sizeof(SasLockList<SasUserLock,vm_address_t> *);
   allocSize *= tableSize;
   SASNearDealloc( slots, allocSize );         // Free storage
}

//  New operator.
void* SasMasterLock::operator new(size_t size, SASBlockHeader * blockHdr )
{
   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock::operator new" << endl;
   #endif

   // Get storage for a new list node from heap in the SAS block.
   void* byteAddr = SASNearAlloc( blockHdr, size );
   return byteAddr;
}

// Delete operator.
void SasMasterLock::operator delete(void * deadObject)
{
   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock::operator delete" << endl;
   #endif

   // Return storage to heap of the SAS block the object is in.
   SASNearDealloc( deadObject, sizeof(SasMasterLock) ); 
}

void
SasMasterLock::lock(vm_address_t addr,
                    sas_userlock_request_t lockT)
{
   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock::lock" << endl;
   #endif

   long initialHash = hash((void *) &addr);
   short index = initialHash & UPPER_BYTES_MASK;

   #ifdef __SOMDebugPrint__
   cout << "Hash result: " << initialHash << endl;
   cout << "Hash index:  " << index       << endl;
   #endif

   slots[index]->lockNode(addr, lockT);
}

void
SasMasterLock::unlock(vm_address_t addr)
{
   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock::unlock" << endl;
   #endif
   long initialHash = hash((void *) &addr);
   short index = initialHash & UPPER_BYTES_MASK;
   #ifdef __SOMDebugPrint__
   cout << "Hash result: " << initialHash << endl;
   cout << "Hash index:  " << index       << endl;
   #endif
   slots[index]->unlockNode(addr);
}

void
SasMasterLock::printHighLevelStats(void)
{
   unsigned tallyOfLockObjects = 0;
   unsigned countInSlot = 0;
   unsigned tallyOfSlotsWithLocks =0;
   unsigned lockDensity = 0;

   #ifdef collectstats
   unsigned curHighestUseage = 0;
   unsigned runningHighestUseage = 0;
   unsigned runningLowestUseage = 0x1000;
   int highestUseageSlot = -1;
   int lowestUseageSlot = -1;
   #endif

   // Gather total stats
   cout << endl << "=========== HIGH LEVEL STATS ============= " << endl;
   for (unsigned int loop = 0; loop < tableSize; ++loop)
    {
      countInSlot = slots[loop]->getCount();
      if (countInSlot)
       {
         ++tallyOfSlotsWithLocks;
         tallyOfLockObjects += countInSlot;
         if (countInSlot > lockDensity)
            lockDensity = countInSlot;
         #ifdef collectstats
         curHighestUseage = slots[loop]->getHighestUseage();
         if (curHighestUseage > runningHighestUseage)
          {
            runningHighestUseage = curHighestUseage;
            highestUseageSlot = loop;
          }
         if (curHighestUseage < runningLowestUseage)
          {
            runningLowestUseage = curHighestUseage;
            lowestUseageSlot = loop;
          }
         #endif
       }
    }

   //////  OK -- got some total statistics to print, so let's do it.
   cout << "Number of table slots: " << tableSize << endl;
   cout << "Total items in table: " << tallyOfLockObjects << endl;
   cout << "Hash table load factor is totalItems/tableSize. " << endl;
   cout << "Number of table slots with items: " << tallyOfSlotsWithLocks
     << endl;
   cout << "Highest item density: " << lockDensity << endl;
   #ifdef collectstats
   cout << "Highest useage count = " << runningHighestUseage
     << " in slot #" << highestUseageSlot << endl;
   cout << "Lowest useage count = " << runningLowestUseage
     << " in slot #" << lowestUseageSlot << endl;
   #endif
}

void
SasMasterLock::printDetailedStats(void)
{
   // I'll have each slot print its detailed stats

   #ifdef collectstats
   cout << endl << "=========== LOW LEVEL DETAILED STATS ============= "
     << endl;
   for (int loop = 0; loop < tableSize; ++loop)
      slots[loop]->printStats(loop);
   #else
   cout << endl << "No detailed statistics were collected. " << endl;
   cout << "You must compile with `collectstats` defined." << endl;
   #endif
}


/////////////////////////////////////////////////////
// Private members
void
SasMasterLock::initHashTable(void)
{
   // Compute size of hash table and allocate the hash table in
   // the heap of the SAS block that "this" object is in.
                
   long allocSize = sizeof(SasLockList<SasUserLock, vm_address_t> *);
   allocSize *= tableSize;

   #ifdef __SOMDebugPrint__
   cout << "slots size = " << allocSize << endl;
   #endif

   void *byteAddr = SASNearAlloc( (void*) this, allocSize );
   slots = (SasLockList<SasUserLock, vm_address_t> **) byteAddr;

   // Now construct a SasUserLock object in each slot in the hash table

   unsigned int loop;
   for (loop = 0; loop < tableSize; ++loop)
    {
      slots[loop] = new( this ) SasLockList<SasUserLock,vm_address_t>();
    }

   #ifdef __SOMDebugPrint__
   cout << loop << " lock slots created in Master Lock." << endl;
   #endif
}


long
SasMasterLock::hash(void * kk)
{
   unsigned char *p;
   union {
      long k;
#ifdef __WORDSIZE_64
      unsigned char h[8];
#else
      unsigned char h[4];
#endif
   } ff;

   #ifdef __SOMDebugPrint__
   cout << "SasMasterLock::hash" << endl;
   cout << "  key = " << hex <<  *((int *)kk) << dec << endl;
   #endif

   ff.k = 0;
   p = (unsigned char *)kk;
#ifdef __WORDSIZE_64
   ff.h[7] = XOMA1HASHTABLE[(int)(*p)];
   ++p;
   ff.h[6] = XOMA1HASHTABLE[(int)(ff.h[7] ^ *p)];
   ++p;
   ff.h[5] = XOMA1HASHTABLE[(int)(ff.h[6] ^ *p)];
   ++p;
   ff.h[4] = XOMA1HASHTABLE[(int)(ff.h[5] ^ *p)];
   ++p;
   ff.h[3] = XOMA1HASHTABLE[(int)(ff.h[4] ^ *p)];
#else
   ff.h[3] = XOMA1HASHTABLE[(int)(*p)];
#endif
   ++p;
   ff.h[2] = XOMA1HASHTABLE[(int)(ff.h[3] ^ *p)];
   ++p;
   ff.h[1] = XOMA1HASHTABLE[(int)(ff.h[2] ^ *p)];
   ++p;
   ff.h[0] = XOMA1HASHTABLE[(int)(ff.h[1] ^ *p)];

   return (ff.k);
}


/////////////////////////////////////////////////////////

