/*
  $Id: wrappercombinelists.h 5146 2010-03-24 23:05:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the Academic BSD License.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrappercombinelists_h_
#define _wrappercombinelists_h_

#include "wrapperabs.h"
#include "../ftindexercombinelists.h"

template <class SimilarityMetric = SimMetricEd>
class WrapperCombineListsBasic 
  : public WrapperAbs<FtIndexerCombineListsBasic<>, FtSearcherMem<FtIndexerCombineListsBasic<> >, SimilarityMetric> {

public:  
  WrapperCombineListsBasic(StringContainerVector* strContainer, 
			  unsigned gramLength = 3, 
			  bool usePartFilter = true,
			  float compressionRatio = 0.5, 
			  bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerCombineListsBasic<>(strContainer, this->gramGen, compressionRatio);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerCombineListsBasic<> >(&this->merger, this->indexer);   
    this->simMetric = new SimilarityMetric(*(this->gramGen));
  }
};

template <class SimilarityMetric = SimMetricEd>
class WrapperCombineListsCost 
  : public WrapperAbs<FtIndexerCombineListsCost<>, FtSearcherMem<FtIndexerCombineListsCost<> >, SimilarityMetric> {
  
public:  
  WrapperCombineListsCost(StringContainerVector* strContainer, 
			 vector<string>* workload, 
			 float optimizeThreshold, 
			 unsigned gramLength = 3, 
			 bool usePartFilter = true,
			 float reductionRatio = 0.5, 
			 bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->simMetric = new SimMetricEd(*(this->gramGen));
    this->indexer = new FtIndexerCombineListsCost<>(strContainer, 
						   this->gramGen, 
						   reductionRatio, 
						   workload, 
						   this->simMetric, 
						   optimizeThreshold);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerCombineListsCost<> >(&this->merger, this->indexer);
  }
};

#endif
