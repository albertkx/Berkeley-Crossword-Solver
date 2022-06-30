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

    To process the short lists we use improved implementation proposed in:
    Naoaki Okazaki, Jun'ichi Tsujii
    "Simple and Efficient Algorithm for Approximate Dictionary Matching", COLING 2010

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/16/2007

*/

#ifndef _mergeoptmerger_h_
#define _mergeoptmerger_h_

#include "listmerger.h"

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
  MergeOptMerger(bool hasDuplicateLists = false)
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
  
  // process the short lists using the algorithm from
  // Naoaki Okazaki, Jun'ichi Tsujii
  // "Simple and Efficient Algorithm for Approximate Dictionary Matching", COLING 2010

  vector<Candi> candis;
  for(unsigned i = 0; i < numShortLists; i++) {
    vector<Candi> tmp;
    tmp.reserve(candis.size() + invLists[i]->size());
    vector<Candi>::const_iterator candiIter = candis.begin();

    typename InvList::iterator invListIter = invLists[i]->begin();
    typename InvList::iterator invListEnd = invLists[i]->end();
    
    while(candiIter != candis.end() || invListIter != invListEnd) {
      if(candiIter == candis.end() || (invListIter != invListEnd && candiIter->id > *invListIter)) {
	tmp.push_back(Candi(*invListIter, 1));
	invListIter++;
      } else if (invListIter == invListEnd || (candiIter != candis.end() && candiIter->id < *invListIter)) {
	tmp.push_back(Candi(candiIter->id, candiIter->count));
	candiIter++;
      } else {
	tmp.push_back(Candi(candiIter->id, candiIter->count + 1));
	candiIter++;
	invListIter++;
      }
    }
    std::swap(candis, tmp);
  }
  
  // verify candidates on long lists
  unsigned stop = invLists.size();
  unsigned start = numShortLists;  
  typename InvList::iterator longListsPointers[invLists.size() - numShortLists];
  for(unsigned j = start; j < stop; j++) longListsPointers[j - start] = invLists[j]->begin();
  
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
      longListsPointers[llindex] = iter;
      if(*iter == candis[i].id) {
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
  
  // process the short lists using the algorithm from
  // Naoaki Okazaki, Jun'ichi Tsujii
  // "Simple and Efficient Algorithm for Approximate Dictionary Matching", COLING 2010

  vector<Candi> candis;
  for(unsigned i = 0; i < numShortLists; i++) {
    vector<Candi> tmp;
    tmp.reserve(candis.size() + invLists[i]->size());
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
