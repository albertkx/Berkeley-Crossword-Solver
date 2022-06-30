/*
  $Id: filtertypes.h 4777 2009-11-20 01:26:06Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _filtertypes_h_
#define _filtertypes_h_

#include <iostream>
#include <fstream>
#include <cstring>
#include "gramgen.h"

using namespace std;

// forward declarations
class AbstractFilter;
class LengthFilter;
class CharsumFilter;
class QueryCharsumStats;
class StrCharsumStats;
class StrLcStats;
class StrHcStats;

// only for edit distance
bool letterCountFilter(StrLcStats* queryLcStats, StrLcStats* candiLcStats, unsigned lcCharNum, float ed);

// only for bag-based similarity metrics, e.g. jaccard
bool hashCountFilter(StrHcStats* queryHcStats, StrHcStats* candiHcStats, unsigned hcBuckets, float jacc);

enum FilterType {
  FT_NONE,
  FT_LENGTH,
  FT_CHARSUM
};

enum PostMergeFilter {
  PMF_NONE,
  PMF_CSF_REG,
  PMF_CSF_OPT,
  PMF_LC,
  PMF_HC
};

// used in charsum filtering of partitions
class CharsumPartInfo {
public:  
  unsigned maxStrLen;
  unsigned minStrLen;
  unsigned maxCharsum;
  unsigned minCharsum;
  unsigned char highChar;
  unsigned char lowChar;
  CharsumPartInfo() {
    maxStrLen = 0;
    minStrLen = 0xFFFFFFFF;
    maxCharsum = 0;
    minCharsum = 0xFFFFFFFF;
    highChar = 0;
    lowChar = 0xFF;
  }
};

class AbstractFilter {
protected:
  FilterType ft;  
  unsigned maxKey;

public:
  AbstractFilter() {}
  AbstractFilter(AbstractFilter* src) {}
  virtual unsigned getFilterLbound() const = 0;
  virtual unsigned getFilterUbound() const = 0;
  virtual unsigned getKey(const string& s, const GramGen* gramGen) const = 0;  
  virtual void adjustBounds(unsigned minKey, unsigned maxKey, unsigned fanout) = 0;
  virtual AbstractFilter* clone() const = 0;
  FilterType getType() const { return ft; }

  // create appropriate sub-class from ifstream and FilterType
  static AbstractFilter* loadFilterInstance(ifstream& fpIn);
  virtual void saveFilterInstance(ofstream& fpOut) const = 0;

  virtual string getName() const = 0;
  
  virtual ~AbstractFilter() {};
};

class LengthFilter : public AbstractFilter {
private:
  unsigned maxStrLen;

public:
  LengthFilter(unsigned maxStrLen) { 
    ft = FT_LENGTH; 
    this->maxStrLen = maxStrLen;
    this->maxKey = maxStrLen;
  }
  
  LengthFilter(const LengthFilter* src) {
    this->ft = src->ft;
    this->maxStrLen = src->maxStrLen;
    this->maxKey = src->maxKey;
  }
  
  LengthFilter(ifstream& fpIn);

  unsigned getFilterLbound() const;
  unsigned getFilterUbound() const;
  unsigned getKey(const string& s, const GramGen* gramGen) const;
  void adjustBounds(unsigned minKey, unsigned maxKey, unsigned fanout);
  AbstractFilter* clone() const;

  string getName() const { return "Length"; }
  
  // methods for saving and loading to/from a file
  void saveFilterInstance(ofstream& fpOut) const;
};

#pragma pack(push,1) 
class CharFreq {
public:
  unsigned char c;
  unsigned freq;  
  static int cmp(const void* a, const void* b) {    
    const CharFreq* x = static_cast<const CharFreq* >(a);
    const CharFreq* y = static_cast<const CharFreq* >(b);
    if(x->freq > y->freq) return -1;
    if(x->freq < y->freq) return 1;    
    return 0;
  }
};
#pragma pack(pop)

class CharsumFilter : public AbstractFilter {
private:
  unsigned maxStrLen;
  unsigned char maxChar;
  unsigned char* charMap;
  signed** lbCache;
  signed** ubCache;
  
  void setCharMap(const unsigned* charFreqs);

public:
  // use this constructor for partitioning filter
  CharsumFilter(unsigned maxStrLen, unsigned char maxChar = 127) { 
    ft = FT_CHARSUM; 
    this->maxChar = maxChar;
    this->maxStrLen = maxStrLen;
    this->maxKey = maxStrLen * maxChar;
    this->charMap = NULL;
  }
  
  CharsumFilter(const CharsumFilter* src) {
    this->ft = src->ft;
    this->maxChar = src->maxChar;
    this->maxStrLen = src->maxStrLen;
    this->maxKey = src->maxKey;
    this->charMap = src->charMap;
  }
  
  // use this constructor for non-partitioning filter (just filtering)
  CharsumFilter(const unsigned* charFreqs, unsigned char maxChar = 127) {
    this->maxChar = maxChar;
    setCharMap(charFreqs);
  }
  
  CharsumFilter(ifstream& fpIn);
  
  unsigned getCharsum(const string& str) const;
  unsigned fillCharsumStats(const string& str, 
			    unsigned* charsum, 
			    unsigned char* lowChars, 
			    unsigned char* highChars, 
			    unsigned numChars);
  
  unsigned getFilterLbound() const;
  unsigned getFilterUbound() const;
  unsigned getKey(const string& s, const GramGen* gramGen) const;
  unsigned char getMaxChar() const { return maxChar; }
  void adjustBounds(unsigned minKey, unsigned maxKey, unsigned fanout);
  AbstractFilter* clone() const;
  
  void prepareCache(unsigned editDistance);
  bool passesFilterCache(QueryCharsumStats* queryStats, StrCharsumStats* candiStats, unsigned editDistance);
  void clearCache(unsigned editDistance);

  bool passesFilter(QueryCharsumStats* queryStats, StrCharsumStats* candiStats, unsigned editDistance);
  
  bool passesFilter(signed queryLen, 
		    signed queryChecksum, 
		    unsigned char* queryLowChars,
		    unsigned char* queryHighChars,
		    signed candiLen, 
		    signed candiChecksum,
		    signed candiLowChar,
		    signed candiHighChar,
		    unsigned editDistance);
  
  bool passesFilter(signed queryLen, 
		    signed queryChecksum, 
		    unsigned char* queryLowChars,
		    unsigned char* queryHighChars,
		    signed candiLen, 
		    signed candiChecksum,
		    unsigned numCandiChars,
		    unsigned char* candiLowChars,
		    unsigned char* candiHighChars,
		    unsigned editDistance);

  bool passesPartitionFilter(signed queryLen, 
			     signed queryChecksum, 
			     unsigned char* queryLowChars,
			     unsigned char* queryHighChars,
			     CharsumPartInfo* partInfo,
			     unsigned editDistance);
  
  // methods for saving and loading to/from a file
  void saveFilterInstance(ofstream& fpOut) const;

  string getName() const { return "Charsum"; }

  ~CharsumFilter() {
    if(charMap) delete[] charMap;
  }
};

#pragma pack(push,1) 
class StrCharsumStats {
public:
  unsigned short length;
  unsigned char lChar;
  unsigned char hChar;
  unsigned charsum;
};
#pragma pack(pop)

#pragma pack(push,1)
class QueryCharsumStats {
public:
  unsigned length;
  unsigned char* lChars;
  unsigned char* hChars;
  unsigned charsum;
  
  QueryCharsumStats(const string& str, unsigned ed, CharsumFilter* csf) {
    lChars = new unsigned char[ed];
    hChars = new unsigned char[ed];
    length = str.size();
    csf->fillCharsumStats(str, &charsum, lChars, hChars, ed);
  }
  
  ~QueryCharsumStats() {
    if(lChars) delete[] lChars;
    if(hChars) delete[] hChars;
  }
};
#pragma pack(pop)

#pragma pack(push,1)
class StrLcStats {
public:
  unsigned short length;
  unsigned char* charFreqs;

  StrLcStats() : length(0), charFreqs(NULL) {}
  
  StrLcStats(const string& str, unsigned lcCharNum, unsigned char* lcChars) {
    charFreqs = NULL;
    length = 0;
    set(str, lcCharNum, lcChars);
  }
  
  void set(const string& str, unsigned lcCharNum, unsigned char* lcChars) {
    length = (unsigned short)str.length();    
    if(!charFreqs) charFreqs = new unsigned char[lcCharNum];
    memset(charFreqs, 0, lcCharNum);
    for(unsigned i = 0; i < str.size(); i++) {
      unsigned char c = (unsigned char)str[i];
      for(unsigned j = 0; j < lcCharNum; j++) {
	if(c == lcChars[j]) {
	  charFreqs[j]++;
	  break;
	}
      }
    }
  }

  ~StrLcStats() {
    if(charFreqs) delete[] charFreqs;
  }
};
#pragma pack(pop)


#pragma pack(push,1)
class StrHcStats {
public:
  unsigned char* hashCounts;
  
  StrHcStats() : hashCounts(NULL) {}
  
  StrHcStats(const vector<unsigned>& gramCodes, unsigned hcBuckets) {
    hashCounts = NULL;
    set(gramCodes, hcBuckets);
  }
  
  void set(const vector<unsigned>& gramCodes, unsigned hcBuckets) {
    if(!hashCounts) hashCounts = new unsigned char[hcBuckets];
    memset(hashCounts, 0, hcBuckets);
    for(unsigned i = 0; i < gramCodes.size(); i++)
      hashCounts[gramCodes[i] % hcBuckets]++;
  }

  ~StrHcStats() {
    if(hashCounts) delete[] hashCounts;
  }
};
#pragma pack(pop)

#endif



