/*
  $Id: wrapperabs.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD License.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrapperabs_h_
#define _wrapperabs_h_

#include "filtertree/src/ftindexermem.h"
#include "filtertree/src/ftsearchermem.h"
#include "filtertree/src/ftindexerondisk.h"
#include "filtertree/src/ftsearcherondisk.h"
#include "common/src/simmetric.h"
#include "common/src/gramgen.h"
#include "common/src/query.h"

// this wrapper should NOT be instantiated because it has no defined constructor
// sub-classes should define constructors
template<class Indexer, class Searcher, class SimilarityMetric>
class WrapperAbs {
protected:
  GramGen* gramGen;
  MergeOptMerger<> merger;
  static bool simAlloc;
  static SimilarityMetric* simMetric;
  Indexer* indexer;
  Searcher* searcher;
  
public:
  void buildIndex(const string& dataFile, const unsigned linesToRead = 0) {
    indexer->buildIndex(dataFile, linesToRead, true);
  }
  
  void buildIndex() {
    indexer->buildIndex();
  }
  
  void insertString(const string& str) {
    indexer->insertString(str);
  }

  void deleteString(unsigned stringId) {
    indexer->deleteString(stringId);
  }

  void integrateUpdates() {
    indexer->integrateUpdates();
  }
  
  void saveIndex(const char* indexFileName) {
    indexer->saveIndex(indexFileName);
  }
  
  void loadIndex(const char* indexFileName) {
    Indexer* oldIndexer = indexer;
    indexer = new Indexer(oldIndexer->strContainer);
    indexer->loadIndex(indexFileName);
    searcher->setFtIndexer(indexer);
    delete oldIndexer;
  }
  
  void search(const string& query, const float simThreshold, vector<unsigned>& results) {
    Query q(query, *simMetric, (float)simThreshold);
    searcher->search(q, results);
  }
  
  virtual ~WrapperAbs() {
    //if(gramGen) delete gramGen;
    if(simAlloc && simMetric) {
      delete simMetric;      
      simAlloc = false;
    }
    if(indexer) delete indexer;    
    if(searcher) delete searcher;
  }
};

template<class Indexer, class Searcher, class SimilarityMetric>
SimilarityMetric* WrapperAbs<Indexer, Searcher, SimilarityMetric>::simMetric;

template<class Indexer, class Searcher, class SimilarityMetric>
bool WrapperAbs<Indexer, Searcher, SimilarityMetric>::simAlloc;


// this wrapper should NOT be instantiated because it has no defined constructor
// sub-classes should define constructors
// this class is designed for use of unsigned short inverted list elements
template<class Indexer, class Searcher, class SimilarityMetric>
class WrapperShortAbs {
protected:
  GramGen* gramGen;
  MergeOptMerger<vector<unsigned short> > merger;
  static bool simAlloc;
  static SimilarityMetric* simMetric;
  Indexer* indexer;
  Searcher* searcher;
  
public:
  void buildIndex(const string& dataFile, const unsigned linesToRead = 0) {
    indexer->buildIndex(dataFile, linesToRead, true);
  }
  
  void buildIndex() {
    indexer->buildIndex();
  }
  
  void insertString(const string& str) {
    indexer->insertString(str);
  }

  void deleteString(unsigned stringId) {
    indexer->deleteString(stringId);
  }

  void integrateUpdates() {
    indexer->integrateUpdates();
  }

  void saveIndex(const char* indexFileName) {
    indexer->saveIndex(indexFileName);
  }
  
  void loadIndex(const char* indexFileName) {
    Indexer* oldIndexer = indexer;
    indexer = new Indexer(oldIndexer->strContainer);
    indexer->loadIndex(indexFileName);
    searcher->setFtIndexer(indexer);
    delete oldIndexer;
  }
  
  void search(const string& query, const float simThreshold, vector<unsigned>& results) {
    Query q(query, *simMetric, (float)simThreshold);
    searcher->search(q, results);
  }
  
  virtual ~WrapperShortAbs() {
    //if(gramGen) delete gramGen;
    if(simAlloc && simMetric) {
      delete simMetric;      
      simAlloc = false;
    }
    if(indexer) delete indexer;    
    if(searcher) delete searcher;
  }
};

template<class Indexer, class Searcher, class SimilarityMetric>
SimilarityMetric* WrapperShortAbs<Indexer, Searcher, SimilarityMetric>::simMetric;

template<class Indexer, class Searcher, class SimilarityMetric>
bool WrapperShortAbs<Indexer, Searcher, SimilarityMetric>::simAlloc;

#endif 
