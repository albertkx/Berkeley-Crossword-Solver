/*  
 $Id: showutil.cc 5716 2010-09-09 04:27:56Z abehm $ 


 Copyright (C) 2010 by The Regents of the University of California
	
 Redistribution of this file is permitted under
 the terms of the BSD license.

 Author: Jiaheng Lu
 Date: 05/14/2007
 
*/

#include "showutil.h"

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


#include "heap.h"


using namespace std;


template<typename T>

void printArray (const T *array, unsigned count){


  for(unsigned i=0; i<count; i++)
    cout << array[i] << " ";

  cout << endl;

}//end printArray


template<typename S>

void printVector (const S *v, unsigned count)
{


  for(unsigned i=0; i<count; i++)
    cout << v->at(i) << " ";

  cout << endl;

}//end printVector

void printVectorUnsigned(const vector<unsigned> *v, unsigned count)
{

  for(unsigned i=0; i<count; i++)
    cout << v->at(i) << " ";

  cout << endl;

}//end printVectorUnsigned

void printArrayUnsigned(const unsigned *array, unsigned count)
{


  for(unsigned i=0; i<count; i++)
    cout << array[i] << " ";

  cout << endl;


}//end printArrayUnsigned


void showVectorInt(vector<int> *v){
	
	for(unsigned i =0 ;i<(*v).size();i++)
	  cout << (*v)[i] << " " ;
	  
	  cout << endl;
	
}//end showVectorInt

void showStringVector(const vector<string> &v){
	
  for(unsigned i =0 ;i<v.size();i++)
    cout << v.at(i) << ";  "  ;
  
  cout << endl;
	
}//end showStringVector


void showArrayUnsigned(Array<unsigned> *v){
	
	for(unsigned i =0 ;i<(*v).size();i++)
	  cout << v->at(i) << " " ;
	  
	  cout << endl;
	
}//end showArrayUnsigned

void showVectorUnsigned(vector<unsigned> *v){
	
  for(unsigned i =0 ;i<(*v).size();i++)
    cout << v->at(i) << " " ;
  
  cout << endl;
	
}//end showVectorUnsigned



void showArrayUnsignedLists(const vector<Array<unsigned>*> &lists)
{
	
  cout << "Now begin show all lists (size is "<< lists.size()<< "): " << endl;
	
  for(unsigned k=0;(unsigned)k<lists.size();k++){ 
		     
    Array<unsigned> *onelist = lists.at(k);
	 
    cout << "List "<< k << " is: " << endl;
	 
    showArrayUnsigned(onelist);
        
  }//end for
	
}//end showVectorUnsignedList




void showUnsignedSet( set<unsigned,less<unsigned> > *s)
{
  cout << "The size of set is " << s->size()<< endl;

  set<unsigned,less<unsigned> >::iterator ite =
    s->begin();

  while ( ite != s->end())
    {
      cout << *ite << " " ;
      ite++;
    }//end while

  cout << endl;

}//end  showUnsignedSet


