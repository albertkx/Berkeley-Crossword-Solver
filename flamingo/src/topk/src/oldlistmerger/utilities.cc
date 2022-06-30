/*  
    $Id: utilities.cc 5716 2010-09-09 04:27:56Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 05/11/2007

*/

#include "utilities.h"

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
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
#include <cassert>

#include "heap.h"
#include "showutil.h"
	
using namespace std;

typedef set<unsigned,less<unsigned> >  UnsignedSet;


unsigned  getTotalLogSize(const vector<Array<unsigned>*> &lists)
{
  unsigned totalSize = 0;

  unsigned numberOfLists = lists.size();

  for(unsigned i=0;i< numberOfLists;i++)
       totalSize +=(unsigned)ceil(log(lists.at(i)->size()));
   
  return  totalSize;
  

}//end getTotalSize


unsigned  getTotalSize(const vector<Array<unsigned>*> &lists)
{
  unsigned totalSize = 0;

  unsigned numberOfLists = lists.size();

  
  for(unsigned i=0;i< numberOfLists;i++)
    {  
      totalSize +=lists.at(i)->size();
      // cout <<"i is " << i <<
      //" size is "<< lists.at(i)->size() <<endl;
    }//end for
  return  totalSize;
  

}//end getTotalSize              

/*
  This function is for the test of 
  consistent of two different merge
  algorithms.

  The order of results does not matter.

*/

bool testConsistent(const vector<unsigned> &result1,
		    const vector<unsigned> &result2)
{
  if (result1.size() != result2.size())
    return false;
  
  UnsignedSet s1 (result1.begin(),result1.end());
  UnsignedSet s2 (result2.begin(),result2.end());

  UnsignedSet::iterator ite1 = s1.begin();
  UnsignedSet::iterator ite2 = s2.begin();

  while (ite1!= s1.end() )
    {
      if (*ite1 != *ite2)
	return false;

      ite1++;
      ite2++;

    }//end while

  return true;
  
}// end testConsistent


bool binarySearch(Array<unsigned> *v,
		  unsigned value,
		  unsigned start,
		  unsigned end)
{
  int low = start;
  int high = end - 1;
  int middle = (low+high+1)/2;

  bool found = false;
  
  do
    {
      if (value == v->at(middle))
	return true;
      else if ( value < v->at(middle))
	high = middle - 1;
      else
	low = middle + 1;

      middle = (low+high+1)/2;

    } while ((low <=high) &&
	     (found == false)
	     );

  return found;

}//end binarySearch

/*
  Get statics data frbom running

*/

void  getStatistics(const vector<Array<unsigned>*> &arrays,
		    unsigned threshold,
		    unsigned longListsSize,
		    const vector<unsigned> &partialResults,
		    const vector<unsigned> &results)
{
  cout<<"~~~~~~~~~~~~~~"<<endl;
  cout << "Partial results size is " << partialResults.size() << endl;
  cout << "Final results size is " << results.size() << endl;
  cout << "Threshold is "<<threshold<<"; Long lists size is "<<longListsSize <<endl;
  cout << "# of lists is " <<arrays.size() << endl;
  cout<<"**************"<<endl;

}//end getStatistics


/*

This function is for DividedSkip algorithm
Seperate to long and short lists sets.
For short lists, use binarySearch;
For long lists, use hash to search.

*/


void  splitTwoSets(vector<Array<unsigned>*> *longLists,
		   vector<Array<unsigned>*> *shortLists,
		   const unsigned threshold,
		   const vector<Array<unsigned>*> &originalLists,
		   unsigned sortedIndex[],
		   unsigned longListsSize)
{

  unsigned originalListSize =  originalLists.size();

  for(unsigned i=0;i<originalListSize;i++)
    {
      if (i<longListsSize)
	longLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      else
	shortLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      
    }//end for

}//end splitTwoSets


void  splitTwoSizeSets(vector<unsigned> *longLists,
		       vector<unsigned> *shortLists,
		       const unsigned threshold,
		       const vector<unsigned> &originalLists,
		       unsigned sortedIndex[],
		       unsigned longListsSize)
{

  unsigned originalListSize =  originalLists.size();

  for(unsigned i=0;i<originalListSize;i++)
    {
      if (i<longListsSize)
	longLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      else
	shortLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      
    }//end for

}//end splitTwoSets



