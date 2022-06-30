/*
  $Id: unittest.cc 5756 2010-10-12 04:21:00Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm
*/

#include "ftindexerdiscardlists.h"
#include "ftindexercombinelists.h"
#include "ftsearchermem.h"
#include "common/src/query.h"
#include "common/src/simmetric.h"
#include "listmerger/src/divideskipmerger.h"
#include "listmerger/src/scancountmerger.h"
#include <fstream>
#include <stdio.h>

using namespace std;
using namespace tr1;

// ASSUMPTIONS: uncompressed indexer with no filters delivers correct results
// it will be used to fill expectedResults

// global vars for performing unittests
StringContainerVector strContainer;
vector<Query*> queries;
vector<string> queryStrings;
vector<unsigned> expectedResults;
GramGenFixedLen gramGen(3);
SimMetricEd simMetric(gramGen);

void init();
void deinit();
bool compareResults(vector<unsigned>& results, const string& identifier);

bool testFtIndexerMem();
bool testFtIndexerDiscardListsLLF();
bool testFtIndexerDiscardListsSLF();
bool testFtIndexerDiscardListsRandom();
bool testFtIndexerDiscardListsTimeCost();
bool testFtIndexerDiscardListsTimeCostPlus();
bool testFtIndexerDiscardListsPanicCost();
bool testFtIndexerDiscardListsPanicCostPlus();
bool testFtIndexerCombineListsBasic();
bool testFtIndexerCombineListsCost();

int main() {
  init();

  bool passed = false;

  cout << "NOTE: THESE TESTS MAY TAKE SOME MINUTES, PLEASE BE PATIENT" << endl << endl;
  
  cout << "TEST FtIndexerMem:" << endl;
  passed = testFtIndexerMem();  
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;
  cout << endl;

  cout << "TEST FtIndexerDiscardListsLLF:" << endl;  
  passed = testFtIndexerDiscardListsLLF();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;
  
  cout << "TEST FtIndexerDiscardListsSLF:" << endl;  
  passed = testFtIndexerDiscardListsSLF();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerDiscardListsRandom:" << endl;  
  passed = testFtIndexerDiscardListsRandom();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerDiscardListsTimeCost:" << endl;  
  passed = testFtIndexerDiscardListsTimeCost();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerDiscardListsTimeCostPlus:" << endl;  
  passed = testFtIndexerDiscardListsTimeCostPlus();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerDiscardListsPanicCost:" << endl;  
  passed = testFtIndexerDiscardListsPanicCost();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerDiscardListsPanicCostPlus:" << endl;  
  passed = testFtIndexerDiscardListsPanicCostPlus();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerCombineListsBasic:" << endl;  
  passed = testFtIndexerCombineListsBasic();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  cout << "TEST FtIndexerCombineListsCost:" << endl;  
  passed = testFtIndexerCombineListsCost();
  if(passed) cout << "--- PASSED ---" << endl;
  else cout << "--- FAILED ---" << endl;  
  cout << endl;

  deinit();
  return 0;
}

void init() {
  cout << "INITIALIZING UNITTEST" << endl;

  vector<string> prefixes;
  prefixes.push_back("string");
  prefixes.push_back("example");  
  prefixes.push_back("test");
  prefixes.push_back("hello");
  prefixes.push_back("world");
  prefixes.push_back("foo");
  prefixes.push_back("bar");
  
  vector<string> suffixes;
  suffixes.push_back("1");
  suffixes.push_back("10");
  suffixes.push_back("100");
  suffixes.push_back("2");
  suffixes.push_back("20");
  suffixes.push_back("200");
  suffixes.push_back("3");
  suffixes.push_back("30");
  suffixes.push_back("300");
  
  for(unsigned j = 0; j < prefixes.size(); j++)
    for(unsigned i = 0; i < suffixes.size(); i++)
      strContainer.insertString(prefixes.at(j) + suffixes.at(i));
  
  // create queries
  queries.push_back(new Query("xample", simMetric, 2.0f));
  queries.push_back(new Query("ring1", simMetric, 2.0f));
  queries.push_back(new Query("wrld", simMetric, 2.0f));
  queries.push_back(new Query("fooa", simMetric, 2.0f));
  queries.push_back(new Query("br", simMetric, 2.0f));  

  for(unsigned i = 0; i < 10; i++) {
    queryStrings.push_back("xample");
    queryStrings.push_back("ring1");
    queryStrings.push_back("wrld");
    queryStrings.push_back("fooa");
    queryStrings.push_back("br");
  }

  // execute queries on uncompressed index without filters to get expected results
  FtIndexerMem<> indexer(&strContainer, &gramGen);
  indexer.buildIndex();
  MergeOptMerger<> merger;
  FtSearcherMem<> searcher(&merger, &indexer);

  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
    searcher.search(**iter, expectedResults);
    
  // sort expected results
  sort(expectedResults.begin(), expectedResults.end());
  
  cout << "UNITTEST INITIALIZED" << endl << endl;
}

