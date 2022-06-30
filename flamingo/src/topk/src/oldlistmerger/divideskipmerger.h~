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

#include <time.h>
#include <sys/time.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <functional>
#include <iterator>
#include <list>
#include <algorithm>

#include <stdint.h>
#include "listmerger.h"
#include "utilities.h"
#include "util/src/misc.h"
#include "heap.h"

// DivideSkip algorithm,
// which combine MergeOpt and MergeSkip algorithm.

template <class InvList = Array<unsigned> >
class DivideSkipMerger : public ListsMerger<DivideSkipMerger<InvList>, InvList> {
 private:  
  // for estimation purposes
  float shortListsSlope;
  float shortListsIntercept;
  float longListsSlope;
  float longListsIntercept;  
  // for calculating L
  float mu;  
  
  // exponential probe, returns iterator to first element greater or equal to value
  inline typename InvList::iterator expProbe(typename InvList::iterator start, 
					     typename InvList::iterator end, 
					     unsigned value) const;  
   
  // mergeskip with weights given
  void mergeSkip(const vector<InvList*> &arrays,
		 const vector<unsigned> &weights,
		 const unsigned threshold,
		 const unsigned longLists,
		 vector<unsigned> &results,
		 vector<unsigned> &counters);

  // detect lists with identical references and return vector of distinct lists and their weights
  void detectDuplicateLists(vector<InvList*> &arrays,
			    vector<InvList*> &newArrays,
			    vector<unsigned> &newWeights);
  
  // we consider list weights
  void mergeWithDuplicateLists(vector<InvList*> &arrays,
			       const unsigned threshold,
			       vector<unsigned> &results);

  // helper for query time estimatiom
  void getQueryStats(const vector<InvList*> &arrays,
		     const unsigned threshold,
		     float& shortListsVar,
		     float& shortListsTime,
		     float& longListsVar,
		     float& longListsTime);

 public:   
  DivideSkipMerger(bool hasDuplicateLists = false) { 
    this->hasDuplicateLists = hasDuplicateLists; 
    this->shortListsSlope = 0.0f;
    this->shortListsIntercept = 0.0f;
    this->longListsSlope = 0.0f;
    this->longListsIntercept = 0.0f;
    this->mu = 0.01;
  }
 
  void merge_Impl(vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);		  
    
  void fillEstimationParams(const vector<vector<InvList* >* >& arrays,
			    const vector<unsigned>& thresholds);  

  float getEstimatedQueryTime(const vector<InvList*>& arrays,
			      const unsigned threshold) const;

  static bool cmpInvList(const InvList* a, const InvList* b) {
    return a->size() < b->size();
  }
  
  // mergeskip, every list has equal weight
  void mergeSkip(const vector<InvList*> &arrays,
		 const unsigned threshold,
		 const unsigned longLists,
		 vector<unsigned> &results,
		 vector<unsigned> &counters);

  string getName() {
    return "DivideSkipMerger";
  }   
};

template <class InvList>
typename InvList::iterator
DivideSkipMerger<InvList>::
expProbe(typename InvList::iterator start, 
	 typename InvList::iterator end, 
	 unsigned value) const {
  unsigned c = 0;    
  for(;;) {      
    typename InvList::iterator iter = start + (1 << c);
    if(iter >= end) return end;
    else if(*iter >= value) return iter;
    c++;
  }  
}

