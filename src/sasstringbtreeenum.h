/*
 * Copyright (c) 2005-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef __SAS_STRINGBTREEENUM_H
#define __SAS_STRINGBTREEENUM_H

/*! \file sasstringbtreeenum.h
*  \brief An enumeration over a Shared Address Space, C String BTree index
*	for shared memory multi-thread/multi-core applications.
*
*	Create enumerations that
*   manage iterations over the keys and associated values of sasstringbtree.h.
*   Iteration is in key order from minimum to maximum contained keys.
*
*   \code
  SASStringBTreeEnum_t senum;

  senum = SASStringBTreeEnumCreate (stringBTree);
  if (senum)
  {
    while (SASStringBTreeEnumHasMore (senum))
      {
        void *addr_val;
        char *key;

        addr_val =  SASStringBTreeEnumNext (senum);
        if (addr_val)
	    {
	      key = SASStringBTreeEnumCurrent (senum)
	      printf ("Key<%s> Value<%p>\n", key, addr_value);
	    }
      }
    SASStringBTreeEnumDestroy (senum);
  }
*   \endcode
*
*   There is an option (via SASStringBTreeEnumCreateStartAt) to create a
*   enumeration starting at a key value any where between the minimum
*   and maximum keys.
*   \code
  SASStringBTreeEnum_t senum;

  senum = SASStringBTreeEnumCreateStartAt (stringBTree, "/mykeys/");
  if (senum)
  {
    while (SASStringBTreeEnumHasMore (senum))
      {
        void *addr_val;
        char *key;

        addr_val =  SASStringBTreeEnumNext (senum);
        if (addr_val)
	    {
	      key = SASStringBTreeEnumCurrent (senum)
	      printf ("Key<%s> Value<%p>\n", key, addr_value);
	    }
      }
    SASStringBTreeEnumDestroy (senum);
  }
*   \endcode
*
*   It is simple to iterate over a subrange of the BTree by checking
*   for a \a stop_key and exiting the SASStringBTreeEnumNext search
*   when the stop value is met or exceeded.
*   \code
  SASStringBTreeEnum_t senum;
  myprefix = "/mykeys/";
  mystop   = "/mykeys0"; // ASCII '0' is the char after '/'

  senum = SASStringBTreeEnumCreateStartAt (stringBTree, myprefix);
  if (senum)
  {
    while (SASStringBTreeEnumHasMore (senum))
      {
        void *addr_val;
        char *key;

        addr_val =  SASStringBTreeEnumNext (senum);
        if (addr_val)
	    {
	      key = SASStringBTreeEnumCurrent (senum)
	      // check for stop value
	      if (strcmp(key, mystop) >= 0)
	        break;
	      printf ("Key<%s> Value<%p>\n", key, addr_value);
	    }
      }
    SASStringBTreeEnumDestroy (senum);
  }
*   \endcode
*
*   \todo Create SASStringBTreeEnumCreateBetween SASStringBTreeEnumCreateStopBefore APIs
**/

#include "sastype.h"
#include "sasstringbtree.h"

/** \brief Handle to an instance of String BTree Enumeration.
 *
 *  Enumerations are allocated from dynamic (malloc) storage and can
 *  not be shared with other processes. This allows multiple threads,
 *  each with their own enumeration, to safely iterate over a
 *  shared String BTree index.
*/
typedef void *SASStringBTreeEnum_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Create a SASStringBTreeEnum_t enumeration that can be used to
*   iterate over the name space of a StringBtree or name Context.
*
*   Create String BTree enumeration for the "C" string name index of
*   specific BTree.
*   The program can then use the SASStringBTreeEnum API to iterate over this
*   name space and retrieve the associated address values.
*
*	@param btree String BTree to create the name enumeration for.
*	@return SASStringBTreeEnum_t enumeration pointer,
*	NULL is returned for failure cases.
*/
extern __C__ SASStringBTreeEnum_t
SASStringBTreeEnumCreate (SASStringBTree_t btree);

/** \brief Create a SASStringBTreeEnum_t enumeration that can be used to
*   iterate over the name space of a StringBtree or name Context,
*   starting at a specific key value.
*
*   Create String BTree enumeration for the "C" string name index of
*   specific BTree.
*   The program can then use the SASStringBTreeEnum API to iterate over this
*   name space and
*
*	@param btree String BTree to create the enumeration over
*	@param start_key Starting key for iteration.
*	@return SASStringBTreeEnum_t enumeration pointer,
*	NULL is returned for failure cases (including start_key > max_key).
*/
extern __C__ SASStringBTreeEnum_t
SASStringBTreeEnumCreateStartAt (SASStringBTree_t btree, char *start_key);

/** \brief Destroy an instance of SASStringBTreeEnum_t enumeration.
*
*	@param sbtenum String BTree enumeration to be destroyed.
*/
extern __C__ void SASStringBTreeEnumDestroy (SASStringBTreeEnum_t sbtenum);

/** \brief Return status of a SASStringBTreeEnum_t enumeration.
*
*	@param sbtenum String BTree enumeration.
*	@return True if this enumeration has more entries.
*/
extern __C__ int SASStringBTreeEnumHasMore (SASStringBTreeEnum_t sbtenum);

/** \brief Return number of entries a SASStringBTreeEnum_t enumeration
*   contains.
*
*   Report the number of entries associated with this enumeration.
*   The count is derived from the underlying String BTree index when
*   the enumeration was created and may not be accurate due to changes
*   in the underly SASStringBTreeEnum_t. The count is decremented for
*   each successful call to SASStringBTreeEnumNext, but may be reset
*   if the underlying BTree has changed.
*
*   \note this value will not be accurate for \a StartAt enumeration
*   until we implement a "count entries left from current" operation
*   on the base SASStringBTree_t.
*
*	@param sbtenum String BTree enumeration.
*	@return number of the entries remaining in this enumeration.
*/
extern __C__ long SASStringBTreeEnumCount (SASStringBTreeEnum_t sbtenum);

/** \brief Return the C string pointer for the current enumeration key value.
*
*   \note This may be a direct pointer into the internal storage of the
*   SASStringBTree_t and should not be modified by the application.
*
*	@param sbtenum String BTree enumeration.
*	@return Pointer to the C string value of the current key position.
*/
extern __C__ char *SASStringBTreeEnumCurrent (SASStringBTreeEnum_t sbtenum);

/** \brief Move the enumeration to the next String BTree key entry and
*   return the associated address value.
*
*   The corresponding C string key value can be obtained via
*   SASStringBTreeEnumCurrent()
*
*	@param sbtenum String BTree enumeration.
*	@return the address value associated for the next String BTree enumeration.
*/
extern __C__ void *SASStringBTreeEnumNext (SASStringBTreeEnum_t sbtenum);

/** \brief Move the enumeration to the next String BTree key entry and
*   return the associated address value.
*
*   The corresponding C string key value can be obtained via
*   SASStringBTreeEnumCurrent()
*
* This nolock form should only be used when the referenced SASStringBTreeEnum_t
* is known to be locked by the application or contained within a
* larger structure with a controlling lock.
*
*	@param sbtenum String BTree enumeration.
*	@return the address value associated for the next String BTree enumeration.
*/
extern __C__ void *SASStringBTreeEnumNext_nolock (SASStringBTreeEnum_t sbtenum);

#endif /* __SAS_STRINGBTREEENUM_H */
