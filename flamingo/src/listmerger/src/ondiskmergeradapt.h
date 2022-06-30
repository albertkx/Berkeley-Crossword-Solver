/*
  $Id: ondiskmergeradapt.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ondiskmergeradapt_h_
#define _ondiskmergeradapt_h_

#include "ondiskmergerabs.h"
#include "filtertree/src/gramlistondisk.h"
#include "filtertree/src/statsutil.h"
#include "filtertree/src/ftindexerondisk.h"
#include "util/src/misc.h"

// forward declarations, overview of classes and template specializations
template<class InvList> class WeightedGramList;         // duplicate inverted lists are grouped into one weighted gramlist, abbrev. wgl
template<class InvList> class Candidate;                // information about a candidate answer, abbrev. candi or c
template<class InvList> class WglDesc;                  // weighted gram list descriptor, adds leafID to a weightedgramlist, abbrev. wgld
template<class InvList> class WglDescSet;               // set of wglds, abbrev. wgldSet
template<class InvList> class HeapMergeElement;         // type of element used on heap for mergeShortLists
template<class InvList> class LeafContext;              // info regarding the processing of one leaf, abbrev. lc
template<class InvList> class OnDiskMergerAdapt;        // main merger class

template<class InvList = vector<unsigned> >
class WeightedGramList {
public:
  unsigned gramCode;                  // hashed q-gram
  unsigned weight;                    // how often the corresponding gram appears in the query
  GramListOnDisk<InvList>* gramList;  // pointer to the inverted list on disk
};

template<class InvList = vector<unsigned> >
class Candidate {
public:
  unsigned id;                    
  unsigned weight;
  Candidate(unsigned id, unsigned weight) : id(id), weight(weight) {}
};

template<class InvList>
class WglDesc {
public:
  WeightedGramList<InvList>* wgl;
  unsigned leafID;  
  WglDesc(unsigned i, WeightedGramList<InvList>* l) : wgl(l), leafID(i) {}
};

template<class InvList>
class WglDescSet {
public:
  vector<WglDesc<InvList>* > wglds;
  unsigned totalListSize;               // sum of listSizes of all wgld in wglds
  WglDescSet() : totalListSize(0) {}
  
  static bool cmpBySize(const WglDescSet* a, const WglDescSet* b) { return a->totalListSize < b->totalListSize; }
  
  ~WglDescSet() { for(unsigned i = 0; i < wglds.size(); i++) delete wglds[i]; }
};

template<class InvList>
class LeafContext {
public:
  unsigned threshold;                        // merging threshold
  fstream* invListsFile;                     // pointer to file of inverted lists
  vector<WeightedGramList<InvList>* >* wgls; // wgls ordered by size (globally ordered for seqMerge and locally ordered for regMerge)
  unsigned initialLists;                     // minimum number of lists to read to guarantee completeness of candidate set
  unsigned initialWeight;                    // total weight of initial lists
  unsigned remainingLists;                   // lists that we have not read yet
  unsigned remainingWeight;                  // total weight of remaining lists
  unsigned currentList;                      // index in wgls for which list to read next
  unsigned* candidateCounts;                 // total number of candidates that have a particular weight needed
  vector<Candidate<InvList> > candidates;    // current candidate set
  
  // helpers for cost model
  unsigned listCounter;                      // number of lists to be read from this leaf
  unsigned weightCounter;                    // total weight of listCounter lists
  unsigned weightedTotalListSize;            // total list size of listCounter lists, multiplied by list weights
  unsigned cumulCandiCount;                  // number of candidates potentially pruned by reading listCounter lists
  
  // sortLeafLists indicates whether wgls should be sorted by leaf size, false for seqMerge, true for regMerge
  LeafContext(unsigned threshold, fstream& invListsFile, vector<WeightedGramList<InvList>* >* wgls, bool sortLeafLists);
  
  // set initialLists to a specifit value, used in seqMerge only
  void setInitialLists(unsigned initialLists);
  
  static bool cmpWglBySize(const WeightedGramList<InvList>* a, const WeightedGramList<InvList>* b) {    
    return a->gramList->listSize < b->gramList->listSize;
  }
  
  ~LeafContext() { if(candidateCounts) delete[] candidateCounts; }
};

template <class InvList>
LeafContext<InvList>::
LeafContext(unsigned threshold, fstream& invListsFile, vector<WeightedGramList<InvList>* >* wgls, bool sortLeafLists)
  : threshold(threshold), invListsFile(&invListsFile), wgls(wgls) {
  
  // init helpers for cost model
  weightCounter = 0;
  weightedTotalListSize = 0;
  listCounter = 0;
  cumulCandiCount = 0;
  
  // determine initialWeight
  unsigned totalWeight = 0;
  for(unsigned i = 0; i < wgls->size(); i++) 
    totalWeight += wgls->at(i)->weight;
  initialWeight = totalWeight - threshold + 1;
  
  // determine the initial number of lists to read to reach initialWeight
  if(sortLeafLists) sort(wgls->begin(), wgls->end(), LeafContext<InvList>::cmpWglBySize);
  initialLists = 0;
  remainingWeight = 0;
  unsigned weightSum = 0;
  for(unsigned i = 0; i < wgls->size(); i++) {
    unsigned currentWeight = wgls->at(i)->weight;
    weightSum += currentWeight;
    if(weightSum >= initialWeight) {
      if(initialLists == 0) initialLists = i + 1;
      else remainingWeight += currentWeight;
    }
  }	
  
  currentList = initialLists;
  remainingLists = wgls->size() - initialLists;
  candidateCounts = new unsigned[remainingWeight+1];
  memset(candidateCounts, 0, sizeof(unsigned) * (remainingWeight+1));    
}

template <class InvList>
void
LeafContext<InvList>::
setInitialLists(unsigned initialLists) {      
  if(initialLists <= this->initialLists) return;
  
  this->initialLists = initialLists;
  currentList = initialLists;
  remainingLists = wgls->size() - initialLists;        
  
  // recompute initialWeight and remainingWeight
  initialWeight = 0;
  unsigned totalWeight = 0;
  for(unsigned i = 0; i < wgls->size(); i++) {
    if(i < initialLists)
      initialWeight += wgls->at(i)->weight;
    totalWeight += wgls->at(i)->weight;
  }
  remainingWeight = totalWeight - initialWeight;
  
  if(candidateCounts) delete[] candidateCounts;
  candidateCounts = new unsigned[remainingWeight+1];
  memset(candidateCounts, 0, sizeof(unsigned) * (remainingWeight+1));
}

// main merger class
template <class InvList = vector<unsigned> >
class OnDiskMergerAdapt : public OnDiskMergerAbs<OnDiskMergerAdapt<InvList>, InvList> {
  
private:
  typedef OnDiskMergerAbs<OnDiskMergerAdapt<InvList>, InvList> SuperClass;

  // parameters for cost model
  bool estimationParamsSet;
  float readInvListTimeSlope;
  float readInvListTimeIntercept;
  float readStringAvgTime;
  
  // exponential probe, returns iterator to first element greater or equal to value
  inline typename InvList::iterator expProbe(typename InvList::iterator start, typename InvList::iterator end, unsigned value) const;
  
  // entry point for merging by reading sublists of one gram sequentially
  void seqMerge(const Query& query,
		vector<WeightedGramList<InvList>* >* allWgls,
		unsigned nLeaves,
		unsigned threshold,
		fstream& invListsFile,
		vector<unsigned>& results);

  // entry point for merging by processing leafs one-by-one (not reading sublists of one gram sequentially)
  void regMerge(const Query& query,
		vector<WeightedGramList<InvList>* >* allWgls,
		unsigned nLeaves,
		unsigned threshold,
		fstream& invListsFile,
		vector<unsigned>& results);
  
  // create wgls from qgls, setting weights of wgls according to how often the same list appears in qgls
  void detectDuplicateLists(vector<QueryGramList<InvList>* >& qgls, vector<WeightedGramList<InvList>* >& wgls) const;

  // read inverted lists, call mergeShortLists to create initial candidate set, cleanup inverted lists
  void mergeInitialLists(const Query& query,
			 LeafContext<InvList>& leafContext, 
			 vector<unsigned>& results);
  
  // merge initial lists to obtain initial candidate set
  void mergeShortLists(const Query& query, LeafContext<InvList>& leafContext, vector<unsigned>& results) const;
  
  // estimate benefit of reading next lc.weightCounter lists
  float getEstimatedBenefit(LeafContext<InvList>& lc, unsigned numberStrings) const;
  
  // read next listsToRead lists and process them to remove candidates
  void readNextLists(const Query& query, 
		     LeafContext<InvList>& leafContext, 
		     unsigned listsToRead,
		     vector<unsigned>& results); 
  
  // add all candidates ids in lc->candidates to results
  void addRemainingCandidates(const Query& query,
			      LeafContext<InvList>* lc, 
			      vector<unsigned>& results) const;
  
  // read initial lists of all leaves from disk, all sublists belonging to the same gram are read sequentially
  unsigned seqReadInitialLists(vector<LeafContext<InvList>* >& leafContexts, 
			       const vector<WglDescSet<InvList>*>& ordWgldSets,
			       fstream& invListsFile) const;
  
  // for seqMerge the lists in all leaves should be globally (!) ordered according to the total size of all sublists belonging to one gram
  // this function reorders the wgls and produces a set of globally ordered gram-lists (ordWgldSets) so we can keep track of which (global) gramlist to read next
  void reorderWgls(vector<WeightedGramList<InvList>*>* ordWgls, 
		   vector<WglDescSet<InvList>*>& ordWgldSets,
		   const vector<WeightedGramList<InvList>*>* allWgls,
		   unsigned nLeaves) const;  

  // only for indexes with positional information, update candidate set from one PosID on an inverted list
  void processPosID(const Query& query, const string& prePostQueryStr, unsigned gramCode, unsigned candiID, 
		    vector<unsigned char>& wglPos, unsigned char candiPos, bool* positionsMatched, Candidate<InvList>* c) const;
  
public:  
  // singleFilterOpt indicates whether we read lists belonging to the same gram sequentially
  OnDiskMergerAdapt(bool singleFilterOpt = true) 
    : SuperClass(singleFilterOpt), estimationParamsSet(false),
      readInvListTimeSlope(0.0f), readInvListTimeIntercept(0.0f), readStringAvgTime(0.0f) {}
  
  // main entry point called from searcher
  void merge_Impl(const Query& query,
		  vector<vector<QueryGramList<InvList>* >* >& allQgls,
		  const unsigned threshold,
		  fstream& invListsFile,
		  unsigned numberFilters,
		  vector<unsigned>& results);
  
  // record how many strings are in each partition (leaf), used in cost model for determining the probability of pruning candidates
  vector<unsigned> numberStringsInLeaf;
  
  // for setting cost model parameters
  bool estimationParamsNeeded_Impl() { return !estimationParamsSet; }
  void setEstimationParams_Impl(float readInvListTimeSlope, float readInvListTimeIntercept, float readStringAvgTime) {
    this->readInvListTimeSlope = readInvListTimeSlope;
    this->readInvListTimeIntercept = readInvListTimeIntercept;
    this->readStringAvgTime = readStringAvgTime;
    estimationParamsSet = true;
  }
  
  // comparison function for sorting
  static bool cmpQglByGramCode(const QueryGramList<InvList>* a, const QueryGramList<InvList>* b) {
    return a->gramCode < b->gramCode;
  }
  
  string getName() {
    return "OnDiskMergerAdapt";
  }
};

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
addRemainingCandidates(const Query& query, LeafContext<InvList>* lc, vector<unsigned>& results) const {
  for(unsigned x = 0; x < lc->candidates.size(); x++) {
    results.push_back(lc->candidates[x].id);
  }
}

template<class InvList>
typename InvList::iterator
OnDiskMergerAdapt<InvList>::
expProbe(typename InvList::iterator start, 
	 typename InvList::iterator end, 
	 unsigned value) const {
  unsigned c = 0;    
  for(;;) {      
    typename InvList::iterator iter = start + (1 << c);
    if(iter >= end) return end;
    else if(*iter >= value) return iter;
    c++;
  }
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
mergeShortLists(const Query& query, LeafContext<InvList>& lc, vector<unsigned>& results) const {
  
  for(unsigned i = 0; i < lc.initialLists; i++) {
    vector<Candidate<InvList> > tmp;
    typename vector<Candidate<InvList> >::const_iterator candiIter = lc.candidates.begin();

    unsigned weight = lc.wgls->at(i)->weight;
    typename InvList::iterator invListIter = lc.wgls->at(i)->gramList->getArray()->begin();
    typename InvList::iterator invListEnd = lc.wgls->at(i)->gramList->getArray()->end();
    
    while(candiIter != lc.candidates.end() || invListIter != invListEnd) {
      if(candiIter == lc.candidates.end() || (invListIter != invListEnd && candiIter->id > *invListIter)) {
	tmp.push_back(Candidate<InvList>(*invListIter, weight));
	invListIter++;
      } else if (invListIter == invListEnd || (candiIter != lc.candidates.end() && candiIter->id < *invListIter)) {
	tmp.push_back(Candidate<InvList>(candiIter->id, candiIter->weight));
	candiIter++;
      } else {
	tmp.push_back(Candidate<InvList>(candiIter->id, candiIter->weight + weight));
	candiIter++;
	invListIter++;
      }
    }
    std::swap(lc.candidates, tmp);
  }
  
  vector<Candidate<InvList> > tmp;
  for(unsigned i = 0; i < lc.candidates.size(); i++) {
    unsigned id = lc.candidates[i].id;
    if(lc.candidates[i].weight >= lc.threshold) results.push_back(id);
    else {
      unsigned weightNeeded = lc.remainingWeight - (lc.threshold - lc.candidates[i].weight) + 1;
      if((signed)weightNeeded > 0) {       
	// check post-merge filter
	bool pmfPruned = false;
	switch(this->pmf) {
	case PMF_CSF_REG:
	case PMF_CSF_OPT: {
	  pmfPruned = !this->charsumFilter->passesFilter(this->queryCharsumStats, 
							 &this->charsumStats[id], 
							 (unsigned)query.threshold);
	  
	} break;
	  
	case PMF_LC: {
	  pmfPruned = !letterCountFilter(this->queryLcStats, 
					 &this->dataLcStats[id], 
					 this->lcCharNum,
					 query.threshold);
	  
	} break;
	  
	case PMF_HC: {
	  pmfPruned = !hashCountFilter(this->queryHcStats, 
				       &this->dataHcStats[id], 
				       this->hcBuckets,
				       query.threshold);
	  
	} break;
	  
	default: break;
	  
	}
	
	if(!pmfPruned) {
	  tmp.push_back(Candidate<InvList>(id, lc.candidates[i].weight));
	  lc.candidateCounts[weightNeeded]++;
	}
      }
    }
  }
  std::swap(lc.candidates, tmp);
}

template<class InvList>
float
OnDiskMergerAdapt<InvList>::
getEstimatedBenefit(LeafContext<InvList>& lc, unsigned numberStrings) const {
  
  float avgListSize = (float)lc.weightedTotalListSize / (float)(lc.weightCounter);
  float psuccess = 1.0f - ( avgListSize / numberStrings );
  
  // calculate the benefit of reading the next weightCounter lists
  float benefitReadLists = 0.0f;
  for(unsigned j = 1; j <= lc.weightCounter; j++) {    
    if(lc.candidateCounts[j] > 0) {
      float p = binomialDistrib(j, lc.weightCounter, psuccess, true);
      benefitReadLists += lc.candidateCounts[j] * readStringAvgTime * p;
    }
  } 
  return benefitReadLists;
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
readNextLists(const Query& query, 
	      LeafContext<InvList>& lc, 
	      unsigned listsToRead, 
	      vector<unsigned>& results) {  
  
  for(unsigned i = 0; i < listsToRead; i++) {
    vector<Candidate<InvList> > tmp;
    WeightedGramList<InvList>* wgl = lc.wgls->at(lc.currentList+i);
    GramListOnDisk<InvList>* gl = wgl->gramList;
    
#ifdef DEBUG_STAT
    this->invListData += gl->listSize * sizeof(typename InvList::value_type);
#endif
    
    InvList* array = gl->getArray(lc.invListsFile);
    unsigned oldRemainingWeight = lc.remainingWeight;
    lc.remainingWeight -= wgl->weight;
    
    // do lookup for every candidate
    typename InvList::iterator start = array->begin();
    
    for(unsigned j = 0; j < lc.candidates.size(); j++) {     
      Candidate<InvList>& candidate = lc.candidates[j];
      unsigned oldWeightNeeded = oldRemainingWeight - (lc.threshold - candidate.weight) + 1;      
      if((signed)oldWeightNeeded <= 0) continue;

      typename InvList::iterator end = expProbe(start, array->end(), candidate.id);
      start = lower_bound(start, end, candidate.id);
      if(start != array->end() && *start == candidate.id) {
	unsigned newWeight = candidate.weight + wgl->weight;
	if(newWeight >= lc.threshold) {
	  lc.candidateCounts[oldWeightNeeded]--;
	  results.push_back(candidate.id);
	}
	else {
	  tmp.push_back(Candidate<InvList>(candidate.id, newWeight));
	}
      }
      else {
	lc.candidateCounts[oldWeightNeeded]--;
	unsigned newWeightNeeded = (oldWeightNeeded > wgl->weight) ? oldWeightNeeded - wgl->weight : 0;
	if(newWeightNeeded <= lc.remainingWeight) {
	  lc.candidateCounts[newWeightNeeded]++;
	  tmp.push_back(Candidate<InvList>(candidate.id, candidate.weight));
	}
      }
    }
    
    std::swap(lc.candidates, tmp); 
    gl->clear();
  }
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
mergeInitialLists(const Query& query, 
		  LeafContext<InvList>& leafContext, 
		  vector<unsigned>& results) {
  
  // read short lists from disk (or retrieve from cache if already read)
  for(unsigned i = 0; i < leafContext.initialLists; i++)
    leafContext.wgls->at(i)->gramList->getArray(leafContext.invListsFile);
  
  mergeShortLists(query, leafContext, results);
  
  // clean up lists
  for(unsigned i = 0; i < leafContext.initialLists; i++)
    leafContext.wgls->at(i)->gramList->clear();
  
#ifdef DEBUG_STAT
    this->initialCandidates += leafContext.candidates.size();    
    for(unsigned i = 0; i < leafContext.initialLists; i++)
      this->invListData += leafContext.wgls->at(i)->gramList->listSize * sizeof(typename InvList::value_type);
#endif
}

template<class InvList>
void  
OnDiskMergerAdapt<InvList>::
detectDuplicateLists(vector<QueryGramList<InvList>* >& qgls, vector<WeightedGramList<InvList>* >& wgls) const {
  
  sort(qgls.begin(), qgls.end(), OnDiskMergerAdapt<InvList>::cmpQglByGramCode);
  
  unsigned i = 0;
  while(i < qgls.size()) {    
    QueryGramList<InvList>* currentQGL = qgls[i];
    unsigned currentCount = 0;    
    while(i < qgls.size()) {
      if(currentQGL->gramCode == qgls[i]->gramCode) {
	currentCount++;
	i++;
      }
      else break;      
    }
    WeightedGramList<InvList>* wgl = new WeightedGramList<InvList>();
    wgl->gramCode = currentQGL->gramCode;
    wgl->gramList = dynamic_cast<GramListOnDisk<InvList>*>(currentQGL->gl);
    wgl->weight = currentCount;
    wgls.push_back(wgl);
  }
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
reorderWgls(vector<WeightedGramList<InvList>*>* ordWgls, 
	    vector<WglDescSet<InvList>*>& ordWgldSets,
	    const vector<WeightedGramList<InvList>*>* allWgls,
	    unsigned nLeaves) const {
  
  tr1::unordered_map<unsigned, WglDescSet<InvList>* > groupedWgls;
  // fill the groupedWgls
  for(unsigned i = 0; i < nLeaves; i++) {    
    const vector<WeightedGramList<InvList>* >& currentLists = allWgls[i];
    for(unsigned j = 0; j < currentLists.size(); j++) {	
      WeightedGramList<InvList>* wgl = currentLists[j];	
      WglDescSet<InvList>* wgldSet = NULL;
      if(groupedWgls.find(wgl->gramCode) == groupedWgls.end()) {
	wgldSet =  new WglDescSet<InvList>();
	groupedWgls[wgl->gramCode] = wgldSet;
	ordWgldSets.push_back(wgldSet);
      }
      else wgldSet = groupedWgls[wgl->gramCode];
      
      wgldSet->wglds.push_back( new WglDesc<InvList>(i, wgl) );
      wgldSet->totalListSize += wgl->gramList->listSize;
    }
  }
  
  // sort the vector of WglDescSet by total-sublist-size
  sort(ordWgldSets.begin(), ordWgldSets.end(), WglDescSet<InvList>::cmpBySize);
  
  // reorder the gramlists for each leaf to be consistent with the global ordering (by the sum of the sublist sizes)
  for(unsigned i = 0; i < ordWgldSets.size(); i++) {
    WglDescSet<InvList>* wgldSet = ordWgldSets[i];
    vector<WglDesc<InvList>* >& wglds = wgldSet->wglds;
    for(unsigned j = 0; j < wglds.size(); j++) {
      ordWgls[wglds[j]->leafID].push_back(wglds[j]->wgl);
    }
  }  
}

template<class InvList>
unsigned
OnDiskMergerAdapt<InvList>::
seqReadInitialLists(vector<LeafContext<InvList>* >& leafContexts, 
		    const vector<WglDescSet<InvList>*>& ordWgldSets,
		    fstream& invListsFile) const {
  
  // we must keep reading all sorted sublists belonging to a gram
  // as long as there exist one leaf for which we have not read initialLists number of lists
  bool done = false;
  unsigned globalCurrentList = 0;
  unsigned nLeaves = leafContexts.size();
  while(!done) {
    WglDescSet<InvList>* wgldSet = ordWgldSets[globalCurrentList];
    vector<WglDesc<InvList>*>& wglds = wgldSet->wglds;
    
    // seek to start offset of first sorted sublist
    WglDesc<InvList>* firstWgld = *(wglds.begin());
    GramListOnDisk<InvList>* firstGl = firstWgld->wgl->gramList;
    invListsFile.seekg( firstGl->startOffset );
    
    // now fill sorted sublists in order in one sequential I/O
    // fillArray is implemented NOT to perform a disk seek
    typename vector<WglDesc<InvList>* >::iterator setiter;
    for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
      WglDesc<InvList>* wgld = *setiter;
      GramListOnDisk<InvList>* gl = wgld->wgl->gramList;
      // sanity check, in some cases this condition could be true, it is NOT an error
      if(invListsFile.tellg() != gl->startOffset) invListsFile.seekg(gl->startOffset);	
      gl->fillArray(&invListsFile);
      leafContexts[wgld->leafID]->listCounter++;
    }
    
    // check whether we need to read more lists
    unsigned doneCount = 0;
    for(unsigned i = 0; i < nLeaves; i++) if(leafContexts[i]->listCounter >= leafContexts[i]->initialLists) doneCount++;
    
    done = doneCount >= nLeaves;
    globalCurrentList++;
  }        
  
  return globalCurrentList;
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
seqMerge(const Query& query,
	 vector<WeightedGramList<InvList>* >* allWgls,
	 unsigned nLeaves,
	 unsigned threshold,
	 fstream& invListsFile,
	 vector<unsigned>& results) { 
  
  // reorder the inverted lists according to a global ordering, fill ordWgldSets and ordWgls
  vector<WeightedGramList<InvList>* > ordWgls[nLeaves];
  vector<WglDescSet<InvList>*> ordWgldSets;
  reorderWgls(ordWgls, ordWgldSets, allWgls, nLeaves);
  
  // initialize the context for all leaves
  vector<LeafContext<InvList>* > leafContexts;
  for(unsigned i = 0; i < nLeaves; i++) {
    LeafContext<InvList>* lc = new LeafContext<InvList>(threshold, invListsFile, &ordWgls[i], false);
    leafContexts.push_back(lc);
  }
  
  // read initial lists for all leaves, read sublists belonging to one gram sequentially
  unsigned globalCurrentList = seqReadInitialLists(leafContexts, ordWgldSets, invListsFile);
  
#ifdef DEBUG_STAT
  this->invListSeeks = globalCurrentList;
#endif  
  
  // perform initial merging for all leaves
  for(unsigned i = 0; i < nLeaves; i++) {
    LeafContext<InvList>* lc = leafContexts[i];
    lc->setInitialLists(lc->listCounter);
    mergeInitialLists(query, *lc, results);
  }
    
  // start cost-based selection of additional lists to read
  unsigned listsToRead = 1; // number of lists we decide to read in every iteration    
  unsigned lastListCounter = ordWgldSets.size();
  unsigned lastListCounterRepeats = 0;
  unsigned lastCandidatesLeft = 0;
  float lastBenefitReadLists = 0.0f;
  while(listsToRead && globalCurrentList < ordWgldSets.size() ) {
    listsToRead = 0;      
        
    // reset cost estimation helpers in all leafContexts
    for(unsigned i = 0; i < nLeaves; i++) {
      leafContexts[i]->listCounter = 0;
      leafContexts[i]->weightCounter = 0;
      leafContexts[i]->weightedTotalListSize = 0;
      leafContexts[i]->cumulCandiCount = 0;
    }
    
    // estimate benefit and cost for reading any number of remaining lists
    unsigned totalListSize = 0;
    unsigned listCounter = 0;
    for(unsigned i = globalCurrentList; i < ordWgldSets.size(); i++) {
      listCounter++;
      
      WglDescSet<InvList>* wgldSet = ordWgldSets[i];
      vector<WglDesc<InvList>*>& wglds = wgldSet->wglds;
      
      // update cost estimation helpers
      typename vector<WglDesc<InvList>*>::iterator setiter;
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	unsigned leafID = wgld->leafID;
	LeafContext<InvList>* lc = leafContexts[leafID];
	WeightedGramList<InvList>* wgl = lc->wgls->at(lc->currentList + lc->listCounter);
	unsigned newWeightCounter = lc->weightCounter + wgl->weight;
	for(unsigned j = lc->weightCounter+1; j <= newWeightCounter; j++)
	  lc->cumulCandiCount += lc->candidateCounts[j];
	lc->weightCounter = newWeightCounter;
	lc->weightedTotalListSize += wgl->gramList->listSize * wgl->weight;
	lc->listCounter++;	  
      }	
      
      // determine benefit of reading lists and cost of not reading lists
      float benefitReadLists = 0.0f;
      float costNoReadLists = 0.0f;
      unsigned candidatesLeft = 0;
      unsigned cumulCandiCount = 0;
      for(unsigned j = 0; j < nLeaves; j++) {
	candidatesLeft += leafContexts[j]->candidates.size();
	cumulCandiCount += leafContexts[j]->cumulCandiCount;
	if(leafContexts[j]->weightCounter > 0)
	  benefitReadLists += getEstimatedBenefit(*leafContexts[j], numberStringsInLeaf[j]);
	if(leafContexts[j]->remainingWeight > 0 && leafContexts[j]->candidates.size() > 0)
	  costNoReadLists += leafContexts[j]->cumulCandiCount * readStringAvgTime;
      }
      
      totalListSize += wgldSet->totalListSize;
      float costReadLists = (listCounter * readInvListTimeIntercept + totalListSize * readInvListTimeSlope) - benefitReadLists;
      
      // heuristic to quit benefit/cost estimation because benefit of reading more lists is extremely unlikely
      if(cumulCandiCount == candidatesLeft && lastBenefitReadLists == benefitReadLists) break;
      else lastBenefitReadLists = benefitReadLists;
      
      // check for NaN, this could happen due to lack in precision in float
      if(costReadLists != costReadLists) break;
      
      if(costReadLists < costNoReadLists) {
	
	// first heuristic to terminate reading lists
	if(candidatesLeft <= nLeaves) break;
	
	// second heuristic to terminate reading lists  	  	  
	if(lastCandidatesLeft == candidatesLeft) {
	  lastListCounterRepeats++;
	  if(lastListCounterRepeats > lastListCounter) break;
	}
	else {
	  lastCandidatesLeft = candidatesLeft;
	  lastListCounter = listCounter;
	  lastListCounterRepeats = 0;
	} 
	
	listsToRead = 1;
	break;
      }	      
    }
    
    // read the next lists using sequential IOs
    for(unsigned i = 0; i < listsToRead; i++) {
      WglDescSet<InvList>* wgldSet = ordWgldSets[globalCurrentList+i];
      vector<WglDesc<InvList>*>& wglds = wgldSet->wglds;
      
      // now fill sorted sublists in order in one sequential I/O
      // fillArray is implemented NOT to perform a disk seek
      typename vector<WglDesc<InvList>*>::iterator setiter;
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	if(leafContexts[wgld->leafID]->candidates.size() > 0) {
	  GramListOnDisk<InvList>* gl = wgld->wgl->gramList;
	  // handle first seek and sanity check in this manner
	  if(invListsFile.tellg() != gl->startOffset) invListsFile.seekg(gl->startOffset);
	  gl->fillArray(&invListsFile);
	}
      }
      
      // now process the read sublists
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	if(leafContexts[wgld->leafID]->candidates.size() > 0) {
	  LeafContext<InvList>* lc = leafContexts[wgld->leafID];
	  readNextLists(query, *lc, 1, results);	    
	  lc->currentList++;	    
	}
      }
    }
    
    // if listsToRead is 0, it means we cannot get a benefit by reading any number of next lists
    globalCurrentList += listsToRead;
    
#ifdef DEBUG_STAT
    this->invListSeeks += listsToRead;
#endif
  }
  
  // add remaining candidates to result set and delete candidate instances
  for(unsigned i = 0; i < nLeaves; i++)
    addRemainingCandidates(query, leafContexts[i], results);
  
  // cleanup
  for(unsigned i = 0; i < ordWgldSets.size(); i++) delete ordWgldSets[i];          
  for(unsigned i = 0; i < leafContexts.size(); i++) delete leafContexts[i];  
}
  

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
regMerge(const Query& query,
	 vector<WeightedGramList<InvList>* >* allWgls,
	 unsigned nLeaves,
	 unsigned threshold,
	 fstream& invListsFile,
	 vector<unsigned>& results) {
  
  // search every leaf node
  for(unsigned i = 0; i < nLeaves; i++) {
      
    // initialize the context for this leaf    
    LeafContext<InvList> lc(threshold, invListsFile, &allWgls[i], true);
    
    // merge the initial short lists
    mergeInitialLists(query, lc, results);
            
    //cout << "INITIAL CANDIDATES FOR LEAF: " << i << " " << lc.candidates.size() << endl;

#ifdef DEBUG_STAT
    this->invListSeeks += lc.initialLists;
#endif
    
    unsigned lastListCounter = lc.wgls->size();
    unsigned lastListCounterRepeats = 0;
    unsigned lastCandidatesLeft = 0;
    float lastBenefitReadLists = 0.0f;    
    
    unsigned listsToRead = 1; // number of lists we decide to read in every iteration
    while(listsToRead && lc.currentList < lc.wgls->size()) {
      /*
      cout << "CANDIDATES FOR CURRENTLIST: " << lc.currentList << " " << lc.candidates.size() << endl;
      for(unsigned y = 0; y < lc.candidates.size(); y++) {
	cout << lc.candidates.at(y)->id << " " << lc.candidates.at(y)->weight << endl;
      }
      */

      listsToRead = 0;
	
      // for every possible number of remaining lists determine the benefits and cost
      unsigned totalListSize = 0;
      unsigned listCounter = 0;
      
      for(unsigned x = lc.currentList; x < lc.wgls->size(); x++) {	
	listCounter++;
	  
	WeightedGramList<InvList>* wgl = lc.wgls->at(x);
	unsigned newWeightCounter = lc.weightCounter + wgl->weight;
	for(unsigned j = lc.weightCounter+1; j <= newWeightCounter; j++)
	  lc.cumulCandiCount += lc.candidateCounts[j];
	lc.weightCounter = newWeightCounter;
	lc.weightedTotalListSize += wgl->gramList->listSize * wgl->weight;  
	totalListSize += wgl->gramList->listSize;
	  
	float benefitReadLists = getEstimatedBenefit(lc, numberStringsInLeaf[i]);
	float costNoReadLists = lc.cumulCandiCount * readStringAvgTime;
	float costReadLists = (listCounter * readInvListTimeIntercept + lc.weightedTotalListSize * readInvListTimeSlope) - benefitReadLists;
	
	// heuristic to quit benefit/cost estimation because benefit of reading more lists is extremely unlikely
	if(lc.cumulCandiCount == lc.candidates.size() && lastBenefitReadLists == benefitReadLists) break;
	else lastBenefitReadLists = benefitReadLists;
	
	// check for NaN, this could happen due to lack in precision in float
	if(costReadLists != costReadLists) break;
	
	if(costReadLists < costNoReadLists) {
	    
	  // first heuristic to terminate reading lists
	  if(lc.candidates.size() <= 1) break;

	  // second heuristic to terminate reading lists  	  	  
	  if(lastCandidatesLeft == lc.candidates.size()) {
	    lastListCounterRepeats++;
	    if(lastListCounterRepeats > lastListCounter) break;
	  }
	  else {
	    lastCandidatesLeft = lc.candidates.size();
	    lastListCounter = listCounter;
	    lastListCounterRepeats = 0;
	  } 
	  
	  listsToRead = 1;
	  break;
	}
      }
      
      if(listsToRead) readNextLists(query, lc, listsToRead, results);
	
      // if listsToRead is 0, it means we cannot get a benefit by reading any number of next lists
      lc.currentList += listsToRead; 
      
