/*  
    $Id: divideskipmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    This imeplementation merges multiple lists by 
    the DividedSkip algorithm.

    The lists are assumed to
    be sorted in an ascending order.

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/23/2007

*/

#ifndef _divideskipmerger_h_
#define _divideskipmerger_h_

#include <cmath>
#include <iostream>

#include "listmerger.h"
#include "util/src/misc.h"

// DivideSkip algorithm,
// which combine MergeOpt and MergeSkip algorithm.

template <class InvList = vector<unsigned> >
class DivideSkipMerger : public ListsMerger<DivideSkipMerger<InvList>, InvList> {
private:  

  // for estimation purposes
  float shortListsSlope;
  float shortListsIntercept;
  float longListsSlope;
  float longListsIntercept;  

  // for calculating L
  float mu;  
  
  // no list weights
  void mergeSkipWithoutDupls(const vector<InvList*> &invLists,
			     unsigned threshold,
			     unsigned longLists,
			     vector<Candi>& candis);
  
  // no list weights
  void mergeWithoutDupls(vector<InvList*> &invLists,
			 unsigned threshold,
			 vector<unsigned> &results);
  
  // using list weights
  void mergeSkipWithDupls(const vector<InvList*> &invLists,
			  const vector<unsigned> &weights,
			  unsigned threshold,
			  unsigned longLists,
			  vector<Candi>& candis);

  // using list weights
  void mergeWithDupls(vector<InvList*> &invLists,
		      unsigned threshold,
		      vector<unsigned> &results);
  
  // helper for query time estimatiom
  void getQueryStats(vector<InvList*> &invLists,
		     unsigned threshold,
		     float& shortListsVar,
		     float& shortListsTime,
		     float& longListsVar,
		     float& longListsTime);

public:   
  DivideSkipMerger(bool hasDuplicateLists = false)
    : ListsMerger<DivideSkipMerger<InvList>, InvList>(hasDuplicateLists),
      shortListsSlope(0.0f), shortListsIntercept(0.0f),
      longListsSlope(0.0f), longListsIntercept(0.0f), mu(0.01f) { }
 
  void merge_Impl(vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results);		  
    
  void fillEstimationParams(vector<vector<InvList* >* >& invLists,
			    vector<unsigned>& thresholds);  

  float getEstimatedQueryTime(vector<InvList*>& invLists,
			      unsigned threshold) const;
  
  string getName() {
    return "DivideSkipMerger";
  }   
};

template <class InvList>
void 
DivideSkipMerger<InvList>::
merge_Impl(vector<InvList*> &invLists,
	   unsigned threshold,
	   vector<unsigned> &results) {
  
  if(threshold > invLists.size() || threshold == 0) return;
  
  if(this->hasDuplicateLists) 
    mergeWithDupls(invLists, threshold, results);
  else 
    mergeWithoutDupls(invLists, threshold, results);  
}
 
template <class InvList>
void  
DivideSkipMerger<InvList>::
mergeWithoutDupls(vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results) {
  
  sort(invLists.begin(), invLists.end(), DivideSkipMerger<InvList>::cmpInvList);

  unsigned longestSize = invLists.back()->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1)) ;  
  if(longListsSize >= threshold) longListsSize = threshold - 1;
  
  // use mergeskip to search the short lists
  vector<Candi> candis;
  mergeSkipWithoutDupls(invLists,
			threshold-longListsSize,
			longListsSize,
			candis);
  
  unsigned stop = invLists.size();
  unsigned start = stop - longListsSize;
  typename InvList::iterator longListsPointers [longListsSize];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = invLists.at(j)->begin();

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
      expProbe(longListsPointers[llindex], invLists[j]->end(), start, end, candis[i].id);
      typename InvList::iterator iter = lower_bound(start, end, candis[i].id);
      longListsPointers[llindex] = start;
      if(iter != invLists[j]->end() && *iter == candis[i].id) {
	count++;
	if(count >= targetCount) {
	  results.push_back(candis[i].id);
	  break;
	}
      }
      else {
	if(count + stop - j - 1 < targetCount) break; // early termination
      }
    }   
  } 
}