void  splitTwoSetsWithDuplicates(vector<Array<unsigned>*> &longLists,
				 vector<Array<unsigned>*> &shortLists,
				 vector<unsigned> &longListsWeights,
				 vector<unsigned> &shortListsWeights,
				 const vector<Array<unsigned>*> &originalLists,
				 const vector<unsigned> &originalWeights,
				 const unsigned shortListsSize)
{

  unsigned originalListSize = originalLists.size();

  for(unsigned i=0;i<originalListSize;i++)
    {
      if (i<shortListsSize) {
	shortLists.push_back(originalLists.at(i));
	shortListsWeights.push_back(originalWeights.at(i));
      }
      else {
	longLists.push_back(originalLists.at(i));
	longListsWeights.push_back(originalWeights.at(i));
      }
    }//end for

  // ORIGINAL CODE BELOW BY JIAHENG
  // FIXED BY ALEX (ABOVE CODE)
  /*
  unsigned originalListSize =  originalLists.size();
  
  unsigned currentWeight = 0;

  unsigned i = 0;
  
  while ((currentWeight+originalWeights.at(i)) < shortListsSize )
    {
      currentWeight += originalWeights.at(i);
      shortLists.push_back(originalLists.at(i));
      shortListsWeights.push_back(originalWeights.at(i));
      i++;
    }//end for

  shortLists.push_back(originalLists.at(i));
  shortListsWeights.push_back(shortListsSize-currentWeight);
  
  unsigned listSizeforLonger =
    currentWeight+originalWeights.at(i)-shortListsSize;
  
  if ( listSizeforLonger >0)
    {
      longLists.push_back(originalLists.at(i));
      longListsWeights.push_back(listSizeforLonger);  
    }

  i++;

  for(; i<originalListSize;i++)
    {
      longLists.push_back(originalLists.at(i));
      longListsWeights.push_back(originalWeights.at(i));  
    }//end for

  */

}//end  splitTwoSetsWithDuplicates

/*

This function is for MergeOpt algorithm
Seperate to long and short lists sets.

*/

void separateTwoSets(vector<Array<unsigned>*> *longLists,
		     vector<Array<unsigned>*> *shortLists,
		     const unsigned threshold,
		     const vector<Array<unsigned>*> &originalLists,
		     unsigned sortedIndex[])
{
  unsigned originalListSize =  originalLists.size();

  for(unsigned i=0;i<originalListSize;i++)
    {
      if (i<threshold-1)
	longLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      else
	shortLists->push_back(originalLists.at(sortedIndex[originalListSize-i-1]));
      
    }//end for

}//end separateTwoSets




/*
  Use binray search to search the value
  in search set of lists;
  change the count 
*/

void  binarySearchSet(unsigned &count,
		      vector<Array<unsigned>* > *lists,
		      unsigned data)
{
  vector<Array<unsigned>*>::iterator ite =
    lists->begin();

  while(ite != lists->end())
      {
	Array<unsigned> *v = *ite;
	if (binarySearch(v,data,0,v->size()))
	  count++;
	ite++;
      }//end while

}//end binarySearchSet

void sortBySize(const vector<unsigned> &allLists,
		unsigned sortedIndex [] )
{
  vector<unsigned>::const_iterator ite = allLists.begin();
  
  unsigned sizeHeap[allLists.size()]; 
  unsigned indexHeap[allLists.size()];

  unsigned index = 0;

  unsigned heapSize = 0;

  while (ite != allLists.end())
    {
      unsigned sizeOfList = *ite;

      heapInsert(sizeOfList,index++,sizeHeap,indexHeap,heapSize);
    
      ite++;

    }//end while

  index = 0;

  while (heapSize > 0)
    {
      sortedIndex[index++] = indexHeap[0];
      heapDelete(sizeHeap,indexHeap,heapSize);
    }//end while

}//end sortInvertedList



