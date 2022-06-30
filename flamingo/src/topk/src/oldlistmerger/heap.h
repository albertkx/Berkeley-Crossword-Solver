/* 
   $Id: heap.h 5716 2010-09-09 04:27:56Z abehm $

   Copyright (C) 2010 by The Regents of the University of California
	
   Redistribution of this file is permitted under
   the terms of the BSD license.
 
   Date: 05/14/2007
   Author: Jiaheng Lu, Alexander Behm
*/
 	
#ifndef _heap_h_
#define _heap_h_

#include <vector>
#include <list>

#include "util/src/array.h"

using namespace std;

// simple heap class, organizes element pointers into a heap
// the element class must implement the < operator taking references of its type
template<typename Element>
class Heap {
private:
  signed maxElements;
  signed increaseSize;
  signed elementCount;
  Element** data;
  
  void rebuild(signed root) {
    if(root*2+1 < elementCount) {
      // root must have a left child
      signed child = 2 * root + 1;      
      if(root*2+2 < elementCount) {
	signed rightChild = child + 1;    
	if( *(data[rightChild]) < *(data[child]) )
	  child = rightChild;
      }
      
      if( *(data[child]) <  *(data[root]) ) {	
	// swap root and child	
	Element* temp = data[root];
	data[root] = data[child];
	data[child] = temp;
	
	rebuild(child);	
      }      
    }
  }
  
public:
  
  Heap(signed initialSize, signed increaseSize = 5) {
    this->maxElements = initialSize;
    this->data = (Element**)malloc(maxElements*sizeof(Element*));
    this->elementCount = 0;
    this->increaseSize = increaseSize;
  }
  
  Element* head() { return data[0]; }
  bool empty() { return elementCount < 1; }
  unsigned size() { return elementCount; } 
  
  void push(Element* e) {
    if(elementCount >= maxElements) {
      maxElements += increaseSize;
      data = (Element**)realloc(data, maxElements*sizeof(Element*));
    }
    
    data[elementCount] = e;
    
    // trickle new item up to appropriate spot in the tree    
    unsigned place = elementCount;
    unsigned parent = (place-1)/2;    
    while ( (place >= 1) && *(data[place]) < *(data[parent]) ) {
      // swap place and parent       
      Element* temp = data[parent];
      data[parent] = data[place];
      data[place] = temp;
      
      place = parent;      
      parent = (place-1)/2;      
    }    
    
    elementCount++;
  }
  
  void pop() {
    if(elementCount <= 0) return;
    elementCount--;
    data[0] = data[elementCount];
    rebuild(0);       
  }
  
  ~Heap() { 
    if(data) free(data); 
  }
};

void showlistUnsigned(list<unsigned> *v);

void makeInitialHeap(vector<int> *heap,
		     const vector< Array<unsigned>*> &lists);

unsigned getMinInHeap(vector<int> *heap);

void deleteMAXUnsignedfromEachList
    (const vector< Array<unsigned>* > &lists);

void heapReplaceHead(unsigned newData,
		     unsigned data[],
		     unsigned index[],
		     unsigned size);

void heapInsert(unsigned newData,
                unsigned newIndex,
                unsigned data[],
                unsigned index[],
                unsigned &size);

void heapDelete(unsigned data[],
                unsigned index[],
                unsigned &size );

void addMAXUnsigned2EachList(const vector< Array<unsigned>* > &lists,
			     unsigned MAXUnsigned);

        	
void makeInitialHeap(unsigned dataHeap[],
		     unsigned indexHeap[],
		     const vector< Array<unsigned>* > &lists);


void deleteMAXUnsignedfromEachList(const vector<Array<unsigned>*> &lists);

	
#endif
