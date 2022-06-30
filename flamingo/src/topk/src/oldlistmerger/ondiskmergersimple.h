/*
  $Id: ondiskmergersimple.h 5716 2010-09-09 04:27:56Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ondiskmergersimple_h_
#define _ondiskmergersimple_h_

#include "ondiskmergerabs.h"
#include "divideskipmerger.h"

#include <fstream>
#include <tr1/unordered_map>
#include <set>

template <class InvList = Array<unsigned> >
class OnDiskMergerSimple : public OnDiskMergerAbs<OnDiskMergerSimple<InvList>, InvList> {
  
private:
  typedef OnDiskMergerAbs<OnDiskMergerSimple<InvList>, InvList> SuperClass;
  
  DivideSkipMerger<InvList>* dsMerger;
  
  // comparison function for ordering set< pair<unsigned, GramList<InvList>* >, GramListOnDiskCmpFunc >
  struct QueryGramListCmpFunc {
    bool operator()(QueryGramList<InvList>* a, QueryGramList<InvList>* b) {
      if(a->gramCode == b->gramCode)
	return 
	  dynamic_cast<GramListOnDisk<InvList>*>(a->gl)->startOffset 
	  < dynamic_cast<GramListOnDisk<InvList>*>(b->gl)->startOffset;
      return a->gl < b->gl;
    }
  };    

public:   

  vector<unsigned> numberStringsInLeaf;
  
  OnDiskMergerSimple(bool singleFilterOpt = true, bool hasDuplicateLists = false) : SuperClass(singleFilterOpt) { 
    this->dsMerger = new DivideSkipMerger<InvList>(hasDuplicateLists);
  }
  
  bool estimationParamsNeeded_Impl() { 
    return false; 
  }

  void setEstimationParams_Impl(float readInvListTimeSlope, float readInvListTimeIntercept, float readStringAvgTime) { }

  void merge_Impl(const Query& query,
		  vector<vector<QueryGramList<InvList>* >* >& arrays,
		  const unsigned threshold,
		  fstream& invListsFile,
		  unsigned numberFilters,
		  vector<unsigned>& results);

  string getName() {
    return "OnDiskMergerSimple";
  }
  
  ~OnDiskMergerSimple() {
    delete dsMerger;
  } 
};

template<class InvList>
void 
OnDiskMergerSimple<InvList>::
merge_Impl(const Query& query,
	   vector<vector<QueryGramList<InvList>* >* >& arrays,
	   const unsigned threshold,
	   fstream& invListsFile,
	   unsigned numberFilters,
	   vector<unsigned>& results) {
  
  // special case if one filter is applied  
  if(numberFilters == 1 && this->singleFilterOpt && arrays.size() > 1) {

    typedef set<QueryGramList<InvList>*, QueryGramListCmpFunc> GLSet;
    
    tr1::unordered_map<unsigned, GLSet* > groupedGlists;
    // fill the groupedGlists
    for(unsigned i = 0; i < arrays.size(); i++) {    
      vector<QueryGramList<InvList>* >* currentLists = arrays.at(i);
      for(unsigned j = 0; j < currentLists->size(); j++) {
	QueryGramList<InvList>* qgl = currentLists->at(j);
	if(groupedGlists.find(qgl->gramCode) == groupedGlists.end())
	  groupedGlists[qgl->gramCode] = new GLSet();
	groupedGlists[qgl->gramCode]->insert(qgl);
      }
    }
    
    // since a filter is applied each gram has several sorted sublists, 
    // we fill all sublists belonging to the same gram with one sequantial I/O
    typename tr1::unordered_map<unsigned, GLSet*>::iterator iter;
    for(iter = groupedGlists.begin(); iter != groupedGlists.end(); iter++) {
      GLSet* currentLists = iter->second;
	
      // seek to start offset of first sorted sublist
      GramListOnDisk<InvList>* firstGl = dynamic_cast<GramListOnDisk<InvList>*>( (*currentLists->begin())->gl );
      invListsFile.seekg( firstGl->startOffset );
#ifdef DEBUG_STAT
      this->invListSeeks++;
#endif
      
      // now fill sorted sublists in order in one sequential I/O
      // fillArray is implemented NOT to perform a disk seek      
      typename GLSet::iterator setiter;
      GramListOnDisk<InvList>* lastGl = dynamic_cast<GramListOnDisk<InvList>*>( (*currentLists->rbegin())->gl );	
      unsigned bytes = 
	(unsigned)(lastGl->startOffset - firstGl->startOffset) + (lastGl->listSize * InvList::elementSize());
      char* buffer = new char[bytes];
      invListsFile.read(buffer, bytes);    
	
      for(setiter = currentLists->begin(); setiter != currentLists->end(); setiter++) {
	QueryGramList<InvList>* qgl = *setiter;
	GramListOnDisk<InvList>* gl = dynamic_cast<GramListOnDisk<InvList>*>(qgl->gl);
	unsigned offset = (unsigned)(gl->startOffset - firstGl->startOffset);
	gl->fillArray(&buffer[offset]);
      }	
      
#ifdef DEBUG_STAT
      this->invListData += bytes;
#endif
      
      delete[] buffer;
    }
    
    // cleanup groupedGlists;
    for(iter = groupedGlists.begin(); iter != groupedGlists.end(); iter++)
      delete iter->second;
      
    // search leaves one-by-one, 
    // if one filter was applied the lists are already read from disk in the above code
    // if no filter or several filters are applied then getArray() will read the list using a random disk I/O
    for(unsigned i = 0; i < arrays.size(); i++) {
      vector<QueryGramList<InvList>* >* currentLeafLists = arrays.at(i);
      vector<InvList*> lists;
      for(unsigned j = 0; j < currentLeafLists->size(); j++) {
	InvList* arr = currentLeafLists->at(j)->gl->getArray(&invListsFile);
	lists.push_back(arr);      
      }	
      
      switch(this->pmf) {
      case PMF_CSF_REG:
      case PMF_CSF_OPT: {
	vector<unsigned> temp;
	dsMerger->merge(lists, threshold, temp);
	for(unsigned j = 0; j < temp.size(); j++)
	  if(this->charsumFilter->passesFilter(this->queryCharsumStats, 
					       &this->charsumStats[temp.at(j)], 
					       (unsigned)query.threshold)) 
	    results.push_back(temp.at(j));
	
      } break;

      case PMF_LC: {
	vector<unsigned> temp;
	dsMerger->merge(lists, threshold, temp);
	for(unsigned j = 0; j < temp.size(); j++)
	  if(letterCountFilter(this->queryLcStats, 
			       &this->dataLcStats[temp.at(j)], 
			       this->lcCharNum,
			       query.threshold))
	    results.push_back(temp.at(j));
      } break;

      default: {
	dsMerger->merge(lists, threshold, results);
      } break;

      }     
    }
  }
  else {
    // search leaves one-by-one, 
    // if one filter was applied the lists are already read from disk in the above code
    // if no filter or several filters are applied then getArray() will read the list using a random disk I/O
    for(unsigned i = 0; i < arrays.size(); i++) {	      
      vector<QueryGramList<InvList>* >* currentLeafLists = arrays.at(i);	
      vector<InvList*> lists;
      for(unsigned j = 0; j < currentLeafLists->size(); j++) {
	InvList* arr = currentLeafLists->at(j)->gl->getArray(&invListsFile);
	lists.push_back(arr);
#ifdef DEBUG_STAT
	this->invListSeeks++;
	this->invListData += arr->size() * InvList::elementSize();
#endif
      }	
      
      switch(this->pmf) {
      case PMF_CSF_REG:
      case PMF_CSF_OPT: {
	vector<unsigned> temp;
	dsMerger->merge(lists, threshold, temp);
	for(unsigned j = 0; j < temp.size(); j++)
	  if(this->charsumFilter->passesFilter(this->queryCharsumStats, 
					       &this->charsumStats[temp.at(j)], 
					       (unsigned)query.threshold)) 
	    results.push_back(temp.at(j));
	
      } break;

      case PMF_LC: {
	vector<unsigned> temp;
	dsMerger->merge(lists, threshold, temp);
	for(unsigned j = 0; j < temp.size(); j++)
	  if(letterCountFilter(this->queryLcStats, 
			       &this->dataLcStats[temp.at(j)], 
			       this->lcCharNum,
			       query.threshold))
	    results.push_back(temp.at(j));
      } break;

      default: {
	dsMerger->merge(lists, threshold, results);
      } break;

      }      
    }
  }
}

#endif
