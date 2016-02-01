/*
 * Copyright (c) 2011-2014 IBM Corporation.
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
#include <stdint.h>
#include <string.h>

#include "sphtimer.h"
#include "sasconf.h"
#include "sastype.h"
#include "sassim.h"
#include "sphlfentry.h"
#include "sphsinglepcqueue.h"
#include "sphdirectpcqueue.h"

int
lfpcqueue_stride_direct_test (char *x4k)
{
  int rc = 0;
  SPHSinglePCQueue_t pcqueue;
  block_size_t lfspace, lf0space, lfTemp;
  block_size_t aSize;;
  SPHLFEntryDirect_t handle;
  SPHLFEntryDirect_t handlex;
  char *temp0, *temp1, *temp2, *temp3, *tempx;
  uintptr_t tptr;
  int cat, sub;
  sphLFEntryID_t entry_tmp;

  aSize = 128;

  memset (x4k, 0, 4096);

  pcqueue = SPHSinglePCQueueInitWithStride (x4k, 1024, aSize, 0);
  if (pcqueue)
    {
      lf0space = SPHSinglePCQueueFreeSpace (pcqueue);
      lfspace = lf0space;
      entry_tmp = SPHSinglePCQueueGetEntryTemplate(pcqueue);

      printf
        ("lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
         x4k, pcqueue, lf0space);

      printf
        ("lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueGetEntryTemplate(%p) = %zu\n",
         x4k, pcqueue, (size_t)entry_tmp);

      if (SPHSinglePCQueueEmpty (pcqueue))
        {
          handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
          if (handlex)
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
                 pcqueue, handlex);
              rc++;
            }
          else
            {

            }
        }
      else
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                  pcqueue);
          rc++;
        }

      if (SPHSinglePCQueueFull (pcqueue))
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                  pcqueue);
          rc++;
        }
      else
        {
        }

      handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
      if (handle)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
             pcqueue, handle);

          temp0 = (char *) SPHLFEntryDirectGetFreePtr (handle);
          strcpy (temp0, "temp0");

          if (SPHSinglePCQueueGetNextCompleteDirect (handle))
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed)\n",
                 handle);
              rc++;
            }

          if (SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p)\n",
                 handle);
              if (!SPHLFEntryDirectIsComplete (handle))
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIsComplete(%p) failed)\n",
                     handle);
                  rc++;
                }
              handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
              if (handlex)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) = %p succeed \n",
                     pcqueue, handlex);
                  tempx = (char *) SPHLFEntryDirectGetFreePtr (handlex);
                  if (temp0 == tempx)
                    {
                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectGetFreePtr(%p) = %p != %p \n",
                         handlex, tempx, temp0);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed none empty queue \n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
          if (lfspace != (lfTemp + aSize))
            {
              printf
                ("error lfpcqueue_stride_direct_test(%p)  lfspace (%zu != (%zu+%zu))\n",
                 pcqueue, lfspace, lfTemp, (aSize));
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
             pcqueue);
          return 10;
        }

      if (SPHSinglePCQueueEmpty (pcqueue))
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                  pcqueue);
          rc++;
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
             pcqueue);
        }

      handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
      if (handle)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
             pcqueue, handle);

          temp1 = (char *) SPHLFEntryDirectGetFreePtr (handle);
          strcpy (temp1, "temp1");

          tptr = (uintptr_t)SPHLFEntryDirectGetPtrAligned (handle,
        		  	  	  	  (sizeof(double)));
          if(tptr & (sizeof(double) -1))
          {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectGetPtrAligned(%p, %zu) failed)\n",
                		handle, (sizeof(double)));
              rc++;
          } else {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectGetPtrAligned(%p, %zu)) = %p\n",
                		handle, (sizeof(double)), (void*)tptr);
          }

          if (SPHLFEntryDirectIsComplete (handle))
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIsComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          if (SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p)\n",
                 handle);
              if (!SPHLFEntryDirectIsComplete (handle))
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIsComplete(%p) failed)\n",
                     handle);
                  rc++;
                }
            }
          else
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
          if (lfspace != (lfTemp + aSize + aSize))
            {
              printf
                ("error lfpcqueue_stride_direct_test(%p)  lfspace (%zu != (%zu+%zu))\n",
                 pcqueue, lfspace, lfTemp, (aSize + aSize));
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
             pcqueue);
          return 10;
        }

      handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
      if (handle)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
             pcqueue, handle);

          temp2 = (char *) SPHLFEntryDirectGetFreePtr (handle);
          strcpy (temp2, "temp2");

          tptr = (uintptr_t)SPHLFEntryDirectIncAndAlign (temp2,
        		  	  	  	  (strlen(temp2)+1), (sizeof(double)));
          if(tptr & (sizeof(double) -1))
          {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIncAndAlign(%p, %zu, %zu) failed)\n",
                		temp2, (strlen(temp2)+1), (sizeof(double)));
              rc++;
          } else {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectIncAndAlign(%p, %zu, %zu)) = %p\n",
                		temp2, (strlen(temp2)+1), (sizeof(double)), (void*)tptr);
          }

          if (SPHSinglePCQueueEntryIsCompleteDirect (handle))
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHSinglePCQueueEntryIsCompleteDirect(%p) failed)\n",
                 handle);
              rc++;
            }

          if (SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p)\n",
                 handle);
              if (!SPHSinglePCQueueEntryIsCompleteDirect (handle))
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHSinglePCQueueEntryIsCompleteDirect(%p) failed)\n",
                     handle);
                  rc++;
                }
            }
          else
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
          if (lfspace != (lfTemp + aSize + aSize + aSize))
            {
              printf
                ("error lfpcqueue_stride_direct_test(%p)  lfspace (%zu != (%zu+%zu))\n",
                 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize));
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
             pcqueue);
          return 10;
        }

      handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
      if (handle)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
             pcqueue, handle);

          temp3 = (char *) SPHLFEntryDirectGetFreePtr (handle);
          strcpy (temp3, "temp3");

          if (SPHLFEntryDirectIsComplete (handle))
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIsComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          if (SPHLFEntryDirectComplete (handle, entry_tmp, 1, 2))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p)\n",
                 handle);
              if (!SPHLFEntryDirectIsComplete (handle))
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHLFEntryDirectIsComplete(%p) failed)\n",
                     handle);
                  rc++;
                }
            }
          else
            {
              printf
                ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                 handle);
              rc++;
            }

          lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
          if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
            {
              printf
                ("error lfpcqueue_stride_direct_test(%p)  lfspace (%zu != (%zu+%zu))\n",
                 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
             pcqueue);
          return 10;
        }

      if (0 != strcmp (temp0, "temp0"))
        {
          printf ("lfpcqueue_stride_direct_test data verify temp0=%s failed\n",
                  temp0);
          return 10;
        }

      if (0 != strcmp (temp1, "temp1"))
        {
          printf ("lfpcqueue_stride_direct_test data verify temp1=%s failed\n",
                  temp1);
          return 10;
        }

      if (0 != strcmp (temp2, "temp2"))
        {
          printf ("lfpcqueue_stride_direct_test data verify temp2=%s failed\n",
                  temp2);
          return 10;
        }

      if (0 != strcmp (temp3, "temp3"))
        {
          printf ("lfpcqueue_stride_direct_test data verify temp3=%s failed\n",
                  temp3);
          return 10;
        }

      printf ("lfpcqueue_stride_direct_test queue full tests\n");
      /* in its current state the queue is neither empty or full */
      if (SPHSinglePCQueueEmpty (pcqueue))
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                  pcqueue);
          rc++;
        }
      else
        {
        }

      if (SPHSinglePCQueueFull (pcqueue))
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                  pcqueue);
          rc++;
        }
      else
        {
        }

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      printf
        ("lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
         x4k, pcqueue, lfspace);

      while (lfspace > 0)
        {
          handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
          if (handle)
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                 pcqueue, handle);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              printf
                ("lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                 x4k, pcqueue, lfspace);

              if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 1))
                {

                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                     handle);
                  rc++;
                }
            }
          else
            {
              printf
                ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                 pcqueue);
              return 10;
            }
        }

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                  pcqueue);
          rc++;
        }
      else
        {
        }

      if (SPHSinglePCQueueFull (pcqueue))
        {
        }
      else
        {
          printf ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                  pcqueue);
          rc++;
        }

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      if (lfspace)
        {
          printf
            ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
             x4k, pcqueue, lfspace);
          rc++;
        }

      printf ("lfpcqueue_stride_direct_test dequeue tests\n");
      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) = %p succeed \n",
             pcqueue, handle);
          tempx = SPHLFEntryDirectGetFreePtr (handlex);
          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf
            ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d,string data=%s\n",
             cat, sub, tempx);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 1)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 1\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }
          if (tempx)
            {
              if (0 != strcmp (tempx, "temp0"))
                {
                  printf
                    ("lfpcqueue_stride_direct_test data verify temp0=%s failed\n",
                     tempx);
                  rc++;
                }
            }
          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p) succeed \n",
                 pcqueue);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) = %p succeed \n",
             pcqueue, handlex);
          tempx = SPHLFEntryDirectGetFreePtr (handlex);
          if (tempx)
            {
              if (0 != strcmp (tempx, "temp1"))
                {
                  printf
                    ("lfpcqueue_stride_direct_test data verify temp1=%s failed\n",
                     tempx);
                  rc++;
                }
            }
          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p) succeed \n",
                 pcqueue);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle = SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) = %p succeed \n",
             pcqueue, handlex);
          tempx = SPHLFEntryDirectGetFreePtr (handlex);
          if (tempx)
            {
              if (0 != strcmp (tempx, "temp2"))
                {
                  printf
                    ("lfpcqueue_stride_direct_test data verify temp2=%s failed\n",
                     tempx);
                  rc++;
                }
            }
          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle =
                  SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntry(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) = %p succeed \n",
             pcqueue, handlex);
          tempx = SPHLFEntryDirectGetFreePtr (handlex);
          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf
            ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d,string data=%s\n",
             cat, sub, tempx);
