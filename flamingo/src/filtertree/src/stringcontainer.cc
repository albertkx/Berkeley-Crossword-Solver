/*
  $Id: stringcontainer.cc 5786 2010-10-22 04:24:20Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include "stringcontainer.h"

#include "common/src/gramgen.h"
#include "common/src/simmetric.h"
#include "util/src/misc.h"
#include <cfloat>
#include <cstring>

template<>
StringContainerVector*
createDefaultStringContainer(unsigned blockSize, unsigned avgStrLen) {
  return new StringContainerVector(true); 
}

template<>
StringContainerRM*
createDefaultStringContainer(unsigned blockSize, unsigned avgStrLen) {
  StringContainerRM* container = new StringContainerRM(true);
  container->createContainer("default2.rm", blockSize, avgStrLen);
  container->openContainer("default2.rm", true);
  return container;
}

FilterStats::
FilterStats(AbstractFilter* filter)
  : filter(filter), gramGen(NULL) {
  begin();
}

void 
FilterStats::
begin() {
  minKey = 0xFFFFFFFF;
  maxKey = 0;
  wtedAvgValCount = 0;
  countMap.clear();
}

void
FilterStats::
next(const string& s) {
  unsigned key = filter->getKey(s, gramGen);
  countMap[key]++;
  if(key < minKey) minKey = key;
  if(key > maxKey) maxKey = key;
}

void
FilterStats::
end(float ed, unsigned strCount) {
  unsigned start = 0;
  unsigned stop = 0;        
  GramGenFixedLen gramGen(3);
  SimMetricEd sim(gramGen);
  sim.getFilterBounds("", 1, filter, start, stop);
  stop = ed * stop;
  
  // compute weighted average
  unsigned valCount = 0;
  unordered_map<unsigned, unsigned>::iterator iter;   
  for(unsigned i = minKey; i <= minKey + stop; i++) {
    iter = countMap.find(i);
    if(iter != countMap.end()) valCount += iter->second;                  
  }    
  float weight = (float)valCount / (float)strCount;
  float totalWeight = weight;
  wtedAvgValCount = (float)valCount * weight;
  
  for(unsigned i = minKey + stop + 1; i <= maxKey; i++) {
    unsigned j = i - (minKey + stop + 1);
    
    iter = countMap.find(i);
    if(iter != countMap.end()) valCount += iter->second;                  
    
    iter = countMap.find(j);
    if(iter != countMap.end()) valCount -= iter->second;            
    
    if((signed) valCount < 0) valCount = 0;
    
    weight = (float)valCount / (float)strCount;
    totalWeight += weight;
    wtedAvgValCount += (float)valCount * weight;      
  }
  wtedAvgValCount /= totalWeight;
  
  countMap.clear();
}

void 
StatsCollector::
begin() {
  maxStrLen = 0;
  minStrLen = 0xFFFFFFFF;
  avgStrLen = 0;
  strCount = 0;
  memset(charFreqs, 0, 256 * sizeof(unsigned));
  
  clearFilterStats();
  
  FilterStats* lengthFilter = new FilterStats(new LengthFilter((unsigned)0));
  lengthFilter->setGramGen(gramGen);
  filterStats.push_back(lengthFilter);

  if(gramGen->getType() != GGT_WORDS) {
    FilterStats* charsumFilter = new FilterStats(new CharsumFilter((unsigned)0));
    charsumFilter->setGramGen(gramGen);
    filterStats.push_back(charsumFilter);
  }
}

void
StatsCollector::
next(const string& s) {
  for(unsigned i = 0; i < filterStats.size(); i++)
    filterStats[i]->next(s);
  
  avgStrLen += s.size();
  strCount++;  

  for(unsigned i = 0; i < s.size(); i++) {
    unsigned char ix = (unsigned char)s[i];
    charFreqs[ix]++;
  }

  if(s.length() < minStrLen) minStrLen = s.length();
  if(s.length() > maxStrLen) maxStrLen = s.length();
  
}

void 
StatsCollector::
end() {      
  avgStrLen /= (float)strCount;    
  float ed = (unsigned)(avgStrLen - floor(avgStrLen * 0.85));
  
  for(unsigned i = 0; i < filterStats.size(); i++)
    filterStats[i]->end(ed, strCount);
}

FilterStats*
StatsCollector::
getBestPartFilter() {
  unsigned bestIx = 0xFFFFFFFF;
  float bestValCount = FLT_MAX;
  for(unsigned i = 0; i < filterStats.size(); i++) {
    float currValCount = filterStats[i]->getWtedAvgValCount();
    if(currValCount < bestValCount) {
      bestIx = i;
      bestValCount = currValCount;
    }
  }
  
  if(bestIx != 0xFFFFFFFF) return filterStats[bestIx];
  else return NULL;
}

void 
StatsCollector::
clearFilterStats() {
  for(unsigned i = 0; i < filterStats.size(); i++)
    if(filterStats[i]) delete filterStats[i];
  filterStats.clear();
}

StatsCollector::  
~StatsCollector() {
  clearFilterStats();
}

void
StringContainerVector::
fillContainer_Impl(const char* fileName, const unsigned count, const unsigned maxLineLen) {
  if(pho != PHO_NONE) {
    StringContainerVector* tempContainer = createDefaultStringContainer<StringContainerVector>(0, 0);
    this->fillContainer(tempContainer, fileName, count, maxLineLen);
    this->copyReorg(tempContainer);
    delete tempContainer;
  }
  else {
    fillContainer(this, fileName, count, maxLineLen);
  }
}

void 
StringContainerVector::
integrateUpdates_Impl(const unordered_set<unsigned>& deletedStringIds, unsigned* stringIdMapping) {
  vector<string> tmp;
  
  unsigned deleted = 0;
  for(unsigned i = 0; i < container.size(); i++) {
    if(deletedStringIds.find(i) == deletedStringIds.end()) {
      tmp.push_back(container[i]);
      stringIdMapping[i] = i - deleted;
    }
    else {
      deleted++;
      stringIdMapping[i] = 0xFFFFFFFF; // mark id as deleted
    }
  }

  if(pho == PHO_NONE) {
    std::swap(container, tmp);
  }
  else {
    // TODO: maintain physical order
  }
}

void
StringContainerRM::
fillContainer_Impl(const char* fileName, const unsigned count, const unsigned maxLineLen) {
  if(pho != PHO_NONE) {
    StringContainerRM* tempContainer = createDefaultStringContainer<StringContainerRM>(blockSize, avgStrLen);
    this->fillContainer(tempContainer, fileName, count, maxLineLen);
    this->copyReorg(tempContainer);
    delete tempContainer;
    int ret = system("rm default2.rm"); // WARNING: OS specific
    if(ret != 0) cout << "Temp container file default2.rm could not be deleted" << endl;
    ret = system("rm ridmap_default2.rm"); // WARNING: OS specific
    if(ret != 0) cout << "Temp ridmapfile file ridmap_default2.rm could not be deleted" << endl;
  }
  else {
    fillContainer(this, fileName, count, maxLineLen);
  }
  
  writeRidMapFile();
  
  flushCache();
}
