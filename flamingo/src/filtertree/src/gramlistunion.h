/*
  $Id: gramlistunion.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Shengyue Ji <shengyuj (at) ics.uci.edu>
*/

#ifndef _gramlistunion_h_
#define _gramlistunion_h_

#include <stdio.h>
#include <stdlib.h>
#include "gramlist.h"

template <typename InvList = vector<unsigned> >
class GramListUnion : public GramList<InvList>
{
private:
  GramListUnion *parent;
public:
  unsigned rank;
  
  unsigned refCount;
  InvList list;
  
  GramListUnion();
  GramListUnion<InvList> *find();
  GramListUnion<InvList> *clean();
  
  static void _union(GramListUnion<InvList> *ga, GramListUnion<InvList> *gb, unsigned unionSize);
  static unsigned testUnion(GramListUnion *ga, GramListUnion *gb, 
			    float th = 0.0, bool testOnly = false);

  InvList* getArray(fstream* invListFile = NULL) { return &list; }
  void free() { 
    refCount--;
    if(refCount == 0) delete this;
  }
  
  void clear() {}
};

template <class InvList>
GramListUnion<InvList>::
GramListUnion()
  : parent(0), rank(0), refCount(1) {}

template <class InvList>
GramListUnion<InvList>*
GramListUnion<InvList>::
find()
{
    if(parent == NULL)return this;
    parent = parent->find();
    return parent;
}

template <class InvList>
GramListUnion<InvList>*
GramListUnion<InvList>::
clean()
{
    if(parent == NULL)return this;
    GramListUnion *tmp;
    tmp = parent->find();
    delete this;
    return tmp;
}


template <class InvList>
void 
GramListUnion<InvList>::
_union(GramListUnion<InvList> *ga, GramListUnion<InvList> *gb, unsigned unionSize)
{
    if(ga->rank < gb->rank)
    {
        GramListUnion *gt = ga;
        ga = gb;
        gb = gt;
    }
    
    if(ga->rank == gb->rank)
        ga->rank++;
            
    ga->refCount += gb->refCount;
    gb->parent = ga;
    
    // merge list b into list a
    InvList *a = &ga->list;
    InvList *b = &gb->list;
    int i, j, k;

    i = a->size() - 1;
    j = b->size() - 1;
    k = unionSize - 1;
    a->resize(unionSize);
    while(j >= 0 && i >= 0)
    {
        if((*a)[i] > (*b)[j])
            (*a)[k--] = (*a)[i--];
        else if((*a)[i] < (*b)[j])
            (*a)[k--] = (*b)[j--];
        else
        {
            (*a)[k--] = (*a)[i--];
            j--;
        }
    }
    while(j >= 0 && k >= 0)
    {
        (*a)[k--] = (*b)[j--];
    }
}

template <class InvList>
unsigned 
GramListUnion<InvList>::
testUnion(GramListUnion *ga, GramListUnion *gb, float th, bool testOnly)
{
    ga = ga->find();
    gb = gb->find();
    
    if(ga == gb)return 0;

    unsigned unionSize = 0;
    unsigned x = 0, y = 0;

    InvList &a = ga->list;
    InvList &b = gb->list;

    while(x < a.size() && y < b.size())
    {        
        unionSize++;
        if(a[x] == b[y])
        {                
            x++; y++;
        }
        else if(a[x] < b[y])
        {
            x++;
        }
        else
        {
            y++;
        }
    }
    unionSize += a.size() - x;
    unionSize += b.size() - y;

    unsigned intersection = a.size() + b.size() - unionSize;
    // jaccard similarity
    if(intersection >= (unsigned)(unionSize * th))
    {
        // do the union here, merge one into the other
        
        if(!testOnly)_union(ga, gb, unionSize);
        return intersection;
    }
    return 0;
}




#endif
