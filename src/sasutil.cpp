/*
 * Copyright (c) 2009, 2011 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */


// Control/Test Program (_main) for SASSIM shared memory on AIX and
// Linux

#include <iostream>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasshm.h"
#include "sasalloc.h"
#include "sasstdio.h"
#include "sasmsync.h"
#include "sasstname.h"
#include "sassim.h"
#include "saslock.h"
#ifdef __SOMTEST__
#include "somdef.h"
#include "somcls.h"
#endif
#include "sphcontext.h"

int echo (char * fileName)
{
    int	rc = 0;
    int	c = 0;
    FILE *fp;
    fp = fopen(fileName, "r");

    if (fp > 0)
    {
	while ( c != EOF )
	{
	    c = fgetc (fp);
	    if ( c != EOF )
		putchar(c);
	}
	fclose(fp);
    } else {
	perror("Open failed ..\n");
	rc = errno;
    }
    return rc;
}

void fdisplayProcMaps (__pid_t pid)
{	
    char buff[32];
    sprintf(buff, "/proc/%d/maps", pid);
    echo (buff);
}

void displayProcMaps (char* pid)
{	
    char buff[32];
    sprintf(buff, "/proc/%s/maps", pid);
    echo (buff);
}

void displayMyProcMaps ()
{	
    __pid_t pid;
    pid = getpid();
    fdisplayProcMaps(pid);
}

void sMem (void *baseAddr, unsigned long len)
{
    unsigned long	*temp = (unsigned long *)baseAddr;
    unsigned long	count = len / sizeof(unsigned long);
    unsigned long	i;
    printf("starting scan @%p, %ld\n",temp, count);
    for (i = 0; i < count; i++)
    {
	if (*temp) printf ("scaned %lx@%p\n",*temp,temp);
	temp++;
    };

    printf("scan complete @%p\n",temp);
};

static union {
    char sasChar[4];
    int  sasInt;
} sasTag = { { 'S', 'A', 'S', ' ' } };


#ifdef __SOMTEST__
char *getBlockClassName(void *block)
{
    static char nullStr[4] = { 0, 0, 0, 0 };
    char		*temp = nullStr;
    char		*block_addr = (char*)block;
    SASBlockHeader	*hdr;
    SOMXObject		*obj;
    __InterfaceTable	*itab;
    SOMClass_imp	*cls;

    if (block_addr)
    {
	hdr	= (SASBlockHeader *) block_addr;
	if ( SOMSASCheckBlockSig( hdr )
	&& (((unsigned long)block) != getMemLow()))
	{
	    block_addr = block_addr +
			    (((sizeof(SASBlockHeader) + 15) / 16) * 16);
	    obj	 = (SOMXObject *)block_addr;
	    itab = obj->iTab;
	    obj  = (SOMXObject *)itab->header.clsPtr;
	    cls  = (SOMClass_imp*)obj->dataLoc;
	    temp = cls->className;
	}
    }
    return temp;
}
#endif

void dumpBlockData(void *blockAddr, unsigned long len)
{
    unsigned int	*dumpAddr = (unsigned int*)blockAddr;
    unsigned char	*charAddr = (unsigned char*)blockAddr;
    void		*tempAddr;
    unsigned char	chars[20];
    unsigned char	temp;
    unsigned int	i, j;

    chars[16] = 0;

    for ( i = 0; i < len; i = i + 16 )
    {
	tempAddr = dumpAddr;
	for ( j = 0; j < 16; j++ )
	{
	    temp = *charAddr++;
	    if ( (temp < 32) || (temp > 126) )
		temp = '.';
	    chars[j] = temp;
	};

	printf ("%p  %08x %08x %08x %08x <%s>\n",
		tempAddr,
		*dumpAddr,
		*(dumpAddr+1),
		*(dumpAddr+2),
		*(dumpAddr+3),
		chars);
	dumpAddr += 4;
    }
}

