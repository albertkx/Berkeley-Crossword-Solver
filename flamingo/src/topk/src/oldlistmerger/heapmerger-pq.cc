
/*
  $Id: heapmerger-pq.cc 5716 2010-09-09 04:27:56Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of 
  the BSD license.

  This imeplementation merges multiple lists by building a priority queue
  (heap) based on the top elements of the lists.  The lists are assumed to
  be sorted in an ascending order.
 
  Date: 05/14/2007
  Author: Chen Li <chenli (at) ics.uci.edu>

*/

#include <iostream>
#include <vector>
#include <queue>
#include <assert.h>

#include "listmerger.h"
#include "util/src/array.h"

// http://support.microsoft.com/kb/837697
typedef pair<Array<unsigned>*, unsigned> ArrayWithPosition;

// Overload the < operator.
bool operator< (const ArrayWithPosition &awp1, const ArrayWithPosition &awp2)
{
  return awp1.first->at(awp1.second) < awp2.first->at(awp2.second);
}

// Overload the > operator.
bool operator> (const ArrayWithPosition& awp1, const ArrayWithPosition &awp2)
{
  return awp1.first->at(awp1.second) > awp2.first->at(awp2.second);
}

void HeapMerger::merge(const vector<Array<unsigned>*> &arrays,
		       const unsigned threshold,
		       const unsigned maxObjectID,
		       vector<unsigned> &results)
{
  // build a heap. Each element is a list with its current position (0)
  priority_queue<ArrayWithPosition> pq;
  for (unsigned i = 0; i < arrays.size(); i ++)
    pq.push((make_pair(arrays.at(i),0)));

  // remove any leftover in the results
  results.clear();

  // iterate over the queue
  unsigned prevObjectId = 0;
  unsigned objFreqency = 0;
  while (!pq.empty()) {
    // remove the head
    ArrayWithPosition awp = pq.top();
    pq.pop();

    // check the object id of the current element
    //assert(awp.first != NULL);
    //assert(awp.first->size() > awp.second);

    unsigned newObjectId = awp.first->at(awp.second);
    if (newObjectId == prevObjectId) {
      objFreqency ++;
    }
    else {
      prevObjectId = newObjectId; // a new object id
      objFreqency = 1;
    }

    if (objFreqency >= threshold) // found a new candidate
      if (!results.empty() && results.back() != newObjectId) // ignore duplcates in the results
	results.push_back(newObjectId); // insert this candidate

    // increment the position of the list by one
    awp.second ++;
    if (awp.second < awp.first->size()) // the list has more elments
      pq.push(awp);
  }

  // return the results
}