template <class InvList>
void 
DivideSkipMerger<InvList>::
mergeSkipWithoutDupls(const vector<InvList*> &invLists,
		      unsigned threshold,
		      unsigned longLists,
		      vector<Candi>& candis) {
  
  const typename InvList::value_type maxUnsigned = (typename InvList::value_type)0xFFFFFFFF;
  
  unsigned numShortLists = invLists.size() - longLists;  

  if(threshold > numShortLists) return;
  
  // add maxUnsigned
  for(unsigned i = 0; i < numShortLists; i++)
    invLists[i]->push_back(maxUnsigned);

  vector<HeapElement<InvList> > heap;
  heap.reserve(numShortLists);
  
  // make initial heap
  make_heap(heap.begin(), heap.end());
  for(unsigned i = 0; i < numShortLists; i++) {
    heap.push_back(HeapElement<InvList>(invLists[i]->begin(), i));
    push_heap(heap.begin(), heap.end());
  }
  
  unsigned pivot = threshold - 1;
  HeapElement<InvList> popped[numShortLists];
  
  while(*(heap.front().iter) != maxUnsigned) {                       
    
    unsigned count = 0;    
    unsigned currentId = *(heap.front().iter);
    
    while(currentId == *(heap.front().iter) && !heap.empty()) {
      popped[count++] = heap.front();
      pop_heap(heap.begin(), heap.end());
      heap.pop_back();
    }
        
    if(count >= threshold) {
      candis.push_back(Candi(currentId, count));
      
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
  for(unsigned i = 0; i < numShortLists; i++)
    invLists[i]->pop_back();
}

template <class InvList>
void  
DivideSkipMerger<InvList>::
mergeWithDupls(vector<InvList*> &invLists,
	       unsigned threshold,
	       vector<unsigned> &results) {
  
  vector<InvList*> newInvLists;  
  vector<unsigned> newWeights;  
  detectDuplicateLists(invLists, newInvLists, newWeights);
  
  // assume that newArray should be sorted according to the length increasing
  unsigned longestSize = newInvLists.at(newInvLists.size()-1)->size();  
  unsigned targetLongListWeight = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));
  if(targetLongListWeight >= threshold) targetLongListWeight = threshold - 1;
  
  unsigned longListsSize = 0;
  unsigned totalLongListsWeight = 0;
  for(unsigned i = 0; i < newInvLists.size(); i++) {
    unsigned currentWeight = newWeights[newInvLists.size() - 1 - i];
    totalLongListsWeight += currentWeight;
    if(totalLongListsWeight >= targetLongListWeight) {
      if(totalLongListsWeight >= threshold) {
	longListsSize = i;
	totalLongListsWeight -= currentWeight;      
      }
      else {
	longListsSize = i + 1;	
      }
      break;
    }
  }
  
  vector<Candi> candis;
  mergeSkipWithDupls(newInvLists,
		     newWeights,
		     threshold-totalLongListsWeight,
		     longListsSize,
		     candis);
  
  unsigned stop = newInvLists.size();
  unsigned start = stop - longListsSize;
  typename InvList::iterator longListsPointers [longListsSize];
  
  unsigned weightRemaining[longListsSize]; // for early termination
  unsigned cumulWeight = 0;
  for(unsigned j = start; j < stop; j++) {
    longListsPointers[j - start] = newInvLists.at(j)->begin();
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
      if(iter != invLists[j]->end() && *iter == candis[i].id) {
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

template <class InvList>
void  
DivideSkipMerger<InvList>::
mergeSkipWithDupls(const vector<InvList*> &invLists,
		   const vector<unsigned> &weights,
		   unsigned threshold,
		   unsigned longLists,
		   vector<Candi>& candis) {
  
  const typename InvList::value_type maxUnsigned = (typename InvList::value_type)0xFFFFFFFF;
  
  unsigned numShortLists = invLists.size() - longLists;
  if(threshold > numShortLists) return;
  
  // add maxUnsigned
  for(unsigned i = 0; i < numShortLists; i++)
    invLists[i]->push_back(maxUnsigned);
  
  vector<HeapElement<InvList> > heap;
  heap.reserve(numShortLists);
  
  // make initial heap
  make_heap(heap.begin(), heap.end());
  for(unsigned i = 0; i < numShortLists; i++) {	    
    heap.push_back(HeapElement<InvList>(invLists[i]->begin(), i));
    push_heap(heap.begin(), heap.end());
  }
  
  unsigned pivot = threshold - 1; 
  HeapElement<InvList> popped[numShortLists];
  
  while(*(heap.front().iter) != maxUnsigned) {             
    
    unsigned weight = 0, count = 0;    
    unsigned currentId = *(heap.front().iter);
    
    while(currentId == *(heap.front().iter) && !heap.empty()) {
      popped[count++] = heap.front();
      weight += weights[heap.front().invListIndex];
      pop_heap(heap.begin(), heap.end());
      heap.pop_back();
    }
    
    if(weight >= threshold) {
      candis.push_back(Candi(currentId, weight));
      
      // move to the next element
      for(unsigned i = 0; i < count; i++) {
	popped[i].iter++;
	heap.push_back(HeapElement<InvList>(popped[i].iter, popped[i].invListIndex));
	push_heap(heap.begin(), heap.end());
      }      
      continue;
    }
    
    // pop more elements from heap
    while(weight < pivot) {      
      // it is possible for a list to have a weight equals to T
      if(weight+ weights[heap.front().invListIndex] > pivot) break;
      popped[count++] = heap.front();
      weight += weights[heap.front().invListIndex];
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
  for(unsigned i = 0; i < numShortLists; i++)
    invLists[i]->pop_back();
}

template <class InvList>
void 
DivideSkipMerger<InvList>::
getQueryStats(vector<InvList*> &invLists,
	      unsigned threshold,
	      float& shortListsVar,
	      float& shortListsTime,
	      float& longListsVar,
	      float& longListsTime) {
  
  struct timeval t1, t2;
  struct timezone tz;  
  
  if(threshold >= invLists.size() || threshold == 0) return;
  
  sort(invLists.begin(), invLists.end(), DivideSkipMerger<InvList>::cmpInvList);
  
  unsigned longestSize = invLists.back()->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));  
  if(longListsSize >= threshold) longListsSize = threshold - 1;
  
  vector<Candi> candis;
  gettimeofday(&t1, &tz);
  mergeSkipWithoutDupls(invLists,
			threshold-longListsSize,
			longListsSize,
			candis);
  
  gettimeofday(&t2, &tz);  
  shortListsTime = ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) / 1000.0f;
  
  // use binary search or hash search for long lists
  vector<unsigned> results;
  gettimeofday(&t1, &tz);
  
  unsigned stop = invLists.size();
  unsigned start = stop - longListsSize;
  typename InvList::iterator longListsPointers [longListsSize];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = invLists.at(j)->begin();
  
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
      expProbe(longListsPointers[llindex], invLists[j]->end(), start, end, candis[i].id);
      typename InvList::iterator iter = lower_bound(start, end, candis[i].id);      
      longListsPointers[llindex] = start;
      if(iter != invLists[j]->end() && *iter == candis[i].id) {
	count++;
	if(count >= targetCount) {
	  results.push_back(candis[i].id);
	  break;
	}
      }
      else {
	if(count + stop - j - 1 < targetCount) break; // early termination
      }
    }   
  }   
  gettimeofday(&t2, &tz);  
  longListsTime = ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) / 1000.0f;

  // calculate the total sizes of short lists
  float log2 = log(2);
  unsigned shortListsSize = 0;
  for(unsigned i = 0; i < start; i++)
    shortListsSize += invLists[i]->size();
  
  // calculate the log2 sum of the total size of long lists
  float logLongLists = 0.0f;
  for(unsigned i = start; i < stop; i++)
    logLongLists += log(invLists[i]->size()) / log2;
  
  shortListsVar = ((float)shortListsSize / (float)((start))) * logLongLists;
  longListsVar = shortListsSize * ( log(invLists.size()) / log2 );
}


