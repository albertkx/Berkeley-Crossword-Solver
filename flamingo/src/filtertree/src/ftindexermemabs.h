/*
  $Id: ftindexermemabs.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftindexermemabs_h_
#define _ftindexermemabs_h_

#include "ftindexerabs.h"
#include "gramlistsimple.h"
#include "util/src/debug.h"

#include <stdint.h>

template<class FtIndexerConcrete>
class FtIndexerMemAbs;

template<class FtIndexerConcrete>
struct FtIndexerTraits<FtIndexerMemAbs<FtIndexerConcrete> > {
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
};

template<class FtIndexerConcrete>
class FtIndexerMemAbs 
  : public FtIndexerAbs<FtIndexerMemAbs<FtIndexerConcrete> > {
  
public:
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
  
protected:  
  typedef 
  FtIndexerAbs<FtIndexerMemAbs<FtIndexerConcrete> >
  SuperClass;  
  
  // insert string into index (and not into stringcontainer) with a given id
  void insertStringIntoIndex(const string& str, unsigned stringId);
  
  void addStringToLeaf(FilterTreeNode<InvList>* node, const string& s, const unsigned stringId);
  
  // used for reading/writing index from/to file, 
  // using logical separation into functions so compressed indexers can use these, too
  void saveLeavesRec(FilterTreeNode<InvList>* node, ofstream& fpOut);
  void loadLeavesRec(FilterTreeNode<InvList>* node, ifstream& fpIn);

public:
  typedef typename SuperClass::GramMap GramMap;

  FtIndexerMemAbs(StringContainer* strContainer) : SuperClass(strContainer) {}
  FtIndexerMemAbs(StringContainer* strContainer, 
		  GramGen* gramGenerator, 
		  const unsigned maxStrLen = 150, 
		  const unsigned ftFanout = 50) 
    : SuperClass(strContainer, gramGenerator, maxStrLen, ftFanout) {}
  
  void buildIndex_Impl(const string& dataFile, const unsigned linesToRead = 0);
  void buildIndex_Impl();
  
  // insert string into stringcontainer and into index
  void insertString_Impl(const string& str);
  
  // clear all inverted lists
  void clearInvertedLists();  
  
  void integrateUpdates_Impl();

  // here are the statically resolved interfaces that a concrete indexer MUST implement
  // we use the CRTP design pattern
  void prepare() {
    static_cast<FtIndexerConcrete*>(this)->prepare_Impl();
  }

  void addStringId(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId) {
    static_cast<FtIndexerConcrete*>(this)->addStringId_Impl(leaf, gramCodes, stringId);
  }

  void saveIndex(const char* indexFileName) {
    static_cast<FtIndexerConcrete*>(this)->saveIndex_Impl(indexFileName);
  }
  
  void loadIndex(const char* indexFileName) {
    static_cast<FtIndexerConcrete*>(this)->loadIndex_Impl(indexFileName);
  }
  
  CompressionArgs* getCompressionArgs() { 
    return static_cast<FtIndexerConcrete*>(this)->getCompressionArgs_Impl(); 
  }
  
  string getName() { 
    return "FtIndexerMemAbs"; 
  }
  
  fstream* getInvListsFile_Impl() { 
    return NULL;
  }
  
  void printFilterTree(FilterTreeNode<InvList>* node = NULL);
};

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
integrateUpdates_Impl() {
  if(this->deletedStringIds.size() > 0) {
    unsigned numStrings = this->strContainer->size();
    unsigned* stringIdMapping = new unsigned[numStrings];
    for(unsigned i = 0; i < numStrings; i++) stringIdMapping[i] = i;
    
    this->strContainer->integrateUpdates(this->deletedStringIds, stringIdMapping);
    
    // brute force updating, scan all inverted lists and remap the string ids
    vector<FilterTreeNode<InvList>* > leaves;
    FilterTreeNode<InvList>::getSubTreeLeaves(this->filterTreeRoot, leaves);
    
    for(unsigned i = 0; i < leaves.size(); i++) {
      unordered_set<uintptr_t> processedInvLists;
      
      typename SuperClass::GramMap& gramMap = leaves[i]->gramMap;
      typename SuperClass::GramMap::iterator iter;
      vector<unsigned> emptyInvLists;	
      for(iter = gramMap.begin(); iter != gramMap.end(); iter++) {
	InvList* invList = iter->second->getArray();
	uintptr_t invListAddr = (uintptr_t)invList;
	
	// account for potentially combined lists (several grams point to same physical list)
	if(processedInvLists.find(invListAddr) == processedInvLists.end()) {
	  InvList newInvList;
	  for(unsigned j = 0; j < invList->size(); j++) {
	    if(stringIdMapping[(*invList)[j]] != 0xFFFFFFFF) {
	      newInvList.push_back(stringIdMapping[(*invList)[j]]);
	    }	    
	  }
	  
	  if(newInvList.size() > 0) std::swap(*invList, newInvList);
	  else emptyInvLists.push_back(iter->first);
	  
	  processedInvLists.insert(invListAddr);
	}	
      }
      
      // remove empty inverted lists from gram map
      for(unsigned i = 0; i < emptyInvLists.size(); i++) {
	delete gramMap[emptyInvLists[i]];
	gramMap.erase(emptyInvLists[i]);
      }
    }
    
    delete[] stringIdMapping;
    this->deletedStringIds.clear();
  }
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
insertString_Impl(const string& str) {
  unsigned stringId = this->strContainer->insertString(str);
  insertStringIntoIndex(str, stringId);
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
insertStringIntoIndex(const string& str, unsigned stringId) {
  if(str.empty()) return;  
  FilterTreeNode<InvList>* homeLeaf = this->findHomeLeafNode(str);
  if(homeLeaf) this->addStringToLeaf(homeLeaf, str, stringId);
} 

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
addStringToLeaf(FilterTreeNode<InvList>* node, const string& s, const unsigned stringId) {
  // add stringId to distinctStringIDs and create the GramListSimple instance if neccessary
  if(!node->distinctStringIDs) node->distinctStringIDs = new GramListSimple<InvList>();
  node->distinctStringIDs->getArray()->push_back(stringId);
  
  // add stringId to gramMap of leaf
  vector<unsigned> gramCodes;
  this->gramGen->decompose(s, gramCodes);    
  addStringId(node, gramCodes, stringId);
}

template<class FtIndexerConcrete> 
void
FtIndexerMemAbs<FtIndexerConcrete>:: 
buildIndex_Impl(const string& dataFile, const unsigned linesToRead) {  
  this->strContainer->fillContainer(dataFile.c_str(), linesToRead, this->maxStrLen);
  this->buildIndex();
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
buildIndex_Impl() {
  
#ifdef DEBUG_STAT  
  this->ixStats->dictSize = this->strContainer->size();
  this->ixStats->gramLen = this->gramGen->getGramLength();
  this->ixStats->maxStrLen = this->maxStrLen;
  this->ixStats->ftFanout = this->ftFanout;
  this->ixStats->partFilters = this->filterTypes.size();
  this->sutil.startTimeMeasurement();
#endif
  
  // prepare compressor for building index
  prepare();
  
  // build empty filtertree
  this->buildHollowTreeRecursive(this->filterTreeRoot, 0);
  
  TIMER_START("INSERTING INTO INDEX", this->strContainer->size());
  for(unsigned stringId = 0; stringId < this->strContainer->size(); stringId++) {
    string currentString;
    this->strContainer->retrieveString(currentString, stringId);
    insertStringIntoIndex(currentString, stringId);
    TIMER_STEP();
  }
  TIMER_STOP();
    
#ifdef DEBUG_STAT
  this->sutil.stopTimeMeasurement();
  this->ixStats->buildTime = this->sutil.getTimeMeasurement();
#endif
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
clearInvertedLists() {
  vector<FilterTreeNode<InvList>*> leaves;
  FilterTreeNode<InvList>::getSubTreeLeaves(this->filterTreeRoot, leaves);
  for(unsigned i = 0; i < leaves.size(); i++) {
    GramMap& gramMap = leaves[i]->gramMap;
    typename GramMap::iterator iter;
    for(iter = gramMap.begin(); iter != gramMap.end(); iter++) delete iter->second;
    gramMap.clear();
  }
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
saveLeavesRec(FilterTreeNode<InvList>* node, ofstream& fpOut) {
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
    
    // write gram map
    GramMap& gramMap = node->gramMap;
    u = gramMap.size();
    fpOut.write((const char*)&u, sizeof(unsigned));
    for(typename GramMap::iterator iter = gramMap.begin();     
	iter != gramMap.end();
	iter++) {
      
      u = iter->first;
      fpOut.write((const char*)&u, sizeof(unsigned));
      
      InvList* arr = iter->second->getArray();
      if(arr) {
	u = arr->size();
	fpOut.write((const char*)&u, sizeof(unsigned));
	for(unsigned i = 0; i < arr->size(); i++) {
	  u = arr->at(i);
	  fpOut.write((const char*)&u, sizeof(unsigned));
	}
      }
      else {
	u = 0;
	fpOut.write((const char*)&u, sizeof(unsigned));
      }      
    }    
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      saveLeavesRec(children[i].node, fpOut);
  }
}

template<class FtIndexerConcrete> 
void 
FtIndexerMemAbs<FtIndexerConcrete>::
loadLeavesRec(FilterTreeNode<InvList>* node, ifstream& fpIn) {
  if(node->isLeaf()) {
    if(node->distinctStringIDs) delete node->distinctStringIDs;
    unsigned distinctStrings;
    fpIn.read((char*)&distinctStrings, sizeof(unsigned));   
    
    if(distinctStrings > 0) node->distinctStringIDs = new GramListSimple<InvList>();
    for(unsigned i = 0; i < distinctStrings; i++) {
      unsigned tmp;
      fpIn.read((char*)&tmp, sizeof(unsigned));
      node->distinctStringIDs->getArray()->push_back(tmp);
    }    
    
    GramMap& gramMap = node->gramMap;
    unsigned gramMapSize;
    unsigned gramCode;
    unsigned arrSize;
    fpIn.read((char*)&gramMapSize, sizeof(unsigned));
    for(unsigned j = 0; j < gramMapSize; j++) {
      fpIn.read((char*)&gramCode, sizeof(unsigned));
      fpIn.read((char*)&arrSize, sizeof(unsigned));
      
      if(arrSize != 0) {	
	GramListSimple<InvList>* gramList = new GramListSimple<InvList>();
	gramMap[gramCode] = gramList;
	InvList* newArr = gramList->getArray();
	for(unsigned i = 0; i < arrSize; i++) {
	  unsigned tmp;
	  fpIn.read((char*)&tmp, sizeof(unsigned));
	  newArr->push_back(tmp);
	}
      }
    }
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      loadLeavesRec(children[i].node, fpIn);
  } 
}

template<class FtIndexerConcrete> 
void
FtIndexerMemAbs<FtIndexerConcrete>::
printFilterTree(FilterTreeNode<InvList>* node) {
  if(!node) {
    printFilterTree(this->filterTreeRoot);
    return;
  }
  
  if(node->isLeaf()) {
    GramMap& gramMap = node->gramMap;
    
    for(typename GramMap::iterator iter = gramMap.begin();
	iter != gramMap.end();
	iter++) {
      
      InvList* arr = iter->second->getArray();
      cout << iter->first << " ";
      if(arr) {
	for(unsigned i = 0; i < arr->size(); i++)
	  cout << arr->at(i) << " ";
      }
      cout << endl;
    }
  }
  else {
    vector<pair<unsigned, FilterTreeNode<InvList>*> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++) {
      cout << children[i].first << endl;
      printFilterTree(children[i].second);
    }
  }
}

#endif
