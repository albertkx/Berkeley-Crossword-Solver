/*
  $Id: wrappersimple.h 5756 2010-10-12 04:21:00Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrappersimple_h_
#define _wrappersimple_h_

#include "wrapperabs.h"

template <class SimilarityMetric = SimMetricEd>
class WrapperSimple : public WrapperAbs<FtIndexerMem<>, FtSearcherMem<>, SimilarityMetric> {  
public:    
  WrapperSimple(StringContainerVector* strContainer, GramGen* gramGen, bool usePartFilter = true) {
    this->gramGen = gramGen;
    this->indexer = new FtIndexerMem<>(strContainer, this->gramGen);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<>(&this->merger, this->indexer);
    if(!this->simAlloc) {
      this->simMetric = new SimilarityMetric(*(this->gramGen));
      this->simAlloc = true;
    }
  }
};

template <class SimilarityMetric = SimMetricEd>
class WrapperShortSimple : 
  public WrapperShortAbs<FtIndexerMem<StringContainerVector, vector<unsigned short> >, 
			 FtSearcherMem<FtIndexerMem<StringContainerVector, vector<unsigned short> >, MergeOptMerger<vector<unsigned short> > >, 
			 SimilarityMetric> {  
public:    
  WrapperShortSimple(StringContainerVector* strContainer, GramGen* gramGen, bool usePartFilter = true) {
    this->gramGen = gramGen;
    this->indexer = new FtIndexerMem<StringContainerVector, vector<unsigned short> >(strContainer, this->gramGen);
    if(usePartFilter) this->indexer->autoAddPartFilter();
    this->searcher = new FtSearcherMem<FtIndexerMem<StringContainerVector, vector<unsigned short> >, MergeOptMerger<vector<unsigned short> > >(&this->merger, this->indexer);
    if(!this->simAlloc) {
      this->simMetric = new SimilarityMetric(*(this->gramGen));
      this->simAlloc = true;
    }    
  }
};

#endif
