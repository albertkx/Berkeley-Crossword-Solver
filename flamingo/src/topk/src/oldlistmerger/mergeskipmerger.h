/*  
    $Id: mergeskipmerger.h 4887 2009-12-16 22:01:55Z abehm $  

    Copyright (C) 2010 by The Regents of the University of California
    
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 05/14/2007

*/

#ifndef _mergeskipmerger_h_
#define _mergeskipmerger_h_

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
#include "utilities.h"
#include "heap.h"
#include "showutil.h"

/*
  MergeSkip algorithm.
  Exploit the threshold and
  use binary search to skip elements.
*/

template <class InvList = Array<unsigned> >
class MergeSkipMerger : public ListsMerger<MergeSkipMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "MergeSkipMerger";
 }
};

template <class InvList>
void  
MergeSkipMerger<InvList>::
merge_Impl(const vector<InvList*> &arrays,
	   const unsigned threshold,
	   vector<unsigned> &results)
{
  const unsigned MAXUnsigned = ~0; 
  
  unsigned numberOfInvertedList = arrays.size();

  if (threshold>numberOfInvertedList)
    return; // no answer

  unsigned pointersIndex [numberOfInvertedList];
        
  for(unsigned k=0;k<numberOfInvertedList;k++)
      pointersIndex[k]=0;

  addMAXUnsigned2EachList(arrays,  MAXUnsigned);

  unsigned  dataHeap [numberOfInvertedList] ;
  unsigned  indexHeap [numberOfInvertedList] ; 
        
  makeInitialHeap(dataHeap,indexHeap,arrays);

  unsigned sizeOfHeaps = numberOfInvertedList;

  //for (unsigned i = 0; i < sizeOfHeaps; i++)
  //cout << "(" << dataHeap[i] << ", " << indexHeap[i] << ") ";
  //cout << endl;
  
  //threshold for skip in heap
  unsigned  pivot = threshold -1 ;
               
  while ( dataHeap[0]  <  MAXUnsigned ){
           
    //cout<< " Current heaps are : " << endl;
    //printArrayUnsigned (dataHeap, sizeOfHeaps);
    //printArrayUnsigned (indexHeap, sizeOfHeaps);
             
    // Container of vector indexes which should be moved to the next position
    unsigned vectorIndexContainer[numberOfInvertedList];
    unsigned containerSize = 0;


    // Check if we can get the result 
    unsigned minData =  dataHeap[0];


    while (minData == dataHeap[0] && containerSize < numberOfInvertedList )
      {         
	vectorIndexContainer[containerSize++] = indexHeap[0];
              
	heapDelete(dataHeap,indexHeap,sizeOfHeaps);
  
      }//end while

    if (containerSize >= threshold) // we got the result
      {
	results.push_back(minData);

	//cout<< "We get a result, rule ID is " << minData <<", count is " << containerSize <<endl;
             
        //move to the next element
	for(unsigned i=0;i<containerSize;i++)
	  {
	    unsigned j = vectorIndexContainer[i];
	    pointersIndex[j]++ ;
	  }//end for

	insertToHeaps(dataHeap,indexHeap,
		      sizeOfHeaps, 
		      arrays,pointersIndex,
		      vectorIndexContainer,
		      containerSize);
             
	continue;

      }//end if
    
    // pop more elements from heap
    // and skip nodes

    while (containerSize < pivot){

      vectorIndexContainer[containerSize++] = indexHeap[0];
             
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);
  

    }//end while 
                  
 
    skipNodes(arrays,vectorIndexContainer,containerSize,
	      dataHeap[0],pointersIndex );

   
    insertToHeaps(dataHeap,indexHeap,
		  sizeOfHeaps,
		  arrays,pointersIndex,
		  vectorIndexContainer,containerSize);
      

  }//end while ( thresholdHeap[0]  < MAX)

  deleteMAXUnsignedfromEachList(arrays);


}//end merge

#endif