#if 1
          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }
#endif
          if (cat != 1)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 1\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }
          if (tempx)
            {
              if (0 != strcmp (tempx, "temp3"))
                {
                  printf
                    ("lfpcqueue_stride_direct_test data verify temp3=%s failed\n",
                     tempx);
                  rc++;
                }
            }
          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle =
                  SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) %p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 1)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 1\n");
              rc++;
            }

          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntry(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle =
                  SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueAllocStrideDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFull (pcqueue))
            {
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                 pcqueue);
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntry(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }
              handle =
                  SPHSinglePCQueueAllocStrideDirect (pcqueue);
              if (handle)
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueAllocStrideDirect(%p) = %p\n",
                     pcqueue, handle);
                  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
                  if (lfspace)
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                         x4k, pcqueue, lfspace);
                      rc++;
                    }

                  if (SPHLFEntryDirectComplete (handle, entry_tmp, 2, 2))
                    {

                    }
                  else
                    {
                      printf
                        ("error lfpcqueue_stride_direct_test SPHLFEntryDirectComplete(%p) failed)\n",
                         handle);
                      rc++;
                    }

                  if (SPHSinglePCQueueFull (pcqueue))
                    {
                    }
                  else
                    {
                      printf
                        ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                         pcqueue);
                      rc++;
                    }
                }
              else
                {
                  printf
                    ("error lfpcqueue_stride_direct_test  SPHSinglePCQueueGetNextCompleteDirect(%p) failed\n",
                     pcqueue);
                  return 10;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 128L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }

              if (SPHSinglePCQueueEmpty (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }

        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 256L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }

              if (SPHSinglePCQueueEmpty (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }

        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextComplete(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntry(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 384L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }

              if (SPHSinglePCQueueEmpty (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }

        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextComplete(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 512L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }

              if (SPHSinglePCQueueEmpty (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }

        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextComplete(%p) =%p succeed \n",
             pcqueue, handlex);

          cat = SPHLFEntryDirectCategory (handlex);
          sub = SPHLFEntryDirectSubcat (handlex);
          printf ("lfpcqueue_stride_direct_test data verify; entry cat=%d,sub=%d\n",
                  cat, sub);

          if (SPHLFEntryDirectIsTimestamped (handlex))
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; entry is timestamped\n");
              rc++;
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test data verify; entry is not timestamped\n");
            }

          if (cat != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; cat should be 2\n");
              rc++;
            }
          if (sub != 2)
            {
              printf
                ("error lfpcqueue_stride_direct_test data verify; subcat should be 2\n");
              rc++;
            }

          if (SPHSinglePCQueueFreeNextEntryDirect (pcqueue, handlex))
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) succeed \n",
                 pcqueue, handlex);
              lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
              if (lfspace != 640L)
                {
                  printf
                    ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
                     x4k, pcqueue, lfspace);
                  rc++;
                }

              if (SPHSinglePCQueueFull (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueFull(%p) failed\n",
                     pcqueue);
                  rc++;
                }

              if (!SPHSinglePCQueueEmpty (pcqueue))
                {
                  printf
                    ("lfpcqueue_stride_direct_test SPHSinglePCQueueEmpty(%p) failed\n",
                     pcqueue);
                  rc++;
                }
            }
          else
            {
              printf
                ("lfpcqueue_stride_direct_test SPHSinglePCQueueFreeNextEntryDirect(%p,%p) failed\n",
                 pcqueue, handlex);
              rc++;
            }

        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) failed for empty queue \n",
             pcqueue);
          rc++;
        }

      handlex = SPHSinglePCQueueGetNextCompleteDirect (pcqueue);
      if (handlex)
        {
          printf
            ("error lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) =%p failed for empty queue \n",
             pcqueue, handlex);
          rc++;
        }
      else
        {
          printf
            ("lfpcqueue_stride_direct_test SPHSinglePCQueueGetNextCompleteDirect(%p) detected empty queue \n",
             pcqueue);

        }
    }
  else
    {
      printf
        ("error lfpcqueue_stride_direct_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
         x4k, x4k);
      return 10;
    }

  return (rc);
}

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

      printf
	("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lf0space);

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      temp0 = (char *) SPHSinglePCQueueAllocRaw (pcqueue);
      if (temp0)
	{
	  printf
	    ("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
	     pcqueue, aSize, temp0);
	  strcpy (temp0, "temp0");
	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
	     pcqueue, aSize);
	  return 10;
	}

      temp1 = (char *) SPHSinglePCQueueAllocRaw (pcqueue);
      if (temp1)
	{
	  printf
	    ("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
	     pcqueue, aSize, temp1);
	  strcpy (temp1, "temp1");
	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
	     pcqueue, aSize);
	  return 10;
	}

      temp2 = (char *) SPHSinglePCQueueAllocRaw (pcqueue);
      if (temp2)
	{
	  printf
	    ("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
	     pcqueue, aSize, temp2);
	  strcpy (temp2, "temp2");
	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
	     pcqueue, aSize);
	  return 10;
	}

      temp3 = (char *) SPHSinglePCQueueAllocRaw (pcqueue);
      if (temp3)
	{
	  printf
	    ("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
	     pcqueue, aSize, temp3);
	  strcpy (temp3, "temp3");
	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_basic_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
	     pcqueue, aSize);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf ("lfpcqueue_basic_test data verify temp0=%s failed\n",
		  temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf ("lfpcqueue_basic_test data verify temp1=%s failed\n",
		  temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf ("lfpcqueue_basic_test data verify temp2=%s failed\n",
		  temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf ("lfpcqueue_basic_test data verify temp3=%s failed\n",
		  temp3);
	  return 10;
	}

      printf ("lfpcqueue_basic_test data\n");
      /* in its current state the queue is neither empty or full */
      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      printf
	("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lfspace);

      while (lfspace > 0)
	{
	  temp4 = (char *) SPHSinglePCQueueAllocRaw (pcqueue);
	  if (temp4)
	    {
	      printf
		("lfpcqueue_basic_test SPHSinglePCQueueAllocRaw(%p, %zu) = %p\n",
		 pcqueue, aSize, temp4);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      printf
		("lfpcqueue_basic_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		 x4k, pcqueue, lfspace);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_basic_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
		 pcqueue, aSize);
	      return 10;
	    }
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("lfpcqueue_basic_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}
    }
  else
    {
      printf
	("error lfpcqueue_basic_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
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

      printf
	("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lf0space);

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	  if (handlex)
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
		 pcqueue, &handle5);
	      rc++;
	    }
	  else
	    {

	    }
	}
      else
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rc++;
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle0, handle);

	  temp0 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp0"))
	    {
	      printf
		("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp0) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	      if (handlex)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
		     pcqueue, &handle5);
		  tempx = (char *) SPHLFEntryGetFreePtr (handlex);
		  if (temp0 == tempx)
		    {
		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHLFEntryGetFreePtr(%p) = %p != %p \n",
			 handlex, tempx, temp0);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed none empty queue \n",
		     pcqueue, &handle5);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle0);
	  return 10;
	}

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
	     pcqueue);
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle1);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle1, handle);

	  temp1 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp1"))
	    {
	      printf
		("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp1) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle1);
	  return 10;
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle2);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle2, handle);

	  temp2 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp2"))
	    {
	      printf
		("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp2) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle2);
	  return 10;
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle3);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle3, handle);

	  temp3 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp3"))
	    {
	      printf
		("error lfpcqueue_stride_test SPHLFEntryAddString(%p, temp3) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle3);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf ("lfpcqueue_stride_test data verify temp0=%s failed\n",
		  temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf ("lfpcqueue_stride_test data verify temp1=%s failed\n",
		  temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf ("lfpcqueue_stride_test data verify temp2=%s failed\n",
		  temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf ("lfpcqueue_stride_test data verify temp3=%s failed\n",
		  temp3);
	  return 10;
	}

      printf ("lfpcqueue_stride_test queue full tests\n");
      /* in its current state the queue is neither empty or full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      printf
	("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lfspace);

      while (lfspace > 0)
	{
	  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 1, &handle4);
	  if (handle)
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		 pcqueue, 2, 1, &handle4, handle);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      printf
		("lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		 x4k, pcqueue, lfspace);

	      if (SPHSinglePCQueueEntryComplete (handle))
		{

		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
		 pcqueue, aSize);
	      return 10;
	    }
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		  pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		  pcqueue);
	  rc++;
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      if (lfspace)
	{
	  printf
	    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	     x4k, pcqueue, lfspace);
	  rc++;
	}

      printf ("lfpcqueue_stride_test dequeue tests\n");
      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 1)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp0"))
		{
		  printf
		    ("lfpcqueue_stride_test data verify temp0=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp1"))
		{
		  printf
		    ("lfpcqueue_stride_test data verify temp1=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp2"))
		{
		  printf
		    ("lfpcqueue_stride_test data verify temp2=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 1)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp3"))
		{
		  printf
		    ("lfpcqueue_stride_test data verify temp3=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 1)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 1\n");
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 256L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 384L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 512L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf ("lfpcqueue_stride_test data verify; entry cat=%d,sub=%d\n",
		  cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error lfpcqueue_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 640L)
		{
		  printf
		    ("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (!SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("error lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) detected empty queue \n",
	     pcqueue, &handle5);

	}
    }
  else
    {
      printf
	("error lfpcqueue_stride_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
	 x4k, x4k);
      return 10;
    }

  return rc;
}

static int
check_freespace_align (SPHSinglePCQueue_t pcqueue, block_size_t fspace,
		       block_size_t stride)
{
  block_size_t lfTemp;

  lfTemp = fspace / stride;
  lfTemp = lfTemp * stride;
  if (fspace != lfTemp)
    {
      printf
	("error lfpcqueue_tests  SPHSinglePCQueueFreeSpace(%p) = %zu should be %zu\n",
	 pcqueue, fspace, lfTemp);
      return 1;
    }
  else
    return 0;
}

int
lfpcqueue_large_stride_test (char *x64k, block_size_t bSize,
			     block_size_t aSize)
{
  int rc = 0;
  SPHSinglePCQueue_t pcqueue;
  block_size_t lfspace, lf0space, lfTemp, exspace;
  SPHLFEntryHandle_t *handle, handle0, handle1, handle2, handle3;
  SPHLFEntryHandle_t *handlex, handle4, handle5;
  char *temp0, *temp1, *temp2, *temp3, *tempx;
  long int extra;
  int cat, sub;

  memset (x64k, 0, 64 * 1024);

  pcqueue = SPHSinglePCQueueInitWithStride (x64k, bSize, aSize, 0);
  if (pcqueue)
    {
      lf0space = SPHSinglePCQueueFreeSpace (pcqueue);
      lfspace = lf0space;

      printf
	("\n\nlfpcqueue_large_stride_test(%p, %zu, %zu)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x64k, bSize, aSize, pcqueue, lf0space);

      if (check_freespace_align (pcqueue, lf0space, aSize))
	{
	  return 100;
	}

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	  if (handlex)
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
		 pcqueue, &handle5);
	      rc++;
	    }
	  else
	    {

	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle0);
      if (handle)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle0, handle);

	  temp0 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp0"))
	    {
	      printf
		("error 1 lfpcqueue_large_stride_test SPHLFEntryAddString(%p, temp0) failed)\n",
		 handle);
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test SPHLFEntryAddString(%p) = %p)\n",
		 handle, temp0);
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error 2 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error 3 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	      if (handlex)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
		     pcqueue, &handle5);
		  tempx = (char *) SPHLFEntryGetFreePtr (handlex);
		  if (temp0 == tempx)
		    {
		    }
		  else
		    {
		      printf
			("error 4 lfpcqueue_large_stride_test SPHLFEntryGetFreePtr(%p) = %p != %p \n",
			 handlex, tempx, temp0);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 5 lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed none empty queue \n",
		     pcqueue, &handle5);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 6 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error 7 lfpcqueue_large_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 7a lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle0);
	  return 10;
	}

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("error 7b lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
	     pcqueue);
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle1);
      if (handle)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle1, handle);

	  temp1 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp1"))
	    {
	      printf
		("error 8 lfpcqueue_large_stride_test SPHLFEntryAddString(%p, temp1) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error 9 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error 10 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 11 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error 12 lfpcqueue_large_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 12a lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle1);
	  return 10;
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle2);
      if (handle)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle2, handle);

	  temp2 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp2"))
	    {
	      printf
		("error 13 lfpcqueue_large_stride_test SPHLFEntryAddString(%p, temp2) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error 14 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error 15 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 16 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error 17 lfpcqueue_large_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 17a lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle2);
	  return 10;
	}

      handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 1, 2, &handle3);
      if (handle)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle3, handle);

	  temp3 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp3"))
	    {
	      printf
		("error 18 lfpcqueue_large_stride_test SPHLFEntryAddString(%p, temp3) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error 19 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error 20 lfpcqueue_large_stride_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 21 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error 22 lfpcqueue_large_stride_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 22a lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle3);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf
	    ("error 22b lfpcqueue_large_stride_test data verify temp0=%s failed\n",
	     temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf
	    ("error 22c lfpcqueue_large_stride_test data verify temp1=%s failed\n",
	     temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf
	    ("error 22d lfpcqueue_large_stride_test data verify temp2=%s failed\n",
	     temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf
	    ("error 22e lfpcqueue_large_stride_test data verify temp3=%s failed\n",
	     temp3);
	  return 10;
	}

      printf ("lfpcqueue_large_stride_test queue full tests\n");
      /* in its current state the queue is neither empty or full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("error 22f lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf
	    ("error 22g lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      printf
	("lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x64k, pcqueue, lfspace);

      extra = 0;
      exspace = 0;
      while (lfspace > 0)
	{
	  handle = SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 1, &handle4);
	  if (handle)
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		 pcqueue, 2, 1, &handle4, handle);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      printf
		("lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		 x64k, pcqueue, lfspace);

	      if (SPHSinglePCQueueEntryComplete (handle))
		{
		  extra++;
		  exspace += aSize;
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) extra %ld, %zu)\n",
		     handle, extra, exspace);
		}
	      else
		{
		  printf
		    ("error 23 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 24 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
		 pcqueue, aSize);
	      return 10;
	    }
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("error 24a lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf
	    ("error 24b lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      if (lfspace)
	{
	  printf
	    ("error 25 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	     x64k, pcqueue, lfspace);
	  rc++;
	}

      printf ("lfpcqueue_large_stride_test dequeue tests\n");
      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 26 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 1)
	    {
	      printf
		("error 27 lfpcqueue_large_stride_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 28 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp0"))
		{
		  printf
		    ("error 28a lfpcqueue_large_stride_test data verify temp0=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("error 28b lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != aSize)
		{
		  printf
		    ("error 28c lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 28d lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error 29 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 30 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("error 30a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 31 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 31a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 31b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp1"))
		{
		  printf
		    ("error 31c lfpcqueue_large_stride_test data verify temp1=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("error 31d lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != aSize)
		{
		  printf
		    ("error 32 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 32a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error 33 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 34 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("error 34a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 35 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 35a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 35b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp2"))
		{
		  printf
		    ("error 35c lfpcqueue_large_stride_test data verify temp2=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("error 35d lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != aSize)
		{
		  printf
		    ("error 36 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 36a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error 37 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 38 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("error 38a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 39 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 39a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 39b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 40 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 1)
	    {
	      printf
		("error 41 lfpcqueue_large_stride_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 42 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp3"))
		{
		  printf
		    ("error 42a lfpcqueue_large_stride_test data verify temp3=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("error 42b lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != aSize)
		{
		  printf
		    ("error 43 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 43a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error 44 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 45 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("error 45a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 46 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 46a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 46b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 50 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 51 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 1)
	    {
	      printf
		("error 52 lfpcqueue_large_stride_test data verify; subcat should be 1\n");
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("error 52a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != aSize)
		{
		  printf
		    ("error 53 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 53a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error 54 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 55 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("error 55a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error 56 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 56a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 56b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      while (extra)
	{
	  handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	  if (handlex)
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) extra %ld\n",
		 pcqueue, &handle5, extra);
	      cat = SPHLFEntryCategory (handlex);
	      sub = SPHLFEntrySubcat (handlex);
	      printf
		("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
		 cat, sub);
	      if (SPHSinglePCQueueFreeNextEntry (pcqueue))
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		     pcqueue);
		}
	      extra--;
	    }

	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 60 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 61 lfpcqueue_large_stride_test data verify; cat is %d should be 2\n",
		 cat);
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 62 lfpcqueue_large_stride_test data verify; subcat is %d should be 2\n",
		 sub);
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      if (exspace)
		{
		}
	      else
		{
		  printf
		    ("error 62a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (aSize + exspace))
		{
		  printf
		    ("error 63 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 63a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideEntry (pcqueue, 2, 2, &handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_large_stride_test SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace != (exspace))
		    {
		      printf
			("error 64 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x64k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error 65 lfpcqueue_large_stride_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      if (exspace)
			{
			}
		      else
			{
			  printf
			    ("error 65a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
			     pcqueue);
			  rc++;
			}
		    }
		}
	      else
		{
		  printf
		    ("error 66 lfpcqueue_large_stride_test  SPHSinglePCQueueAllocStrideEntry(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("error 66a lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("error 66b lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 70 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 71 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 72 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (aSize + exspace))
		{
		  printf
		    ("error 73 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 73a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("error 73b lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 73c lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("error 73d lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 80 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 81 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 82 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (aSize + aSize + exspace))
		{
		  printf
		    ("error 83 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 83a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("error 83b lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 83c lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("error 83d lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 84 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 85 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 86 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (3 * aSize + exspace))
		{
		  printf
		    ("error 87 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 87a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("error 87b lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 87c lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("error 87d lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 90 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 91 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 92 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (4 * aSize + exspace))
		{
		  printf
		    ("error 93 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 93a lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  if (exspace)
		    {
		    }
		  else
		    {
		      printf
			("error 93b lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	    }
	  else
	    {
	      printf
		("error 93c lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("error 93d lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_large_stride_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      printf
		("error 95 lfpcqueue_large_stride_test data verify; entry is timestamped\n");
	      rc++;
	    }
	  else
	    {
	      printf
		("lfpcqueue_large_stride_test data verify; entry is not timestamped\n");
	    }
	  if (cat != 2)
	    {
	      printf
		("error 96 lfpcqueue_large_stride_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error 97 lfpcqueue_large_stride_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != (5 * aSize))
		{
		  printf
		    ("error 98 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x64k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("error 99 lfpcqueue_large_stride_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (!SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("error 99a lfpcqueue_large_stride_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error 99b lfpcqueue_large_stride_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  if (exspace)
	    {
	    }
	  else
	    {
	      printf
		("error 99c lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
		 pcqueue, &handle5);
	      rc++;
	    }
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("error 99d lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_large_stride_test SPHSinglePCQueueGetNextComplete(%p,%p) detected empty queue \n",
	     pcqueue, &handle5);

	}
    }
  else
    {
      printf
	("error 100 lfpcqueue_large_stride_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
	 x64k, x64k);
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

      printf
	("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lf0space);

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	  if (handlex)
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
		 pcqueue, &handle5);
	      rc++;
	    }
	  else
	    {

	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      handle =
	SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle0);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle0, handle);

	  temp0 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp0"))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp0) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
	      if (handlex)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
		     pcqueue, &handle5);
		  tempx = (char *) SPHLFEntryGetFreePtr (handlex);
		  if (temp0 == tempx)
		    {
		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHLFEntryGetFreePtr(%p) = %p != %p \n",
			 handlex, tempx, temp0);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed none empty queue \n",
		     pcqueue, &handle5);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle0);
	  return 10;
	}

      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) false, succeeds\n",
	     pcqueue);
	}

      handle =
	SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle1);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle1, handle);

	  temp1 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp1"))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp1) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle1);
	  return 10;
	}

      handle =
	SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle2);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle2, handle);

	  temp2 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp2"))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp2) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle2);
	  return 10;
	}

      handle =
	SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 1, 2, &handle3);
      if (handle)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
	     pcqueue, 1, 2, &handle3, handle);

	  temp3 = (char *) SPHLFEntryGetFreePtr (handle);
	  if (SPHLFEntryAddString (handle, (char *) "temp3"))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHLFEntryAddString(%p, temp3) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryIsComplete (handle))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  if (SPHSinglePCQueueEntryComplete (handle))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p)\n",
		 handle);
	      if (!SPHSinglePCQueueEntryIsComplete (handle))
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryIsComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		 handle);
	      rc++;
	    }

	  lfTemp = SPHSinglePCQueueFreeSpace (pcqueue);
	  if (lfspace != (lfTemp + aSize + aSize + aSize + aSize))
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test(%p)  lfspace (%zu != (%zu+%zu))\n",
		 pcqueue, lfspace, lfTemp, (aSize + aSize + aSize + aSize));
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
	     pcqueue, 1, 2, &handle3);
	  return 10;
	}

      if (0 != strcmp (temp0, "temp0"))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify temp0=%s failed\n",
	     temp0);
	  return 10;
	}

      if (0 != strcmp (temp1, "temp1"))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify temp1=%s failed\n",
	     temp1);
	  return 10;
	}

      if (0 != strcmp (temp2, "temp2"))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify temp2=%s failed\n",
	     temp2);
	  return 10;
	}

      if (0 != strcmp (temp3, "temp3"))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify temp3=%s failed\n",
	     temp3);
	  return 10;
	}

      printf ("lfpcqueue_stride_timestamp_test queue full tests\n");
      /* in its current state the queue is neither empty or full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      printf
	("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	 x4k, pcqueue, lfspace);

      while (lfspace > 0)
	{
	  handle =
	    SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 1, &handle4);
	  if (handle)
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		 pcqueue, 2, 1, &handle4, handle);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      printf
		("lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		 x4k, pcqueue, lfspace);

	      if (SPHSinglePCQueueEntryComplete (handle))
		{

		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
		     handle);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocRaw(%p,%zu) failed\n",
		 pcqueue, aSize);
	      return 10;
	    }
	}

      /* in its current state the queue is not empty and is full */
      if (SPHSinglePCQueueEmpty (pcqueue))
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
	     pcqueue);
	  rc++;
	}
      else
	{
	}

      if (SPHSinglePCQueueFull (pcqueue))
	{
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
	     pcqueue);
	  rc++;
	}

      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
      if (lfspace)
	{
	  printf
	    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
	     x4k, pcqueue, lfspace);
	  rc++;
	}

      printf ("lfpcqueue_stride_timestamp_test dequeue tests\n");
      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp0 = SPHLFEntryTimeStamp (handlex);
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx\n",
		 tstamp0);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 1)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp0"))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test data verify temp0=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp1"))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test data verify temp1=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  tempx = SPHLFEntryGetNextString (handlex);
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp2"))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test data verify temp2=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  tempx = SPHLFEntryGetNextString (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d,string data=%s\n",
	     cat, sub, tempx);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 1)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 1\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (tempx)
	    {
	      if (0 != strcmp (tempx, "temp3"))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test data verify temp3=%s failed\n",
		     tempx);
		  rc++;
		}
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 1)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 1\n");
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }
	  if (SPHSinglePCQueueFull (pcqueue))
	    {
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	      handle =
		SPHSinglePCQueueAllocStrideTimeStamped (pcqueue, 2, 2,
							&handle4);
	      if (handle)
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) = %p\n",
		     pcqueue, 2, 2, &handle4, handle);
		  lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
		  if (lfspace)
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
			 x4k, pcqueue, lfspace);
		      rc++;
		    }

		  if (SPHSinglePCQueueEntryComplete (handle))
		    {

		    }
		  else
		    {
		      printf
			("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueEntryComplete(%p) failed)\n",
			 handle);
		      rc++;
		    }

		  if (SPHSinglePCQueueFull (pcqueue))
		    {
		    }
		  else
		    {
		      printf
			("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
			 pcqueue);
		      rc++;
		    }
		}
	      else
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test  SPHSinglePCQueueAllocStrideTimeStamped(%p, %d, %d, %p) failed\n",
		     pcqueue, 2, 2, &handle4);
		  return 10;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))

	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 128L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 256L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 384L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 512L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) succeed \n",
	     pcqueue, &handle5);
	  cat = SPHLFEntryCategory (handlex);
	  sub = SPHLFEntrySubcat (handlex);
	  printf
	    ("lfpcqueue_stride_timestamp_test data verify; entry cat=%d,sub=%d\n",
	     cat, sub);
	  if (SPHLFEntryIsTimestamped (handlex))
	    {
	      tstamp1 = SPHLFEntryTimeStamp (handlex);
	      tstampd = tstamp1 - tstamp0;
	      printf
		("lfpcqueue_stride_timestamp_test data verify; entry is timestamped %llx, %llx\n",
		 tstamp1, tstampd);
	    }
	  else
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; entry is not timestamped\n");
	      rc++;
	    }
	  if (cat != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; cat should be 2\n");
	      rc++;
	    }
	  if (sub != 2)
	    {
	      printf
		("error lfpcqueue_stride_timestamp_test data verify; subcat should be 2\n");
	      rc++;
	    }

	  if (SPHSinglePCQueueFreeNextEntry (pcqueue))
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) succeed \n",
		 pcqueue);
	      lfspace = SPHSinglePCQueueFreeSpace (pcqueue);
	      if (lfspace != 640L)
		{
		  printf
		    ("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueFreeSpace(%p) = %zu\n",
		     x4k, pcqueue, lfspace);
		  rc++;
		}

	      if (SPHSinglePCQueueFull (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFull(%p) failed\n",
		     pcqueue);
		  rc++;
		}

	      if (!SPHSinglePCQueueEmpty (pcqueue))
		{
		  printf
		    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueEmpty(%p) failed\n",
		     pcqueue);
		  rc++;
		}
	    }
	  else
	    {
	      printf
		("lfpcqueue_stride_timestamp_test SPHSinglePCQueueFreeNextEntry(%p) failed\n",
		 pcqueue);
	      rc++;
	    }

	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}

      handlex = SPHSinglePCQueueGetNextComplete (pcqueue, &handle5);
      if (handlex)
	{
	  printf
	    ("error lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) failed for empty queue \n",
	     pcqueue, &handle5);
	  rc++;
	}
      else
	{
	  printf
	    ("lfpcqueue_stride_timestamp_test SPHSinglePCQueueGetNextComplete(%p,%p) detected empty queue \n",
	     pcqueue, &handle5);

	}
    }
  else
    {
      printf
	("error lfpcqueue_stride_timestamp_test(%p)  SPHSinglePCQueueInitWithStride(%p) failed\n",
	 x4k, x4k);
      return 10;
    }

  return rc;
}

