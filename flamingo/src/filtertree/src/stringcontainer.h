/*
  $Id: stringcontainer.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _stringcontainer_h_
#define _stringcontainer_h_

#include "stringrm.h"
#include "util/src/misc.h"
#include "util/src/debug.h"
#include "common/src/filtertypes.h"
#include <math.h>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <iterator>
#include <cstring>

enum PhysOrd {
  PHO_NONE,
  PHO_AUTO,
  PHO_LENGTH,
  PHO_CHARSUM,
  PHO_LENGTH_CHARSUM,
  PHO_CHARSUM_LENGTH
};

typedef struct
{
  unsigned stringId;
  unsigned charsum;
  unsigned length;
} StringAttribs;

class FilterStats {
public:  
  unsigned minKey;
  unsigned maxKey;
  float wtedAvgValCount;
  unordered_map<unsigned, unsigned> countMap;  
  AbstractFilter* filter;
  GramGen* gramGen;
  
  FilterStats(AbstractFilter* filter);
  
  void begin();
  void next(const string& s);
  void end(float ed, unsigned strCount);

  void setGramGen(GramGen* gramGen) { this->gramGen = gramGen; }

  unsigned getMinKey() { return minKey; }
  unsigned getMaxKey() { return maxKey; }
  float getWtedAvgValCount() { return wtedAvgValCount; }
  FilterType getFilterType() { return filter->getType(); }
  
  ~FilterStats() {
    if(filter) delete filter;
  }
};

class StatsCollector {
public:  
  vector<FilterStats*> filterStats;  
  float avgStrLen;
  unsigned strCount;
  unsigned minStrLen;
  unsigned maxStrLen;
  unsigned charFreqs[256];

  GramGen* gramGen;
  
  StatsCollector() : avgStrLen(0.0f), strCount(0),
    minStrLen(0), maxStrLen(0), gramGen(NULL) {
    memset(charFreqs, 0, sizeof(unsigned) * 256);
  }
  
  void begin();
  void next(const string& s);
  void end();
  
  float getAvgStrLen() { return avgStrLen; }
  unsigned getMaxStrLen() { return maxStrLen; }
  unsigned* getCharFreqs() { return charFreqs; }
  FilterStats* getBestPartFilter();
  void clearFilterStats();
  
  void setGramGen(GramGen* gramGen) { 
    this->gramGen = gramGen; 
    for(unsigned i = 0; i < filterStats.size(); i++)
      filterStats[i]->setGramGen(gramGen);
  }
  
  ~StatsCollector();
};

// function template for creating any stringcontainer with default initialization
template<class T> 
T* createDefaultStringContainer(unsigned blockSize, unsigned avgStrLen);

template <class ConcreteStringContainer>
class StringContainerAbs {
protected:

  // collect statistical info on string collection for auto-tuning
  bool gatherStats;
  PhysOrd pho; // physical ordering of records  
  StatsCollector statsCollector; 

  // comparison function used in qsort for ordering by PHO_LENGTH
  int static cmpStringAttribsLen(const void* a, const void* b) {    
    const StringAttribs* x = static_cast<const StringAttribs* >(a);
    const StringAttribs* y = static_cast<const StringAttribs* >(b);
    if(x->length < y->length) return -1;
    if(x->length > y->length) return 1;
    return 0;
  }

  // comparison function used in qsort for ordering by PHO_CHARSUM
  int static cmpStringAttribsCharsum(const void* a, const void* b) {    
    const StringAttribs* x = static_cast<const StringAttribs* >(a);
    const StringAttribs* y = static_cast<const StringAttribs* >(b);
    if(x->charsum < y->charsum) return -1;
    if(x->charsum > y->charsum) return 1;
    return 0;
  }

  // comparison function used in qsort for ordering by PHO_LENGTH_CHARSUM  
  int static cmpStringAttribsLenCharsum(const void* a, const void* b) {    
    const StringAttribs* x = static_cast<const StringAttribs* >(a);
    const StringAttribs* y = static_cast<const StringAttribs* >(b);
    if(x->length < y->length) return -1;
    if(x->length > y->length) return 1;
    if(x->charsum < y->charsum) return -1;
    if(x->charsum > y->charsum) return 1;
    return 0;
  }
  
  // comparison function used in qsort for ordering by PHO_CHARSUM_LENGTH
  int static cmpStringAttribsCharsumLen(const void* a, const void* b) {    
    const StringAttribs* x = static_cast<const StringAttribs* >(a);
    const StringAttribs* y = static_cast<const StringAttribs* >(b);
    if(x->charsum < y->charsum) return -1;
    if(x->charsum > y->charsum) return 1;
    if(x->length < y->length) return -1;
    if(x->length > y->length) return 1;
    return 0;
  }
  
  void copyReorg(ConcreteStringContainer* tempContainer);
  
public:

  StringContainerAbs(bool gatherStats = false) : gatherStats(gatherStats), pho(PHO_NONE) {}
  StringContainerAbs(PhysOrd pho, bool gatherStats) : gatherStats(gatherStats), pho(pho) {}
  
  // interface for filling string containers
  void fillContainer(const char* fileName, const unsigned count, const unsigned maxLineLen = 300) {
    static_cast<ConcreteStringContainer*>(this)->fillContainer_Impl(fileName, count, maxLineLen);
  }
  
  // commonly used fill functions
  void fillContainer(ConcreteStringContainer* target, 
		     const char* fileName, 
		     const unsigned count, 
		     const unsigned maxLineLen = 300);

  template<typename InputIterator>
  void fillContainer(InputIterator first, InputIterator last) {
    static_cast<ConcreteStringContainer*>(this)->fillContainer_Impl(first, last);
  }
  
  // interfaces for concrete string containers
  void retrieveString(string& target, const unsigned i) {
    static_cast<ConcreteStringContainer*>(this)->retrieveString_Impl(target, i);
  }
  
  unsigned insertString(const string& s) {
    return static_cast<ConcreteStringContainer*>(this)->insertString_Impl(s);
  }

  unsigned size() {
    return static_cast<ConcreteStringContainer*>(this)->size_Impl();
  }

  void integrateUpdates(const unordered_set<unsigned>& deletedStringIds, unsigned* stringIdMapping) {
    static_cast<ConcreteStringContainer*>(this)->integrateUpdates_Impl(deletedStringIds, stringIdMapping);
  }
  
  void flushCache() {
    return static_cast<ConcreteStringContainer*>(this)->flushCache_Impl();
  }
  
  RecordID* getRecordID(unsigned i) {
    return static_cast<ConcreteStringContainer*>(this)->getRecordID_Impl(i);
  }   

  StatsCollector* getStatsCollector() { 
    if(gatherStats) return &statsCollector; 
    else return NULL;
  }
  
  void initStatsCollector(GramGen* gramGen) {
    statsCollector.setGramGen(gramGen);
  }

  PhysOrd getPhysOrd() {
    return static_cast<ConcreteStringContainer*>(this)->getPhysOrd_Impl();
  }  
};

template <class ConcreteStringContainer>
void 
StringContainerAbs<ConcreteStringContainer>::
copyReorg(ConcreteStringContainer* tempContainer) {
  
  CharsumFilter csFilter((unsigned)0);
  LengthFilter lenFilter((unsigned)0);

  // fill array of string attribs for sorting
  StringAttribs* strAttribs = new StringAttribs[tempContainer->size()];
  for(unsigned i = 0; i < tempContainer->size(); i++) {   
    string currentString;
    tempContainer->retrieveString(currentString, i);    
    strAttribs[i].charsum = csFilter.getCharsum(currentString);
    strAttribs[i].stringId = i;    
    if(statsCollector.gramGen != NULL) strAttribs[i].length = lenFilter.getKey(currentString, statsCollector.gramGen);
    else {
      cout << "WARNING: gramGen not set in statsCollector, using stringlength as filtering attribute" << endl;
      strAttribs[i].length = currentString.size();
    }
  }
  
  // sort collection
  switch(this->pho) {
    
  case PHO_NONE: break;
    
  case PHO_AUTO: {
    if(gatherStats) {
      FilterStats* bestFilterStats = statsCollector.getBestPartFilter();
      switch(bestFilterStats->getFilterType()) {

      case FT_LENGTH: {
	qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsLenCharsum);
      } break;	

      case FT_CHARSUM: {
	qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsCharsumLen);  
      } break;	

      case FT_NONE: break;    
      }
    }
    else {
      cout << "WARNING: no physical ordering set because container has no stats" << endl;
    }
  } break;
    
  case PHO_LENGTH: {
    qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsLen);  
  } break;

  case PHO_CHARSUM: {
    qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsCharsum);  
  } break;

  case PHO_LENGTH_CHARSUM: {
    qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsLenCharsum);  
  } break;

  case PHO_CHARSUM_LENGTH: {
    qsort(strAttribs, tempContainer->size(), sizeof(StringAttribs), cmpStringAttribsCharsumLen);  
  } break;

  default: {
    cout << "WARNING: invalid PhysOrd specified!" << endl;
  } break;

  }
  
  if(gatherStats) statsCollector.begin();      
  
  // copy strings into new collection
  TIMER_START("SORTING CONTAINER", tempContainer->size());
  for(unsigned i = 0; i < tempContainer->size(); i++) {  
    string currentString;
    tempContainer->retrieveString(currentString, strAttribs[i].stringId);
    if(gatherStats) statsCollector.next(currentString);
    insertString(currentString);
    TIMER_STEP();
  }
  TIMER_STOP();
  
  if(gatherStats) statsCollector.end();
  
  delete[] strAttribs;
}

template <class ConcreteStringContainer>
void
StringContainerAbs<ConcreteStringContainer>::
fillContainer(ConcreteStringContainer* target, const char* fileName, const unsigned count, const unsigned maxLineLen) {
  ifstream fileData(fileName);
  if (!fileData) {
    cout << "can't open input file \"" << fileName << "\"" << endl;
    return;
  }
  
  cout << "INPUTFILE: \"" << fileName << "\"" << endl;
  
  char line[maxLineLen + 1];
  bool isIgnore = false;
  
  if(gatherStats) statsCollector.begin();
  
  TIMER_START("FILLING CONTAINER", count);
  while (true) {
    fileData.getline(line, maxLineLen + 1);
    if (fileData.eof())
      break;
    if (fileData.rdstate() & ios::failbit) {
      isIgnore = true;
      while (fileData.rdstate() & ios::failbit) {      
	fileData.clear(fileData.rdstate() & ~ios::failbit);
	fileData.getline(line, maxLineLen);
      }
      cout << "open reading input file \"" << fileName << "\"" << endl
	   << "line length might exceed " << maxLineLen << " characters" << endl;
      return;
    }
    else {
      const string &s = string(line);
      target->insertString(s);
      if(gatherStats) statsCollector.next(s);
    }
    if (count != 0 && target->size() == count)
      break;
    TIMER_STEP();
  }
  TIMER_STOP();
  
  if(gatherStats) statsCollector.end();

  fileData.close();
  
  if (isIgnore)
    cout << "WARNING" << endl << "some lines in the file exceeded " 
	 << maxLineLen << " characters and were ignored" << endl;
  
  //cout << "CONTAINER FILLED" << endl;
  //cout << "MINSTRLEN: " << statsCollector.minStrLen << endl;
  //cout << "MAXSTRLEN: " << statsCollector.maxStrLen << endl;
  //cout << "AVGSTRLEN: " << statsCollector.avgStrLen << endl;  
  //cout << "MIN LEN:   " << statsCollector.filterStats[0]->minKey << endl;
  //cout << "MAX LEN:   " << statsCollector.filterStats[0]->maxKey << endl;

  flushCache();  
}

// very simple string container which is just a wrapper around stl vector
class StringContainerVector : public StringContainerAbs<StringContainerVector> {
private:
  vector<string> container;
  
public:  
  
  StringContainerVector(bool gatherStats = false) : StringContainerAbs<StringContainerVector>(gatherStats) { }
  StringContainerVector(PhysOrd pho, bool gatherStats) 
    : StringContainerAbs<StringContainerVector>(pho, gatherStats) {}
  
  void fillContainer_Impl(const char* fileName, const unsigned count, const unsigned maxLineLen = 300);

  void integrateUpdates_Impl(const unordered_set<unsigned>& deletedStringIds, unsigned* stringIdMapping);
  
  template<typename InputIterator>
  void fillContainer_Impl(InputIterator first, InputIterator last);

  void retrieveString_Impl(string& target, const unsigned i) { target = container[i]; }
  unsigned insertString_Impl(const string& s) { container.push_back(s); return container.size()-1; }
  unsigned size_Impl() { return container.size(); }
  void flushCache_Impl() {}
  RecordID* getRecordID_Impl(unsigned i) { cout << "WARNING: This container has no RIDs" << endl; return NULL; }
  
  PhysOrd getPhysOrd_Impl() { return PHO_NONE; }
};

template<typename InputIterator>
void 
StringContainerVector::
fillContainer_Impl(InputIterator first, InputIterator last) {
#ifdef TIMER_START
  cout << "FILLING CONTAINER..." << endl;
#endif

  if(gatherStats) statsCollector.begin();
  while(first != last) {    
    if(gatherStats) statsCollector.next(*first);
    insertString(*first);
    first++;
  }
  if(gatherStats) statsCollector.end();
}

// disk-based string container consisting of the following:
// 1. string record manager
// 2. mapping from stringID -> recordID
// the mapping is stored in memory
class StringContainerRM : public StringContainerAbs<StringContainerRM> {
private:
  vector<RecordID> ridMap;
  StringRM stringRM;
  
  unsigned blockSize;
  unsigned avgStrLen;
  char ridMapFileName[256]; // file name for ridMap
  
  void writeRidMapFile() {
    ofstream ridMapFile;
    ridMapFile.open(ridMapFileName, ios::out | fstream::trunc);
    unsigned ridMapSize = ridMap.size();
    ridMapFile.write((const char*)&ridMapSize, sizeof(unsigned));
    for(unsigned i = 0; i < ridMapSize; i++) {
      RecordID rid = ridMap[i];
      ridMapFile.write((const char*)&rid, sizeof(RecordID));
    }

    // write stats
    ridMapFile.write((const char*)&gatherStats, sizeof(bool));
    ridMapFile.write((const char*)&statsCollector.avgStrLen, sizeof(float));
    ridMapFile.write((const char*)&statsCollector.strCount, sizeof(unsigned));
    ridMapFile.write((const char*)&statsCollector.minStrLen, sizeof(unsigned));
    ridMapFile.write((const char*)&statsCollector.maxStrLen, sizeof(unsigned));
    ridMapFile.write((const char*)&statsCollector.charFreqs, 256 * sizeof(unsigned));
    
    unsigned size = statsCollector.filterStats.size();
    ridMapFile.write((const char*)&size, sizeof(unsigned));
    for(unsigned i = 0; i < size; i++) {
      FilterStats* stats = statsCollector.filterStats[i];
      stats->filter->saveFilterInstance(ridMapFile);
      ridMapFile.write((const char*)&stats->minKey, sizeof(unsigned));
      ridMapFile.write((const char*)&stats->maxKey, sizeof(unsigned));
      ridMapFile.write((const char*)&stats->wtedAvgValCount, sizeof(float));
    }
    
    ridMapFile.close();
  }

  void readRidMapFile() {
    ifstream ridMapFile;
    ridMapFile.open(ridMapFileName, ios::in);
    ridMap.clear();
    unsigned ridMapSize = 0;
    ridMapFile.read((char*)&ridMapSize, sizeof(unsigned));
    if(ridMapSize > 0) {
      for(unsigned i = 0; i < ridMapSize; i++) {
	RecordID rid;
	ridMapFile.read((char*)&rid, sizeof(RecordID));
	ridMap.push_back(rid);
      }
    }

    // read stats
    ridMapFile.read((char*)&gatherStats, sizeof(bool));
    ridMapFile.read((char*)&statsCollector.avgStrLen, sizeof(float));
    ridMapFile.read((char*)&statsCollector.strCount, sizeof(unsigned));
    ridMapFile.read((char*)&statsCollector.minStrLen, sizeof(unsigned));
    ridMapFile.read((char*)&statsCollector.maxStrLen, sizeof(unsigned));
    ridMapFile.read((char*)&statsCollector.charFreqs, 256 * sizeof(unsigned));
    
    unsigned size = 0;
    ridMapFile.read((char*)&size, sizeof(unsigned));
    statsCollector.clearFilterStats();
    for(unsigned i = 0; i < size; i++) {
      AbstractFilter* filter = AbstractFilter::loadFilterInstance(ridMapFile);
      FilterStats* stats = new FilterStats(filter);
      statsCollector.filterStats.push_back(stats);
      ridMapFile.read((char*)&stats->minKey, sizeof(unsigned));
      ridMapFile.read((char*)&stats->maxKey, sizeof(unsigned));
      ridMapFile.read((char*)&stats->wtedAvgValCount, sizeof(float));
    }

    ridMapFile.close();
  }
  
public:    
  StringContainerRM(bool gatherStats = false) : StringContainerAbs<StringContainerRM>(gatherStats) { pho = PHO_NONE; memset(ridMapFileName, 0, 256); }
  StringContainerRM(PhysOrd pho, bool gatherStats = false)
    : StringContainerAbs<StringContainerRM>(pho, gatherStats) { 
    this->pho = pho; memset(ridMapFileName, 0, 256);
  }
  
  void fillContainer_Impl(const char* fileName, const unsigned count, const unsigned maxLineLen = 300);  

  template<typename InputIterator>
  void fillContainer_Impl(InputIterator first, InputIterator last);

  void createContainer(const char* fileName, const unsigned blockSize = 4096, const unsigned avgstrlen = 50) {
    this->blockSize = blockSize;
    this->avgStrLen = avgstrlen;
    stringRM.createStringCollection(fileName, blockSize, avgstrlen);
    sprintf(ridMapFileName, "ridmap_%s", fileName);
    ridMap.clear();
    writeRidMapFile();
  }
  
  void openContainer(const char* fileName, bool disableStreamBuffer = false) {
    stringRM.openStringCollection(fileName, disableStreamBuffer);    
    sprintf(ridMapFileName, "ridmap_%s", fileName);
    readRidMapFile();
  }
  
  // combined create and open into one call
  void createAndOpen(const char* fileName, 
		     const unsigned blockSize = 4096, 
		     const unsigned avgstrlen = 50,  
		     bool disableStreamBuffer = false) {
    this->blockSize = blockSize;
    this->avgStrLen = avgstrlen;
    ridMap.clear();
    sprintf(ridMapFileName, "ridmap_%s", fileName);
    writeRidMapFile();
    stringRM.createStringCollection(fileName, blockSize, avgstrlen);
    stringRM.openStringCollection(fileName, disableStreamBuffer);
  }
  
  void retrieveString_Impl(string& target, const unsigned i) {
    stringRM.retrieveString(target, ridMap[i]);
  }
  
  unsigned insertString_Impl(const string& s) {
    RecordID tmp;
    stringRM.insertString(s, tmp);
    ridMap.push_back(tmp);
    return ridMap.size()-1;
  }
  
  void integrateUpdates_Impl(const unordered_set<unsigned>& deletedStringIds, unsigned* stringIdMapping) {
    cout << "OPERATION NOT SUPPORTED FOR DISK-BASED STRINGCONTAINER" << endl;
  }

  bool inCache(unsigned sid) { return stringRM.inCache(ridMap[sid]); }

  unsigned size_Impl() { return ridMap.size(); }
  void flushCache_Impl() { stringRM.flushCache(); }
  RecordID* getRecordID_Impl(unsigned i) { return &ridMap[i]; }
  PhysOrd getPhysOrd_Impl() { return pho; }
};

template<typename InputIterator>
void StringContainerRM::fillContainer_Impl(InputIterator first, InputIterator last) {
#ifdef TIMER_START
  cout << "FILLING CONTAINER..." << endl;
#endif
  if(pho != PHO_NONE) {
    StringContainerRM* tempContainer = createDefaultStringContainer<StringContainerRM>(blockSize, avgStrLen);
    while(first != last) {
      tempContainer->insertString(*first);
      first++;
    }
    copyReorg(tempContainer);
    delete tempContainer;
    int ret = system("rm default2.rm"); // WARNING: OS specific
    if(ret != 0) cout << "Temp container file default2.rm could not be deleted" << endl;
    ret = system("rm ridmap_default2.rm"); // WARNING: OS specific
    if(ret != 0) cout << "Temp ridmapfile file ridmap_default2.rm could not be deleted" << endl;
  }
  else {
    if(gatherStats) statsCollector.begin();
    while(first != last) {
      if(gatherStats) statsCollector.next(*first);
      insertString(*first);
      first++;
    }
    if(gatherStats) statsCollector.end();
  }
  
  writeRidMapFile();

  flushCache();
}


#endif
