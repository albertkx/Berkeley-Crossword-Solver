/*  
    $Id: mergeskipmerger.h 4887 2009-12-16 22:01:55Z abehm $  

    Copyright (C) 2010 by The Regents of the University of California
    
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/14/2007

*/

#ifndef _mergeskipmerger_h_
#define _mergeskipmerger_h_

#include "listmerger.h"

/*
  MergeSkip algorithm.
  Exploit the threshold and
  use binary search to skip elements.
*/

template <class InvList = vector<unsigned> >
class MergeSkipMerger : public ListsMerger<MergeSkipMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "MergeSkipMerger";
 }
};

template <class InvList>
void  
MergeSkipMerger<InvList>::
merge_Impl(const vector<InvList*> &invLists,
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
  
  unsigned pivot = threshold - 1;
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
      
      // move to the next element
      for(unsigned i = 0; i < count; i++) {
	popped[i].iter++;
	heap.push_back(HeapElement<InvList>(popped[i].iter, popped[i].invListIndex));
	push_heap(heap.begin(), heap.end());
      }      
      continue;
    }
    
    // pop more elements from heap
    while(count < pivot) {
      popped[count++] = heap.front();
      pop_heap(heap.begin(), heap.end());
      heap.pop_back();
    }
    
    // skip
    for(unsigned i = 0; i < count; i++) {
      popped[i].iter = lower_bound(invLists[popped[i].invListIndex]->begin(), 
				   invLists[popped[i].invListIndex]->end(),
				   *(heap.front().iter));
    }
    
    // insert to heap
    for(unsigned i = 0; i < count; i++) {
      heap.push_back(HeapElement<InvList>(popped[i].iter, popped[i].invListIndex));
      push_heap(heap.begin(), heap.end());
    }
  }
  
  // remove maxUnsigned
  for(unsigned i = 0; i < numLists; i++)
    invLists[i]->pop_back();
}

#endif
