/*
  $Id: wrapperondisk.h 4927 2009-12-17 21:37:29Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 02/01/2010
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrapperondisk_h_
#define _wrapperondisk_h_

#include "wrapperabs.h"

template <class SimilarityMetric = SimMetricEd>
class WrapperOnDiskSimple : public WrapperAbs<FtIndexerOnDisk<>, FtSearcherOnDisk<>, SimilarityMetric> {  
private:
  OnDiskMergerSimple<> onDiskMerger;

public:  
  WrapperOnDiskSimple(StringContainerRM* strContainer, 
		      const string& invListsFileName = "invLists.ix", 	
		      unsigned gramLength = 3, 
		      bool usePartFilter = true,
		      bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerOnDisk<>(strContainer, 
					  this->gramGen, 
					  false, 
					  invListsFileName);
    if(usePartFilter) this->indexer->autoAddPartFilter();    
    this->searcher = new FtSearcherOnDisk<>(&onDiskMerger, this->indexer);
    this->simMetric = new SimilarityMetric(*(this->gramGen));
  }  
};

template <class SimilarityMetric = SimMetricEd>
class WrapperOnDiskAdapt : public WrapperAbs<FtIndexerOnDisk<>,
					     FtSearcherOnDisk<FtIndexerOnDisk<>, OnDiskMergerAdapt<> >, 
					     SimilarityMetric> {  
private:
  OnDiskMergerAdapt<> onDiskMerger;

public:  
  WrapperOnDiskAdapt(StringContainerRM* strContainer, 
		     const string& invListsFileName = "invLists.ix", 	
		     unsigned gramLength = 3, 
		     bool usePartFilter = true,
		     bool usePrePost = true) {
    this->gramGen = new GramGenFixedLen(gramLength, usePrePost);
    this->indexer = new FtIndexerOnDisk<>(strContainer, 
					  this->gramGen, 
					  false, 
					  invListsFileName);
    if(usePartFilter) this->indexer->autoAddPartFilter();    
    this->searcher = new FtSearcherOnDisk<>(&onDiskMerger, this->indexer);
    this->simMetric = new SimilarityMetric(*(this->gramGen));
  }  
};

#endif