template <class InvList>
void 
DivideSkipMerger<InvList>::
mergeSkip(const vector<InvList*> &arrays,
	  const unsigned threshold,
	  const unsigned longLists,
	  vector<unsigned> &results,
	  vector<unsigned> &counters) {
  
  const typename InvList::elementType maxUnsigned = (typename InvList::elementType)0xFFFFFFFF;
  unsigned numberOfInvertedList = arrays.size() - longLists;
  
  if (threshold > numberOfInvertedList) return;
  
  unsigned pointersIndex [numberOfInvertedList];  
  memset(pointersIndex, 0, numberOfInvertedList * sizeof(unsigned));

  // add Max unsigned
  for(unsigned i = 0; i < numberOfInvertedList; i++) 
    arrays.at(i)->append(maxUnsigned);

  unsigned dataHeap [numberOfInvertedList];
  unsigned indexHeap [numberOfInvertedList]; 
        
  // make initial heap
  unsigned size = 0;
  for(unsigned i = 0; i < numberOfInvertedList; i++) {	    
    unsigned t = arrays.at(i)->at(0);
    heapInsert(t,i,dataHeap,indexHeap,size);    
  }

  unsigned sizeOfHeaps = numberOfInvertedList;
  unsigned pivot = threshold - 1; 
  unsigned vectorIndexContainer[numberOfInvertedList];

  while( dataHeap[0]  <  maxUnsigned ) {                       
    
    // Container of vector indexes which should be moved to the next position 
    unsigned containerSize = 0;    
    unsigned minData =  dataHeap[0];
    
    while(minData == dataHeap[0] && containerSize < numberOfInvertedList) {         
      vectorIndexContainer[containerSize++] = indexHeap[0];      
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);	
    }
    
    if(containerSize >= threshold) {
      counters.push_back(containerSize);
      results.push_back(minData);
		
      //move to the next element
      for(unsigned i = 0; i < containerSize; i++) {
	unsigned j = vectorIndexContainer[i];
	pointersIndex[j]++;
      }
      
      // insert to heaps
      for(unsigned i = 0; i < containerSize; i++) {
	unsigned index = vectorIndexContainer[i];       
	unsigned position = pointersIndex[index]; 	
	unsigned newData = arrays.at(index)->at(position);
	heapInsert(newData,index,dataHeap,indexHeap, sizeOfHeaps);
      }                 
      continue;      
    }
    
    // pop more elements from heap
    // and skip nodes  
    while (containerSize < pivot) {
      vectorIndexContainer[containerSize++] = indexHeap[0];             
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);  
    }
                  
    // skip nodes
    for(unsigned i = 0; i < containerSize; i++) {
      unsigned j = vectorIndexContainer[i];
      unsigned oldPosition = pointersIndex[j];          
      pointersIndex[j] = arrays.at(j)->binarySearch(dataHeap[0], oldPosition);
    }

    // insert to heaps
    for(unsigned i = 0; i < containerSize; i++) {
      unsigned index = vectorIndexContainer[i];       
      unsigned position = pointersIndex[index]; 	
      unsigned newData = arrays.at(index)->at(position);
      heapInsert(newData,index,dataHeap,indexHeap, sizeOfHeaps);
    }
  }

  // remove last MaxUnsigned element
  for(unsigned i = 0; i < numberOfInvertedList; i++) 
    arrays.at(i)->removeLastElement();
}


template <class InvList>
void 
DivideSkipMerger<InvList>::
merge_Impl(vector<InvList*> &arrays,
	   const unsigned threshold,
	   vector<unsigned> &results) {
  
  unsigned sizeOfInvertedLists = arrays.size();
  if ((sizeOfInvertedLists < threshold) || ((signed)threshold <= 0))
    return;
  
  if (this->hasDuplicateLists) {
    mergeWithDuplicateLists(arrays,threshold,results);
    return;
  }  

  sort(arrays.begin(), arrays.end(), DivideSkipMerger<InvList>::cmpInvList);

  unsigned longestSize = arrays.at(arrays.size()-1)->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1)) ;  
  if (longListsSize >= threshold) longListsSize = threshold - 1;  

  // use mergeskip to search the short lists
  vector<unsigned> partialResults, counters;
  mergeSkip(arrays,
	    threshold-longListsSize,
	    longListsSize,
	    partialResults,
	    counters);
  
  unsigned stop = arrays.size();
  unsigned start = stop - longListsSize;
  typename InvList::iterator longListsPointers [longListsSize];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = arrays.at(j)->begin();
  
  // use exponential probe + binary search for long lists
  for(register unsigned i = 0; i < partialResults.size(); i++) {
    unsigned ID = partialResults.at(i);
    if (counters.at(i) < threshold) {
      unsigned freq = 0;
      for(unsigned j = start; j < stop; j++) {	
	unsigned llindex = j - start;
	typename InvList::iterator start = longListsPointers[llindex];
	typename InvList::iterator end = expProbe(start, arrays.at(j)->end(), ID);	
	typename InvList::iterator iter = lower_bound(start, end, ID);
	if(*iter == ID) freq++;
	longListsPointers[llindex] = iter;	
      }
      if(freq+counters.at(i) >= threshold) results.push_back(ID);
    }
    else results.push_back(ID); 
  }
}
 