/*
  Sort the inverted list with increasing order
  according to the their sizes
  by heapSort

  This function is used in MergeOpt.

*/


void sortBySizeOfLists(const vector<Array<unsigned>*> &allLists,
		       unsigned sortedIndex [] )
{
  vector<Array<unsigned>*>::const_iterator ite = allLists.begin();
  
  unsigned sizeHeap[allLists.size()]; 
  unsigned indexHeap[allLists.size()];

  unsigned index = 0;

  unsigned heapSize = 0;

  while (ite != allLists.end())
    {
      unsigned sizeOfList = (*ite)->size();

      heapInsert(sizeOfList,index++,sizeHeap,indexHeap,heapSize);
    
      ite++;

    }//end while

  index = 0;

  while (heapSize > 0)
    {
      sortedIndex[index++] = indexHeap[0];
      heapDelete(sizeHeap,indexHeap,heapSize);
    }//end while

}//end sortInvertedList


/* Insert items to heap
   This function is used in binarySearchMerger
*/

void  insertToHeaps(unsigned dataHeap[],
		    unsigned indexHeap[],
		    unsigned &heapSize,
		    const vector< Array<unsigned>* > &lists,
		    unsigned  pointersIndexList[],			  
		    unsigned  vectorIndexContainer[],
		    unsigned  containerSize)

{
  for(unsigned i=0;i<containerSize;i++)
    {
      unsigned index = vectorIndexContainer[i];
      
      unsigned position = pointersIndexList[index];

      unsigned newData = 
	(lists.at(index))->at(position);

      heapInsert(newData,index,dataHeap,indexHeap, heapSize);

    }//end for

}//end insertToHeaps

/*
  This fucntion skips nodes
  for mergebinarySeach algorithm
*/

void  skipNodes (const vector<Array<unsigned>* > &lists,
		 unsigned vectorIndexContainer[],
		 unsigned containerSize, 
		 unsigned pivotData,
		 unsigned pointersIndexList[])

{
    
  for(unsigned i=0;i<containerSize;i++)
    {

      unsigned j = vectorIndexContainer[i];

      unsigned oldPosition =  pointersIndexList[j];
          
      pointersIndexList[j] =
	lists.at(j)->binarySearch(pivotData,oldPosition);

      // cout<< "new data" << lists.at(j)->at( pointersIndexList[j]) <<
      //	" old data " << pivotData <<endl;

      //assert(lists.at(j)->at( pointersIndexList[j]) >= pivotData);
    
    }//end for
   
}//end skipNodes

/* 
   This function can be deleted for the final release
*/

void  CountSkipNodes (const vector<Array<unsigned>* > &lists,
		      unsigned vectorIndexContainer[],
		      unsigned containerSize, 
		      unsigned pivotData,
		      unsigned pointersIndexList[],
		      unsigned &elementScanned)

{
    
  for(unsigned i=0;i<containerSize;i++)
    {

      unsigned j = vectorIndexContainer[i];

      unsigned oldPosition =  pointersIndexList[j];
          
      pointersIndexList[j] =
	lists.at(j)->binarySearch(pivotData,oldPosition);

      elementScanned += (unsigned)ceil(0.01*log(lists.at(j)->size()));

      // cout<< "new data" << lists.at(j)->at( pointersIndexList[j]) <<
      //	" old data " << pivotData <<endl;

      //assert(lists.at(j)->at( pointersIndexList[j]) >= pivotData);
    
    }//end for
   
}//end skipNodes



unsigned max(unsigned s1, unsigned s2)
{

  if (s1>s2)
    return s1;
  else
    return s2;

}//end max

/* 
 This function selects the shorest
 string for computing the thresholds 
 in filters.

*/

unsigned  shorestStringSize(const vector<string> &strings
			    )
{
  unsigned minSize = ~0;

  for(unsigned i=0;i<(unsigned)strings.size();i++)
    {
      unsigned s = strings.at(i).length();
      
      if (s < minSize)
	minSize = s;
      
    }//end for

  return minSize ;
  
}//end 



