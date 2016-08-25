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

#define sas_printf printf
#include <stdio.h>
#include <stdlib.h>
#include "sasallocpriv.h"
#include "freenode.h"
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#include "sassim.h"
#include "saslock.h"
#include "sphcontext.h"
#include "sphcontextpriv.h"

#include "sassimpleheap.h"
#include "sasstringbtree.h"
#include "sasstringbtreeenum.h"
#include "sasindex.h"
#include "sasindexenum.h"
#include "sasindexkey.h"

struct SPHContextHeader;

typedef struct SPHContextExpandList {
		block_size_t	count;
		block_size_t	max_count;
		SPHContextHeader	*heap[254];
		} SPHContextExpandList;

typedef struct SPHContextHeader {
		SASBlockHeader blockHeader;
		block_size_t	pageSize;
		SASIndex_t		objID;
		SASStringBTree_t	name;
		void			*dummy3;
		void			*dummy4;
		void			*dummy5;
		SPHContextExpandList	*expandList;
		freeNode		*headerFreeSpace;
		} SPHContextHeader;


#ifdef __WORDSIZE_64
# ifdef __s390x__
# define DEFAULT_BLOCK (1024*1024)
# else
# define DEFAULT_BLOCK (4096*4096)
# endif
#else
#define DEFAULT_BLOCK (1024*1024)
#endif

static SPHContext_t currentContext = NULL;

SPHContext_t 
SPHContextInit (void* heap_seg,  sas_type_t sasType,
		block_size_t heap_size)
{
    SASBlockHeader	*heapBlock = (SASBlockHeader*)heap_seg;
    char		*heapStart = NULL;
    
    if ( heapBlock )
    {
		heapStart = (char*) heapBlock + heap_offset;
		initSOMSASBlock(heapBlock, sasType, 
		                           heap_size, heapStart);
    }

    return (SPHContext_t)heapBlock;
}

SPHContext_t
SPHContextCreate (block_size_t heap_size)
{
    SASBlockHeader	*heapBlock = NULL;
    SPHContext_t	result;
    
    heapBlock = (SASBlockHeader*)SASBlockAlloc ((long)heap_size);
    if ( heapBlock )
    {
	    result = SPHContextInit(heapBlock, 
	                            SAS_RUNTIME_CONTEXT, 
		                        heap_size);
		if (result != NULL)
		{
			SPHContextHeader	*header = (SPHContextHeader*) result;
			header->name  = SASStringBTreeCreate (DEFAULT_BLOCK);
#ifdef __SASDebugPrint__
			if (header->name)
	    			sas_printf("SPHContextAlloc BTree=%p\n", header->name);
			else
	    			sas_printf("SPHContextAlloc() BTree create failed\n");
#endif
			header->objID = SASIndexCreate (DEFAULT_BLOCK);
#ifdef __SASDebugPrint__
			if (header->objID)
	    			sas_printf("SPHContextAlloc Index=%p\n", header->objID);
			else
	    			sas_printf("SPHContextAlloc() Index create failed\n");
#endif
		}
		return result;
    } else 
	    return NULL;
}

void *
SPHContextAllocNoLock (SPHContext_t heap,
				block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    freeNode		*mem = NULL;

    if (SOMSASCheckBlockSigAndType (headerBlock,
              SAS_RUNTIME_CONTEXT) )
    {
		heapSize = headerBlock->blockSize - heap_offset;
		if ( alloc_size < heapSize )
		{
		    mem = freeNode_allocSpace(headerBlock->blockFreeSpace,
		                    &headerBlock->blockFreeSpace, alloc_size);
#ifdef __SASDebugPrint__
		} else {
	    	sas_printf("SPHContextAlloc(%p, %zu) range check failed\n",
	    	                 heap, alloc_size);
#endif
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextAlloc(%p, %zu) type check failed\n",
    	          heap, alloc_size);
#endif
    }
    return mem;
}

