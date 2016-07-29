/*
 * Copyright (c) 2011 IBM Corporation.
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "sasshm.h"
#include "sasconf.h"
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
#include "sphlflogentry.h"
#include "sphlflogger.h"
#include "sphthread.h"
#include "sphtimer.h"

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

int
lflogentry_test_handle (SPHLFLogger_t log, 
			SPHLFLoggerHandle_t *handle_in,
			int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             handle_in);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(handle, val1);
	    rc2 = SPHLFlogEntryAddInt(handle, val2);
	    rc3 = SPHLFlogEntryAddInt(handle, val3);
	    SPHLFLogEntryComplete(handle);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_test (SPHLFLogger_t log, int val1, int val2, int val3)
{
	int	rc1, rc2, rc3;
	SPHLFLoggerHandle_t *handle, handle0;


	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    rc1 = SPHLFlogEntryAddInt(&handle0, val1);
	    rc2 = SPHLFlogEntryAddInt(&handle0, val2);
	    rc3 = SPHLFlogEntryAddInt(&handle0, val3);
	    SPHLFLogEntryComplete(&handle0);
	} else {
	    return -1;
	}

	return (rc1 | rc2 | rc3);
}

int
lflogentry_fasttest (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_fastteststrong (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryStrongComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_fasttestweak (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocTimeStamped (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_fasttestnolockweak (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocTimeStampedNoLock (log,
                             123, 76,
                             112,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetest (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStamped (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
lflogentry_stridetestnolock (SPHLFLogger_t log, int val1, int val2, int val3)
{
	SPHLFLoggerHandle_t *handle, handle0;
	int	*array;

	handle = SPHLFLoggerAllocStrideTimeStampedNoLock (log,
                             123, 76,
                             &handle0);
	if (handle)
	{
	    array  = (int*)SPHLFLogEntryGetFreePtr(handle);
	    array[0] = val1;
	    array[1] = val2;
	    array[2] = val3;
	    SPHLFLogEntryWeakComplete(&handle0);
	} else {
	    return -1;
	}

	return 0;
}

int
test_unit (void)
{
	SPHLFLogger_t logger;
	SPHLFLoggerHandle_t *handle, handle0;
	sphtimer_t	tempt, startt, endt, freqt;
	unsigned int i, tempn_i, temp0_i, temp1_i, temp2_i;
	int rc, rtn = 0;
	int	*tarray;
	double clock, nano, rate;
	block_size_t cap, units, p10, log_alloc;
	SPHLFLoggerHandle_t *handlex, handle4, handle5;
	sphtimer_t entry_timestamp, prev_timestamp;
	SPHLFLogIterator_t *iter, iter0;
		
	log_alloc = SegmentSize;
	
	logger = SPHLFLoggerCreate (log_alloc);
	if ( logger )
	{
		printf("\nSPHLFLoggerCreate (%zu) success \n", log_alloc);

		cap = SPHLFLoggerFreeSpace( logger );
			
		units = cap / 128;
			
		printf("SPHLFLoggerFreeSpace() = %zu units=%zu\n",
			cap, units);

		for ( i = 0; i < units; i++ )
		{
			rc = lflogentry_test (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n", 
					logger);
				break;
			}
		}
#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			SPHLFLoggerFreeSpace( logger ));

		printf("\ntest_unit() verify log contents\n");
		iter = SPHLFLoggerCreateIterator(logger, &iter0);
		if (iter)
		{
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
				   logger, &iter0, iter);
		} else {
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
				   logger, &iter0, iter);
			return (rtn + 10);
		}

		handlex = SPHLFLoggerIteratorNext (iter, &handle4);
		printf("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
			   iter->logger, iter->current, iter->free, iter->start_log, iter->end_log);
		if (handlex)
		{
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
				   logger, &handle4, handlex);
			printf("   @%p->id=%x\n",
				   handlex->entry, handlex->entry->entryID.idUnit);
			
			entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
			temp0_i = SPHLFlogEntryGetNextInt(handlex);
			temp1_i = SPHLFlogEntryGetNextInt(handlex);
			temp2_i = SPHLFlogEntryGetNextInt(handlex);
			if ((temp0_i != 0)
			  ||  (temp1_i != 0x12345678)
			  ||  (temp2_i != 0xdeadbeef))
			{
				printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x\n",
					   temp0_i, temp1_i, temp2_i);
				rtn++;
			}
		} else {
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
				   logger, &handle4, handlex);
			return (rtn + 10);
		}
		
		while (handlex)
		{
			tempn_i = temp0_i;
			prev_timestamp = entry_timestamp;
			handlex = SPHLFLoggerIteratorNext (iter, &handle5);
			if (handlex)
			{
				temp0_i = SPHLFlogEntryGetNextInt(handlex);
				temp1_i = SPHLFlogEntryGetNextInt(handlex);
				temp2_i = SPHLFlogEntryGetNextInt(handlex);
				entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
				if ((temp0_i != (tempn_i+1))
				 ||  (temp1_i != 0x12345678)
				 ||  (temp2_i != 0xdeadbeef)
				 ||  (entry_timestamp < prev_timestamp))
				{
					printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x, %llu,%llu\n",
						   temp0_i, temp1_i, temp2_i,
						   prev_timestamp, entry_timestamp);
					rtn++;
				}
			}
		}
		
		printf("test_unit() verify log complete\n\n");
#endif /* SPH_TIMERTEST_VERIFY */
		SPHLFLoggerResetAsync (logger);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;
			
		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_test (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n", 
					logger);
				break;
			}
		}
			
		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);
			
		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_test X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);


