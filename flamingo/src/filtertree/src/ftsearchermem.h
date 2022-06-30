/*
  $Id: ftsearchermem.h 5758 2010-10-12 07:22:09Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftsearchermem_h_
#define _ftsearchermem_h_

#include "ftsearcherabs.h"
#include "ftindexermem.h"
#include "listmerger/src/mergeoptmerger.h"

template <class FtIndexer, class Merger>
class FtSearcherMem;

template <class TFtIndexer, class TMerger>
struct FtSearcherTraits<FtSearcherMem<TFtIndexer, TMerger> > {
  typedef TFtIndexer FtIndexer;
  typedef TMerger Merger;
};

template <class FtIndexer = FtIndexerMem<>, class Merger = MergeOptMerger<> >
class FtSearcherMem 
  : public FtSearcherAbs<FtSearcherMem<FtIndexer, Merger> > {
  
public:
  typedef FtIndexerTraits<FtIndexer> IndexerTraitsType;
  typedef typename IndexerTraitsType::InvList InvList;
  
protected:
  typedef FtSearcherAbs<FtSearcherMem<FtIndexer, Merger> > SuperClass;

public:
#ifdef DEBUG_STAT
  typedef WorkloadStats WlStatsType;
  typedef QueryStats QueryStatsType;
#endif
  
  FtSearcherMem(Merger* merger, FtIndexer* ftIndexer = NULL) : SuperClass(merger, ftIndexer) {
#ifdef DEBUG_STAT
    this->queryStats = new QueryStats();
#endif
  }
  
  void prepare_Impl() {}

  void searchLeafNodesPanic_Impl(const vector<FilterTreeNode<InvList>*>& leaves,
				 const Query& query,
				 vector<unsigned>& resultStringIDs);
  
  void processLeaves_Impl(const vector<FilterTreeNode<InvList>*>& leaves,
			  const Query& query,
			  const vector<unsigned>&__restrict__ queryGramCodes,
			  vector<unsigned>&__restrict__ resultStringIDs);

  string getName() {
    return "FtSearcherMem";
  }
};

template<class FtIndexer, class Merger>
void 
FtSearcherMem<FtIndexer, Merger>::
processLeaves_Impl(const vector<FilterTreeNode<InvList>*>& leaves,
		   const Query& query,
		   const vector<unsigned>&__restrict__ queryGramCodes,
		   vector<unsigned>&__restrict__ resultStringIDs) {
  
#ifdef DEBUG_STAT
  this->queryStats->strLen = query.str.size();
  this->queryStats->mergeThresh = this->mergeThreshold;
  this->queryStats->simThresh = query.threshold;
#endif
  
  // Step 0: Check for panic
  if((signed)this->mergeThreshold <= 0) {
    this->searchLeafNodesPanic(leaves, query, resultStringIDs);
    return;    
  }
  
  for(unsigned i = 0; i < leaves.size(); i++) {          
    // Step 1: Preprocess
#ifdef DEBUG_STAT    
    this->sutil.startTimeMeasurement();    
#endif
    
    vector<unsigned> candidates;
    vector<InvList*> lists;
    leaves[i]->getInvertedLists(queryGramCodes, lists);
    
#ifdef DEBUG_STAT
    this->sutil.stopTimeMeasurement();
    this->queryStats->preprocTime += this->sutil.getTimeMeasurement();
#endif
    
    // Step 2.1: Merge
#ifdef DEBUG_STAT
    this->sutil.startTimeMeasurement();
#endif
    
    this->merger->merge(lists, this->mergeThreshold, candidates);
    
#ifdef DEBUG_STAT
    this->sutil.stopTimeMeasurement();
    this->queryStats->mergeTime += this->sutil.getTimeMeasurement();
    this->queryStats->candidates += candidates.size();
#endif
    
    // Step 2.2: Postprocess
    this->postProcess(query, candidates, resultStringIDs);   
  }
}

template<class FtIndexer, class Merger>
void 
FtSearcherMem<FtIndexer, Merger>::
searchLeafNodesPanic_Impl(const vector<FilterTreeNode<InvList>*>& leaves,
			  const Query& query,
			  vector<unsigned>& resultStringIDs) {
  
#ifdef DEBUG_STAT
  this->sutil.startTimeMeasurement();
#endif
  
  if(this->ftIndexer->filterTypes.size() > 0) {
    for(unsigned i = 0; i < leaves.size(); i++) {
      FilterTreeNode<InvList>* leaf = leaves[i];
      
      // search the stringids in this leaf if there are any
      if(leaf->distinctStringIDs) {
	string dictString;
	InvList* stringIDs = leaf->distinctStringIDs->getArray( this->ftIndexer->getInvListsFile() );
	for(unsigned j = 0; j < stringIDs->size(); j++) {
	  unsigned stringId = stringIDs->at(j);
	  this->ftIndexer->strContainer->retrieveString(dictString, stringId);
	  if (query.sim.simBatch(dictString, query.str, query.threshold))
	    resultStringIDs.push_back(stringId);	    
	}
	leaf->distinctStringIDs->clear();	

#ifdef DEBUG_STAT
	this->queryStats->candidates += stringIDs->size();
#endif
      }
    }
  }
  else {
    // search all stringids
    string dictString;
    for(unsigned i = 0; i < this->ftIndexer->strContainer->size(); i++) {    
      this->ftIndexer->strContainer->retrieveString(dictString, i);
      if ( query.sim.simBatch(dictString, query.str, query.threshold))
	resultStringIDs.push_back(i);
    }
#ifdef DEBUG_STAT
	this->queryStats->candidates = this->ftIndexer->strContainer->size();
#endif
  }
    
#ifdef DEBUG_STAT
  this->sutil.stopTimeMeasurement();
  this->queryStats->panicTime = this->sutil.getTimeMeasurement();
  this->queryStats->panics = 1;
#endif
}

#endif
