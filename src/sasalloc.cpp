/*
 * Copyright (c) 1994-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#include <stdlib.h>
#include "sasallocpriv.h"
#include "freenode.h"
#ifdef __SAS__
#include "sasstuff.h"
#endif
#ifdef __SASDebugPrint__
#include "sasio.h"
#endif
#include "sasanchr.h"
#ifdef __SASSIM__
#include "sassim.h"
#endif

#ifndef __SAS__
void  *SASAnchor;
#endif
/*
void setSASAnchor (void *val)
{
#ifdef __SASDebugPrint__
    sas_printf("setSASAnchor\n");
#endif
#ifdef __SAS__
    // get addressability for SAS anchor
    anchor *mainAnchor = (anchor*)anchor_addr;
    mainAnchor->myFinder = (Finder*)val;
#else
#ifdef __SASSIM__
    SASBlockHeader	*anchorBlock = (SASBlockHeader*)getfastMemLow();
    SASAnchor_t	*anchor = (SASAnchor_t*)anchorBlock->special;
    anchor->finder = val;
#else
    SASAnchor = val;
#endif
#endif
};

void *getSASAnchor(void)
{
#ifdef __SASDebugPrint__
    sas_printf("getSASAnchor\n");
#endif
#ifdef __SAS__
    // get addressability for SAS anchor
    anchor *mainAnchor = (anchor*)anchor_addr;
    return ( (void*)(mainAnchor->myFinder) );
#else
#ifdef __SASSIM__
    SASBlockHeader	*anchorBlock = (SASBlockHeader*)getfastMemLow();
    SASAnchor_t		*anchor = (SASAnchor_t*)anchorBlock->special;
    return (anchor->finder);
#else
    return SASAnchor;
#endif
#endif
}
*/
void setSASFinder (void *val)
{
#ifdef __SASDebugPrint__
	sas_printf("setSASAnchor\n");
#endif
#ifdef __SAS__
    // get addressability for SAS anchor
    anchor *mainAnchor = (anchor*)anchor_addr;
    mainAnchor->myFinder = (Finder*)val;
#else
#ifdef __SASSIM__
    SASBlockHeader	*anchorBlock = (SASBlockHeader*)getfastMemLow();
    SASAnchor_t		*anchor = (SASAnchor_t*)anchorBlock->special;
    anchor->finder = val;
#else
    SASAnchor = val;
#endif
#endif
}

void *getSASBlockSpecial(void *block)
{
    SASBlockHeader *blockPtr = (SASBlockHeader*)block;
    void *result = blockPtr->special;
#ifdef __SASDebugPrint__
	sas_printf("getSASBlockSpecial(%p) = %p\n",
			block, result);
#endif
    return result;
}

void setSASBlockSpecial (void *block, void *val)
{
    SASBlockHeader *blockPtr = (SASBlockHeader*)block;
    blockPtr->special = val;
#ifdef __SASDebugPrint__
	sas_printf("setSASBlockSpecial(%p, %p)\n",
			block, val);
#endif
    return;
}

void *getSASFinder()
{
#ifdef __SASDebugPrint__
	sas_printf("getSASAnchor\n");
#endif
#ifdef __SAS__
    // get addressability for SAS anchor
    anchor *mainAnchor = (anchor*)anchor_addr;
    return ( (void*)(mainAnchor->myFinder) );
#else
#ifdef __SASSIM__
    SASBlockHeader	*anchorBlock = (SASBlockHeader*)getfastMemLow();
    SASAnchor_t		*anchor = (SASAnchor_t*)anchorBlock->special;
    return (anchor->finder);
#else
    return SASAnchor;
#endif
#endif
}

void initSOMSASBlock(   SASBlockHeader *header, sas_type_t sasType,
			long blockSize, void *blockHeap)
{
#ifdef __SASDebugPrint__
    sas_printf("initSOMSASBlock (%p, %u, %li, %p)\n", 
                header, sasType, blockSize, blockHeap);
#endif
    node_size_t		remaining;

    header->special  = NULL;
    header->blockSig1= block__Signature_1;
    header->blockType= sasType;
    header->blockSig2= block__Signature_2;
    header->blockSize= blockSize;

	if (blockHeap == NULL)
	{
		header->blockFreeSpace = NULL;
	} else {
	    remaining= blockSize - (((char *)blockHeap) - ((char *)header));
	    header->blockFreeSpace = (freeNode *)blockHeap;
	    freeNode_init(header->blockFreeSpace, remaining);
	}

    header->baseBlock  = header;
    header->nextBlock   = NULL;
}