#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);


#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */
					
		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;
			
		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fasttest (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n", 
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);
			
		printf ("lflogentry_fasttest X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);
		SPHLFLoggerSetCachePrefetch(logger, 1);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fasttest (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_fasttest prefetch0 X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);
		SPHLFLoggerSetCachePrefetch(logger, 2);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fasttest (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_fasttest prefetch1 X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fastteststrong (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_fastteststrong X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fasttestweak (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_fasttestweak X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 10000000;
		while (p10 > units)
			p10 = p10 / 10;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_fasttestnolockweak (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocTimeStamped (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = p10 / (clock / (double)freqt);

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);

		printf ("lflogentry_fasttestnolockweak X %zu ave= %6.2fns rate=%10.1f/s\n",
			p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		printf("\nSPHLFLoggerDestroy (%p) \n", logger);
		SPHLFLoggerDestroy( logger );
	} else
		printf("SPHLFLoggerCreate (%zu) failed \n", log_alloc);

	logger = SPHLFCircularLoggerCreate (log_alloc, 128);
	if ( logger )
	{
		printf("\nSPHLFCirgularLoggerCreate (%zu) success \n", log_alloc);

		rc += SPHLFLoggerPrefetch(logger);
		
		cap = SPHLFLoggerFreeSpace( logger );
		units = cap / 128;
		
		printf("SPHLFLoggerFreeSpace() = %zu, units=%zu\n",
		       cap, units);

		for ( i = 0; i < units; i++ )
		{
			handle = SPHLFLoggerAllocStrideTimeStamped (logger,
								0, 0,
								&handle0);
			if ( handle )
			{
			} else {
				printf("SPHLFLoggerAllocStrideTimeStamped (%p) failed\n", 
					logger);
			}
		}

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */
					
		p10 = 100000000;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_stridetest (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocStrideTimeStamped (%p) failed\n", 
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = 1000000000.0 / nano;

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);
		printf ("lflogentry_stridetest X %zu ave= %6.2fns rate=%10.1f/s\n",
				p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("\ntest_unit() verify log contents\n");
		iter = SPHLFLoggerCreateIterator(logger, &iter0);
		if (iter)
		{
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
				   logger, &iter0, iter);
		} else {
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
				   logger, &iter0, iter);
			return (rtn + 10);
		}

		handlex = SPHLFLoggerIteratorNext (iter, &handle4);
		printf("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
			   iter->logger, iter->current, iter->free, iter->start_log, iter->end_log);
		if (handlex)
		{
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
				   logger, &handle4, handlex);
			printf("   @%p->id=%x\n",
				   handlex->entry, handlex->entry->entryID.idUnit);
			
			entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
			tarray = (int*)SPHLFLogEntryGetFreePtr(handlex);
			temp0_i = tarray[0];
			temp1_i = tarray[1];
			temp2_i = tarray[2];
			if ((temp1_i != 0x12345678)
			  ||  (temp2_i != 0xdeadbeef))
			{
				printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x\n",
					   temp0_i, temp1_i, temp2_i);
				rtn++;
			}
		} else {
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
				   logger, &handle4, handlex);
			return (rtn + 10);
		}
		
		while (handlex)
		{
			tempn_i = temp0_i;
			prev_timestamp = entry_timestamp;
			handlex = SPHLFLoggerIteratorNext (iter, &handle5);
			if (handlex)
			{
				temp0_i = SPHLFlogEntryGetNextInt(handlex);
				temp1_i = SPHLFlogEntryGetNextInt(handlex);
				temp2_i = SPHLFlogEntryGetNextInt(handlex);
				entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
				if ((temp0_i != (tempn_i+1))
				 ||  (temp1_i != 0x12345678)
				 ||  (temp2_i != 0xdeadbeef)
				 ||  (entry_timestamp < prev_timestamp))
				{
					printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x, %llu,%llu\n",
						   temp0_i, temp1_i, temp2_i,
						   prev_timestamp, entry_timestamp);
					rtn++;
				}
			}
		}
		
		printf("test_unit() verify log complete\n\n");
			
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		SPHLFLoggerResetAsync (logger);

#ifdef SPH_TIMERTEST_VERIFY
		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */

		p10 = 100000000;

		startt = sphgettimer();
		for ( i = 0; i < p10; i++ )
		{
			rc = lflogentry_stridetestnolock (logger, i, 0x12345678, 0xdeadbeef);
			if ( !rc )
			{
			} else {
				printf("SPHLFLoggerAllocStrideTimeStampedNoLock (%p) failed\n",
					logger);
				break;
			}
		}

		endt = sphgettimer();
		tempt = endt -startt;
		clock = tempt;
		freqt = sphfastcpufreq();
		nano = (clock * 1000000000.0) / (double)freqt;
		nano = nano / p10;
		rate = 1000000000.0 / nano;

		printf ("\nstartt=%lld, endt=%lld, deltat=%lld, freqt=%lld\n",
			startt, endt, tempt, freqt);
		printf ("lflogentry_stridetestnolock X %zu ave= %6.2fns rate=%10.1f/s\n",
				p10, nano, rate);

#ifdef SPH_TIMERTEST_VERIFY
		printf("\ntest_unit() verify log contents\n");
		iter = SPHLFLoggerCreateIterator(logger, &iter0);
		if (iter)
		{
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p succeeded\n",
				   logger, &iter0, iter);
		} else {
			printf("  SPHLFLoggerCreateIterator(%p,%p) = %p failed\n",
				   logger, &iter0, iter);
			return (rtn + 10);
		}

		handlex = SPHLFLoggerIteratorNext (iter, &handle4);
		printf("   SPHLFLoggerIteratorNext() [%p,%lx,%lx,%lx,%lx]\n",
			   iter->logger, iter->current, iter->free, iter->start_log, iter->end_log);
		if (handlex)
		{
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p succeeded\n",
				   logger, &handle4, handlex);
			printf("   @%p->id=%x\n",
				   handlex->entry, handlex->entry->entryID.idUnit);

			entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
			tarray = (int*)SPHLFLogEntryGetFreePtr(handlex);
			temp0_i = tarray[0];
			temp1_i = tarray[1];
			temp2_i = tarray[2];
			if ((temp1_i != 0x12345678)
			  ||  (temp2_i != 0xdeadbeef))
			{
				printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x\n",
					   temp0_i, temp1_i, temp2_i);
				rtn++;
			}
		} else {
			printf("  SPHLFLoggerIteratorNext(%p,%p) = %p failed\n",
				   logger, &handle4, handlex);
			return (rtn + 10);
		}

		while (handlex)
		{
			tempn_i = temp0_i;
			prev_timestamp = entry_timestamp;
			handlex = SPHLFLoggerIteratorNext (iter, &handle5);
			if (handlex)
			{
				temp0_i = SPHLFlogEntryGetNextInt(handlex);
				temp1_i = SPHLFlogEntryGetNextInt(handlex);
				temp2_i = SPHLFlogEntryGetNextInt(handlex);
				entry_timestamp = SPHLFLogEntryTimeStamp (handlex);
				if ((temp0_i != (tempn_i+1))
				 ||  (temp1_i != 0x12345678)
				 ||  (temp2_i != 0xdeadbeef)
				 ||  (entry_timestamp < prev_timestamp))
				{
					printf("  SPHLFLoggerIteratorNext() data mismatch found %d,%x,%x, %llu,%llu\n",
						   temp0_i, temp1_i, temp2_i,
						   prev_timestamp, entry_timestamp);
					rtn++;
				}
			}
		}

		printf("test_unit() verify log complete\n\n");

		printf("SPHLFLoggerFreeSpace() = %zu\n",
			   SPHLFLoggerFreeSpace( logger ));
#endif /* SPH_TIMERTEST_VERIFY */


		printf("\nSPHLFLoggerDestroy (%p) \n", logger);
		SPHLFLoggerDestroy( logger );
	} else
		printf("SPHLFLoggerCreate (%zu) failed \n", log_alloc);

	return rtn;
}

int main ()
{
    int	rc;

    SAS_IO_INIT		// init the io stuff

    rc = SASJoinRegion();

    if (rc)
    {
	printf("SASJoinRegion Error# %d\n", rc);

	return 1;
    } else {
	printf("SAS Joined\n");
	
		rc = test_unit();
    }

    //SASCleanUp();
    printf("SAS removed\n");
    SASRemove();
    return rc;
};

