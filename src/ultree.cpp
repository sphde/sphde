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
#ifndef __SAS__
#include <stdio.h>
#include <string.h>
#endif
#include "sasio.h"
#include "sasalloc.h"
#include "ultree.h"

void
uLongTreeNode::init(search_t k, info_t i)
{
#ifdef __SOMDebugPrint__
    sas_printf("uLongTreeNode::init\n");
#endif

    key	= k;
    info  = i;
    left	= NULL;
    right	= NULL;
};

void
uLongTreeNode::kill()
{
    SASNearDealloc(this, sizeof(uLongTreeNode));
};

uLongTreeNode
**uLongTreeNode::searchNode (uLongTreeNode **root, search_t k)
{
   uLongTreeNode	*p = *root;
   uLongTreeNode	**pp = root;
   uLongTreeNode	**qq = NULL;
   int				finished = 0;

   do {
      p = *pp;
      if ( p )
      {
         if ( p->key == k)
         {
            qq = pp;
            finished = 1;
         } else {
            if ( p->key < k )
            {
               if ( !p->right ) qq = pp;
               pp = &p->right;
            } else {
               qq = pp;
               pp = &p-> left;
            };
         }
      } else
         finished = 1;
	} while (!finished);

	return qq;
};

uLongTreeNode
*uLongTreeNode::searchNode (uLongTreeNode *root, search_t k)
{
    uLongTreeNode	*p = root;
    uLongTreeNode	*q = NULL;
    int				finished = 0;

    do {
	if ( p )
	    {
	    if ( p->key == k )
		{
		q = p;
		finished = 1;
		}
	    else {
		if ( p->key < k )
		    {
		    if ( !p->right ) q = p;
		    p = p->right;
		    } else {
			if ( !p->left ) q = p;
			p = p-> left;
			};
		}
	    } else
		finished = 1;
	} while (!finished);

	return q;
};

uLongTreeNode
*uLongTreeNode::searchNextNode (uLongTreeNode *root, search_t k)
{
    uLongTreeNode	*p = root;
    uLongTreeNode	*q = NULL;
    int				finished = 0;

    do {
	if ( p )
	    {
	    if ( p->key == k )
		{
			if (p->right)
			{
				q = p->right;
            p = p->right;
			} else {
			   finished = 1;
			};
		}
	    else {
		if ( p->key < k )
		    {
		      if ( p->right )
            {
               if ( p->right->key > k ) q = p->right;
            }
		      p = p->right;
		    } else {
			   if ( p->key > k ) q = p;
			   p = p-> left;
			};
		}
	    } else
		finished = 1;
	} while (!finished);

	return q;
};

uLongTreeNode
*uLongTreeNode::searchEqualOrNextNode (uLongTreeNode *root, search_t k)
{
    uLongTreeNode	*p = root;
    uLongTreeNode	*q = NULL;
    int				finished = 0;

    do {
	if ( p )
	    {
	    if ( p->key == k )
		{
         q = p;
         finished = 1;
		}
	    else {
		if ( p->key < k )
		    {
		      if ( p->right )
            {
               if ( p->right->key > k ) q = p->right;
            }
		      p = p->right;
		    } else {
			   if ( p->key > k ) q = p;
			   p = p-> left;
			};
		}
		} else
		finished = 1;
	} while (!finished);

	return q;
};

uLongTreeNode
**uLongTreeNode::searchEqualOrNextNode (uLongTreeNode **root, search_t k)
{
   uLongTreeNode	*p = *root;
   uLongTreeNode	**pp = root;
   uLongTreeNode	**qq = NULL;
   int				finished = 0;

   do {
      p = *pp;
      if ( p )
      {
         if ( p->key == k )
         {
            qq = pp;
            finished = 1;
         } else {
            if ( p->key < k )
            {
               if ( p->right )
               {
                  if ( p->right->key > k )
                  {
                     qq = &p->right;
                  }
               }
               pp = &p->right;
            } else {
               if ( p->key > k ) qq = pp;
               pp = &p-> left;
            };
         }
      } else
         finished = 1;
    } while (!finished);

    return qq;
};

uLongTreeNode
*uLongTreeNode::searchPrevNode (uLongTreeNode *root, search_t k)
{
    uLongTreeNode	*p = root;
    uLongTreeNode	*q = NULL;
    int				finished = 0;

    do {
	if ( p )
	    {
	    if ( p->key == k )
		{
			if (p->left)
			{
				q = p->left;
            p = p->left;
			} else {
			   finished = 1;
			};
		}
	    else {
		if ( p->key < k )
		    {
			   if ( p->key < k ) q = p;
			   p = p-> right;
		    } else {
		      if ( p->left )
            {
               if ( p->left->key < k ) q = p->left;
            }
		      p = p->left;
			};
		}
	    } else
		finished = 1;
	} while (!finished);

	return q;
};

