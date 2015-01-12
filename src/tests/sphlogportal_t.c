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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "sastype.h"
#include "sassim.h"
#include "sphlflogger.h"
#include "sphlflogentry.h"
#include "sphlogportal.h"

int
logportal_basic_test (char *x4k, char *y4k, char *z4k, char *w4k)
{
    int rc = 0;
    int status;
    SPHLogPortal_t portal;
    SPHLFLogger_t lfLog, lfLog2, lfLog3,
    			tmpl0, tmpl1, tmpl2, tmpl3;
    block_size_t aSize;
    SPHLFLoggerHandle_t *handle, handle0, handle1, handle2, handle3;
    SPHLFPortalIterator_t *p_iter, p_iter0;
    char *temp0, *temp1, *temp2, *temp3, *temp4;
    longPtr_t ltmp0, ltmp1, ltmp2;
    int  itmp0, itmp1, itmp2;
    int  icnt0, icnt1;
#ifdef __LP64__
    int  expect_cap = 240;
#else
    int  expect_cap = 480;
#endif
    memset (x4k, 0, 4096);
    memset (y4k, 0, 4096);
    memset (z4k, 0, 4096);

	printf("logportal_basic_test(%p,%p,%p)\n",
			x4k, y4k, z4k);
    portal = SPHLogPortalInit (x4k, 4096);

    lfLog = SPHLFLoggerInit (y4k, 4096);
    lfLog2 = SPHLFLoggerInit (z4k, 4096);
    if ((lfLog != NULL) && (lfLog2 != NULL) && (portal != NULL))
    {
		printf("SPHLogPortalInit(%p)\n",
				portal);
		status = SPHLogPortalEmpty (portal);
		if(status)
		{
    		printf("SPHLogPortalEmpty(%p) succeeds %d\n",
    				portal, status);
		}else{
    		printf("error SPHLogPortalEmpty(%p) failed with %d\n",
    				portal, status);
    		rc++;
		}

		itmp0 = SPHLogPortalEntries (portal);
		if(itmp0 == 0)
		{
			printf("SPHLogPortalEntries(%p) succeeds %d\n",
					portal, itmp0);
		} else {
			printf("SPHLogPortalEntries(%p) failed with %d\n",
					portal, itmp0);
			rc++;
		}

		itmp0 = SPHLogPortalCapacity (portal);
		if(itmp0 == expect_cap)
		{
			printf("SPHLogPortalCapacity(%p) succeeds %d\n",
					portal, itmp0);
		} else {
			printf("SPHLogPortalCapacity(%p) failed with %d expected %d\n",
					portal, itmp0, expect_cap);
			rc++;
		}

		tmpl1 = SPHLogPortalGetCurrentLogger (portal);
		if (!tmpl1)
		{
			printf("SPHLogPortalCurrentLogger(%p) correct returned %p\n",
					portal, tmpl1);
		} else {
			printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
					portal, tmpl1);
			rc++;
		}

		tmpl0 = SPHLogPortalAddLogger (portal, lfLog);
		if (tmpl0)
		{
			printf("SPHLogPortalAddLogger(%p,%p) succeeded %p\n",
					portal, lfLog, tmpl0);
		} else {
			printf("SPHLogPortalAddLogger(%p,%p) failed with %p\n",
					portal, lfLog, tmpl0);
			rc++;
		}

		status = SPHLogPortalEmpty (portal);
		if(!status)
		{
    		printf("SPHLogPortalEmpty(%p) succeeds %d\n",
    				portal, status);
		}else{
    		printf("error SPHLogPortalEmpty(%p) failed with %d\n",
    				portal, status);
    		rc++;
		}

		itmp0 = SPHLogPortalEntries (portal);
		if(itmp0 == 1)
		{
			printf("SPHLogPortalEntries(%p) succeeds %d\n",
					portal, itmp0);
		} else {
			printf("SPHLogPortalEntries(%p) failed with %d\n",
					portal, itmp0);
			rc++;
		}

		tmpl1 = SPHLogPortalGetCurrentLogger (portal);
		if (tmpl1)
		{
			if (tmpl0 == tmpl1)
			{
				printf("SPHLogPortalCurrentLogger(%p) succeeded %p\n",
						portal, tmpl1);
			} else {
				printf("SPHLogPortalCurrentLogger(%p) failed expected %p got %p\n",
						portal, tmpl0, tmpl1);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
					portal, tmpl1);
			rc++;
		}

		itmp1 = SPHLogPortalGetCurrentIndex (portal);
		if(itmp1 == 0)
		{
			printf("SPHLogPortalGetCurrentIndex(%p) succeeds %d\n",
					portal, itmp1);
		} else {
			printf("SPHLogPortalGetCurrentIndex(%p) failed with %d\n",
					portal, itmp1);
			rc++;
		}

		tmpl2 = SPHLogPortalGetLoggerByIndex (portal, itmp1);
		if (tmpl2)
		{
			if (tmpl2 == tmpl1)
			{
				printf("SPHLogPortalGetLoggerByIndex(%p,%d) succeeded %p\n",
						portal, itmp1, tmpl2);
			} else {
				printf("SPHLogPortalGetLoggerByIndex(%p,%d) failed expected %p got %p\n",
						portal, itmp1, tmpl1, tmpl2);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p,%d) failed with %p\n",
					portal, itmp1, tmpl2);
			rc++;
		}

		itmp2 = 1;
		tmpl3 = SPHLogPortalGetLoggerByIndex (portal, itmp2);
		if (!tmpl3)
		{
			printf("SPHLogPortalGetLoggerByIndex(%p,%d) correct %p\n",
						portal, itmp2, tmpl3);
		} else {
			printf("SPHLogPortalCurrentLogger(%p,%d) failed with %p\n",
					portal, itmp2, tmpl3);
			rc++;
		}

		itmp2 = -1;
		tmpl3 = SPHLogPortalGetLoggerByIndex (portal, itmp2);
		if (!tmpl3)
		{
			printf("SPHLogPortalGetLoggerByIndex(%p,%d) correct %p\n",
						portal, itmp2, tmpl3);
		} else {
			printf("SPHLogPortalCurrentLogger(%p,%d) failed with %p\n",
					portal, itmp2, tmpl3);
			rc++;
		}

		tmpl1 = SPHLogPortalAddLogger (portal, lfLog2);
		if (tmpl1)
		{
			printf("SPHLogPortalAddLogger(%p,%p) succeeded adding a second Logger %p\n",
					portal, lfLog2, tmpl1);
		} else {
			printf("SPHLogPortalAddLogger(%p,%p) failed with %p\n",
					portal, lfLog2, tmpl1);
			rc++;
		}

		itmp0 = SPHLogPortalEntries (portal);
		if(itmp0 == 2)
		{
			printf("SPHLogPortalEntries(%p) succeeds %d\n",
					portal, itmp0);
		} else {
			printf("SPHLogPortalEntries(%p) failed with %d\n",
					portal, itmp0);
			rc++;
		}

		tmpl2 = SPHLogPortalGetCurrentLogger (portal);
		if (tmpl2)
		{
			if (tmpl0 == tmpl2)
			{
				printf("SPHLogPortalCurrentLogger(%p) succeeded %p\n",
						portal, tmpl2);
			} else {
				printf("SPHLogPortalCurrentLogger(%p) failed expected %p got %p\n",
						portal, tmpl0, tmpl2);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
					portal, tmpl2);
			rc++;
		}

		itmp1 = SPHLogPortalGetCurrentIndex (portal);
		if(itmp1 == 0)
		{
			printf("SPHLogPortalGetCurrentIndex(%p) succeeds %d\n",
					portal, itmp1);
		} else {
			printf("SPHLogPortalGetCurrentIndex(%p) failed with %d\n",
					portal, itmp1);
			rc++;
		}

		itmp2 = 1;
		tmpl3 = SPHLogPortalGetLoggerByIndex (portal, itmp2);
		if (tmpl3)
		{
			if (tmpl3 == tmpl1)
			{
				printf("SPHLogPortalGetLoggerByIndex(%p,%d) correct %p\n",
						portal, itmp2, tmpl3);
			} else {
				printf("SPHLogPortalGetLoggerByIndex(%p,%d) failed expected %p got %p\n",
						portal, itmp2, tmpl1, tmpl3);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p,%d) failed with %p\n",
					portal, itmp2, tmpl3);
			rc++;
		}

		ltmp0 = (longPtr_t)lfLog;
		ltmp1 = ltmp0 + 4096;
		aSize = 128;
		temp0 = (char*)SPHLogPortalAllocRaw (portal, aSize);
		if (temp0)
		{
		    printf("SPHLogPortalAllocRaw(%p, %zu) = %p\n",
		    		portal, aSize, temp0);
		    strcpy (temp0, "temp0");

		    ltmp2 = (longPtr_t)temp0;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocRaw() = %p in expected range\n",
		    			    		temp0);
		    } else {
		    	printf(" error SPHLogPortalAllocRaw() = %p not between %lx and %lx\n",
		    			    		temp0, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocRaw(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		temp1 = (char*)SPHLogPortalAllocRaw (portal, aSize);
		if (temp1)
		{
		    printf("SPHLogPortalAllocRaw(%p, %zu) = %p\n",
		    		portal, aSize, temp1);
		    strcpy (temp1, "temp1");

		    ltmp2 = (longPtr_t)temp1;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocRaw() = %p in expected range\n",
		    			temp1);
		    } else {
		    	printf(" error SPHLogPortalAllocRaw() = %p not between %lx and %lx\n",
		    			temp1, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocRaw(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		temp2 = (char*)SPHLogPortalAllocRaw (portal, aSize);
		if (temp2)
		{
		    printf("SPHLogPortalAllocRaw(%p, %zu) = %p\n",
		    		portal, aSize, temp2);
		    strcpy (temp2, "temp2");

		    ltmp2 = (longPtr_t)temp2;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocRaw() = %p in expected range\n",
		    			temp2);
		    } else {
		    	printf(" error SPHLogPortalAllocRaw() = %p not between %lx and %lx\n",
		    			temp2, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocRaw(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		temp3 = (char*)SPHLogPortalAllocRaw (portal, aSize);
		if (temp3)
		{
		    printf("SPHLogPortalAllocRaw(%p, %zu) = %p\n",
		    		portal, aSize, temp3);
		    strcpy (temp3, "temp3");

		    ltmp2 = (longPtr_t)temp3;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocRaw() = %p in expected range\n",
		    			temp3);
		    } else {
		    	printf(" error SPHLogPortalAllocRaw() = %p not between %lx and %lx\n",
		    			temp3, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocRaw(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		if (0 != strcmp(temp0, "temp0"))
		{
		    printf("logportal_basic_test data verify temp0=%s failed\n", temp0);
		    return 10;
		}

		if (0 != strcmp(temp1, "temp1"))
		{
		    printf("logportal_basic_test data verify temp1=%s failed\n", temp1);
		    return 10;
		}

		if (0 != strcmp(temp2, "temp2"))
		{
		    printf("logportal_basic_test data verify temp2=%s failed\n", temp2);
		    return 10;
		}

		if (0 != strcmp(temp3, "temp3"))
		{
		    printf("logportal_basic_test data verify temp3=%s failed\n", temp3);
		    return 10;
		}

		aSize = 128-16;
		handle = SPHLogPortalAllocTimeStamped (portal,
				                             1, 16,
				                             aSize,
				                             &handle0);
		if (handle)
		{
			temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
		    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
		    		portal, aSize, handle, temp0);

		    SPHLFlogEntryAddString(handle, (char*)"temp0");

		    ltmp2 = (longPtr_t)temp0;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
		    			    		temp0);
		    } else {
		    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
		    			    		temp0, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		handle = SPHLogPortalAllocTimeStamped (portal,
				                             1, 16+1,
				                             aSize,
				                             &handle1);
		if (handle)
		{
			temp1 = (char*)SPHLFLogEntryGetFreePtr (handle);
		    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
		    		portal, aSize, handle, temp1);

		    SPHLFlogEntryAddString(handle, (char*)"temp1");

		    ltmp2 = (longPtr_t)temp1;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
		    			temp1);
		    } else {
		    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
		    			temp1, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		handle = SPHLogPortalAllocTimeStamped (portal,
				                             1, 16+2,
				                             aSize,
				                             &handle2);
		if (handle)
		{
			temp2 = (char*)SPHLFLogEntryGetFreePtr (handle);
		    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
		    		portal, aSize, handle, temp2);

		    SPHLFlogEntryAddString(handle, (char*)"temp2");

		    ltmp2 = (longPtr_t)temp2;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
		    			temp2);
		    } else {
		    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
		    			temp2, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		handle = SPHLogPortalAllocTimeStamped (portal,
				                             1, 16+3,
				                             aSize,
				                             &handle3);
		if (handle)
		{
			temp3 = (char*)SPHLFLogEntryGetFreePtr (handle);
		    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
		    		portal, aSize, handle, temp3);

		    SPHLFlogEntryAddString(handle, (char*)"temp3");

		    ltmp2 = (longPtr_t)temp3;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
		    			temp3);
		    } else {
		    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
		    			temp3, ltmp0, ltmp1);
		    	rc++;
		    }
		} else {
		    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		if (0 != strcmp(temp0, "temp0"))
		{
		    printf("logportal_basic_test data verify temp0=%s failed\n", temp0);
		    return 10;
		}

		if (0 != strcmp(temp1, "temp1"))
		{
		    printf("logportal_basic_test data verify temp1=%s failed\n", temp1);
		    return 10;
		}

		if (0 != strcmp(temp2, "temp2"))
		{
		    printf("logportal_basic_test data verify temp2=%s failed\n", temp2);
		    return 10;
		}

		if (0 != strcmp(temp3, "temp3"))
		{
		    printf("logportal_basic_test data verify temp3=%s failed\n", temp3);
		    return 10;
		}

		printf("logportal_basic_test data verify success\n");

		aSize = 128;
		temp4 = temp0;
		while (temp4)
		{
			temp4 = (char*)SPHLogPortalAllocRaw (portal, aSize);

		    ltmp2 = (longPtr_t)temp4;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
#if 0
		    	printf("SPHLogPortalAllocRaw() = %p in expected range\n",
		    			    		temp4);
#endif
		    } else {
		    	if (temp4)
		    	{
					ltmp0 = (longPtr_t)lfLog2;
					ltmp1 = ltmp0 + 4096;
				    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
				    {
				    	printf(" SPHLogPortalAllocRaw() = %p switched %lx and %lx\n",
										temp4, ltmp0, ltmp1);
				    }
		    	}
		    }
		}

		p_iter = SPHLFPortalCreateIterator (portal, &p_iter0);
		if (p_iter)
		{
			printf ("SPHLFPortalCreateIterator(%p,%p)=%p success\n",
					portal, &p_iter0, p_iter);

			if (p_iter != &p_iter0)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p)=%p != %p\n",
						portal, &p_iter0, p_iter, &p_iter0);
				rc++;
			}
			if (portal != p_iter->portal)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p) %p != %p\n",
						portal, &p_iter0, portal, p_iter->portal);
				rc++;
			}
			if (p_iter->current != 0)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p) current != 0\n",
						portal, &p_iter0);
				rc++;
			}

			if (rc)
			{
				printf ("p_iter->portal=%p\n", p_iter->portal);
				printf ("p_iter->current=%ld\n", p_iter->current);
				printf ("p_iter->capacity=%ld\n", p_iter->capacity);
				printf ("p_iter->next_free=%ld\n", p_iter->next_free);
				printf ("p_iter->logIter.logger=%p\n", p_iter->logIter.logger);
				printf ("p_iter->logIter.current=%p\n", (void*)p_iter->logIter.current);
				printf ("p_iter->logIter.start_log=%p\n", (void*)p_iter->logIter.start_log);
				printf ("p_iter->logIter.end_log=%p\n", (void*)p_iter->logIter.end_log);
				printf ("p_iter->logIter.options=%ux\n", p_iter->logIter.options);
			}
		} else {
			printf("logportal_basic_test SPHLFPortalCreateIterator failed\n");
			rc += 10;
		}

    } else {
    	if (!lfLog)
    		printf("error logportal_basic_test(%p,%p)  SPHLFLoggerInit(%p) failed\n",
    				x4k, y4k, lfLog);
    	if (!portal)
    		printf("error logportal_basic_test(%p,%p)  SPHLogPortalInit(%p) failed\n",
    				x4k, y4k, portal);
    	return 10;
    }

    memset (x4k, 0, 4096);
    memset (y4k, 0, 4096);
    memset (z4k, 0, 4096);

	printf("logportal_basic_test(%p,%p,%p) continuing switch test\n",
			x4k, y4k, z4k);

    portal = SPHLogPortalInit (x4k, 4096);
    lfLog = SPHLFLoggerInit (y4k, 4096);
    lfLog2 = SPHLFLoggerInit (z4k, 4096);
    if ((lfLog != NULL) && (lfLog2 != NULL) && (portal != NULL))
    {
		printf("SPHLogPortalInit(%p)\n",
				portal);

		tmpl0 = SPHLogPortalAddLogger (portal, lfLog);
		if (tmpl0)
		{
			printf("SPHLogPortalAddLogger(%p,%p) succeeded %p\n",
					portal, lfLog, tmpl0);
		} else {
			printf("SPHLogPortalAddLogger(%p,%p) failed with %p\n",
					portal, lfLog, tmpl0);
			rc++;
		}

		tmpl1 = SPHLogPortalAddLogger (portal, lfLog2);
		if (tmpl1)
		{
			printf("SPHLogPortalAddLogger(%p,%p) succeeded adding a second Logger %p\n",
					portal, lfLog2, tmpl1);
		} else {
			printf("SPHLogPortalAddLogger(%p,%p) failed with %p\n",
					portal, lfLog2, tmpl1);
			rc++;
		}

		itmp0 = SPHLogPortalEntries (portal);
		if(itmp0 == 2)
		{
			printf("SPHLogPortalEntries(%p) succeeds %d\n",
					portal, itmp0);
		} else {
			printf("SPHLogPortalEntries(%p) failed with %d\n",
					portal, itmp0);
			rc++;
		}

		tmpl2 = SPHLogPortalGetCurrentLogger (portal);
		if (tmpl2)
		{
			if (tmpl0 == tmpl2)
			{
				printf("SPHLogPortalCurrentLogger(%p) succeeded %p\n",
						portal, tmpl2);
			} else {
				printf("SPHLogPortalCurrentLogger(%p) failed expected %p got %p\n",
						portal, tmpl0, tmpl2);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
					portal, tmpl2);
			rc++;
		}


		ltmp0 = (longPtr_t)lfLog;
		ltmp1 = ltmp0 + 4096;
		aSize = 128-16;
		icnt0 = 0;
		handle = SPHLogPortalAllocTimeStamped (portal,
				                             15, icnt0,
				                             aSize,
				                             &handle0);
		if (handle)
		{
			temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
		    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
		    		portal, aSize, handle, temp0);

		    SPHLFlogEntryAddString(handle, (char*)"temp0");

		    ltmp2 = (longPtr_t)temp0;
		    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
		    {
		    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
		    			    		temp0);
		    } else {
		    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
		    			    		temp0, ltmp0, ltmp1);
		    	rc++;
		    }

		    SPHLFLoggerEntryComplete(handle);
			if (SPHLFLoggerEntryIsComplete(handle))
			{
			    printf("  SPHLFLoggerEntryIsComplete(%p) succeeded, returned %d\n",
				   handle,
				   SPHLFLoggerEntryIsComplete(handle));
			} else {
			    printf("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
				   handle,
				   SPHLFLoggerEntryIsComplete(handle));
			    rc++;
			}
		} else {
		    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
		    		portal, aSize);
		    return 10;
		}

		while (handle)
		{
			handle = SPHLogPortalAllocTimeStamped (portal,
					                             15, ++icnt0,
					                             aSize,
					                             &handle1);
			if (handle)
			{
				temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
#if 0
			    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
			    		portal, aSize, handle, temp0);
#endif
			    SPHLFlogEntryAddString(handle, (char*)"temp0");

			    SPHLFLoggerEntryComplete(handle);
			} else {
			    printf("SPHLogPortalAllocTimeStamped(%p,%zu) return NULL at %d\n",
			    		portal, aSize, icnt0);
			}
		}

		printf("logportal_basic_test switch test pass1 with %d entries\n",
				icnt0);

		tmpl2 = SPHLogPortalGetCurrentLogger (portal);
		if (tmpl2)
		{
			if (lfLog2 == tmpl2)
			{
				printf("SPHLogPortalCurrentLogger(%p) succeeded %p\n",
						portal, tmpl2);

				if(SPHLFLoggerFull(tmpl2))
				{
					printf("SPHLogPortalCurrentLogger(%p) @ %p is full\n",
							portal, tmpl2);
				} else {
					printf("SPHLFLoggerFull(%p) failed not full\n",
							tmpl2);
					rc++;
				}
			} else {
				printf("SPHLogPortalCurrentLogger(%p) failed expected %p got %p\n",
						portal, lfLog2, tmpl2);
				rc++;
			}
		} else {
			printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
					portal, tmpl2);
			rc++;
		}

		itmp1 = SPHLogPortalGetCurrentIndex (portal);
		if(itmp1 == 1)
		{
			printf("SPHLogPortalGetCurrentIndex(%p) succeeds %d\n",
					portal, itmp1);
		} else {
			printf("SPHLogPortalGetCurrentIndex(%p) failed with %d\n",
					portal, itmp1);
			rc++;
		}

		p_iter = SPHLFPortalCreateIterator (portal, &p_iter0);
		if (p_iter)
		{
			printf ("SPHLFPortalCreateIterator(%p,%p)=%p success\n",
					portal, &p_iter0, p_iter);

			if (p_iter != &p_iter0)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p)=%p != %p\n",
						portal, &p_iter0, p_iter, &p_iter0);
				rc++;
			}
			if (portal != p_iter->portal)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p) %p != %p\n",
						portal, &p_iter0, portal, p_iter->portal);
				rc++;
			}
			if (p_iter->current != 0)
			{
				printf ("SPHLFPortalCreateIterator(%p,%p) current != 0\n",
						portal, &p_iter0);
				rc++;
			}

			if (rc)
			{
				printf ("p_iter->portal=%p\n", p_iter->portal);
				printf ("p_iter->current=%ld\n", p_iter->current);
				printf ("p_iter->capacity=%ld\n", p_iter->capacity);
				printf ("p_iter->next_free=%ld\n", p_iter->next_free);
				printf ("p_iter->logIter.logger=%p\n", p_iter->logIter.logger);
				printf ("p_iter->logIter.current=%p\n", (void*)p_iter->logIter.current);
				printf ("p_iter->logIter.start_log=%p\n", (void*)p_iter->logIter.start_log);
				printf ("p_iter->logIter.end_log=%p\n", (void*)p_iter->logIter.end_log);
				printf ("p_iter->logIter.options=%ux\n", p_iter->logIter.options);
			}

			icnt1 = 0;
			handle = SPHLFPortalIteratorNext(p_iter, &handle2);
			if (handle)
			{
				temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
			    printf("SPHLFPortalIteratorNext(%p, %p) = %p free=%p\n",
			    		portal, &handle2, handle, temp0);

				if (0 != strcmp(temp0, "temp0"))
				{
				    printf("logportal_basic_test data verify temp0=%s failed\n", temp0);
				    return 10;
				}
				if (!SPHLFLogEntryIsComplete(handle))
				{
				    printf("SPHLFLogEntryIsComplete(%p) failed\n", handle);
				    rc++;
				}
				if (!SPHLFLogEntryIsTimestamped(handle))
				{
				    printf("SPHLFLogEntryIsTimestamped(%p) failed\n", handle);
				    rc++;
				}
				if (15 != SPHLFLogEntryCategory(handle))
				{
				    printf("SPHLFLogEntryCategory(%p) failed\n", handle);
				    rc++;
				}
				if (icnt1 != SPHLFLogEntrySubcat(handle))
				{
				    printf("SPHLFLogEntrySubcat(%p) failed\n", handle);
				    rc++;
				}

				while (handle)
				{
					icnt1++;
					handle = SPHLFPortalIteratorNext(p_iter, &handle2);
					if (handle)
					{
		#if 0
					    printf("SPHLFPortalIteratorNext(%p, %p) = %p\n",
					    		portal, &handle2, handle);
		#endif
						if (!SPHLFLogEntryIsComplete(handle))
						{
						    printf("SPHLFLogEntryIsComplete(%p) failed\n", handle);
						    rc++;
						}
						if (!SPHLFLogEntryIsTimestamped(handle))
						{
						    printf("SPHLFLogEntryIsTimestamped(%p) failed\n", handle);
						    rc++;
						}
						if (15 != SPHLFLogEntryCategory(handle))
						{
						    printf("SPHLFLogEntryCategory(%p) failed\n", handle);
						    rc++;
						}
						if (icnt1 != SPHLFLogEntrySubcat(handle))
						{
						    printf("SPHLFLogEntrySubcat(%p) failed\n", handle);
						    rc++;
						}
					} else {
					    printf("SSPHLFPortalIteratorNext(%p,%p) return NULL at %d\n",
					    		portal, &handle2, icnt1);
					}
				}
				if (icnt0 != icnt1)
				{
					rc++;
				}
				printf("logportal_basic_test switch test pass2 with %d entries\n",
						icnt1);


			    memset (w4k, 0, 4096);
			    lfLog3 = SPHLFLoggerInit (w4k, 4096);
			    if (lfLog3)
			    {
					tmpl2 = SPHLogPortalAddLogger (portal, lfLog3);
					if (tmpl2)
					{
						printf("SPHLogPortalAddLogger(%p,%p) succeeded %p\n",
								portal, lfLog3, tmpl2);
					} else {
						printf("SPHLogPortalAddLogger(%p,%p) failed with %p\n",
								portal, lfLog3, tmpl2);
						rc++;
					}

					itmp0 = SPHLogPortalEntries (portal);
					if(itmp0 == 3)
					{
						printf("SPHLogPortalEntries(%p) succeeds %d\n",
								portal, itmp0);
					} else {
						printf("SPHLogPortalEntries(%p) failed with %d\n",
								portal, itmp0);
						rc++;
					}

					tmpl3 = SPHLogPortalGetCurrentLogger (portal);
					if (tmpl3)
					{
						if (tmpl3 == tmpl1)
						{
							printf("SPHLogPortalCurrentLogger(%p) succeeded %p\n",
									portal, tmpl1);
						} else {
							printf("SPHLogPortalCurrentLogger(%p) failed expected %p got %p\n",
									portal, tmpl1, tmpl3);
							rc++;
						}
					} else {
						printf("SPHLogPortalCurrentLogger(%p) failed with %p\n",
								portal, tmpl2);
						rc++;
					}

					printf("logportal_basic_test added logger with %d current entries\n",
							icnt0);


					ltmp0 = (longPtr_t)lfLog3;
					ltmp1 = ltmp0 + 4096;

					handle = SPHLogPortalAllocTimeStamped (portal,
							                             15, icnt0,
							                             aSize,
							                             &handle0);
					if (handle)
					{
						temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
					    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
					    		portal, aSize, handle, temp0);

					    SPHLFlogEntryAddString(handle, (char*)"temp0");

					    ltmp2 = (longPtr_t)temp0;
					    if ((ltmp0 < ltmp2) && (ltmp2 < ltmp1))
					    {
					    	printf("SPHLogPortalAllocTimeStamped() = %p in expected range\n",
					    			    		temp0);
					    } else {
					    	printf(" error SPHLogPortalAllocTimeStamped() = %p not between %lx and %lx\n",
					    			    		temp0, ltmp0, ltmp1);
					    	rc++;
					    }

					    SPHLFLoggerEntryComplete(handle);
						if (SPHLFLoggerEntryIsComplete(handle))
						{
						    printf("  SPHLFLoggerEntryIsComplete(%p) succeeded, returned %d\n",
							   handle,
							   SPHLFLoggerEntryIsComplete(handle));
						} else {
						    printf("  SPHLFLoggerEntryIsComplete(%p) failed, returned %d\n",
							   handle,
							   SPHLFLoggerEntryIsComplete(handle));
						    rc++;
						}
					} else {
					    printf("error SPHLogPortalAllocTimeStamped(%p,%zu) failed\n",
					    		portal, aSize);
					    return 10;
					}

					itmp2 = SPHLogPortalGetCurrentIndex (portal);
					if(itmp2 == 2)
					{
						printf("SPHLogPortalGetCurrentIndex(%p) succeeds %d\n",
								portal, itmp2);
					} else {
						printf("SPHLogPortalGetCurrentIndex(%p) failed with %d\n",
								portal, itmp2);
						rc++;
					}

					while (handle)
					{
						handle = SPHLogPortalAllocTimeStamped (portal,
								                             15, ++icnt0,
								                             aSize,
								                             &handle1);
						if (handle)
						{
							temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
#if 0
						    printf("SPHLogPortalAllocTimeStamped(%p, %zu) = %p free=%p\n",
						    		portal, aSize, handle, temp0);
#endif
						    SPHLFlogEntryAddString(handle, (char*)"temp0");

						    SPHLFLoggerEntryComplete(handle);
						} else {
						    printf("SPHLogPortalAllocTimeStamped(%p,%zu) return NULL at %d\n",
						    		portal, aSize, icnt0);
						}
					}

					printf("logportal_basic_test switch test pass3 with %d entries\n",
							icnt0);


					p_iter = SPHLFPortalCreateIterator (portal, &p_iter0);
					if (p_iter)
					{
						printf ("SPHLFPortalCreateIterator(%p,%p)=%p success\n",
								portal, &p_iter0, p_iter);

						if (p_iter != &p_iter0)
						{
							printf ("SPHLFPortalCreateIterator(%p,%p)=%p != %p\n",
									portal, &p_iter0, p_iter, &p_iter0);
							rc++;
						}
						if (portal != p_iter->portal)
						{
							printf ("SPHLFPortalCreateIterator(%p,%p) %p != %p\n",
									portal, &p_iter0, portal, p_iter->portal);
							rc++;
						}
						if (p_iter->current != 0)
						{
							printf ("SPHLFPortalCreateIterator(%p,%p) current != 0\n",
									portal, &p_iter0);
							rc++;
						}

						if (rc)
						{
							printf ("p_iter->portal=%p\n", p_iter->portal);
							printf ("p_iter->current=%ld\n", p_iter->current);
							printf ("p_iter->capacity=%ld\n", p_iter->capacity);
							printf ("p_iter->next_free=%ld\n", p_iter->next_free);
							printf ("p_iter->logIter.logger=%p\n", p_iter->logIter.logger);
							printf ("p_iter->logIter.current=%p\n", (void*)p_iter->logIter.current);
							printf ("p_iter->logIter.start_log=%p\n", (void*)p_iter->logIter.start_log);
							printf ("p_iter->logIter.end_log=%p\n", (void*)p_iter->logIter.end_log);
							printf ("p_iter->logIter.options=%ux\n", p_iter->logIter.options);
						}

						icnt1 = 0;
						handle = SPHLFPortalIteratorNext(p_iter, &handle2);
						if (handle)
						{
							temp0 = (char*)SPHLFLogEntryGetFreePtr (handle);
						    printf("SPHLFPortalIteratorNext(%p, %p) = %p free=%p\n",
						    		portal, &handle2, handle, temp0);

							if (0 != strcmp(temp0, "temp0"))
							{
							    printf("logportal_basic_test data verify temp0=%s failed\n", temp0);
							    return 10;
							}
							if (!SPHLFLogEntryIsComplete(handle))
							{
							    printf("SPHLFLogEntryIsComplete(%p) failed\n", handle);
							    rc++;
							}
							if (!SPHLFLogEntryIsTimestamped(handle))
							{
							    printf("SPHLFLogEntryIsTimestamped(%p) failed\n", handle);
							    rc++;
							}
							if (15 != SPHLFLogEntryCategory(handle))
							{
							    printf("SPHLFLogEntryCategory(%p) failed\n", handle);
							    rc++;
							}
							if (icnt1 != SPHLFLogEntrySubcat(handle))
							{
							    printf("SPHLFLogEntrySubcat(%p) failed\n", handle);
							    rc++;
							}

							while (handle)
							{
								icnt1++;
								handle = SPHLFPortalIteratorNext(p_iter, &handle2);
								if (handle)
								{
					#if 0
								    printf("SPHLFPortalIteratorNext(%p, %p) = %p\n",
								    		portal, &handle2, handle);
					#endif
									if (!SPHLFLogEntryIsComplete(handle))
									{
									    printf("SPHLFLogEntryIsComplete(%p) failed\n", handle);
									    rc++;
									}
									if (!SPHLFLogEntryIsTimestamped(handle))
									{
									    printf("SPHLFLogEntryIsTimestamped(%p) failed\n", handle);
									    rc++;
									}
									if (15 != SPHLFLogEntryCategory(handle))
									{
									    printf("SPHLFLogEntryCategory(%p) failed\n", handle);
									    rc++;
									}
									if (icnt1 != SPHLFLogEntrySubcat(handle))
									{
									    printf("SPHLFLogEntrySubcat(%p) failed\n", handle);
									    rc++;
									}
								} else {
								    printf("SSPHLFPortalIteratorNext(%p,%p) return NULL at %d\n",
								    		portal, &handle2, icnt1);
								}
							}

							if (icnt0 != icnt1)
							{
								rc++;
							}
							printf("logportal_basic_test switch test pass4 with %d entries\n",
									icnt1);
						} else {
							printf("logportal_basic_test SPHLFPortalCreateIterator failed\n");
							rc += 10;
						}
					} else {
					    printf("error SPHLFPortalCreateIterator(%p,%p) failed\n",
					    		portal, &p_iter0);
					    return 10;

					}
			    } else {
				    printf("error SPHLFLoggerInit(%p,%d) failed\n",
				    		w4k, 4096);
				    return 10;
			    }

			} else {
			    printf("error SPHLFPortalIteratorNext(%p,%p) failed\n",
			    		portal, &handle2);
			    return 10;
			}
		} else {
			printf("logportal_basic_test SPHLFPortalCreateIterator failed\n");
			rc += 10;
		}

    } else {
    	if (!lfLog)
    		printf("error logportal_basic_test(%p,%p)  SPHLFLoggerInit(%p) failed\n",
    				x4k, y4k, lfLog);
    	if (!portal)
    		printf("error logportal_basic_test(%p,%p)  SPHLogPortalInit(%p) failed\n",
    				x4k, y4k, portal);
    	return 10;
    }

    return rc;
}

