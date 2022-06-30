/*
  $Id: wrapperdiscardlists.h 5146 2010-03-24 23:05:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the Academic BSD License.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrapperdiscardlists_h_
#define _wrapperdiscardlists_h_

#include "wrapperabs.h"
#include "../ftindexerdiscardlists.h"

template <class SimilarityMetric = SimMetricEd>
class WrapperDiscardListsLLF 
  : public WrapperAbs<FtIndexerDiscardListsLLF<>, FtSearcherMem<FtIndexerDiscardListsLLF<> >, SimilarityMetric> {  
  
public:  
  WrapperDiscardListsLLF(StringContainerVector* strContainer,
			unsigned gramLength = 3, 
			bool usePartFilter = true,
			float compressionRatio = 0.5, 
			bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerDiscardListsLLF<>(strContainer, this->gramGen, compressionRatio);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerDiscardListsLLF<> >(&this->merger, this->indexer);   
    this->simMetric = new SimilarityMetric(*(this->gramGen));
  }
};

template <class SimilarityMetric = SimMetricEd>
class WrapperDiscardListsSLF 
  : public WrapperAbs<FtIndexerDiscardListsSLF<>, FtSearcherMem<FtIndexerDiscardListsSLF<> >, SimilarityMetric> {
  
 public:  
  WrapperDiscardListsSLF(StringContainerVector* strContainer,
			unsigned gramLength = 3,
			bool usePartFilter = true,
			float compressionRatio = 0.5, 
			bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerDiscardListsSLF<>(strContainer, this->gramGen, compressionRatio);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerDiscardListsSLF<> >(&this->merger, this->indexer);   
    this->simMetric = new SimMetricEd(*(this->gramGen));
  }
};


template <class SimilarityMetric = SimMetricEd>
class WrapperDiscardListsRandom 
  : public WrapperAbs<FtIndexerDiscardListsRandom<>, FtSearcherMem<FtIndexerDiscardListsRandom<> >, SimilarityMetric> {
  
public:  
  WrapperDiscardListsRandom(StringContainerVector* strContainer,
			   unsigned gramLength = 3, 
			   bool usePartFilter = true,
			   float compressionRatio = 0.5, 
			   bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerDiscardListsRandom<>(strContainer, this->gramGen, compressionRatio);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerDiscardListsRandom<> >(&this->merger, this->indexer);   
    this->simMetric = new SimMetricEd(*(this->gramGen));
  }
};

template <class SimilarityMetric = SimMetricEd>
class WrapperDiscardListsPanicCost 
  : public WrapperAbs<FtIndexerDiscardListsPanicCost<>, FtSearcherMem<FtIndexerDiscardListsPanicCost<> >, SimilarityMetric>{
  
public:  
  WrapperDiscardListsPanicCost(StringContainerVector* strContainer,
			      vector<string>* workload, 
			      float optimizeThreshold, 
			      unsigned gramLength = 3, 
			      bool usePartFilter = true,
			      float reductionRatio = 0.5, 
			      bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->simMetric = new SimMetricEd(*(this->gramGen));
    this->indexer = new FtIndexerDiscardListsPanicCost<>(strContainer, 
							this->gramGen, 
							reductionRatio, 
							workload, 
							this->simMetric, 
							optimizeThreshold);
    if(usePartFilter) this->indexer->autoAddPartFilter();   
    this->searcher = new FtSearcherMem<FtIndexerDiscardListsPanicCost<> >(&this->merger, this->indexer);   
  }
};

template <class SimilarityMetric = SimMetricEd>
class WrapperDiscardListsTimeCost 
  : public WrapperAbs<FtIndexerDiscardListsTimeCost<>, FtSearcherMem<FtIndexerDiscardListsTimeCost<> >, SimilarityMetric> {
  
public:  
  WrapperDiscardListsTimeCost(StringContainerVector* strContainer,
			     vector<string>* workload, 
			     float optimizeThreshold, 
			     unsigned gramLength = 3, 
			     bool usePartFilter = true,
			     float reductionRatio = 0.5, 
			     bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->simMetric = new SimMetricEd(*(this->gramGen));
    this->indexer = new FtIndexerDiscardListsTimeCost<>(strContainer, 
						       this->gramGen, 
						       reductionRatio, 
						       workload, 
						       this->simMetric, 
						       optimizeThreshold);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerDiscardListsTimeCost<> >(&this->merger, this->indexer);   
  }
};

#endif
