/*
  $Id: ftindexerdiscardlists.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the Academic BSD license.
    
  Date: 12/27/2007
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftindexerdiscardlists_h_
#define _ftindexerdiscardlists_h_

#include "ftindexermemabs.h"
#include "ftindexermem.h"
#include "common/src/simmetric.h"
#include "listmerger/src/divideskipmerger.h"

struct ValPair {
  unsigned gramCode;
  unsigned listSize;
};

struct ValPairCmpFunc {
  bool operator()(const ValPair& vp1, const ValPair& vp2) const { ;
    return (vp1.listSize >= vp2.listSize);
  }
};

template<class FtIndexerConcrete>
class FtIndexerDiscardListsAbs;

template<class FtIndexerConcrete>
struct FtIndexerTraits<FtIndexerDiscardListsAbs<FtIndexerConcrete> > {
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
};

template<class FtIndexerDiscardListsConcrete>
class FtIndexerDiscardListsAbs 
  : public FtIndexerMemAbs<FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete> > {   
  
public:
  typedef FtIndexerTraits<FtIndexerDiscardListsConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;

#ifdef DEBUG_STAT
  typedef HolesGlobalIndexStats IxStatsType;
#endif

private:
  typedef 
  FtIndexerMemAbs<FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete> > 
  SuperClass;
  
protected:
  typedef typename SuperClass::GramMap GramMap;
  
  float reductionRatio;
  set<unsigned> holeGrams;      

  bool duplicateGram(const unsigned gramCode, 
		     const vector<unsigned>& gramCodes, 
		     const unsigned iterStep);
  
  void fillStructsForHoles(set<ValPair, ValPairCmpFunc>& orderedGramSet, 
			   unordered_map<unsigned, unsigned>& countMap,
			   long unsigned* totalListElements);  
  
  void saveHoleGrams(ofstream& fpOut);
  void loadHoleGrams(ifstream& fpIn);
  
public:
  FtIndexerDiscardListsAbs(StringContainer* strContainer) : SuperClass(strContainer) {
#ifdef DEBUG_STAT
    this->ixStats = new HolesGlobalIndexStats();
#endif
  }
  
  FtIndexerDiscardListsAbs(StringContainer* strContainer,
			  GramGen* gramGenerator, 
			  float reductionRatio,
			  unsigned maxStrLen = 150, 
			  unsigned ftFanout = 50) 
    : SuperClass(strContainer, gramGenerator, maxStrLen, ftFanout), reductionRatio(reductionRatio) {
#ifdef DEBUG_STAT
    this->ixStats = new HolesGlobalIndexStats();
    dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->reductionRatio = reductionRatio;
#endif
  }
    
  // statically delegate the prepare implementation to appropriate subclass
  void prepare_Impl() {
    static_cast<FtIndexerDiscardListsConcrete*>(this)->chooseHoleGrams();
  }
  
  void addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId);
  CompressionArgs* getCompressionArgs_Impl();

  string getName() { 
    return "FtIndexerDiscardListsAbs"; 
  }

  void saveIndex_Impl(const char* indexFileName);
  void loadIndex_Impl(const char* indexFileName); 
};


template<class FtIndexerDiscardListsConcrete>
bool
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
duplicateGram(const unsigned gramCode,
	      const vector<unsigned>& gramCodes,
	      const unsigned iterStep) {     
  for(signed i = (signed)iterStep - 1; i >= 0; i--) {
    if(i > 0) 
      if(gramCode == gramCodes.at(i)) return true;    
  }
  return false;
}

template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
fillStructsForHoles(set<ValPair, ValPairCmpFunc>& orderedGramSet, 
		    unordered_map<unsigned, unsigned>& countMap,
		    long unsigned* totalListElements) {
  
  unsigned long tmpTotalListElements = 0;
  
  // compute gram counts
  for(unsigned stringId = 0; stringId < this->strContainer->size(); stringId++) {
    vector<unsigned> gramCodes;
    string currentString;
    this->strContainer->retrieveString(currentString, stringId);
    this->gramGen->decompose(currentString, gramCodes);
    
    for(unsigned i = 0; i < gramCodes.size(); i++) {
      unsigned gramCode = gramCodes.at(i);      
      if(!duplicateGram(gramCode, gramCodes, i)) {
	if(countMap.find(gramCode) == countMap.end()) countMap[gramCode] = 1;
	else countMap[gramCode]++;
	tmpTotalListElements++;
      }
    }
  }
  
  // create set ordered descending by listSize
  for (unordered_map<unsigned, unsigned>::iterator iter = countMap.begin(); 
       iter != countMap.end(); 
       iter++ ) {
    
    struct ValPair vp;
    vp.gramCode = iter->first;
    vp.listSize = iter->second;
    
    orderedGramSet.insert(vp);
  }  
  
  *totalListElements = tmpTotalListElements;
}
  
template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId) {
  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes.at(i);
    
    // ignore grams in set of holeGrams
    if(holeGrams.find(gramCode) != holeGrams.end()) continue;
    
    GramMap& gramMap = leaf->gramMap;
    if (gramMap.find(gramCode) == gramMap.end()) {
      // a new gram
      GramList<InvList>* newGramList = new GramListSimple<InvList>();
      gramMap[gramCode] = newGramList;    
      InvList* array = newGramList->getArray();
      array->push_back(stringId);
    }
    else {
      // an existing gram
      GramList<InvList>* gramList = gramMap[gramCode];
      InvList* array = gramList->getArray();
      // avoid adding duplicate elements
      if(array->back() != stringId)
	array->push_back(stringId);
    }	      
  }
}

template<class FtIndexerDiscardListsConcrete>
CompressionArgs*
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
getCompressionArgs_Impl() {
  this->compressionArgs.blackList = &holeGrams;
  this->compressionArgs.holesOptimized = true;
  return &(this->compressionArgs);
}

template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
saveHoleGrams(ofstream& fpOut) {
  unsigned u;
  u = holeGrams.size();
  fpOut.write((const char*)&u, sizeof(unsigned));    
  
  for(set<unsigned>::iterator iter = holeGrams.begin();
      iter != holeGrams.end();
      iter++) {
    u = *iter;
    fpOut.write((const char*)&u, sizeof(unsigned));    
  }
}

template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
loadHoleGrams(ifstream& fpIn) {
  unsigned nGrams;
  unsigned u;
  fpIn.read((char*)&nGrams, sizeof(unsigned));
  for(unsigned i = 0; i < nGrams; i++) {
    fpIn.read((char*)&u, sizeof(unsigned));
    holeGrams.insert(u);
  }        
}

template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
saveIndex_Impl(const char* indexFileName) {
  ofstream fpOut;
  fpOut.open(indexFileName, ios::out);
  if(fpOut.is_open()) {
    this->saveBasicInfo(fpOut);
    this->saveLeavesRec(this->filterTreeRoot, fpOut);    
    saveHoleGrams(fpOut);    
    fpOut.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;  
}

template<class FtIndexerDiscardListsConcrete>
void
FtIndexerDiscardListsAbs<FtIndexerDiscardListsConcrete>::
loadIndex_Impl(const char* indexFileName) {
  ifstream fpIn;
  fpIn.open(indexFileName, ios::in);
  if(fpIn.is_open()) {
    this->filterTypes.clear();  
    this->loadBasicInfo(fpIn);  
    this->buildHollowTreeRecursive(this->filterTreeRoot, 0);  
    this->loadLeavesRec(this->filterTreeRoot, fpIn);  
    loadHoleGrams(fpIn);
    fpIn.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;    
}

template<class StringContainer, class InvList>
class FtIndexerDiscardListsLLF;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerDiscardListsLLF<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerDiscardListsLLF 
  : public FtIndexerDiscardListsAbs<FtIndexerDiscardListsLLF<StringContainer, InvList> > {

private:
  typedef FtIndexerDiscardListsAbs<FtIndexerDiscardListsLLF<StringContainer, InvList> > SuperClass;
  
public:
  FtIndexerDiscardListsLLF(StringContainer* strContainer) : SuperClass(strContainer) {}
  FtIndexerDiscardListsLLF(StringContainer* strContainer,
			  GramGen* gramGen, 
			  float reductionRatio, 
			  unsigned maxStrLength = 150, 
			  unsigned ftFanout = 50)
    : SuperClass(strContainer, gramGen, reductionRatio, maxStrLength, ftFanout) {
    this->indexerType = HOLES_GLOBAL_LLF;
  }
  
  void chooseHoleGrams();
  
  string getName() { 
    return "FtIndexerDiscardListsLLF"; 
  }
};

template<class StringContainer, class InvList>
void 
FtIndexerDiscardListsLLF<StringContainer, InvList>::
chooseHoleGrams() {
  set<ValPair, ValPairCmpFunc> orderedGramSet;
  unordered_map<unsigned, unsigned> countMap;
  unsigned long totalListElements = 0;
  
  this->fillStructsForHoles(orderedGramSet, countMap, &totalListElements);
  
  // create gram blacklist, i.e. which grams should be holes
  unsigned long memmax = (unsigned long)(totalListElements * this->reductionRatio);
  unsigned memsum = 0;
  set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
  while(memsum < memmax && iter != orderedGramSet.end()) {
    memsum += (*iter).listSize;
    this->holeGrams.insert((*iter).gramCode);
    iter++;
  }
  
#ifdef DEBUG_STAT
  dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->numberHoles = this->holeGrams.size();
#endif
}


template<class StringContainer, class InvList>
class FtIndexerDiscardListsSLF;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerDiscardListsSLF<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerDiscardListsSLF 
  : public FtIndexerDiscardListsAbs<FtIndexerDiscardListsSLF<StringContainer, InvList> > {

 private:
  typedef FtIndexerDiscardListsAbs<FtIndexerDiscardListsSLF<StringContainer, InvList> > SuperClass;

 public:
  FtIndexerDiscardListsSLF(StringContainer* strContainer) : SuperClass(strContainer) {}
  FtIndexerDiscardListsSLF(StringContainer* strContainer, 
			  GramGen* gramGen, 
			  float reductionRatio, 
			  unsigned maxStrLength = 150, 
			  unsigned ftFanout = 50)
    : SuperClass(strContainer, gramGen, reductionRatio, maxStrLength, ftFanout) {
    this->indexerType = HOLES_GLOBAL_SLF;
  }
  
  void chooseHoleGrams();

  string getName() { 
    return "FtIndexerDiscardListsSLF"; 
  }
};

template<class StringContainer, class InvList>
void 
FtIndexerDiscardListsSLF<StringContainer, InvList>::
chooseHoleGrams() {
  set<ValPair, ValPairCmpFunc> orderedGramSet;
  unordered_map<unsigned, unsigned> countMap;
  unsigned long totalListElements = 0;
  
  this->fillStructsForHoles(orderedGramSet, countMap, &totalListElements);
  
  // create gram blacklist, i.e. which grams should be holes
  unsigned long memmax = (unsigned long)(totalListElements * this->reductionRatio);
  unsigned memsum = 0;
  set<ValPair, ValPairCmpFunc>::reverse_iterator iter =  orderedGramSet.rbegin();
  while(memsum < memmax && iter != orderedGramSet.rend()) {
    memsum += (*iter).listSize;
    this->holeGrams.insert((*iter).gramCode);
    iter++;
  }  

#ifdef DEBUG_STAT
  dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->numberHoles = this->holeGrams.size();
#endif
}

template<class StringContainer, class InvList>
class FtIndexerDiscardListsRandom;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerDiscardListsRandom<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerDiscardListsRandom 
  : public FtIndexerDiscardListsAbs<FtIndexerDiscardListsRandom<StringContainer, InvList> > {
  
private:
  typedef FtIndexerDiscardListsAbs<FtIndexerDiscardListsRandom<StringContainer, InvList> > SuperClass;

public:
  FtIndexerDiscardListsRandom(StringContainer* strContainer) : SuperClass(strContainer) {}
  FtIndexerDiscardListsRandom(StringContainer* strContainer,
			     GramGen* gramGen, 
			     float reductionRatio, 
			     unsigned maxStrLength = 150, 
			     unsigned ftFanout = 50)
    : SuperClass(strContainer, gramGen, reductionRatio, maxStrLength, ftFanout) {
    this->indexerType = HOLES_GLOBAL_RANDOM;
  }
  
  void chooseHoleGrams();

  string getName() { 
    return "FtIndexerDiscardListsRandom"; 
  }
};

template<class StringContainer, class InvList>
void 
FtIndexerDiscardListsRandom<StringContainer, InvList>::
chooseHoleGrams() {
  set<ValPair, ValPairCmpFunc> orderedGramSet;
  unordered_map<unsigned, unsigned> countMap;
  unsigned long totalListElements = 0;
  
  this->fillStructsForHoles(orderedGramSet, countMap, &totalListElements);
  
  // create gram blacklist, i.e. which grams should be holes
  unsigned long memmax = (unsigned long)(totalListElements * this->reductionRatio);
  unsigned memsum = 0;
  
  while(memsum < memmax && orderedGramSet.size() > 0) {
    unsigned rand = random() % orderedGramSet.size();
    unsigned iterfind = 0;
    set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
    while(iterfind < rand) {
      iter++;
      iterfind++;
    }
    memsum += (*iter).listSize;
    this->holeGrams.insert((*iter).gramCode);
    orderedGramSet.erase(iter);
  }  
  
#ifdef DEBUG_STAT
  dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->numberHoles = this->holeGrams.size();
#endif
}


// used by the cost based hole choosers
typedef struct {
  unsigned length;
  vector<unsigned> gramCodes;
  char holes;
  unsigned originalCandidates;
  vector<unsigned*> scanCounts;
  vector<unsigned*>* nonZeroScanCounts;
  unsigned panicCost;
  unsigned originalThreshold;
  vector<FilterTreeNode<>* > affectedLeaves;
  unordered_map<unsigned, unsigned> gramOccur;
  bool isPanic;
} QueryInfo;

template<class FtIndexerConcrete>
class FtIndexerDiscardListsCostAbs;

template<class FtIndexerConcrete>
struct FtIndexerTraits<FtIndexerDiscardListsCostAbs<FtIndexerConcrete> > {
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
};

template<class FtIndexerDiscardListsConcrete>
class FtIndexerDiscardListsCostAbs 
  : public FtIndexerDiscardListsAbs
  <FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete> > {   

public:
  typedef FtIndexerTraits<FtIndexerDiscardListsConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;

private:
  typedef 
  FtIndexerDiscardListsAbs<FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete> > 
  SuperClass;
  
protected:
  typedef typename SuperClass::GramMap GramMap;

  const vector<string>* workload;
  const SimMetric* simMetric;
  float simThreshold;
  bool ratioCost;
  float dictSamplingFrac;
  float queriesSamplingFrac;

  void fillSampleSets(const bool trueRand, vector<string>& dictSampleSet, vector<string>& queriesSampleSet);

  bool cleanseOrderedGramSet(const unsigned long totalElements,
			     const unsigned numberLists,
			     set<ValPair, ValPairCmpFunc>& orderedGramSet);

  void getFilterTreeLeaves(const FtIndexerMem<>& ftIndexer,
			   FilterTreeNode<vector<unsigned> >* node, 
			   const string& query, 
			   const vector<AbstractFilter*>& filterTypes, 
			   const unsigned filterId, 
			   vector<FilterTreeNode<vector<unsigned> >* >& leaves);

  unsigned initScanCountArray(const vector<vector<unsigned>*> &arrays,
			      const unsigned threshold,
			      const unsigned arraySize,
			      unsigned scanCountArray[]);
  
  void fillQueryInfo(const FtIndexerMem<>& sampleIndexer,
		     const vector<string>& queriesSampleSet, 
		     const unsigned dictSize,
		     QueryInfo queryInfo[], 
		     unsigned& totalMergeElements,
		     unsigned& totalCandidates,
		     unsigned& totalPanicCandidates);

  void freeQueryInfo(QueryInfo queryInfo[], const unsigned size);

public:
  FtIndexerDiscardListsCostAbs(StringContainer* strContainer) : SuperClass(strContainer) {} ;
  FtIndexerDiscardListsCostAbs(StringContainer* strContainer,
			      GramGen* gramGen, 
			      float reductionRatio, 
			      const vector<string>* workload,
			      const SimMetric* simMetric,
			      float simThreshold,
			      bool ratioCost = true,
			      float dictSamplingFrac = 0.01f,
			      float queriesSamplingFrac = 0.25f,
			      unsigned maxStrLen = 150, 
			      unsigned ftFanout = 50)
    : SuperClass(strContainer, gramGen, reductionRatio, maxStrLen, ftFanout),
      workload(workload), simMetric(simMetric), simThreshold(simThreshold), ratioCost(ratioCost),
      dictSamplingFrac(dictSamplingFrac), queriesSamplingFrac(queriesSamplingFrac) {}
  
  // statically delegate the chooseHoleGrams method to appropriate subclass
  void chooseHoleGrams() {
    static_cast<FtIndexerDiscardListsConcrete*>(this)->chooseHoleGrams_Cost();
  }  
};

template<class FtIndexerDiscardListsConcrete>
void 
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
fillSampleSets(const bool trueRand, vector<string>& dictSampleSet, vector<string>& queriesSampleSet) {
  if(trueRand) {
    srand(time(NULL));
  }
  else srand(150);  

  // choose dictionary sample
  if(dictSamplingFrac < 1.0) {
    unsigned dictSampleStrings = (unsigned)ceil((float)this->strContainer->size() * dictSamplingFrac); 
    for(unsigned i = 0; i < dictSampleStrings; i++) {
      unsigned stringIndex = rand() % this->strContainer->size();
      string currentString;
      this->strContainer->retrieveString(currentString, stringIndex);
      dictSampleSet.push_back(currentString);
    }
  }
  else {
    for(unsigned i = 0; i < this->strContainer->size(); i++) {
      string currentString;
      this->strContainer->retrieveString(currentString, i);
      dictSampleSet.push_back(currentString);
    }
  }

  // choose queries
  if(queriesSamplingFrac < 1.0) {
    unsigned queriesSampleStrings = (unsigned)floor((float)workload->size() * queriesSamplingFrac); 
    for(unsigned i = 0; i < queriesSampleStrings; i++) {
      unsigned stringIndex = rand() % workload->size();
      queriesSampleSet.push_back(workload->at(stringIndex));
    }
  }
  else {
    for(unsigned i = 0; i < workload->size(); i++) {
      queriesSampleSet.push_back(workload->at(i));
    }
  }
}

template<class FtIndexerDiscardListsConcrete>
bool
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
cleanseOrderedGramSet(const unsigned long totalElements,
		      const unsigned numberLists,
		      set<ValPair, ValPairCmpFunc>& orderedGramSet) {
  
  set<ValPair, ValPairCmpFunc>::iterator del;
  bool found = false;
  
  unsigned avgListSize = (unsigned) floor( (double)totalElements / (double)numberLists );
  unsigned threshold = avgListSize;
  unsigned long sum = 0;    
  unsigned long memmax = (unsigned long)(totalElements * this->reductionRatio);
  for(set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
      iter != orderedGramSet.end();
      iter++) {
    
    sum += iter->listSize;
    
    if(iter->listSize <= threshold && sum >= memmax) {
      del = iter;
      found = true;
      break;
    }
  }
  
  if(found) {
    orderedGramSet.erase(del, orderedGramSet.end());  
    return true;
  }
  else return false;  
}

template<class FtIndexerDiscardListsConcrete>
void 
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
getFilterTreeLeaves(const FtIndexerMem<>& ftIndexer,
		    FilterTreeNode<vector<unsigned> >* node, 
		    const string& query, 
		    const vector<AbstractFilter*>& filterTypes, 
		    const unsigned filterId, 
		    vector<FilterTreeNode<vector<unsigned> >* >& leaves) {

  // in case no filters were selected just add the leaf of the root
  if(filterTypes.size() == 0) {
    leaves.push_back(node);
    return;
  }
  
  if(node->isLeaf()) leaves.push_back((FilterTreeNode<vector<unsigned> >*)node);
  else {
    AbstractFilter* filter = filterTypes.at(filterId);
      
    // get the bounds for this filter
    unsigned lbound, ubound;
    simMetric->getFilterBounds(query,
			       simThreshold,
			       filter,
			       lbound,
			       ubound);
    
    vector<KeyNodePair<InvList> >& children = node->children;
    // nodeLbound and nodeUbound denote upper and lower bound of node that we are looking at
    unsigned nodeLbound, nodeUbound;    
    nodeLbound = filter->getFilterLbound();
    // for all children check if we need to recurse into them
    for(unsigned i = 0; i < children.size(); i++) {
      // selectively recurse into children, 
      // i.e. recurse into all nodes that overlap with [lbound, ubound]
      nodeUbound = children.at(i).key;
      if(lbound <= nodeUbound && ubound >= nodeLbound) 
	getFilterTreeLeaves(ftIndexer,
			    children.at(i).node,
			    query,
			    filterTypes,
			    filterId+1,
			    leaves);
	
      nodeLbound = nodeUbound;
    }
  }
}
  
template<class FtIndexerDiscardListsConcrete>
unsigned 
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
initScanCountArray(const vector<vector<unsigned>*> &arrays,
		   const unsigned threshold,
		   const unsigned arraySize,
		   unsigned scanCountArray[]) {
  unsigned resultSize = 0;
  for(unsigned i=0;i<arrays.size();i++) {    
    vector<unsigned> *v = arrays.at(i);    
    for(unsigned j=0;j<v->size();j++) {
      unsigned id = v->at(j);           
      scanCountArray[id]++;      
      if (scanCountArray[id] == threshold ) resultSize++;      
    }    
  }    
  return resultSize;  
}
  
template<class FtIndexerDiscardListsConcrete>
void 
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
fillQueryInfo(const FtIndexerMem<>& sampleIndexer,
	      const vector<string>& queriesSampleSet, 
	      const unsigned dictSize,
	      QueryInfo queryInfo[], 
	      unsigned& totalMergeElements,
	      unsigned& totalCandidates,
	      unsigned& totalPanicCandidates) {

  totalMergeElements = 0;
  totalCandidates = 0;
  totalPanicCandidates = 0;

  for(unsigned i = 0; i < queriesSampleSet.size(); i++) {   
    string currentString = queriesSampleSet.at(i);

    // fill gram occurences
    this->gramGen->decompose(currentString, queryInfo[i].gramCodes);  
    for(unsigned g = 0; g < queryInfo[i].gramCodes.size(); g++) {
      unsigned gramCode = queryInfo[i].gramCodes.at(g);
      unordered_map<unsigned, unsigned>* map = &queryInfo[i].gramOccur;
      if(map->find(gramCode) == map->end()) (*map)[gramCode] = 1;       
      else ((*map)[gramCode])++;
    }
  
    queryInfo[i].length = currentString.size();
    queryInfo[i].holes = 0;     
    queryInfo[i].originalThreshold = simMetric->getMergeThreshold(currentString, simThreshold, (unsigned)0);
    getFilterTreeLeaves(sampleIndexer, 
			sampleIndexer.filterTreeRoot, 
			currentString, 
			this->filterTypes, 
			0,  
			queryInfo[i].affectedLeaves);    

    unsigned nLeaves = queryInfo[i].affectedLeaves.size();
    
    // init array of nonZeroScanCounts
    queryInfo[i].nonZeroScanCounts = new vector<unsigned*>[nLeaves];

    queryInfo[i].panicCost = 0;    
    queryInfo[i].isPanic = false;
    queryInfo[i].originalCandidates = 0;    
    for(unsigned j = 0; j < nLeaves; j++) {
      FilterTreeNode<vector<unsigned> >* leaf = queryInfo[i].affectedLeaves.at(j);

      unsigned* newScanCount = new unsigned[dictSize];
      memset(newScanCount, 0, dictSize * sizeof(unsigned));
      queryInfo[i].scanCounts.push_back(newScanCount);
      vector<vector<unsigned>* > invertedLists;
      leaf->getInvertedLists(currentString, *(this->gramGen), invertedLists);

      // fill panic cost
      if(leaf->distinctStringIDs)
	queryInfo[i].panicCost += leaf->distinctStringIDs->getArray()->size();               
           
      // if the query panics set a flag
      if((signed)queryInfo[i].originalThreshold <= 0) {
	queryInfo[i].isPanic = true;
	if(leaf->distinctStringIDs)
	  totalPanicCandidates += leaf->distinctStringIDs->getArray()->size();
      }
      else {
	// get other costs, i.e. merge and postprocess
	queryInfo[i].originalCandidates += 
	  initScanCountArray(invertedLists, 
			     queryInfo[i].originalThreshold, 
			     queriesSampleSet.size(),
			     queryInfo[i].scanCounts.at(j));     
	
	// fill nonZeroScanCount
	unsigned* arr = queryInfo[i].scanCounts.at(j);
	vector<unsigned*>* nonZeroScanCounts = &queryInfo[i].nonZeroScanCounts[j];
	for(unsigned z = 0; z < dictSize; z++)
	  if(arr[z] > 0) nonZeroScanCounts->push_back( &arr[z] ); 	
      }
      
      for(unsigned k = 0; k < invertedLists.size(); k++)
	totalMergeElements += invertedLists.at(k)->size();      
    } 
    // sanity check
    if(queryInfo[i].originalCandidates > queryInfo[i].panicCost)
      cout << "INSANE " << queryInfo[i].originalCandidates << " " << queryInfo[i].panicCost << endl;
    totalCandidates += queryInfo[i].originalCandidates;   
  }   
}

template<class FtIndexerDiscardListsConcrete>
void 
FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsConcrete>::
freeQueryInfo(QueryInfo queryInfo[], const unsigned size) {
  for(unsigned i = 0; i < size; i++) {
    unsigned scanCountSize = queryInfo[i].scanCounts.size();
    for(unsigned j = 0; j < scanCountSize; j++) {
      unsigned* scanCountArr = queryInfo[i].scanCounts.at(j);
      if(scanCountArr) delete[] scanCountArr;
    }
    vector<unsigned*>* nonZeroScanCounts = queryInfo[i].nonZeroScanCounts;
    if(nonZeroScanCounts) delete[] nonZeroScanCounts;
  }    
}

template<class StringContainer, class InvList>
class FtIndexerDiscardListsTimeCost;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerDiscardListsTimeCost<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerDiscardListsTimeCost 
  : public FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsTimeCost<StringContainer, InvList> > {

private:
  typedef 
  FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsTimeCost<StringContainer, InvList> > 
  SuperClass;

protected:  
  typedef typename SuperClass::GramMap GramMap;  

  float avgPostprocessTime;
  float avgMergeTime;
 
  void estimateAvgPostprocessTime(const vector<string>& dictSampleSet,
				  const vector<string>& queriesSampleSet,
				  const QueryInfo queryInfo[],
				  const unsigned iterations);

  void estimateAvgMergeTime(const vector<string>& dictSampleSet,
			    const vector<string>& queriesSampleSet,
			    const QueryInfo queryInfo[],
			    const unsigned iterations);

  inline float getApproxPostprocessTime(const unsigned totalCandidates) { return avgPostprocessTime * (float)totalCandidates; }
  inline float getApproxMergeTime(const unsigned totalElements) { return avgMergeTime * (float)totalElements; }
  
  unsigned chooseGram(set<ValPair, ValPairCmpFunc>& orderedGramSet,
		      const vector<string>& queriesSampleSet,
		      const vector<string>& dictSampleSet,
		      GramMap& queriesMap,
		      QueryInfo queryInfo[],
		      unsigned& totalMergeElements,
		      unsigned& totalCandidates,
		      unsigned& totalPanicCandidates,
		      unsigned long& memsum,
		      float& bestRatio);  

  unsigned calculateNewCandidates(const string& query,
				  const unsigned threshold,
				  const unsigned holeOccur,
				  const unsigned arraySize,
				  const vector<unsigned*>& nonZeroScanCounts,
				  const vector<unsigned>* affectedStrings,
				  const bool makePermanent,
				  unsigned scanCountArray[]);
  
  void simulateHole(const bool makePermanent,
		    const unsigned hole,
		    const unsigned dictSize,
		    const vector<string>& queriesSampleSet,
		    GramMap& queriesMap,	     
		    QueryInfo queryInfo[],
		    unsigned& oldTotalMergeElements,
		    unsigned& oldTotalCandidates,
		    unsigned& oldTotalPanicCandidates,
		    unsigned& newTotalMergeElements, 
		    unsigned& newTotalCandidates,
		    unsigned& newTotalPanicCandidates);  

public:
  FtIndexerDiscardListsTimeCost(StringContainer* strContainer) : SuperClass(strContainer) {} ;
  FtIndexerDiscardListsTimeCost(StringContainer* strContainer,
			       GramGen* gramGen, 
			       const float reductionRatio, 
			       const vector<string>* workload,
			       const SimMetric* simMetric,
			       const float simThreshold,
			       const bool ratioCost = false,
			       const float dictSamplingFrac = 0.01f,
			       const float queriesSamplingFrac = 0.25f,
			       const unsigned maxStrLen = 150, 
			       const unsigned ftFanout = 50)
    : SuperClass(strContainer,
		 gramGen, 
		 reductionRatio, 
		 workload, 
		 simMetric, 
		 simThreshold, 
		 ratioCost, 
		 dictSamplingFrac, 
		 queriesSamplingFrac,		 
		 maxStrLen, 
		 ftFanout),
      avgPostprocessTime(0.0f), avgMergeTime(0.0f) {
    this->indexerType = HOLES_GLOBAL_TIMECOST;
  }
  
  void chooseHoleGrams_Cost();  

  string getName() { 
    return "FtIndexerDiscardListsTimeCost"; 
  }
};

template<class StringContainer, class InvList>
void 
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
estimateAvgPostprocessTime(const vector<string>& dictSampleSet,
			   const vector<string>& queriesSampleSet,
			   const QueryInfo queryInfo[],
			   const unsigned iterations) {
  StatsUtil sutil;  
  
  srand(150);
  
  float totalTime = 0.0;  
  unsigned counter = 0;
  unsigned rep = 2;

  // for some reason this estimation is sometimes much better than the one below...  
  //  for(unsigned z = 0; z < iterations; z++) { 
  //  unsigned q = rand() % queriesSampleSet.size();
  //  string queryString = queriesSampleSet.at(q);
  //  sutil.startTimeMeasurement();
  //  distanceMeasure->similar(queryString, queryString);
  //  sutil.stopTimeMeasurement();    
  //  totalTime += sutil.getTimeMeasurement(TFMSEC);
  //  counter++;   
  //  }  
  
  for(unsigned i = 0; i < rep; i++) {
    for(unsigned z = 0; z < queriesSampleSet.size(); z++) { 
      unsigned q = z;
      for(unsigned j = 0; j < queryInfo[q].affectedLeaves.size(); j++) {
	FilterTreeNode<>* leaf = queryInfo[q].affectedLeaves.at(j);
	if(!leaf->distinctStringIDs) continue;
	InvList* distinctIDs = leaf->distinctStringIDs->getArray();
	for(unsigned k = 0; k < distinctIDs->size(); k++) {
	  string queryString = queriesSampleSet.at(q);
	  string dictString = dictSampleSet.at(distinctIDs->at(k));
	  
	  sutil.startTimeMeasurement();
	  (*(this->simMetric))(dictString, queryString);
	  sutil.stopTimeMeasurement();    
	  totalTime += sutil.getTimeMeasurement(TFMSEC);
	  counter++;	
	}
      }           
    }  
  }

  avgPostprocessTime = totalTime / (float)counter;
}
  
template<class StringContainer, class InvList>
void 
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
estimateAvgMergeTime(const vector<string>& dictSampleSet,
		     const vector<string>& queriesSampleSet,
		     const QueryInfo queryInfo[],
		     const unsigned iterations) {
  StatsUtil sutil;

  srand(150);

  float totalTime = 0.0;
  float totalElements = 0.0;

  DivideSkipMerger<InvList> merger(false);

  unsigned rep = 2;

  for(unsigned i = 0; i < rep; i++) {
    for(unsigned z = 0; z < queriesSampleSet.size(); z++) {
      unsigned q = z;
      string query = queriesSampleSet.at(q);
      for(unsigned j = 0; j < queryInfo[q].affectedLeaves.size(); j++) {    

	FilterTreeNode<InvList>* leaf = queryInfo[q].affectedLeaves.at(j);  
	vector<InvList*> invertedLists;
	leaf->getInvertedLists(query, *(this->gramGen), invertedLists);

	if(invertedLists.size() <= 0) continue;
	
	vector<unsigned> candidates;
	sutil.startTimeMeasurement();
	merger.merge(invertedLists, queryInfo[q].originalThreshold, candidates);
	sutil.stopTimeMeasurement();    
	totalTime += sutil.getTimeMeasurement(TFMSEC);
	
	for(unsigned k = 0; k < invertedLists.size(); k++) 
	  totalElements += invertedLists.at(k)->size();           
      }
    }
  }

  if(totalElements > 0)
    avgMergeTime = totalTime / totalElements;
  else {
    //cout << "WARNING: forced estimated mergetime to zero! something is wrong. check ftixerholesglobal.cc" << endl;
    // just set some default value
    avgMergeTime = 1.0e-35;
  }
}
  
template<class StringContainer, class InvList>
unsigned 
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
chooseGram(set<ValPair, ValPairCmpFunc>& orderedGramSet,
	   const vector<string>& queriesSampleSet,
	   const vector<string>& dictSampleSet,
	   GramMap& queriesMap,
	   QueryInfo queryInfo[],
	   unsigned& totalMergeElements,
	   unsigned& totalCandidates,
	   unsigned& totalPanicCandidates,
	   unsigned long& memsum,
	   float& bestRatio) {

  float currentRatio = 0;
  bestRatio = 0.0;
  pair<unsigned, unsigned> bestGram;
  bestGram.first = 0;
  bestGram.second = 0;
  set<ValPair, ValPairCmpFunc>::iterator remove_best;
    
  for(set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
      iter != orderedGramSet.end();
      iter++) {
    
    unsigned gramHole = iter->gramCode;
    
    // simulate the hole and estimate new number of candidates and number of elements to merge
    unsigned newTotalMergeElements = totalMergeElements;
    unsigned newTotalCandidates = totalCandidates;
    unsigned newTotalPanicCandidates = totalPanicCandidates;
    simulateHole(false,
		 gramHole,  
		 dictSampleSet.size(),
		 queriesSampleSet,
		 queriesMap,	     
		 queryInfo,
		 totalMergeElements,
		 totalCandidates,
		 totalPanicCandidates,
		 newTotalMergeElements, 
		 newTotalCandidates,
		 newTotalPanicCandidates);   

    float approxMergeTime = getApproxMergeTime(newTotalMergeElements);      
    float approxPostprocessTime = getApproxPostprocessTime(newTotalCandidates);
    float approxPanicTime =  getApproxPostprocessTime(newTotalPanicCandidates);
    float approxTotalTime = approxMergeTime + approxPostprocessTime + approxPanicTime;
    if(this->ratioCost) currentRatio = (float)iter->listSize / (float)approxTotalTime;
    else currentRatio = 1.0 / (float)approxTotalTime;

    if(currentRatio > bestRatio) {
      bestGram.first = iter->gramCode;
      bestGram.second = iter->listSize;
      bestRatio = currentRatio;
      remove_best = iter;     
    }
  }

  memsum += bestGram.second;
  orderedGramSet.erase(remove_best);
   
  return bestGram.first;
}
  
template<class StringContainer, class InvList>
unsigned 
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
calculateNewCandidates(const string& query,
		       const unsigned threshold,
		       const unsigned holeOccur,
		       const unsigned arraySize,
		       const vector<unsigned*>& nonZeroScanCounts,
		       const vector<unsigned>* affectedStrings,
		       const bool makePermanent,
		       unsigned scanCountArray[]) {
  if(affectedStrings != NULL) {
    for(unsigned i = 0; i < affectedStrings->size(); i++) {
      unsigned id = affectedStrings->at(i);
      scanCountArray[id] -= holeOccur; 
      //if( (signed)scanCountArray[id] < 0) scanCountArray[id] = 0;
    }
  } 

  // use non-zero scan count vector to calculate new candidates
  unsigned resultSize = 0; 
  unsigned end = nonZeroScanCounts.size();
  for(unsigned i = 0; i < end; i++)
    if( *(nonZeroScanCounts.at(i)) >= threshold && (signed)*(nonZeroScanCounts.at(i)) > 0) resultSize++;  

  // roll back
  if(!makePermanent) {
    if(affectedStrings != NULL) {
      for(unsigned i = 0; i < affectedStrings->size(); i++) {
	unsigned id = affectedStrings->at(i);      
	scanCountArray[id] += holeOccur;
      }
    }
  } 

  return resultSize; 
}

template<class StringContainer, class InvList>
void
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
simulateHole(const bool makePermanent,
	     const unsigned hole,
	     const unsigned dictSize,
	     const vector<string>& queriesSampleSet,
	     GramMap& queriesMap,	     
	     QueryInfo queryInfo[],
	     unsigned& oldTotalMergeElements,
	     unsigned& oldTotalCandidates,
	     unsigned& oldTotalPanicCandidates,
	     unsigned& newTotalMergeElements, 
	     unsigned& newTotalCandidates,
	     unsigned& newTotalPanicCandidates) {
  newTotalMergeElements = oldTotalMergeElements;
  newTotalCandidates = oldTotalCandidates;  
  newTotalPanicCandidates = oldTotalPanicCandidates;

  if(queriesMap.find(hole) != queriesMap.end()) {	
    GramList<>* gramList = queriesMap[hole];
    vector<unsigned>* changedQueries = gramList->getArray();
    for(unsigned i = 0; i < changedQueries->size(); i++) {
      unsigned changedQueryId = changedQueries->at(i);
      string query = queriesSampleSet.at(changedQueryId);
      unsigned holes = queryInfo[changedQueryId].holes + queryInfo[changedQueryId].gramOccur[hole];
      
      vector<FilterTreeNode<>* >* affectedLeaves = &queryInfo[changedQueryId].affectedLeaves;

      // calculate T
      signed T = (signed)this->simMetric->getMergeThreshold(query, this->simThreshold, holes);
      // check for panic
      unsigned currentCandidates = 0;
      if(T <= 0) {
	if(!queryInfo[changedQueryId].isPanic) {
	  newTotalPanicCandidates += queryInfo[changedQueryId].panicCost;	
	  if(makePermanent) queryInfo[changedQueryId].isPanic = true;
	}
      }
      else {
	for(unsigned j = 0; j < affectedLeaves->size(); j++) {
	  GramMap& gramMap = affectedLeaves->at(j)->gramMap;	 	  	
	  vector<unsigned>* invertedList = NULL;
	  if(gramMap.find(hole) != gramMap.end()) invertedList = gramMap[hole]->getArray();
	  currentCandidates += 
	    calculateNewCandidates(query, 
				   T,
				   queryInfo[changedQueryId].gramOccur[hole],
				   dictSize,
				   queryInfo[changedQueryId].nonZeroScanCounts[j],
				   invertedList, 
				   makePermanent,
				   queryInfo[changedQueryId].scanCounts.at(j));	  
	}      		

	for(unsigned j = 0; j < affectedLeaves->size(); j++) {
	  GramMap& gramMap = affectedLeaves->at(j)->gramMap;
	  if(gramMap.find(hole) != gramMap.end())
	    newTotalMergeElements -= gramMap[hole]->getArray()->size();
	}      		
      }

      // sanity check
      if(currentCandidates > queryInfo[changedQueryId].panicCost)
	cout << "INSANE " << currentCandidates << " " << queryInfo[changedQueryId].panicCost << endl;      
      
      newTotalCandidates -= queryInfo[changedQueryId].originalCandidates;
      newTotalCandidates += currentCandidates;      
	
      if(makePermanent) {
	queryInfo[changedQueryId].originalCandidates = currentCandidates;	
	//queryInfo[changedQueryId].holes++;
	queryInfo[changedQueryId].holes += queryInfo[changedQueryId].gramOccur[hole];
      }
    }

    if(makePermanent) {
      oldTotalMergeElements = newTotalMergeElements;
      oldTotalCandidates = newTotalCandidates;  
      oldTotalPanicCandidates = newTotalPanicCandidates;
    }
  }
}

template<class StringContainer, class InvList>
void
FtIndexerDiscardListsTimeCost<StringContainer, InvList>::
chooseHoleGrams_Cost() {  
  vector<string> dictSampleSet;
  vector<string> queriesSampleSet;
  this->fillSampleSets(false, dictSampleSet, queriesSampleSet);
  unsigned queriesSampleSize = queriesSampleSet.size();
    
  set<ValPair, ValPairCmpFunc> orderedGramSet;
  unordered_map<unsigned, unsigned> countMap;
  unsigned long totalListElements = 0;  

  this->fillStructsForHoles(orderedGramSet, countMap, &totalListElements);
    
  // ATTENTION: heuristic for speedup
  // cleanse orderedGramSet of insignificant grams, e.g. grams with listsize 1  
  bool memCanBeSatisfied = this->cleanseOrderedGramSet(totalListElements, countMap.size(), orderedGramSet);
  if(!memCanBeSatisfied) {
    for(set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
	iter != orderedGramSet.end();
	iter++) {      
      this->holeGrams.insert(iter->gramCode);
    }
    cout << "MEMORY REQUIREMENT CANNOT BE SATISFIED BY HOLES" << endl;
    return;
  }
    
  // create index to run the queries on using global holes indexer
  // create a temp string collection
  StringContainerVector* tmpDictContainer = new StringContainerVector();
  tmpDictContainer->fillContainer(dictSampleSet.begin(), dictSampleSet.end());
  FtIndexerMem<> sampleIndexer(tmpDictContainer, this->gramGen, this->maxStrLen, this->ftFanout);
  for(unsigned i = 0; i < this->filterTypes.size(); i++)
    sampleIndexer.addPartFilter(this->filterTypes.at(i)->clone());
  sampleIndexer.buildIndex();
  delete tmpDictContainer;

  // create index of queries
  // create a temp string collection
  StringContainerVector* tmpQueriesContainer = new StringContainerVector();
  tmpQueriesContainer->fillContainer(queriesSampleSet.begin(), queriesSampleSet.end());
  FtIndexerMem<> queriesIndexer(tmpQueriesContainer, this->gramGen, this->maxStrLen, this->ftFanout);
  queriesIndexer.buildIndex();
  delete tmpQueriesContainer;
  GramMap& queriesMap = queriesIndexer.filterTreeRoot->gramMap;
  
  // pre-process queries and store information into queryInfo
  unsigned totalMergeElements = 0;
  unsigned totalCandidates = 0; 
  unsigned totalPanicCandidates = 0;
  QueryInfo* queryInfo = new QueryInfo[queriesSampleSize];
  this->fillQueryInfo(sampleIndexer, 
		      queriesSampleSet, 
		      dictSampleSet.size(),
		      queryInfo, 
		      totalMergeElements, 
		      totalCandidates,
		      totalPanicCandidates);
  
  estimateAvgPostprocessTime(dictSampleSet, queriesSampleSet, queryInfo, this->workload->size());
  estimateAvgMergeTime(dictSampleSet, queriesSampleSet, queryInfo, this->workload->size());
      
  unsigned long memmax = (unsigned long)(totalListElements * this->reductionRatio);
  unsigned long memsum = 0;    

  while(memsum < memmax) {       
    unsigned chosenGram = 0;
    float bestRatio = 0.0;
      
    // choose gram
    chosenGram = chooseGram(orderedGramSet,
			    queriesSampleSet,
			    dictSampleSet,
			    queriesMap,
			    queryInfo,
			    totalMergeElements,
			    totalCandidates,					  
			    totalPanicCandidates,
			    memsum,
			    bestRatio);     
      
    // make change permanent
    unsigned tmpTotalMergeElements = totalMergeElements;
    unsigned tmpTotalCandidates = totalCandidates;
    unsigned tmpTotalPanicCandidates = totalPanicCandidates;
    simulateHole(true,
		 chosenGram,  
		 dictSampleSet.size(),
		 queriesSampleSet,
		 queriesMap,	     
		 queryInfo,
		 totalMergeElements,
		 totalCandidates,
		 totalPanicCandidates,
		 tmpTotalMergeElements, 
		 tmpTotalCandidates,
		 tmpTotalPanicCandidates);

    // insert gram into blacklist
    this->holeGrams.insert(chosenGram);
  }    
  
  // free QueryInfo memory
  this->freeQueryInfo(queryInfo, queriesSampleSet.size());
  delete[] queryInfo;
  
#ifdef DEBUG_STAT
  dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->numberHoles = this->holeGrams.size();
#endif
}

template<class StringContainer, class InvList>
class FtIndexerDiscardListsPanicCost;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerDiscardListsPanicCost<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerDiscardListsPanicCost 
  : public FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsPanicCost<StringContainer, InvList> > {

private:
  typedef 
  FtIndexerDiscardListsCostAbs<FtIndexerDiscardListsPanicCost<StringContainer, InvList> > 
  SuperClass;

protected:  
  typedef typename SuperClass::GramMap GramMap;  

  void simulateHole(unsigned hole,
		    vector<string>& queriesSampleSet,
		    GramMap& queriesMap,	     
		    QueryInfo queryInfo[],
		    unsigned& oldPanics,
		    unsigned& newPanics,
		    bool makePermanent);   
 
public:
  FtIndexerDiscardListsPanicCost(StringContainer* strContainer) : SuperClass(strContainer) {} ;
  FtIndexerDiscardListsPanicCost(StringContainer* strContainer,
				GramGen* gramGen, 
				const float reductionRatio, 
				const vector<string>* workload,
				const SimMetric* simMetric,
				const float simThreshold,
				const bool ratioCost = false,
				const float dictSamplingFrac = 0.01f,
				const float queriesSamplingFrac = 0.25f,
				const unsigned maxStrLen = 150, 
				const unsigned ftFanout = 50)
    : SuperClass(strContainer,
		 gramGen, 
		 reductionRatio, 
		 workload, 
		 simMetric, 
		 simThreshold, 
		 ratioCost, 
		 dictSamplingFrac, 
		 queriesSamplingFrac,		 
		 maxStrLen, 
		 ftFanout) {
    
    this->indexerType = HOLES_GLOBAL_PANICCOST;    
  }

  string getName() { 
    return "FtIndexerDiscardListsPanicCost";
  }

  void chooseHoleGrams_Cost();
};

template<class StringContainer, class InvList>
void
FtIndexerDiscardListsPanicCost<StringContainer, InvList>::
simulateHole(unsigned hole,
	     vector<string>& queriesSampleSet,
	     GramMap& queriesMap,	     
	     QueryInfo queryInfo[],
	     unsigned& oldPanics,
	     unsigned& newPanics,
	     bool makePermanent) {
  
  if(queriesMap.find(hole) != queriesMap.end()) {	
    GramList<>* gramList = queriesMap[hole];
    vector<unsigned>* changedQueries = gramList->getArray();
    for(unsigned i = 0; i < changedQueries->size(); i++) {
      unsigned changedQueryId = changedQueries->at(i);
      string query = queriesSampleSet.at(changedQueryId);
      unsigned holes = queryInfo[changedQueryId].holes + queryInfo[changedQueryId].gramOccur[hole];      

      // calculate T      
      signed T = (signed)this->simMetric->getMergeThreshold(query, this->simThreshold, holes);
      // check for panic
      if(T <= 0) {
	if(!queryInfo[changedQueryId].isPanic) {
	  newPanics += queryInfo[changedQueryId].panicCost;
	  if(makePermanent) queryInfo[changedQueryId].isPanic = true;
	}
      }

      if(makePermanent) queryInfo[changedQueryId].holes += queryInfo[changedQueryId].gramOccur[hole];
    }
      
    if(makePermanent)
      oldPanics = newPanics;
  }
}

template<class StringContainer, class InvList>
void
FtIndexerDiscardListsPanicCost<StringContainer, InvList>::
chooseHoleGrams_Cost() {
  vector<string> dictSampleSet;
  vector<string> queriesSampleSet;
  this->fillSampleSets(false, dictSampleSet, queriesSampleSet);
  unsigned queriesSampleSize = queriesSampleSet.size();
     
  set<ValPair, ValPairCmpFunc> orderedGramSet;
  unordered_map<unsigned, unsigned> countMap;
  unsigned long totalListElements = 0;  
  
  this->fillStructsForHoles(orderedGramSet, countMap, &totalListElements);
     
  // ATTENTION: heuristic for speedup
  // cleanse orderedGramSet of insignificant grams, e.g. grams with listsize 1  
  bool memCanBeSatisfied = this->cleanseOrderedGramSet(totalListElements, countMap.size(), orderedGramSet);
  if(!memCanBeSatisfied) {
    for(set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
	iter != orderedGramSet.end();
	iter++) {      
      this->holeGrams.insert(iter->gramCode);
    }
    cout << "MEMORY REQUIREMENT CANNOT BE SATISFIED BY HOLES" << endl;
    return;
  }  
  
  // create index to run the queries on using global holes indexer
  // create a temp string collection
  StringContainerVector* tmpDictContainer = new StringContainerVector(false);
  tmpDictContainer->fillContainer(dictSampleSet.begin(), dictSampleSet.end());
  FtIndexerMem<> sampleIndexer(tmpDictContainer, this->gramGen, this->maxStrLen, this->ftFanout);
  for(unsigned i = 0; i < this->filterTypes.size(); i++)
    sampleIndexer.addPartFilter(this->filterTypes.at(i)->clone());
  sampleIndexer.buildIndex();
  delete tmpDictContainer;

  // create index of queries
  // create a temp string collection
  StringContainerVector* tmpQueriesContainer = new StringContainerVector(false);
  tmpQueriesContainer->fillContainer(queriesSampleSet.begin(), queriesSampleSet.end());
  FtIndexerMem<> queriesIndexer(tmpQueriesContainer, this->gramGen, this->maxStrLen, this->ftFanout);
  queriesIndexer.buildIndex();
  delete tmpQueriesContainer;
  GramMap& queriesMap = queriesIndexer.filterTreeRoot->gramMap;
          
  // pre-process queryinfo
  unsigned totalMergeElements = 0;
  unsigned totalCandidates = 0;  
  unsigned totalPanicCandidates = 0;
  QueryInfo* queryInfo = new QueryInfo[queriesSampleSize];
  this->fillQueryInfo(sampleIndexer, 
		      queriesSampleSet, 
		      dictSampleSet.size(),
		      queryInfo, 
		      totalMergeElements, 
		      totalCandidates,
		      totalPanicCandidates);
     
  unsigned totalPanics = totalPanicCandidates;
     
  unsigned long memmax = (unsigned long)(totalListElements * this->reductionRatio);
  unsigned long memsum = 0;
     
  while(memsum < memmax) {
    float currentRatio = 0;
    float bestRatio = 0;
    pair<unsigned, unsigned> bestGram;
    bestGram.first = 0;
    bestGram.second = 0;
    set<ValPair, ValPairCmpFunc>::iterator remove;
       
    for(set<ValPair, ValPairCmpFunc>::iterator iter = orderedGramSet.begin();
	iter != orderedGramSet.end();
	iter++) {
	 
      // simulate the hole and estimate new number of candidates and number of elements to merge
      unsigned newTotalPanics = totalPanics;
      simulateHole(iter->gramCode,
		   queriesSampleSet,
		   queriesMap,	     
		   queryInfo,
		   totalPanics,
		   newTotalPanics,
		   false);
	 
      if(newTotalPanics <= 0) newTotalPanics = 1; 
      if(this->ratioCost) currentRatio = (float)iter->listSize / (float)newTotalPanics;
      else currentRatio = 1.0 / (float)newTotalPanics;
	 
      if(currentRatio > bestRatio) {
	bestGram.first = iter->gramCode;
	bestGram.second = iter->listSize;
	bestRatio = currentRatio;
	remove = iter;
      }
    }
       
    memsum += bestGram.second;
    orderedGramSet.erase(remove);
    this->holeGrams.insert(bestGram.first);
       
    // permanently record the hole for all affected queries
    unsigned newTotalPanics = totalPanics;
    simulateHole(bestGram.first,
		 queriesSampleSet,
		 queriesMap,	     
		 queryInfo,
		 totalPanics,
		 newTotalPanics,
		 true);
       
    //cout << "CHOSEN " << memsum << " " << memmax << " " << bestGram.first << " " << bestGram.second << " " << totalPanics << endl;
  }
     
  // free QueryInfo memory
  this->freeQueryInfo(queryInfo, queriesSampleSet.size());
  delete[] queryInfo;
  
#ifdef DEBUG_STAT
  dynamic_cast<HolesGlobalIndexStats*>(this->ixStats)->numberHoles = this->holeGrams.size();
#endif    
}   

#endif
