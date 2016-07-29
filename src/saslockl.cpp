/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     IBM Corporation, M.P. Johnson - initial API and implementation
 */

#ifndef _SASLOCKLIST_C
#define _SASLOCKLIST_C

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "saslockl.h"
extern "C" {
/*   #include <mach.h> */
   #include <stddef.h>
/*   #include <cthreads.h> */
}
#include <new>
using namespace std;

#include "sasalloc.h"

//----------------------------------------------------------------------------
// Node methods
//----------------------------------------------------------------------------


// Node constructor
template<class Item, class Key>
SasLockListNode<Item, Key>::SasLockListNode(Item *itemPtr,
                                            SasLockListNode<Item, Key>
                                            *nextNodePtr)
: item(itemPtr), next(nextNodePtr)
{
   // intentionally left blank
}

// Node destructor
template<class Item, class Key>
SasLockListNode<Item, Key>::~SasLockListNode(void)
{
   // NOTE - CTOR was given pointer to item. So, DTOR should NOT
   // delete the item! Instead, let client of this class determine
   // when to DTOR the item.
}

// Node New operator.
template<class Item, class Key>
void*
SasLockListNode<Item, Key>::operator new
( size_t size, SasLockList<Item,Key> * const lockList )
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   // Get storage for a new list node from heap in the same
   // SAS block that the SasLockList object is in.

   void *byteAddr = SASNearAlloc( (void*) lockList, size );
   return byteAddr;
}

// Node Delete operator.
template<class Item, class Key>
void
SasLockListNode<Item, Key>::operator delete( void *deadObject )
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   // Return storage to heap of the SAS block the object is in.
   SASNearDealloc( deadObject, sizeof(SasLockListNode<Item, Key>) );
}

//----------------------------------------------------------------------------
// List methods
//----------------------------------------------------------------------------

// List constructor
template<class Item, class Key>
SasLockList<Item, Key>::SasLockList(void)
: headPtr(NULL), count(0)
{
   void *byteAddr = SASNearAlloc( (void*) this, sizeof(SasUserLock) );
   lock = new(byteAddr) SasUserLock();
}

// List destructor
template<class Item, class Key>
SasLockList<Item, Key>::~SasLockList(void)
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   SasLockListNode<Item,Key> * tempPtr = NULL;
   SasLockListNode<Item,Key> * iteratorPtr;

   lock->write_lock();

   // Remove each node on the list
   iteratorPtr = headPtr;
   while (iteratorPtr != NULL)
    {
      // No need to lock items before deleting--no other thread or task ever has
      // a pointer to these items except via this list.

      // DTOR and free the Item that "this" at.
      Item *p = iteratorPtr->item;        // point to item to free
      p->~Item();                         // DTOR item
      SASNearDealloc( p, sizeof(Item) );  // free item storage

      // Now DTOR and free the SasLockListNode itself
      tempPtr = iteratorPtr;
      iteratorPtr = iteratorPtr->next;
      delete tempPtr;
    }

   // DTOR and free the SasUserLock that "this" object points to.
   lock->~SasUserLock();                            // DTOR
   SASNearDealloc( lock, sizeof(SasUserLock) );     // free storage
   return;
}

// List New operator.
template<class Item, class Key>
void*
SasLockList<Item, Key>::operator new
(size_t size, SasMasterLock * const masterLock )
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   // Get storage for a new list node from heap in the SAS block.
   void* byteAddr = SASNearAlloc( masterLock, size );
   return byteAddr;
}

// List Delete operator.
template<class Item, class Key>
void
SasLockList<Item, Key>::operator delete(void * deadObject)
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   // Return storage to heap of the SAS block the object is in.
   SASNearDealloc( deadObject, sizeof(SasLockList<Item, Key>) );
}

template<class Item, class Key>
boolean_t
SasLockList<Item, Key>::isEmpty(void) const
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif
   lock->read_lock();
   if (count == 0)
    {
      lock->unlock();
      return TRUE;
    }

   lock->unlock();
   return FALSE;
}

// Find the node matching keyAddr and lock it.  If the specific node can't be
// found, use any free node we may find in the list and lock that
// (simultaneously, setting the free node's address datamember to the keyAddr).
// If the specific node can't be found, and there is no free node in the list,
// then new-up a SasUserLock object and add it to the list.
template<class Item, class Key>
void
SasLockList<Item, Key>::lockNode(Key keyAddr, sas_userlock_request_t lockT)
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif
   SasLockListNode<Item, Key> * iteratorPtr;
   SasUserLock * firstFreeLock = NULL;

   lock->write_lock();

   // Search the list for the specified key
   for (iteratorPtr = headPtr;
        iteratorPtr != NULL;
        iteratorPtr = iteratorPtr->next)
    {
      if (iteratorPtr->item->operator==(keyAddr))
       {
#ifdef __SOMDebugPrint__
         fprintf (stderr, "%s - found the lock\n", __FUNCTION__);
#endif

         // If we found the lock we're after, great--let's go ahead and lock it.
         // NOTE:  To prevent the task from locking the slot and then going to
         // sleep while waiting for the desired keyAddr lock (thus preventing
         // all other tasks from accessing this slot), we pass in a pointer to
         // the slot's lock object. The SasUserLock object can unlock the slot
         // right after getting its spin lock, but before going for the keyAddr
         // lock.

         switch (lockT) {
            case SasUserLock__READ:
               iteratorPtr->item->read_lock(lock, keyAddr);	// lock item
               return;
            case SasUserLock__WRITE:
               iteratorPtr->item->write_lock(lock, keyAddr);	// lock item
               return;
          }
       }   // end 'if..==keyAddr' Mark the first free lock object we find so
      // that in case we don't find
      // an existing lock for this keyAddr, we'll be able to immediately get
      // a free lock object and use it.
      if (firstFreeLock == NULL)
         if (iteratorPtr->item->operator==(NullAddress))
            firstFreeLock = iteratorPtr->item;

    }	// end 'for' loop

   // Specified node was not found.  Check if there's a free node to use
   if (firstFreeLock != NULL)
    {
#ifdef __SOMDebugPrint__
      fprintf (stderr, "%s - found a free lock to use\n", __FUNCTION__);
#endif
      switch (lockT)
       {
         case SasUserLock__READ:
            firstFreeLock->read_lock(lock, keyAddr);
            return;
         case SasUserLock__WRITE:
            firstFreeLock->write_lock(lock, keyAddr);
            return;
       }
    }

   // No free lock to use--so new-up one...
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s - new-up a lock\n", __FUNCTION__);
#endif

   char *byteAddr = (char*) SASNearAlloc( (void*) this, sizeof(SasUserLock) );
   SasUserLock * newUserLock = new(byteAddr) SasUserLock();
   add(newUserLock);
   switch (lockT) {
      case SasUserLock__READ:
         newUserLock->read_lock(lock, keyAddr);	 // lock item
         break;
      case SasUserLock__WRITE:
         newUserLock->write_lock(lock, keyAddr); // lock item
         break;
    }

   return;
}