int main ()
{
    void	*BaseAddr3 = NULL;
//  void	*BaseAddr4 = NULL;
//  void	*BaseAddr5 = NULL;
//  unsigned long	temp;
//  unsigned long	align;
    int	rc;
    int			i, ss;
    int			notDone = 1;

    int			count;
    void		*addrList[4096];
    unsigned long	sizeList[4096];
    char		*dumpAddr;
    unsigned long	dumpLen;
    unsigned long	tUsed, tFree, tUncom, tUsedReg, tFreeReg;
    unsigned int	anchorFree;

    SAS_IO_INIT		// init the io stuff

    rc = SASJoinRegion();

    if (rc)
    {
	printf("SASJoinRegion Error# %d\n", rc);
	BaseAddr3 = NULL;
    } else {
	printf("SAS Joined\n");

	do {
            cout << "0) Exit          7) List"             << endl; 
	    cout << "1) Stats         8) Path"             << endl;
	    cout << "2) Details       9) Map"              << endl;
	    cout << "3) Reset        10) Lock High Stats"  << endl;
	    cout << "4) Dump         11) Lock Details"     << endl;
	    cout << "5) Next         12) Semaphores Stats" << endl;
	    cout << "6) Remove"                            << endl;

	    cout << ": ";
	    cin >> ss;

	    switch ( ss ) {
	    case 0: // Exit
		notDone = 0;
		break;
	    case 1: // Stat
		{
		    //SASSeize();

		    SASListInUseMem (addrList, sizeList, &count);

		    tUsed = 0L;
		    for ( i = 0; i < count; i++)
		    {
			tUsed = tUsed + sizeList[i];
		    };

		    SASListFreeMem (addrList, sizeList, &count);

		    tFree = 0L;
		    for ( i = 0; i < count; i++)
		    {
			tFree = tFree + sizeList[i];
		    };

		    SASListUncommittedMem (addrList, sizeList, &count);

		    tUncom = 0;
		    for ( i = 0; i < count; i++)
		    {
			tUncom = tUncom + sizeList[i];
		    };

		    SASListFreeRegion (addrList, sizeList, &count);

		    tFreeReg = 0;
		    for ( i = 0; i < count; i++)
		    {
			tFreeReg = tFreeReg + sizeList[i];
		    };

		    SASListAllocatedRegion (addrList, sizeList, &count);

		    tUsedReg = 0;
		    for ( i = 0; i < count; i++)
		    {
			tUsedReg = tUsedReg + sizeList[i];
		    };

		    anchorFree = SASAnchorFreeSpace();

		    //SASRelease();
		    
		    printf ("Total in use      %ldKB\n", (tUsed/1024));
		    printf ("Total free        %ldKB\n", (tFree/1024));
		    printf ("Total Uncommitted %ldKB\n", (tUncom/1024));
		    printf ("Total Region free %ldKB\n", (tFreeReg/1024));
		    printf ("Total Region used %ldKB\n", (tUsedReg/1024));
		    printf ("Anchor Free Space %d\n", (anchorFree));

		    break;
		};

	    case 2: // Details
		{
		    SASListInUseMem (addrList, sizeList, &count);

		    tUsed = 0L;
		    for ( i = 0; i < count; i++)
		    {
			tUsed = tUsed + sizeList[i];
#ifdef __SOMTEST__
			if ( i != 0 )
			    clsName = getBlockClassName(addrList[i]);
			printf ("%d, %p, %ldKB, %s\n", i, addrList[i],
						    (sizeList[i]/1024),
						    clsName);
#else
			printf ("%d, %p, %ldKB\n", i, addrList[i],
						    (sizeList[i]/1024));
#endif
		    }
		    printf ("Total in use      %ldKB\n", (tUsed/1024));

		    SASListFreeMem (addrList, sizeList, &count);

		    tFree = 0L;
		    for ( i = 0; i < count; i++)
		    {
			tFree = tFree + sizeList[i];
			printf ("%d, %p, %ldKB\n", i, addrList[i],
						(sizeList[i]/1024));
		    }
		    printf ("Total free        %ldKB\n", (tFree/1024));

		    SASListUncommittedMem (addrList, sizeList, &count);

		    tUncom = 0;
		    for ( i = 0; i < count; i++)
		    {
			tUncom = tUncom + sizeList[i];
			printf ("%d, %p, %ldKB\n", i, addrList[i],
						(sizeList[i]/1024));
		    }
		    printf ("Total Uncommitted %ldKB\n", (tUncom/1024));

		    SASListFreeRegion (addrList, sizeList, &count);

		    tFreeReg = 0;
		    for ( i = 0; i < count; i++)
		    {
			tFreeReg = tFreeReg + sizeList[i];
			printf ("%d, %p, %ldKB\n", i, addrList[i],
						(sizeList[i]/1024));
		    }
		    printf ("Total Region free %ldKB\n", (tFreeReg/1024));

		    SASListAllocatedRegion (addrList, sizeList, &count);

		    tUsedReg = 0;
		    for ( i = 0; i < count; i++)
		    {
			tUsedReg = tUsedReg + sizeList[i];
			printf ("%d, %p, %ldKB\n", i, addrList[i],
						(sizeList[i]/1024));
		    }
		    printf ("Total Region used %ldKB\n", (tUsedReg/1024));
		    break;
		};
	    case 3: // Reset
		{
		    SASResetSem();
		    SASReset();
		    break;
		};
	    case 4: // Dump
		{
		    printf("@?");
		    scanf("%p", &dumpAddr);
		    printf("len?");
		    scanf("%lx", &dumpLen);
		    dumpBlockData(dumpAddr, dumpLen);
		    break;
		};
	    case 5: // Next
		{
		    dumpAddr = dumpAddr + dumpLen;
		    dumpBlockData(dumpAddr, dumpLen);
		    break;
		};
	    case 6: // Remove
		{
		    SASRemove();
		    return 0;
		};
	    case 7: // List
		{
		    SASListInUseMem (addrList, sizeList, &count);

		    tUsed = 0L;
		    for ( i = 0; i < count; i++)
		    {
			tUsed = tUsed + sizeList[i];

			printf ("%d, %p, %ldKB\n", i, addrList[i],
									(sizeList[i]/1024));
			if ( i != 0 )
			{
			    int *tmpptr = (int*)addrList[i];
			  
			    if ( *tmpptr == sasTag.sasInt )
				printf(" %s\n", (char*)addrList[i]);
			    else
				sMem(addrList[i],sizeList[i]);						
			}
		    };
		    printf ("Total in use      %ldKB\n", (tUsed/1024));
		    break;
		};
	    case 8: // Path
		{
		    char *sas_path;

		    sas_path = getenv("SASSTOREPATH");
		    if (sas_path != NULL)
			printf ("SASSTOREPATH <%s>\n", sas_path);
		    else
			printf ("SASSTOREPATH defaults to .\n");
		    break;
		};
	    case 9: // Map
		{
		    displayMyProcMaps();
		    break;
		};
	    case 10: // Lock High Stats
		{
		    SASLockPrintHighLevelStats();
		    break;
		};
	    case 11: // Lock Details
		{
		    SASLockPrintDetailedStats();
		    break;
		};
	    case 12: // Semaphores Stats
		{
		    //SASPrintSemStatsAll();
		    SASPrintLockSemStats();
		    break;
		};
	    default :
		cout << endl << "try again" << endl;
	    }

	} while (notDone);
    }

    SASCleanUp();
    return 0;
};

