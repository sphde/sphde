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

#include "sphtimer.h"
#include "sasconf.h"
#include "sastype.h"
#include "sassim.h"
#include "sphlfentry.h"
#include "sphsinglepcqueue.h"

int
lfpcqueue_basic_test (char *x4k)
{
    int rc = 0;
    SPHSinglePCQueue_t pcqueue;
    block_size_t lfspace, lf0space, lfTemp;
    block_size_t aSize;;
    char *temp0, *temp1, *temp2, *temp3, *temp4;

	aSize = 128;

    memset (x4k, 0, 4096);

    pcqueue = SPHSinglePCQueueInitWithStride (x4k, 1024, aSize, 0);
    if (pcqueue)
    {
		lf0space = SPHSinglePCQueueFreeSpace (pcqueue);
		lfspace = lf0space;

		printf("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lf0space);

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
		} else {
			printf("lfpcqueue_basic_test SPHSinglePCQueueEmpty(%p) failed\n",
			pcqueue);
			rc++;
		}

			if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
			pcqueue);
			rc++;
		} else {
		}

		temp0 = (char*)SPHSinglePCQueueAllocRaw (pcqueue);
		if (temp0)
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
			       pcqueue, aSize, temp0);
			strcpy (temp0, "temp0");
			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize))
			{
				printf("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize));
			    rc++;
			}
		} else {
			printf("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
			        pcqueue, aSize);
			return 10;
		}

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
			printf("lfpcqueue_basic_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
			       pcqueue);
		}

		temp1 = (char*)SPHSinglePCQueueAllocRaw (pcqueue);
		if (temp1)
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
			       pcqueue, aSize, temp1);
			strcpy (temp1, "temp1");
			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize))
			{
				printf("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize));
			    rc++;
			}
		} else {
			printf("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
			       pcqueue, aSize);
			return 10;
		}

		temp2 = (char*)SPHSinglePCQueueAllocRaw (pcqueue);
		if (temp2)
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
			        pcqueue, aSize, temp2);
			strcpy (temp2, "temp2");
			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			            pcqueue, lfspace, lfTemp, (aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
			       pcqueue, aSize);
			return 10;
		}

		temp3 = (char*)SPHSinglePCQueueAllocRaw (pcqueue);
		if (temp3)
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
			       pcqueue, aSize, temp3);
			strcpy (temp3, "temp3");
			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
			       pcqueue, aSize);
			return 10;
		}

		if (0 != strcmp(temp0, "temp0"))
		{
			printf("lfpcqueue_basic_test data verify temp0=%s failed\n", temp0);
			return 10;
		}

		if (0 != strcmp(temp1, "temp1"))
		{
			printf("lfpcqueue_basic_test data verify temp1=%s failed\n", temp1);
			return 10;
		}

		if (0 != strcmp(temp2, "temp2"))
		{
			printf("lfpcqueue_basic_test data verify temp2=%s failed\n", temp2);
			return 10;
		}

		if (0 != strcmp(temp3, "temp3"))
		{
			printf("lfpcqueue_basic_test data verify temp3=%s failed\n", temp3);
			return 10;
		}

		printf("lfpcqueue_basic_test data\n");
		/* in its current state the queue is neither empty or full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		printf("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lfspace);

		while (lfspace > 0)
		{
			temp4 = (char*)SPHSinglePCQueueAllocRaw (pcqueue);
			if (temp4)
			{
				printf("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
				       pcqueue, aSize, temp4);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				printf("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
					x4k, pcqueue, lfspace);
			} else {
				printf("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
				       pcqueue, aSize);
				return 10;
			}
		}

		/* in its current state the queue is not empty and is full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_basic_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
		} else {
			printf("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		}
    } else {
		printf("error lfpcqueue_basic_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
			   x4k, x4k);
		return 10;
    }

    return rc;
}

int
lfpcqueue_stride_test (char *x4k)
{
    int rc = 0;
    SPHSinglePCQueue_t pcqueue;
    block_size_t lfspace, lf0space, lfTemp;
    block_size_t aSize;;
    SPHLFEntryHandle_t *handle, handle0, handle1, handle2, handle3;
    SPHLFEntryHandle_t *handlex, handle4, handle5;
    char *temp0, *temp1, *temp2, *temp3, *tempx;
    int cat, sub;

	aSize = 128;

    memset (x4k, 0, 4096);

    pcqueue = SPHSinglePCQueueInitWithStride (x4k, 1024, aSize, 0);
    if (pcqueue)
    {
		lf0space = SPHSinglePCQueueFreeSpace (pcqueue);
		lfspace = lf0space;

		printf("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lf0space);

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
        	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
        	if (handlex)
        	{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
    			       pcqueue, &handle5);
    			rc++;
        	} else {

        	}
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
			pcqueue);
			rc++;
		}

			if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			pcqueue);
			rc++;
		} else {
		}

		handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
		if (handle)
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle0, handle);

			temp0 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp0"))
			{
				printf("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp0) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
	        	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	        	if (handlex)
	        	{
	    			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	    			       pcqueue, &handle5);
	    			tempx = (char*)SPHLFEntryGetFreePtr (handlex);
	    			if (temp0 == tempx)
	    			{} else {
		    			printf("error lfpcqueue_stride_test SPHLFEntryGetFreePtr(%p) = %p != %p \n",
		    			       handlex, tempx, temp0);
		    			rc++;
	    			}
	        	} else {
	    			printf("error lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed none empty queue \n",
	    			       pcqueue, &handle5);
	    			rc++;
	        	}
			} else {
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize))
			{
				printf("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle0);
			return 10;
		}

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
			       pcqueue);
		}

        handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle1);
		if (handle)
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle1, handle);

			temp1 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp1"))
			{
				printf("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp1) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize))
			{
				printf("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle1);
			return 10;
		}

		handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle2);
		if (handle)
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle2, handle);

			temp2 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp2"))
			{
				printf("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp2) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			            pcqueue, lfspace, lfTemp, (aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle2);
			return 10;
		}

		handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle3);
		if (handle)
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle3, handle);

			temp3 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp3"))
			{
				printf("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp3) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle3);
			return 10;
		}

		if (0 != strcmp(temp0, "temp0"))
		{
			printf("lfpcqueue_stride_test data verify temp0=%s failed\n", temp0);
			return 10;
		}

		if (0 != strcmp(temp1, "temp1"))
		{
			printf("lfpcqueue_stride_test data verify temp1=%s failed\n", temp1);
			return 10;
		}

		if (0 != strcmp(temp2, "temp2"))
		{
			printf("lfpcqueue_stride_test data verify temp2=%s failed\n", temp2);
			return 10;
		}

		if (0 != strcmp(temp3, "temp3"))
		{
			printf("lfpcqueue_stride_test data verify temp3=%s failed\n", temp3);
			return 10;
		}

		printf("lfpcqueue_stride_test queue full tests\n");
		/* in its current state the queue is neither empty or full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		printf("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lfspace);

		while (lfspace > 0)
		{
			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 1, &handle4);
			if (handle)
			{
				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
				       pcqueue, 2, 1, &handle4, handle);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				printf("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
					x4k, pcqueue, lfspace);

				if (SPHSinglePCQueueEntryComplete(handle))
				{

				} else {
					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
				       pcqueue, aSize);
				return 10;
			}
		}

		/* in its current state the queue is not empty and is full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
		} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		}

		lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		if (lfspace)
		{
			printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
				   x4k, pcqueue, lfspace);
			rc++;
		}

		printf("lfpcqueue_stride_test dequeue tests\n");
    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
    		tempx = SPHLFEntryGetNextString (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
					cat,sub,tempx);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 1)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 1\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp0"))
    			{
    				printf("lfpcqueue_stride_test data verify temp0=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		tempx = SPHLFEntryGetNextString (handlex);
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp1"))
    			{
    				printf("lfpcqueue_stride_test data verify temp1=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		tempx = SPHLFEntryGetNextString (handlex);
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp2"))
    			{
    				printf("lfpcqueue_stride_test data verify temp2=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
    		tempx = SPHLFEntryGetNextString (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
					cat,sub,tempx);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 1)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 1\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp3"))
    			{
    				printf("lfpcqueue_stride_test data verify temp3=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 1)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 1\n");
    			rc++;
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 256L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 384L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 512L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
    			printf("error lfpcqueue_stride_test data verify; entry is timestamped\n");
    			rc++;
    		} else {
    			printf("lfpcqueue_stride_test data verify; entry is not timestamped\n");
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 640L)
				{
					printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (!SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("error lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	} else {
			printf("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) detected empty queue \n",
			       pcqueue, &handle5);

    	}
    } else {
		printf("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
			   x4k, x4k);
		return 10;
    }

    return rc;
}


int
lfpcqueue_stride_timestamp_test (char *x4k)
{
    int rc = 0;
    SPHSinglePCQueue_t pcqueue;
    block_size_t lfspace, lf0space, lfTemp;
    block_size_t aSize;
    sphtimer_t tstamp0, tstamp1, tstampd;
    SPHLFEntryHandle_t *handle, handle0, handle1, handle2, handle3;
    SPHLFEntryHandle_t *handlex, handle4, handle5;
    char *temp0, *temp1, *temp2, *temp3, *tempx;
    int cat, sub;

	aSize = 128;
	tstamp0 = 0LL;

    memset (x4k, 0, 4096);

    pcqueue = SPHSinglePCQueueInitWithStride (x4k, 1024, aSize, 0);
    if (pcqueue)
    {
		lf0space = SPHSinglePCQueueFreeSpace (pcqueue);
		lfspace = lf0space;

		printf("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lf0space);

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
        	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
        	if (handlex)
        	{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
    			       pcqueue, &handle5);
    			rc++;
        	} else {

        	}
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
			pcqueue);
			rc++;
		}

			if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			pcqueue);
			rc++;
		} else {
		}

		handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle0);
		if (handle)
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle0, handle);

			temp0 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp0"))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp0) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
	        	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	        	if (handlex)
	        	{
	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	    			       pcqueue, &handle5);
	    			tempx = (char*)SPHLFEntryGetFreePtr (handlex);
	    			if (temp0 == tempx)
	    			{} else {
		    			printf("error lfpcqueue_stride_timestamp_test SPHLFEntryGetFreePtr(%p) = %p != %p \n",
		    			       handlex, tempx, temp0);
		    			rc++;
	    			}
	        	} else {
	    			printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed none empty queue \n",
	    			       pcqueue, &handle5);
	    			rc++;
	        	}
			} else {
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize))
			{
				printf("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle0);
			return 10;
		}

        if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
			       pcqueue);
		}

        handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle1);
		if (handle)
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle1, handle);

			temp1 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp1"))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp1) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize))
			{
				printf("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle1);
			return 10;
		}

		handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle2);
		if (handle)
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle2, handle);

			temp2 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp2"))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp2) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			            pcqueue, lfspace, lfTemp, (aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle2);
			return 10;
		}

		handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle3);
		if (handle)
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
			       pcqueue, 1, 2, &handle3, handle);

			temp3 = (char*)SPHLFEntryGetFreePtr (handle);
			if (SPHLFEntryAddString (handle, (char*)"temp3"))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp3) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryIsComplete(handle))
			{
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			if (SPHSinglePCQueueEntryComplete(handle))
			{
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
				       handle);
				if (!SPHSinglePCQueueEntryIsComplete(handle))
				{
					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			           handle);
			    rc++;
			}

			lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
			if (lfspace != (lfTemp+aSize+aSize+aSize+aSize))
			{
				printf("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
			           pcqueue, lfspace, lfTemp, (aSize+aSize+aSize+aSize));
			    rc++;
			}
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
			       pcqueue, 1, 2, &handle3);
			return 10;
		}

		if (0 != strcmp(temp0, "temp0"))
		{
			printf("lfpcqueue_stride_timestamp_test data verify temp0=%s failed\n", temp0);
			return 10;
		}

		if (0 != strcmp(temp1, "temp1"))
		{
			printf("lfpcqueue_stride_timestamp_test data verify temp1=%s failed\n", temp1);
			return 10;
		}

		if (0 != strcmp(temp2, "temp2"))
		{
			printf("lfpcqueue_stride_timestamp_test data verify temp2=%s failed\n", temp2);
			return 10;
		}

		if (0 != strcmp(temp3, "temp3"))
		{
			printf("lfpcqueue_stride_timestamp_test data verify temp3=%s failed\n", temp3);
			return 10;
		}

		printf("lfpcqueue_stride_timestamp_test queue full tests\n");
		/* in its current state the queue is neither empty or full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		printf("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			x4k, pcqueue, lfspace);

		while (lfspace > 0)
		{
			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 1, &handle4);
			if (handle)
			{
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
				       pcqueue, 2, 1, &handle4, handle);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				printf("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
					x4k, pcqueue, lfspace);

				if (SPHSinglePCQueueEntryComplete(handle))
				{

				} else {
					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
				           handle);
				    rc++;
				}
			} else {
				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
				       pcqueue, aSize);
				return 10;
			}
		}

		/* in its current state the queue is not empty and is full */
		if (SPHSinglePCQueueEmpty (pcqueue))
		{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
			       pcqueue);
			rc++;
		} else {
		}

		if (SPHSinglePCQueueFull (pcqueue))
		{
		} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			       pcqueue);
			rc++;
		}

		lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		if (lfspace)
		{
			printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
				   x4k, pcqueue, lfspace);
			rc++;
		}

		printf("lfpcqueue_stride_timestamp_test dequeue tests\n");
    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
    		tempx = SPHLFEntryGetNextString (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d,string data=%s\n",
					cat,sub,tempx);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{   tstamp0 = SPHLFEntryTimeStamp(handlex);
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx\n",
    					tstamp0);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 1)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 1\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp0"))
    			{
    				printf("lfpcqueue_stride_timestamp_test data verify temp0=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		tempx = SPHLFEntryGetNextString (handlex);
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp1"))
    			{
    				printf("lfpcqueue_stride_timestamp_test data verify temp1=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		tempx = SPHLFEntryGetNextString (handlex);
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp2"))
    			{
    				printf("lfpcqueue_stride_timestamp_test data verify temp2=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
    		tempx = SPHLFEntryGetNextString (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d,string data=%s\n",
					cat,sub,tempx);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{   tstamp1 = SPHLFEntryTimeStamp(handlex);
    		    tstampd = tstamp1 - tstamp0;
			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 1)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 1\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (tempx)
    		{
    			if (0 != strcmp(tempx, "temp3"))
    			{
    				printf("lfpcqueue_stride_timestamp_test data verify temp3=%s failed\n", tempx);
    				rc++;
    			}
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        		tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 1)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 1\n");
    			rc++;
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        	    tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}
    		if (SPHSinglePCQueueFull (pcqueue))
    		{
    		} else {
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    			       pcqueue);
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    			handle = SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2, &handle4);
    			if (handle)
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
    				       pcqueue, 2, 2, &handle4, handle);
    				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
    				if (lfspace)
    				{
    					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
    						x4k, pcqueue, lfspace);
    					rc++;
    				}

    				if (SPHSinglePCQueueEntryComplete(handle))
    				{

    				} else {
    					printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
    				           handle);
    				    rc++;
    				}

    	    		if (SPHSinglePCQueueFull (pcqueue))
    	    		{
    	    		} else {
    	    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    	    			       pcqueue);
    	    			rc++;
    	    		}
    			} else {
    				printf("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
    						pcqueue, 2, 2, &handle4);
    				return 10;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))

    		{
    			tstamp1 = SPHLFEntryTimeStamp(handlex);
    		    tstampd = tstamp1 - tstamp0;
			    printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 128L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        		tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 256L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        		tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 384L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        		tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 512L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
			       pcqueue, &handle5);
    		cat = SPHLFEntryCategory (handlex);
    		sub = SPHLFEntrySubcat (handlex);
			printf("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
					cat,sub);
    		if (SPHLFEntryIsTimestamped(handlex))
    		{
        		tstamp1 = SPHLFEntryTimeStamp(handlex);
        		tstampd = tstamp1 - tstamp0;
    			printf("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
    					tstamp1, tstampd);
    		} else {
    			printf("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
    			rc++;
    		}
    		if (cat != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
    			rc++;
    		}
    		if (sub != 2)
    		{
    			printf("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
    			rc++;
    		}

    		if (SPHSinglePCQueueFreeNextEntry(pcqueue))
    		{
    			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
    			       pcqueue);
				lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
				if (lfspace != 640L)
				{
					printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
						x4k, pcqueue, lfspace);
					rc++;
				}

    			if (SPHSinglePCQueueFull (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}

    			if (!SPHSinglePCQueueEmpty (pcqueue))
    			{
    				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
    				       pcqueue);
    				rc++;
    			}
    		} else {
				printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
					   pcqueue);
				rc++;
    		}

    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	}

    	handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
    	if (handlex)
    	{
			printf("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
			       pcqueue, &handle5);
			rc++;
    	} else {
			printf("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) detected empty queue \n",
			       pcqueue, &handle5);

    	}
    } else {
		printf("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
			   x4k, x4k);
		return 10;
    }

    return rc;
}

/* Fake out the SAS region range checking for these tests.
*/
extern unsigned long memLow, memHigh;

void
setmemrange (unsigned long low, unsigned long high)
{
    unsigned long	ak, bk;

    memLow = low;
    memHigh = high;

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
    unsigned long	a4k, b4k;
    char        a[stack_buff_size], b[stack_buff_size];
    char *source_address, *dest_address;

    memset (a, 0, stack_buff_size);
    memset (b, 0, stack_buff_size);
    a4k = (unsigned long)(a+stack_block_mask) & ~stack_block_mask;
    b4k = (unsigned long)(b+stack_block_mask) & ~stack_block_mask;

    source_address=(char*)a4k;
    dest_address=(char*)b4k;

    printf ("source_address=%p, dest_address=%p\n",
            source_address, dest_address);
#if 1
    rc =+ lfpcqueue_basic_test(source_address);
#endif
#if 1
    rc =+ lfpcqueue_stride_test(source_address);
#endif
#if 1
    rc =+ lfpcqueue_stride_timestamp_test(source_address);
#endif
    return rc;
}
