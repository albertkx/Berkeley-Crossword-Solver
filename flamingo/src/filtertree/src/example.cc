/*
  $Id: example.cc 6078 2011-04-29 23:26:06Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 02/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include <vector>
#include <algorithm>

#include "common/src/query.h"
#include "common/src/simmetric.h"

#include "ftindexermem.h"
#include "ftsearchermem.h"
#include "ftindexerdiscardlists.h"
#include "ftindexercombinelists.h"
#include "listmerger/src/mergeoptmerger.h"
#include "listmerger/src/scancountmerger.h"

#include "ftindexerondisk.h"
#include "ftsearcherondisk.h"
#include "listmerger/src/ondiskmergersimple.h"
#include "listmerger/src/ondiskmergeradapt.h"

// create a dummy dictionary
extern void readString(vector<string>& data, const string& filenameData, unsigned count, unsigned maxLineLen);
std::vector<string> dictionary;
void initDictionary();

void memBasicUsage1();
void memBasicUsage2();
void memBasicUsage3();
void memBasicUsage4();
void memDiscardLists();
void memCombineLists();
void memAdvancedUsage();

void memTopkQueries();
void memIndexMaintenance();

void ondiskBasicUsage1();
void ondiskBasicUsage2();

int main() {
  
  initDictionary();

  memBasicUsage1();
  memBasicUsage2();
  memBasicUsage3();
  memBasicUsage4();
  memDiscardLists();
  memCombineLists();
  memAdvancedUsage();
  
  memTopkQueries();
  memIndexMaintenance();
  
  ondiskBasicUsage1();
  ondiskBasicUsage2();
  
  return 0;
}

void initDictionary() {
  // params: target vector, filename, number strings to read, max line length
  readString(dictionary, "../data/female_names.txt", 4000, 20);
}


// builds in-memory index with length filter for partitioning
// fills index from existing collection
void memBasicUsage1() {  
  cout << "----- MEM BASIC USAGE 1 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2); // fixed-length grams
  SimMetricEd simMetric(gramGen); // edit distance
  //SimMetricEdNorm simMetric(gramGen); // normalized edit distance
  //SimMetricJacc simMetric(gramGen); // jaccard similarity
  //SimMetricCos simMetric(gramGen); // cosine similarity
  //SimMetricDice simMetric(gramGen); // dice similarity
  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true); // true indicates statistics gathering, e.g. for auto part filtering
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end()); // fill the container from a collection
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new LengthFilter(20)); // add length filtering with a maximum string length of 20
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer with default template arguments
  // default: same as indexer, i.e. assumed simple indexer with vector<unsigned> as inverted lists and MergeOptMerger as merger type
  FtSearcherMem<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f); // query string, similarity metric, similarity threshold
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerMem<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}

// builds in-memory index with charsum filter for partitioning
// fills index from data file
void memBasicUsage2() {  
  cout << "----- MEM BASIC USAGE 2 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2); // fixed-length grams
  SimMetricEdNorm simMetric(gramGen); // normalized edit distance
  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true);
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer("../data/female_names.txt", 4000); // fill the container from a datafile and use the first 4000 lines
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new CharsumFilter(20)); // add charsum filtering with a maximum string length of 20
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer with default template arguments
  // default: same as indexer, i.e. assumed simple indexer with vector<unsigned> as inverted lists and MergeOptMerger as merger type
  FtSearcherMem<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 0.8f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerMem<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}

// builds in-memory index with length filter for partitioning
// fills index from data file
// uses non-default list-merging algorithm, ScanCount
void memBasicUsage3() {  
  cout << "----- MEM BASIC USAGE 3 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2, false); // fixed-length grams without pre-and postfixing
  SimMetricJaccBag simMetric(gramGen); // jaccard distance (with bag semantics)
  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true);
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer("../data/female_names.txt", 4000); // fill the container from a datafile and use the first 4000 lines
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new LengthFilter(20)); // add length filtering with a maximum string length of 50
  indexer.buildIndex();
  
  // create merger
  ScanCountMerger<> merger(4000);
  // create searcher, specifying a non-default merger
  FtSearcherMem<FtIndexerMem<>, ScanCountMerger<> > searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 0.7f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerMem<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}

void memBasicUsage4() {  
  cout << "----- MEM BASIC USAGE 4 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2); // fixed-length grams
  SimMetricEdSwap simMetric(gramGen); // Damerau-Levenshtein edit distance

  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true); // true indicates statistics gathering, e.g. for auto part filtering
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end()); // fill the container from a collection
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new LengthFilter(20)); // add length filtering with a maximum string length of 20
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer with default template arguments
  // default: same as indexer, i.e. assumed simple indexer with vector<unsigned> as inverted lists and MergeOptMerger as merger type
  FtSearcherMem<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("patricia", simMetric, 1.0f); // query string, similarity metric, similarity threshold
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerMem<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}

// builds in-memory index with length filter for partitioning
// compresses index by discarding some inverted lists (based on impact tto given workload)
// fills index from existing collection
// other compressed indexers are:
// FtIndexerDiscardListsLLF - discard long lists first
// FtIndexerDiscardListsSLF - discard short lists first
// FtIndexerDiscardListsRandom - randomly discard lists first
// FtIndexerDiscardListsPanicCost - minimize number of panics
// FtIndexerDiscardListsTimeCost - minimize total running time
// FtIndexerCombineListsBasic - combine lists based on correlation
// FtIndexerCombineListsCost - combine lists based on total running time
void memDiscardLists() {
  cout << "----- MEM DISCARDLISTS ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2);
  SimMetricEd simMetric(gramGen);
  
  // create index compressed with holes, using TimeCost as hole selection algorithm
  // default template parameters: in-memory index using vector<unsigned> as an inverted list
  StringContainerVector strContainer(true);
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end());
  // params: string container, gram generator, compression ratio, training workload, training metric, training threshold, ratio cost, 
  //         data sampling fraction, queries sampling fraction, max sting length, fanout
  FtIndexerDiscardListsTimeCost<> 
    indexer(&strContainer, &gramGen, 0.5f, &dictionary, &simMetric, 1.0f, false, 0.01f, 0.25f, 20, 10);
  indexer.addPartFilter(new LengthFilter(20));
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer
  // must specify indexer type as template argument
  FtSearcherMem<FtIndexerDiscardListsTimeCost<> > searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerDiscardListsTimeCost<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "----------------------------" << endl << endl;;
}

// builds in-memory index with length filter for partitioning
// compresses index by combining some inverted lists (based on impact tto given workload)
// fills index from existing collection
// other compressed indexers are:
// FtIndexerDiscardListsLLF - discard long lists first
// FtIndexerDiscardListsSLF - discard short lists first
// FtIndexerDiscardListsRandom - randomly discard lists first
// FtIndexerDiscardListsPanicCost - minimize number of panics
// FtIndexerDiscardListsTimeCost - minimize total running time
// FtIndexerCombineListsBasic - combine lists based on correlation
// FtIndexerCombineListsCost - combine lists based on total running time
void memCombineLists() {
  cout << "----- MEM COMPRESSION UNION ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2);
  SimMetricEd simMetric(gramGen);
  
  // create index compressed with holes, using TimeCost as hole selection algorithm
  // default template parameters: in-memory index using vector<unsigned> as an inverted list
  StringContainerVector strContainer(true);
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end());
  // params: string container, gram generator, compression ratio, training workload, training metric, training threshold,  
  //         data sampling fraction, queries sampling fraction, max sting length, fanout
  FtIndexerCombineListsCost<> 
    indexer(&strContainer, &gramGen, 0.5f, &dictionary, &simMetric, 1.0f, 0.01f, 1.0f, 20, 10);
  indexer.addPartFilter(new LengthFilter(20));
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer
  // must specify indexer type as template argument
  FtSearcherMem< FtIndexerCombineListsCost<> > searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerCombineListsCost<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "----------------------------" << endl << endl;;
}

// builds in-memory index with length filter for partitioning
// compresses index by discarding some inverted lists (based on impact tto given workload)
// fills index from existing collection
// uses non-default list-merging algorithm, ScanCount
// explicitly specifies template parameters, and constructor arguments
void memAdvancedUsage() {
  cout << "----- MEM ADVANCED USAGE ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2);
  SimMetricEd simMetric(gramGen);
  
  // create index compressed with holes, using TimeCost as hole selection algorithm 
  // and specifiying the non-default string container and vector<unsigned> as inverted list
  StringContainerRM strContainer(true);
  strContainer.createContainer("tmpcollection.txt");
  strContainer.openContainer("tmpcollection.txt");  
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end());
  // params: string container, gram generator, compression ratio, training workload, training metric, training threshold, ratio cost, 
  //         data sampling fraction, queries sampling fraction, max sting length, fanout
  FtIndexerDiscardListsTimeCost<StringContainerRM, vector<unsigned> > 
    indexer(&strContainer, &gramGen, 0.5f, &dictionary, &simMetric, 2.0f, false, 0.01f, 0.25f, 20, 10);
  indexer.addPartFilter(new LengthFilter(20));
  indexer.buildIndex();
  
  // create merger specifying inverted list type
  ScanCountMerger<vector<unsigned> > merger(dictionary.size());
  // create searcher passing merger and indexer
  // specify all template arguments, need to specify merger if not default (MergeOptMerger)
  FtSearcherMem<FtIndexerDiscardListsTimeCost<StringContainerRM, vector<unsigned> >, ScanCountMerger<vector<unsigned> > > searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerDiscardListsTimeCost<StringContainerRM, vector<unsigned> > indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "-------------------------" << endl << endl;
}

// builds in-memory index with length filter for partitioning
// fills index from data file
// runs a Top-K query based on Edit Distance
// ATTENTION: Top-K queries are only supported with edit distance
void memTopkQueries() {  
  cout << "----- MEM TOPK QUERIES ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2); // fixed-length grams
  SimMetricEd simMetric(gramGen); // edit distance
  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true); // true indicates statistics gathering, e.g. for auto part filtering
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end()); // fill the container from a collection
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new LengthFilter(20)); // add length filtering with a maximum string length of 20
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer with default template arguments
  // default: same as indexer, i.e. assumed simple indexer with vector<unsigned> as inverted lists and MergeOptMerger as merger type
  FtSearcherMem<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 5, QueryTopk); // asking for top-5 most similar strings
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerMem<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}


// builds in-memory index with length filter for partitioning
// fills index from data file
// shows how to perform inserts and deletes on the index
// deletes are buffered and not immediately reflected in inverted lists
// the buffered deletions cost memory and degrade query performance (need to intersect with answers)
// the buffered deletions should periodically be integrated into the inverted (also shown below)
void memIndexMaintenance() {  
  cout << "----- MEM INDEX MAINTENANCE ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2); // fixed-length grams
  SimMetricEd simMetric(gramGen); // edit distance
  
  // create simple indexer with default template arguments
  // default: in-memory index using vector<unsigned> as an inverted list container
  // first create a string container and fill it with strings to index
  StringContainerVector strContainer(true); // true indicates statistics gathering, e.g. for auto part filtering
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end()); // fill the container from a collection
  FtIndexerMem<> indexer(&strContainer, &gramGen, 20, 10); // maxStrLen=20, fanout=10
  indexer.addPartFilter(new LengthFilter(20)); // add length filtering with a maximum string length of 20
  indexer.buildIndex();
  
  // create merger
  MergeOptMerger<> merger;
  // create searcher passing merger and indexer with default template arguments
  // default: same as indexer, i.e. assumed simple indexer with vector<unsigned> as inverted lists and MergeOptMerger as merger type
  FtSearcherMem<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 2.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << resultStringIDs[i] << " " << tmp << endl;
  }
 
  // insert a couple of strings
  indexer.insertString("kathryn");
  indexer.insertString("kathein");
  indexer.insertString("cathrin");
  indexer.insertString("cathryn");
  
  resultStringIDs.clear();
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS AFTER INSERTIONS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << resultStringIDs[i] << " " << tmp << endl;
  }
  
  // delete first 1000 strings from index and stringcontainer
  for(unsigned i = 0; i < 1000; i++)
    indexer.deleteString(i);

  resultStringIDs.clear();
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS AFTER DELETIONS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << resultStringIDs[i] << " " << tmp << endl;
  }
  
  // integrate the deletions into the inverted lists
  // WARNING: this operation takes time and should be done periodically but infrequently
  indexer.integrateUpdates();
  
  resultStringIDs.clear();
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS AFTER INTEGRATION: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << resultStringIDs[i] << " " << tmp << endl;
  }

  cout << "----------------------" << endl << endl;
}

// builds disk-based index on disk-based string collection
// uses length filter for partitioning, and default list-merging algorithm
// fills index from existing collection
void ondiskBasicUsage1() {
  cout << "----- ONDISK BASIC USAGE 1 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2);
  SimMetricEd simMetric(gramGen);
  
  // using disk-based string container
  StringContainerRM strContainer(PHO_LENGTH, true);
  strContainer.createAndOpen("collection.rm");
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer(dictionary.begin(), dictionary.end()); // fill the container from dictionary
  // params: container, gramgen, disableStreamBuffer, index filename, bytes per run, max str len, fanout
  FtIndexerOnDisk<> indexer(&strContainer, &gramGen, false, "invlists.ix", 50000, 20, 10);
  indexer.addPartFilter(new LengthFilter(20)); // param: max str len
  indexer.buildIndex();
  
  // create merger specifying inverted list type
  OnDiskMergerSimple<> merger;
  // create searcher passing merger and indexer
  FtSearcherOnDisk<> searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerOnDisk<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "-------------------------" << endl << endl;
}


// builds disk-based index on disk-based string collection
// uses length filter for partitioning
// uses adaptive list-merging algorithm
// fills index from existing collection
void ondiskBasicUsage2() {
  cout << "----- ONDISK BASIC USAGE 2 ----" << endl;

  // create gramgenerator and similarity metric
  GramGenFixedLen gramGen(2);
  SimMetricEd simMetric(gramGen);
  
  // using disk-based string container
  StringContainerRM strContainer(PHO_LENGTH, true);
  strContainer.createAndOpen("collection.rm");
  strContainer.initStatsCollector(&gramGen);
  strContainer.fillContainer("../data/female_names.txt", 4000); // fill the container from a datafile and use the first 4000 lines
  // params: container, gramgen, disableStreamBuffer, index filename, bytes per run, max str len, fanout
  FtIndexerOnDisk<> indexer(&strContainer, &gramGen, false, "invlists.ix", 50000, 20, 10);
  indexer.autoAddPartFilter(); // automatically choose a partitioning filter based on container stats
  indexer.buildIndex();
  
  // create merger specifying inverted list type
  OnDiskMergerAdapt<> merger;
  // create searcher passing merger and indexer
  FtSearcherOnDisk<FtIndexerOnDisk<>, OnDiskMergerAdapt<> > searcher(&merger, &indexer);
  
  vector<unsigned> resultStringIDs;
  Query query("kathrin", simMetric, 1.0f);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "SAVING INDEX" << endl;
  indexer.saveIndex("exampleindex.ix");

  cout << "LOADING INDEX" << endl;
  FtIndexerOnDisk<> indexerLoaded(&strContainer);
  indexerLoaded.loadIndex("exampleindex.ix");
  
  resultStringIDs.clear();
  searcher.setFtIndexer(&indexerLoaded);
  searcher.search(query, resultStringIDs);  
  cout << "SIMILAR STRINGS: " << endl;
  for(unsigned i = 0; i < resultStringIDs.size(); i++) {
    string tmp;
    strContainer.retrieveString(tmp, resultStringIDs[i]);
    cout << tmp << endl;
  }
  
  cout << "-------------------------" << endl << endl;
}
