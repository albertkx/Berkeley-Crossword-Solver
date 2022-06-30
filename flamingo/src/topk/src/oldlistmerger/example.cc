/*  
 $Id: example.cc 5716 2010-09-09 04:27:56Z abehm $   

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Date: 02/05/2008
 Author: Jiaheng Lu
 */

 
#include <fstream>
#include <iostream>
#include <vector>

#include "listmerger.h"
#include "divideskipmerger.h"
#include "heapmerger.h"
#include "mergeoptmerger.h"
#include "mergeskipmerger.h"
#include "mergeskipplusmerger.h"
#include "scancountmerger.h"
#include "mergeskiptest.h"
#include "showutil.h"
#include "util/src/array.h"

using namespace std;

void example1()
{
  cout << "Example 1: merge lists without duplicate" << endl;
 
  Array<unsigned> list1;
  list1.append(1);
  list1.append(5);
  list1.append(8);

  Array<unsigned> list2;
  list2.append(3);
  list2.append(4);
  list2.append(5);

  Array<unsigned> list3;
  list3.append(8);
  list3.append(10);
  list3.append(13);
  list3.append(15);

  Array<unsigned> list4;
  list4.append(8);
  list4.append(10);
  list4.append(13);
  list4.append(15);
  
  vector<Array<unsigned>*> lists;
  lists.push_back(&list1);
  lists.push_back(&list3);
  lists.push_back(&list2); 
  lists.push_back(&list4);

  const unsigned threshold = 2;

  cout<< "Merging threshold is " << threshold << endl;

  vector<unsigned> result;

  
  //ListsMerger *mergeLists = new HeapMerger();
  //ListsMerger *mergeLists = new MergeOptMerger();
  //set max reord ID as the maxmal unsigned integer
  //ListsMerger *mergeLists = new ScanCountMerger(9999999);
  //ListsMerger *mergeLists = new MergeSkipMerger();  
  DivideSkipMerger<> *mergeLists = new DivideSkipMerger<>();  
  //ListsMerger *mergeLists = new MergeSkipPlusMerger();  

  mergeLists->merge(lists, threshold, result);

  cout<<"Results size is "<<result.size()<<endl;

}//end example1


void example2()
{
  cout << "Example 2: merge lists with duplicate" << endl;
 
  Array<unsigned> list1;
  list1.append(1);
  list1.append(5);
  list1.append(8);

  Array<unsigned> list2;
  list2.append(3);
  list2.append(4);
  list2.append(5);

  Array<unsigned> list3;
  list3.append(8);
  list3.append(10);
  list3.append(13);
  list3.append(15);

  Array<unsigned> list4;
  list4.append(8);
  list4.append(10);
  list4.append(13);
  list4.append(15);


  vector<Array<unsigned>*> lists;
  lists.push_back(&list1);
  lists.push_back(&list1);
  lists.push_back(&list2); 
  lists.push_back(&list3);
  lists.push_back(&list3);
  lists.push_back(&list3);
  lists.push_back(&list4);
  lists.push_back(&list4);


  const unsigned threshold = 5;

  cout<< "Merging threshold is " << threshold << endl;

  vector<unsigned> result;

  
  //set max reord ID as the maxmal unsigned integer
  //ListsMerger *mergeLists = new ScanCountMerger(9999999);
  DivideSkipMerger<> *mergeLists = new DivideSkipMerger<>(true);  
 
  mergeLists->merge(lists, threshold, result);

  cout<<"Results size is "<<result.size()<<endl;

}//end example2

int main() 
{
  
  //Example 1: merge lists without any duplicate
  
  example1();
  
  //Example 2: merge lists with duplicate
  
  example2();


}//end main
