//  
// $Id: Rect.cpp 5027 2010-02-18 19:41:48Z rares $
//
// RETree.cpp : Defines the entry point for the console application.
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
//          Liang Jin <liangj (at) ics.uci.edu>

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "assert.h"
#include "index.h"
#include "parameters.h"


#define BIG_NUM (FLT_MAX/4.0)


#define Undefined(x) ((x)->boundary[0] > (x)->boundary[NUMDIMS-1])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


/*-----------------------------------------------------------------------------
| Initialize a rectangle to have all 0 coordinates.
-----------------------------------------------------------------------------*/
void RTreeInitRect(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	for (i=0; i<NUMDIMS; i++)
		r->boundary[i] = (RectReal)0;
	
	//memset(r->trie, ' ', TRIELEN);
	//memset(r->minstr, ' ', MAXLEN);
	//memset(r->maxstr, ' ', MAXLEN);
	//r->trie = new char[TRIELEN];
	r->trie = new string();

	r->realtrie = new Trie();

	r->compratio = 1;

	//r->nfa = NULL;
}

void RTreeDeleteRect(struct Rect *R)
{
	delete R->trie;

	delete R->realtrie;

	//if(R->nfa != NULL) delete R->nfa;

	return;
}


/*-----------------------------------------------------------------------------
| Return a rect whose first low side is higher than its opposite side -
| interpreted as an undefined rect.
-----------------------------------------------------------------------------*/
struct Rect RTreeNullRect()
{
	struct Rect r;
	//register int i;

	r.boundary[0] = (RectReal)1;
	r.boundary[NUMDIMS-1] = (RectReal)-1;
	
	/*for (i=1; i<NUMDIMS; i++)
		r.boundary[i] = r.boundary[i+NUMDIMS] = (RectReal)0;*/
	/*comment by liang*/
	//memset(r.trie, ' ', TRIELEN);
	//memset(r.minstr, ' ', MAXLEN);
	//memset(r.maxstr, ' ', MAXLEN);
	//r.trie = new char[MAXLEN];
	//r.trie = new string();
	
	return r;
}


#if 0


#endif

/*-----------------------------------------------------------------------------
| Print out the data for a rectangle.
-----------------------------------------------------------------------------*/
/*void RTreePrintRect(struct Rect *R, int depth)
{
	register struct Rect *r = R;
	register int i;
	assert(r);

	RTreeTabIn(depth);
	printf("rect:\n");
	RTreeTabIn(depth+1);
	printf("%f\t%f\n", r->boundary[0], r->boundary[NUMDIMS - 1]);
	RTreeTabIn(depth+1);
	printf("%s\t%s\n", r->strbound[0], r->strbound[NUMDIMS - 1]);
}*/

/*-----------------------------------------------------------------------------
| Calculate the n-dimensional volume of a rectangle
-----------------------------------------------------------------------------*/
/*RectReal RTreeRectVolume(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	register RectReal volume = (RectReal)1;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;

	volume = r->boundary[NUMDIMS - 1] - r->boundary[0];
	
	volume = volume * GetQGramDistance(r->strbound[0], r->strbound[NUMDIMS - 1]);
		
	assert(volume >= 0.0);
	return volume;
}*/

/*float GetQGramDistance(char* str1, char* str2)
{
	int i, tmp;
	float result = 0.0;
	char char1, char2;
	for(i=0; i<QGRAM; i++)
	{
		if(str1[i]>='0' && str1[i]<='9')
			char1 = str1[i] - '0' + 26;
		else if(str1[i]>='a' && str1[i]<='z')
			char1 = str1[i] - 'a';
		else
			char1 = 36;
		
		if(str2[i]>='0' && str2[i]<='9')
			char2 = str2[i] - '0' + 26;
		else if(str2[i]>='a' && str2[i]<='z')
			char2 = str2[i] - 'a';
		else
			char2 = 36;
			
        tmp = char2 - char1;
                
        result = result*NUMCHAR + tmp;			 
	}
	
	result = (float)result / MAXCHAR;
	
	return result;
	 
}*/