#ifdef __SAS__
void  SASMemRemove(void *startAddr, long size, int asyncBool)
#else
void  SASMemRemove(void *, long, int)
#endif
{
#ifdef __SASDebugPrint__
    sas_printf("SASMemRemove\n");
#endif
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myMemRemove)(startAddr, size, asyncBool);
#endif
};

#ifdef __SAS__
void  SASMemBring(void *startAddr, long size, int asyncBool)
#else
void  SASMemBring(void *, long, int)
#endif
{
#ifdef __SASDebugPrint__
    sas_printf("SASMemBring\n");
#endif
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myMemBring)(startAddr, size, asyncBool);
#endif
};

#ifdef __SAS__
void  SASMemSync (void *startAddr, long size, int asyncBool)
#else
void  SASMemSync (void */*startAddr*/, long /*size*/, int /*asyncBool*/)
#endif
{
#ifdef __SASDebugPrint__
    sas_printf("SASMemSync\n");
#endif
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myMemSync)(startAddr, size, asyncBool);
#endif // __SAS__
}

void* SASSOMAlloc (long blockSize, long /*allocSize*/)
{
#ifdef __SASDebugPrint__
    sas_printf("SASSOMAlloc\n");
#endif
    void 	*temp;

    if (blockSize <= block__align4k)
    {
	blockSize = block__align4k;
    }
    else {
#ifdef block__align16k
      if (blockSize <= block__align16k)
      {
	blockSize = block__align16k;
      }
      else {
#endif
	if (blockSize <= block__align64k)
	{
	    blockSize = block__align64k;
	}
	else {
#ifdef block__align256k
	  if (blockSize <= block__align256k)
	  {
	     blockSize = block__align256k;
	  }
	  else {
#endif
	    if (blockSize <= block__align1m)
	    {
		blockSize = block__align1m;
	    }
	    else {
		if (blockSize <= block__align4m)
		{
		    blockSize = block__align4m;
		}
		else {
		    if (blockSize <= block__align16m)
		    {
			    blockSize = block__align16m;
		    }
#ifdef block__align256k
		    else {
		      if (blockSize <= block__align256k)
		      {
			blockSize = block__align256k;
		      }
		    }
#endif
		}
	    }
#ifdef block__align256k
	  }
#endif
	}
#ifdef block__align16k
      }
#endif
    }

#ifdef __BCPLUSPLUS__
# ifdef __SASSIM__
    temp = SASBlockAlloc (blockSize);
# else
    if (blockSize == block__Size64K)
    {
	    temp = malloc(blockSize);
    } else {
	unsigned long	block;
	block = (long)malloc(blockSize+blockSize);
#  ifdef __SASDebugPrint__
	sas_printf("SASAlloc::SASBlockAlloc @%p \n",(void *)block);
#  endif
	block = block + blockSize - 1;
	block = (block / blockSize) * blockSize;
	temp  = (void *)block;	
    }
# endif
#else
# ifdef __SASSIM__
    temp = SASBlockAlloc (blockSize);
# else
	unsigned long	block;
#  ifdef __SAS__
    long  bringSize;
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    block = (long)(*taskAnchor->myAllocate)(blockSize);
    if (  blockSize > block__Size64K )
    {
	bringSize = block__Size64K;
    } else {
	bringSize = blockSize;
    }
    SASMemBring (block, bringSize, 1);
#  else
    block = (long)malloc(blockSize+blockSize);
#  endif
#  ifdef __SASDebugPrint__
    sas_printf("SASAlloc::SASBlockAlloc @%p \n",(void *)block);
#  endif
    block = block + blockSize - 1;
    block = (block / blockSize) * blockSize;
    temp  = (void *)block;
# endif
#endif
#ifdef __SASDebugPrint__
    sas_printf("SASAlloc::SASBlockAlloc @%p \n", temp);
#endif

#ifndef __SAS__
//#ifndef __SASSIM__
    long	*first	= (long *)temp;
    long	*last	= &first[(blockSize / sizeof(long)) - 1];
    // initialize block to zeros
    for ( ; first != last; )
    {
	*first++ = 0L;
    };
//#endif
#endif

    return temp;
};

