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

#ifndef __SAS_INDEXENUM_H
#define __SAS_INDEXENUM_H

#include "sastype.h"
#include "sasindex.h"

/*!
 * \file  sasindexenum.h
 * \brief Enumeration API for iteration over Shared Address Space
 * binary B-tree index defined in sasindex.h.
 * For use by shared memory multi-thread/multi-core applications.
 *
 *
 * Create enumerations that manage iterations over the keys and
 * associated values of sasindex.h.
 * Iteration is in key order from minimum to maximum contained keys.
 *
 * \code
  SASIndexEnum_t ndxenum;

  senum = SASIndexEnumCreate (index);
  if (ndxenum)
  {
    while (SASIndexEnumHasMore (ndxenum))
      {
        void *addr_val;
        char *key;

        addr_val =  SASStringBTreeEnumNext (ndxenum);
        if (addr_val)
	    {
	      printf ("Indexed Value<%p>\n", addr_value);
	    }
      }
    SASIndexEnumDestroy (ndxenum);
  }
*   \endcode
*
 */

/** \brief Handle to an instance of binary BTree Index Enumeration.
 *
 *  Enumerations are allocated from dynamic (malloc) storage and can
 *  not be shared with other processes. This allows multiple threads,
 *  each with their own enumeration, to safely iterate over a
 *  shared binary BTree index.
*/
typedef void	*SASIndexEnum_t;

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Create a SASIndexEnum_t enumeration that can be used to
*   iterate over the key space of a SASIndex_t.
*
*   Create binary BTree enumeration for the
*   specific binary BTree Index.
*   The program can then use the SASIndexEnum API to iterate over this
*   key space and retrieve the associated address values.
*
*	@param btree SASIndex_t to create the name enumeration for.
*	@return SASIndexEnum_t enumeration pointer,
*	NULL is returned for failure cases.
*/
extern __C__ SASIndexEnum_t
SASIndexEnumCreate (SASIndex_t	btree);

/** \brief Destroy an instance of SASIndexEnum_t enumeration.
*
*	@param indexenum binary BTree index enumeration to be destroyed.
*/
extern __C__ void
SASIndexEnumDestroy (SASIndexEnum_t	indexenum);

/** \brief Return status of a SASIndexEnum_t enumeration.
*
*	@param indexenum binary BTree Index enumeration.
*	@return True if this enumeration has more entries.
*/
extern __C__ int
SASIndexEnumHasMore (SASIndexEnum_t	indexenum);

/** \brief Move the enumeration to the next binary BTree Index key entry
*   and return the associated address value.
*
*	@param indexenum binary BTree enumeration.
*	@return the address value associated for the next String BTree enumeration.
*/
extern __C__ void*
SASIndexEnumNext(SASIndexEnum_t	indexenum);

/** \brief Move the enumeration to the next binary BTree Index key entry
*   and return the associated address value.
*
*   This nolock form should only be used when the referenced SASIndex_t
*   is known to be locked by the application or contained within a
*   larger structure with a controlling lock.
*
*	@param indexenum binary BTree enumeration.
*	@return the address value associated for the next String BTree enumeration.
*/
extern __C__ void*
SASIndexEnumNext_nolock(SASIndexEnum_t	indexenum);

#endif /* __SAS_INDEXENUM_H */