template <class InvList>
void 
DivideSkipMerger<InvList>::
fillEstimationParams(vector<vector<InvList* >* >& invLists,
		     vector<unsigned>& thresholds) {

  vector<float> shortListsXVals;
  vector<float> shortListsYVals;
  vector<float> longListsXVals;
  vector<float> longListsYVals;

  float shortListsXVal = 0.0f;
  float shortListsYVal = 0.0f;
  float longListsXVal = 0.0f;
  float longListsYVal = 0.0f;

  for(unsigned i = 0; i < invLists.size(); i++) {
    vector<InvList*>* currentArrs = invLists.at(i);
    if(currentArrs->size() <= 0) continue;
    getQueryStats(*currentArrs,
		  thresholds.at(i),
		  shortListsXVal,
		  shortListsYVal,
		  longListsXVal,
		  longListsYVal);
    
    shortListsXVals.push_back(shortListsXVal);
    shortListsYVals.push_back(shortListsYVal);
    longListsXVals.push_back(longListsXVal);
    longListsYVals.push_back(longListsYVal);
  }
  
  // perform linear regressions
  linearRegression(shortListsXVals, shortListsYVals, shortListsSlope, shortListsIntercept);
  linearRegression(longListsXVals, longListsYVals, longListsSlope, longListsIntercept);
}


template <class InvList>
float
DivideSkipMerger<InvList>::
getEstimatedQueryTime(vector<InvList*>& invLists,
		      unsigned threshold) const {
  
  if(threshold >= invLists.size()) return 0.0f;

  sort(invLists.begin(), invLists.end(), DivideSkipMerger<InvList>::cmpInvList);
  
  unsigned longestSize = invLists.back()->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));  
  if(longListsSize >= threshold) longListsSize = threshold - 1;
  
  // calculate the total sizes of short lists
  float log2 = log(2);
  unsigned shortListsSize = 0;
  unsigned numShortLists = invLists.size() - longListsSize;
  for(unsigned i = 0; i < numShortLists; i++)
    shortListsSize += invLists[i]->size();
  
  // calculate the log2 sum of the total size of long lists
  float logLongLists = 0.0f;
  for(unsigned i = numShortLists; i < invLists.size();i++)
    logLongLists += log(invLists[i]->size()) / log2; 
  
  float shortListsVar = ((float)shortListsSize / (float)((threshold - longListsSize))) * logLongLists;
  float longListsVar = shortListsSize * ( log(invLists.size()) / log2 );
  
  return shortListsVar*shortListsSlope + longListsVar*longListsSlope + shortListsIntercept + longListsIntercept;
}

#endif
