/*
  $Id: ftsearcherabs.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftsearcherabs_h_
#define _ftsearcherabs_h_

#include "ftindexerabs.h"
#include "statsutil.h"
#include "filtertreenode.h"
#include "common/src/filtertypes.h"
#include "common/src/query.h"

template<class FtSearcherConcrete>
struct FtSearcherTraits;

template <class FtSearcherConcrete>
class FtSearcherAbs {
public:
  typedef FtSearcherTraits<FtSearcherConcrete> TraitsType;
  typedef typename TraitsType::FtIndexer FtIndexer;
  typedef typename TraitsType::Merger Merger;

  typedef FtIndexerTraits<FtIndexer> IndexerTraitsType;  
  typedef typename IndexerTraitsType::InvList InvList;
  
protected:  
#ifdef DEBUG_STAT
  QueryStats* queryStats;
  StatsUtil sutil;
#endif
  
  unsigned mergeThreshold;
  GramGen* gramGen;
  StatsUtil sutilLocal;
  
  Merger* merger;
  FtIndexer* ftIndexer;
  
  void getAffectedLeavesRec(FilterTreeNode<InvList>* node,
			    const Query& query,
			    const vector<unsigned>& queryGramCodes,
			    const vector<AbstractFilter*>* filterTypes,
			    const unsigned filterId,
			    vector<FilterTreeNode<InvList>*>& leaves);
  
  void processLeaves(const vector<FilterTreeNode<InvList>*>& leaves,
		     const Query& query,
		     const vector<unsigned>& queryGramCodes,
		     vector<unsigned>& resultStringIDs) {
    static_cast<FtSearcherConcrete*>(this)->processLeaves_Impl(leaves, 
							       query, 
							       queryGramCodes,
							       resultStringIDs);
  }
  
  void searchLeafNodesPanic(const vector<FilterTreeNode<InvList>*>& leaves,
			    const Query& query,
			    vector<unsigned>& resultStringIDs) {
    static_cast<FtSearcherConcrete*>(this)->searchLeafNodesPanic_Impl(leaves, 
								      query, 
								      resultStringIDs);

  }
  
  // remove deleted string ids and remove false positives by computing real similarity
  void postProcess(const Query& query,
		   const vector<unsigned>& candidates, 
		   vector<unsigned>& resultStringIDs);
  
public:
  FtSearcherAbs(Merger* merger, FtIndexer* ftIndexer = NULL);
    
  void prepare() { static_cast<FtSearcherConcrete*>(this)->prepare_Impl(); }
  void search(const Query& query, vector<unsigned>& resultStringIDs);
  void rangeSearch(const Query& query, vector<unsigned> queryGramCodes, vector<unsigned>& resultStringIDs);
  void topkSearch(const Query& query, vector<unsigned> queryGramCodes, vector<unsigned>& resultStringIDs);
  
  void setFtIndexer(FtIndexer* ftIndexer) { 
    this->ftIndexer = ftIndexer; if(ftIndexer) gramGen = ftIndexer->getGramGen(); 
  }

  string getName() {
    return "FtSearcherAbs";
  }
  
#ifdef DEBUG_STAT
  QueryStats* getQueryStats() { return queryStats; }
#endif
  
  virtual ~FtSearcherAbs() {
#ifdef DEBUG_STAT
    if(queryStats) delete queryStats;
#endif
  }
};

template<class FtSearcherConcrete>
FtSearcherAbs<FtSearcherConcrete>::
FtSearcherAbs(Merger* merger, FtIndexer* ftIndexer)
  : merger(merger), ftIndexer(ftIndexer) {
  if(ftIndexer) gramGen = ftIndexer->getGramGen();
  else gramGen = NULL;
}

template<class FtSearcherConcrete>
void 
FtSearcherAbs<FtSearcherConcrete>::
getAffectedLeavesRec(FilterTreeNode<InvList>* node,
		     const Query& query,
		     const vector<unsigned>& queryGramCodes,
		     const vector<AbstractFilter*>* filterTypes,
		     const unsigned filterId,
		     vector<FilterTreeNode<InvList>*>& leaves) {
  
  if(node->isLeaf()) leaves.push_back(node);
  else {
    AbstractFilter* filter = filterTypes->at(filterId);
    
    // get query bounds for this filter
    unsigned lbound, ubound;
    query.sim.getFilterBounds(query.str, query.threshold, filter, lbound, ubound);    
    unsigned nodeLbound = filter->getFilterLbound();
    
    // binary search to first affected child
    vector<KeyNodePair<InvList> >& children = node->children;
    typename vector<KeyNodePair<InvList> >::iterator iter;
    KeyNodePair<InvList> lboundKnp(lbound, NULL);
    iter = lower_bound(children.begin(), children.end(), lboundKnp);
    
    // continue recursing into children as long as their keys overlap with lbound, ubound
    while(iter != children.end() && nodeLbound < ubound) {
      getAffectedLeavesRec(iter->node, query, queryGramCodes, filterTypes, filterId+1, leaves);
      nodeLbound = iter->key;
      iter++;
    }    
  }
}

class SimIdPair {
public:

  float sim;
  unsigned id;
  
  SimIdPair(float sim, unsigned id) : sim(sim), id(id) {}
  
  bool operator==(const SimIdPair& b) const {
    return sim == b.sim;
  }    
  
  bool operator<(const SimIdPair& b) const {    
    return sim >= b.sim;
  }
};

template<class FtSearcherConcrete>
void
FtSearcherAbs<FtSearcherConcrete>::
topkSearch(const Query& query, vector<unsigned> queryGramCodes, vector<unsigned>& resultStringIDs) {
  
  // answer topk queries with multiple range searches
  Query rangeQuery(query.str, query.sim, 0.0f, 0, QueryRange);
  rangeQuery.threshold = rangeQuery.sim.topkInitialRange();
  unordered_set<unsigned> resultIDs;  

  // try multiple range searches, and exit if we detect a panic to switch to "scan" mode
  while(true) {
    
    unsigned T = query.sim.getMergeThreshold(rangeQuery.str, queryGramCodes, rangeQuery.threshold, ftIndexer->getCompressionArgs());
    
    // if we panic, immediately switch to scan mode
    if((signed)T <= 0) break;
    
    vector<unsigned> tmpResults;
    rangeSearch(rangeQuery, queryGramCodes, tmpResults);       
    
    for(unsigned i = 0; i < tmpResults.size(); i++) {
      unordered_set<unsigned>::iterator iter = resultIDs.find(tmpResults[i]);
      if(iter == resultIDs.end()) {
	resultStringIDs.push_back(tmpResults[i]);	
	if(resultStringIDs.size() >= query.k) return;
	resultIDs.insert(tmpResults[i]);
      }
    }
    
    rangeQuery.threshold = rangeQuery.sim.topkNextRange(rangeQuery.threshold, rangeQuery.str);
  }

  // still haven't found k results, switch to "scan" mode
  if(resultStringIDs.size() < query.k) {    
    // TODO: need to switch between min/max heap for different sim functions
    // currently is max heap, e.g. for edit distance

    vector<SimIdPair> topElements;
    topElements.reserve(query.k + 10);
    
    if(this->ftIndexer->strContainer->getPhysOrd() == PHO_LENGTH
       || this->ftIndexer->strContainer->getPhysOrd() == PHO_LENGTH_CHARSUM) {
      
      // determine whether to forward-scan or backward-scan strings
      bool forwardScan = true;
      if(this->ftIndexer->strContainer->getStatsCollector()) {
	if(query.str.length() > this->ftIndexer->strContainer->getStatsCollector()->getAvgStrLen()) {
	  forwardScan = false;
	}
      }
      
      if(forwardScan) {
	for(unsigned i = 0; i < this->ftIndexer->strContainer->size(); i++) {
	  string candi;
	  this->ftIndexer->strContainer->retrieveString(candi, i);	
	  
	  // length filter, assuming edit distance, TODO: make generic
	  unsigned lengthDiff = (candi.length() > query.str.length()) 
	    ? candi.length() - query.str.length() 
	    : query.str.length()- candi.length();
	  
	  float simBound = 50000000; // TODO: change dirty constant
	  if(!topElements.empty()) {
	    simBound = topElements.front().sim;
	    if(lengthDiff > simBound) {
	      if(candi.length() >= query.str.length()) break;
	      else continue;
	    }
	  }
	  
	  // first try to beat simBound with letter counting filter
	  if(!rangeQuery.sim.lcFilter(rangeQuery.str, candi, simBound)) continue;
	  
	  // compute real similarity and try to push to heap
	  float trueSim = rangeQuery.sim.simBatch(rangeQuery.str, candi);
	  if(trueSim < simBound) {
	    SimIdPair sip(trueSim, i);
	    if(topElements.size() >= query.k) {
	      pop_heap(topElements.begin(), topElements.end());
	      topElements.pop_back();
	    }
	    topElements.push_back(sip);
	    push_heap(topElements.begin(), topElements.end());
	  }	  
	}
      }
      else {
	for(unsigned i = this->ftIndexer->strContainer->size()-1; i >= 0; i--) {
	  string candi;
	  this->ftIndexer->strContainer->retrieveString(candi, i);	
	  
	  // length filter, assuming edit distance, TODO: make generic
	  unsigned lengthDiff = (candi.length() > query.str.length()) 
	    ? candi.length() - query.str.length() 
	    : query.str.length()- candi.length();
	  
	  float simBound = 50000000; // TODO: change dirty constant
	  if(!topElements.empty()) {
	    simBound = topElements.front().sim;
	    if(lengthDiff > simBound) {
	      if(candi.length() <= query.str.length()) break;
	      else continue;
	    }
	  }
	  
	  // first try to beat simBound with letter counting filter
	  if(!rangeQuery.sim.lcFilter(rangeQuery.str, candi, simBound)) continue;
	  
	  // compute real similarity and try to push to heap
	  float trueSim = rangeQuery.sim.simBatch(rangeQuery.str, candi);
	  if(trueSim < simBound) {
	    SimIdPair sip(trueSim, i);
	    if(topElements.size() >= query.k) {
	      pop_heap(topElements.begin(), topElements.end());
	      topElements.pop_back();
	    }
	    topElements.push_back(sip);
	    push_heap(topElements.begin(), topElements.end());     
	  }	  
	}
      }
    }
    else {
      for(unsigned i = 0; i < this->ftIndexer->strContainer->size(); i++) {
	string candi;
	this->ftIndexer->strContainer->retrieveString(candi, i);	
	
	// length filter, assuming edit distance, TODO: make generic
	unsigned lengthDiff = (candi.length() > query.str.length()) 
	  ? candi.length() - query.str.length() 
	  : query.str.length()- candi.length();
	
	float simBound = 50000000; // TODO: change dirty constant
	if(!topElements.empty()) {
	  simBound = topElements.front().sim;
	  if(lengthDiff > simBound) continue;
	}

	// first try to beat simBound with letter counting filter
	if(!rangeQuery.sim.lcFilter(rangeQuery.str, candi, simBound)) continue;
	
	// compute real similarity and try to push to heap
	float trueSim = rangeQuery.sim.simBatch(rangeQuery.str, candi);
	if(trueSim < simBound) {
	  SimIdPair sip (trueSim, i);
	  if(topElements.size() >= query.k) {
	    pop_heap(topElements.begin(), topElements.end());
	    topElements.pop_back();
	  }
	  topElements.push_back(sip);
	  push_heap(topElements.begin(), topElements.end());     
	}
      }
    }      

    // fill result string ids
    resultStringIDs.clear();
    while(!topElements.empty()) {
      resultStringIDs.push_back(topElements.front().id);
      pop_heap(topElements.begin(), topElements.end());
      topElements.pop_back();
    }
  }
}

template<class FtSearcherConcrete>
void
FtSearcherAbs<FtSearcherConcrete>::
rangeSearch(const Query& query, vector<unsigned> queryGramCodes, vector<unsigned>& resultStringIDs) {
  
  vector<AbstractFilter*>* filterTypes = &ftIndexer->filterTypes;
  
#ifdef DEBUG_STAT
  this->queryStats->reset();
  this->sutil.startTimeMeasurement();
#endif

  mergeThreshold = query.sim.getMergeThreshold(query.str, 
					       queryGramCodes, 
					       query.threshold,
					       ftIndexer->getCompressionArgs());
  
#ifdef DEBUG_STAT
  this->sutil.stopTimeMeasurement();
  this->queryStats->threshTime += this->sutil.getTimeMeasurement();
#endif
  
  // get affected leaves and process them
  vector<FilterTreeNode<InvList>*> leaves;
  getAffectedLeavesRec(ftIndexer->filterTreeRoot,
		       query,
		       queryGramCodes,
		       filterTypes,
		       0,
		       leaves);
  
  processLeaves(leaves,
		query,
		queryGramCodes,
		resultStringIDs);
  
  

#ifdef DEBUG_STAT
  this->queryStats->totalTime += 
    this->queryStats->threshTime +
    this->queryStats->preprocTime +
    this->queryStats->mergeTime +
    this->queryStats->postprocTime +
    this->queryStats->panicTime;
  this->queryStats->results += resultStringIDs.size();
#endif
}


template<class FtSearcherConcrete>
void 
FtSearcherAbs<FtSearcherConcrete>::
search(const Query& query, vector<unsigned>& resultStringIDs) {
  
  // create gram codes from query
  vector<unsigned> queryGramCodes;
  gramGen->decompose(query.str, queryGramCodes);
  
  // prepare simmetric
  switch(query.sim.smt) { 

  case SMT_JACCARD:
  case SMT_COSINE:
  case SMT_DICE:
  case SMT_JACCARD_BAG:
  case SMT_COSINE_BAG:
  case SMT_DICE_BAG:
  case SMT_GRAMCOUNT: {
    StatsCollector* statsColl = ftIndexer->strContainer->getStatsCollector();
    if(statsColl) {   
      query.sim.prepare(statsColl->minStrLen, queryGramCodes);
    }
    else {
      cout << "ERROR: gram based simmetric selected but no stats in stringcontainer. minStrLen needed!" << endl;
      return;
    }
  } break;
  
  case SMT_EDIT_DISTANCE:
  case SMT_EDIT_DISTANCE_NORM:
  case SMT_EDIT_DISTANCE_SWAP: {
    query.sim.prepare(0, queryGramCodes);
  } break;

  
  default: {
    query.sim.prepare(0, queryGramCodes);
  } break;

  }
  
  // distinguish different query types
  switch(query.type) {
  case QueryRange: {
    rangeSearch(query, queryGramCodes, resultStringIDs);
  } break;
    
  case QueryTopk: {
    topkSearch(query, queryGramCodes, resultStringIDs);
  } break;

  default: {
    cout << "ERROR: SPECIFIED QUERYTYPE NOT IMPLEMENTED" << endl;
  } break;

  }  
}

template<class FtSearcherConcrete>
void 
FtSearcherAbs<FtSearcherConcrete>::
postProcess(const Query& query,
	    const vector<unsigned>& candidates, 
	    vector<unsigned>& resultStringIDs) {
  
#ifdef DEBUG_STAT
  this->sutil.startTimeMeasurement();
#endif
  
  const unordered_set<unsigned>& deletedStringIds = this->ftIndexer->getDeletedStringIds();
  if(deletedStringIds.size() > 0) {
    for(unsigned i = 0; i < candidates.size(); i++) {
      unsigned stringId = candidates[i];
      if(deletedStringIds.find(stringId) != deletedStringIds.end()) continue;
      string dictString;
      this->ftIndexer->strContainer->retrieveString(dictString, stringId);
      if(query.sim.simBatch(dictString, query.str, query.threshold))
	resultStringIDs.push_back(stringId);
    }         
  }
  else {
    for(unsigned i = 0; i < candidates.size(); i++) {
      unsigned stringId = candidates[i];    
      string dictString;
      this->ftIndexer->strContainer->retrieveString(dictString, stringId);
      if(query.sim.simBatch(dictString, query.str, query.threshold))
	resultStringIDs.push_back(stringId);
    }     
  }
  
#ifdef DEBUG_STAT
  this->sutil.stopTimeMeasurement();
  this->queryStats->postprocTime += this->sutil.getTimeMeasurement();
#endif
}

#endif
