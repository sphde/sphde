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

#ifndef _SASLOCKLIST_H
#define _SASLOCKLIST_H

// Class Specification *************************************************
//
// Class name:   SasLockList and SasLockListNode
// Parent class: None
//
// Summary: SasLockList is a template for an unordered linear singly-
// linked list.
//
//   To instantiate a SasLockList of a particular type:
//     SasLockList<ItemType, KeyType> listName;
//
//   The ItemType class must provide:
//        operator == (KeyType)
//                 or (const KeyType&)
//
//
// Usage Examples:
//
//   To instantiate a SasLockList of a particular type:
//     SasLockList<ItemType, KeyType> listName;
//
// Usage Intention:
//
//	This class is intended to be used solely by SasMasterLock class.
//	Please see SasMasterLock.H for more details.
// End Class Specification *********************************************


//----------------------------------------------------------------------------
// Includes and forward references
//----------------------------------------------------------------------------

#ifndef _NO_INLINE_ASM
#define _NO_INLINE_ASM
#endif

class SasMasterLock;
template<class Item, class Key> class SasLockListNode;
template<class Item, class Key> class SasLockList;

#include "sasulock.h"
extern "C" {
/* #include <mach.h> */
#include <stddef.h>
/* #include <cthreads.h> */
}


//----------------------------------------------------------------------------
// Error codes
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// Class Description for list node
//----------------------------------------------------------------------------
template<class Item, class Key>
class SasLockListNode
{
public:
  // Allow the SasLockList class to access the private node members
  friend class SasLockList<Item, Key>;
private:
  SasLockListNode(Item *itemPtr,
	      SasLockListNode<Item, Key> *nextNodePtr);
  ~SasLockListNode(void);

  void * operator new( size_t size, SasLockList<Item,Key> * const lockList );
  void operator delete( void *deadObject );

  // Instance data for object
  Item                       *item;
  SasLockListNode<Item, Key> *next;
};

//----------------------------------------------------------------------------
// Class Description for list
//----------------------------------------------------------------------------
template<class Item, class Key>
class SasLockList
{
public:
  SasLockList(void);
  ~SasLockList(void);
  void * operator new(size_t size, SasMasterLock * const masterLock );
  void operator delete( void *deadObject );
  boolean_t isEmpty(void) const;
  void	    lockNode(Key keyAddr, sas_userlock_request_t lockT);
  void      unlockNode(Key keyAddr);
#ifdef collectstats
  void	    printStats(int slotNum);
  unsigned  getHighestUseage(void);
#endif
  void      read_lock(void) const;
  void      write_lock(void) const;
  void      unlock(void) const;
  size_t    getCount(void) const;

private:
  void      add(Item * itemPtr);
  SasLockListNode<Item, Key>	* headPtr;   // First node in list
  size_t			count;      // Current number of nodes
  SasUserLock			*lock;      // Locking mechanism
};


#endif /* _SasLockList_H */

