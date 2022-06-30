/*
  $Id: ondiskmergerfixedlists.h 4014 2008-09-25 00:19:38Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ondiskmergerfixedlists_h_
#define _ondiskmergerfixedlists_h_

#include "ondiskmergerabs.h"
#include "divideskipmerger.h"

#include <fstream>
#include <tr1/unordered_map>
#include <set>

template <class InvList = Array<unsigned> >
class OnDiskMergerFixedLists : public OnDiskMergerAbs<OnDiskMergerFixedLists<InvList>, InvList> {
  
private:
  typedef OnDiskMergerAbs<OnDiskMergerFixedLists<InvList>, InvList> SuperClass;

  DivideSkipMerger<InvList>* dsMerger;
  
  static bool CmpFunc(const pair<unsigned, GramList<InvList>*>& a, const pair<unsigned, GramList<InvList>*>& b) {
    return dynamic_cast<GramListOnDisk<InvList>*>(a.second)->listSize
      < dynamic_cast<GramListOnDisk<InvList>*>(b.second)->listSize;
  }

  // comparison function for ordering set< pair<unsigned, GramList<InvList>* >, GramListOnDiskCmpFunc >
  struct GramListOnDiskCmpFunc {
    bool operator()(pair<unsigned, GramList<InvList>*>* a, pair<unsigned, GramList<InvList>*>* b) {
      if(a->first == b->first)
	return 
	  dynamic_cast<GramListOnDisk<InvList>*>(a->second)->startOffset 
	  < dynamic_cast<GramListOnDisk<InvList>*>(b->second)->startOffset;
      return a->second < b->second;
    }
  };

public: 
  unsigned additionalListsToRead;  
  unsigned numberStrings;
  
  OnDiskMergerFixedLists(unsigned additionalListsToRead, 
			 bool singleFilterOpt = true, 
			 bool hasDuplicateLists = false) : SuperClass(singleFilterOpt) { 
    this->additionalListsToRead = additionalListsToRead;
    this->dsMerger = new DivideSkipMerger<InvList>(hasDuplicateLists);
  }
  
  bool estimationParamsNeeded_Impl() { return false; }
  void setEstimationParams_Impl(float readInvListTimeSlope, float readInvListTimeIntercept, float readStringAvgTime) { }

  void merge_Impl(vector<vector<pair<unsigned, GramList<InvList>* > >* >& arrays,
		  const unsigned threshold,
		  fstream& invListsFile,
		  unsigned numberFilters,
		  vector<unsigned>& results);
  
  string getName() {
    return "OnDiskMergerFixedLists";
  }

  ~OnDiskMergerFixedLists() {
    delete dsMerger;
  }  
};

template<class InvList>
void 
OnDiskMergerFixedLists<InvList>::
merge_Impl(vector<vector<pair<unsigned, GramList<InvList>* > >* >& arrays,
	   const unsigned threshold,
	   fstream& invListsFile,
	   unsigned numberFilters,
	   vector<unsigned>& results) {
  
  // special case if one filter is applied  
  if(numberFilters == 1 && this->singleFilterOpt && arrays.size() > 1) {
    typedef set< pair<unsigned, GramList<InvList>* >*, GramListOnDiskCmpFunc > GLSet;
    
    tr1::unordered_map<unsigned, GLSet* > groupedGlists;
    // fill the groupedGlists
    for(unsigned i = 0; i < arrays.size(); i++) {    
      vector<pair<unsigned, GramList<InvList>* > >* currentLists = arrays.at(i);
      for(unsigned j = 0; j < currentLists->size(); j++) {
	pair<unsigned, GramList<InvList>* >& p = currentLists->at(j);
	if(groupedGlists.find(p.first) == groupedGlists.end())
	  groupedGlists[p.first] = new GLSet();
	groupedGlists[p.first]->insert(&p);
      }
    }
    
    // since a filter is applied each gram has several sorted sublists, 
    // we fill all sublists belonging to the same gram with one sequantial I/O
    typename tr1::unordered_map<unsigned, GLSet*>::iterator iter;
    for(iter = groupedGlists.begin(); iter != groupedGlists.end(); iter++) {
      GLSet* currentLists = iter->second;
      
      // seek to start offset of first sorted sublist
      GramListOnDisk<InvList>* firstGl = dynamic_cast<GramListOnDisk<InvList>*>( (*currentLists->begin())->second );
      invListsFile.seekg( firstGl->startOffset );
      
#ifdef DEBUG_STAT
      this->numberSeeks++;
#endif	
      
      // now fill sorted sublists in order in one sequential I/O
      // fillArray is implemented NOT to perform a disk seek      
      typename GLSet::iterator setiter;
      for(setiter = currentLists->begin(); setiter != currentLists->end(); setiter++) {
	pair<unsigned, GramList<InvList>*>* p = *setiter;
	GramListOnDisk<InvList>* gl = dynamic_cast<GramListOnDisk<InvList>*>(p->second);
	gl->fillArray(&invListsFile);
      }
    }
    
    // cleanup groupedGlists;
    for(iter = groupedGlists.begin(); iter != groupedGlists.end(); iter++)
      delete iter->second;
    
    // search leaves one-by-one, 
    // if one filter was applied the lists are already read from disk in the above code
    // if no filter or several filters are applied then getArray() will read the list using a random disk I/O
    for(unsigned i = 0; i < arrays.size(); i++) {
      vector<pair<unsigned, GramList<InvList>* > >* currentLeafLists = arrays.at(i);
      vector<InvList*> lists;
      for(unsigned j = 0; j < currentLeafLists->size(); j++) {
	InvList* arr = currentLeafLists->at(j).second->getArray(&invListsFile);
	lists.push_back(arr);      
      }
      
      dsMerger->merge(lists, threshold, results);
    }
  }
  else {
    // search leaves one-by-one, 
    // if one filter was applied the lists are already read from disk in the above code
    // if no filter or several filters are applied then getArray() will read the list using a random disk I/O
    for(unsigned i = 0; i < arrays.size(); i++) {
      vector<pair<unsigned, GramList<InvList>* > >* currentLeafLists = arrays.at(i);
      sort(currentLeafLists->begin(), currentLeafLists->end(), OnDiskMergerFixedLists<InvList>::CmpFunc);
      vector<InvList*> lists;
      unsigned initialLists = currentLeafLists->size() - threshold + 1;
      unsigned listsToRead = initialLists + additionalListsToRead;
      if(listsToRead > currentLeafLists->size()) listsToRead = currentLeafLists->size();
      unsigned finalThreshold = 1 + additionalListsToRead;
      if(finalThreshold > threshold) finalThreshold = threshold;
	
      for(unsigned j = 0; j < listsToRead; j++) {
	InvList* arr = currentLeafLists->at(j).second->getArray(&invListsFile);
#ifdef DEBUG_STAT
	this->numberSeeks++;
#endif
	lists.push_back(arr);
      }	
      
      dsMerger->merge(lists, finalThreshold, results);
    }  
  }
}

#endif
