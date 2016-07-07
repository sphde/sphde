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

/* Change Activity: */
/* End Change Activity */

#ifndef _SASAlloc_H
#define _SASAlloc_H

#include <stdint.h>

#include "freenode.h"
#include "sasconf.h"
#include "sastype.h"


#define block__Size256	0x00000100L
#define block__Size512	0x00000200L
#define block__Size1K	0x00000400L
#define block__Size4K	0x00001000L
#define block__Size16K	0x00004000L
#define block__Size64K	0x00010000L
#define block__Size256K	0x00040000L
#define block__Size1M	0x00100000L
#define block__Size4M	0x00400000L
#define block__Size16M	0x01000000L
#define block__Size64M	0x04000000L
#define block__Size256M	0x10000000L
#define block__max SegmentSize
#define block__align256	0x00000100L
#define block__align512	0x00000200L
#define block__align1K	0x00000400L
#define block__align4k  0x00001000L
#define block__align16k 0x00004000L
#define block__align64k 0x00010000L
#define block__align256k 0x00040000L
#define block__align1m   0x00100000L
#define block__align4m   0x00400000L
#define block__align16m  0x01000000L
#define block__align64m  0x04000000L
#define block__align256m 0x10000000L
#ifdef __WORDSIZE_64
#define block__Signature_1 0x0102030405060708L
#define block__Signature_2 0xA6A7A8A9AAABACADL 
#define block__mask256 0xffffffffffffff00L
#define block__mask512 0xfffffffffffffe00L
#define block__mask1k  0xfffffffffffffc00L
#define block__mask4k  0xfffffffffffff000L
#define block__mask16k 0xffffffffffffc000L
#define block__mask64k 0xffffffffffff0000L
#define block__mask256k 0xfffffffffffc0000L
#define block__mask1m   0xfffffffffff00000L
#define block__mask4m   0xffffffffffc00000L
#define block__mask16m  0xffffffffff000000L
#define block__mask64m  0xfffffffffc000000L
#define block__mask256m 0xfffffffff0000000L
#else
#define block__Signature_1 0x01020304L
#define block__Signature_2 0xA6A7A8A9L 
#define block__mask256 0xffffff00L
#define block__mask512 0xfffffe00L
#define block__mask1k  0xfffffc00L
#define block__mask4k  0xfffff000L
#define block__mask16k 0xffffc000L
#define block__mask64k 0xffff0000L
#define block__mask256k 0xfffc0000L
#define block__mask1m   0xfff00000L
#define block__mask4m   0xffc00000L
#define block__mask16m  0xff000000L
#define block__mask64m  0xfc000000L
#define block__mask256m 0xf0000000L
#endif
#define block__private 0
#define block__shared 1
#define blockAlign sizeof(long)
#define blockRound (sizeof(long)-1)

typedef struct SASBlockHeader {
    void                  *special;
    uintptr_t             blockSig1;
    sas_type_t            blockType;
    uintptr_t             blockSig2;
    block_size_t          blockSize;
    FreeNode              *blockFreeSpace;
    struct SASBlockHeader *baseBlock;
    struct SASBlockHeader *nextBlock;
} SASBlockHeader;

#ifdef __WORDSIZE_64
# define heap_offset 128
#else
# define heap_offset 64
#endif
#define default_page 4096
#define default_max  63

#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

extern __C__ void
*getSASBlockSpecial(void *block);

extern __C__ void
setSASBlockSpecial (void *block, void *val);

extern __C__ void
initSOMSASBlock(SASBlockHeader *header, sas_type_t sasType,
					long blockSize, void* blockHeap);

static inline int
SOMSASCheckBlockSig (SASBlockHeader* header)
{
    return ((header->blockSig1 == block__Signature_1)
	    &&(header->blockSig2 == block__Signature_2));
}

static inline int
SOMSASCheckBlockSigAndType (SASBlockHeader* header, 
                                                           sas_type_t sasType)
{
    return ((header->blockSig1 == block__Signature_1)
	    &&(header->blockSig2 == block__Signature_2)
	    &&((header->blockType & SAS_TYPE_CHECK_MASK) 
	          == (sasType & SAS_TYPE_MASK)));
}

static inline int
SOMSASCheckBlockSigAndTypeAndSubtype (SASBlockHeader* header, 
                                                           sas_type_t sasType)
{
    return ((header->blockSig1 == block__Signature_1)
	    &&(header->blockSig2 == block__Signature_2)
	    &&((header->blockType & SAS_SUBTYPE_CHECK_MASK) 
	          == (sasType & SAS_SUBTYPE_CHECK_MASK)));
}

static inline int
SOMSASCheckBlockSubType (SASBlockHeader* header, 
                                                           sas_type_t sasType)
{
    return ((header->blockType &  SAS_SUB_TYPE_MASK)
                 == (sasType & SAS_SUB_TYPE_MASK));
}

static inline void
SOMSASSetBlockSubType (SASBlockHeader* header, 
                                                           sas_type_t sasType)
{
    header->blockType = (header->blockType &  ~SAS_SUB_TYPE_MASK)
                 || (sasType & SAS_SUB_TYPE_MASK);
}

static inline sas_type_t
SOMSASGetBlockSubType (SASBlockHeader* header)
{
    return (header->blockType &  SAS_SUB_TYPE_MASK);
}

static inline sas_type_t
SOMSASGetBlockType (SASBlockHeader* header)
{
    return (header->blockType);
}

extern __C__ SASBlockHeader *
SASFindHeader (void *nearObj);

extern __C__ void
SASMemSync(void *startAddr, long size, int asyncBool);

extern __C__ void
SASMemRemove(void *startAddr, long size, int asyncBool);

extern __C__ void
SASMemBring(void *startAddr, long size, int asyncBool);
 

extern __C__ void*
SASSOMAlloc(long blockSize, long allocSize);

extern __C__ void
SASSOMDealloc(void *blockAddr, long allocSize);

extern __C__ void
SASSOMDestroy(void *blockAddr, long blockSize);

extern __C__ void*
SASSOMHeapAlloc(SASBlockHeader *block, long allocSize);

extern __C__ void
SASSOMHeapDealloc(SASBlockHeader *block, void *memAddr, long allocSize);


extern __C__ void*
SASNearAlloc(void *nearObj, long allocSize);

extern __C__ void
SASNearDealloc(void *memAddr, long allocSize);

extern __C__ void*
SASLocalAlloc(long allocSize);

extern __C__ void
SASLocalDealloc(void *memAddr, long allocSize);

#endif /* _SASAlloc_H */