/* Fake out the SAS region range checking for these tests.
*/

void
setmemrange (unsigned long low, unsigned long high)
{
  unsigned long ak, bk;

  setSASmemrange (low, high);

  ak = getMemLow ();
  bk = getMemHigh ();
  printf ("getMemLow()=%lx, getMemHigh()=%lx\n", ak, bk);
}

#define stack_block_size 65536
#define stack_block_mask (stack_block_size - 1)
#define stack_buff_size (stack_block_size + stack_block_size)

int
main (int argc, char *argv[])
{
  int rc = 0;
  unsigned long a4k, b4k;
  char a[stack_buff_size], b[stack_buff_size];
  char *source_address, *dest_address;

  memset (a, 0, stack_buff_size);
  memset (b, 0, stack_buff_size);
  a4k = (unsigned long) (a + stack_block_mask) & ~stack_block_mask;
  b4k = (unsigned long) (b + stack_block_mask) & ~stack_block_mask;

  source_address = (char *) a4k;
  dest_address = (char *) b4k;

  printf ("source_address=%p, dest_address=%p\n",
	  source_address, dest_address);
#if 1
  rc += lfpcqueue_basic_test (source_address);
#endif
#if 1
  rc += lfpcqueue_stride_test (source_address);
#endif
#if 1
  rc += lfpcqueue_stride_direct_test (source_address);
#endif
#if 1
  rc += lfpcqueue_stride_timestamp_test (source_address);
#endif
#if 1
  rc += lfpcqueue_large_stride_test (source_address, 1024, 128);
#endif
#if 1
  rc += lfpcqueue_large_stride_test (source_address, 2 * 1024, 256);
#endif
#if 1
  rc += lfpcqueue_large_stride_test (source_address, 8 * 1024, 1024);
#endif
#if 1
  rc += lfpcqueue_large_stride_test (source_address, 16 * 1024, 2048);
#endif
  return (rc);
}
