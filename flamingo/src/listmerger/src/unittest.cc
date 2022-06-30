/*  
 $Id: unittest.cc 5748 2010-09-30 08:08:37Z abehm $   

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Date: 05/14/2007
 Author: Jiaheng Lu, Alexander Behm
*/

#include <vector>
#include <string.h>
#include <set>

#include "listmerger.h"
#include "divideskipmerger.h"
#include "heapmerger.h"
#include "mergeoptmerger.h"
#include "mergeskipmerger.h"
#include "scancountmerger.h"
#include "util/src/array.h"
#include "util/src/debug.h"

void getResults(vector<unsigned>* results, unsigned threshold);

template<class Merger>
bool runTest(Merger& merger);

void initInvLists();
void destroyInvLists();
void initExpectedResults();
void destroyExpectedResults();

unsigned numInvLists = 20;
unsigned maxListLength = 1000000;
unsigned maxStringId = 1000000;
unsigned maxThreshold = numInvLists + 5;

vector<vector<unsigned>* > invLists;
vector<vector<unsigned>* > expectedResults;

int main() {  
  initInvLists();
  initExpectedResults();

  cout << endl;
  
  bool passed = false;

  DivideSkipMerger<> dsMerger;
  passed = runTest<DivideSkipMerger<> >(dsMerger);  
  if(passed) cout << "PASSED" << endl;
  else cout << "FAILED" << endl;
  cout << endl;

  HeapMerger<> heapMerger;
  passed = runTest<HeapMerger<> >(heapMerger);  
  if(passed) cout << "PASSED" << endl;
  else cout << "FAILED" << endl;
  cout << endl;

  MergeOptMerger<> moMerger;
  passed = runTest<MergeOptMerger<> >(moMerger);  
  if(passed) cout << "PASSED" << endl;
  else cout << "FAILED" << endl;
  cout << endl;

  MergeSkipMerger<> msMerger;
  passed = runTest<MergeSkipMerger<> >(msMerger);  
  if(passed) cout << "PASSED" << endl;
  else cout << "FAILED" << endl;
  cout << endl;

  ScanCountMerger<> scMerger(maxStringId);
  passed = runTest<ScanCountMerger<> >(scMerger);
  if(passed) cout << "PASSED" << endl;
  else cout << "FAILED" << endl;
  cout << endl;  

  destroyExpectedResults();
  destroyInvLists();
}

template<class Merger>
bool runTest(Merger& merger) {
  
  string msg = "RUNNING TEST FOR " + merger.getName();
  TIMER_START(msg, maxThreshold);
  for(unsigned i = 0; i < maxThreshold; i++) {
    vector<unsigned> results;
    merger.merge(invLists, i+1, results);
    
    if(results.size() != expectedResults[i]->size()) return false;
    
    sort(results.begin(), results.end()); // in case merger does not deliver results sorted by id
    
    // compare results with expected results
    for(unsigned j = 0; j < expectedResults[i]->size(); j++) {
      if(results[j] != expectedResults[i]->at(j)) return false;
    }
    TIMER_STEP();
  }
  TIMER_STOP();
  
  return true;
}

void initExpectedResults() {
  TIMER_START("COMPUTING EXPECTED RESULTS", maxThreshold);
  for(unsigned i = 0; i < maxThreshold; i++) {
    vector<unsigned>* v = new vector<unsigned>();
    getResults(v, i+1);
    expectedResults.push_back(v);
    TIMER_STEP();
  }
  TIMER_STOP();
}


void getResults(vector<unsigned>* results, unsigned threshold) {
  // quick and dirty implementation of scancount 
  unsigned counts[maxStringId];
  memset(counts, 0, sizeof(unsigned) * maxStringId);
  
  for(unsigned i = 0; i < invLists.size(); i++) {
    for(unsigned j = 0; j < invLists[i]->size(); j++) {
      counts[invLists[i]->at(j)]++;
    }
  }
		  
  for(unsigned i = 0; i < maxStringId; i++) {
    if(counts[i] >= threshold) results->push_back(i);
  }
}

void initInvLists() {
  srand(50);
  
  TIMER_START("GENERATING INVERTED LISTS", numInvLists);
  for(unsigned i = 0; i < numInvLists; i++) {
    vector<unsigned>* v = new vector<unsigned>();
    invLists.push_back(v);
    
    unsigned listLength = rand() % maxListLength;
    if(listLength == 0) listLength = 1;

    set<unsigned> elements;
    for(unsigned j = 0; j < listLength; j++) {
      elements.insert(rand() % maxStringId);
    }
    
    for(set<unsigned>::iterator iter = elements.begin(); iter != elements.end(); iter++) {
      v->push_back(*iter);
    }
    TIMER_STEP();
  }
  TIMER_STOP();
}

void destroyInvLists() {
  for(unsigned i = 0; i < invLists.size(); i++)
    delete invLists[i];
}

void destroyExpectedResults() {
  for(unsigned i = 0; i < expectedResults.size(); i++)
    delete expectedResults[i];
}