/* 
  This function is only for MergeSkip algorithm.

  Use MergeSkip algorithm to process
  short lists in DivideSkip algorithm.

*/

void  mergeSkipShortLists(const vector<Array<unsigned>*> &arrays,
			     const unsigned threshold,
			     vector<unsigned> &results,
			     vector<unsigned> &counters)
{

  //const unsigned maxUnsigned = 0x10000000 - 2; 
  const unsigned maxUnsigned = 0xFFFFFFFF; 

  unsigned numberOfInvertedList = arrays.size();

  if (threshold>numberOfInvertedList)
    return; // no answer

  unsigned pointersIndex [numberOfInvertedList];
        
  for(unsigned k=0;k<numberOfInvertedList;k++)
      pointersIndex[k]=0;

  addMAXUnsigned2EachList(arrays,  maxUnsigned);

  unsigned  dataHeap [numberOfInvertedList] ;
  unsigned  indexHeap [numberOfInvertedList] ; 
        
  makeInitialHeap(dataHeap,indexHeap,arrays);

  unsigned sizeOfHeaps = numberOfInvertedList;

  unsigned  pivot = threshold -1 ;
 
  while ( dataHeap[0]  <  maxUnsigned ){
           
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
	counters.push_back(containerSize);
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
  

    }//end while (containerSize < pivot )
                  
    //printArray( vectorIndexContainer,containerSize);

    // cout<< "Pivot node is " <<  dataHeap[0] << endl;

    skipNodes(arrays, vectorIndexContainer, containerSize,
	      dataHeap[0], pointersIndex);

    //cout<<"After skip, current nodes are : " <<endl;

    //showCurrentNodes(&pointersNode, &pointersIndex, numberOfInvertedList);

    insertToHeaps(dataHeap,
		  indexHeap,
		  sizeOfHeaps,
		  arrays,
		  pointersIndex,
		  vectorIndexContainer,
		  containerSize);

  }//end while  ( thresholdHeap[0]  < MAX)

  deleteMAXUnsignedfromEachList(arrays);
  
}//end  mergeSkipShortLists


