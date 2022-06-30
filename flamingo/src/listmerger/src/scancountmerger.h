/*  
    $Id: scancountmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
    
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu, Alexander Behm
    Date: 05/14/2007

*/

#ifndef _scancountmerger_h_
#define _scancountmerger_h_

#include "listmerger.h"


/*
  Scan-count algorithm.
  Scan all lists sequentially and
  count the numbers within an array.
  Note that the result set is unsorted.
*/

template <class InvList = vector<unsigned> >
class ScanCountMerger : public ListsMerger<ScanCountMerger<InvList>, InvList>
{
private:
  unsigned maxObjectID;

public:  
  ScanCountMerger(unsigned maxObjectID, bool hasDuplicateLists = false) {
    this->maxObjectID = maxObjectID;
    this->hasDuplicateLists = hasDuplicateLists;
    
    count = new short[ maxObjectID ];
    queryId = new unsigned [maxObjectID ];
    
    for(unsigned i=0;i<maxObjectID;i++)
      queryId[i] = 0;
    
    currentQuery = 0;
  }     
  
  void merge_Impl(const vector<InvList*> &invLists,
		  const unsigned threshold,
		  vector<unsigned> &results);

  string getName() {
    return "ScanCountMerger";
  }

  ~ScanCountMerger() {
    delete [] count;
    delete [] queryId;
  }

 private:

  short *count;
  unsigned * queryId;
  unsigned currentQuery;
};

template <class InvList>
void  
ScanCountMerger<InvList>::merge_Impl(const vector<InvList*> &invLists,
				     const unsigned threshold,
				     vector<unsigned> &results) {
  if (threshold<=0) return;
  
  currentQuery++;
  
  for(unsigned i = 0; i < invLists.size(); i++){

    vector<unsigned> *v = invLists.at(i);

    for(unsigned j = 0; j < v->size(); j++) {
	unsigned id = v->at(j);
	if(queryId[id] == currentQuery) {
	  count[id]++;
	}
	else {
	  queryId[id] = currentQuery;
	  count[id] = 1;
	}

	if(count[id] == (short)threshold) {
	  results.push_back(id);
	}
      }
  }
}
#endif
