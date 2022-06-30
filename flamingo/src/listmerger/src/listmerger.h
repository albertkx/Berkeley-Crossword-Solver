/*  
 $Id: listmerger.h 5788 2010-10-22 10:09:57Z abehm $ 

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Author: Chen Li, Jiaheng Lu, and Yiming Lu, Alexander Behm
 Date: 05/11/2007
*/
 	
#ifndef _listsmerger_h_
#define _listsmerger_h_

#include <vector>
#include <string>
#include <set>
#include <stdint.h>

using namespace std;
 
template <class ListsMergerConcrete, class InvList>
class ListsMerger {
protected:
  bool hasDuplicateLists;
  
  // detect lists with identical references and return vector of distinct lists and their weights
  void detectDuplicateLists(vector<InvList*> &invLists,
			    vector<InvList*> &newInvLists,
			    vector<unsigned> &newWeights);
  
public:
  
  ListsMerger(bool hasDuplicateLists = false) : hasDuplicateLists(hasDuplicateLists) {}
  
  void merge(vector<InvList*> &arrays, 
	     const unsigned threshold, // threshold of count
	     vector<unsigned> &results) {
    static_cast<ListsMergerConcrete*>(this)->merge_Impl(arrays, threshold, results);
  }
  
  static bool cmpInvList(const InvList* a, const InvList* b) {
    return a->size() < b->size();
  }
  
  // exponential probe, returns iterator to first element greater or equal to value
  inline void expProbe(const typename InvList::iterator start, 
		       const typename InvList::iterator end, 
		       typename InvList::iterator& lbound, 
		       typename InvList::iterator& ubound, 
		       unsigned value) const; 

  string getName() {
    return "ListsMerger";
  }
  
  ~ListsMerger() {};
}; 

class Candi {
public:
  unsigned id;
  unsigned count;

  Candi(unsigned id, unsigned count) : id(id), count(count) {}
};

template <class InvList>
class HeapElement {
public:
  unsigned invListIndex;
  typename InvList::iterator iter;

  HeapElement(typename InvList::iterator iter, unsigned invListIndex) 
    : invListIndex(invListIndex), iter(iter) {}

  HeapElement() {}
  
  bool operator<(const HeapElement& e) const {
    if(*iter < *(e.iter)) return false;
    else return true;
  }
};

template <class ListsMergerConcrete, class InvList>
void  
ListsMerger<ListsMergerConcrete, InvList>::
detectDuplicateLists(vector<InvList*> &invLists,
		     vector<InvList*> &newInvLists,
		     vector<unsigned> &newWeights) {
  
  // we need to take care of unequal lists with the same size
  // do exhaustive search within each list length group
  sort(invLists.begin(), invLists.end(), cmpInvList);
  set<uintptr_t> invListsAdded;
  for(unsigned i = 0 ; i < invLists.size(); i++) {   
    InvList *currentArray = invLists.at(i);
    uintptr_t arrAddr = (uintptr_t)currentArray;
    unsigned currentCount = 1;
    
    // if the array has not been added previously
    if(invListsAdded.find(arrAddr) == invListsAdded.end()) {
      // search all invLists with the same length for identical pointers
      for(unsigned j = i + 1; j < invLists.size(); j++) {       
	InvList *iArray = invLists.at(j);
	if(iArray->size() != currentArray->size()) break;
	if(iArray == currentArray) currentCount++;
      }    
      // add the array
      newInvLists.push_back(currentArray);
      newWeights.push_back(currentCount);
      invListsAdded.insert(arrAddr);
    }
  }
}

// exponential probe, returns iterator to first element greater or equal to value
template <class ListsMergerConcrete, class InvList>
void
ListsMerger<ListsMergerConcrete, InvList>::
expProbe(const typename InvList::iterator start, 
	 const typename InvList::iterator end, 
	 typename InvList::iterator& lbound, 
	 typename InvList::iterator& ubound, 
	 unsigned value) const {
  
  unsigned c = 0;    
  lbound = start;
  for(;;) {      
    ubound = start + (1 << c);
    if(ubound >= end) {
      ubound = end;
      return;
    }
    else if(*ubound >= value) return;
    c++;
    lbound = ubound;
  }
}

#endif
