/*  
    $Id: scancountmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
    
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 05/14/2007

*/

#ifndef _scancountmerger_h_
#define _scancountmerger_h_

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
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

#include "listmerger.h"


/*
  Scan-count algorithm.
  Scan all lists sequentially and
  count the numbers within an array.
  Note that the result set is unsorted.
*/

template <class InvList = Array<unsigned> >
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
  
  void merge_Impl(const vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);

  string getName() {
    return "ScanCountMerger";
  }

  ~ScanCountMerger() {
    delete [] count;
    delete [] queryId;
  }//end  ~ScanCountMerger()

 private:

  short *count;
  unsigned * queryId;
  unsigned currentQuery;

  void mergeWithDuplicateLists(const vector<InvList*> &arrays,
			       const unsigned threshold,
			       vector<unsigned> &results);

};

template <class InvList>
void  
ScanCountMerger<InvList>::merge_Impl(const vector<InvList*> &arrays,
				     const unsigned threshold,
				     vector<unsigned> &results)
{
  if (threshold<=0) return;
  
  if (this->hasDuplicateLists)
    {
      mergeWithDuplicateLists(arrays,threshold,results);
      return;
    }//end if
    
  currentQuery++;
  
  for(unsigned i=0;i<arrays.size();i++){

    Array<unsigned> *v = arrays.at(i);

    for(unsigned j=0;j<v->size();j++)
      {
	unsigned id = v->at(j);

	if ( queryId[id] == currentQuery )
	  {
	    count[id]++;
	  }
	//Otherwise reset the count to 0
	else 
	  {
	    queryId[id] = currentQuery;
	    count[id] = 1;
	  }

	if (count[id] == (short)threshold )
	    {
	      results.push_back(id);
	    }//end if

      }//end for

  }//end while


}//end merge

/*

 Merge with duplicate lists

*/

template <class InvList>
void  
ScanCountMerger<InvList>::
mergeWithDuplicateLists(const vector<InvList*> &arrays,
			const unsigned threshold,
			vector<unsigned> &results)
{
  
  vector<Array<unsigned>*> newArrays;
  
  vector<unsigned> newWeights;
  
  detectDuplicateLists(arrays,newArrays,newWeights);

  currentQuery++;

  set<unsigned> setResult;
  
  for(unsigned i=0;i<newArrays.size();i++){

    Array<unsigned> *v =  newArrays.at(i);

    for(unsigned j=0;j<v->size();j++)
      {
	unsigned id = v->at(j);
	
	if ( queryId[id] == currentQuery )
	  {
	    count[id]+= newWeights.at(i);
	  }
	//Otherwise reset the count to 0
	else 
	  {
	    queryId[id] = currentQuery;
	    count[id] =  newWeights.at(i);
	  }

	if (count[id] >= (short)threshold )
	    {
	      setResult.insert(id);
	      //cout<<"Id "<<id<<" is added as a result. "<<endl; 
	    }//end if

      }//end for
    
  }//end while

  results.insert(results.end(),setResult.begin(),setResult.end());

}//end mergeWithDuplicateLists

#endif
