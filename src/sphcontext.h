#ifndef __SPH_CONTEXT_H
#define __SPH_CONTEXT_H

/*
 * Copyright (c) 2006, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

/*!
 * \file  sphcontext.h
 * \brief Shared Persistent Heap, name/address context for
 *        shared memory multi-thread/multi-core applications.
 *
 * A Shared Persistent Heap name/address context manages reversible
 * associations between name strings and address values.  Either (name
 * or address) can be used to retrieve the other at a later time.  In the
 * current implementation name-to-address associations are one-to-one within
 * a specific context.
 *
 * Nothing prevents different context instances from having the same names or
 * address values. And also different contexts can have different names
 * associated with the same address value.
 *
 * \todo In a future implementation the intent is to allow multiple named
 * associations to a common address value within the same context.  However the
 * restriction that a specific name can only be associated with one address
 * value will remain.
 *
 * Contexts may be nested as deeply as desired but there isn't an API for
 * finding them.  They have to be searched for level by level using 
 * SPHContextFindByName or SPHContextFindByAddr and using the result as the input
 * context for the next level.
 */

#include "sastype.h"
#include "sasstringbtreeenum.h"

/** \brief Handle to an instance of SPH Context.
*
*	The type is SAS_RUNTIME_CONTEXT
*/
typedef void *SPHContext_t; 

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

/** \brief Initialize a shared storage block as a name/address
*   mapping context.
*
*	Initialize the control blocks within the specified storage
*	block as a Context.
*	The storage block must be power of two in size and have the
*	same power of two (or better) alignment.
*	The type should be SAS_RUNTIME_CONTEXT.
*
*	@param heap_block a block of allocated SAS storage matching the heap_size.
*	@param sasType a SAS type code for internal consistency checking.
*	@param heap_size power of two size of the heap to be initialized.
*	@return at handle to the initialized SPHContext_t
*/
extern __C__ SPHContext_t 
SPHContextInit (void *heap_block , sas_type_t sasType, 
		block_size_t heap_size);

/** \brief Create a name/address mapping context..
*
*	Allocate a SAS block and initialize the control blocks as a
*	Context.
*	The block size must be power of two in size.
*
*	@param heap_size power of 2 size of the context to be allocated and initialized.
*	@return a handle of the newly allocated SPHContext_t. Or NULL for the failure case.
*/
extern __C__ SPHContext_t 
SPHContextCreate (block_size_t heap_size);

/** \brief Destroy a name/address mapping context and free the shared storage block.
*
*	The sas_type_t must be of type SAS_RUNTIME_CONTEXT.
*
*	@param contxt context to be destroyed and the storage freed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int 
SPHContextDestroy (SPHContext_t contxt);

/** \brief Add a name/address mapping to this context.
*
*   A reversible association is created between the name string and the address value.
*   Either (name or address) can be use to retrieve the other at a later time.
*   Within a context an address value can have multiple named associations,
*   but an name can only be associated with one address value.
*
*   \todo The intent is to support multiple name associations to a single
*   value, but this is not currently implemented. But the application can
*   always use multiple contexts to associate multiple names to an address
*   value.
*
*	@param contxt context to be destroyed and the storage freed.
*	@param name C string for the "name" to be associated with the address "value"
*	@param value address "value" to be associated with the "name" string
*	@return true (!=0) value indicates success, false (==0) failure.
*/
extern __C__ int
SPHContextAddName (SPHContext_t contxt, char *name, void *value);

/** \brief Replace the existing "old name" with a "new name" string for
*   the associated address value in this context.
*
*   A reversible association is created between the newname string and the address value.
*   Either (name or address) can be use to retrieve the other at a later time.
*   Within a context an address value can have multiple named associations,
*   but an name can only be associated with one address value.
*
*	@param contxt context to be destroyed and the storage freed.
*	@param oldname C string for an existing "named" association.
*	@param newname replacement C string for an existing "named" association.
*	@param value address "value" to be associated with the "newname" string.
*	@return true (!=0) value indicates success, false (==0) failure.
*/
extern __C__ int
SPHContextRename (SPHContext_t contxt, char *oldname, char *newname, void *value);

/** \brief Create a SASStringBTreeEnum_t enumeration that can used to
*   iterate over the name space of this context.
*
*   Create String BTree enumeration for the internal name index of this context.
*   The program can then use the SASStringBTreeEnum API to iterate over this
*   contexts name space.
*   For example; to list all the named associations.
*
*	@param contxt context to create the name enumeration for.
*	@return SASStringBTreeEnum_t enumeration pointer,
*	NULL is returned for failure cases.
*/
extern __C__ SASStringBTreeEnum_t
SPHContextGetNameEnum (SPHContext_t contxt);

