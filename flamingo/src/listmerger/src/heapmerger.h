/*  
    $Id: heapmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/11/2007

*/

#ifndef _heapmerger_h_
#define _heapmerger_h

#include "listmerger.h"

/*
  Heap-merge algorithm.
  Maintain a heap for all head elements
  of lists and sequentially scan all lists.
*/

template <class InvList = vector<unsigned> >
class HeapMerger : public ListsMerger<HeapMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "HeapMerger";   
 }
};

template <class InvList>
void 
HeapMerger<InvList>::
merge_Impl(const vector<InvList* > &invLists,
	   unsigned threshold,
	   vector<unsigned> &results) {
  
  const typename InvList::value_type maxUnsigned = (typename InvList::value_type)0xFFFFFFFF;
  
  unsigned numLists = invLists.size();
  if(threshold > numLists) return;
  
  // add maxUnsigned
  for(unsigned i = 0; i < numLists; i++)
    invLists[i]->push_back(maxUnsigned);

  vector<HeapElement<InvList> > heap;
  heap.reserve(numLists);
  
  // make initial heap
  make_heap(heap.begin(), heap.end());
  for(unsigned i = 0; i < numLists; i++) {
    heap.push_back(HeapElement<InvList>(invLists[i]->begin(), i));
    push_heap(heap.begin(), heap.end());
  }
  
  HeapElement<InvList> popped[numLists];
  
  while(*(heap.front().iter) != maxUnsigned) {                       
    
    unsigned count = 0;    
    unsigned currentId = *(heap.front().iter);
    
    while(currentId == *(heap.front().iter) && !heap.empty()) {
      popped[count++] = heap.front();
      pop_heap(heap.begin(), heap.end());
      heap.pop_back();
    }
    
    if(count >= threshold) {
      results.push_back(currentId);
    }
    
    // move to the next element
    for(unsigned i = 0; i < count; i++) {
      popped[i].iter++;
      heap.push_back(HeapElement<InvList>(popped[i].iter, popped[i].invListIndex));
      push_heap(heap.begin(), heap.end());
    }
  }
  
  // remove maxUnsigned
  for(unsigned i = 0; i < numLists; i++)
    invLists[i]->pop_back();
}


#endif