void
uLongTreeNode::deleteNode (uLongTreeNode **pp)
{
   uLongTreeNode	*q;
   uLongTreeNode *p = *pp;

   if ( p )
	{
      if ( p->right == NULL )
      {	// re-attach left subtree in place of node p
         q = p;
         *pp = p->left;
         q->kill();
      } else {
         if ( p->left == NULL )
         {	// re-attach right subtree in place of p
            q = p;
            *pp = p->right;
            q->kill();
         } else {	// neither subtree is empty
            q = p->right;	// move right then as far left as possible
            while ( q->left ) q = q->left;
            q->left = p->left;
            q = p;
            *pp = p->right;
            q->kill();
			};
		};
	};
};

uLongTreeNode
*uLongTreeNode::removeNode (uLongTreeNode **pp)
{
   uLongTreeNode  *q = NULL;
   uLongTreeNode  *p = *pp;

   if ( p )
	{
      if ( p->right == NULL )
      {	// re-attach left subtree in place of node p
         q = p;
         *pp = p->left;
      } else {
         if ( p->left == NULL )
         {	// re-attach right subtree in place of p
            q = p;
            *pp = p->right;
         } else {	// neither subtree is empty
			   q = p->right;	// move right then as far left as possible
            while ( q->left ) q = q->left;
            q->left = p->left;
            q = p;
            *pp = p->right;

			};
		};
	};
   return q;
};

uLongTreeNode
*uLongTreeNode::insertNode (uLongTreeNode **root, search_t k, info_t i)
{
    uLongTreeNode	*p;
#ifdef __SOMDebugPrint__
    sas_printf("uLongTreeNode::insertNode\n");
#endif
    uLongTreeNode *n = (uLongTreeNode *)SASNearAlloc(root, sizeof(uLongTreeNode));
    n->init(k, i);

    p = *root;
    if (*root != NULL)
	{
	do {
	    if ( k < p->key )
		{
		if ( p->left )
		    {
		    p = p->left;
		    } else {
			p->left = n;
			p = NULL;
			};
		} else {
		    if ( k > p->key )
			{
			if ( p->right )
			    {
			    p = p->right;
			    } else {
				p->right = n;
				p = NULL;
				};
			} else {
			    n->kill();
			    n = NULL;
			    p = NULL;
			    };
		    };
	    } while ( p );
	} else {
	    *root = n;
	    };

	    return n;
};

uLongTreeNode
*uLongTreeNode::insertNode (uLongTreeNode **root, uLongTreeNode *n)
{
   uLongTreeNode	*p;
#ifdef __SASDebugPrint__
   sas_printf("uLongTreeNode::insertNode\n");
#endif
   search_t       k = n->getKey();

   p = *root;
   if (*root != NULL)
	{
      do {
         if ( k < p->key )
         {
            if ( p->left )
            {
               p = p->left;
            } else {
               p->left = n;
               p = NULL;
            };
         } else {
            if ( k > p->key )
            {
               if ( p->right )
               {
                  p = p->right;
               } else {
                  p->right = n;
                  p = NULL;
				   };
            } else {
               n = NULL;
               p = NULL;
            };
         };
      } while ( p );
   } else {
      *root = n;
   };

   return n;
};

void
uLongTreeNode::listNodes (int indent)
{
    int	count = 0;
    int	i;

    for (i = 0; i < indent; i++) { sas_printf(" "); };
    count = listNodes (indent, count );
};

int
uLongTreeNode::listNodes (int indent, int count)
{

    if ( left  ) count = left->listNodes (indent, count);

    sas_printf("\t%p\t@<%p>\n",(void*)key,(void*)info);
    count++;
    if ( right ) count = right->listNodes (indent, count);

    return count;
};

void
uLongTreeNode::listNodesDepth ()
{
    int	count = 0;
    count = listNodesDepth ( count, 0 );
};

int
uLongTreeNode::listNodesDepth (int count, int depth)
{
    if ( left  )
	{
	sas_printf("<");
	count = left->listNodesDepth (count, depth + 1);
	};

	sas_printf("%p-%p[%d] ",((void *)key), ((void *)info), depth);
	count++;
	if ((count % 3) == 0) sas_printf("\n");

	if ( right )
	    {
	    sas_printf(">");
	    count = right->listNodesDepth (count, depth + 1);
	    };

	    return count;
};

int
uLongTreeNode::totalNodes ()
{
    int	count = 0;
    count = totalNodes ( count );
    return count;
};

int
uLongTreeNode::totalNodes (int count)
{
    if ( left  )
	{
	count = left->totalNodes (count);
	};

	count++;

	if ( right )
	    {
	    count = right->totalNodes (count);
	    };

	    return count;
};
int
uLongTreeNode::maxNodeDepth ()
{
    int	max = 0;
    max = maxNodeDepth ( max, 0 );
    return max;
};

int
uLongTreeNode::maxNodeDepth (int max, int depth)
{
    if ( left  ) max = left->maxNodeDepth (max, depth + 1);

    if ( max < depth ) max = depth;

    if ( right ) max = right->maxNodeDepth (max, depth + 1);

    return max;
};
