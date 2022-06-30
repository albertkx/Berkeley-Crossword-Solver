/*  
    $Id: heapmerger.h 4887 2009-12-16 22:01:55Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 05/11/2007

*/

#ifndef _heapmerger_h_
#define _heapmerger_h

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

/*
  Heap-merge algorithm.
  Maintain a heap for all head elements
  of lists and sequentially scan all lists.
*/

template <class InvList = Array<unsigned> >
class HeapMerger : public ListsMerger<HeapMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "HeapMerger";   
 }
};

template <class InvList>
void 
HeapMerger<InvList>::
merge_Impl(const vector<InvList* > &arrays,
	   const unsigned threshold,
	   vector<unsigned> &results)
{

  const unsigned MAXUnsigned = ~0;     
	 
  unsigned sizeOfInvertedLists = arrays.size();

  if ((sizeOfInvertedLists < threshold)|| (threshold == 0))
    return;

  unsigned pointers [sizeOfInvertedLists];
        
  for(unsigned k=0;k<sizeOfInvertedLists;k++)
    pointers[k] = 0;
         	
  addMAXUnsigned2EachList(arrays,  MAXUnsigned);

  unsigned dataHeap[sizeOfInvertedLists];
  unsigned indexHeap[sizeOfInvertedLists];

  makeInitialHeap(dataHeap,indexHeap,arrays);

  unsigned heapSize = sizeOfInvertedLists;

  unsigned freq = 0;

  unsigned previous = dataHeap[0];

  while(true){
        	
    unsigned smallest = dataHeap[0];
    unsigned smallestIndex = indexHeap[0];
  
    if (smallest == MAXUnsigned)
      { 
	deleteMAXUnsignedfromEachList(arrays);
	return;
      }
    else
      {	

	pointers[smallestIndex]++;
	
	unsigned newData = arrays.at(smallestIndex)->at(pointers[smallestIndex]);

	heapReplaceHead(newData,
			dataHeap,indexHeap,
			heapSize);

	if (smallest==previous)
	  {	  
	    freq++;  
	  }//if
	else
	  {
	    freq = 1;
	    previous = smallest; 
	  }
	    
      }//end else

    if (freq >= threshold) {
      if (results.size()==0)
	results.push_back(smallest);
      else
	 if (results.back() != smallest)
	   results.push_back(smallest);
    }

  }//end else        	
}


#endif
