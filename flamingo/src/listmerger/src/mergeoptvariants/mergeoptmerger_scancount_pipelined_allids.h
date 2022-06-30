/*  
    $Id: mergeoptmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    This merger solves the T-Occurrence problem 
    with the algorithm MergeOpt originally proposed in:
    Sunita Sarawagi, Alok Kirpal
    "Efficent set joins on similarity predicates", in SIGMOD 2004
    
    The main idea is to separate the short lists from the long lists.
    We first process the short lists and then vefify the results by 
    binary searching the long lists.

    To process the short lists we use ScanCount.

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/16/2007

*/

#ifndef _mergeoptmerger_h_
#define _mergeoptmerger_h_

#include "listmerger.h"
#include <string.h>
#include <algorithm>

template <class InvList = vector<unsigned> >
class MergeOptMerger : public ListsMerger<MergeOptMerger<InvList>, InvList> {
private:
  
  unsigned short* sc;
  unsigned maxId;
  unsigned* queryId;
  unsigned currentQuery;

  // using list weights
  void mergeWithDupls(vector<InvList*> &invLists,
		      unsigned threshold,
		      vector<unsigned> &results);
  
  // no list weights
  void mergeWithoutDupls(vector<InvList*> &invLists,
			 unsigned threshold,
			 vector<unsigned> &results);  
  
public:
  MergeOptMerger(unsigned maxId = 100000, bool hasDuplicateLists = false)
    : ListsMerger<MergeOptMerger<InvList>, InvList>(hasDuplicateLists) {    
    sc = new unsigned short[maxId];   
    memset(sc, 0, sizeof(unsigned short) * maxId);
    queryId = new unsigned [maxId];
    memset(queryId, 0, sizeof(unsigned) * maxId);
    this->maxId = maxId;
    currentQuery = 0;
  }
  
  void merge_Impl(vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results);
  
  string getName() {
   return "MergeOptMerger";
  }
  
  ~MergeOptMerger() {
    if(sc) delete[] sc;
    if(queryId) delete[] queryId;
  }  
};

template <class InvList>
void  
MergeOptMerger<InvList>::
merge_Impl(vector<InvList*> &invLists,
	   unsigned threshold,
	   vector<unsigned> &results) {  

  if(threshold > invLists.size()) return;
  
  if(this->hasDuplicateLists) mergeWithDupls(invLists, threshold, results);
  else mergeWithoutDupls(invLists, threshold, results);  
}

template <class InvList>
void  
MergeOptMerger<InvList>::
mergeWithoutDupls(vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results) {
  
  sort(invLists.begin(), invLists.end(), MergeOptMerger<InvList>::cmpInvList);
  
  unsigned numShortLists = invLists.size() - threshold + 1;
  
  currentQuery++;
  
  vector<Candi> candis;
  for(unsigned i = 0; i < numShortLists; i++) {
    for(unsigned j = 0; j < invLists[i]->size(); j++) {      
      unsigned id = invLists[i]->at(j);
      if(queryId[id] == currentQuery) {
	sc[id]++;	
      }
      else {
	queryId[id] = currentQuery;
	sc[id] = 1;
      }
    }
  }
  
  // verify candidates on long lists
  unsigned stop = invLists.size();
  unsigned start = numShortLists;  
  typename InvList::iterator longListsPointers[invLists.size() - numShortLists];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = invLists[j]->begin();
  
  for(unsigned i = 0; i < maxId; i++) {
    if(sc[i] > 0) {      
      if(sc[i] >= threshold) {
	results.push_back(i);
	continue;
      }
      
      unsigned targetCount = threshold - sc[i];
      unsigned count = 0;
      for(unsigned j = start; j < stop; j++) {	
	unsigned llindex = j - start;
	typename InvList::iterator start, end;
	expProbe(longListsPointers[llindex], invLists[j]->end(), start, end, i);
	
	typename InvList::iterator iter = lower_bound(start, end, i);
	longListsPointers[llindex] = iter;
	if(*iter == i) {
	  count++;
	  if(count >= targetCount) {
	    results.push_back(i);
	    break;
	  }
	}
	else {
	  if(count + stop - j - 1 < targetCount) break; // early termination
	}
      }   
    }      
  }
}

