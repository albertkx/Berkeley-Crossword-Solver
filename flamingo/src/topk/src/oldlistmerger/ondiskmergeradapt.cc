/*
  $Id: ondiskmergeradapt.cc 5716 2010-09-09 04:27:56Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include "ondiskmergeradapt.h"

float 
Candidate<Array<PosID> >::
wildcardEd(const string &s1, const string& s2, const char wildchar) {
  uint i, iCrt, iPre, j;
  uint
    n = s1.length(), 
    m = s2.length();
  
  if (n == 0)
    return m;
  if (m == 0)
    return n;
  
  uint d[2][m + 1];
  
  for (j = 0; j <= m; j++)
    d[0][j] = j;
  
  iCrt = 1;
  iPre = 0;
  for (i = 1; i <= n; i++) {
    d[iCrt][0] = i;
    for (j = 1; j <= m; j++)
      d[iCrt][j] = min(min(d[iPre][j] + 1, 
                           d[iCrt][j - 1] + 1), 
                       d[iPre][j - 1] + ( (s1[i - 1] == s2[j - 1] || s2[j - 1] == wildchar) ? 0 : 1));
    iPre = !iPre;
    iCrt = !iCrt;
  }
  
  return d[iPre][m];
}

float 
Candidate<Array<PosID> >::
wildcardEd(const char* s1, const char* s2, unsigned n, unsigned m, const char wildchar) {
  uint i, iCrt, iPre, j;
  
  if (n == 0)
    return m;
  if (m == 0)
    return n;
  
  uint d[2][m + 1];
  
  for (j = 0; j <= m; j++)
    d[0][j] = j;
  
  iCrt = 1;
  iPre = 0;
  for (i = 1; i <= n; i++) {
    d[iCrt][0] = i;
    for (j = 1; j <= m; j++)
      d[iCrt][j] = min(min(d[iPre][j] + 1, 
                           d[iCrt][j - 1] + 1), 
                       d[iPre][j - 1] + ( (s1[i - 1] == s2[j - 1] || s2[j - 1] == wildchar) ? 0 : 1));
    iPre = !iPre;
    iCrt = !iCrt;
  }
  
  return d[iPre][m];
}

void
Candidate<Array<PosID> >::
checkMismatches(const Query& query, unsigned gramLength) {
  unsigned count = 0;
  unsigned location = 0;
  for(set<unsigned char>::iterator iter = mismatchPositions.begin();
      iter != mismatchPositions.end();
      iter++) {
    if(*iter > location) {
      count++;
      location = *iter + gramLength - 1;
    }
  }  
  if(count > query.threshold) isPruned |= POSFILTER_MISMATCH;
}

void
Candidate<Array<PosID> >::
checkPartialString(const Query& query, const string& prePostQueryStr) {
  float ed = wildcardEd(prePostQueryStr, partialStr);
  if(ed > query.threshold) isPruned |= POSFILTER_DP;
}

void
Candidate<Array<PosID> >::
checkSubstrFilter(const Query& query, 
		  const string& prePostQueryStr,
		  const string& gram,
		  unsigned gramLength,
		  unsigned stringLength,
		  unsigned char stringPos,
		  unsigned char queryPos,
		  unsigned substrFilterCount) {
    
  unsigned queryLength = prePostQueryStr.size();

  // sanity check
  if(stringPos >= queryLength) return;

  signed lengthDiff = abs( (signed)queryLength - (signed)stringLength );  

  // we have a mismatch
  if(substrFilterCount == 0) {    
    if(lengthDiff + (signed)gramLength <= (signed)query.threshold) return;
  }
  // we have a match
  else { 
    signed posDiff = (signed)stringPos - (signed)queryPos;
    signed diff = abs( (signed)stringLength - (signed)queryLength - posDiff) + abs(posDiff);    
    if(diff <= query.threshold) return; 
  }

  // make sure there was no collision
  if( gram.size() == 0 ) return;
  
  unsigned start_pos = 0;
  unsigned query_end_pos = 0;
  unsigned string_end_pos = 0;
  
  start_pos = stringPos;
  query_end_pos = stringPos + query.threshold + gramLength;
  string_end_pos = query_end_pos;
  
  if(queryLength > stringLength) query_end_pos += lengthDiff;
  else string_end_pos += lengthDiff;
  
  if(query_end_pos >= prePostQueryStr.size()) query_end_pos = prePostQueryStr.size();  
  if(string_end_pos >= stringLength) string_end_pos = stringLength;    
  
  float ed = wildcardEd(prePostQueryStr.c_str() + start_pos,
			partialStr.c_str() + start_pos,
			query_end_pos - start_pos,
			string_end_pos - start_pos,
			WILDCHAR);
  
  if(ed > query.threshold) isPruned |= POSFILTER_SUBSTR;
}

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
processPosID(const Query& query, 
	     const string& prePostQueryStr, 
	     unsigned gramCode,
	     unsigned candiID,
	     vector<unsigned char>& wglPos,
	     unsigned char candiPos,
	     bool* posMatched,
	     Candidate<Array<PosID> >* c) const {
  
  string& gram = (*(this->gramcode2gram))[gramCode];
  c->updatePartialString(gram, candiPos);
  
  unsigned gramLength = this->gramGen->getGramLength();
  unsigned prePostCandiStrLen = this->gramGen->getPrePostStrLen(this->charsumStats[candiID].length);
  
  // apply position filter and remember matching grams from queries perspective
  unsigned substrFilterCount = 0;
  unsigned char substrFilterQueryPos = 0;
  for(unsigned i = 0; i < wglPos.size(); i++) {        
    unsigned char posDiff = (wglPos.at(i) > candiPos) ? wglPos.at(i) - candiPos : candiPos - wglPos.at(i);    
    if(posDiff <= (unsigned char)query.threshold) {
      substrFilterCount++;
      substrFilterQueryPos = wglPos.at(i);
      posMatched[i] = true;
    }
  }
  
  if( posFilters & POSFILTER_SUBSTR && substrFilterCount < 2 && !(c->isPruned & POSFILTER_SUBSTR) ) {
    c->checkSubstrFilter(query, prePostQueryStr, gram, gramLength, prePostCandiStrLen, candiPos, substrFilterQueryPos, substrFilterCount);
  }
}

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
addRemainingCandidates(const Query& query, const string& prePostQueryStr, LeafContext<Array<PosID> >* lc, vector<unsigned>& results) const {
  for(unsigned x = 0; x < lc->candidates.size(); x++) {
    Candidate<Array<PosID> >* c = lc->candidates.at(x);
    if(posFilters & POSFILTER_ENDDP) c->checkPartialString(query, prePostQueryStr);
    if(!c->isPruned) results.push_back(c->id);
    delete c;
  }
}

template<>
Array<PosID>::iterator
OnDiskMergerAdapt<Array<PosID> >::
expProbe(Array<PosID>::iterator start, 
	 Array<PosID>::iterator end, 
	 unsigned value) const {
  unsigned c = 0;
  for(;;) {      
    Array<PosID>::iterator iter = start + (1 << c);
    if(iter >= end) return end;
    else if(iter->id >= value) return iter;
    c++;
  }
}

template<>
void  
OnDiskMergerAdapt<Array<PosID> >::
detectDuplicateLists(vector<QueryGramList<Array<PosID> >* >& qgls, vector<WeightedGramList<Array<PosID> >* >& wgls) const {
  
  sort(qgls.begin(), qgls.end(), OnDiskMergerAdapt<Array<PosID> >::cmpQglByGramCode);
  
  unsigned i = 0;
  while(i < qgls.size()) {    
    QueryGramList<Array<PosID> >* currentQGL = qgls.at(i);
    unsigned currentCount = 0;          

    WeightedGramList<Array<PosID> >* wgl = new WeightedGramList<Array<PosID> >();
    while(i < qgls.size()) {
      if(currentQGL->gramCode == qgls.at(i)->gramCode) {
	currentQGL = qgls.at(i);
	currentCount++;
	wgl->positions.push_back(currentQGL->pos);
	i++;	
      }
      else break;      
    }
    wgl->gramCode = currentQGL->gramCode;
    wgl->gramList = dynamic_cast<GramListOnDisk<Array<PosID> >*>(currentQGL->gl);
    wgl->weight = currentCount;
    wgls.push_back(wgl);
  }  
}

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
heapMerge(const Query& query, const string& prePostQueryStr, LeafContext<Array<PosID> >& lc, vector<unsigned>& results) const {
    
  typedef Array<PosID> InvList;
  unsigned numberLists = lc.initialLists;
  
  unsigned pointersIndex[numberLists];  
  memset(pointersIndex, 0, numberLists * sizeof(unsigned));
  
  Heap<HeapMergeElement<InvList> > heap(numberLists);
  
  // make initial heap
  for(unsigned i = 0; i < numberLists; i++) {
    unsigned j = 0;
    InvList* invList = lc.wgls->at(i)->gramList->getArray();
    unsigned currentID = invList->at(0).id;
    while(j < invList->size()) {
      PosID& e = invList->at(j);
      if(currentID == e.id) {
	heap.push(new HeapMergeElement<InvList>(&e, i));	
	pointersIndex[i]++;
      }
      else break;
      j++;
    }
  }
  
  HeapMergeElement<InvList>* poppedElements[numberLists];
  while( !heap.empty() ) {                       

    // Container of vector indexes which should be moved to the next position 
    unsigned poppedElementCount = 0;
    unsigned currentID = heap.head()->element->id;
    unsigned weight = 0;    
    Candidate<InvList>* c = new Candidate<InvList>(this->gramGen->getPrePostStrLen(this->charsumStats[currentID].length));
    
    while( currentID == heap.head()->element->id && poppedElementCount < numberLists) {         
      
      poppedElements[poppedElementCount++] = heap.head();
      unsigned currentListIndex = heap.head()->listIndex;
      vector<unsigned char>& wglPos = lc.wgls->at(heap.head()->listIndex)->positions;
      bool posMatched[wglPos.size()];
      memset(posMatched, 0, sizeof(bool) * wglPos.size());      
      
      unsigned sameListCount = 0;      
      while( currentListIndex == heap.head()->listIndex && currentID == heap.head()->element->id ) {	
	sameListCount++;
	
	processPosID(query, prePostQueryStr, lc.wgls->at(currentListIndex)->gramCode, currentID,
		     wglPos, heap.head()->element->pos, posMatched, c);
	
	heap.pop();
	if(heap.empty()) break; // for safety
      }
      
      // count matched grams from queries perspective and set mismatched grams from queries perspective
      unsigned matches = 0;
      for(unsigned i = 0; i < wglPos.size(); i++) {
	if(posMatched[i]) matches++;
	else c->mismatchPositions.insert( wglPos.at(i) );	  
      }
      
      // make the weight as tight as possible
      weight += min(sameListCount, matches);
      
      if(heap.empty()) break; // for safety
    }        
        
    // the weight may be 0 due to the position filter, disregard candidates with zero weight
    // also analyze mismatching positions to see if we can prune this candidate
    if(posFilters & POSFILTER_MISMATCH)
      c->checkMismatches(query, gramGen->getGramLength());
    
    if((weight > 0) && (!c->isPruned)) {
      
      if(posFilters & POSFILTER_LENGTH) c->isPruned |= checkLength(query, prePostQueryStr, *c);
      
      if( (posFilters & POSFILTER_DP) && (!c->isPruned) ) {
	c->checkPartialString(query, prePostQueryStr);
	
	if(!c->isPruned) {
	  c->id = currentID;
	  c->weight = weight;
	  lc.candidates.push_back(c);
	}
	else delete c;	
      }
      else {
	c->id = currentID;
	c->weight = weight;
	lc.candidates.push_back(c);
      }
    }
    else delete c;
    
    // move to next elements and insert to heap
    for(unsigned i = 0; i < poppedElementCount; i++) {
      unsigned index = poppedElements[i]->listIndex;
      unsigned position = pointersIndex[index];      
      InvList* invList = lc.wgls->at(index)->gramList->getArray();
      if(position < invList->size()) {
	unsigned currentID = invList->at(position).id;
	while(position < invList->size()) {
	  PosID& e = invList->at(position);
	  if(currentID == e.id) {
	    heap.push(new HeapMergeElement<InvList>(&e, index));	    
	    pointersIndex[index]++;
	  }
	  else break;
	  position++;
	}
      }      
      delete poppedElements[i];
    }                 
  } 
}

template<>
void 
OnDiskMergerAdapt<Array<PosID> >::
readNextLists(const Query& query,
	      const string& prePostQueryStr,
	      LeafContext<Array<PosID> >& lc, 
	      unsigned listsToRead, 
	      vector<unsigned>& results) {
  
  typedef Array<PosID> InvList;
  
  for(unsigned i = 0; i < listsToRead; i++) {          
    WeightedGramList<InvList>* wgl = lc.wgls->at(lc.currentList+i);
    GramListOnDisk<InvList>* gl = wgl->gramList;    
    InvList* array = gl->getArray(lc.invListsFile);
    lc.remainingWeight -= wgl->weight;    
    
    vector<unsigned char>& wglPos = wgl->positions;
    bool posMatched[wglPos.size()]; // remember matched positions from queries perspective
    
    // do lookup for every candidate
    bool cleanseNeeded = false;
    InvList::iterator start = array->begin();
    
    for(unsigned j = 0; j < lc.candidates.size(); j++) {      
      Candidate<InvList>* candidate = lc.candidates.at(j);
      if((signed)candidate->weightNeeded <= 0) continue;      
      
      InvList::iterator end = expProbe(start, array->end(), candidate->id);
      PosID toFind(candidate->id, 0);
      start = lower_bound(start, end, toFind);
            
      // iterate through inverted list from start as long as the string ids match
      unsigned listMatchCount = 0;
      memset(posMatched, 0, sizeof(bool) * wglPos.size());
      while(start != array->end()) {
	if(start->id == candidate->id) {
	  listMatchCount++;
	  processPosID(query, prePostQueryStr, wgl->gramCode, candidate->id, wglPos, start->pos, posMatched, candidate);
	}
	else break; // string ids no longer match	
	start++;
      }
      
      // count matched grams from queries perspective and set mismatched grams from queries perspective
      unsigned matches = 0;
      for(unsigned p = 0; p < wglPos.size(); p++) {
	if(posMatched[p]) matches++;
	else candidate->mismatchPositions.insert( wglPos.at(p) );
      }
      
      // keep weight as tight as possible      
      unsigned weight = min(listMatchCount, matches);
      if(posFilters & POSFILTER_MISMATCH)
	candidate->checkMismatches(query, gramGen->getGramLength());
      
      if(listMatchCount > 0 && (posFilters & POSFILTER_DP) ) 
	candidate->checkPartialString(query, prePostQueryStr);
      
      if(!candidate->isPruned) {      
	// check if we add any weight
	if(weight > 0) {
	  candidate->weight += weight;
	  if(candidate->weight >= lc.threshold) {
	    lc.candidateCounts[candidate->weightNeeded]--;
	    candidate->weightNeeded = 0;
	    results.push_back(candidate->id);	  
	    cleanseNeeded = true;
	  }
	}
	// we are adding no weight
	else {
	  lc.candidateCounts[candidate->weightNeeded]--;
	  candidate->weightNeeded = (candidate->weightNeeded > wgl->weight) ? candidate->weightNeeded - wgl->weight : 0;
	  if(candidate->weightNeeded > lc.remainingWeight) candidate->weightNeeded = 0;
	  if(candidate->weightNeeded > 0) lc.candidateCounts[candidate->weightNeeded]++;
	  else cleanseNeeded = true;
	}      
      }
      else { // we must remove the candidate
	lc.candidateCounts[candidate->weightNeeded]--;
	candidate->weightNeeded = 0;
	cleanseNeeded = true;
      }
    }      	
    
    // do we need to remove items from the candidate set
    // i.e. we can guarantee that some elements are either answers or cannot be answers
    if(cleanseNeeded) cleanseCandidates(lc.candidates);
    
    gl->clear();
  }
}
