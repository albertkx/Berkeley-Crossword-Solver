/*
  $Id: heap.cc 5716 2010-09-09 04:27:56Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
  
  Author: Jiaheng Lu
  Date: 04/14/2007

*/


/*

This file provides all basic operations for heap.

*/

#include "heap.h"

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

using namespace std;


/* Test if the node has
   the right child in heap
*/


bool hasRightChildInHeap(unsigned nodeIndex, unsigned size){

  if (nodeIndex*2+2 >= size)
    return false;
  else
    return true;
  

}//end hasRightChildInHeap


bool isLeafInHeap(unsigned nodeIndex, unsigned size){

  if (nodeIndex*2+1 >= size)
    return true;
  else
    return false;

}//end isLeafInHeap

/*
  rebuild the heap
  after the deletion
*/


void heapRebuild(unsigned data[], 
		 unsigned index[],
                 unsigned root,
                 unsigned size)

{
  if (! isLeafInHeap( root, size))//if the root is not a leaf 
    {
      //root must have a left child
      unsigned child = 2 * root + 1;

      if  (hasRightChildInHeap(root, size))
	{

          unsigned rightChild = child +1;
          
          if ( data[rightChild] < data[child])
	    child = rightChild ;

	}//end if
   
      if (  data[child] <  data[root]) 
	{

	  // swap root and child

	  unsigned temp = data[root];
	  data[root] = data[child];
	  data[child] = temp;

	  temp = index[root];
	  index[root] = index[child];
	  index[child] = temp;

	  heapRebuild( data, index, child, size);

	}//end if
          
    }//end if 

  //else root is a leaf, so we are done.

}//end heapRebuild 




/*  
    Replace the current
    head node to a new node.
    The size of heap 
    is not changed.

*/

void heapReplaceHead(unsigned newData,
		     unsigned data[],
		     unsigned index[],
		     unsigned size)

{

  // replace the newItem as the top of the tree

  data[0] = newData;

  heapRebuild(data,index,0,size);

}//end heapReplaceHead


/*  Insert a node to heap;
    First insert it to the end of heap;
    Then  trickle new item up to appropriate spot

*/

void heapInsert(unsigned newData,
                unsigned newIndex,
                unsigned data[],
                unsigned index[],
                unsigned &size)

{

  // insert newItem into the bottom of the tree

  data[size] = newData;
  index[size] = newIndex;

  // trickle new item up to appropriate spot in the tree

  unsigned place = size;

  unsigned parent = (place-1)/2;


  
  while ( (place >= 1) &&
          data[place] < data[parent]  
        )       
    {
      // swap place and parent 

      unsigned temp = data[parent];
      data[parent] = data[place];
      data[place] = temp;

               
      temp = index[parent];
      index[parent] = index[place];
      index[place] = temp;

      place = parent;

      parent = (place-1)/2;
  
    }//end while

  ++size;
 

}//end heapInsert

void printHeap( unsigned heap[], unsigned size ){

  for(unsigned i=0;i<size;i++)
    cout<< " " << heap[i] ;

  cout <<endl;

}//end printHeap


void heapDelete(unsigned data[],
                unsigned index[],
                unsigned &size )
 
{
 
  data[0] = data[size-1];
  index[0] = index[size-1];

  --size;

  heapRebuild(data, index, 0, size );

}//end heapDelete


void  deleteMAXUnsignedfromEachList(const vector<Array<unsigned>*> &lists)
{
	
  for(unsigned k=0;k<unsigned(lists.size());k++)
    { 
		
      Array<unsigned> *onelist = lists.at(k);
      
      onelist->removeLastElement();
        
    }//end for
	
}//end deleteMAXUnsignedfromEachList


void addMAXUnsigned2EachList(const vector<Array<unsigned>* > &lists,
			     unsigned MAXUnsigned){
	
  for(unsigned k=0;k< (unsigned)lists.size();k++){ 
	
    Array<unsigned> *onelist = lists.at(k);
	 
    onelist->append(MAXUnsigned);
        
  }//end for

}//end addMAXUnsigned2EachList	
	
        	
void makeInitialHeap(unsigned dataHeap[],
		     unsigned indexHeap[],
		     const vector< Array<unsigned>* > &lists)
{
  unsigned size = 0;

  for(unsigned i=0;i< unsigned(lists.size());i++)
    {	
      unsigned t = lists.at(i)->at(0);	
      heapInsert(t,i,dataHeap,indexHeap,size);

    }//end for
	   
}//end makeInitialHeap