template <class InvList>
void 
DivideSkipMerger<InvList>::
getQueryStats(const vector<InvList*> &arrays,
	      const unsigned threshold,
	      float& shortListsVar,
	      float& shortListsTime,
	      float& longListsVar,
	      float& longListsTime) {
  
  struct timeval t1, t2;
  struct timezone tz;  

  unsigned sizeOfInvertedLists = arrays.size();  
  if (sizeOfInvertedLists < threshold) return;

  unsigned sortedIndex[sizeOfInvertedLists];
  sortBySizeOfLists(arrays,sortedIndex);
  
  unsigned longestSize = arrays.at(sortedIndex[arrays.size()-1])->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));  
  if (longListsSize >= threshold) longListsSize = threshold - 1;

  vector<InvList*> longLists, shortLists;
  splitTwoSets(&longLists,&shortLists,threshold,
	       arrays,sortedIndex,longListsSize); 

  vector<unsigned> partialResults, counters;
  gettimeofday(&t1, &tz);
  mergeSkipShortLists(shortLists,
		      threshold-longListsSize,
		      partialResults,
		      counters);
  gettimeofday(&t2, &tz);  
  shortListsTime = ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) / 1000.0f;
  
  unsigned longListsPointers [longLists.size()];
  memset(longListsPointers, 0, sizeof(unsigned)*longLists.size());

  // use binary search or hash search for long lists
  vector<unsigned> results;
  gettimeofday(&t1, &tz);
  for(register unsigned i=0;i<(unsigned)partialResults.size();i++) {
    unsigned ID = partialResults.at(i);
    if (counters.at(i)<threshold) {
      unsigned freq = 0;
      for(unsigned j=0;j<(unsigned)longLists.size();j++) {
	unsigned pos = longLists.at(j)->binarySearch(ID,longListsPointers[j]);
	longListsPointers[j]=pos;	
	if (pos < longLists.at(j)->size() ) {
	  if (longLists.at(j)->at(pos)==ID)
	    freq++;
	}	
      }
      if(freq+counters.at(i) >= threshold) results.push_back(ID);
    }
    else results.push_back(ID);      
  }
  gettimeofday(&t2, &tz);  
  longListsTime = ((t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)) / 1000.0f;

  // calculate the total sizes of short lists
  float log2 = log(2);
  unsigned shortListsSize = 0;
  for(unsigned i=0;i<shortLists.size();i++)
    shortListsSize += shortLists.at(i)->size();

  // calculate the log2 sum of the total size of long lists
  float logLongLists = 0.0f;
  for(unsigned i=0;i<longLists.size();i++)
    logLongLists += log(longLists.at(i)->size()) / log2; 
  
  shortListsVar = ((float)shortListsSize / (float)((threshold - longLists.size()))) * logLongLists;
  longListsVar = shortListsSize * ( log(sizeOfInvertedLists) / log2 );
}


template <class InvList>
void 
DivideSkipMerger<InvList>::
fillEstimationParams(const vector<vector<InvList* >* >& arrays,
		     const vector<unsigned>& thresholds) {

  vector<float> shortListsXVals;
  vector<float> shortListsYVals;
  vector<float> longListsXVals;
  vector<float> longListsYVals;

  float shortListsXVal = 0.0f;
  float shortListsYVal = 0.0f;
  float longListsXVal = 0.0f;
  float longListsYVal = 0.0f;

  for(unsigned i = 0; i < arrays.size(); i++) {
    vector<InvList*>* currentArrs = arrays.at(i);
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
getEstimatedQueryTime(const vector<InvList*>& arrays,
		      const unsigned threshold) const {

  unsigned sizeOfInvertedLists = arrays.size();  
  if (sizeOfInvertedLists < threshold) return 0.0f;

  unsigned sortedIndex[sizeOfInvertedLists];
  sortBySizeOfLists(arrays,sortedIndex);
  
  unsigned longestSize = arrays.at(sortedIndex[arrays.size()-1])->size();
  unsigned longListsSize = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));  
  if (longListsSize >= threshold) longListsSize = threshold - 1;

  vector<InvList*> longLists, shortLists;
  splitTwoSets(&longLists,&shortLists,threshold,
	       arrays,sortedIndex,longListsSize); 

  // calculate the total sizes of short lists
  float log2 = log(2);
  unsigned shortListsSize = 0;
  for(unsigned i=0;i<shortLists.size();i++)
    shortListsSize += shortLists.at(i)->size();

  // calculate the log2 sum of the total size of long lists
  float logLongLists = 0.0f;
  for(unsigned i=0;i<longLists.size();i++)
    logLongLists += log(longLists.at(i)->size()) / log2; 
  
  float shortListsVar = ((float)shortListsSize / (float)((threshold - longLists.size()))) * logLongLists;
  float longListsVar = shortListsSize * ( log(sizeOfInvertedLists) / log2 );

  return shortListsVar*shortListsSlope + longListsVar*longListsSlope + shortListsIntercept + longListsIntercept;
}