int
SPHContextFreeNoLock (SPHContext_t heap,
			void* free_block,
			block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    freeNode		*free_node = (freeNode*)free_block;
    int rc;

    freeNode_init(free_node, alloc_size);

    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
		heapSize = headerBlock->blockSize - heap_offset;
		if (( alloc_size < heapSize )
		&& ( (unsigned long)free_block >= ((unsigned long)headerBlock + heap_offset) ) )
		{
		    freeNode_deallocSpace(free_node, &headerBlock->blockFreeSpace, alloc_size);
		    rc = 0;
		} else {
			rc = -2;
#ifdef __SASDebugPrint__
        	sas_printf("SPHContextFree(%p, %p, %zu) range check failed\n",
        				heap, free_block, alloc_size);
#endif
		}
    } else {
		rc = -1;
#ifdef __SASDebugPrint__
        sas_printf("SPHContextFree(%p, ...) does not match type/subtype\n",
        				heap);
#endif
    }
    return rc;
}

int
SPHContextAddNameNoLock (SPHContext_t contxt, char *key, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    int			rc = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	char			*buff;
    	block_size_t	alloc_size = strlen(key) + 1;
    	
    	buff = (char*) SPHContextAllocNoLock(contxt, alloc_size);
    	if (buff)
    	{
			SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
			SASIndexKey_t		keyID;
    		buff = strcpy(buff, key);
    		
    		rc = SASStringBTreePut(header->name, key, value);
    		if (rc)
    		{
    			SASIndexKeyInitRef (&keyID, value);
    			rc = SASIndexPut (header->objID, &keyID, buff);
    			if (!rc)
    			{
#ifdef __SASDebugPrint__
    	    	    sas_printf("SPHContextAddNameNoLock(%p, %s, %p) Duplicate ObjID\n",
    	    	                 contxt, key, value);
#endif
					SASStringBTreeRemove (header->name, key);
	        	    SPHContextFreeNoLock(contxt, buff, alloc_size);
    			}
    		} else {
#ifdef __SASDebugPrint__
	    	    sas_printf("SPHContextAddNameNoLock(%p, %s, %p) Duplicate Name\n",
	    	                 contxt, key, value);
#endif
	        	SPHContextFreeNoLock(contxt, buff, alloc_size);
    		}
#ifdef __SASDebugPrint__
		} else {
	    	sas_printf("SPHContextAddNameNoLock(%p, %s, %p) alloc failed\n", 
	    	                 contxt, key, value);
#endif
		}
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextAddNameNoLock(%p, %s, %p) type check failed\n",
    	          contxt, key, value);
#endif
    }
    return rc;
}

int
SPHContextAddName (SPHContext_t contxt, char *key, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    int				result = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__WRITE);
    	result = SPHContextAddNameNoLock(contxt, key, value); 
		SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextAddName(%p, %s, %p) type check failed\n",
    	          contxt, key, value);
#endif
    }
    return result;
}

void*
SPHContextFindByNameNoLock (SPHContext_t contxt, char *key)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*result = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
		SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
		
		result = SASStringBTreeGet(header->name, key);
#ifdef __SASDebugPrint__
		if (!result)
    	sas_printf("SPHContextFindByNameNoLock(%p, %s) not found\n", 
    	                 contxt, key);
#endif
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextFindByNameNoLock(%p, %s) type check failed\n",
    	          contxt, key);
#endif
    }
    return result;
}

void*
SPHContextFindByAddrNoLock (SPHContext_t contxt, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*result = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
		SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
		SASIndexKey_t		keyID;
		
		SASIndexKeyInitRef (&keyID, value);
		result = SASIndexGet (header->objID, &keyID);
#ifdef __SASDebugPrint__
		if (result)
    	sas_printf("SPHContextFindByAddrNoLock(%p, %p) not found\n", 
    	                 contxt, value);
#endif
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextFindByAddrNoLock(%p, %p) type check failed\n",
    	          contxt, value);
#endif
    }
    return result;
}

SASStringBTreeEnum_t
SPHContextGetNameEnum (SPHContext_t contxt)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    SASStringBTreeEnum_t	result = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
	SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
			
    	SASLock(contxt, SasUserLock__READ);
	result = SASStringBTreeEnumCreate( header->name );
	SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextGetNameEnum(%p) type check failed\n",
    	          contxt);
#endif
    }
    return result;
}

void*
SPHContextFindByName (SPHContext_t contxt, char *key)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*result = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__READ);
    	result = SPHContextFindByNameNoLock(contxt, key); 
	SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextFindByName(%p, %s) type check failed\n",
    	          contxt, key);
