/*  
 $Id: unittest.cc 5716 2010-09-09 04:27:56Z abehm $   

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Date: 05/14/2007
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

void randomGenerateList(Array<unsigned> *oneList) {
  
  unsigned setSize = random() % 100;
  if(setSize == 0) setSize = 1;
  
  set<unsigned> s;
  
  for(unsigned i=0;i<setSize;i++) {
    unsigned data = random() % 150;    
    s.insert(data);
  }
  
  for(set<unsigned>::iterator ite = s.begin();ite!=s.end();ite++)
    oneList->append(*ite);    
}

void randomListsGeneration(vector<Array<unsigned>*> &arrays, unsigned seed) {  
  srand(seed);
  
  unsigned listsNumber = random() % 120;
  
  if(listsNumber == 0) listsNumber = 1;

  for(unsigned i=0;i<listsNumber;i++) {
    Array<unsigned> *oneList = new Array<unsigned>; 
    randomGenerateList(oneList);
    
    unsigned duplicateNumber = random() % 10;
    
    if(duplicateNumber == 0)
      duplicateNumber = 1;
    
    for(unsigned j=0;j<duplicateNumber;j++)
      arrays.push_back(oneList);      
  }  
}

void freeIndex(vector<Array<unsigned>*> &arrays) {
  set<Array<unsigned>*> toFree;
  for(unsigned i = 0; i < arrays.size(); i++)
    toFree.insert(arrays.at(i));

  for(set<Array<unsigned>*>::iterator iter = toFree.begin(); iter != toFree.end(); iter++)
    delete *iter;
}

void testRandomMergeAlgorithmsWithDuplicate(unsigned seed)
{
  //srand((unsigned)time(0));
  srand(seed);

  vector<Array<unsigned>*> arrays1;
  randomListsGeneration(arrays1, seed);
  vector<Array<unsigned>*> arrays2;
  randomListsGeneration(arrays2, seed);
  vector<Array<unsigned>*> arrays3;
  randomListsGeneration(arrays3, seed);

  unsigned threshold = random() % 100;
  if (threshold == 0) threshold = 1;

  cout<< "Merging threshold is " << threshold << endl;

  vector<unsigned> result1;
  vector<unsigned> result2;
  vector<unsigned> result3;

  ScanCountMerger<> *mergeLists1 = new ScanCountMerger<>(9999999, false);
  DivideSkipMerger<> *mergeLists2 = new DivideSkipMerger<>(true);
  DivideSkipMerger<> *mergeLists3 = new DivideSkipMerger<>(false);

  mergeLists1->merge(arrays1, threshold, result1);
  mergeLists2->merge(arrays2, threshold, result2);
  mergeLists3->merge(arrays3, threshold, result3);  
  
  cout<<result1.size() << " = " << result2.size()
      << " = " << result3.size() << endl;

  if ((result1.size() != result2.size() ) || (result1.size() != result3.size()))
    cout<<"Wrong!!!"<< " " << seed << endl;

  freeIndex(arrays1);
  freeIndex(arrays2);
  freeIndex(arrays3);

  delete mergeLists1;
  delete mergeLists2;
  delete mergeLists3;
  
}

int main() {  
  cout<<"Random generating lists and perform testing." <<endl;

  for(unsigned i=0;i<50;i++)
    testRandomMergeAlgorithmsWithDuplicate(i);

  cout<<"OK pass the test."<<endl;
}
