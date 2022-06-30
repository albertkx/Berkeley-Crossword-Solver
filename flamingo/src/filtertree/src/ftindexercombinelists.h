/*
  $Id: ftindexercombinelists.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the Academic BSD license.
    
  Date: 04/04/2008
  Author: Shengyue Ji <shengyuj (at) ics.uci.edu>
          Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftindexerunionglobal_h_
#define _ftindexerunionglobal_h_

#include "ftindexermemabs.h"
#include "gramlistunion.h"
#include "statsutil.h"
#include "common/src/simmetric.h"
#include "listmerger/src/divideskipmerger.h"

#include <stdint.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>

// uncompressed indexer that uses GramListUnion as inverted list structure
// we create a new uncompressed indexer to maximize code reuse
template<class StringContainer, class InvList>
class FtIndexerUncompressedUnion;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerUncompressedUnion<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerUncompressedUnion 
  : public FtIndexerMemAbs<FtIndexerUncompressedUnion<StringContainer, InvList> > { 

private:
  typedef FtIndexerMemAbs<FtIndexerUncompressedUnion<StringContainer, InvList> > SuperClass;
  
public:
  typedef typename SuperClass::GramMap GramMap;
  
  FtIndexerUncompressedUnion(StringContainer* strContainer,
			     GramGen* gramGenerator, 
			     unsigned maxStrLen = 150, 
			     unsigned ftFanout = 50) 
    : SuperClass(strContainer, gramGenerator, maxStrLen, ftFanout) {
#ifdef DEBUG_STAT
    this->ixStats = new IndexStats();
#endif
  }
  
  void prepare_Impl() {};
  void addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId);
  CompressionArgs* getCompressionArgs_Impl() { return &(this->compressionArgs); }

  // WARNING: these will not be implemented here, this indexer cannot be saved/loaded 
  // it should only be used within union indexers and therefore saving/loading is not neccessary
  void saveIndex_Impl(const char* indexFileName) { 
    cout << "WARNING: not implemented" << endl;
  }
  void loadIndex_Impl(const char* indexFileName) {
    cout << "WARNING: not implemented" << endl;
  }
};

template<class StringContainer, class InvList>
void 
FtIndexerUncompressedUnion<StringContainer, InvList>::
addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId) {
  typename 
    FtIndexerMemAbs<FtIndexerUncompressedUnion<StringContainer, InvList> >::GramMap& 
    gramMap = leaf->gramMap;

  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes.at(i);
    
    if (gramMap.find(gramCode) == gramMap.end()) {
      // a new gram
      GramListUnion<InvList>* newGramList = new GramListUnion<InvList>();
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

class CandiPair {
public:
  unsigned ida;
  unsigned idb;
  CandiPair() { ida = 0; idb = 0; }
  CandiPair(unsigned x, unsigned y) {
    if(x <= y) { ida = x; idb = y; }
    else { ida = y; idb = x; }
  }
  bool operator()(const CandiPair &x, const CandiPair &y) const { return x.ida == y.ida && x.idb == y.idb; }
  size_t operator()(const CandiPair &x) const { return x.ida ^ x.idb; }
};

template<class FtIndexerConcrete>
class FtIndexerCombineListsAbs;

template<class FtIndexerConcrete>
struct FtIndexerTraits<FtIndexerCombineListsAbs<FtIndexerConcrete> > {
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
};

template <class FtIndexerCombineListsConcrete>
class FtIndexerCombineListsAbs 
  : public FtIndexerMemAbs<FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete> > {

public:
  typedef FtIndexerTraits<FtIndexerCombineListsConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;

#ifdef DEBUG_STAT
  typedef UnionGlobalIndexStats IxStatsType;
#endif

private:
  typedef 
  FtIndexerMemAbs<FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete> > 
  SuperClass;
  
protected:
  typedef typename SuperClass::GramMap GramMap;
  typedef typename tr1::unordered_set<CandiPair, CandiPair, CandiPair> CandiSet;
  typedef typename tr1::unordered_map<unsigned, vector<unsigned> > SigMap;  

  vector<string> sampleDictionary;
  FtIndexerUncompressedUnion<>* globalIndex;
  unsigned originalIndexSize;
  float reductionRatio;
  float dictSamplingFrac;
  GramMap gramMapSkeleton; // this contains which lists should be combined
  unsigned* rnds; // precalculated array of random numbers
  unsigned sigCount; // number of minhash signatures
  unsigned bandSize; // band size for lsh
  SigMap sigs; // map from gramcode to vector of minhash signatures
  
  // estimate similarity threshold that will satisfy given memory constraint
  float lshGreedyTh(float ratio);
  // get candidate pairs from adjacent grams
  void adjCandis(GramMap& gramMap, float unionTh, CandiSet& cps, unsigned minListSize = 10);
  // get candidate pairs from minhash-lsh
  void lshCandis(GramMap& gramMap, float unionTh, CandiSet& cps, unsigned minListSize = 10, unsigned seed = 250);
  // generate minhash signatures
  void genSigs(const GramMap& gramMap, unsigned minListSize = 10); 
  // fast hash function approximating permutation functions for minhash
  unsigned intHash(unsigned key, unsigned hid) const; 
  // combine lists from a given set of candidates satisfying a similarity threshold
  unsigned unionLists(GramMap& gramMap, float unionTh, CandiSet& cps, float ratio); 
  
  void getRandomSample(const vector<string>& source, 
		       vector<string>& target, 
		       float samplingFrac, 
		       bool trueRand = false);
  
  void getRandomSample(StringContainer* source,
		       vector<string>& target, 
		       float samplingFrac, 
		       bool trueRand = false);

  void copyGramMapSkeleton(GramMap& source, GramMap& destination);

  // for saving/loading the index
  void saveLeavesWithSharedListsRec(FilterTreeNode<InvList>* node, ofstream& fpOut);
  void loadLeavesWithSharedListsRec(FilterTreeNode<InvList>* node, ifstream& fpIn);

public:
  FtIndexerCombineListsAbs(StringContainer* strContainer) : SuperClass(strContainer) {
    globalIndex = NULL;
    rnds = NULL;
#ifdef DEBUG_STAT
    this->ixStats = new UnionGlobalIndexStats();
#endif
  }

  FtIndexerCombineListsAbs(StringContainer* strContainer,
			  GramGen* gramGenerator, 
			  float reductionRatio,
			  float dictSamplingFrac,
			  unsigned maxStrLen = 150, 
			  unsigned ftFanout = 50,
			  unsigned sigCount = 100,
			  unsigned bandSize = 2)
    : SuperClass(strContainer, gramGenerator, maxStrLen, ftFanout) {
    this->reductionRatio = reductionRatio;
    this->dictSamplingFrac = dictSamplingFrac;
    this->globalIndex = NULL;
    this->originalIndexSize = 0;
    this->rnds = NULL;
    this->sigCount = sigCount;
    this->bandSize = bandSize;
    
#ifdef DEBUG_STAT
    this->ixStats = new UnionGlobalIndexStats();
    dynamic_cast<UnionGlobalIndexStats*>(this->ixStats)->reductionRatio = reductionRatio;
#endif
  }
  
  // statically delegate the prepare implementation to appropriate subclass
  void prepare_Impl() {
    static_cast<FtIndexerCombineListsConcrete*>(this)->chooseUnionLists();
  }
  
  void addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId);
  CompressionArgs* getCompressionArgs_Impl() { return NULL; }
  
  void saveIndex_Impl(const char* indexFileName);
  void loadIndex_Impl(const char* indexFileName);

  ~FtIndexerCombineListsAbs() {
    if(globalIndex) delete globalIndex;
    if(rnds) delete rnds;
  }
};

template<class FtIndexerCombineListsConcrete>
void 
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId) {
  typename SuperClass::GramMap& gramMap = leaf->gramMap;

  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes.at(i);
    
    if (gramMap.find(gramCode) == gramMap.end()) {
      // check if the gram shares a list with another gram
      if(gramMapSkeleton.find(gramCode) != gramMapSkeleton.end()) {
	// copy the structure of combined lists from the gramMapSkeleton to the gramMap of this leaf
	copyGramMapSkeleton(gramMapSkeleton, gramMap);      
	// get the list and append element
	GramList<InvList>* gramList = gramMap[gramCode];
	InvList* array = gramList->getArray();
	array->push_back(stringId);      
      }
      else {
	// a new gram that does not share its list with any other gram
	GramListSimple<InvList>* newGramList = new GramListSimple<InvList>();
	gramMap[gramCode] = newGramList;    
	InvList* array = newGramList->getArray();
	array->push_back(stringId);
      }
    }
    else {
      // append stringId to list of an existing gram
      GramList<InvList>* gramList = gramMap[gramCode];
      InvList* array = gramList->getArray();    
      // avoid adding duplicate elements
      if(array->size() > 0) {
	if(array->back() != stringId) 
	  array->push_back(stringId);	
      }
      else array->push_back(stringId);
    }	        
  }
}

template<class FtIndexerCombineListsConcrete>
void 
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
getRandomSample(const vector<string>& source, 
		vector<string>& target, 
		float samplingFrac, 
		bool trueRand) {
  if(trueRand) srand(time(NULL));
  else srand(250);

  if(samplingFrac < 1.0) {
    unsigned numberStrings = (unsigned)((float)source.size() * samplingFrac);
    for(unsigned i = 0; i < numberStrings; i++) {
      unsigned index = rand() % source.size();
      target.push_back(source.at(index));
    }
  }
  else {
    for(unsigned i = 0; i < source.size(); i++)
      target.push_back(source.at(i));
  }
}

template<class FtIndexerCombineListsConcrete>
void 
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
getRandomSample(StringContainer* source, 
		vector<string>& target, 
		float samplingFrac, 
		bool trueRand) {
  if(trueRand) srand(time(NULL));
  else srand(250);
  
  if(samplingFrac < 1.0) {
    unsigned numberStrings = (unsigned)((float)source->size() * samplingFrac);
    for(unsigned i = 0; i < numberStrings; i++) {
      unsigned index = rand() % source->size();
      string currentString;
      source->retrieveString(currentString, index);
      target.push_back(currentString);
    }
  }
  else {
    for(unsigned i = 0; i < source->size(); i++) {
      string currentString;
      source->retrieveString(currentString, i);
      target.push_back(currentString);
    }
  }
}


template<class FtIndexerCombineListsConcrete>
void 
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
copyGramMapSkeleton(GramMap& source, GramMap& destination) {
  typename GramMap::iterator it;
  for(it = source.begin(); it != source.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);
    gl->rank = 0;
  }
  for(it = source.begin(); it != source.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);        
    if(gl->refCount > 1) {
      if(gl->rank == 0)	{
	gl->rank = it->first;
	GramListUnion<InvList> *tmpgl = new GramListUnion<InvList>();
	tmpgl->rank = it->first;
	destination[it->first] = tmpgl;
      }
      else {
	GramListUnion<InvList>* tmpgl = dynamic_cast<GramListUnion<InvList>* >(destination[gl->rank]);
	destination[it->first] = tmpgl;
	tmpgl->refCount++;
      }
    }
  }
}

template<class FtIndexerCombineListsConcrete>
float 
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
lshGreedyTh(float ratio) {
  float ths[] = {0.9, 0.8, 0.7, 0.6, 0.6, 0.4, 0.3, 
		 0.25, 0.2, 0.18, 0.16, 0.14, 0.12, 0.1, 0.09, 0.08, 0.07, 0.06, 0.05};  

  // find number of candidate required to satisfy memory constraint
  unsigned i;
  StringContainerVector* tmpDictContainer = new StringContainerVector();
  tmpDictContainer->fillContainer(this->sampleDictionary.begin(), this->sampleDictionary.end());
  for(i = 0; i < 18; i++) {
    FtIndexerUncompressedUnion<> index(tmpDictContainer, this->gramGen);
    index.buildIndex();    
    GramMap& gramMap = index.filterTreeRoot->gramMap;
    CandiSet cps;

    adjCandis(gramMap, ths[i], cps); 
    lshCandis(gramMap, ths[i], cps);
    unsigned newIndexSize = unionLists(gramMap, ths[i], cps, 0.0);
    
    if(newIndexSize <= ratio * this->originalIndexSize) break;
  }
  delete tmpDictContainer;

  return ths[i];
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
adjCandis(GramMap& gramMap, float unionTh, CandiSet& cps, unsigned minListSize) {
  CandiSet ncps;
  unsigned i, j;
  
  for(i = 0; i < sampleDictionary.size(); i++) {
    vector<unsigned> gids;
    this->gramGen->decompose(sampleDictionary.at(i), gids);
    for(j = 0; j < gids.size() - 1; j++) {
      CandiPair cp(gids[j], gids[j + 1]);
      if(cp.ida == cp.idb)continue;
      
      if(cps.count(cp) || ncps.count(cp)) continue;
      
      if(gramMap[cp.ida]->getArray()->size() < minListSize
	 || gramMap[cp.idb]->getArray()->size() < minListSize) {
	ncps.insert(cp);
	continue;
      }
      
      if(GramListUnion<InvList>::testUnion(dynamic_cast<GramListUnion<InvList>* >(gramMap[cp.ida]), 
					   dynamic_cast<GramListUnion<InvList>* >(gramMap[cp.idb]), 
					   unionTh, 
					   true))
	cps.insert(cp);
      else ncps.insert(cp);
    }
  }
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
lshCandis(GramMap& gramMap, float unionTh, CandiSet& cps, unsigned minListSize, unsigned seed) {
  unsigned i, j, k;
  srand(seed);
  rnds = (unsigned *)realloc(rnds, sigCount * 4 * sizeof(unsigned));
  for(i = 0; i < sigCount * 4; i++)
    rnds[i] = rand();
  
  genSigs(gramMap, minListSize);
  
  CandiSet ncps;
  for(i = 0; i < sigCount; i += bandSize) {
    SigMap lsh;
    SigMap::const_iterator cit;
    
    for(cit = sigs.begin(); cit != sigs.end(); cit++) {
      if(cit->second.size() < i + bandSize)
	continue;
      
      unsigned sum = 0;
      for(j = 0; j < bandSize; j++) {
	sum = (sum << 3);
	sum = (sum ^ cit->second[i + j]);
      }
      
      for(k = 0; k < lsh[sum].size(); k++) {
	CandiPair cp(lsh[sum][k], cit->first);
	if(cps.count(cp) || ncps.count(cp))continue;
	
	if(GramListUnion<InvList>::testUnion(dynamic_cast<GramListUnion<InvList>* >(gramMap[cp.ida]), 
					     dynamic_cast<GramListUnion<InvList>* >(gramMap[cp.idb]), 
					     unionTh, 
					     true))
	  cps.insert(cp);
	else ncps.insert(cp);
      }
      lsh[sum].push_back(cit->first);
    }
  }
}

template<class FtIndexerCombineListsConcrete>
unsigned
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
unionLists(GramMap& gramMap, float unionTh, CandiSet& cps, float ratio) {
  unsigned listsUnioned = 0;
  unsigned localSize = this->originalIndexSize;
  CandiSet::const_iterator cit;
  for(cit = cps.begin(); cit != cps.end(); cit++) {
    unsigned saved = GramListUnion<InvList>::testUnion(dynamic_cast<GramListUnion<InvList>* >(gramMap[cit->ida]), 
						       dynamic_cast<GramListUnion<InvList>* >(gramMap[cit->idb]), 
						       unionTh);
    if(saved) {
      listsUnioned++;
      localSize -= saved;
      if(localSize <= ratio * this->originalIndexSize) break;
    }
  }
  
  typename GramMap::iterator it;
  for(it = gramMap.begin(); it != gramMap.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);
    gl->find();
  }
  for(it = gramMap.begin(); it != gramMap.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);
    gramMap[it->first] = gl->clean();   
  }
  
  return localSize;
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
genSigs(const GramMap& gramMap, unsigned minListSize) {
  unsigned j, k;
  typename GramMap::const_iterator cit;
  for(cit = gramMap.begin(); cit != gramMap.end(); cit++) {
    const GramListUnion<InvList> *gl= dynamic_cast<const GramListUnion<InvList>* >(cit->second);
    if(gl->list.size() < minListSize) continue;
    unsigned count = sigCount;
    if(gl->list.size() < count)
      count = gl->list.size();
    
    vector<unsigned> &sig = sigs[cit->first];
    sig.resize(count);
    for(k = 0; k < count; k++)
      sig[k] = intHash(gl->list[0], k);
    
    for(j = 1; j < gl->list.size(); j++)
      for(k = 0; k < count; k++) {
	unsigned newSig = intHash(gl->list[j], k);
	if(newSig < sig[k])sig[k] = newSig;
      }
  }
}


template<class FtIndexerCombineListsConcrete>
unsigned
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
intHash(unsigned key, unsigned hid) const {
  unsigned rnd;    
  rnd = rnds[hid << 2];
  key = (key + rnd) + (key << (rnd & 31));
  rnd = rnds[(hid << 2) + 1];
  key = (key ^ rnd) ^ (key >> (rnd & 31));
  rnd = rnds[(hid << 2) + 2];
  key = (key + rnd) + (key << (rnd & 31));
  rnd = rnds[(hid << 2) + 3];
  key = (key ^ rnd) ^ (key >> (rnd & 31));
  return key;
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
saveLeavesWithSharedListsRec(FilterTreeNode<InvList>* node, ofstream& fpOut) {
  unsigned u;
  
  if(node->isLeaf()) {
    if(node->distinctStringIDs) {
      InvList* distinctArr = node->distinctStringIDs->getArray();
      // write distinct string ids
      u = distinctArr->size();
      fpOut.write((const char*)&u, sizeof(unsigned));
      for(unsigned i = 0; i < distinctArr->size(); i++) {
	u = distinctArr->at(i);
	fpOut.write((const char*)&u, sizeof(unsigned));      
      }      
    }
    else {
      u = 0;
      fpOut.write((const char*)&u, sizeof(unsigned));
    }
    
    unordered_map<uintptr_t, unsigned> listAddrMap; // maps from list address to first gram code that points to the list
    // write gram map
    GramMap& gramMap = node->gramMap;
    u = gramMap.size();
    fpOut.write((const char*)&u, sizeof(unsigned));
    for(typename GramMap::iterator iter = gramMap.begin();     
	iter != gramMap.end();
	iter++) {
      
      u = iter->first;
      fpOut.write((const char*)&u, sizeof(unsigned));

      vector<unsigned>* arr = iter->second->getArray();      
      if(arr) {
	uintptr_t listAddr = (uintptr_t)arr;
	if(listAddrMap.find(listAddr) == listAddrMap.end()) {
	  // insert the address into the map
	  listAddrMap[listAddr] = iter->first;

	  unsigned isFirst = 1;
	  fpOut.write((const char*)&isFirst, sizeof(unsigned)); // it is the first gram referencing this list

	  // write the list contents
	  u = arr->size();
	  fpOut.write((const char*)&u, sizeof(unsigned));
	  for(unsigned i = 0; i < arr->size(); i++) {
	    u = arr->at(i);
	    fpOut.write((const char*)&u, sizeof(unsigned));
	  }
	}
	else {
	  // a previous gram referencing this list has stored the content
	  unsigned isFirst = 0;	  
	  fpOut.write((const char*)&isFirst, sizeof(unsigned)); // it is NOT the first gram referencing this list
	  unsigned firstGramCode = listAddrMap[listAddr];
	  fpOut.write((const char*)&firstGramCode, sizeof(unsigned)); // write the gramcode that points to the list
	}
      }
      else {	
	u = 2;
	fpOut.write((const char*)&u, sizeof(unsigned)); // whether it is a first list or not
      }      
    }    
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      saveLeavesWithSharedListsRec(children.at(i).node, fpOut);
  }
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
loadLeavesWithSharedListsRec(FilterTreeNode<InvList>* node, ifstream& fpIn) {
  if(node->isLeaf()) {
    // load distinct strings
    if(node->distinctStringIDs) delete node->distinctStringIDs;
    node->distinctStringIDs = new GramListSimple<InvList>();
    unsigned distinctStrings;
    fpIn.read((char*)&distinctStrings, sizeof(unsigned));
    if(distinctStrings > 0) node->distinctStringIDs = new GramListSimple<InvList>();
    for(unsigned i = 0; i < distinctStrings; i++) {
      unsigned tmp;
      fpIn.read((char*)&tmp, sizeof(unsigned)); 
      node->distinctStringIDs->getArray()->push_back(tmp);
    }    
    
    // load gram map
    GramMap& gramMap = node->gramMap;
    unsigned gramMapSize;
    unsigned gramCode;
    unsigned arrSize;
    unsigned isFirst;
    unsigned firstGramCode;
    fpIn.read((char*)&gramMapSize, sizeof(unsigned));    
    for(unsigned j = 0; j < gramMapSize; j++) {
      fpIn.read((char*)&gramCode, sizeof(unsigned));      
      fpIn.read((char*)&isFirst, sizeof(unsigned));      
      switch(isFirst) {

      case 0: {
	fpIn.read((char*)&firstGramCode, sizeof(unsigned));      	
	GramListUnion<InvList>* gramList = dynamic_cast<GramListUnion<InvList>* >(gramMap[firstGramCode]);
	gramList->refCount++;
	gramMap[gramCode] = gramList;
      }	break;
	
      case 1: {
	fpIn.read((char*)&arrSize, sizeof(unsigned));
	if(arrSize != 0) {	
	  GramListUnion<InvList>* gramList = new GramListUnion<InvList>();
	  gramList->refCount = 1;
	  gramMap[gramCode] = gramList;
	  InvList* newArr = gramList->getArray();
	  for(unsigned i = 0; i < arrSize; i++) {
	    unsigned tmp;
	    fpIn.read((char*)&tmp, sizeof(unsigned));
	    newArr->push_back(tmp);		
	  }
	}		
      } break;

      case 2: {
	continue;
      } break;

      default: {
	cout << "THIS SHOULD NOT HAPPEN" << endl;
      } break;
      }
    }
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      loadLeavesWithSharedListsRec(children.at(i).node, fpIn);
  } 
}


template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
saveIndex_Impl(const char* indexFileName) {
  ofstream fpOut;
  fpOut.open(indexFileName, ios::out);
  if(fpOut.is_open()) {
    this->saveBasicInfo(fpOut);
    this->saveLeavesWithSharedListsRec(this->filterTreeRoot, fpOut);
    fpOut.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;  
}

template<class FtIndexerCombineListsConcrete>
void
FtIndexerCombineListsAbs<FtIndexerCombineListsConcrete>::
loadIndex_Impl(const char* indexFileName) {
  ifstream fpIn;
  fpIn.open(indexFileName, ios::in);
  if(fpIn.is_open()) {
    this->filterTypes.clear();  
    this->loadBasicInfo(fpIn);  
    this->buildHollowTreeRecursive(this->filterTreeRoot, 0);  
    this->loadLeavesWithSharedListsRec(this->filterTreeRoot, fpIn);  
    fpIn.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;    
}

template<class StringContainer, class InvList>
class FtIndexerCombineListsBasic;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerCombineListsBasic<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerCombineListsBasic 
  : public FtIndexerCombineListsAbs<FtIndexerCombineListsBasic<StringContainer, InvList> > {
  
private:
  typedef FtIndexerCombineListsAbs<FtIndexerCombineListsBasic<StringContainer, InvList> > SuperClass;
  typedef typename SuperClass::GramMap GramMap;

public:
  FtIndexerCombineListsBasic(StringContainer* strContainer) : SuperClass(strContainer) {}

  FtIndexerCombineListsBasic(StringContainer* strContainer,
			    GramGen* gramGenerator, 
			    float reductionRatio,
			    float dictSamplingFrac = 0.1f,
			    unsigned maxStrLen = 150, 
			    unsigned ftFanout = 50) 
    : SuperClass(strContainer, gramGenerator, reductionRatio, dictSamplingFrac, maxStrLen, ftFanout) {}

  string getName() { 
    return "FtIndexerCombineListsBasic"; 
  }
  
  void chooseUnionLists();
};

template<class StringContainer, class InvList>
void
FtIndexerCombineListsBasic<StringContainer, InvList>::
chooseUnionLists() {  
  if(this->reductionRatio <= 0) return;
  
  // first build a global uncompressed index without filters
  // create sample dictionary
  this->sampleDictionary.clear();
  this->getRandomSample(this->strContainer, this->sampleDictionary, this->dictSamplingFrac);

  // build the index on the sampling dictionary
  StringContainerVector* tmpDictContainer = new StringContainerVector();
  tmpDictContainer->fillContainer(this->sampleDictionary.begin(), this->sampleDictionary.end());

  this->globalIndex = new FtIndexerUncompressedUnion<>(tmpDictContainer, this->gramGen);
  this->globalIndex->buildIndex();

  delete tmpDictContainer;
  
  // determine the orginal index size
  this->originalIndexSize = 
    FilterTreeNode<InvList>::getSubTreeInvertedListsSize(this->globalIndex->filterTreeRoot);
  
  float unionTh = this->lshGreedyTh(1.0f - this->reductionRatio);
  
  // TODO do something about the method
  unsigned method = 3;
  GramMap& gramMap = this->globalIndex->filterTreeRoot->gramMap;
  typename SuperClass::CandiSet cps;
  if(method & 1) this->adjCandis(gramMap, unionTh, cps);
  if(method & 2) this->lshCandis(gramMap, unionTh, cps);
  
  this->unionLists(gramMap, unionTh, cps, 1.0f - this->reductionRatio);
  
  // now copy the structure to the union skeleton iun this class
  this->copyGramMapSkeleton(this->globalIndex->filterTreeRoot->gramMap,
			    this->gramMapSkeleton);
  
  // cleanup
  delete this->globalIndex;
  this->globalIndex = NULL;
  free(this->rnds);
  this->rnds = NULL;
}


struct QuerySample {
  unsigned id;
  int th;    
  float queryTime;
  unsigned candiCount;
  vector<unsigned char> scanCounts;
};

template<class StringContainer, class InvList>
class FtIndexerCombineListsCost;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerCombineListsCost<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template <class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerCombineListsCost 
  : public FtIndexerCombineListsAbs<FtIndexerCombineListsCost<StringContainer, InvList> > {

private:
  typedef FtIndexerCombineListsAbs<FtIndexerCombineListsCost<StringContainer, InvList> > SuperClass;
  typedef typename SuperClass::GramMap GramMap;
  typedef typename SuperClass::CandiSet CandiSet;

  FtIndexerUncompressedUnion<>* queryIndex;
  DivideSkipMerger<> merger;

  const vector<string>* workload;
  vector<string> sampleWorkload;
  float queriesSamplingFrac;  

  const SimMetric* simMetric;
  float simThreshold;
  
  float avgPostprocessTime;
  float totalQueryTime;
  unsigned currentIndexSize;
  vector<QuerySample> qs;
  
  void buildPostprocessEstimationModel(const vector<string>& sampleDictionary,
				       const vector<string>& sampleWorkload,
				       const unsigned iterations = 100);
  inline float getApproxPostprocessTime(const unsigned totalCandidates) { return avgPostprocessTime * (float)totalCandidates; }
  
  void buildMergeEstimationModel(const vector<string>& sampleWorkload);  
    
  void initEstimations();
  void greedyUnion(GramMap& gramMap, float unionTh, float ratio, unsigned k, CandiSet& cps);
  void greedyIteration(GramMap& gramMap, float unionTh, unsigned k, CandiSet& cps);
  bool unionEstimation(GramMap& gramMap, 
		       float unionTh, 
		       unsigned gida, 
		       unsigned gidb, 
		       unsigned &reduction, 
		       float &time, 
		       bool update = false);
  
public:
  FtIndexerCombineListsCost(StringContainer* strContainer) : SuperClass(strContainer) {
    queryIndex = NULL;
    workload = NULL;
    simMetric = NULL;
    simThreshold = 0.0f;
    queriesSamplingFrac = 0.0f;
    avgPostprocessTime = 0.0f;
    totalQueryTime = 0.0f;
    currentIndexSize = 0;
  }
  
  FtIndexerCombineListsCost(StringContainer* strContainer,
			   GramGen* gramGenerator, 
			   const float reductionRatio,
			   const vector<string>* workload,
			   const SimMetric* simMetric,
			   const float simThreshold,
			   const float dictSamplingFrac = 0.1f,
			   const float queriesSamplingFrac = 0.25f,
			   const unsigned maxStrLen = 150, 
			   const unsigned ftFanout = 50) 
    : SuperClass(strContainer, gramGenerator, reductionRatio, dictSamplingFrac, maxStrLen, ftFanout) {
    
    this->queryIndex = NULL;
    this->workload = workload;
    this->simMetric = simMetric;
    this->simThreshold = simThreshold;
    this->queriesSamplingFrac = queriesSamplingFrac;
    this->avgPostprocessTime = 0.0f;
    this->totalQueryTime = 0.0f;
    this->currentIndexSize = 0;
  }
  
  string getName() { 
    return "FtIndexerCombineListsCost"; 
  }
  
  void chooseUnionLists();

  ~FtIndexerCombineListsCost() {
    if(queryIndex) delete queryIndex;
  }
};


template<class StringContainer, class InvList>
void 
FtIndexerCombineListsCost<StringContainer, InvList>::
buildMergeEstimationModel(const vector<string>& sampleWorkload) {
  vector<vector<vector<unsigned>*>*> allLists;
  vector<unsigned> allThresholds;
  FilterTreeNode<>* leaf = this->globalIndex->filterTreeRoot;

  // gather all lists from all queries
  for(unsigned i = 0; i < sampleWorkload.size(); i++) {
    const string& query = sampleWorkload.at(i);
    vector<vector<unsigned>*>* invertedLists = new vector<vector<unsigned>*>();
    vector<unsigned> gids;
    this->gramGen->decompose(query, gids);
    leaf->getInvertedLists(gids, *invertedLists);
    allLists.push_back(invertedLists);
    allThresholds.push_back(this->simMetric->getMergeThreshold(query, gids, this->simThreshold, NULL));
  }

  // build model
  merger.fillEstimationParams(allLists, allThresholds);
    
  // cleanup
  for(unsigned i = 0; i < allLists.size(); i++) {
    vector<vector<unsigned>*>* invertedLists = allLists.at(i);
    delete invertedLists;
  }
}

template<class StringContainer, class InvList>
void 
FtIndexerCombineListsCost<StringContainer, InvList>::
buildPostprocessEstimationModel(const vector<string>& sampleDictionary,
				const vector<string>& sampleWorkload,
				const unsigned iterations) {
  StatsUtil sutil;
  
  srand(150);
  
  float totalTime = 0.0;  
  unsigned counter = 0;

  for(unsigned z = 0; z < iterations; z++) { 
    unsigned q = rand() % sampleWorkload.size();
    string queryString = sampleWorkload.at(q);
    sutil.startTimeMeasurement();
    (*simMetric)(queryString, queryString);
    sutil.stopTimeMeasurement();    
    totalTime += sutil.getTimeMeasurement(TFMSEC);
    counter++;   
    for(unsigned i = 0; i < sampleDictionary.size(); i++) {
      sutil.startTimeMeasurement();
      (*simMetric)(queryString, sampleDictionary.at(i));
      sutil.stopTimeMeasurement();    
      totalTime += sutil.getTimeMeasurement(TFMSEC);
      counter++;   
    }
  }  
  
  avgPostprocessTime = totalTime / (float)counter;
}

template<class StringContainer, class InvList>
void
FtIndexerCombineListsCost<StringContainer, InvList>::
initEstimations() {  	   
  buildPostprocessEstimationModel(this->sampleDictionary, sampleWorkload);
  buildMergeEstimationModel(sampleWorkload);

  qs.resize(sampleWorkload.size());
  
  unsigned i,j,k;

  // init estimation    
  totalQueryTime = 0.0;
  float totalPPTime = 0.0;
  for(i = 0; i < sampleWorkload.size(); i++) {
    QuerySample &q = qs[i];
    q.th = (int)simMetric->getMergeThreshold(sampleWorkload.at(i), simThreshold, NULL);
    q.candiCount = 0;
    q.queryTime = 0.0f;
    q.scanCounts.resize(this->sampleDictionary.size());
    for(j = 0; j < q.scanCounts.size(); j++)
      q.scanCounts[j] = 0;
    if(q.th <= 0) {
      // panic
      q.candiCount = this->sampleDictionary.size();
      q.queryTime = getApproxPostprocessTime(q.candiCount);
      totalQueryTime += q.queryTime;
      continue;
    }
        
    // init scan count vector        
    FilterTreeNode<>* leaf = this->globalIndex->filterTreeRoot;
    vector<vector<unsigned>* > invertedLists;
    leaf->getInvertedLists(sampleWorkload.at(i), *(this->gramGen), invertedLists);
    for(j = 0; j < invertedLists.size(); j++) {
      vector<unsigned>* a = invertedLists.at(j);
      for(k = 0; k < a->size(); k++)
	q.scanCounts[a->at(k)]++;      
    }

    // count candidates
    for(j = 0; j < q.scanCounts.size(); j++)
      if(q.scanCounts[j] >= (unsigned)q.th)
	q.candiCount++;
       
    // initialize query time
    float mergeTime = merger.getEstimatedQueryTime(invertedLists, q.th);
    float currentPPTime = getApproxPostprocessTime(q.candiCount);
    q.queryTime = mergeTime + currentPPTime;
    totalPPTime += currentPPTime;
    
    // add to total query time and candi count
    totalQueryTime += q.queryTime;
  }
}


template<class StringContainer, class InvList>
void
FtIndexerCombineListsCost<StringContainer, InvList>::
greedyUnion(GramMap& gramMap, float unionTh, float ratio, unsigned k, CandiSet& cps) {
  currentIndexSize = this->originalIndexSize;  
  while(currentIndexSize > (unsigned)(ratio * this->originalIndexSize) && cps.size()) {
    //cout << "ITERATION: " << currentIndexSize << " " << (unsigned)(ratio * this->originalIndexSize) << endl;
    greedyIteration(gramMap, unionTh, k, cps);
  }
  
  // compress paths
  typename GramMap::iterator it;
  for(it = gramMap.begin(); it != gramMap.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);
    gl->find();
  }
  for(it = gramMap.begin(); it != gramMap.end(); it++) {
    GramListUnion<InvList>* gl = dynamic_cast<GramListUnion<InvList>* >(it->second);
    gramMap[it->first] = gl->clean();   
  }
}


template<class StringContainer, class InvList>
void
FtIndexerCombineListsCost<StringContainer, InvList>::
greedyIteration(GramMap& gramMap, float unionTh, unsigned k, CandiSet& cps) {
  // greedy search for best score
  unsigned reduction = 0;
  float time = totalQueryTime;
  
  multimap<int, CandiPair> topk;
  multimap<int, CandiPair>::iterator it;
  
  // loop for all candidates
  typename CandiSet::iterator csit;
  for(csit = cps.begin(); csit != cps.end(); csit++)
    if(unionEstimation(gramMap, unionTh, csit->ida, csit->idb, reduction, time)) {
	int score = (int)(-1000000 * time + reduction);
	it = topk.begin();
	if(topk.size() < k || score > it->first) {
	  topk.insert(pair<int, CandiPair>(score, *csit));
	  if(topk.size() > k) {
	    it = topk.begin();
	    topk.erase(it);
	  }
	}
	//            else if(score < 2 * it->first)
	//                cps.erase(csit);
    }
  
  if(topk.size() == 0) {
    cps.clear();
    return;
  }

  // do union for all top k
  GramMap& queriesMap = this->queryIndex->filterTreeRoot->gramMap;
  for(it = topk.begin(); it != topk.end(); it++) {
    if(unionEstimation(gramMap, unionTh, it->second.ida, it->second.idb, reduction, time, true)) {
      currentIndexSize -= GramListUnion<InvList>::testUnion(dynamic_cast<GramListUnion<InvList>* >(gramMap[it->second.ida]), 
							    dynamic_cast<GramListUnion<InvList>* >(gramMap[it->second.idb]));      

      //listsUnioned++;      

      if(queriesMap.count(it->second.ida) && queriesMap.count(it->second.idb))
	GramListUnion<InvList>::testUnion(dynamic_cast<GramListUnion<InvList>* >(queriesMap[it->second.ida]), 
					  dynamic_cast<GramListUnion<InvList>* >(queriesMap[it->second.idb]));      
    }    
    cps.erase(it->second);
  }
}


template<class StringContainer, class InvList>
bool
FtIndexerCombineListsCost<StringContainer, InvList>::
unionEstimation(GramMap& gramMap, float unionTh, unsigned gida, unsigned gidb,
		unsigned &reduction, float &time, bool update) {
  
  vector<unsigned> exta, extb;
  
  reduction = 0;
  time = totalQueryTime;

  GramListUnion<InvList>* gla = dynamic_cast<GramListUnion<InvList>* >(gramMap[gida]);
  GramListUnion<InvList>* glb = dynamic_cast<GramListUnion<InvList>* >(gramMap[gidb]);    
  if(gla == glb) return false;
  
  vector<unsigned> &a = gla->find()->list;
  vector<unsigned> &b = glb->find()->list;
  
  // gida, gidb might be missing for query list, if so use dummy instead
  GramMap& queriesMap = this->queryIndex->filterTreeRoot->gramMap;  
  vector<unsigned> dummy;
  vector<unsigned> *qa = &dummy, *qb = &dummy;
  if(queriesMap.count(gida)) {
    GramListUnion<InvList>* qla = dynamic_cast<GramListUnion<InvList>* >(queriesMap[gida]);
    qa = &qla->find()->list;
  }
  if(queriesMap.count(gidb)) {
    GramListUnion<InvList>* qlb = dynamic_cast<GramListUnion<InvList>* >(queriesMap[gidb]);  
    qb = &qlb->find()->list;  
  }

  // compute exta and extb
  unsigned i = 0, j = 0, k = 0;
  while(i < a.size() && j < b.size()) {
    if(a[i] > b[j]) {
      exta.push_back(b[j]);
      j++;
    }
    else if(a[i] < b[j]) {
      extb.push_back(a[i]);
      i++;
    }
    else {
      // increase reduction
      reduction++;
      i++; j++;
    }                
  }
  while(j < b.size()) {
    exta.push_back(b[j]);
    j++;
  }
  while(i < a.size()) {
    extb.push_back(a[i]);
    i++;
  }
  if(reduction < (unsigned)(unionTh * (exta.size() + a.size())))
    return false;
  
  // for each affected query
  i = 0; j = 0;
  while(i < qa->size() || j < qb->size()) {        
    // retrieve query q, move cursor forward, set ea and eb
    int ea = 0, eb = 0;
    if(i < qa->size()) {
      k = (*qa)[i];
      if(j < qb->size() && (*qb)[j] <= k) {
	k = (*qb)[j];
	j++;
	eb = 1;
      }
      if(k == (*qa)[i])	{
	i++;
	ea = 1;
      }
    }
    else {
      k = (*qb)[j];
      j++;
      eb = 1;
    }
    QuerySample &q = qs[k];
    
    // panic never changes        
    if(q.th < 0)continue;
    vector<unsigned> gids;
    vector<unsigned> gsize;
    this->gramGen->decompose(sampleWorkload.at(k), gids);
      
    GramListUnion<InvList>* tmpgla = dynamic_cast<GramListUnion<InvList>* >(gramMap[gida]);
    GramListUnion<InvList>* tmpglb = dynamic_cast<GramListUnion<InvList>* >(gramMap[gidb]);
    // find counta and countb, initialize gram size list
    unsigned counta = 0, countb = 0;    
    for(k = 0; k < gids.size(); k++)
      if(gramMap.count(gids[k])) {
	GramListUnion<InvList>* tmpglk = dynamic_cast<GramListUnion<InvList>* >(gramMap[gids[k]]);
	unsigned size = tmpglk->find()->list.size();
	if(ea && tmpglk->find() == tmpgla->find()) {
	  size += exta.size();
	  counta++;
	}
	else if(eb && tmpglk->find() == tmpglb->find()) {
	  size += extb.size();
	  countb++;
	}
	if(size)gsize.push_back(size);
      }

    // count candidate delta for q
    unsigned delta = 0;
    if(counta) {
      for(k = 0; k < exta.size(); k++) {
	unsigned original = 0;
	original = q.scanCounts[exta[k]];
	if(original < (unsigned)q.th && original + counta >= (unsigned)q.th)
	  delta++;
	
	if(update)q.scanCounts[exta[k]] = original + counta;
      }
    }
    if(countb) {
      for(k = 0; k < extb.size(); k++) {
	unsigned original = 0;
	original = q.scanCounts[extb[k]];
	if(original < (unsigned)q.th && original + countb >= (unsigned)q.th)
	  delta++;
	
	if(update)q.scanCounts[extb[k]] = original + countb;
      }
    }

    FilterTreeNode<>* leaf = this->globalIndex->filterTreeRoot;
    vector<vector<unsigned>* > invertedLists;
    leaf->getInvertedLists(gids, invertedLists);
    if(update) {
      q.candiCount += delta;
      totalQueryTime -= q.queryTime;
      // TODO CHECK WITH SHENGYUE ABOUT DELTA, SEE BELOW
      q.queryTime = merger.getEstimatedQueryTime(invertedLists, q.th) + getApproxPostprocessTime(q.candiCount);
      assert(q.queryTime == q.queryTime);
      totalQueryTime += q.queryTime;
      time = totalQueryTime;
      continue;
    }
    
    // recompute time for q
    time -= q.queryTime;
    time += merger.getEstimatedQueryTime(invertedLists, q.th);
    assert(time == time);
    time += getApproxPostprocessTime(q.candiCount + delta); // TODO SEE HERE FOR DELTA
  }
  return true;  
}

template<class StringContainer, class InvList>
void
FtIndexerCombineListsCost<StringContainer, InvList>::
chooseUnionLists() {  
  if(this->reductionRatio <= 0) return;
  
  // first build a global uncompressed index without filters
  // create sample dictionary
  this->sampleDictionary.clear();
  this->getRandomSample(this->strContainer, this->sampleDictionary, this->dictSamplingFrac);
  sampleWorkload.clear();
  this->getRandomSample(*workload, sampleWorkload, queriesSamplingFrac);		      

  // build the index on the sampling dictionary
  StringContainerVector* tmpDictContainer = new StringContainerVector();
  tmpDictContainer->fillContainer(this->sampleDictionary.begin(), this->sampleDictionary.end());
  this->globalIndex = new FtIndexerUncompressedUnion<>(tmpDictContainer, this->gramGen);
  this->globalIndex->buildIndex();
  delete tmpDictContainer;

  // build the index on the queries
  StringContainerVector* tmpQueriesContainer = new StringContainerVector();
  tmpQueriesContainer->fillContainer(sampleWorkload.begin(), sampleWorkload.end());
  queryIndex = new FtIndexerUncompressedUnion<>(tmpQueriesContainer, this->gramGen);
  queryIndex->buildIndex();
  delete tmpQueriesContainer;  

  // determine the orginal index size
  this->originalIndexSize = 
    FilterTreeNode<InvList>::getSubTreeInvertedListsSize(this->globalIndex->filterTreeRoot);

  float unionTh = this->lshGreedyTh(1.0f - this->reductionRatio);
  
  // TODO do something about the method
  unsigned method = 3;
  GramMap& gramMap = this->globalIndex->filterTreeRoot->gramMap;
  typename SuperClass::CandiSet cps;
  if(method & 1) this->adjCandis(gramMap, unionTh, cps);
  if(method & 2) this->lshCandis(gramMap, unionTh, cps);

  initEstimations();

  unsigned k = cps.size() > 100 ? cps.size() / 100 : 1;
  greedyUnion(gramMap, unionTh, 1.0f - this->reductionRatio, k, cps);
  
  // now copy the structure to the union skeleton iun this class
  this->copyGramMapSkeleton(this->globalIndex->filterTreeRoot->gramMap,
			    this->gramMapSkeleton);

  // cleanup
  delete this->globalIndex;
  this->globalIndex = NULL;
  free(this->rnds);
  this->rnds = NULL;
};

#endif
