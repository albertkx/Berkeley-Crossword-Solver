/*
  $Id$

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/23/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ondiskmergerabs_h_
#define _ondiskmergerabs_h_

#include "filtertree/src/gramlist.h"
#include "filtertree/src/gramlistondisk.h"
#include "filtertree/src/statsutil.h"
#include "filtertree/src/stringcontainer.h"
#include "common/src/query.h"
#include "common/src/filtertypes.h"

template <class OnDiskMergerConcrete, class InvList>
class OnDiskMergerAbs {

protected:
  GramGen* gramGen;
  
public:   
  bool singleFilterOpt;
  
  // for stats gathering
  unsigned numberStrings;
  unsigned invListSeeks;
  double invListData;
  unsigned initialCandidates;
  
  PostMergeFilter pmf;

  // for charsumfiltering
  QueryCharsumStats* queryCharsumStats;
  StrCharsumStats* charsumStats;
  CharsumFilter* charsumFilter;

  // for letter count filtering
  unsigned lcCharNum;
  StrLcStats* queryLcStats;
  StrLcStats* dataLcStats;
  
  // for hash count filtering
  unsigned hcBuckets;
  StrHcStats* queryHcStats;
  StrHcStats* dataHcStats;

  StringContainerRM* strContainer;   // TODO: JUST FOR EXPERIMENTS, REMOVE LATER
  unordered_map<unsigned, string>* gramcode2gram;
  
  OnDiskMergerAbs(bool singleFilterOpt = true) 
    : singleFilterOpt(singleFilterOpt), numberStrings(1),
    invListSeeks(0), invListData(0.0), initialCandidates(0),
    pmf(PMF_NONE), queryCharsumStats(NULL), charsumStats(NULL), charsumFilter(NULL),
    lcCharNum(NULL), queryLcStats(NULL), dataLcStats(NULL),
    hcBuckets(NULL), queryHcStats(NULL), dataHcStats(NULL),
    strContainer(NULL), gramcode2gram(NULL) { }
  
  bool estimationParamsNeeded() {
    return static_cast<OnDiskMergerConcrete*>(this)->estimationParamsNeeded_Impl();
  }
  
  void setEstimationParams(float readInvListTimeSlope, float readInvListTimeIntercept, float readStringAvgTime) { 
    static_cast<OnDiskMergerConcrete*>(this)->setEstimationParams_Impl(readInvListTimeSlope, readInvListTimeIntercept, readStringAvgTime);
  }

  void setGramGen(GramGen* gramGen) { 
    this->gramGen = gramGen;
  }
  
  void merge(const Query& query,
	     vector<vector<QueryGramList<InvList>* >* >& qgls,
	     unsigned threshold,
	     fstream& invListsFile,
	     unsigned numberFilters,
	     vector<unsigned>& results) {
    static_cast<OnDiskMergerConcrete*>(this)->merge_Impl(query, qgls, threshold, invListsFile, numberFilters, results);
  }
  
  string getName() {
    return "OnDiskMergerAbs";
  }
};

#endif