void deinit() {
  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
    delete *iter;  
}

bool compareResults(vector<unsigned>& results, const string& identifier) {
  // compare results
  sort(results.begin(), results.end());
  if(results.size() != expectedResults.size()) {
    cout << "FAILED IN " << identifier << endl;
    return false;
  }

  for(unsigned i = 0; i < results.size(); i++)
    if(results.at(i) != expectedResults.at(i)) {
      cout << "FAILED IN " << identifier << endl;
      return false;
    }

  return true;
}

bool testFtIndexerMem() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<> searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      vector<unsigned> results;      

      // begin block for indexer with lengthfilter
      {
	FtIndexerMem<> indexer(&strContainer, &gramGen, maxStrLength, fanout);
	indexer.addPartFilter(new LengthFilter(maxStrLength));
	indexer.buildIndex();      

	// execute queries and compute results
	results.clear();
	searcher.setFtIndexer(&indexer);	
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);
	
	success = success && compareResults(results, "FtIndexerMem, LENGTH FILTER BUILT");

	// save index, load it into differrent indexer and repeat
	indexer.saveIndex("UnittestIndex.ix");
	FtIndexerMem<> loadedIndexer(&strContainer);
	loadedIndexer.loadIndex("UnittestIndex.ix");

	if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	  cout << "FtIndexerMem, LENGTH FILTER LOADED INCORRECTLY" << endl;
	  success = false;
	}

	results.clear();
	searcher.setFtIndexer(&loadedIndexer);	
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);
	
	success = success && compareResults(results, "FtIndexerMem, LENGTH FILTER LOADED");
      }

      // begin block for indexer with checksum filter
      {
	FtIndexerMem<> indexer(&strContainer, &gramGen, maxStrLength, fanout);
	indexer.addPartFilter(new CharsumFilter(maxStrLength));
	indexer.buildIndex();      

	// execute queries and compute results
	searcher.setFtIndexer(&indexer);	
	results.clear();
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);
	
	success = success && compareResults(results, "FtIndexerMem, CHARSUM FILTER BUILT");

	// save index, load it into differrent indexer and repeat
	indexer.saveIndex("UnittestIndex.ix");
	FtIndexerMem<> loadedIndexer(&strContainer);
	loadedIndexer.loadIndex("UnittestIndex.ix");

	if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	  cout << "FtIndexerMem, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	  success = false;
	}

	results.clear();
	searcher.setFtIndexer(&loadedIndexer);	
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);
	
	success = success && compareResults(results, "FtIndexerMem, CHARSUM FILTER LOADED");
      }

      // begin block for indexer with both length and checksum filters
      {
	FtIndexerMem<> indexer(&strContainer, &gramGen, maxStrLength, fanout);
	indexer.addPartFilter(new LengthFilter(maxStrLength));
	indexer.addPartFilter(new CharsumFilter(maxStrLength));
	indexer.buildIndex();      

	// execute queries and compute results
	searcher.setFtIndexer(&indexer);      
	results.clear();
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);

	success = success && compareResults(results, "FtIndexerMem, LENGTH+CHARSUM FILTER BUILT");

	// save index, load it into differrent indexer and repeat
	indexer.saveIndex("UnittestIndex.ix");
	FtIndexerMem<> loadedIndexer(&strContainer);
	loadedIndexer.loadIndex("UnittestIndex.ix");

	if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	   && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	  cout << "FtIndexerMem, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	  success = false;
	}

	results.clear();
	searcher.setFtIndexer(&loadedIndexer);	
	for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	  searcher.search(**iter, results);
	
	success = success && compareResults(results, "FtIndexerMem, LENGTH FILTER LOADED");
      }

    }
  }

  return success;
}

bool testFtIndexerDiscardListsLLF() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsLLF<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsLLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsLLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsLLF, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsLLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsLLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsLLF, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsLLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsLLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsLLF, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsLLF, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerDiscardListsSLF() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsSLF<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsSLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsSLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsSLF, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsSLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsSLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsSLF, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsSLF<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsSLF<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsSLF, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsSLF, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerDiscardListsRandom() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsRandom<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsRandom<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsRandom<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsRandom, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsRandom<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsRandom<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsRandom, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsRandom<> indexer(&strContainer, &gramGen, compressionRatio, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsRandom<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsRandom, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsRandom, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerDiscardListsTimeCost() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsTimeCost<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsTimeCost, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsTimeCost, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsTimeCost, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCost, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}


