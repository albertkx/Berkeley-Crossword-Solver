/*
  $Id: ftindexerondisk.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _diskindexer_h_
#define _diskindexer_h_

#include "ftindexerabs.h"
#include "ftindexermem.h"
#include "gramlistondisk.h"
#include "util/src/misc.h"

#include <stdint.h>
#include <fstream>
#include <tr1/unordered_map>
#include <cstdlib>
#include <cstring>

template <class StringContainer, class InvList>
class FtIndexerOnDisk;

template<class TStringContainer, class TInvList>
struct FtIndexerTraits<FtIndexerOnDisk<TStringContainer, TInvList> > {
  typedef TStringContainer StringContainer;
  typedef TInvList InvList;
};

template<class StringContainer = StringContainerRM, class InvList = vector<unsigned> >
class FtIndexerOnDisk 
  : public FtIndexerAbs<FtIndexerOnDisk<StringContainer, InvList> > {
  
protected:
  typedef struct {
    unsigned gramCode;
    unsigned numberElements;
    char* data;
  } PartialList;
  
  typedef 
  FtIndexerAbs<FtIndexerOnDisk<StringContainer, InvList> > 
  SuperClass;
  
  typedef typename SuperClass::GramMap GramMap;
  typedef pair<unsigned, GramListOnDisk<InvList>*> GramListOnDiskPair;
  
  bool static cmpGramListOnDiskPair(const GramListOnDiskPair& a, const GramListOnDiskPair& b) {
    return a.second->startOffset < b.second->startOffset;
  }

  bool static randCmpFilterTreeNode(FilterTreeNode<InvList>* a, FilterTreeNode<InvList>* b) {
    return (uintptr_t)a % 666 > (uintptr_t)b % 666;
  }
  
  bool disableStreamBuffer;
  unsigned maxBytesPerRun;
  string invListsFileName;
  unsigned totalPartialLists;
  streampos indexSize;
  tr1::unordered_map<unsigned, unsigned> totalGramListSizes;
  static vector<unsigned> stringid2leafid;
  bool scatteredOrg;
  
  void flushRun(FtIndexerMem<StringContainerVector, InvList>& memIndexer, fstream& fpOut);
  void mergeRuns(unsigned numberRuns, GramMap& targetMap, fstream& fpIn, fstream& fpOut);
  
  void reorganizeIndex(GramMap& sourceMap, vector<FilterTreeNode<InvList>* >& leaves, fstream& fpIndex);
  void reorganizeIndexFlush(vector<pair<char*, unsigned> >& dataToFlush, fstream& fpIndex, streampos& nextWriteOffset);
  
  void scatterIndex(vector<FilterTreeNode<InvList>* >& leaves, fstream& srcIndex);
  
  // for letter count filter, used to determine standard deviation of character frequencies in dataset
  void aggCharStdev(string& s, unsigned* charFreqs, float numberStrings, float* stdevAgg);
  
  // comparison function for letter count filter
  static bool cmpLcPair(pair<unsigned, float> a, pair<unsigned, float> b) { return a.second > b.second; }

  // for saving/loading index
  void saveFtIndexerOnDiskInfo(ofstream& fpOut);
  void loadFtIndexerOnDiskInfo(ifstream& fpIn);
  void saveLeavesRec(FilterTreeNode<InvList>* node, ofstream& fpOut);
  void loadLeavesRec(FilterTreeNode<InvList>* node, ifstream& fpIn);

public:  
#ifdef DEBUG_STAT
  typedef OnDiskIndexStats IxStatsType;
#endif

  // insert string into stringcontainer and into index
  void insertString_Impl(const string& str) {
    cout << "INSERT CURRENTLY NOT SUPPORTED FOR DISK-BASED INDEXES" << endl;
  }

  // mapping from hashed gram to gram string, for experiments with positional information
  unordered_map<unsigned, string> gramcode2gram;
  void addStringToMap(string& currentString);
  
  FtIndexerOnDisk(StringContainer* strContainer, PostMergeFilter pmf = PMF_NONE) 
: SuperClass(strContainer, pmf), disableStreamBuffer(false), indexSize(0) {
    this->indexerType = ONDISK;
#ifdef DEBUG_STAT
    this->ixStats = new OnDiskIndexStats();
#endif
  }
  
  FtIndexerOnDisk(StringContainer* strContainer, 
		  GramGen* gramGen, 
		  bool disableStreamBuffer = false,
		  const string& invListsFileName = "invlists.ix",
		  const unsigned maxBytesPerRun = 5000000,
		  const unsigned maxStrLen = 150,
		  const unsigned ftFanout = 50,
		  PostMergeFilter pmf = PMF_NONE,
		  unsigned lcCharNum = 0, 
		  unsigned hcBuckets = 0,
		  bool scatteredOrg = false) 
    : SuperClass(strContainer, gramGen, maxStrLen, ftFanout, pmf),
      disableStreamBuffer(disableStreamBuffer), maxBytesPerRun(maxBytesPerRun),
      invListsFileName(invListsFileName), indexSize(0), scatteredOrg(scatteredOrg) {
    this->indexerType = ONDISK;
    this->lcCharNum = lcCharNum;
    this->hcBuckets = hcBuckets;
#ifdef DEBUG_STAT
    this->ixStats = new OnDiskIndexStats();
#endif
  }

  void buildIndex_Impl(const string& dataFile, const unsigned linesToRead = 0);  
  void buildIndex_Impl();
  
  bool checkCorrectness(FtIndexerMem<StringContainer, InvList>* memIndexer, const string& invListsFileName);  
  
  void saveIndex(const char* indexFileName);
  void loadIndex(const char* indexFileName);
  
  fstream invListsFile;
  void openInvListsFile() {
    if(invListsFile.is_open()) invListsFile.close();
    if(disableStreamBuffer) invListsFile.rdbuf()->pubsetbuf(0, 0);
    invListsFile.open(invListsFileName.c_str(), ios::in | ios::out | ios::binary);
  }
  
  CompressionArgs* getCompressionArgs_Impl() { return &(this->compressionArgs); }
  fstream* getInvListsFile_Impl() { return &invListsFile; }
  
  string getName() { return "FtIndexerOnDisk"; }
  
  ~FtIndexerOnDisk() {
    invListsFile.close();
  }  
};

template<class StringContainer, class InvList>
void 
FtIndexerOnDisk<StringContainer, InvList>::
aggCharStdev(string& s, unsigned* charFreqs, float numberStrings, float* stdevAgg) {
  unsigned localCharFreqs[256];
  memset(localCharFreqs, 0, sizeof(unsigned) * 256);
  
  for(unsigned i = 0; i < s.size(); i++)
    localCharFreqs[(unsigned)(unsigned char)s[i]]++;
  
  for(unsigned i = 0; i < s.size(); i++) {
    unsigned c = (unsigned)(unsigned char)s[i];
    float mean = (float)charFreqs[c] / numberStrings;
    float diff = (mean > localCharFreqs[c]) ? mean - localCharFreqs[c] : localCharFreqs[c] - mean;
    stdevAgg[c] += diff * diff;
  }  
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
addStringToMap(string& currentString) {
  vector<unsigned> gramCodes;
  vector<string> grams;
  
  this->gramGen->decompose(currentString, gramCodes);
  this->gramGen->decompose(currentString, grams);

  if(gramCodes.size() != grams.size()) cout << "ERROR: GRAM SETS ARE UNEQUAL SIZE" << endl;
  
  for(unsigned i = 0; i < grams.size(); i++) {
    unsigned gramCode = gramCodes.at(i);
    string& gram = grams.at(i);
    if(gramcode2gram.find(gramCode) != gramcode2gram.end()) {
      string& s = gramcode2gram[gramCode];
      if(s.size() > 0) {
	if(s.compare(gram) != 0) {
	  s.clear();
	}
      }
    }
    else gramcode2gram[gramCode] = gram;
  }
}

template<class StringContainer, class InvList>
vector<unsigned> FtIndexerOnDisk<StringContainer, InvList>::stringid2leafid;

// functions that depend on the type of the inverted list element, e.g. unsigned
template<class InvList>
class InvListSpecializer {
public:
  typedef typename tr1::unordered_map<unsigned, GramList<InvList>* > GramMap;
  typedef pair<unsigned, GramListOnDisk<InvList>*> GramListOnDiskPair;
  
  static vector<unsigned>* stringid2leafid;
  
  static int cmp(const void* a, const void* b) {    
    unsigned x = *((unsigned*)a);
    unsigned y = *((unsigned*)b);
    if(stringid2leafid->at(x) > stringid2leafid->at(y) || x > y) return 1;
    if(stringid2leafid->at(x) < stringid2leafid->at(y) || x < y) return -1;
    return 0;
  }  
  
  static void updateLeafOffsets(GramListOnDiskPair& currentPair,
				pair<char*, unsigned>& writeMe, 
				GramListOnDisk<InvList>* gl,
				vector<FilterTreeNode<InvList>* >& leaves) {    
    
    // need to update offsets of filtertree leaves
    unsigned* list = (unsigned*)writeMe.first;
    unsigned sortedSubListSize = 1;
    streampos sortedSubListOffset = gl->startOffset;
    unsigned prevLeaf = stringid2leafid->at(list[0]); // set to first value
    for(unsigned j = 1; j < gl->listSize; j++) {      
      unsigned currentLeaf = stringid2leafid->at(list[j]);      
      if( currentLeaf != prevLeaf ) {
	// update the grammap in appropriate leaf
	GramMap& gramMap = leaves.at(prevLeaf)->gramMap;
	if(gramMap.find(currentPair.first) == gramMap.end())
	  gramMap[currentPair.first] = new GramListOnDisk<InvList>(sortedSubListSize, sortedSubListOffset);  
	prevLeaf = currentLeaf;
	sortedSubListSize = 1;
	sortedSubListOffset = gl->startOffset + (streampos)(j * sizeof(typename InvList::value_type));
      }
      else sortedSubListSize++;
    }
    GramMap& gramMap = leaves.at(prevLeaf)->gramMap;
    if(gramMap.find(currentPair.first) == gramMap.end())
      gramMap[currentPair.first] = new GramListOnDisk<InvList>(sortedSubListSize, sortedSubListOffset);
  }  
};
template<class InvList> 
vector<unsigned>* InvListSpecializer<InvList>::stringid2leafid;

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
saveIndex(const char* indexFileName) {
  ofstream fpOut;
  fpOut.open(indexFileName, ios::out);
  if(fpOut.is_open()) {
    this->saveBasicInfo(fpOut);
    this->saveFtIndexerOnDiskInfo(fpOut);
    this->saveLeavesRec(this->filterTreeRoot, fpOut);    
    fpOut.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;  
}

template<class StringContainer, class InvList> 
void 
FtIndexerOnDisk<StringContainer, InvList>::
loadIndex(const char* indexFileName) {
  ifstream fpIn;
  fpIn.open(indexFileName, ios::in);
  if(fpIn.is_open()) {
    this->filterTypes.clear();  
    this->loadBasicInfo(fpIn);  
    this->loadFtIndexerOnDiskInfo(fpIn);
    this->buildHollowTreeRecursive(this->filterTreeRoot, 0);
    this->loadLeavesRec(this->filterTreeRoot, fpIn);  
    fpIn.close();
  }
  else cout << "ERROR: could not open file " << indexFileName << endl;  
  openInvListsFile();
}

template<class StringContainer, class InvList> 
void 
FtIndexerOnDisk<StringContainer, InvList>::
saveFtIndexerOnDiskInfo(ofstream& fpOut) {
  unsigned size = invListsFileName.size();
  fpOut.write((const char*)&size, sizeof(unsigned));
  fpOut.write((const char*)invListsFileName.c_str(), size);  

  fpOut.write((const char*)&maxBytesPerRun, sizeof(unsigned));  
  fpOut.write((const char*)&totalPartialLists, sizeof(unsigned));  
  
  size = gramcode2gram.size();
  fpOut.write((const char*)&size, sizeof(unsigned));  
  for(unordered_map<unsigned, string>::iterator iter = gramcode2gram.begin();
      iter != gramcode2gram.end();
      iter++) {
    unsigned gramCode = iter->first;
    string& gram = iter->second;
    fpOut.write((const char*)&gramCode, sizeof(unsigned));
    size = gram.size();
    fpOut.write((const char*)&size, sizeof(unsigned));
    if(size > 0) fpOut.write((const char*)gram.c_str(), size);
  }  
}

template<class StringContainer, class InvList> 
void 
FtIndexerOnDisk<StringContainer, InvList>::
saveLeavesRec(FilterTreeNode<InvList>* node, ofstream& fpOut) {
  unsigned u;
  
  if(node->isLeaf()) {
    if(node->distinctStringIDs) {
      GramListOnDisk<vector<unsigned> >* gl = dynamic_cast<GramListOnDisk<vector<unsigned> >*>(node->distinctStringIDs);
      fpOut.write((const char*)&gl->listSize, sizeof(unsigned));
      fpOut.write((const char*)&gl->startOffset, sizeof(streampos));
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
      GramListOnDisk<InvList>* gl = dynamic_cast<GramListOnDisk<InvList>*>(iter->second);
      fpOut.write((const char*)&gl->listSize, sizeof(unsigned));
      fpOut.write((const char*)&gl->startOffset, sizeof(streampos));      
    }    
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      saveLeavesRec(children.at(i).node, fpOut);
  }
}

template<class StringContainer, class InvList> 
void 
FtIndexerOnDisk<StringContainer, InvList>::
loadLeavesRec(FilterTreeNode<InvList>* node, ifstream& fpIn) {
  if(node->isLeaf()) {
    if(node->distinctStringIDs) delete node->distinctStringIDs;
    unsigned distinctStrings;
    fpIn.read((char*)&distinctStrings, sizeof(unsigned));
    if(distinctStrings > 0) {
      streampos startOffset;
      fpIn.read((char*)&startOffset, sizeof(streampos));      
      node->distinctStringIDs = new GramListOnDisk<vector<unsigned> >(distinctStrings, startOffset);
    }
    
    GramMap& gramMap = node->gramMap;
    unsigned gramMapSize;
    unsigned gramCode;
    unsigned listSize;
    streampos startOffset;
    fpIn.read((char*)&gramMapSize, sizeof(unsigned));    
    for(unsigned j = 0; j < gramMapSize; j++) {
      fpIn.read((char*)&gramCode, sizeof(unsigned));
      fpIn.read((char*)&listSize, sizeof(unsigned));      
      fpIn.read((char*)&startOffset, sizeof(streampos));      
      gramMap[gramCode] = new GramListOnDisk<InvList>(listSize, startOffset);
    }
  }
  else {
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      loadLeavesRec(children.at(i).node, fpIn);
  } 
}

template<class StringContainer, class InvList> 
void 
FtIndexerOnDisk<StringContainer, InvList>::
loadFtIndexerOnDiskInfo(ifstream& fpIn) {
  unsigned size = 0;

  fpIn.read((char*)&size, sizeof(unsigned));
  char* invListData = new char[size];
  fpIn.read(invListData, size);
  invListsFileName.assign(invListData, size);
  delete[] invListData;
  
  fpIn.read((char*)&maxBytesPerRun, sizeof(unsigned));  
  fpIn.read((char*)&totalPartialLists, sizeof(unsigned));
  
  fpIn.read((char*)&size, sizeof(unsigned));
  gramcode2gram.clear();
  for(unsigned i = 0; i < size; i++) {
    unsigned gramCode = 0;
    unsigned gramSize = 0;
    string gram;
    fpIn.read((char*)&gramCode, sizeof(unsigned));
    fpIn.read((char*)&gramSize, sizeof(unsigned));
    if(gramSize > 0) {
      char* gramTempData = new char[gramSize];
      fpIn.read(gramTempData, gramSize);
      gram.assign(gramTempData, gramSize);
      delete[] gramTempData;
    }
    gramcode2gram[gramCode] = gram;
  }
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
buildIndex_Impl(const string& dataFile, 
		const unsigned linesToRead) {
  
  this->strContainer->fillContainer(dataFile.c_str(), linesToRead, this->maxStrLen);  
  this->buildIndex();
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
buildIndex_Impl() {

  int sysret;
  
#ifdef DEBUG_STAT
  
  sysret = system("sync");
  sysret = system("echo 3 > /proc/sys/vm/drop_caches");

  this->ixStats->dictSize = this->strContainer->size();
  this->ixStats->gramLen = this->gramGen->getGramLength();
  this->ixStats->maxStrLen = this->maxStrLen;
  this->ixStats->ftFanout = this->ftFanout;
  this->ixStats->partFilters = this->filterTypes.size();
  dynamic_cast<OnDiskIndexStats*>(this->ixStats)->runBufferSize = this->maxBytesPerRun;
  this->sutil.startTimeMeasurement();
#endif  

  totalPartialLists = 0;
  
  string tempDataFileName;
  if(!getRandomFileName(tempDataFileName)) {
    cout << "ERROR: could not get random file name for creating runs" << endl;
    return;
  }
  
  fstream fpRunFile;
  if(disableStreamBuffer) fpRunFile.rdbuf()->pubsetbuf(0, 0);
  fpRunFile.open(tempDataFileName.c_str(), ios::in | ios::out | ios::trunc | ios::binary);

  // build empty filtertree  
  this->buildHollowTreeRecursive(this->filterTreeRoot, 0);
  vector<FilterTreeNode<InvList>* > leaves;
  FilterTreeNode<InvList>::getSubTreeLeaves(this->filterTreeRoot, leaves);
  
  // if there are filters we need the stringid2leafid vector and a map
  tr1::unordered_map<uintptr_t, unsigned> leafAddr2leafid;
  if(this->filterTypes.size() > 0) {
    stringid2leafid.resize(this->strContainer->size());
    for(unsigned i = 0; i < leaves.size(); i++) {
      uintptr_t leafAddr = (uintptr_t)leaves.at(i);
      leafAddr2leafid[leafAddr] = i;
    }
  }
  
  // buffer inverted lists in an in-memory indexer, do not duplicate filter tree
  StringContainerVector emptyStrContainer;
  FtIndexerMem<StringContainerVector, InvList> 
    memIndexer(&emptyStrContainer, this->gramGen, this->maxStrLen, this->ftFanout);
  memIndexer.buildIndex();

  // remember number of elements in memIndexer
  unsigned currentBytes = 0;  
  unsigned numberRuns = 0;

  // prepare for letter count filter if neccessary
  float* stdevAgg = NULL;
  unsigned* charFreqs = NULL;  
  if(this->pmf == PMF_LC) {
    if(this->strContainer->getStatsCollector()) charFreqs = this->strContainer->getStatsCollector()->getCharFreqs();
    else cout << "ERROR: CANNOT INIT LETTER COUNT FILTER. STRCOLLECTION HAS NO STATS" << endl;
    
    stdevAgg = new float[256];
    for(unsigned i = 0; i < 256; i++) stdevAgg[i] = 0.0f;
  }
  
  // 1. add strings to memIndexer until maxBytesPerRun is reached  
  // 2. flush in memory inverted lists to disk, creating a run
  // 3. merge runs to create final index
  // 4. if specified, reorganize index to reflect partitioning filter
  TIMER_START("CREATING RUNS", this->strContainer->size());
  for(unsigned i = 0; i < this->strContainer->size(); i++) {
    
    string currentString;
    this->strContainer->retrieveString(currentString, i);
    
    // gather information for post-merge filtering
    switch(this->pmf) {
    case PMF_CSF_REG:
    case PMF_CSF_OPT: {
      this->charsumStats[i].length = currentString.size();
      this->charsumFilter->fillCharsumStats(currentString, 
					    &this->charsumStats[i].charsum, 
					    &this->charsumStats[i].lChar, 
					    &this->charsumStats[i].hChar, 
					    1);

    } break;

    case PMF_LC: {
      aggCharStdev(currentString, charFreqs, this->strContainer->size(), stdevAgg);
    } break;
      
    case PMF_HC: {
      vector<unsigned> gramCodes;
      this->gramGen->decompose(currentString, gramCodes);
      this->dataHcStats[i].set(gramCodes, this->hcBuckets);
    } break;
      
    default: break;

    }
    
    // check if we need to update the stringid2leaf vector
    if(this->filterTypes.size() > 0) {
      uintptr_t leafAddr = (uintptr_t)this->findHomeLeafNode(currentString);
      stringid2leafid[i] = leafAddr2leafid[leafAddr];
    }
    
    unsigned bytes = this->gramGen->getNumGrams(currentString) * sizeof(typename InvList::value_type);
    if(currentBytes + bytes >= maxBytesPerRun) {
      // flush run to disk, and clear invertedLists
      flushRun(memIndexer, fpRunFile);
      numberRuns++;
      memIndexer.clearInvertedLists();
      currentBytes = 0;
    }
    
    //addStringToMap(currentString);
    
    // insert string to in-memory indexer and stringrm
    memIndexer.insertString(currentString);
    
    currentBytes += bytes;
    TIMER_STEP();
  }
  // at the end always flush last run
  flushRun(memIndexer, fpRunFile);
  numberRuns++;
  memIndexer.clearInvertedLists();
  TIMER_STOP();

#ifdef DEBUG_STAT
  this->sutil.stopTimeMeasurement();
  dynamic_cast<OnDiskIndexStats*>(this->ixStats)->createRunsTime = this->sutil.getTimeMeasurement();
  this->sutil.startTimeMeasurement();  
#endif
  
  // are we using any filters?
  if(this->filterTypes.size() > 0) {    
    
    // merge runs into a temporary map (a global index without filters)
    GramMap tempMap;
    fstream fpIndex;
    if(disableStreamBuffer) fpIndex.rdbuf()->pubsetbuf(0, 0);
    if(scatteredOrg) fpIndex.open("scatter.tmp", ios::in | ios::out | ios::trunc | ios::binary);  
    else fpIndex.open(invListsFileName.c_str(), ios::in | ios::out | ios::trunc | ios::binary);
    mergeRuns(numberRuns, tempMap, fpRunFile, fpIndex);
    fpRunFile.close();
    
#ifdef DEBUG_STAT
    this->sutil.stopTimeMeasurement();
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->mergeRunsTime = this->sutil.getTimeMeasurement();
    this->sutil.startTimeMeasurement();
#endif
    
    // reorganize index to reflect that filters are being used
    reorganizeIndex(tempMap, leaves, fpIndex);
    
    // scatter sublists
    if(scatteredOrg) scatterIndex(leaves, fpIndex);
    
    fpIndex.close();
    
    // cleanup tempMap
    for(typename GramMap::iterator iter = tempMap.begin(); iter != tempMap.end(); iter++)
      delete iter->second;

#ifdef DEBUG_STAT
    this->sutil.stopTimeMeasurement();
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->reorgTime = this->sutil.getTimeMeasurement();
#endif
  }
  else {
    // we are not using a partitioning filter, so just merge runs
    fstream fpIndex;
    if(disableStreamBuffer) fpIndex.rdbuf()->pubsetbuf(0, 0);
    fpIndex.open(invListsFileName.c_str(), ios::in | ios::out | ios::trunc | ios::binary);  
    mergeRuns(numberRuns, this->filterTreeRoot->gramMap, fpRunFile, fpIndex);
    fpIndex.close();
    fpRunFile.close();
    
#ifdef DEBUG_STAT
    this->sutil.stopTimeMeasurement();
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->mergeRunsTime = this->sutil.getTimeMeasurement();
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->reorgTime = 0.0;
#endif
  }
  
  // cleanup 
  if(this->filterTypes.size() > 0) stringid2leafid.clear();
  totalGramListSizes.clear();
  
  // finish and cleanup letter count filter
  if(this->pmf == PMF_LC) {
    // compute stdev, and find characters with highest freq * stdev
    vector<pair<unsigned, float> > pairs;
    for(unsigned i = 0; i < 256; i++) {
      stdevAgg[i] = sqrt((stdevAgg[i] / this->strContainer->size()));
      pair<unsigned, float> p(i, stdevAgg[i] * charFreqs[i]);
      pairs.push_back(p);
    }
    sort(pairs.begin(), pairs.end(), FtIndexerOnDisk<StringContainer, InvList>::cmpLcPair);
    
    // set the highest ranking chars
    this->lcChars = new unsigned char[this->lcCharNum];
    for(unsigned i = 0; i < this->lcCharNum; i++)
      this->lcChars[i] = (unsigned char)pairs.at(i).first;
    
    this->dataLcStats = new StrLcStats[this->strContainer->size()];
    
    // scan the datasets again and set string stats
    TIMER_START("CREATING LETTER COUNT FILTER", this->strContainer->size());
    for(unsigned i = 0; i < this->strContainer->size(); i++) {
      string str;
      this->strContainer->retrieveString(str, i);
      this->dataLcStats[i].set(str, this->lcCharNum, this->lcChars);
      TIMER_STEP();
    }
    TIMER_STOP();
    
    delete[] stdevAgg;
  }
  
  // delete temp files
  // WARNING: this is OS dependent, don't know how to delete the temp file by other means...
  char cmd[tempDataFileName.size() + 5];
  sprintf(cmd, "rm %s", tempDataFileName.c_str());
  int i;
  i = system(cmd);
  
  if(scatteredOrg) {
    char cmd[15];
    sprintf(cmd, "rm %s", "scatter.tmp");
    int i;
    i = system(cmd);
  }
  
  openInvListsFile();
    
#ifdef DEBUG_STAT
  dynamic_cast<OnDiskIndexStats*>(this->ixStats)->indexSize = indexSize;
  this->ixStats->buildTime = 
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->createRunsTime +
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->mergeRunsTime +
    dynamic_cast<OnDiskIndexStats*>(this->ixStats)->reorgTime;
#endif

  //unsigned ftSize = FilterTreeNode<InvList>::getSubTreeSize(this->filterTreeRoot);
  //cout << "FILTERTREESIZE: " << ftSize << endl;

}

template<class StringContainer, class InvList>
bool
FtIndexerOnDisk<StringContainer, InvList>::
checkCorrectness(FtIndexerMem<StringContainer, InvList>* memIndexer, const string& invListsFileName) {
    
  ifstream fpIn;
  fpIn.open(invListsFileName.c_str(), ios::in | ios::binary);
  
  vector<FilterTreeNode<InvList>*> memLeaves;
  FilterTreeNode<InvList>::getSubTreeLeaves(memIndexer->filterTreeRoot, memLeaves);
  
  vector<FilterTreeNode<InvList>* > diskLeaves;
  FilterTreeNode<InvList>::getSubTreeLeaves(this->filterTreeRoot, diskLeaves);
  
  if(memLeaves.size() != diskLeaves.size()) {
    cout << "NUMBER OF LEAVES NOT EQUAL!" << endl;
    return false;
  }
  
  for(unsigned leaf = 0; leaf < memLeaves.size(); leaf++) {
    
    // if filters are used then also check the distinctstringids
    if(this->filterTypes.size() > 0) {
      GramListOnDisk<>* diskGl = dynamic_cast<GramListOnDisk<>* >(diskLeaves.at(leaf)->distinctStringIDs);
      GramListSimple<>* memGl = dynamic_cast<GramListSimple<>* >(memLeaves.at(leaf)->distinctStringIDs);
      
      // only enter if any of the lists is unequal to NULL
      if(diskGl != NULL || memGl != NULL) {
	
	// check if both lists are unequal to null
	if( (diskGl == NULL && memGl != NULL) || (diskGl != NULL && memGl == NULL) ) {
	  cout << "ERROR IN COMPARING DISTINCTSTRINGIDS, ONE LIST IS NULL" << endl;
	  cout << "LEAF: " << leaf << endl;
	  return false;
	}
	
	// check size of lists
	if( diskGl->listSize != memGl->getArray()->size() ) {
	  cout << "ERROR IN COMPARING DISTINCTSTRINGIDS, LISTS HAVE UNEQUAL SIZE" << endl;
	  cout << "MEMLISTSIZE: " << memGl->getArray()->size() << " - DISKLISTSIZE: " << diskGl->listSize << endl;
	  cout << "LEAF: " << leaf << endl;
	  return false;
	}
	
	// read disklist	
	fpIn.seekg(diskGl->startOffset);
	unsigned bytes = diskGl->listSize * sizeof(unsigned);
	char* buffer = new char[bytes];
	fpIn.read(buffer, bytes);
	
	// check elements
	unsigned* diskArr = (unsigned*)buffer;
	vector<unsigned>* memArr = memGl->getArray();
	for(unsigned i = 0; i < diskGl->listSize; i++) {
	  if(memArr->at(i) != diskArr[i]) {
	    cout << "ERROR IN COMPARING DISTINCTSTRINGIDS, UNEQUAL ELEMENTS" << endl;
	    cout << "LEAF: " << leaf << endl;
	    for(unsigned j = 0; j < memArr->size(); j++) {
	      cout << memArr->at(j) << " " << diskArr[j] << endl;
	    }
	    return false;
	  }
	}
	
	delete[] buffer;
      }    
    }
    
    // iterate through mem lists and compare with disk lists
    typename FilterTreeNode<InvList>::GramMap& memGramMap = memLeaves.at(leaf)->gramMap;
    GramMap& diskGramMap = diskLeaves.at(leaf)->gramMap;    
    typename FilterTreeNode<InvList>::GramMap::iterator iter;
    for(iter = memGramMap.begin(); iter != memGramMap.end(); iter++) {
      GramList<InvList>* gl = iter->second;
      InvList* memArr = gl->getArray();
      
      // retrieve disk array
      if(diskGramMap.find(iter->first) == diskGramMap.end()) {
	cout << "NOT SAME GRAM LIST" << endl;
	cout << "LEAF: " << leaf << endl;
	cout << "GRAM: " << iter->first << endl;
	return false;
      }
      
      GramListOnDisk<InvList>* diskList = dynamic_cast<GramListOnDisk<InvList>*>(diskGramMap[iter->first]);
      fpIn.seekg(diskList->startOffset);
      unsigned bytes = diskList->listSize * sizeof(typename InvList::value_type);
      char* buffer = new char[bytes];
      fpIn.read(buffer, bytes);      
      
      if(diskList->listSize != memArr->size()) {
	cout << "ARRAYS NOT SAME SIZE" << endl;
	cout << "LEAF: " << leaf << endl;
	cout << "GRAM: " << iter->first << endl;
	cout << diskList->listSize << " " << memArr->size() << endl;
	return false;
      }
      
      typename InvList::value_type* diskArr = (typename InvList::value_type*) buffer;
      for(unsigned i = 0; i < memArr->size(); i++) {
	
	// compare elements
	if(memArr->at(i) != diskArr[i]) {
	  cout << "FAILURE!" << endl;
	  cout << "LEAF: " << leaf << endl;
	  cout << "GRAM: " << iter->first << endl;
	  for(unsigned j = 0; j < memArr->size(); j++) {
	    cout << memArr->at(j) << " " << diskArr[j] << endl;
	  }
	  return false;
	}
      }
      
      delete[] buffer;
    }    
  }
  
  fpIn.close();
  
  return true;
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
flushRun(FtIndexerMem<StringContainerVector, InvList>& memIndexer, fstream& fpOut) {
  typename FilterTreeNode<InvList>::GramMap& gramMap = memIndexer.filterTreeRoot->gramMap;
  typename FilterTreeNode<InvList>::GramMap::iterator iter;
  unsigned numberPartialLists = gramMap.size();
  totalPartialLists += numberPartialLists;
  fpOut.write((const char*)&numberPartialLists, sizeof(unsigned));
  for(iter = gramMap.begin(); iter != gramMap.end(); iter++) {
    unsigned gramCode = iter->first;
    GramList<InvList>* gl = iter->second;
    InvList* arr = gl->getArray();
    fpOut.write((const char*)&gramCode, sizeof(unsigned));
    unsigned size = arr->size();
    fpOut.write((const char*)&size, sizeof(unsigned));
    const char* listData = (const char*)&arr->at(0);
    fpOut.write(listData, size * sizeof(typename InvList::value_type));
    if(totalGramListSizes.find(gramCode) == totalGramListSizes.end()) totalGramListSizes[gramCode] = size;
    else totalGramListSizes[gramCode] += size;
  }
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
mergeRuns(unsigned numberRuns, GramMap& targetMap, fstream& fpIn, fstream& fpOut) {
  
  tr1::unordered_map<unsigned, streampos> nextListOffsets;
  streampos nextOffset = 0; // next offset to write a sublist which is the FIRST sublist of a whole list
  
  // create empty file with correct size and remember list offsets
  // if we have filters activated then keep space for every distinct stringid at the beginning of the file
  // the stringids will be ordered and written in the reorganize step, here we just leave space
  if(this->filterTypes.size() > 0) {
    unsigned dummy = 0;
    for(unsigned i = 0; i < this->strContainer->size(); i++)
      fpOut.write((const char*)&dummy, sizeof(unsigned));
    nextOffset = fpOut.tellp();
  }
  // create rest of empty file which holds the inverted lists
  tr1::unordered_map<unsigned, unsigned>::iterator iter;
  for(iter = totalGramListSizes.begin(); iter != totalGramListSizes.end(); iter++) {
    unsigned bytes = iter->second * sizeof(typename InvList::value_type);
    char* dummy = new char[bytes];
    memset(dummy, 0, bytes);
    fpOut.write(dummy, bytes);
    delete[] dummy;
  }  
  this->indexSize = fpOut.tellp(); // record total size
  
  int sysret;
  sysret = system("sync");
  sysret = system("echo 3 > /proc/sys/vm/drop_caches");
  
  // go through runs and create final index  
  fpIn.seekg(0);
  TIMER_START("MERGING RUNS", totalPartialLists);
  for(unsigned run = 0; run < numberRuns; run++) {
    unsigned numberPartialLists;
    fpIn.read((char*)&numberPartialLists, sizeof(unsigned));
    
    // buffer the lists
    vector<PartialList*> partialLists;
    vector<PartialList*> completeLists;
    PartialList* allLists = new PartialList[numberPartialLists];
    
    // read partial list data
    for(unsigned list = 0; list < numberPartialLists; list++) {
      fpIn.read((char*)&allLists[list].gramCode, sizeof(unsigned));
      fpIn.read((char*)&allLists[list].numberElements, sizeof(unsigned));
      unsigned bytes = allLists[list].numberElements * sizeof(typename InvList::value_type);
      allLists[list].data = new char[bytes];
      fpIn.read(allLists[list].data, bytes);
      
      // decide in which vector to put the partial list
      if(allLists[list].numberElements == totalGramListSizes[allLists[list].gramCode])
	completeLists.push_back(&allLists[list]);
      else 
	partialLists.push_back(&allLists[list]);
    }
        
    // write partial lists
    // we need to pay some random IOs, because we might have to append to a previously written sublist
    for(unsigned list = 0; list < partialLists.size(); list++) {
      PartialList* pList = partialLists.at(list);
      unsigned gramCode = pList->gramCode;
      streampos writeOffset = 0;
      if(nextListOffsets.find(gramCode) == nextListOffsets.end()) {
	writeOffset = nextOffset;
	nextOffset += totalGramListSizes[gramCode] * sizeof(typename InvList::value_type);
	targetMap[gramCode] = new GramListOnDisk<InvList>(totalGramListSizes[gramCode], writeOffset);
      }
      else writeOffset = nextListOffsets[gramCode];	
      
      // determine list size in bytes and seek to writing position
      unsigned bytes = pList->numberElements * sizeof(typename InvList::value_type);
      fpOut.seekp(writeOffset);
      fpOut.write(pList->data, bytes);
      writeOffset += bytes;
      nextListOffsets[gramCode] = writeOffset; 
      delete[] pList->data;
      TIMER_STEP();
    }
    
    // write complete lists in one sequential write
    fpOut.seekp(nextOffset);
    for(unsigned list = 0; list < completeLists.size(); list++) {
      PartialList* cList = completeLists.at(list);
      streampos writeOffset = fpOut.tellp();
      // materialize a new gramlistondisk and remember size and starting offset of inverted list
      targetMap[cList->gramCode] = new GramListOnDisk<InvList>(cList->numberElements, writeOffset);
      unsigned bytes = cList->numberElements * sizeof(typename InvList::value_type);
      fpOut.write(cList->data, bytes);
      delete[] cList->data;	  	
      TIMER_STEP();
    }
    nextOffset = fpOut.tellp();	
    delete[] allLists;
  }
  TIMER_STOP();
}

// this is required if filters are used
// the big inverted lists are split into several sorted sublists reflecting the filters used
// we must remember the appropriate offsets and set them in the filter tree leaves
template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
reorganizeIndex(GramMap& sourceMap, vector<FilterTreeNode<InvList>* >& leaves, fstream& fpIndex) {
  int sysret;
  sysret = system("sync");
  sysret = system("echo 3 > /proc/sys/vm/drop_caches");

  fpIndex.seekp(0);
  fpIndex.seekg(0);
  
  // handle the distinctstringids at the beginning of the file
  // 1. we need to sort the distinctstringids by the leafid they belong to
  // 2. we need to create a GramListOnDisk instance for every leaf that has at least one stringid
  // 3. we need set the size and the starting offset for the sorted sublist (the distinctstringids of one leaf)
  unsigned* distinctIDs = new unsigned[this->strContainer->size()];
  for(unsigned i = 0; i < this->strContainer->size(); i++) distinctIDs[i] = i;
  InvListSpecializer<vector<unsigned> >::stringid2leafid = &stringid2leafid;
  qsort(distinctIDs, 
	this->strContainer->size(), 
	sizeof(unsigned), 
	InvListSpecializer<vector<unsigned> >::cmp);
  fpIndex.write( (const char*)distinctIDs, this->strContainer->size() * sizeof(unsigned) );
  fpIndex.clear();
  
  // update offsets of filtertree leaves
  unsigned sortedSubListSize = 1;
  streampos sortedSubListOffset = 0; // here we always start at the beginning of the file
  unsigned prevLeaf = stringid2leafid[distinctIDs[0]]; // set to first value      
  for(unsigned j = 1; j < this->strContainer->size(); j++) {
    unsigned currentLeaf = stringid2leafid[distinctIDs[j]];
    if( currentLeaf != prevLeaf ) {
      // update the grammap in appropriate leaf
      leaves.at(prevLeaf)->distinctStringIDs = 
	new GramListOnDisk<vector<unsigned> >(sortedSubListSize, sortedSubListOffset);
      prevLeaf = currentLeaf;
      sortedSubListSize = 1;
      sortedSubListOffset = (streampos)(j * sizeof(unsigned));
    }
    else sortedSubListSize++;
  }
  leaves.at(prevLeaf)->distinctStringIDs = 
    new GramListOnDisk<vector<unsigned> >(sortedSubListSize, sortedSubListOffset);
  delete[] distinctIDs;
  
  // we want to read the file sequentially so we must sort the GramListOnDisks by startOffset
  vector<GramListOnDiskPair> sortedGramLists;
  for(typename GramMap::iterator iter = sourceMap.begin(); iter != sourceMap.end(); iter++) {
    GramListOnDiskPair p;
    p.first = iter->first;
    p.second = dynamic_cast<GramListOnDisk<InvList>*>(iter->second);
    sortedGramLists.push_back(p);
  }
  sort(sortedGramLists.begin(), sortedGramLists.end(), FtIndexerOnDisk::cmpGramListOnDiskPair);  
  
  // sequentially read every list, reorganize the list and determine offsets for filtertree leaves
  // write the modified lists ONLY when buffer space is exceeded (at least one complete list must be read though)  
  unsigned currentBytes = 0;
  streampos nextWriteOffset = (streampos)(this->strContainer->size() * sizeof(unsigned));
  fpIndex.seekp(nextWriteOffset);
  fpIndex.seekg(nextWriteOffset);
  vector<pair<char*, unsigned> > dataToFlush;
  TIMER_START("REORGANIZING INDEX", sortedGramLists.size());
  for(unsigned i = 0; i < sortedGramLists.size(); i++) {
    GramListOnDiskPair& currentPair = sortedGramLists.at(i);
    GramListOnDisk<InvList>* gl = currentPair.second;
    
    // check if we have exceeded available memory
    unsigned bytes = gl->listSize * sizeof(typename InvList::value_type);
    if(currentBytes + bytes >= maxBytesPerRun) {
      reorganizeIndexFlush(dataToFlush, fpIndex, nextWriteOffset);
      currentBytes = 0;
      nextWriteOffset = fpIndex.tellp();
      fpIndex.seekg(gl->startOffset);
    }
    
    pair<char*, unsigned> writeMe; // first points fo the data, second indicates the data size in bytes
    writeMe.second = gl->listSize * sizeof(typename InvList::value_type);
    writeMe.first = new char[writeMe.second];
    fpIndex.read(writeMe.first, writeMe.second);   

    // sort the inverted list by leafid that stringid belongs to in the filtertree WITH filters
    // for some reason unknown to me the STL sort function could not handle this, here is the snippet
    // unsigned* begin = (unsigned*)writeMe.first;
    // unsigned* end = begin + gl->listSize;
    // sort(begin, end, FtIndexerOnDisk::cmpInvListElements);
    InvListSpecializer<InvList>::stringid2leafid = &stringid2leafid;
    qsort((void*)writeMe.first, 
	  gl->listSize, 
	  sizeof(typename InvList::value_type),
	  InvListSpecializer<InvList>::cmp);
    
    // update the offsets in the filter tree leaf nodes, this function is specialized by the type of inverted list
    InvListSpecializer<InvList>::updateLeafOffsets(currentPair, writeMe, gl, leaves);
    
    dataToFlush.push_back(writeMe);      
    currentBytes += bytes;
    TIMER_STEP();
  }
  reorganizeIndexFlush(dataToFlush, fpIndex, nextWriteOffset);
  TIMER_STOP();
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
reorganizeIndexFlush(vector<pair<char*, unsigned> >& dataToFlush, fstream& fpIndex, streampos& nextWriteOffset) {
  // flush the pending data to disk
  fpIndex.seekp(nextWriteOffset);
  for(unsigned j = 0; j < dataToFlush.size(); j++) {
    pair<char*, unsigned>& p = dataToFlush.at(j);        
    fpIndex.write(p.first, p.second);
    delete[] p.first;
  }
  dataToFlush.clear();
  fpIndex.clear();
}

template<class StringContainer, class InvList>
void
FtIndexerOnDisk<StringContainer, InvList>::
scatterIndex(vector<FilterTreeNode<InvList>* >& leaves, fstream& srcIndex) {
  
  fstream fpIndex;
  if(disableStreamBuffer) fpIndex.rdbuf()->pubsetbuf(0, 0);
  fpIndex.open(invListsFileName.c_str(), ios::in | ios::out | ios::trunc | ios::binary);  
  
  // read and write distinctStringIDs at beginning of file
  char* distinctIDs = new char[this->strContainer->size() * sizeof(unsigned)];
  srcIndex.seekg(0);
  srcIndex.read(distinctIDs, this->strContainer->size() * sizeof(unsigned));
  fpIndex.seekp(0);
  fpIndex.write(distinctIDs, this->strContainer->size() * sizeof(unsigned));
  delete[] distinctIDs;
  
  // create empty file with right size
  unsigned bigWrites = this->indexSize / maxBytesPerRun;
  unsigned lastWriteSize = this->indexSize % maxBytesPerRun;  
  for(unsigned i = 0; i < bigWrites; i++) {
    char* data = new char[maxBytesPerRun];
    fpIndex.write((const char*)data, maxBytesPerRun); 
    delete[] data;
  }
  char* data = new char[lastWriteSize];
  fpIndex.write((const char*)data, lastWriteSize);
  delete[] data;    
   
  // randomly sort leaves
  sort(leaves.begin(), leaves.end(), FtIndexerOnDisk::randCmpFilterTreeNode);  
  
  // count lists to process for monitoring progress
  unsigned listsToProcess = 0;
  for(unsigned i = 0; i < leaves.size(); i++) listsToProcess += leaves.at(i)->gramMap.size();
  
  char* buffer = new char[maxBytesPerRun];
  unsigned currentBytes = 0;
  fpIndex.seekp(this->strContainer->size() * sizeof(unsigned));
  TIMER_START("SCATTERING INDEX", listsToProcess);
  for(unsigned i = 0; i < leaves.size(); i++) {
    GramMap& gramMap = leaves.at(i)->gramMap;
    
    for(typename GramMap::iterator iter = gramMap.begin();
	iter != gramMap.end();
	iter++) {
      
      GramListOnDisk<InvList>* gl = static_cast<GramListOnDisk<InvList>* >(iter->second);
      unsigned bytes = gl->listSize * sizeof(typename InvList::value_type);
      if(currentBytes + bytes >= maxBytesPerRun) {
	// flush sublists to disk
	fpIndex.write(buffer, currentBytes);
	currentBytes = 0;
      }      
      srcIndex.seekg(gl->startOffset);
      srcIndex.read(buffer + currentBytes, bytes);
      gl->startOffset = fpIndex.tellp() + (streampos)currentBytes;
      currentBytes += bytes;	
      TIMER_STEP();
    }
    if(currentBytes > 0) {
      fpIndex.write(buffer, currentBytes);
      currentBytes = 0;
    }    
  }
  TIMER_STOP();
  
  delete[] buffer;
  
  fpIndex.close();
}

#endif