void  SASSOMDealloc(void */*blockAddr*/, long /*allocSize*/)
{
#ifdef __SASDebugPrint__
    sas_printf("SASSOMDealloc\n");
#endif
};

void  SASSOMDestroy(void *blockAddr, long blockSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASSOMDestroy\n");
#endif

    if (blockSize <= block__align4k)
    {
	blockSize = block__align4k;
    }
    else {
#ifdef block__align16k
      if (blockSize <= block__align16k)
      {
	blockSize = block__align16k;
      }
      else {
#endif
	if (blockSize <= block__align64k)
	{
	    blockSize = block__align64k;
	}
	else {
#ifdef block__align256k
	  if (blockSize <= block__align256k)
	  {
	     blockSize = block__align256k;
	  }
	  else {
#endif
	    if (blockSize <= block__align1m)
	    {
		blockSize = block__align1m;
	    }
	    else {
		if (blockSize <= block__align4m)
		{
		    blockSize = block__align4m;
		}
		else {
		    if (blockSize <= block__align16m)
		    {
			blockSize = block__align16m;
		    }
#ifdef block__align256k
		    else {
		      if (blockSize <= block__align256k)
		      {
			blockSize = block__align256k;
		      }
		    }
#endif
		}
	    }
#ifdef block__align256k
	  }
#endif
	}
#ifdef block__align16k
      }
#endif
    }

#ifdef __SASDebugPrint__
    sas_printf("SASAlloc::SASBlockDealloc @%p \n", blockAddr);
#endif
#ifdef __SAS__
    // sasBlockDealloc and sasBlockDelete
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myDeAllocate)(blockAddr,blockSize);
#else
#ifdef __SASSIM__
    SASBlockDealloc (blockAddr, blockSize);
#else
    free(blockAddr);
#endif
#endif
}

void* SASLocalAlloc (long allocSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASLocalAlloc\n");
#endif
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    void* temp = (*taskAnchor->myMalloc)(allocSize);
#else  
    void* temp = malloc(allocSize);
#endif
    return temp;
};

void  SASLocalDealloc (void *memAddr, long /*allocSize*/)
{
#ifdef __SASDebugPrint__
    sas_printf("SASLocalDealloc\n");
#endif
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myFree)(memAddr);
#else  
    free(memAddr);
#endif
}; 

