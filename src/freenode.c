/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe      - initial API and implementation
 *     IBM Corporation, Adhemerval Zanella - C code conversion
 */

// utility class for managing a heap
#include <stdlib.h>

#include "freenode.h"

// internal functions
static void freeNode_insertNode (FreeNode *freeNode, FreeNode *head, node_size_t size);
static FreeNode* freeNode_allocNode(FreeNode *freeNode, FreeNode **head, node_size_t size);

void freeNode_init (FreeNode *freeNode, node_size_t size)
{
  freeNode->nextNode = 0;
  freeNode->nodeSize = (node_size_t)size;
}

node_size_t freeNode_freeFragmentsTotal (FreeNode *freeNode)
{  
   node_size_t freeNodes = 1;
   FreeNode *head = freeNode->nextNode;
   while (head) {
     freeNodes++;
     head = head->nextNode;
   };
   return freeNodes;
};

node_size_t freeNode_maxFragment (FreeNode *freeNode)
{  
   node_size_t freeSize = freeNode->nodeSize;
   FreeNode *head = freeNode->nextNode;
   while (head) {
      if (freeSize < head->nodeSize)
          freeSize = head->nodeSize;
          
      head = head->nextNode;
   };
   return freeSize;
};

node_size_t freeNode_freeSpaceTotal (FreeNode *freeNode)
{  
   node_size_t freeSpace = freeNode->nodeSize;
   FreeNode *head = freeNode->nextNode;
   while (head) {
      freeSpace += head->nodeSize;
      head = head->nextNode;
   };
   return freeSpace;
};

static
void freeNode_insertNode (FreeNode *freeNode, FreeNode *head, node_size_t size)
{
   FreeNode *temp = head->nextNode;
   while (head)
   {
      if (((unsigned long)freeNode > (unsigned long)(((char *)head)+head->nodeSize)) && 
          ((unsigned long)freeNode > (unsigned long)temp) && 
          (temp != NULL)) {		
         head = temp;
         temp = head->nextNode;
      } else {
         if ((char *)freeNode == (((char *)head)+head->nodeSize)) { 
            // adjacent to head node
            head->nodeSize = head->nodeSize + size;
            if (temp) {
               if ((char *)temp == (((char *)head)+head->nodeSize)) { 
                  // adjacent to temp node too !
                  head->nextNode = temp->nextNode;
                  head->nodeSize = head->nodeSize+temp->nodeSize;
               };
            };
            break;
         } else {
            if (temp) {
               if ((char *)temp == (((char *)freeNode)+size)) { 
                  //in front of temp node
                  head->nextNode = freeNode;
                  freeNode->nextNode = temp->nextNode;
                  freeNode->nodeSize = size+temp->nodeSize;
                  break;
               } else {
                  // insert between
                  freeNode->nextNode = head->nextNode;
                  freeNode->nodeSize = size;
                  head->nextNode = freeNode;
                  break;
               };
            } else { // end of list
               freeNode->nextNode = NULL;
               freeNode->nodeSize = size;
               if (freeNode != head)
                 head->nextNode = freeNode;
               break;
            }
         }
      }
   }
}

void freeNode_deallocSpace (FreeNode *freeNode, FreeNode **head, node_size_t size)
{
   FreeNode *h = *head;
   FreeNode *temp = h;
   node_size_t nSize;

   nSize = (node_size_t)((size + nodeRound ) / nodeAlign ) * nodeAlign ;

   if (h != NULL) {
      if ((unsigned long)freeNode < (unsigned long)temp) {
         // node before head
         if ((char *)temp == (((char *)freeNode)+nSize)) {
            freeNode->nextNode = h->nextNode;
            freeNode->nodeSize = h->nodeSize+nSize;
            *head = freeNode; 
         } else { 
            // node is discontiguous
            freeNode->nextNode = h;
            freeNode->nodeSize = nSize;
            *head = freeNode;
         }
      } else {
	freeNode_insertNode (freeNode, temp, nSize);
      }
   } else { 
      // list was empty
      freeNode->nextNode = NULL;
      freeNode->nodeSize = nSize;
      *head = freeNode;
   }
}

// the node pointer to by *head is big enough now allocate size space from it
static
FreeNode *freeNode_allocNode(FreeNode *freeNode, FreeNode **head, node_size_t size)
{
   FreeNode *h = *head;
   FreeNode *temp;
   node_size_t delta;

   delta = h->nodeSize - size;
   temp = h; // allocate from the front of the node
   if (delta) { 
      // head node bigger then required
      h = (FreeNode *)((char *)temp + size);
      h->nextNode = temp->nextNode;
      h->nodeSize = delta;
      *head = h;
   } else { 
      // delta == 0, head node just the right size
      *head = h->nextNode;
   };

   return (temp);
}

FreeNode* freeNode_allocSpace(FreeNode *freeNode, FreeNode **head, node_size_t size)
{
   FreeNode *temp = NULL;
   FreeNode *h = *head;
   node_size_t nSize;
   nSize = (node_size_t)((size+nodeRound)/nodeAlign)*nodeAlign;

   if (h != NULL) { 
      // free list is not empty
      if (h->nodeSize >= nSize) { 
         // head node is big enough
         temp = freeNode_allocNode (freeNode, head, nSize);
      } else { 
         // got search down the list
         temp = h;
         h = h->nextNode;
         while ( h ) {
            if (h->nodeSize >= nSize) {
               temp = freeNode_allocNode (freeNode, &temp->nextNode, nSize);
               break;
            } else {
               temp = h;
               h = h->nextNode;
            };
         };
         if (h == NULL) temp = NULL;
      };
   };

   return (temp);
}