#ifdef DEBUG_STAT
      this->invListSeeks += listsToRead;
#endif	
      //cout << "----------------------------------------------" << endl;
    }                 
      
    // add remaining candidates to result set and clear candidate information
    addRemainingCandidates(query, &lc, results);
  }    
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
merge_Impl(const Query& query,
	   vector<vector<QueryGramList<InvList>* >* >& allQgls,
	   unsigned threshold,
	   fstream& invListsFile,
	   unsigned numberFilters,
	   vector<unsigned>& results) {    
  
  unsigned nLeaves = allQgls.size();
  
  // eliminate duplicate inverted lists, and assign weights to lists indicating how many its gram appears in the query
  vector<WeightedGramList<InvList>* > allWgls[nLeaves];
  for(unsigned i = 0; i < nLeaves; i++) detectDuplicateLists(*allQgls[i], allWgls[i]);
  
  // check for single filter optimization, reading lists sequentially
  if(numberFilters == 1 && this->singleFilterOpt && allQgls.size() > 1) {
    seqMerge(query, allWgls, nLeaves, threshold, invListsFile, results);
  }
  else { // leaves are processed ony-by-one, lists of the same gram are NOT read sequantially (if there is partitioning)
    regMerge(query, allWgls, nLeaves, threshold, invListsFile, results);    
  }
  
  // clean up weighted gramlists
  for(unsigned i = 0; i < nLeaves; i++)
    for(unsigned j = 0; j < allWgls[i].size(); j++)
      delete allWgls[i][j];
}

#endif
