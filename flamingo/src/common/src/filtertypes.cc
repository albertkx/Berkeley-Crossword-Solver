/*
  $Id: filtertypes.cc 4777 2009-11-20 01:26:06Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include "filtertypes.h"
#include "util/src/misc.h"
#include <cmath>
#include <climits>

AbstractFilter* 
AbstractFilter::
loadFilterInstance(ifstream& fpIn) {
  FilterType filterType;
  fpIn.read((char*)&filterType, sizeof(FilterType));

  switch(filterType) {
  case FT_LENGTH: return new LengthFilter(fpIn); break;
  case FT_CHARSUM: return new CharsumFilter(fpIn); break;
  default: {
    cout << "WARNING: attempt to read filter from file failed. Unknown filter type." << endl;
    return NULL;
  } break;
  }
}

LengthFilter::
LengthFilter(ifstream& fpIn) {
  ft = FT_LENGTH;
  fpIn.read((char*)&maxStrLen, sizeof(unsigned));
  fpIn.read((char*)&this->maxKey, sizeof(unsigned));
}

unsigned 
LengthFilter::
getFilterLbound() const {
  return 0;
}

unsigned 
LengthFilter::
getFilterUbound() const  {
  return maxKey;
}

void 
LengthFilter::
adjustBounds(unsigned minKey, unsigned maxKey, unsigned fanout) {
  // we only adjust the upper bound for the length filter
  this->maxKey = maxKey + (fanout - (maxKey % fanout));
}

unsigned
LengthFilter::
getKey(const string& s, const GramGen* gramGen) const {
  return gramGen->getLength(s);
}

AbstractFilter* 
LengthFilter::
clone() const {
  return new LengthFilter(this);
}

void
LengthFilter::
saveFilterInstance(ofstream& fpOut) const {
  fpOut.write((const char*)&ft, sizeof(FilterType));
  fpOut.write((const char*)&maxStrLen, sizeof(unsigned));
  fpOut.write((const char*)&this->maxKey, sizeof(unsigned));
}

CharsumFilter::
CharsumFilter(ifstream& fpIn) {
  ft = FT_CHARSUM;
  fpIn.read((char*)&maxStrLen, sizeof(unsigned));
  fpIn.read((char*)&maxChar, sizeof(unsigned char));
  fpIn.read((char*)&this->maxKey, sizeof(unsigned));

  // read charmap
  unsigned charMapSize;
  fpIn.read((char*)&charMapSize, sizeof(unsigned));
  if(charMapSize == 256) {
    charMap = new unsigned char[256];
    fpIn.read((char*)&charMap, sizeof(unsigned char) * 256);
  }
  else {
    charMap = NULL;
  }
}

void 
CharsumFilter::
setCharMap(const unsigned* charFreqs) {  

  if(charFreqs == NULL) {
    charMap = NULL;
    return;
  }

  double totalFreq = 0;
  CharFreq freqs[256];
  for(unsigned i = 0; i < 256; i++) {
    freqs[i].c = (unsigned char)i;
    freqs[i].freq = charFreqs[i];
    totalFreq += charFreqs[i];
  }
  qsort(freqs, 256, sizeof(CharFreq), CharFreq::cmp);
  
  // set the charmap and the maxChar
  charMap = new unsigned char[256];
  double currFreq = 0;
  maxChar = 0;
  for(unsigned i = 0; i < 256; i++) {
    charMap[(unsigned char)freqs[i].c] = i;
    currFreq += freqs[i].freq;
    if(maxChar == 0 && (currFreq / totalFreq) >= 0.95) maxChar = i;
  }  
}

unsigned 
CharsumFilter::
getFilterLbound() const {
  return 0;
}

unsigned 
CharsumFilter::
getFilterUbound() const {
  return maxKey;
}

void 
CharsumFilter::
adjustBounds(unsigned minKey, unsigned maxKey, unsigned fanout) {
  // we only adjust the upper bound for the charsum filter  
  this->maxKey = maxKey + (fanout - (maxKey % fanout));
}

unsigned
CharsumFilter::
getKey(const string& s, const GramGen* gramGen) const { 
  return getCharsum(s);
}

AbstractFilter* 
CharsumFilter::
clone() const {
  return new CharsumFilter(this);
}

void
CharsumFilter::
saveFilterInstance(ofstream& fpOut) const {
  fpOut.write((const char*)&ft, sizeof(FilterType));
  fpOut.write((const char*)&maxStrLen, sizeof(unsigned));
  fpOut.write((const char*)&maxChar, sizeof(unsigned char));
  fpOut.write((const char*)&this->maxKey, sizeof(unsigned));

  // write charmap
  unsigned charMapSize = 0;
  if(charMap) {
    charMapSize = 256;
    fpOut.write((const char*)&charMapSize, sizeof(unsigned));
    fpOut.write((const char*)&charMap, sizeof(unsigned char) * charMapSize);
  }
  else {
    fpOut.write((const char*)&charMapSize, sizeof(unsigned));
  }
}

unsigned 
CharsumFilter::
getCharsum(const string& str) const {
  unsigned sum = 0;
  if(charMap) {
    for(unsigned i = 0; i < str.size(); i++) {
      unsigned char c = charMap[(unsigned char)str[i]];
      sum += (c > maxChar) ? maxChar : c;
    }
  }
  else {
    for(unsigned i = 0; i < str.size(); i++) {
      unsigned char c = (unsigned char)str[i];
      sum += (c > maxChar) ? maxChar : c;
    }
  }
  return sum;  
}

unsigned 
CharsumFilter::
fillCharsumStats(const string& str, 
		 unsigned* charsum, 
		 unsigned char* lowChars, 
		 unsigned char* highChars, 
		 unsigned numChars) {
  
  unsigned sum = 0;
  unsigned charCount = 0;

  vector<unsigned char> queryHighHeap;
  queryHighHeap.reserve(str.size());
  make_heap(queryHighHeap.begin(), queryHighHeap.end());

  vector<unsigned char> queryLowHeap;
  queryLowHeap.reserve(str.size());
  make_heap(queryLowHeap.begin(), queryLowHeap.end());
  
  unsigned char reverseChars[str.size()];
  
  if(charMap) {
    for(unsigned i = 0; i < str.size(); i++) {
      unsigned char c = charMap[(unsigned char)str[i]];
      if(c > maxChar) c = maxChar;
      sum += c;
      reverseChars[i] = (unsigned char)0xFF - charMap[(unsigned char)str[i]];
      
      queryHighHeap.push_back(reverseChars[i]);
      push_heap(queryHighHeap.begin(), queryHighHeap.end());      

      queryLowHeap.push_back(charMap[(unsigned char)str[i]]);
      push_heap(queryLowHeap.begin(), queryLowHeap.end());
    }
    *charsum = sum;
    
    while(charCount < numChars && charCount < str.size()) {  
      lowChars[charCount] = queryLowHeap.front();
      highChars[charCount] = (unsigned char)0xFF - queryHighHeap.front();
      if(lowChars[charCount] > maxChar) lowChars[charCount] = maxChar;
      if(highChars[charCount] > maxChar) highChars[charCount] = maxChar;

      pop_heap(queryLowHeap.begin(), queryLowHeap.end());
      queryLowHeap.pop_back();

      pop_heap(queryHighHeap.begin(), queryHighHeap.end());
      queryHighHeap.pop_back();
      
      charCount++;
    }     
  }
  else {
    for(unsigned i = 0; i < str.size(); i++) {
      unsigned char c = (unsigned char)str[i];
      if(c > maxChar) c = maxChar;    
      sum += c;
      reverseChars[i] = (unsigned char)0xFF - (unsigned char)str[i];

      queryHighHeap.push_back(reverseChars[i]);
      push_heap(queryHighHeap.begin(), queryHighHeap.end());      

      queryLowHeap.push_back((unsigned char)str[i]);
      push_heap(queryLowHeap.begin(), queryLowHeap.end());
    }
    *charsum = sum;
    
    while(charCount < numChars && charCount < str.size()) {  
      lowChars[charCount] = queryLowHeap.front();
      highChars[charCount] = (unsigned char)0xFF - queryHighHeap.front();
      if(lowChars[charCount] > maxChar) lowChars[charCount] = maxChar;
      if(highChars[charCount] > maxChar) highChars[charCount] = maxChar;
      
      pop_heap(queryLowHeap.begin(), queryLowHeap.end());
      queryLowHeap.pop_back();
      
      pop_heap(queryHighHeap.begin(), queryHighHeap.end());
      queryHighHeap.pop_back();
      
      charCount++;
    } 
  }
  
  return charCount;
}

void
CharsumFilter::
prepareCache(unsigned editDistance) {
  unsigned lengths = editDistance * 2 + 1; // answers could be ed shorter, ed longer, or same length  
  
  // initialize cache
  lbCache = new signed*[lengths];
  ubCache = new signed*[lengths];
  for(unsigned i = 0; i < lengths; i++) {
    lbCache[i] = new signed[maxChar+1];
    ubCache[i] = new signed[maxChar+1];
    for(unsigned j = 0; j <= maxChar; j++) {
      lbCache[i][j] = INT_MIN;
      ubCache[i][j] = INT_MAX;
    }
  } 
}

bool 
CharsumFilter::
passesFilterCache(QueryCharsumStats* queryStats, StrCharsumStats* candiStats, unsigned editDistance) {
  signed deltaLen = (candiStats->length > queryStats->length) ? candiStats->length - queryStats->length : queryStats->length - candiStats->length;
  signed ed = (signed)editDistance;
  if(deltaLen > ed) return false;

  unsigned cacheIx;  
  if(queryStats->length <= candiStats->length) {
    cacheIx = candiStats->length - queryStats->length;

    // check the bounds in the cache
    if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar] || (signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;
    
    // check if we need to compute cache values
    if(lbCache[cacheIx][candiStats->lChar] == INT_MIN) {
      // compute both bounds
      if(ubCache[cacheIx][candiStats->hChar] == INT_MAX) {
	signed subst = ed - deltaLen;
	ubCache[cacheIx][candiStats->hChar] = (signed)queryStats->charsum + deltaLen * candiStats->hChar;
	lbCache[cacheIx][candiStats->lChar] = (signed)queryStats->charsum + deltaLen * candiStats->lChar;
	for(signed i = 0; i < subst; i++) {
	  ubCache[cacheIx][candiStats->hChar] += (candiStats->hChar > queryStats->lChars[i]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[i] : 0;
	  lbCache[cacheIx][candiStats->lChar] += (candiStats->lChar < queryStats->hChars[i]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[i] : 0;
	}      
	
	if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar] || (signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;
      }
      // only compute lower bound
      else {
	signed subst = ed - deltaLen;
	lbCache[cacheIx][candiStats->lChar] = (signed)queryStats->charsum + deltaLen * candiStats->lChar;
	for(signed i = 0; i < subst; i++) {	  
	  lbCache[cacheIx][candiStats->lChar] += (candiStats->lChar < queryStats->hChars[i]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[i] : 0;
	}      	
	if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar]) return false;
      }
    }
    else {
      // only compute upper bound
      if(ubCache[cacheIx][candiStats->hChar] == INT_MAX) {
	signed subst = ed - deltaLen;
	ubCache[cacheIx][candiStats->hChar] = (signed)queryStats->charsum + deltaLen * candiStats->hChar;
	for(signed i = 0; i < subst; i++) {
	  ubCache[cacheIx][candiStats->hChar] += (candiStats->hChar > queryStats->lChars[i]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[i] : 0;	  
	}      	
	if((signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;
      }
    }    
  }
  else {
    cacheIx = (queryStats->length - candiStats->length) << 1;
        
    // check the bounds in the cache
    if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar] || (signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;

    // check if we need to compute cache values
    if(lbCache[cacheIx][candiStats->lChar] == INT_MIN) {
      // compute both bounds
      if(ubCache[cacheIx][candiStats->hChar] == INT_MAX) {
	signed subst = ed - deltaLen;
	ubCache[cacheIx][candiStats->hChar] = (signed)queryStats->charsum;
	lbCache[cacheIx][candiStats->lChar] = (signed)queryStats->charsum;
	for(signed i = 0; i < deltaLen; i++) {
	  ubCache[cacheIx][candiStats->hChar] -= (signed)queryStats->lChars[i];
	  lbCache[cacheIx][candiStats->lChar] -= (signed)queryStats->hChars[i];
	}
	for(signed i = 0; i < subst; i++) {
	  unsigned qix = i + deltaLen;
	  ubCache[cacheIx][candiStats->hChar] += (candiStats->hChar > queryStats->lChars[qix]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[qix] : 0;	
	  lbCache[cacheIx][candiStats->lChar] += (candiStats->lChar < queryStats->hChars[qix]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[qix] : 0;	
	}            	
	if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar] || (signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;	
      }
      // compute only lower bound
      else {
	signed subst = ed - deltaLen;
	lbCache[cacheIx][candiStats->lChar] = (signed)queryStats->charsum;
	for(signed i = 0; i < deltaLen; i++) {
	  lbCache[cacheIx][candiStats->lChar] -= (signed)queryStats->hChars[i];
	}
	for(signed i = 0; i < subst; i++) {
	  unsigned qix = i + deltaLen;	  
	  lbCache[cacheIx][candiStats->lChar] += (candiStats->lChar < queryStats->hChars[qix]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[qix] : 0;	
	}	
	if((signed)candiStats->charsum < lbCache[cacheIx][candiStats->lChar]) return false;
      }
    }
    else {
      // compute only upper bound
      if(ubCache[cacheIx][candiStats->hChar] == INT_MAX) {
	signed subst = ed - deltaLen;
	ubCache[cacheIx][candiStats->hChar] = (signed)queryStats->charsum;
	for(signed i = 0; i < deltaLen; i++) {
	  ubCache[cacheIx][candiStats->hChar] -= (signed)queryStats->lChars[i];
	}
	for(signed i = 0; i < subst; i++) {
	  unsigned qix = i + deltaLen;
	  ubCache[cacheIx][candiStats->hChar] += (candiStats->hChar > queryStats->lChars[qix]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[qix] : 0;		  
	}            	
	if((signed)candiStats->charsum > ubCache[cacheIx][candiStats->hChar]) return false;
      }
    }    
  }  
  
  return true;
}

void 
CharsumFilter::
clearCache(unsigned editDistance) {
  unsigned lengths = editDistance * 2 + 1; // answers could be ed shorter, ed longer, or same length  
  for(unsigned i = 0; i < lengths; i++) {
    delete lbCache[i];
    delete ubCache[i];
  } 
  delete lbCache;
  delete ubCache;
}

bool 
CharsumFilter::
passesFilter(QueryCharsumStats* queryStats, StrCharsumStats* candiStats, unsigned editDistance) {
  signed ed = (signed)editDistance;
  register signed ubound = (signed)queryStats->charsum;
  register signed lbound = (signed)queryStats->charsum;
  register signed deltaLen = (candiStats->length > queryStats->length) ? candiStats->length - queryStats->length : queryStats->length - candiStats->length;
  if(deltaLen > ed) return false;
  
  signed subst = ed - deltaLen;
  if(queryStats->length <= candiStats->length) {
    ubound += deltaLen * candiStats->hChar;
    lbound += deltaLen * candiStats->lChar;
    for(signed i = 0; i < subst; i++) {
      ubound += (candiStats->hChar > queryStats->lChars[i]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[i] : 0;
      lbound += (candiStats->lChar < queryStats->hChars[i]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[i] : 0;
    }
  }
  else {
    for(signed i = 0; i < deltaLen; i++) {
      ubound -= (signed)queryStats->lChars[i];
      lbound -= (signed)queryStats->hChars[i];
    }
    for(signed i = 0; i < subst; i++) {
      unsigned qix = i + deltaLen;
      ubound += (candiStats->hChar > queryStats->lChars[qix]) ? (signed)candiStats->hChar - (signed)queryStats->lChars[qix] : 0;
      lbound += (candiStats->lChar < queryStats->hChars[qix]) ? (signed)candiStats->lChar - (signed)queryStats->hChars[qix] : 0;
    }
  }
  
  if((signed)candiStats->charsum < lbound || (signed)candiStats->charsum > ubound) return false;
  else return true;
}


bool 
CharsumFilter::
passesFilter(signed queryLen, 
	     signed queryChecksum, 
	     unsigned char* queryLowChars,
	     unsigned char* queryHighChars,
	     signed candiLen, 
	     signed candiChecksum,
	     signed candiLowChar,
	     signed candiHighChar,
	     unsigned editDistance) {
  
  signed ed = (signed)editDistance;
  signed ubound = (signed)queryChecksum;
  signed lbound = (signed)queryChecksum;
  signed deltaLen = (candiLen > queryLen) ? candiLen - queryLen : queryLen - candiLen;
  if(deltaLen > ed) return false;
  
  signed subst = ed - deltaLen;
  if(queryLen <= candiLen) {
    ubound += deltaLen * candiHighChar;
    lbound += deltaLen * candiLowChar;
    for(signed i = 0; i < subst; i++) {
      ubound += (candiHighChar > queryLowChars[i]) ? (signed)candiHighChar - (signed)queryLowChars[i] : 0;
      lbound += (candiLowChar < queryHighChars[i]) ? (signed)candiLowChar - (signed)queryHighChars[i] : 0;
    }
  }
  else {
    for(signed i = 0; i < deltaLen; i++) {
      ubound -= (signed)queryLowChars[i];
      lbound -= (signed)queryHighChars[i];
    }
    for(signed i = 0; i < subst; i++) {
      unsigned qix = i + deltaLen;
      ubound += (candiHighChar > queryLowChars[qix]) ? (signed)candiHighChar - (signed)queryLowChars[qix] : 0;
      lbound += (candiLowChar < queryHighChars[qix]) ? (signed)candiLowChar - (signed)queryHighChars[qix] : 0;
    }    
  }
  
  if(candiChecksum < lbound || candiChecksum > ubound) return false;
  else return true;
}

bool 
CharsumFilter::
passesFilter(signed queryLen, 
	     signed queryChecksum, 
	     unsigned char* queryLowChars,
	     unsigned char* queryHighChars,
	     signed candiLen, 
	     signed candiChecksum,
	     unsigned numCandiChars,
	     unsigned char* candiLowChars,
	     unsigned char* candiHighChars,
	     unsigned editDistance) {
  
  signed ed = (signed)editDistance;
  signed maxCharIx = (signed)numCandiChars - (signed)1;
  signed ubound = (signed)queryChecksum;
  signed lbound = (signed)queryChecksum;
  signed deltaLen = (candiLen > queryLen) ? candiLen - queryLen : queryLen - candiLen;
  if(deltaLen > ed) return false;
  
  signed subst = ed - deltaLen;
  if(queryLen <= candiLen) {
    for(signed i = 0; i < deltaLen; i++) {
      unsigned qix = (i > maxCharIx) ? maxCharIx : i;      
      ubound += candiHighChars[qix];
      lbound += candiLowChars[qix];
    }
    for(signed i = 0; i < subst; i++) {
      unsigned cix = (i + deltaLen > maxCharIx) ? maxCharIx : i + deltaLen;
      ubound += (candiHighChars[cix] > queryLowChars[i]) ? (signed)candiHighChars[cix] - (signed)queryLowChars[i] : 0;
      lbound += (candiLowChars[cix] < queryHighChars[i]) ? (signed)candiLowChars[cix] - (signed)queryHighChars[i] : 0;
    }    
  }
  else {
    for(signed i = 0; i < deltaLen; i++) {
      ubound -= queryLowChars[i];
      lbound -= queryHighChars[i];
    }
    for(signed i = 0; i < subst; i++) {
      unsigned qix = i + deltaLen;
      unsigned cix = (i > maxCharIx) ? maxCharIx : i;
      ubound += (candiHighChars[cix] > queryLowChars[qix]) ? (signed)candiHighChars[cix] - (signed)queryLowChars[qix] : 0;
      lbound += (candiLowChars[cix] < queryHighChars[qix]) ? (signed)candiLowChars[cix] - (signed)queryHighChars[qix] : 0;
    }    
  }  
  
  if(candiChecksum < lbound || candiChecksum > ubound) return false;
  else return true;
}

bool 
CharsumFilter::
passesPartitionFilter(signed queryLen, 
		      signed queryChecksum, 
		      unsigned char* queryLowChars,
		      unsigned char* queryHighChars,
		      CharsumPartInfo* partInfo,
		      unsigned editDistance) {

  // sanity
  if(partInfo->minStrLen == 0xFFFFFFFF) return true;

  signed ed = (signed)editDistance;
  signed ubound = (signed)queryChecksum;
  signed lbound = (signed)queryChecksum;

  signed deltaLenUb = 
    ((signed)partInfo->maxStrLen > queryLen) 
    ? (signed)partInfo->maxStrLen - queryLen 
    : queryLen - (signed)partInfo->maxStrLen;

  signed deltaLenLb = 
    ((signed)partInfo->minStrLen > queryLen) 
    ? (signed)partInfo->minStrLen - queryLen 
    : queryLen - (signed)partInfo->minStrLen;
  
  // compute upper bound
  if(queryLen <= (signed)partInfo->maxStrLen) {
    ubound += deltaLenUb * partInfo->highChar;
  }
  else {
    for(signed i = 0; i < deltaLenUb; i++) {
      ubound -= (signed)queryLowChars[i];
    }
  }
  for(signed i = deltaLenUb; i < (signed)ed; i++) {
    ubound += ((signed)partInfo->highChar > (signed)queryLowChars[i]) ? (signed)partInfo->highChar - (signed)queryLowChars[i] : 0;
  }

  // compute lower bound  
  if(queryLen <= (signed)partInfo->minStrLen) {
    lbound += (signed)deltaLenLb * (signed)partInfo->lowChar;
  }
  else {
    for(signed i = 0; i < deltaLenLb; i++) {
      lbound -= (signed)queryHighChars[i];
    }
  }
  for(signed i = deltaLenLb; i < (signed)ed; i++) {
    lbound += (partInfo->lowChar < (signed)queryHighChars[i]) ? partInfo->lowChar - (signed)queryHighChars[i] : 0;
  }
  
  // sanity (cases can happen due to overflow)
  if(lbound > ubound || lbound > queryChecksum || ubound < queryChecksum) return true;
  
  if((signed)partInfo->maxCharsum < lbound || (signed)partInfo->minCharsum > ubound) return false;
  else return true;  
}

bool letterCountFilter(StrLcStats* queryLcStats, StrLcStats* candiLcStats, unsigned lcCharNum, float ed) {

  unsigned T = (unsigned)ed;
  if(queryLcStats->length > candiLcStats->length) {
    unsigned diff = queryLcStats->length - candiLcStats->length;
    if(diff > T) return false;
  }
  else {
    unsigned diff = candiLcStats->length - queryLcStats->length;
    if(diff > T) return false;
  }
  
  unsigned gtsum = 0;
  unsigned ltsum = 0;
  for(unsigned i = 0; i < lcCharNum; i++) {    

    if(queryLcStats->charFreqs[i] > candiLcStats->charFreqs[i]) {
      gtsum += queryLcStats->charFreqs[i] - candiLcStats->charFreqs[i];
      if(gtsum > T) return false;      
    }
    else {
      ltsum += candiLcStats->charFreqs[i] - queryLcStats->charFreqs[i];
      if(ltsum > T) return false;
    }
  }
  
  return true;
}

// only for bag-based similarity metrics, e.g. jaccard
bool hashCountFilter(StrHcStats* queryHcStats, StrHcStats* candiHcStats, unsigned hcBuckets, float jacc) {
  // compute jaccard bound
  uint in = 0;
  uint un = 0;
  for(uint i = 0; i < hcBuckets; i++) {
    in += min(queryHcStats->hashCounts[i], candiHcStats->hashCounts[i]);
    un += max(queryHcStats->hashCounts[i], candiHcStats->hashCounts[i]);
  }
  
  float jaccBound = (float)in / (float)un;  
  if(jaccBound < jacc) return false;
  else return true;
}