/*-----------------------------------------------------------------------------
| Combine two rectangles, make one that includes both.
-----------------------------------------------------------------------------*/
/*struct Rect RTreeCombineRect(struct Rect *R, struct Rect *Rr)
{
	register struct Rect *r = R, *rr = Rr;
	register int i, j;
	struct Rect new_rect;
	assert(r && rr);

	if (Undefined(r))
		return *rr;

	if (Undefined(rr))
		return *r;

	new_rect.boundary[0] = MIN(r->boundary[0], rr->boundary[0]);
	new_rect.boundary[NUMDIMS - 1] = MAX(r->boundary[NUMDIMS - 1], rr->boundary[NUMDIMS - 1]);
	
	if(strcmp(r->strbound[0], rr->strbound[0]) <= 0)
		strcpy(new_rect.strbound[0], r->strbound[0]);
	else
		strcpy(new_rect.strbound[0], rr->strbound[0]);
	
	if(strcmp(r->strbound[NUMDIMS - 1], rr->strbound[NUMDIMS - 1]) >= 0)
		strcpy(new_rect.strbound[NUMDIMS - 1], r->strbound[NUMDIMS - 1]);
	else
		strcpy(new_rect.strbound[NUMDIMS - 1], rr->strbound[NUMDIMS - 1]);
		
	
	return new_rect;
}*/


/*-----------------------------------------------------------------------------
| Decide whether two rectangles overlap.
-----------------------------------------------------------------------------*/
/*int RTreeOverlap(struct Rect *R, struct Rect *S)
{
	register struct Rect *r = R, *s = S;
	register int i, j;
	assert(r && s);

	if(r->boundary[0] > s->boundary[NUMDIMS - 1] ||
	   s->boundary[0] > r->boundary[NUMDIMS - 1])
	{
		return FALSE;
	}
	if(strcmp(r->strbound[0], s->strbound[NUMDIMS - 1]) > 0 ||
	   strcmp(s->strbound[0], r->strbound[NUMDIMS - 1]) > 0)
	{
		return FALSE;
	}
	
	return TRUE;
}*/


/*-----------------------------------------------------------------------------
| Decide whether rectangle r is contained in rectangle s.
-----------------------------------------------------------------------------*/
/*int RTreeContained(struct Rect *R, struct Rect *S)
{
	register struct Rect *r = R, *s = S;
	register int i, j, result;
	assert((int)r && (int)s);

 	// undefined rect is contained in any other
	//
	if (Undefined(r))
		return TRUE;

	// no rect (except an undefined one) is contained in an undef rect
	//
	if (Undefined(s))
		return FALSE;

	result = TRUE;
	if((r->boundary[0] >= s->boundary[0] &&
	    r->boundary[NUMDIMS - 1] <= s->boundary[NUMDIMS - 1])
	 &&(strcmp(r->strbound[0], s->strbound[0]) >= 0 &&
	    strcmp(r->strbound[NUMDIMS - 1], s->strbound[NUMDIMS - 1]) <= 0))
	{
		return TRUE;
	}
	return FALSE;
}*/


/*int QueryOverlap(struct Query *Q, struct Rect *S)
{
	register struct Rect *s = S;
	register struct Query *q = Q;
	register int i, j;
	int kplus1, gaps;
	char* temp;
	
	assert(q && s);

		
	if(q->numVal - q->numDelta > s->boundary[NUMDIMS - 1] ||
	   s->boundary[0] > q->numVal + q->numDelta)
	{
		return FALSE;
	}
	
	kplus1 = q->strDelta + 1;
	gaps = (int)ceil(strlen(q->strVal) / kplus1);
	
	temp = (char*)malloc(gaps*sizeof(char));
		
	for(i=0; i < kplus1; i++)
	{
		memset(temp, ' ', gaps);
		strncpy(temp, q->strVal + i*gaps, gaps);
		
	 	if(strcmp(temp, s->strbound[0]) >= 0 &&
	   	   strcmp(temp, s->strbound[NUMDIMS - 1]) <= 0)
		{
			free(temp);
			return TRUE;
		}
	}
	
	free(temp);
	
	return FALSE;
}*/
