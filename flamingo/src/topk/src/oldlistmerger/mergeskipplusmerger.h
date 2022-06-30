/*  
    $Id: mergeskipplusmerger.h 4887 2009-12-16 22:01:55Z abehm $  

    Copyright (C) 2010 by The Regents of the University of California
    
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 01/17/2008

*/

#ifndef _mergeskipplusmerger_h_
#define _mergeskipplusmerger_h_

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
  MergeSkipPlus algorithm.
  Improve MergeSkip by avoid push and pop heap twice for some cases
*/

template <class InvList = Array<unsigned> >
class MergeSkipPlusMerger : public ListsMerger<MergeSkipPlusMerger<InvList>, InvList>
{
 public:
  void merge_Impl(const vector<InvList*> &arrays,
		  const unsigned threshold,
		  vector<unsigned> &results);
 string getName() {
   return "MergeSkipPlusMerger";
 }
};

template <class InvList>
void  
MergeSkipPlusMerger<InvList>::
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

  //cache the index for records equal to the T'th record in heap
  unsigned interContainer[numberOfInvertedList];
  unsigned interContainerSize = 0;
               
  while ( dataHeap[0]  <  MAXUnsigned ){
    
    //cout<< " Current heaps are : " << endl;
    //printArrayUnsigned (dataHeap, sizeOfHeaps);
    //printArrayUnsigned (indexHeap, sizeOfHeaps);
    
    // Container of vector indexes which should be moved to the next position
    unsigned vectorIndexContainer[numberOfInvertedList];
    unsigned containerSize = 0;
    

    // Check if we can get the result 
    unsigned minData =  dataHeap[0];


    while (minData == dataHeap[0] && sizeOfHeaps > 0 )
      {         
	vectorIndexContainer[containerSize++] = indexHeap[0];
	
	heapDelete(dataHeap,indexHeap,sizeOfHeaps);
	
      }//end while

    //assert ( containerSize+interContainerSize+sizeOfHeaps == numberOfInvertedList);
    
    if (containerSize+interContainerSize >= threshold) // we got the result
      {
	results.push_back(minData);
	
	//cout<< "We get a result, rule ID is " << minData <<", count is " << containerSize <<endl;
	
        //move to the next element
	for(unsigned i=0;i<containerSize;i++)
	  {
	    unsigned j = vectorIndexContainer[i];
	    pointersIndex[j]++ ;
	  }//end for
	
        for(unsigned i=0;i<interContainerSize;i++)
	  {
	    unsigned j = interContainer[i];
	    pointersIndex[j]++ ;
	  }//end for
	
	insertToHeaps(dataHeap,indexHeap,
		      sizeOfHeaps, 
		      arrays,pointersIndex,
		      vectorIndexContainer,
		      containerSize);
	
        insertToHeaps(dataHeap,indexHeap,
		      sizeOfHeaps, 
		      arrays,pointersIndex,
		      interContainer,
		      interContainerSize);
	
        interContainerSize = 0;
             
	continue;

      }//end if
    
    // pop more elements from heap
    // and skip nodes
    
    unsigned poppedMax = indexHeap[0], poppedMaxSize = 0;
    
    while (containerSize < threshold-1){
      
      vectorIndexContainer[containerSize++] = indexHeap[0];

      if (poppedMax == indexHeap[0])
	poppedMaxSize ++;
      else
           {
             poppedMax =  indexHeap[0];
             poppedMaxSize = 1;
           }//end else    
      
      heapDelete(dataHeap,indexHeap,sizeOfHeaps);
      
    }//end while 

    //process records in interContainer            
    if (interContainerSize > 0) {

      skipNodes(arrays,interContainer,interContainerSize,
		dataHeap[0],pointersIndex );
      
      insertToHeaps(dataHeap,indexHeap,
		    sizeOfHeaps,
		    arrays,pointersIndex,
		    interContainer,interContainerSize);
    }//end if
   
    //move records to new interContainer
    interContainerSize = 0;
    if (poppedMax == indexHeap[0]) {
      //cout<<"Move to inter container"<<endl;
      for(unsigned i=0;i<poppedMaxSize;i++)
	interContainer[interContainerSize++] = vectorIndexContainer[containerSize-i-1];
      containerSize = containerSize - poppedMaxSize;
   }//end if
    
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