#endif
    }
    return result;
}

void*
SPHContextFindByAddr (SPHContext_t contxt, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*result = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__READ);
    	result = SPHContextFindByAddrNoLock(contxt, value); 
		SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextFindByAddr(%p, %p) type check failed\n",
    	          contxt, value);
#endif
    }
    return result;
}

int
SPHContextRemoveByNameNoLock (SPHContext_t contxt, char *key)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*value = NULL;
    int				result = -1;
    int				rc = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
		SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
		SASIndexKey_t		keyID;
		char				*strbuff = NULL;
		
		value = SASStringBTreeGet(header->name, key);
		if (value)
		{		
			SASIndexKeyInitRef (&keyID, value);
			rc++;
			strbuff = (char*)SASIndexGet (header->objID, &keyID);
			if (strbuff)
			{
				int		strsize = strlen(strbuff) + 1;
				
				rc = SPHContextFreeNoLock (contxt, strbuff, strsize);
				if (rc)
				{
					result = -1;
#ifdef __SASDebugPrint__
		    		sas_printf("SPHContextRemoveByNameNoLock(%p, %s) free@%p failed\n", 
		    	                 contxt, key, strbuff);
#endif
				} else {
					result = 0;
					SASIndexRemove (header->objID, &keyID);
					SASStringBTreeRemove (header->name, key);
				}
			}
		} else {
			result = -3;
#ifdef __SASDebugPrint__
    		sas_printf("SPHContextRemoveByNameNoLock(%p, %s) not found\n", 
    	                 contxt, key);
#endif
		}
    } else {
		result = -4;
#ifdef __SASDebugPrint__
    	sas_printf("SPHContextRemoveByNameNoLock(%p, %s) type check failed\n",
    	          contxt, key);
#endif
    }
    return result;
}

int
SPHContextRename (SPHContext_t contxt, char *oldkey, char *newkey, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    void			*oldval;
    int				result = 0;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__WRITE);
    	oldval = SPHContextFindByNameNoLock(contxt, oldkey);
    	if (oldval && (oldval == value))
    	{
		    SPHContextRemoveByNameNoLock(contxt,oldkey);
    	    result = SPHContextAddNameNoLock(contxt, newkey, value);
    	}
		SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextRename(%p, %s, %s, %p) type check failed\n",
    	          contxt, oldkey, newkey, value);
#endif
    }
    return result;
}


int
SPHContextRemoveByAddrNoLock (SPHContext_t contxt, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    int				result = 0;
    int				rc;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
		SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
		SASIndexKey_t		keyID;
		char				*strbuff;
		
		SASIndexKeyInitRef (&keyID, value);
		strbuff = (char*)SASIndexGet (header->objID, &keyID);
		if (strbuff)
		{
			void	*val2;			
			int		strsize = strlen(strbuff);
	
			val2 = (char*)SASStringBTreeGet(header->name, strbuff);
			if (value == val2)
			{
				SASIndexRemove (header->objID, &keyID);
				SASStringBTreeRemove (header->name, strbuff);
				rc = SPHContextFreeNoLock (contxt, strbuff, strsize);
				if (rc)
				{
					result = -1;
#ifdef __SASDebugPrint__
		    		sas_printf("SPHContextRemoveByAddrNoLock(%p, %p) free@%p failed\n", 
		    	                 contxt, value, strbuff);
#endif
				} else {
					result = 0;
				}
			} else {
				result = -2;
#ifdef __SASDebugPrint__
	    		sas_printf("SPHContextRemoveByAddrNoLock(%p, %p) %p!=%p\n", 
	    	                 contxt, value, value, val2);
#endif
			}
		} else {
			result = -3;
#ifdef __SASDebugPrint__
    		sas_printf("SPHContextRemoveByAddrNoLock(%p, %p) not found\n", 
    	                 contxt, value);
#endif
		}
    } else {
		result = -4;
#ifdef __SASDebugPrint__
    	sas_printf("SPHContextRemoveByAddrNoLock(%p, %p) type check failed\n",
    	          contxt, value);
#endif
    }
    return result;
}