template <class InvList>
void  
DivideSkipMerger<InvList>::
detectDuplicateLists(vector<InvList*> &arrays,
		     vector<InvList*> &newArrays,
		     vector<unsigned> &newWeights) {
  
  // we need to take care of unequal lists with the same size
  // do exhaustive search within each list length group
  sort(arrays.begin(), arrays.end(), DivideSkipMerger<InvList>::cmpInvList);
  set<uintptr_t> arraysAdded;
  for(unsigned i = 0 ; i < arrays.size(); i++) {   
    InvList *currentArray = arrays.at(i);
    uintptr_t arrAddr = (uintptr_t)currentArray;
    unsigned currentCount = 1;
    
    // if the array has not been added previously
    if(arraysAdded.find(arrAddr) == arraysAdded.end()) {
      // search all arrays with the same length for identical pointers
      for(unsigned j = i + 1; j < arrays.size(); j++) {       
	InvList *iArray = arrays.at(j);
	if(iArray->size() != currentArray->size()) break;
	if(iArray == currentArray) currentCount++;
      }    
      // add the array
      newArrays.push_back(currentArray);
      newWeights.push_back(currentCount);
      arraysAdded.insert(arrAddr);
    }
  }  

  // ALEX DEBUG
  /*
  unsigned weightSum = 0;
  for(unsigned i = 0; i < newArrays.size(); i++) {
    InvList* tmp = newArrays.at(i);
    bool contains = false;
    for(unsigned j = 0; j < tmp->size(); j++) {      
      if(tmp->at(j) == 49) {
	contains = true;
	break;
      }
    }
    if(contains) {
      cout << "LIST: " << (unsigned)newArrays.at(i) << " " << newWeights.at(i) << endl;
      weightSum += newWeights.at(i);
    }
  }
  cout << "WEIGHTSUM: " << weightSum << endl;
  */
}


template <class InvList>
void  
DivideSkipMerger<InvList>::
mergeSkip(const vector<InvList*> &arrays,
	  const vector<unsigned> &weights,
	  const unsigned threshold,
	  const unsigned longLists,
	  vector<unsigned> &results,
	  vector<unsigned> &counters) {
  
  const typename InvList::elementType maxUnsigned = (typename InvList::elementType)0xFFFFFFFF;
  unsigned numberOfInvertedList = arrays.size() - longLists;
  unsigned pointersIndex [numberOfInvertedList];
  memset(pointersIndex, 0, numberOfInvertedList * sizeof(unsigned));
  
  // add Max unsigned
  for(unsigned i = 0; i < numberOfInvertedList; i++) 
    arrays.at(i)->append(maxUnsigned);

  unsigned dataHeap [numberOfInvertedList];
  unsigned indexHeap [numberOfInvertedList]; 
        
  // make initial heap
  unsigned size = 0;
  for(unsigned i = 0; i < numberOfInvertedList; i++) {	    
    unsigned t = arrays.at(i)->at(0);
    heapInsert(t,i,dataHeap,indexHeap,size);    
  }

  unsigned sizeOfHeaps = numberOfInvertedList;
  unsigned pivot = threshold - 1; 
  unsigned vectorIndexContainer[numberOfInvertedList];
  
  while ( dataHeap[0]  <  maxUnsigned ) {             
             
    // Container of vector indexes which should be moved to the next position
    unsigned containerWeight = 0, containerSize = 0;
    
    // Check if we can get the result 
    unsigned minData =  dataHeap[0];    
    while (minData == dataHeap[0] && containerSize < numberOfInvertedList ) {         
      vectorIndexContainer[containerSize++] = indexHeap[0];      
      containerWeight += weights[indexHeap[0]];      
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);      	
    }
    
    if (containerWeight >= threshold) {
      counters.push_back(containerWeight);
      results.push_back(minData);      	
      //move to the next element
      for(unsigned i=0;i<containerSize;i++) {
	unsigned j = vectorIndexContainer[i];
	pointersIndex[j]++;
      }
      
      // insert to heaps
      for(unsigned i = 0; i < containerSize; i++) {
	unsigned index = vectorIndexContainer[i];       
	unsigned position = pointersIndex[index]; 	
	unsigned newData = arrays.at(index)->at(position);
	heapInsert(newData,index,dataHeap,indexHeap, sizeOfHeaps);
      }                     
      continue;      
    }
    
    // pop more elements from heap
    // and skip nodes    
    while(containerWeight < pivot) {      
      // FIX BY ALEX
      // it is possible for a list to have a weight equals to T
      if(containerWeight + weights[indexHeap[0]] > pivot) break;
      vectorIndexContainer[containerSize++] = indexHeap[0];             
      containerWeight += weights[indexHeap[0]];
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);
    }                    

    // skip nodes
    for(unsigned i = 0; i < containerSize; i++) {
      unsigned j = vectorIndexContainer[i];
      unsigned oldPosition = pointersIndex[j];          
      pointersIndex[j] = arrays.at(j)->binarySearch(dataHeap[0], oldPosition);
    }

    // insert to heaps
    for(unsigned i = 0; i < containerSize; i++) {
      unsigned index = vectorIndexContainer[i];       
      unsigned position = pointersIndex[index]; 	
      unsigned newData = arrays.at(index)->at(position);
      heapInsert(newData,index,dataHeap,indexHeap, sizeOfHeaps);
    }
  }
  // remove last MaxUnsigned element
  for(unsigned i = 0; i < numberOfInvertedList; i++) 
    arrays.at(i)->removeLastElement();
}


