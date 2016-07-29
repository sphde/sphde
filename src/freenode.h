/*
 * Copyright (c) 2009-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

// utility class for managing a heap

#ifndef __SJM_freenode_H
#define __SJM_freenode_H

#define nodeAlign sizeof(struct freeNode)
#define nodeRound (sizeof(struct freeNode)-1)

typedef unsigned long node_size_t;

struct freeNode 
{
  struct freeNode *nextNode;
  node_size_t     nodeSize;
};
typedef struct freeNode FreeNode;


#ifdef __cplusplus
#define __C__ "C"
#else
#define __C__
#endif

extern __C__ void 
freeNode_init (FreeNode *freeNode, node_size_t size);

extern __C__ FreeNode*
freeNode_allocSpace(FreeNode *freeNode, FreeNode **head, node_size_t size);

extern __C__ void
freeNode_deallocSpace(FreeNode *freeNode, FreeNode **head, node_size_t size);

extern __C__ node_size_t
freeNode_freeSpaceTotal (FreeNode *freeNode);

extern __C__ node_size_t
freeNode_freeFragmentsTotal (FreeNode *freeNode);

extern __C__ node_size_t
freeNode_maxFragment (FreeNode *freeNode);

#endif
