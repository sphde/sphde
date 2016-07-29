/*
 * Copyright (c) 1995-2014 IBM Corporation.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Contributors:
 *     IBM Corporation, Steven Munroe - initial API and implementation
 */

#ifndef _ULONGTREE_H
#define _ULONGTREE_H

typedef unsigned long search_t;
typedef unsigned long info_t;

class uLongTreeNode {
		search_t		key;
		info_t			info;
		uLongTreeNode	*left;
		uLongTreeNode	*right;
	public:
		void setInfo(info_t i)
			{
				info = i;
			};
		void setKey(search_t k)
			{
				key = k;
			};
		info_t getInfo()
			{
				return info;
			};
		search_t getKey()
			{
				return key;
			};
		void init(search_t k, info_t i);
		void kill();
		uLongTreeNode *insertNode (	uLongTreeNode **root,
									search_t k, info_t i);
		uLongTreeNode *insertNode (	uLongTreeNode **root,
									uLongTreeNode *node);
		uLongTreeNode *searchNode (	uLongTreeNode *root,
									search_t key);
		uLongTreeNode *searchNextNode (	uLongTreeNode *root,
										search_t key);
		uLongTreeNode *searchEqualOrNextNode (	uLongTreeNode *root,
												search_t key);
		uLongTreeNode *searchPrevNode (	uLongTreeNode *root,
										search_t key);
		uLongTreeNode **searchNode (uLongTreeNode **root,
									search_t key);
		uLongTreeNode **searchEqualOrNextNode (	uLongTreeNode **root,
												search_t key);
		uLongTreeNode *removeNode (uLongTreeNode **pp);
		void deleteNode (uLongTreeNode **pp);
		int totalNodes ();		int maxNodeDepth ();

		void listNodes (int indent);
		void listNodesDepth ();
	private:
		int maxNodeDepth (int max, int depth);
		int totalNodes (int count); 
		int listNodes (int indent, int count);
  		int listNodesDepth (int count, int depth);
};
#endif /* _STRTREE_H */
