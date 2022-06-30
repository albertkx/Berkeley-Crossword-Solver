//  
// $Id: index.h 4143 2008-12-08 23:23:55Z abehm $
//
// Trie.h: interface for the Trie class.
//
//  Copyright (C) 2004 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: October, 2004
//
// Author: 
//          Liang Jin (liangj (at) ics.uci.edu)
//

#ifndef _INDEX_
#define _INDEX_

#include "parameters.h"
#include "Trie.h"
#include "NFA.h"


#define NDEBUG

typedef float RectReal;


/*-----------------------------------------------------------------------------
| Global definitions.
-----------------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


struct Query
{
	float numVal;
	float numDelta;
	char strVal[MAXLEN];
	int strDelta;
};

struct Record
{
	float numVal;
	char strVal[MAXLEN];
};

struct Rect
{
	RectReal boundary[NUMDIMS]; /* xmin,ymin,...,xmax,ymax,... */
	Trie* realtrie;
	float compratio;
	string* trie;
	//NFA* nfa;
	//char minstr[MAXLEN];
	//char maxstr[MAXLEN];
};

struct Node;

struct Branch
{
	struct Rect rect;
	struct Node *child;
};

/* max branching factor of a node */
//#define MAXCARD (int)((PGSIZE-(2*sizeof(int))) / sizeof(struct Branch))
//#define LEAFCARD (int)((PAGE-sizeof(int))/sizeof(struct Record))

#define MAXCARD 10
#define LEAFCARD 40


struct Node
{
	bool isLeaf;
	int count;
	int level; /* 0 is leaf, others positive */
	struct Branch branch[MAXCARD];
	struct Record *records[LEAFCARD];
};



struct ListNode
{
	struct ListNode *next;
	struct Node *node;
};




/*
 * If passed to a tree search, this callback function will be called
 * with the ID of each data rect that overlaps the search rect
 * plus whatever user specific pointer was passed to the search.
 * It can terminate the search early by returning 0 in which case
 * the search will return the number of hits found up to that point.
 */
typedef int (*SearchHitCallback)(int id, void* arg);


extern int RTreeSearch(struct Node*, struct Rect*, SearchHitCallback, void*);
extern int RTreeInsertRect(struct Rect*, int, struct Node**, int depth);
extern void RTreeDeleteRect(struct Rect*);
extern struct Node * RTreeNewIndex();
extern struct Node * RTreeNewNode();
extern void RTreeInitNode(struct Node*);
extern void RTreeFreeNode(struct Node *);
extern void RTreeFreeLeafNode(struct Node *);
extern void RTreePrintNode(struct Node *, int);
extern void RTreeTabIn(int);
extern struct Rect RTreeNodeCover(struct Node *);
extern void RTreeInitRect(struct Rect*);
extern struct Rect RTreeNullRect();
extern RectReal RTreeRectArea(struct Rect*);
extern RectReal RTreeRectSphericalVolume(struct Rect *R);
extern RectReal RTreeRectVolume(struct Rect *R);
extern struct Rect RTreeCombineRect(struct Rect*, struct Rect*);
extern int RTreeOverlap(struct Rect*, struct Rect*);
extern void RTreePrintRect(struct Rect*, int);
extern int RTreeAddBranch(struct Branch *, struct Node *, struct Node **);
extern int RTreePickBranch(struct Rect *, struct Node *);
extern void RTreeDisconnectBranch(struct Node *, int);
extern void RTreeSplitNode(struct Node*, struct Branch*, struct Node**);

extern int RTreeSetNodeMax(int);
extern int RTreeSetLeafMax(int);
extern int RTreeGetNodeMax();
extern int RTreeGetLeafMax();

extern float GetQGramDistance(char*, char*);
extern bool QueryOverlap(struct Query*, struct Rect*);

#endif /* _INDEX_ */