int
SPHContextRemoveByName (SPHContext_t contxt, char *key)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    int				result = -1;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__WRITE);
    	result = SPHContextRemoveByNameNoLock(contxt, key); 
		SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextRemoveByName(%p, %s) type check failed\n",
    	          contxt, key);
#endif
    }
    return result;
}

int
SPHContextRemoveByAddr (SPHContext_t contxt, void *value)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)contxt;
    int				result = -1;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(contxt, SasUserLock__WRITE);
    	result = SPHContextRemoveByAddrNoLock(contxt, value); 
		SASUnlock(contxt);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextRemoveByAddr(%p, %p) type check failed\n",
    	          contxt, value);
#endif
    }
    return result;
}

void *
SPHContextAlloc (SPHContext_t heap, block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    void		*mem = NULL;
    
    if (SOMSASCheckBlockSigAndType (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	mem = SPHContextAllocNoLock(heap, alloc_size); 
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
    	sas_printf("SPHContextAlloc(%p, %zu) type check failed\n",
    	          heap, alloc_size);
#endif
    }
    return mem;
}

int
SPHContextFree (SPHContext_t heap, 
			void* free_block, 
			block_size_t alloc_size)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    int rc;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	rc = SPHContextFreeNoLock(heap, free_block, alloc_size);
		SASUnlock(heap);
    } else {
			rc = -1;
#ifdef __SASDebugPrint__
        sas_printf("SPHContextFree(%p, ...) does not match type/subtype\n",
        				heap);
#endif
    }
    return rc;
}
		
void* SPHContextNearAlloc(void *nearObj, long allocSize)
{
    return SASNearAlloc (nearObj, allocSize);
}

void SPHContextNearDealloc(void *memAddr, long allocSize)
{							
    SASNearDealloc (memAddr, allocSize);
}

block_size_t
SPHContextFreeSpaceNoLock (SPHContext_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	if (headerBlock->blockFreeSpace != NULL)
    		heapFree = freeNode_freeSpaceTotal(headerBlock->blockFreeSpace);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SPHContextFreeSpace(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return heapFree;
}

block_size_t 
SPHContextFreeSpace (SPHContext_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapFree = 0;
    
    if ( SOMSASCheckBlockSigAndType (headerBlock, SAS_SIMPLEHEAP_TYPE) )
    {
    	SASLock(heap, SasUserLock__READ);
    	heapFree = SPHContextFreeSpaceNoLock(heap);
		SASUnlock(heap);
#ifdef __SASDebugPrint__
    } else {
        sas_printf("SPHContextFreeSpace(%p) does not match type/subtype\n",
        				heap);
#endif
    }
    return heapFree;
}

int
SPHContextDestroyNoLock (SPHContext_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    block_size_t	heapSize;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
		SPHContextHeader	*header = (SPHContextHeader*) headerBlock;
		if (header->name)
			SASStringBTreeDestroy(header->name);
		if (header->objID)
			SASIndexDestroy(header->objID);
			
		heapSize = headerBlock->blockSize;
		SASBlockDealloc (heap, heapSize);
		rc = 0;
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SPHContextDestroy(%p) does not match type/subtype\n",
        				heap);
#endif
		rc = -1;
    }
    return rc;
}

int
SPHContextDestroy (SPHContext_t heap)
{
    SASBlockHeader	*headerBlock = (SASBlockHeader*)heap;
    int rc;
    
    if ( SOMSASCheckBlockSigAndTypeAndSubtype (headerBlock, 
              SAS_RUNTIME_CONTEXT) )
    {
    	SASLock(heap, SasUserLock__WRITE);
    	rc = SPHContextDestroyNoLock(heap);
		SASUnlock(heap);
    } else {
#ifdef __SASDebugPrint__
        sas_printf("SPHContextDestroy(%p) does not match type/subtype\n",
        				heap);
#endif
		rc = -1;
    }
    return rc;
}

