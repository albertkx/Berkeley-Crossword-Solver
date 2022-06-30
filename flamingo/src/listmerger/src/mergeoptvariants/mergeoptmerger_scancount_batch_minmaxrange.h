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

#ifndef _mergeoptscmerger_h_
#define _mergeoptscmerger_h_

#include "listmerger.h"
#include <cstring>

template <class InvList = vector<unsigned> >
class MergeOptMerger : public ListsMerger<MergeOptMerger<InvList>, InvList> {
private:

  // using list weights
  void mergeWithDupls(vector<InvList*> &invLists,
		      unsigned threshold,
		      vector<unsigned> &results);
  
  // no list weights
  void mergeWithoutDupls(vector<InvList*> &invLists,
			 unsigned threshold,
			 vector<unsigned> &results);  
  
public:
  // maxId is not used
  MergeOptMerger(unsigned maxId = 100000, bool hasDuplicateLists = false)
    : ListsMerger<MergeOptMerger<InvList>, InvList>(hasDuplicateLists) {}
  
  void merge_Impl(vector<InvList*> &invLists,
		  unsigned threshold,
		  vector<unsigned> &results);
  
  string getName() {
   return "MergeOptMerger";
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

  /*************************************************************/
  /* process the short lists with a counter array - by jongik  */
  /*************************************************************/
  // verify the range of ids and allocate the appropriate size of memory
  unsigned min = 0xFFFFFFFF;
  unsigned max = 0;;
  for(unsigned i = 0; i < numShortLists; i++){
    unsigned size = invLists[i]->size();
    if(size > 0){
      if(min > invLists[i]->at(0)) min = invLists[i]->at(0);
      if(max < invLists[i]->at(size - 1)) max = invLists[i]->at(size - 1);
    }
  }

  if(min > max) return;

  unsigned short* sc = new unsigned short[(max - min)+1];
  memset(sc, 0, sizeof(unsigned short) * (max-min+1));

  // scan short lists and count the number of occurrences
  for(unsigned i = 0; i < numShortLists; i++)
    for(unsigned j = 0; j < invLists[i]->size(); j++)
      sc[invLists[i]->at(j) - min]++;

  // collect candidates - leverage the effect of the cache for the sc array
  vector<Candi> candis;
  for(unsigned id = 0; id < max-min+1; id++)
    if(sc[id] > 0) candis.push_back(Candi(id+min, sc[id]));

  delete [] sc;
  /*************************************************************/

  
  // process long lists
  unsigned stop = invLists.size();
  for(unsigned j = numShortLists; j < stop; j++) {
    if(candis.size() < stop - j) break; // termination heuristic
    
    vector<Candi> tmp;
    tmp.reserve(candis.size());
    typename InvList::iterator listIter = invLists[j]->begin();
        
    for(unsigned i = 0; i < candis.size(); i++) {
      typename InvList::iterator start, end;
      expProbe(listIter, invLists[j]->end(), start, end, candis[i].id);      
      listIter = lower_bound(start, end, candis[i].id);
      if(*listIter == candis[i].id) {
	candis[i].count++;
	if(candis[i].count >= threshold) results.push_back(candis[i].id);
	else tmp.push_back(candis[i]);
      }
      else {
	if(candis[i].count + stop - j > threshold) {
	  tmp.push_back(candis[i]);
	}
      }
    }
    
    std::swap(candis, tmp);       
  }
  
  // add surviving candidates
  for(unsigned i = 0; i < candis.size(); i++)
    results.push_back(candis[i].id);
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
  
  // process short lists with a counter array - begin
  unsigned min = 0xFFFFFFFF;
  unsigned max = 0;;
  for(unsigned i = 0; i < numShortLists; i++){
    unsigned size = invLists[i]->size();
    if(size > 0){
      if(min > invLists[i]->at(0)) min = invLists[i]->at(0);
      if(max < invLists[i]->at(size - 1)) max = invLists[i]->at(size - 1);
    }
  }

  if(min > max) return;

  unsigned short* sc = new unsigned short[(max - min)+1];
  memset(sc, 0, sizeof(unsigned short) * (max-min+1));

  for(unsigned i = 0; i < numShortLists; i++)
    for(unsigned j = 0; j < invLists[i]->size(); j++)
      sc[invLists[i]->at(j) - min] += newWeights[i];

  
  vector<Candi> candis;
  for(unsigned id = 0; id < max-min+1; id++)
    if(sc[id] > 0) candis.push_back(Candi(id+min, sc[id]));

  delete [] sc;
  // process short lists with a counter array - end


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