bool testFtIndexerDiscardListsTimeCostPlus() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsTimeCost<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsTimeCostPlus, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsTimeCostPlus, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsTimeCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsTimeCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsTimeCostPlus, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsTimeCostPlus, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerDiscardListsPanicCost() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsPanicCost<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsPanicCost, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsPanicCost, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, true, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsPanicCost, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCost, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerDiscardListsPanicCostPlus() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerDiscardListsPanicCost<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerDiscardListsPanicCostPlus, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsPanicCostPlus, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerDiscardListsPanicCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, false, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerDiscardListsPanicCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerDiscardListsPanicCostPlus, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerDiscardListsPanicCostPlus, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerCombineListsBasic() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger(true);
  
  FtSearcherMem<FtIndexerCombineListsBasic<> > searcher(&merger);
  
  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerCombineListsBasic<> indexer(&strContainer, &gramGen, compressionRatio, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsBasic, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsBasic<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerCombineListsBasic, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);	  

	  success = success && compareResults(results, "FtIndexerCombineListsBasic, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerCombineListsBasic<> indexer(&strContainer, &gramGen, compressionRatio, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsBasic, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsBasic<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerCombineListsBasic, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsBasic, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerCombineListsBasic<> indexer(&strContainer, &gramGen, compressionRatio, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsBasic, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsBasic<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerCombineListsBasic, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsBasic, LENGTH FILTER LOADED");
	}
	
      }
    }
  }

  return success;
}

bool testFtIndexerCombineListsCost() {  
  GramGenFixedLen gramGen(3);
  SimMetricEd simMetric(gramGen);
  MergeOptMerger<> merger;
  
  FtSearcherMem<FtIndexerCombineListsCost<> > searcher(&merger);

  bool success = true;

  // try different filters with different fanouts and values for max string length
  for(unsigned maxStrLength = 10; maxStrLength <= 200; maxStrLength += 10) {
    for(unsigned fanout = 1; fanout <= 10; fanout++) {
      for(float compressionRatio = 0.1f; compressionRatio <= 0.7f; compressionRatio += 0.1f) {
	vector<unsigned> results;
	
	// begin block for indexer with lengthfilter
	{
	  FtIndexerCombineListsCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  cout << "INDEX BUILT" << endl;

	  // execute queries and compute results
	  results.clear();
	  searcher.setFtIndexer(&indexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, LENGTH FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH) {
	    cout << "FtIndexerCombineListsCost, LENGTH FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, LENGTH FILTER LOADED");
	}
	
	// begin block for indexer with checksum filter
	{
	  FtIndexerCombineListsCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);	
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerCombineListsCost, CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, CHARSUM FILTER LOADED");
	}
	
	// begin block for indexer with both length and checksum filters
	{
	  FtIndexerCombineListsCost<> indexer(&strContainer, &gramGen, compressionRatio, &queryStrings, &simMetric, 2.0f, 0.1f, 0.1f, maxStrLength, fanout);
	  indexer.addPartFilter(new LengthFilter(maxStrLength));
	  indexer.addPartFilter(new CharsumFilter(maxStrLength));
	  indexer.buildIndex();      
	  
	  // execute queries and compute results
	  searcher.setFtIndexer(&indexer);      
	  results.clear();
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, LENGTH+CHARSUM FILTER BUILT");
	  
	  // save index, load it into differrent indexer and repeat
	  indexer.saveIndex("UnittestIndex.ix");
	  FtIndexerCombineListsCost<> loadedIndexer(&strContainer);
	  loadedIndexer.loadIndex("UnittestIndex.ix");
	  
	  if(loadedIndexer.filterTypes.at(0)->getType() != FT_LENGTH
	     && loadedIndexer.filterTypes.at(1)->getType() != FT_CHARSUM) {
	    cout << "FtIndexerCombineListsCost, LENGTH+CHARSUM FILTER LOADED INCORRECTLY" << endl;
	    success = false;
	  }
	  
	  results.clear();
	  searcher.setFtIndexer(&loadedIndexer);	
	  for(vector<Query*>::iterator iter = queries.begin(); iter != queries.end(); iter++)
	    searcher.search(**iter, results);
	  
	  success = success && compareResults(results, "FtIndexerCombineListsCost, LENGTH FILTER LOADED");
	}	
      }
    }
  }

  return success;
}
