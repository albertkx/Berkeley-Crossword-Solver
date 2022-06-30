/*
  $Id: ondiskmergeradapt.h 5721 2010-09-09 05:43:08Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ondiskmergeradapt_h_
#define _ondiskmergeradapt_h_

#include "ondiskmergerabs.h"
#include "divideskipmerger.h"
#include "filtertree/src/gramlist.h"
#include "filtertree/src/gramlistondisk.h"
#include "filtertree/src/statsutil.h"
#include "filtertree/src/ftindexerondisk.h"
#include "util/src/misc.h"

#include <fstream>
#include <tr1/unordered_map>
#include <set>
#include <algorithm>

#define WILDCHAR 91

#define POSFILTER_NONE           0
#define POSFILTER_MISMATCH       1
#define POSFILTER_DP             (1<<1)
#define POSFILTER_ENDDP          (1<<2)
#define POSFILTER_SUBSTR         (1<<3)
#define POSFILTER_LENGTH         (1<<4)

// forward declarations, overview of classes and template specializations
template<class InvList> class WeightedGramList;         // duplicate inverted lists are grouped into one weighted gramlist, abbrev. wgl
template<class InvList> class Candidate;                // information about a candidate answer, abbrev. candi or c
template<class InvList> class WglDesc;                  // weighted gram list descriptor, adds leafID to a weightedgramlist, abbrev. wgld
template<class InvList> class WglDescSet;               // set of wglds, abbrev. wgldSet
template<class InvList> class HeapMergeElement;         // type of element used on heap for heapMerge
template<class InvList> class LeafContext;              // info regarding the processing of one leaf, abbrev. lc
template<class InvList> class OnDiskMergerAdapt;        // main merger class

// template specializations for index with positional information
template<> class WeightedGramList<Array<PosID> >;
template<> class Candidate<Array<PosID> >;

template<class InvList = Array<unsigned> >
class WeightedGramList {
public:
  unsigned gramCode;                  // hashed q-gram
  unsigned weight;                    // how often the corresponding gram appears in the query
  GramListOnDisk<InvList>* gramList;  // pointer to the inverted list on disk
};

template<class InvList = Array<unsigned> >
class Candidate {
public:
  unsigned id;                        // string id
  unsigned weight;                    // number of occurrences on all lists (we remove duplicate lists, so we actually accummulate weights)
  unsigned weightNeeded;              // number of lists (weight) that candidate must be absent from to be pruned
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
  
  ~WglDescSet() { for(unsigned i = 0; i < wglds.size(); i++) delete wglds.at(i); }
};

template<class InvList>
class HeapMergeElement {
public:
  typename InvList::elementType* element;
  unsigned listIndex;
  
  HeapMergeElement(typename InvList::elementType* el, unsigned li) 
    : element(el), listIndex(li) {}
  
  bool operator==(const HeapMergeElement& e) const {
    return *element == *(e.element);
  }    
  
  bool operator<(const HeapMergeElement& e) const {
    if(*element == *(e.element)) return listIndex < e.listIndex;
    return *element < *(e.element);
  }
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
  vector<Candidate<InvList>* > candidates;   // current candidate set
  
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
template <class InvList = Array<unsigned> >
class OnDiskMergerAdapt : public OnDiskMergerAbs<OnDiskMergerAdapt<InvList>, InvList> {
  
private:
  typedef OnDiskMergerAbs<OnDiskMergerAdapt<InvList>, InvList> SuperClass;

  // parameters for cost model
  bool estimationParamsSet;
  float readInvListTimeSlope;
  float readInvListTimeIntercept;
  float readStringAvgTime;
  
  // indicates which positional filters are to be used, see defines at the beginning of this file
  char posFilters;  
  
  // exponential probe, returns iterator to first element greater or equal to value
  inline typename InvList::iterator expProbe(typename InvList::iterator start, typename InvList::iterator end, unsigned value) const;
    
  // remove candidates from candidate set with weightNeeded == 0
  void cleanseCandidates(vector<Candidate<InvList>* >& candidates) const;
  
  // length filter, returns POSFILTER_LENGTH if pruned or POSFILTER_NONE if not pruned
  char checkLength(const Query& query, const string& prePostQueryStr, const Candidate<InvList>& candidate) const;  
  
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

  // read inverted lists, call heapMerge to create initial candidate set, cleanup inverted lists
  void mergeInitialLists(const Query& query, 
			 const string& prePostQueryStr,
			 LeafContext<InvList>& leafContext, 
			 vector<unsigned>& results);
  
  // merge initial lists to obtain initial candidate set
  void heapMerge(const Query& query, const string& prePostQueryStr, LeafContext<InvList>& leafContext, vector<unsigned>& results) const;
  
  // estimate benefit of reading next lc.weightCounter lists
  float getEstimatedBenefit(LeafContext<InvList>& lc, unsigned numberStrings) const;
  
  // read next listsToRead lists and process them to remove candidates
  void readNextLists(const Query& query, 
		     const string& prePostQueryStr, 
		     LeafContext<InvList>& leafContext, 
		     unsigned listsToRead,
		     vector<unsigned>& results); 
  
  // add all candidates ids in lc->candidates to results
  void addRemainingCandidates(const Query& query, 
			      const string& prePostQueryStr, 
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
  OnDiskMergerAdapt(bool singleFilterOpt = true, char posFilters = POSFILTER_NONE) 
    : SuperClass(singleFilterOpt), estimationParamsSet(false), posFilters(posFilters) {}
  
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

  // comparison function for sorting  
  static bool cmpCandidateByWeightNeeded(const Candidate<InvList>* a, const Candidate<InvList>* b) {
    if(a->weightNeeded == 0) return false;
    if(b->weightNeeded == 0) return true;
    return a->id < b->id;
  } 

  string getName() {
    return "OnDiskMergerAdapt";
  }
};

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
addRemainingCandidates(const Query& query, const string& prePostQueryStr, LeafContext<InvList>* lc, vector<unsigned>& results) const {
  for(unsigned x = 0; x < lc->candidates.size(); x++) {
    results.push_back(lc->candidates.at(x)->id);   
    delete lc->candidates.at(x);
  }
}

template<class InvList>
char
OnDiskMergerAdapt<InvList>::
checkLength(const Query& query, const string& prePostQueryStr, const Candidate<InvList>& candidate) const {  
  signed l1 = prePostQueryStr.size();
  signed l2 = candidate.partialStr.size();
  if( abs(l1 - l2) > (signed)query.threshold ) return POSFILTER_LENGTH;
  else return POSFILTER_NONE;
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
cleanseCandidates(vector<Candidate<InvList>* >& candidates) const {
  sort(candidates.begin(), candidates.end(), OnDiskMergerAdapt<InvList>::cmpCandidateByWeightNeeded);  
  while(candidates.size() > 0 && candidates.back()->weightNeeded == 0) {
    delete candidates.back();
    candidates.pop_back();
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
heapMerge(const Query& query, const string& prePostQueryStr, LeafContext<InvList>& lc, vector<unsigned>& results) const {
  
  unsigned pointersIndex[lc.initialLists];  
  memset(pointersIndex, 0, lc.initialLists * sizeof(unsigned));
  
  Heap<HeapMergeElement<InvList> > heap(lc.initialLists);
  
  // make initial heap
  for(unsigned i = 0; i < lc.initialLists; i++) {
    unsigned& e = lc.wgls->at(i)->gramList->getArray()->at(0);
    heap.push(new HeapMergeElement<InvList>(&e, i));
  }
  
  HeapMergeElement<InvList>* poppedElements[lc.initialLists];
  while( !heap.empty() ) {                       
    // Container of vector indexes which should be moved to the next position 
    unsigned poppedElementCount = 0;
    HeapMergeElement<InvList>* minData = heap.head();
    
    Candidate<InvList>* c = new Candidate<InvList>();
    c->id = *(minData->element);
    c->weight = 0;
    
    // pop elements with same id and accumulate weight in candidate
    while( *(minData) == *(heap.head()) && poppedElementCount < lc.initialLists) {         
      poppedElements[poppedElementCount++] = heap.head();
      c->weight += lc.wgls->at(heap.head()->listIndex)->weight;
      heap.pop();
      if(heap.empty()) break;
    }        
    
    // check post-perme filter
    bool pmfPruned = false;
    switch(this->pmf) {
    case PMF_CSF_REG:
    case PMF_CSF_OPT: {
      pmfPruned = !this->charsumFilter->passesFilter(this->queryCharsumStats, 
						     &this->charsumStats[c->id], 
						     (unsigned)query.threshold);

    } break;
      
    case PMF_LC: {
      pmfPruned = !letterCountFilter(this->queryLcStats, 
				     &this->dataLcStats[c->id], 
				     this->lcCharNum,
				     query.threshold);
      
    } break;

    case PMF_HC: {
      pmfPruned = !hashCountFilter(this->queryHcStats, 
				   &this->dataHcStats[c->id], 
				   this->hcBuckets,
				   query.threshold);
      
    } break;

    default: break;
      
    }
    
    if(!pmfPruned) {
      // check if candidate-weight exceeds merge threshold
      if(c->weight >= lc.threshold) {
	results.push_back(c->id);
	delete c;	  
      }
      else {
	c->weightNeeded = lc.remainingWeight - (lc.threshold - c->weight) + 1;
	if((signed)c->weightNeeded > 0) {
	  lc.candidateCounts[c->weightNeeded]++;
	  lc.candidates.push_back(c);
	}
	else delete c;
      }
    }
    else delete c;
    
    // move to next element and insert to heaps
    for(unsigned i = 0; i < poppedElementCount; i++) {
      unsigned index = poppedElements[i]->listIndex; 
      pointersIndex[index]++;
      unsigned position = pointersIndex[index]; 	
      if(position < lc.wgls->at(index)->gramList->getArray()->size()) {
	unsigned& e = lc.wgls->at(index)->gramList->getArray()->at(position);
	heap.push(new HeapMergeElement<InvList>(&e, index));	
      }
      delete poppedElements[i];
    }
  }
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
	      const string& prePostQueryStr,
	      LeafContext<InvList>& leafContext, 
	      unsigned listsToRead, 
	      vector<unsigned>& results) {  
  
  for(unsigned i = 0; i < listsToRead; i++) {          
    WeightedGramList<InvList>* wgl = leafContext.wgls->at(leafContext.currentList+i);
    GramListOnDisk<InvList>* gl = wgl->gramList;
    
#ifdef DEBUG_STAT
    this->invListData += gl->listSize * InvList::elementSize();
#endif

    InvList* array = gl->getArray(leafContext.invListsFile);
    leafContext.remainingWeight -= wgl->weight;    
    
    // do lookup for every candidate
    bool cleanseNeeded = false;
    typename InvList::iterator start = array->begin();
    
    for(unsigned j = 0; j < leafContext.candidates.size(); j++) {     
      Candidate<InvList>* candidate = leafContext.candidates.at(j);
      if((signed)candidate->weightNeeded <= 0) continue;      
      
      typename InvList::iterator end = expProbe(start, array->end(), candidate->id);
      start = lower_bound(start, end, candidate->id);
      if(*start == candidate->id) {
	candidate->weight += wgl->weight;
	if(candidate->weight >= leafContext.threshold) {
	  leafContext.candidateCounts[candidate->weightNeeded]--;
	  candidate->weightNeeded = 0;
	  results.push_back(candidate->id);	  
	  cleanseNeeded = true;
	}
      }
      else {	
	leafContext.candidateCounts[candidate->weightNeeded]--;
	candidate->weightNeeded = (candidate->weightNeeded > wgl->weight) ? candidate->weightNeeded - wgl->weight : 0;
	if(candidate->weightNeeded > leafContext.remainingWeight) candidate->weightNeeded = 0;
	if(candidate->weightNeeded > 0) leafContext.candidateCounts[candidate->weightNeeded]++;
	else cleanseNeeded = true;
      }      
    }
    
    // do we need to remove items from the candidate set
    // i.e. we can guarantee that some elements are either answers or cannot be answers
    if(cleanseNeeded) cleanseCandidates(leafContext.candidates);
    
    gl->clear();
  }
}

template<class InvList>
void 
OnDiskMergerAdapt<InvList>::
mergeInitialLists(const Query& query, 
		  const string& prePostQueryStr, 
		  LeafContext<InvList>& leafContext, 
		  vector<unsigned>& results) {
  
  // read short lists from disk (or retrieve from cache if already read)
  for(unsigned i = 0; i < leafContext.initialLists; i++)
    leafContext.wgls->at(i)->gramList->getArray(leafContext.invListsFile);
  
  heapMerge(query, prePostQueryStr, leafContext, results);
  
  // clean up lists
  for(unsigned i = 0; i < leafContext.initialLists; i++)
    leafContext.wgls->at(i)->gramList->clear();
  
#ifdef DEBUG_STAT
    this->initialCandidates += leafContext.candidates.size();    
    for(unsigned i = 0; i < leafContext.initialLists; i++)
      this->invListData += leafContext.wgls->at(i)->gramList->listSize * InvList::elementSize();
#endif
}

template<class InvList>
void  
OnDiskMergerAdapt<InvList>::
detectDuplicateLists(vector<QueryGramList<InvList>* >& qgls, vector<WeightedGramList<InvList>* >& wgls) const {
  
  sort(qgls.begin(), qgls.end(), OnDiskMergerAdapt<InvList>::cmpQglByGramCode);
  
  unsigned i = 0;
  while(i < qgls.size()) {    
    QueryGramList<InvList>* currentQGL = qgls.at(i);
    unsigned currentCount = 0;    
    while(i < qgls.size()) {
      if(currentQGL->gramCode == qgls.at(i)->gramCode) {
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
      WeightedGramList<InvList>* wgl = currentLists.at(j);	
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
    WglDescSet<InvList>* wgldSet = ordWgldSets.at(i);
    vector<WglDesc<InvList>* >& wglds = wgldSet->wglds;
    for(unsigned j = 0; j < wglds.size(); j++) {
      ordWgls[wglds.at(j)->leafID].push_back(wglds.at(j)->wgl);
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
    WglDescSet<InvList>* wgldSet = ordWgldSets.at(globalCurrentList);
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
      leafContexts.at(wgld->leafID)->listCounter++;
    }
    
    // check whether we need to read more lists
    unsigned doneCount = 0;
    for(unsigned i = 0; i < nLeaves; i++) if(leafContexts.at(i)->listCounter >= leafContexts.at(i)->initialLists) doneCount++;
    
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
    
  // only needed for indexer with positional information
  string prePostQueryStr;
  if(posFilters != POSFILTER_NONE) this->gramGen->getPrePostString(query.str, prePostQueryStr);
  
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
    LeafContext<InvList>* lc = leafContexts.at(i);
    lc->setInitialLists(lc->listCounter);
    mergeInitialLists(query, prePostQueryStr, *lc, results);
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
      leafContexts.at(i)->listCounter = 0;
      leafContexts.at(i)->weightCounter = 0;
      leafContexts.at(i)->weightedTotalListSize = 0;
      leafContexts.at(i)->cumulCandiCount = 0;
    }
    
    // estimate benefit and cost for reading any number of remaining lists
    unsigned totalListSize = 0;
    unsigned listCounter = 0;
    for(unsigned i = globalCurrentList; i < ordWgldSets.size(); i++) {
      listCounter++;
      
      WglDescSet<InvList>* wgldSet = ordWgldSets.at(i);
      vector<WglDesc<InvList>*>& wglds = wgldSet->wglds;
      
      // update cost estimation helpers
      typename vector<WglDesc<InvList>*>::iterator setiter;
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	unsigned leafID = wgld->leafID;
	LeafContext<InvList>* lc = leafContexts.at(leafID);
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
	candidatesLeft += leafContexts.at(j)->candidates.size();
	cumulCandiCount += leafContexts.at(j)->cumulCandiCount;
	if(leafContexts.at(j)->weightCounter > 0)
	  benefitReadLists += getEstimatedBenefit(*leafContexts.at(j), numberStringsInLeaf.at(j));
	if(leafContexts.at(j)->remainingWeight > 0 && leafContexts.at(j)->candidates.size() > 0)
	  costNoReadLists += leafContexts.at(j)->cumulCandiCount * readStringAvgTime;
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
      WglDescSet<InvList>* wgldSet = ordWgldSets.at(globalCurrentList+i);
      vector<WglDesc<InvList>*>& wglds = wgldSet->wglds;
      
      // now fill sorted sublists in order in one sequential I/O
      // fillArray is implemented NOT to perform a disk seek
      typename vector<WglDesc<InvList>*>::iterator setiter;
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	if(leafContexts.at(wgld->leafID)->candidates.size() > 0) {
	  GramListOnDisk<InvList>* gl = wgld->wgl->gramList;
	  // handle first seek and sanity check in this manner
	  if(invListsFile.tellg() != gl->startOffset) invListsFile.seekg(gl->startOffset);
	  gl->fillArray(&invListsFile);
	}
      }
      
      // now process the read sublists
      for(setiter = wglds.begin(); setiter != wglds.end(); setiter++) {
	WglDesc<InvList>* wgld = *setiter;
	if(leafContexts.at(wgld->leafID)->candidates.size() > 0) {
	  LeafContext<InvList>* lc = leafContexts.at(wgld->leafID);
	  readNextLists(query, prePostQueryStr, *lc, 1, results);	    
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
    addRemainingCandidates(query, prePostQueryStr, leafContexts.at(i), results);
  
  // cleanup
  for(unsigned i = 0; i < ordWgldSets.size(); i++) delete ordWgldSets.at(i);          
  for(unsigned i = 0; i < leafContexts.size(); i++) delete leafContexts.at(i);  
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
  
  // only needed for indexer with positional information
  string prePostQueryStr;
  if(posFilters != POSFILTER_NONE) this->gramGen->getPrePostString(query.str, prePostQueryStr);
  
  // search every leaf node
  for(unsigned i = 0; i < nLeaves; i++) {
      
    // initialize the context for this leaf    
    LeafContext<InvList> lc(threshold, invListsFile, &allWgls[i], true);
    
    // merge the initial short lists
    mergeInitialLists(query, prePostQueryStr, lc, results);
            
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
	  
	float benefitReadLists = getEstimatedBenefit(lc, numberStringsInLeaf.at(i));
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
      
      if(listsToRead) readNextLists(query, prePostQueryStr, lc, listsToRead, results);
	
      // if listsToRead is 0, it means we cannot get a benefit by reading any number of next lists
      lc.currentList += listsToRead; 
      
#ifdef DEBUG_STAT
      this->invListSeeks += listsToRead;
#endif	
      //cout << "----------------------------------------------" << endl;
    }                 
      
    // add remaining candidates to result set and clear candidate information
    addRemainingCandidates(query, prePostQueryStr, &lc, results);
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
  for(unsigned i = 0; i < nLeaves; i++) detectDuplicateLists(*allQgls.at(i), allWgls[i]);
  
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
      delete allWgls[i].at(j);
}



// specialize the weighted gramlist for the inverted list type, if we have positions we also record them
template<>
class WeightedGramList<Array<PosID> > {
public:
  unsigned gramCode;
  unsigned weight;
  GramListOnDisk<Array<PosID> >* gramList;
  vector<unsigned char> positions;
};

// specialize the candidate for the inverted list type, if we have positions then we also record the positions of mismatching grams
#pragma pack(push,1) 
template<>
class Candidate<Array<PosID> > {
public:
  unsigned id;
  unsigned weight;
  unsigned weightNeeded;
  set<unsigned char> mismatchPositions; // positions of mismatched grams from queries perspective  
  char isPruned; // state of this candidate, can be pruned by various filters, 0 denotes not pruned, !0 denotes pruned
  
  string partialStr;
  
  Candidate(unsigned length, unsigned char wildchar = WILDCHAR) 
    : isPruned(POSFILTER_NONE), partialStr(string(length, wildchar)) { }
  
  void updatePartialString(const string& gram, unsigned char pos) {    
    // make sure there was no collision
    if( gram.size() > 0 ) for(unsigned i = 0; i < gram.size(); i++) partialStr[i+pos] = gram[i];
  }
  
  float wildcardEd(const string& s1, const string& s2, const char wildchar = WILDCHAR);
  float wildcardEd(const char* s1, const char* s2, unsigned n, unsigned m, const char wildchar = WILDCHAR);
  
  // a bunch of filters for the edit distance that use positional information
  void checkMismatches(const Query& query, unsigned gramLength);
  void checkPartialString(const Query& query, const string& prePostQueryStr);
  void checkSubstrFilter(const Query& query, 
			 const string& prePostQueryStr,
			 const string& gram,
			 unsigned gramLength,
			 unsigned stringLength,
			 unsigned char stringPos,
			 unsigned char queryPos,
			 unsigned substrFilterCount);

  void applyPosFilters();

};
#pragma pack(pop)



// declarations for member-function specializations follow (definitions are in .cc)

template<>
void  
OnDiskMergerAdapt<Array<PosID> >::
detectDuplicateLists(vector<QueryGramList<Array<PosID> >* >& allQgls, vector<WeightedGramList<Array<PosID> >* >& allWgls) const;

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
readNextLists(const Query& query,
	      const string& prePostQueryStr,
	      LeafContext<Array<PosID> >& leafContext, 
	      unsigned listsToRead, 
	      vector<unsigned>& results);

// WARNING: this will currently only work for edit distance, other similarity functions will NOT work
template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
heapMerge(const Query& query, const string& prePostQueryStr, LeafContext<Array<PosID> >& leafContext, vector<unsigned>& results) const;

template<>
Array<PosID>::iterator
OnDiskMergerAdapt<Array<PosID> >::
expProbe(Array<PosID>::iterator start, Array<PosID>::iterator end, unsigned value) const;

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
addRemainingCandidates(const Query& query, const string& prePostQueryStr, LeafContext<Array<PosID> >* lc, vector<unsigned>& results) const;

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
processPosID(const Query& query, 
	     const string& prePostQueryStr, 
	     unsigned gramCode,
	     unsigned candiID, 
	     vector<unsigned char>& wglPos,
	     unsigned char candiPos,
	     bool* positionsMatched,
	     Candidate<Array<PosID> >* c) const;
#endif
