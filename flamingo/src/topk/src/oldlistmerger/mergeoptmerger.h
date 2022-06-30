/*  
    $Id: mergeoptmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    This imeplementation merges multiple lists by 
    the algorithm proposed in SIGMOD'04.
    "Effiicent set joins on similarity predicates"
    Separate lists to two disjoint sets
    according to the lengths of lists.
    Heap-merge short lists and
    binary search for longer lists.

    The lists are assumed to
    be sorted in an ascending order.

    Author: Jiaheng Lu 
    Date: 05/16/2007

*/

#ifndef _mergeoptmerger_h_
#define _mergeoptmerger_h_

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <list>

#include "listmerger.h"
#include "heap.h"
#include "showutil.h"
#include "utilities.h"
#include "util/src/array.h"


/*
  MergeOpt algorithm.
  Separate lists to two disjoint sets
  according to the lengths of lists.
  Heap-merge short lists and
  binary search for longer lists.
*/

template <class InvList = Array<unsigned> >
class MergeOptMerger : public ListsMerger<MergeOptMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "MergeOptMerger";
 }
};

template <class InvList>
void  
MergeOptMerger<InvList>::
merge_Impl(const vector<InvList*> &arrays,
	   const unsigned threshold,
	   vector<unsigned> &results)
{

  const unsigned MAXUnsigned = ~0;
 	 
  unsigned sizeOfInvertedLists = arrays.size();

  if ((sizeOfInvertedLists < threshold) || (threshold == 0))
    return;

  unsigned sortedIndex[sizeOfInvertedLists];
  sortBySizeOfLists(arrays,sortedIndex);

  vector<Array<unsigned>*> longLists,shortLists;

  separateTwoSets(&longLists,&shortLists,threshold,
		  arrays, sortedIndex);

  addMAXUnsigned2EachList(shortLists,MAXUnsigned); 

  unsigned shortListSize = shortLists.size();

  unsigned dataHeap[shortListSize];
  unsigned indexHeap[shortListSize];

  makeInitialHeap(dataHeap,indexHeap,shortLists);

  unsigned heapSize = shortListSize;

  unsigned freq = 0;

  unsigned previous = dataHeap[0];

  unsigned pointers [shortListSize];

  for(register unsigned i=0;i<shortListSize;i++)
    pointers[i] = 0;

  while(true){
         	
    unsigned smallest = dataHeap[0];
    unsigned smallestIndex = indexHeap[0];
        
    if (smallest == MAXUnsigned)
      { 
	deleteMAXUnsignedfromEachList(shortLists);

	 //binary search in long lists for previous	   
	binarySearchSet(freq,&longLists,previous);
	   
	//add to result set
	if (freq >= threshold) {
	  if (results.size()==0)
	    results.push_back(previous);
	  else
	    //check duplicate
	    if (results.back() != previous)
	      results.push_back(previous);
	}
	    
	return;
      }
    else
      {	
	pointers[smallestIndex]++;

	unsigned newData = shortLists.at(smallestIndex)->at(pointers[smallestIndex]);

	heapReplaceHead(newData,
			dataHeap,indexHeap,
			heapSize);

	if (smallest==previous)
	  {	  
	    freq++;  
	  }//if
	else
	  {
	    //binary search in long lists for previous	   
	    binarySearchSet(freq,&longLists,previous);
	   
	    //add to result set
	    if (freq >= threshold) {
	      if (results.size()==0)
		results.push_back(previous);
	      else
		if (results.back() != previous)
		  results.push_back(previous);
	    }

	    //set to a new counter
	    freq = 1;
	    previous = smallest; 
	  }//end else
	    
      }//end else

  }//end while

}//end merge

#endif
