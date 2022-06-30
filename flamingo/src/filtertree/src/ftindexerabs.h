/*
  $Id: ftindexerabs.h 5786 2010-10-22 04:24:20Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/17/2007
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _ftindexerabs_h_
#define _ftindexerabs_h_

#include <cmath>

#include "filtertreenode.h"
#include "statsutil.h"
#include "gramlist.h"
#include "gramlistsimple.h"
#include "stringcontainer.h"
#include "common/src/filtertypes.h"
#include "common/src/gramgen.h"
#include "common/src/compressionargs.h"

enum FtIndexerType {
  UNCOMPRESSED,
  HOLES_GLOBAL_LLF,
  HOLES_GLOBAL_SLF,
  HOLES_GLOBAL_RANDOM,
  HOLES_GLOBAL_TIMECOST,
  HOLES_GLOBAL_PANICCOST,
  ONDISK
};

template<class FtIndexerConcrete>
struct FtIndexerTraits;

template<class FtIndexerConcrete> 
class FtIndexerAbs {
public:
  typedef FtIndexerTraits<FtIndexerConcrete> TraitsType;
  typedef typename TraitsType::StringContainer StringContainer;
  typedef typename TraitsType::InvList InvList;
  
protected:
#ifdef DEBUG_STAT
  IndexStats* ixStats;
  StatsUtil sutil;
#endif
  
  typedef typename tr1::unordered_map<unsigned, GramList<InvList>* > GramMap;
  
  // string ids marked as deleted
  unordered_set<unsigned> deletedStringIds;
  
  void buildHollowTreeRecursive(FilterTreeNode<InvList>* node, unsigned filterId);
  void deleteFilterTreeRec(FilterTreeNode<InvList>* node);
  
  void setPostMergeFilter(PostMergeFilter pmf);  
  
  FilterTreeNode<InvList>* findHomeLeafNode(const string& str);
  
  // for saving/loading indexes
  void saveBasicInfo(ofstream& fpOut);
  void loadBasicInfo(ifstream& fpIn);
  
public:
  FtIndexerType indexerType;
  GramGen* gramGen;
  CompressionArgs compressionArgs;  
  unsigned maxStrLen;
  unsigned ftFanout;
  bool indexLoaded;
  
  StringContainer* strContainer;
  vector<AbstractFilter*> filterTypes;
  FilterTreeNode<InvList>* filterTreeRoot;

  PostMergeFilter pmf;  
  
  // for charsum post-merge filter
  CharsumFilter* charsumFilter;
  StrCharsumStats* charsumStats;

  // for letter count post-merge filter
  unsigned lcCharNum;
  unsigned char* lcChars;
  StrLcStats* dataLcStats;

  // for hash count post-merge filter
  unsigned hcBuckets;
  StrHcStats* dataHcStats;
  
  FtIndexerAbs(StringContainer* strContainer, PostMergeFilter pmf = PMF_NONE);
  FtIndexerAbs(StringContainer* strContainer, 
	       GramGen* gramGenerator, 
	       unsigned maxStrLen = 150, 
	       unsigned ftFanout = 50,
	       PostMergeFilter pmf = PMF_NONE);
  
  void buildIndex(const string& dataFile, unsigned linesToRead = 0) {
    static_cast<FtIndexerConcrete*>(this)->buildIndex_Impl(dataFile, linesToRead);
  }
  
  void buildIndex() {
    static_cast<FtIndexerConcrete*>(this)->buildIndex_Impl();
  }
  
  // insert string into stringcontainer and into index
  void insertString(const string& str) {
    static_cast<FtIndexerConcrete*>(this)->insertString_Impl(str);
  }

  // mark string id as deleted (this id won't appear in answers, but the inverted lists still tontain it)
  void deleteString(unsigned stringId) { deletedStringIds.insert(stringId); }   
  
  // remove string ids marked as deleted from inverted lists and from stringcontainer, and re-assign all string ids
  // WARNING: this procedure will not retain the PhysOrd of strings if any was specified
  void integrateUpdates() {
    static_cast<FtIndexerConcrete*>(this)->integrateUpdates_Impl();
  }

  void autoAddPartFilter();
  void addPartFilter(const AbstractFilter* filter);
  void clearPartFilters();
    
  GramGen* getGramGen() const { return gramGen; }
  void deleteFilterTree() {
    deleteFilterTreeRec(filterTreeRoot);
    filterTreeRoot = NULL;    
  } 
  
  // should be implemented by all indexers, too
  string getName() { 
    return "FtIndexerAbs"; 
  }
  
  // for compressed indexes only
  CompressionArgs* getCompressionArgs() { 
    return static_cast<FtIndexerConcrete*>(this)->getCompressionArgs_Impl(); 
  }

  // for disk-based indexes
  fstream* getInvListsFile() { 
    return static_cast<FtIndexerConcrete*>(this)->getInvListsFile_Impl();
  }
    
  FtIndexerType getType() { return indexerType; }
  
  const unordered_set<unsigned>& getDeletedStringIds() { return deletedStringIds; }

#ifdef DEBUG_STAT
  IndexStats* getIndexStats() { return ixStats; }
#endif

  ~FtIndexerAbs() {
    deleteFilterTree();
    clearPartFilters();
    if(charsumStats) delete[] charsumStats;
    if(charsumFilter) delete charsumFilter;
    if(lcChars) delete[] lcChars;
    if(dataLcStats) delete[] dataLcStats;
    if(dataHcStats) delete[] dataHcStats;
    if(indexLoaded) delete gramGen;
#ifdef DEBUG_STAT
    if(this->ixStats) delete this->ixStats;
#endif
  }  
};

template<class FtIndexerConcrete> 
FilterTreeNode<typename FtIndexerAbs<FtIndexerConcrete>::InvList>*
FtIndexerAbs<FtIndexerConcrete>::
findHomeLeafNode(const string& str) {

  if(str.empty()) return NULL;

  // position currentNode to appropriate leaf node in filter tree      
  FilterTreeNode<InvList>* currentNode = this->filterTreeRoot; 
  for(unsigned filterId = 0; filterId < this->filterTypes.size(); filterId++) {
    unsigned key = this->filterTypes[filterId]->getKey(str, this->gramGen);
    KeyNodePair<InvList> knp(key, NULL);	
    typename vector<KeyNodePair<InvList> >::iterator iter = 
      lower_bound(currentNode->children.begin(), currentNode->children.end(), knp);
    if(iter != currentNode->children.end()) currentNode = iter->node;	
  }
  
  return currentNode;
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
buildHollowTreeRecursive(FilterTreeNode<InvList>* node, unsigned filterId) {

  // special case: no partitioning filters used, use root as leaf
  if(filterTypes.size() == 0) return;
  
  unsigned filterLbound, filterUbound;
  AbstractFilter* filter = filterTypes[filterId];
  filterLbound = filter->getFilterLbound();
  filterUbound = filter->getFilterUbound();
  //if(filter->getType() == FT_CHARSUM) filterUbound = 15600; // google
  //if(filter->getType() == FT_CHARSUM) filterUbound = 24900; // pubmed
  double step = (filterUbound - filterLbound) / (double)ftFanout;
  for(unsigned i = 0; i < ftFanout; i++) {
    FilterTreeNode<InvList>* newNode = new FilterTreeNode<InvList>();
    unsigned key = (unsigned)round((i+1)*step);
    node->children.push_back( KeyNodePair<InvList>(key, newNode) );
    if(filterId+1 < filterTypes.size())
      buildHollowTreeRecursive(newNode, filterId+1);
  }
}

// used for reading/writing index from/to file, 
// using logical separation into functions so compressed indexers can use these, too
template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
saveBasicInfo(ofstream& fpOut) {
  unsigned u;
  
  // write primitive information
  fpOut.write((const char*)&this->maxStrLen, sizeof(unsigned));
  fpOut.write((const char*)&this->ftFanout, sizeof(unsigned));
  
  // write charsum info
  fpOut.write((const char*)&this->pmf, sizeof(PostMergeFilter));
  switch(this->pmf) {
  case PMF_CSF_REG:
  case PMF_CSF_OPT: {
    unsigned contSize = strContainer->size();
    fpOut.write((const char*)&contSize, sizeof(unsigned));
    fpOut.write((const char*)this->charsumStats, sizeof(StrCharsumStats) * contSize);
  } break;

  case PMF_LC: {    
    fpOut.write((const char*)&this->lcCharNum, sizeof(unsigned));
    if(this->lcCharNum > 0) {
      fpOut.write((const char*)this->lcChars, this->lcCharNum);
      unsigned contSize = strContainer->size();    
      fpOut.write((const char*)&contSize, sizeof(unsigned));
      for(unsigned i = 0; i < strContainer->size(); i++) {
	fpOut.write((const char*)&this->dataLcStats[i].length, sizeof(unsigned short));
	fpOut.write((const char*)&this->dataLcStats[i].charFreqs, this->lcCharNum);
      }
    }
  } break;

  default: break;
    
  }

  // write deleted string ids
  u = deletedStringIds.size();
  fpOut.write((const char*)&u, sizeof(unsigned));
  for(unordered_set<unsigned>::iterator iter = deletedStringIds.begin();
      iter != deletedStringIds.end();
      iter++) {
    u = *iter;
    fpOut.write((const char*)&u, sizeof(unsigned));
  }
  
  // write gramgenerator
  this->gramGen->saveGramGenInstance(fpOut);
  
  // write filterTypes
  u = this->filterTypes.size();
  fpOut.write((const char*)&u, sizeof(unsigned));
  for(unsigned i = 0; i < this->filterTypes.size(); i++) {
    AbstractFilter* filter = this->filterTypes[i];
    filter->saveFilterInstance(fpOut);
  }  
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
loadBasicInfo(ifstream& fpIn) {
  unsigned nFilters;
  
  fpIn.read((char*)&this->maxStrLen, sizeof(unsigned));
  fpIn.read((char*)&this->ftFanout, sizeof(unsigned));
  
  // load charsum info
  fpIn.read((char*)&this->pmf, sizeof(PostMergeFilter));  
  if(this->pmf != PMF_NONE) {        
    switch(this->pmf) {
      
    case PMF_CSF_REG: {
      unsigned contSize = 0;
      fpIn.read((char*)&contSize, sizeof(unsigned));
      if(this->charsumStats) delete[] this->charsumStats;
      this->charsumStats = new StrCharsumStats[contSize];
      fpIn.read((char*)this->charsumStats, sizeof(StrCharsumStats) * contSize);
      this->charsumFilter = new CharsumFilter((unsigned*)NULL);
    } break;
      
    case PMF_CSF_OPT: {
      unsigned contSize = 0;
      fpIn.read((char*)&contSize, sizeof(unsigned));
      if(this->charsumStats) delete[] this->charsumStats;
      this->charsumStats = new StrCharsumStats[contSize];
      fpIn.read((char*)this->charsumStats, sizeof(StrCharsumStats) * contSize);
      if(strContainer->getStatsCollector()) {
	this->charsumFilter = new CharsumFilter(strContainer->getStatsCollector()->getCharFreqs());
      }
      else {
	cout << "WARNING: charsum filter disabled because stringcontainer has no stats" << endl;	
	this->charsumFilter = new CharsumFilter((unsigned*)NULL);
      }
    } break;
      
    case PMF_LC: {
      fpIn.read((char*)&this->lcCharNum, sizeof(unsigned));
      if(lcCharNum > 0) {
	this->lcChars = new unsigned char[this->lcCharNum];
	fpIn.read((char*)this->lcChars, this->lcCharNum);

	unsigned contSize = 0;
	fpIn.read((char*)&contSize, sizeof(unsigned));
	this->dataLcStats = new StrLcStats[contSize];
	for(unsigned i = 0; i < contSize; i++) {
	  fpIn.read((char*)&this->dataLcStats[i].length, sizeof(unsigned short));
	  this->dataLcStats[i].charFreqs = new unsigned char[this->lcCharNum];
	  fpIn.read((char*)& this->dataLcStats[i].charFreqs, this->lcCharNum);	  
	}
      }
    } break;
      
    default: {
      charsumFilter = NULL;
      charsumStats = NULL;
      lcChars = NULL;
      dataLcStats = NULL;
      dataHcStats = NULL;
    } break;
      
    }
  }
  else {
    charsumFilter = NULL;
    charsumStats = NULL;
    lcChars = NULL;
    dataLcStats = NULL;
    dataHcStats = NULL;
  }
  
  // read deleted string ids
  deletedStringIds.clear();
  unsigned size;
  fpIn.read((char*)&size, sizeof(unsigned));
  for(unsigned i = 0; i < size; i++) {
    unsigned tmp;
    fpIn.read((char*)&tmp, sizeof(unsigned));
    deletedStringIds.insert(tmp);
  }
  
  // load gramgenerator
  this->gramGen = GramGen::loadGramGenInstance(fpIn);  
  
  // load filters
  fpIn.read((char*)&nFilters, sizeof(unsigned));
  for(unsigned i = 0; i < nFilters; i++) {
    AbstractFilter* newFilter = AbstractFilter::loadFilterInstance(fpIn);
    this->filterTypes.push_back(newFilter);
  }

  this->indexLoaded = true;
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
setPostMergeFilter(PostMergeFilter pmf) {
  this->pmf = pmf;
  if(strContainer->size() > 0) {
    switch(pmf) {
      
    case PMF_CSF_REG: {
      charsumFilter = new CharsumFilter((unsigned*)NULL);
      charsumStats = new StrCharsumStats[strContainer->size()];
    } break;
      
    case PMF_CSF_OPT: {
      if(strContainer->getStatsCollector()) {
	charsumFilter = new CharsumFilter(strContainer->getStatsCollector()->getCharFreqs());
	charsumStats = new StrCharsumStats[strContainer->size()];
      }
      else {
	cout << "WARNING: charsum filter disabled because stringcontainer has no stats" << endl;
	charsumFilter = new CharsumFilter((unsigned*)NULL);
	charsumStats = new StrCharsumStats[strContainer->size()];
      }
    } break;
      
    case PMF_HC: {
      dataHcStats = new StrHcStats[strContainer->size()];
    } break;

    default: {
      charsumFilter = NULL;
      charsumStats = NULL;
      lcChars = NULL;
      dataLcStats = NULL;
      dataHcStats = NULL;
    } break;      
      
    }
  }
  else {
    charsumFilter = NULL;
    charsumStats = NULL;
    lcChars = NULL;
    dataLcStats = NULL;
    dataHcStats = NULL;
  }
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
deleteFilterTreeRec(FilterTreeNode<InvList>* node) {
  if(!node) return;
  if(!node->isLeaf()) {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++) {
      deleteFilterTreeRec(children[i].node);
    }
  }
  delete node;
}

template<class FtIndexerConcrete> 
FtIndexerAbs<FtIndexerConcrete>::
FtIndexerAbs(StringContainer* strContainer, PostMergeFilter pmf)
  : gramGen(NULL), maxStrLen(0), ftFanout(0), indexLoaded(false),
    strContainer(strContainer), filterTreeRoot(new FilterTreeNode<InvList>()) {
#ifdef DEBUG_STAT
  this->ixStats = NULL;
#endif
  charsumFilter = NULL;
  charsumStats = NULL;
  lcChars = NULL;
  dataLcStats = NULL;
  dataHcStats = NULL;  
  setPostMergeFilter(pmf);
}

template<class FtIndexerConcrete> 
FtIndexerAbs<FtIndexerConcrete>::
FtIndexerAbs(StringContainer* strContainer, GramGen* gramGen, unsigned maxStrLen, unsigned ftFanout, PostMergeFilter pmf) 
  : gramGen(gramGen), maxStrLen(maxStrLen), ftFanout(ftFanout), indexLoaded(false),
    strContainer(strContainer), filterTreeRoot(new FilterTreeNode<InvList>()) {
#ifdef DEBUG_STAT
  this->ixStats = NULL;
#endif
  charsumFilter = NULL;
  charsumStats = NULL;
  lcChars = NULL;
  dataLcStats = NULL;
  dataHcStats = NULL;
  setPostMergeFilter(pmf);
}

template<class FtIndexerConcrete> 
void
FtIndexerAbs<FtIndexerConcrete>::
autoAddPartFilter() {
  
  clearPartFilters();
  
  StatsCollector* statsCollector = strContainer->getStatsCollector();
  if(statsCollector) {
    FilterStats* bestFilterStats = statsCollector->getBestPartFilter();
    AbstractFilter* bestFilter = NULL;

    switch(bestFilterStats->getFilterType()) {
      
    case FT_LENGTH: {
      bestFilter = new LengthFilter(maxStrLen);
    } break;
      
    case FT_CHARSUM: {
      bestFilter = new CharsumFilter(maxStrLen);
    } break;

    case FT_NONE: break;    
      
    }
    
    if(bestFilter) {
      bestFilter->adjustBounds(bestFilterStats->getMinKey(), bestFilterStats->getMaxKey(), ftFanout);
      addPartFilter(bestFilter);    
    }
  }
  else {
    cout << "WARNING: no partitioning filter set because container has no stats" << endl;    
  }
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
addPartFilter(const AbstractFilter* filter) {
  for(unsigned i = 0; i < filterTypes.size(); i++) {
    AbstractFilter* tmp = filterTypes[i];
    if(filter->getType() == tmp->getType()) return;
  }
  filterTypes.push_back((AbstractFilter*)filter);   
}

template<class FtIndexerConcrete> 
void 
FtIndexerAbs<FtIndexerConcrete>::
clearPartFilters() {
  for(unsigned i = 0; i < filterTypes.size(); i++) {
    AbstractFilter* tmp = filterTypes[i];
    if(tmp) delete tmp;
  }
  filterTypes.clear();
}

#endif