SPHContext_t
SPHSetupProjectContext(char *project_name)
{
	SPHContext_t result = NULL;
	SPHContext_t rootcntx;
	void *rootlock	= (void*)getfastMemLow();
	int	rc;

    /* Can't use SASSeize here as this could cause deadlock when we try
     * to allocate blocks for new project contexts. So we use SASLock
     * on memLow.
     * */
	SASLock(rootlock, SasUserLock__WRITE);
	rootcntx = (SPHContext_t)getSASFinder();
	if (!rootcntx)
	{
		rootcntx = SPHContextCreate(64 * 1024);
		setSASFinder(rootcntx);
	}

	result = SPHContextFindByName(rootcntx, project_name);
	if (result)
	{
		currentContext = result;
	} else {
		result = SPHContextCreate(1024 * 1024);
		if (result)
		{
			rc = SPHContextAddName(rootcntx, project_name, result);
			if (rc)
			{
				currentContext = result;
			} else {
				/* Root context may be full, so need to cleanup.  */
				SPHContextDestroy(result);
				currentContext = result = NULL;
			}
		} else {
			/* Context Create may be out of file or region space.  */
			currentContext = NULL;
		}
	}

	SASUnlock(rootlock);
	return result;
}

SPHContext_t
SPHSetupAltProjectContext(char *project_name)
{
	SPHContext_t result = NULL;
	SPHContext_t rootcntx;
	void *rootlock	= (void*)getfastMemLow();
	int	rc;

    /* Can't use SASSeize here as this could cause deadlock when we try
     * to allocate blocks for new project contexts. So we use SASLock
     * on memLow.
     * */
	SASLock(rootlock, SasUserLock__WRITE);
	rootcntx = (SPHContext_t)getSASFinder();
	if (!rootcntx)
	{
		rootcntx = SPHContextCreate(64 * 1024);
		setSASFinder(rootcntx);
	}

	result = SPHContextFindByName(rootcntx, project_name);
	if (result)
	{
	} else {
		result = SPHContextCreate(1024 * 1024);
		if (result)
		{
			rc = SPHContextAddName(rootcntx, project_name, result);
			if (!rc)
			{
				/* Root context may be full, so need to cleanup.  */
				SPHContextDestroy(result);
				result = NULL;
			}
		}
	}

	SASUnlock(rootlock);
	return result;
}

SPHContext_t
SPHRemoveProjectContext(char *project_name)
{
	SPHContext_t result = NULL;
	SPHContext_t rootcntx;
	void *rootlock	= (void*)getfastMemLow();
	int	rc;

    /* Can't use SASSeize here as this could cause deadlock when we try
     * to allocate blocks for new project contexts. So we use SASLock
     * on memLow.
     * */
	SASLock(rootlock, SasUserLock__WRITE);
	rootcntx = (SPHContext_t)getSASFinder();
	if (rootcntx)
	{
		result = SPHContextFindByName(rootcntx, project_name);
		if (result)
		{
			rc = SPHContextRemoveByName(rootcntx, project_name);
			if (!rc)
			{
				if (result == currentContext)
					currentContext = NULL;
			} else {
				/* remove failed */
				result = NULL;
			}
		}
	}
	SASUnlock(rootlock);
	return result;
}

int
SPHDestroyProjectContext(char *project_name)
{
	SPHContext_t projcntx;
	SPHContext_t rootcntx;
	void *rootlock	= (void*)getfastMemLow();
	int	rc, result;

    /* Can't use SASSeize here as this could cause deadlock when we try
     * to allocate blocks for new project contexts. So we use SASLock
     * on memLow.
     * */
	SASLock(rootlock, SasUserLock__WRITE);
	result = 1;
	rootcntx = (SPHContext_t)getSASFinder();
	if (rootcntx)
	{
		projcntx = SPHContextFindByName(rootcntx, project_name);
		if (projcntx)
		{
			result = rc = SPHContextRemoveByName(rootcntx, project_name);
			if (!rc)
			{
				result = SPHContextDestroy(projcntx);
				if (projcntx == currentContext)
					currentContext = NULL;
			}
		}
	}
	SASUnlock(rootlock);
	return result;
}

SPHContext_t
getProjectContextByName(char *project_name)
{
	SPHContext_t result = NULL;
	SPHContext_t rootcntx;

	rootcntx = (SPHContext_t)getSASFinder();
	if (rootcntx)
	{
		result = SPHContextFindByName(rootcntx, project_name);
	}

	return result;
}

SPHContext_t
getCurrentProjectContext()
{
	return currentContext;
}