void  mergeSkipShortListsWithDuplicate(const vector<Array<unsigned>*> &arrays,
				       const vector<unsigned> &weights,
				       const unsigned threshold,
				       vector<unsigned> &results,
				       vector<unsigned> &counters)
{

  const unsigned maxUnsigned = 0xFFFFFFFF; 

  unsigned numberOfInvertedList = arrays.size();

  unsigned pointersIndex [numberOfInvertedList];
        
  for(unsigned k=0;k<numberOfInvertedList;k++)
      pointersIndex[k]=0;

  addMAXUnsigned2EachList(arrays,  maxUnsigned);

  unsigned  dataHeap [numberOfInvertedList] ;
  unsigned  indexHeap [numberOfInvertedList] ; 
        
  makeInitialHeap(dataHeap,indexHeap,arrays);

  unsigned sizeOfHeaps = numberOfInvertedList;

  unsigned  pivot = threshold -1 ;
  
  while ( dataHeap[0]  <  maxUnsigned ){
           
    //cout<< " Current heaps are : " << endl;
    //printArrayUnsigned (dataHeap, sizeOfHeaps);
    //printArrayUnsigned (indexHeap, sizeOfHeaps);
             
    // Container of vector indexes which should be moved to the next position
    unsigned vectorIndexContainer[numberOfInvertedList];
    unsigned containerWeight = 0, containerSize = 0;
    
    // Check if we can get the result 
    unsigned minData =  dataHeap[0];
    
    while (minData == dataHeap[0] && containerSize < numberOfInvertedList )
      {         
	vectorIndexContainer[containerSize++] = indexHeap[0];

	containerWeight += weights[indexHeap[0]];

	heapDelete(dataHeap,indexHeap,sizeOfHeaps);

	
      }//end while
    
    if (containerWeight >= threshold) // we got the result
      {
	counters.push_back(containerWeight);
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
    
    while (containerWeight < pivot){
      
      // FIX BY ALEX
      // it is possible for a list to have a weight equals to T
      if(containerWeight + weights[indexHeap[0]] > pivot) break;

      vectorIndexContainer[containerSize++] = indexHeap[0];
             
      containerWeight += weights[indexHeap[0]];

      heapDelete(dataHeap,indexHeap,sizeOfHeaps);

    }//end while (containerWeight < pivot )
                  
    //printArray( vectorIndexContainer,containerSize);

    // cout<< "Pivot node is " <<  dataHeap[0] << endl;

    skipNodes(arrays, vectorIndexContainer, containerSize,
	      dataHeap[0], pointersIndex);

    //cout<<"After skip, current nodes are : " <<endl;

    //showCurrentNodes(&pointersNode, &pointersIndex, numberOfInvertedList);

    insertToHeaps(dataHeap,
		  indexHeap,
		  sizeOfHeaps,
		  arrays,
		  pointersIndex,
		  vectorIndexContainer,
		  containerSize);

  }//end while  ( thresholdHeap[0]  < MAX)

  deleteMAXUnsignedfromEachList(arrays);
  
}//end   mergeSkipShortListsWithDuplicate


/*
void detectDuplicateLists(const vector<Array<unsigned>*> &arrays,
			  vector<Array<unsigned>*> &newArrays,
			  vector<unsigned> &newWeights)
{
  
  unsigned sizeOfInvertedLists = arrays.size();
  
  unsigned sortedIndex[sizeOfInvertedLists];
  sortBySizeOfLists(arrays,sortedIndex);//increasing order

  Array<unsigned> *currentArray=arrays.at(sortedIndex[0]);
  unsigned currentCount = 1;

  for(unsigned i=1;i<sizeOfInvertedLists;i++)
    {
      Array<unsigned> *iArray = arrays.at(sortedIndex[i]);

      if (iArray == currentArray)
	currentCount++;
      else
	{
	  newArrays.push_back(currentArray);
	  newWeights.push_back(currentCount);
	  currentCount = 1;
	  currentArray = iArray;
	}//end if
      
    }//end for

  newArrays.push_back(currentArray);
  newWeights.push_back(currentCount);

    
}//end detectDuplicateLists
*/

// BUGFIX BY ALEX
void detectDuplicateLists(const vector<Array<unsigned>*> &arrays,
			  vector<Array<unsigned>*> &newArrays,
			  vector<unsigned> &newWeights) {
  
  unsigned sizeOfInvertedLists = arrays.size();  
  unsigned sortedIndex[sizeOfInvertedLists];
  sortBySizeOfLists(arrays,sortedIndex);
  
  // we need to take care of unequal lists with the same size
  // do exhaustive search within each list length group
  set<uintptr_t> arraysAdded;
  for(unsigned i=0;i<sizeOfInvertedLists;i++) {   
    Array<unsigned> *currentArray=arrays.at(sortedIndex[i]);
    uintptr_t arrAddr = (uintptr_t)currentArray;
    unsigned currentCount = 1;    
    
    // if the array has not been added previously
    if(arraysAdded.find(arrAddr) == arraysAdded.end()) {
      // search all arrays with the same length for identical pointers
      for(unsigned j = i + 1; j < sizeOfInvertedLists; j++) {       
	Array<unsigned> *iArray = arrays.at(sortedIndex[j]);
	if(iArray->size() != currentArray->size()) break;
	if(iArray == currentArray) currentCount++;
      }    
      // add the array
      newArrays.push_back(currentArray);
      newWeights.push_back(currentCount);
      arraysAdded.insert(arrAddr);
    }
  }  

  // ALEX DEBUG
  /*
  unsigned weightSum = 0;
  for(unsigned i = 0; i < newArrays.size(); i++) {
    Array<unsigned>* tmp = newArrays.at(i);
    bool contains = false;
    for(unsigned j = 0; j < tmp->size(); j++) {      
      if(tmp->at(j) == 49) {
	contains = true;
	break;
      }
    }
    if(contains) {
      cout << "LIST: " << (unsigned)newArrays.at(i) << " " << newWeights.at(i) << endl;
      weightSum += newWeights.at(i);
    }
  }
  cout << "WEIGHTSUM: " << weightSum << endl;
  */
}