/** \brief Find the address value associated with a name within a specific context.
*
*   The context name index is searched for the specified name, and if found,
*   the associated address value is returned.
*
*	@param contxt context to be searched for "name".
*	@param name C string for the "named" association we are looking for.
*	@return address value for the matching association, or NULL is not found.
*/
extern __C__ void*
SPHContextFindByName (SPHContext_t contxt, char *name);

/** \brief Find the name associated with an address value within a specific context.
*
*   The context value index is searched for the specified value, and if found,
*   the associated name C string is returned.
*
*	@param contxt context to be searched for "name".
*	@param value address value for the association we are looking for.
*	@return address value for the matching associated name, or NULL is not found.
*/
extern __C__ void*
SPHContextFindByAddr (SPHContext_t contxt, void *value);

/** \brief Remove a name/address association by matching name for the specified context.
*
*   The context name index is searched for the specified name, and if found,
*   the association is removed.
*   Any other named associations to that address value are not changed.
*
*	@param contxt context to remove the named association from.
*	@param name C string for the "named" association we are looking to remove.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int
SPHContextRemoveByName (SPHContext_t contxt, char *name);

/** \brief Remove a name/address associations by matching address value for the specified context.
*
*   The context value index is searched for the specified address, and if found,
*   all associations are removed.
*
*	@param contxt context to remove the associations from.
*	@param value for the associations we are looking to remove.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int
SPHContextRemoveByAddr (SPHContext_t contxt, void *value);

/** \brief Return the remaining free space within the
*	specified context.
*
*	internal use only
*
*	@param contxt to checked for size.
*	@return the remaining free space for the internal heap in bytes.
*/
extern __C__ block_size_t 
SPHContextFreeSpace (SPHContext_t contxt);

/** \brief Setup the root and named project contexts. A project context
*   is just a second level context named in the regions root context.
*
*	The concept of projects allows multiple applications to share a region
*	while separating application specific data into projects for ease of
*	management.
*
*	The root and named project contexts are created if they don't exist.
*	If the root context already exists but the named project context does
*	not, a new context is created and added to the root context using the
*	project name. If both already exist simply set the current project and
*	return that value as the result. This setup persists for the life of
*	the region and new project contexts can be added at any time after.
*
*	The root context is 64K.   A project context is 1M.
*
*	\warning SPHSetupProjectContext should not be used before SASJoinRegion()
*	or after SASCleanUp()/SASRemove()
*
*	@param project_name name of the project context this application will use.
*	@return the address of the named project context.
*	A NULL value indicates error.
*/
extern __C__ SPHContext_t
SPHSetupProjectContext(char *project_name);

/** \brief Setup root and named project contexts. A project context
*   is just a second level context named in the regions root context.
*
*	Same function as SPHSetupProjectContext except the current project
*	context is not set/changed.
*
*	@param project_name name of the project context this application will use.
*	@return the address of the named project context.
*	A NULL value indicates error.
*/
extern __C__ SPHContext_t
SPHSetupAltProjectContext(char *project_name);

/** \brief Remove an existing project context from the root and return
*   the named project context. The project context is not destroyed.
*
*   If the matching project context is also the current project context,
*   the current project is reset to NULL.
*
*	@param project_name name of the project context to be removed.
*	@return the address of the named project context.
*	A NULL value indicates error.
*/
extern __C__ SPHContext_t
SPHRemoveProjectContext(char *project_name);

/** \brief Remove an existing project context from the root and destroy
*   that project context.
*
*   If the matching project context is also the current project context,
*   then current project is reset to NULL.
*
*	@param project_name name of the project context to be removed.
*	@return a 0 value indicates success, otherwise failure.
*/
extern __C__ int
SPHDestroyProjectContext(char *project_name);

/** \brief return the address of the named project context.
*
*   \note Assumes that SPHSetupProjectContext or SPHSetupAltProjectContext
*   have been called earlier for this region with this project name.
*
*	@param project_name name of the project context to be looked up in the
*	root context.
*	@return the address of the named project context, or NULL if not found.
*/
extern __C__ SPHContext_t
getProjectContextByName(char *project_name);

/** \brief return the address of the current project context.
*
*	@return the address of the current project context, or NULL if there is no current project context.
*/
extern __C__ SPHContext_t
getCurrentProjectContext();

#endif /* __SPH_CONTEXT_H */