template<class Item, class Key>
void
SasLockList<Item, Key>::unlockNode(Key keyAddr)
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   SasLockListNode<Item, Key> * iterPtr;
   lock->write_lock();

   // Search the list for the specified key
   for (iterPtr = headPtr; iterPtr != NULL; iterPtr = iterPtr->next)
    {
      if (iterPtr->item->operator==(keyAddr))
         break;
    }
   if (iterPtr == NULL)
    {
#ifdef __SOMDebugPrint__
      fprintf (stderr, "%s: error - unlock requested for an address that "
               "is not locked\n", __FUNCTION__);
#endif
      return;
    }
   iterPtr->item->unlock();	// unlock item
   lock->unlock();		// unlock list
}

#ifdef collectstats
// Print stats on lock objects in this list
template<class Item, class Key>
void
SasLockList<Item, Key>::printStats(int slotNum)
{
   SasLockListNode<Item, Key> * iteratorPtr;
   lock->write_lock();
   if (count)
    {
      cout << '\t' << '\t' << "SLOT #" << slotNum << endl;
      cout << "Number of lock objects created in this slot: "
        << count << endl;
      int k = 1;
      cout << '\t' << "LOCK #" << '\t' << "USAGE" << endl;
      // Run the list to get usage info on each lock; also check if lock is in
      // a locked state.
      for (iteratorPtr = headPtr;
           iteratorPtr != NULL;
           iteratorPtr = iteratorPtr->next)
       {
         cout << '\t'<< k << '\t' << iteratorPtr->item->getUseageCount()
           << endl;
         if (!(iteratorPtr->item->operator==(NullAddress)))
          {
            cout << "++++++++++++++ ERROR +++++++++++++++ " << endl;
            cout << "Lock @ "
              << iteratorPtr->item->getAddrKey()
              << " PID "
              << iteratorPtr->item->getWriterPID()
              << " TID "
              << iteratorPtr->item->getWriterTID()
              << " is still locked!! " << endl;
          }
         ++k;
       }
      cout << "_________________________________________________ " << endl;
    }
   lock->unlock();		// unlock list
}
#endif


// Acquire a read lock on the list
template<class Item, class Key>
void
SasLockList<Item, Key>::read_lock(void) const
{
   lock->read_lock();
}

// Acquire a write lock on the list
template<class Item, class Key>
void
SasLockList<Item, Key>::write_lock(void) const
{
   lock->write_lock();
}

//Unlock the list
template<class Item, class Key>
void
SasLockList<Item, Key>::unlock(void) const
{
   lock->unlock();
}


// The next two methods (getHighestUseage and getCount) are used when
// getting/printing stats and there is no locking requirement at such times.
#ifdef collectstats
template<class Item, class Key>
unsigned
SasLockList<Item, Key>::getHighestUseage(void)
{
   SasLockListNode<Item, Key> * iteratorPtr;
   unsigned runningHighestUseage = 0;
   unsigned curUseage = 0;
   if (count)
    {
      for (iteratorPtr = headPtr;
           iteratorPtr != NULL;
           iteratorPtr = iteratorPtr->next)
       {
         curUseage = iteratorPtr->item->getUseageCount();
         if ( curUseage > runningHighestUseage )
          {
            runningHighestUseage = curUseage;
          }
       }
      return runningHighestUseage;
    }
   return 0;
}
#endif

// Return the number of items in the list
template<class Item, class Key>
size_t
SasLockList<Item, Key>::getCount(void) const
{
   int temp;
   temp = count;
   return temp;
}



/////////////////////////////////////////////////////////////////
// Private methods

// Add an item to the list NOTE:  No locking is done in this private method.
// Caller must lock and unlock the list.
template<class Item, class Key>
void
SasLockList<Item, Key>::add(Item *itemPtr)
{
#ifdef __SOMDebugPrint__
   fprintf (stderr, "%s\n", __FUNCTION__);
#endif

   // Create new list node in the same SAS block that "this"
   // object is in.

   SasLockListNode<Item, Key> *nodePtr;
   nodePtr = new( this ) SasLockListNode<Item, Key>(itemPtr, headPtr);

   // Update the list pointers and count
   headPtr = nodePtr;
   ++count;
}

#endif // _SasLockList_C