template <class InvList>
void  
MergeOptMerger<InvList>::
mergeWithDupls(vector<InvList*> &invLists,
	       unsigned threshold,
	       vector<unsigned> &results) {
  
  vector<InvList*> newInvLists;
  vector<unsigned> newWeights;
  detectDuplicateLists(invLists, newInvLists, newWeights);
  
  // assume that newArray should be sorted according to the length increasing
  unsigned numShortLists = 0;
  unsigned weightCounter = 0;
  unsigned shortListsWeight = invLists.size() - threshold + 1;
  for(unsigned i = 0; i < newInvLists.size() && weightCounter < shortListsWeight; i++) {
    weightCounter += newWeights[i];
    numShortLists++;
  }
  

  // OLD Heap-Based merging of short lists
  /*
  const typename InvList::value_type maxUnsigned = (typename InvList::value_type)0xFFFFFFFF;
  
  // add maxUnsigned
  for(unsigned i = 0; i < numShortLists; i++)
    newInvLists[i]->push_back(maxUnsigned);
  
  vector<HeapElement<InvList> > heap;
  heap.reserve(numShortLists);
  
  // make initial heap
  make_heap(heap.begin(), heap.end());
  for(unsigned i = 0; i < numShortLists; i++) {
    heap.push_back(HeapElement<InvList>(newInvLists[i]->begin(), i));
    push_heap(heap.begin(), heap.end());
  }
  
  HeapElement<InvList> popped[numShortLists];  
  vector<Candi> candis;
  
  // merge the short lists
  while(*(heap.front().iter) != maxUnsigned) {                       
    
    unsigned weight = 0;    
    unsigned count = 0;
    unsigned currentId = *(heap.front().iter);
    
    while(currentId == *(heap.front().iter) && !heap.empty()) {
      popped[count++] = heap.front();
      weight += newWeights[heap.front().invListIndex];
      pop_heap(heap.begin(), heap.end());
      heap.pop_back();
    }
    
    candis.push_back(Candi(currentId, weight));
    
    // move to the next element and push
    for(unsigned i = 0; i < count; i++) {
      popped[i].iter++;
      heap.push_back(HeapElement<InvList>(popped[i].iter, popped[i].invListIndex));
      push_heap(heap.begin(), heap.end());
    }
  }
  
  // remove maxUnsigned
  for(unsigned i = 0; i < numShortLists; i++)
    newInvLists[i]->pop_back(); 
  */

  
  vector<Candi> candis;
  for(unsigned i = 0; i < numShortLists; i++) {
    vector<Candi> tmp;
    vector<Candi>::const_iterator candiIter = candis.begin();

    typename InvList::iterator invListIter = invLists[i]->begin();
    typename InvList::iterator invListEnd = invLists[i]->end();
    
    while(candiIter != candis.end() || invListIter != invListEnd) {
      if(candiIter == candis.end() || (invListIter != invListEnd && candiIter->id > *invListIter)) {
	tmp.push_back(Candi(*invListIter, newWeights[i]));
	invListIter++;
      } else if (invListIter == invListEnd || (candiIter != candis.end() && candiIter->id < *invListIter)) {
	tmp.push_back(Candi(candiIter->id, candiIter->count));
	candiIter++;
      } else {
	tmp.push_back(Candi(candiIter->id, candiIter->count + newWeights[i]));
	candiIter++;
	invListIter++;
      }
    }
    std::swap(candis, tmp);
  }


  // verify candidates on long lists
  unsigned stop = newInvLists.size();
  unsigned start = numShortLists;  
  typename InvList::iterator longListsPointers[newInvLists.size() - numShortLists];

  unsigned totalLongListsWeight = invLists.size() - shortListsWeight;
  unsigned weightRemaining[newInvLists.size() - numShortLists]; // for early termination
  unsigned cumulWeight = 0;
  for(unsigned j = start; j < stop; j++) {
    longListsPointers[j - start] = newInvLists[j]->begin();
    cumulWeight += newWeights[j];
    weightRemaining[j - start] = totalLongListsWeight - cumulWeight;
  }
  
  // use exponential probe + binary search for long lists
  for(unsigned i = 0; i < candis.size(); i++) {
    
    if(candis[i].count >= threshold) {
      results.push_back(candis[i].id);
      continue;
    }
    
    unsigned targetCount = threshold - candis[i].count;
    unsigned count = 0;
    for(unsigned j = start; j < stop; j++) {	
      unsigned llindex = j - start;
      typename InvList::iterator start, end;
      expProbe(longListsPointers[llindex], newInvLists[j]->end(), start, end, candis[i].id);
      
      typename InvList::iterator iter = lower_bound(start, end, candis[i].id);
      longListsPointers[llindex] = iter;
      if(*iter == candis[i].id) {
	count += newWeights[j];
	if(count >= targetCount) {
	  results.push_back(candis[i].id);
	  break;
	}
      }
      else {
	if(count + weightRemaining[llindex] < targetCount) break; // early termination
      }
    }   
  }
}


#endif