SASBlockHeader *SASFindHeader (void *nearObj)
{
#ifdef __SASDebugPrint__
   sas_printf("SASFindHeader @%p \n",nearObj);
#endif
   uintptr_t		temp	= (uintptr_t)nearObj;
    SASBlockHeader	*block = NULL;
   
#ifdef __SAS__
    if ((temp >= 0x40000000) && (temp < 0xC0000000))
    {
#else
#ifdef __SASSIM__
    if ( ((temp >= getfastMemLow()) && (temp < getfastMemHigh())) || 
         ((temp >= __SAS_TEMP_ADDRESS) && (temp < (__SAS_TEMP_FREE)))) {
#endif
#endif
	temp = temp & block__mask512;
	block = (SASBlockHeader *)temp;

#ifdef __SASDebugPrint__
   sas_printf("SASFindHeader mask=%lx -> %p \n",block__mask512, block);
#endif
	if ( !SOMSASCheckBlockSig(block) )
	{
	    temp = temp & block__mask1k;
	    block = (SASBlockHeader *)temp;

	    if ( !SOMSASCheckBlockSig(block) )
	    {
		temp = temp & block__mask4k;
		block = (SASBlockHeader *)temp;

		if ( !SOMSASCheckBlockSig(block) )
		{
#ifdef block__mask16k
		  temp = temp & block__mask16k;
		  block = (SASBlockHeader *)temp;

		  if ( !SOMSASCheckBlockSig(block) )
		  {
#endif
		    temp = temp & block__mask64k;
		    block = (SASBlockHeader *)temp;

		    if ( !SOMSASCheckBlockSig(block) )
		    {
#ifdef block__mask256k
		      temp = temp & block__mask256;
		      block = (SASBlockHeader *)temp;

		      if ( !SOMSASCheckBlockSig(block) )
		      {
#endif
			temp = temp & block__mask1m;
			block = (SASBlockHeader *)temp;

			if ( !SOMSASCheckBlockSig(block) )
			{
			    temp = temp & block__mask4m;
			    block = (SASBlockHeader *)temp;

			    if ( !SOMSASCheckBlockSig(block) )
			    {
				temp = temp & block__mask16m;
				block = (SASBlockHeader *)temp;

				if ( !SOMSASCheckBlockSig(block) )
				{
#ifdef block__mask64m
				  temp = temp & block__mask64m;
				  block = (SASBlockHeader *)temp;

				  if ( !SOMSASCheckBlockSig(block) )
				  {
#ifdef block__mask256m
				    temp = temp & block__mask256m;
				    block = (SASBlockHeader *)temp;

				    if ( !SOMSASCheckBlockSig(block) )
				    {
				      block = NULL;
				    }
#else
				    block = NULL;
#endif
				  }
#else
				    block = NULL;
#endif
				}
			    }
			}
#ifdef block__mask256k
		      }
#endif
		    }
#ifdef block__mask16k
		  }
#endif
		}
	   }
	}
#ifdef __SASDebugPrint__
	    sas_printf("SASFindHeader low=%lx, high=%lx, tmp_low=%lx, tmp_hign=%lx\n",
			getfastMemLow(), getfastMemHigh(),
			__SAS_TEMP_ADDRESS, __SAS_TEMP_FREE);
#endif
#ifdef __SAS__
    }
#else
#ifdef __SASSIM__
    }
#endif
#endif
    return block;
};

void* SASSOMHeapAlloc (SASBlockHeader *block, long allocSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASAlloc::SASNearAlloc in %p for %ld\n", 
    		block, allocSize);
#endif
    void			*temp = NULL;

    if (block)
    {
	temp = freeNode_allocSpace(block->blockFreeSpace, &block->blockFreeSpace,
						allocSize);
    };

#ifdef __SASDebugPrint__
    sas_printf("SASAlloc::SASNearAlloc @%p \n",temp);
#endif

    return temp;
}

void* SASNearAlloc (void *nearObj, long allocSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASNearAlloc (%p, %ld), \n", nearObj, allocSize);
#endif
    void		*temp = NULL;
    SASBlockHeader	*block;

    block = SASFindHeader (nearObj);
#ifdef __SASDebugPrint__
    sas_printf("SASNearAlloc::SASFindHeader @%p \n",block);
#endif
    if (block)
    {
	temp = freeNode_allocSpace(block->blockFreeSpace, &block->blockFreeSpace,
						allocSize);
    }
    else
    {
#ifdef __SAS__
	task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
	temp = (*taskAnchor->myMalloc)(allocSize);
#else
	temp = malloc(allocSize);
#endif
    }

#ifdef __SASDebugPrint__
    sas_printf("SASNearAlloc: return=%p \n", temp);
#endif
    return temp;
}

void
SASSOMHeapDealloc (SASBlockHeader *block, void *memAddr, long allocSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASSOMHeapDealloc in %p @ %p for %ld\n",
			    block, memAddr, allocSize);
#endif
    if (block)
    {
	freeNode_deallocSpace(((freeNode *)memAddr), &block->blockFreeSpace,
					allocSize);
    }
}


void  SASNearDealloc (void *memAddr, long allocSize)
{
#ifdef __SASDebugPrint__
    sas_printf("SASNearDealloc (%p, %ld) \n", memAddr, allocSize);
#endif
    SASBlockHeader	*block;

    block = SASFindHeader (memAddr);
#ifdef __SASDebugPrint__
    sas_printf("SASNearDealloc: find=%p \n", block);
#endif
    if (block)
    {
	freeNode_deallocSpace(((freeNode *)memAddr), &block->blockFreeSpace,
					allocSize);
    }
    else {
#ifdef __SAS__
    task_anchor *taskAnchor = (task_anchor*)taskAnchor_addr;
    (*taskAnchor->myFree)(memAddr);
#else
    free(memAddr);
#endif
    }
}