template <class InvList>
void  
DivideSkipMerger<InvList>::
mergeWithDuplicateLists(vector<InvList*> &arrays,
			const unsigned threshold,
			vector<unsigned> &results) {

  vector<InvList*> newArrays;  
  vector<unsigned> newWeights;  
  detectDuplicateLists(arrays, newArrays, newWeights);

  //assume that newArray should be sorted according to the length increasing
  unsigned longestSize = newArrays.at(newArrays.size()-1)->size();  
  unsigned targetLongListWeight = (unsigned)floor(threshold / (mu*log2(longestSize) + 1));
  if(targetLongListWeight >= threshold) targetLongListWeight = threshold - 1;
  
  // START BUGFIX BY ALEX
  // THE CORRECT CHECK IS BELOW
  unsigned longListsSize = 0;
  unsigned totalLongListWeight = 0;
  for(unsigned i = 0; i < newArrays.size(); i++) {
    unsigned currentWeight = newWeights.at( newArrays.size() - 1 - i );
    totalLongListWeight += currentWeight;
    if(totalLongListWeight >= targetLongListWeight) {
      if(totalLongListWeight >= threshold) {
	longListsSize = i;
	totalLongListWeight -= currentWeight;      
      }
      else {
	longListsSize = i + 1;	
      }
      break;
    }
  }
  // END BUGFIX BY ALEX
  
  // ALEX DEBUG
  /*
  cout << "THRESHOLD: " << threshold << endl;
  cout << "L: " << longListsSize << endl;
  unsigned llsum = 0;
  for(unsigned i = 0; i < longLists.size(); i++) {
    cout << "LONGLIST:  " 
	 << (unsigned)longLists.at(i) 
	 << " " 
	 << longListsWeights.at(i) 
	 << " " 
	 << longLists.at(i)->size() 
	 << endl;
    llsum += longListsWeights.at(i);
  }
  cout << "LLSUM: " << llsum << endl;

  unsigned slsum = 0;
  for(unsigned i = 0; i < shortLists.size(); i++) {
    cout << "SHORTLIST: " 
	 << (unsigned)shortLists.at(i) 
	 << " " 
	 << shortListsWeights.at(i) 
	 << " " 
	 << shortLists.at(i)->size() << endl;
    slsum += shortListsWeights.at(i);
  }
  cout << "SLSUM: " << slsum << endl;
  */


  // FIXED BY ALEX: new threshold is T - totalLongListWeight
  //cout << "BEFORE MERGING SHORT LISTS" << endl;
  //cout << "TOTALLONGLISTWEIGHT: " << threshold << " " << totalLongListWeight << endl;
  //cout << "ST IS: " << threshold-totalLongListWeight << endl;
  vector<unsigned> partialResults, counters;
  mergeSkip(newArrays,
	    newWeights,
	    threshold-totalLongListWeight,
	    longListsSize,
	    partialResults,
	    counters);
  //cout << "AFTER MERGING SHORT LISTS" << endl;    

  unsigned stop = newArrays.size();
  unsigned start = stop - longListsSize;
  typename InvList::iterator longListsPointers [longListsSize];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = newArrays.at(j)->begin();

  for(register unsigned i = 0; i < partialResults.size(); i++) {
    unsigned ID = partialResults.at(i);    
    if (counters.at(i) < threshold) {
      unsigned freq = 0;
      for(unsigned j = start; j < stop; j++) {	
	unsigned llindex = j - start;
	typename InvList::iterator start = longListsPointers[llindex];
	typename InvList::iterator end = expProbe(start, newArrays.at(j)->end(), ID);	
	typename InvList::iterator iter = lower_bound(start, end, ID);
	if(*iter == ID) freq += newWeights.at(j);
	longListsPointers[llindex] = iter;	
      }
      if(freq+counters.at(i) >= threshold) results.push_back(ID);
    }
    else results.push_back(ID); 
  }  
}

#endif
