/*
  $Id: ftindexermem.h 5786 2010-10-22 04:24:20Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftindexeruncompressed_h_
#define _ftindexeruncompressed_h_

#include "ftindexermemabs.h"

template <class StringContainer, class InvList>
class FtIndexerMem;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerMem<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template <class StringContainer = StringContainerVector, class InvList = vector<unsigned> >
class FtIndexerMem 
  : public FtIndexerMemAbs<FtIndexerMem<StringContainer, InvList> > { 

private:
  typedef 
  FtIndexerMemAbs<FtIndexerMem<StringContainer, InvList> > 
  SuperClass;
  
public:
  typedef typename SuperClass::GramMap GramMap;

#ifdef DEBUG_STAT
  typedef IndexStats IxStatsType;
#endif

  FtIndexerMem(StringContainer* strContainer) : SuperClass(strContainer) {
#ifdef DEBUG_STAT
    this->ixStats = new IndexStats();
#endif
  }
  
  FtIndexerMem(StringContainer* strContainer,
	       GramGen* gramGenerator, 
	       unsigned maxStrLen = 150, 
	       unsigned maxChildNodes = 50) 
    : SuperClass(strContainer, gramGenerator, maxStrLen, maxChildNodes) {
#ifdef DEBUG_STAT
    this->ixStats = new IndexStats();
#endif    
  }

  void prepare_Impl() {};    
  void addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId);  
  CompressionArgs* getCompressionArgs_Impl() { return &(this->compressionArgs); }
  
  void saveIndex_Impl(const char* indexFileName);
  void loadIndex_Impl(const char* indexFileName);

  string getName() {
    return "FtIndexerMem";
  } 
};

template<class StringContainer, class InvList>
void 
FtIndexerMem<StringContainer, InvList>::
addStringId_Impl(FilterTreeNode<InvList>* leaf, const vector<unsigned>& gramCodes, unsigned stringId) {
  typename SuperClass::GramMap& gramMap = leaf->gramMap;
  
  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes[i];

    if (gramMap.find(gramCode) == gramMap.end()) {
      // a new gram
      GramListSimple<InvList>* newGramList = new GramListSimple<InvList>();
      gramMap[gramCode] = newGramList;
      newGramList->getArray()->push_back(stringId);
    }
    else {
      // an existing gram
      GramList<InvList>* gramList = gramMap[gramCode];
      // avoid adding duplicate elements
      if(gramList->getArray()->back() != stringId)
	gramList->getArray()->push_back(stringId);
    }	        
  }
}

template<class StringContainer, class InvList> 
void 
FtIndexerMem<StringContainer, InvList>::
saveIndex_Impl(const char* indexFileName) {
  ofstream fpOut;
  fpOut.open(indexFileName, ios::out);
  if(fpOut.is_open()) {
    this->saveBasicInfo(fpOut);
    this->saveLeavesRec(this->filterTreeRoot, fpOut);    
    fpOut.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;
}

template<class StringContainer, class InvList> 
void 
FtIndexerMem<StringContainer, InvList>::
loadIndex_Impl(const char* indexFileName) {
  ifstream fpIn;
  fpIn.open(indexFileName, ios::in);
  if(fpIn.is_open()) {
    this->filterTypes.clear();
    this->loadBasicInfo(fpIn);  
    this->buildHollowTreeRecursive(this->filterTreeRoot, 0);  
    this->loadLeavesRec(this->filterTreeRoot, fpIn);  
    fpIn.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;
}

#endif