/* Fake out the SAS region range checking for these tests.
*/

void
setmemrange (unsigned long low, unsigned long high)
{
    unsigned long	ak, bk;

    setSASmemrange (low, high);

    ak = getMemLow();
    bk = getMemHigh();
    printf ("getMemLow()=%lx, getMemHigh()=%lx\n",
            ak, bk);
}

#define stack_block_size 65536
#define stack_block_mask (stack_block_size - 1)
#define stack_buff_size (stack_block_size + stack_block_size)

int main(int argc, char *argv[])
{
    int rc = 0;
    unsigned long	a4k, b4k, c4k, d4k;
    char        a[(stack_block_size * 6)], b[stack_buff_size];
    char *source_address, *dest_address, *dest2_address, *dest3_address;

    memset (a, 0, (stack_block_size * 5));
    memset (b, 0, stack_buff_size);
    a4k = (unsigned long)(a+stack_block_mask) & ~stack_block_mask;
    b4k = a4k + stack_block_size;
    c4k = b4k + stack_block_size;
    d4k = c4k + stack_block_size;

    source_address=(char*)a4k;
    dest_address=(char*)b4k;
    dest2_address=(char*)c4k;
    dest3_address=(char*)d4k;

    printf ("source_address=%p, dest_address=%p dest2_address=%p dest3_address=%p\n",
            source_address, dest_address, dest2_address, dest3_address);
#if 1
    rc += logportal_basic_test(source_address, dest_address,
    		dest2_address, dest3_address);
#endif
    return rc;
}
