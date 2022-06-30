/*  
 $Id: example.cc 5748 2010-09-30 08:08:37Z abehm $   

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
#include "scancountmerger.h"

using namespace std;

int main() {
  vector<unsigned> list1;
  list1.push_back(1);
  list1.push_back(5);
  list1.push_back(8);

  vector<unsigned> list2;
  list2.push_back(3);
  list2.push_back(4);
  list2.push_back(5);

  vector<unsigned> list3;
  list3.push_back(8);
  list3.push_back(10);
  list3.push_back(13);
  list3.push_back(15);

  vector<unsigned> list4;
  list4.push_back(8);
  list4.push_back(10);
  list4.push_back(13);
  list4.push_back(15);
  
  vector<vector<unsigned>*> lists;
  lists.push_back(&list1);
  lists.push_back(&list3);
  lists.push_back(&list2); 
  lists.push_back(&list4);

  const unsigned threshold = 2;

  cout<< "Occurrence threshold is " << threshold << endl;

  vector<unsigned> results;
  
  DivideSkipMerger<> merger;
  //HeapMerger<> merger;
  //MergeOptMerger<> merger;
  //MergeSkipMerger<> merger;
  //ScanCountMerger<> merger(999999);
  
  merger.merge(lists, threshold, results);
  
  cout << "Number of elements that occur at least " 
       << threshold << " times: " << results.size() << endl;
}
