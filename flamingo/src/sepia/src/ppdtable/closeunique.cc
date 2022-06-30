/*
  $Id: closeunique.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "closeunique.h"

#include <algorithm>

CloseUnique::CloseUnique(const vector<string> *d, Clusters *c, ContQueryPivot *q,
                         const unsigned samplePer,
                         const unsigned queueSize, const unsigned uniqueNo):
  PPDSample(d, c, q, samplePer), queueSize(queueSize), uniqueNo(uniqueNo)
{
  // init - sample
  sample = Sample(data->size());
  sampleMax = static_cast<unsigned>(countUnique() * .4);
  buildSample();

  // init - loop
  sampleIdx = 0;
  unsigned j=samplePair.begin()->index;
  samplePair.erase(samplePair.begin());
  cluster=clusters->getCluster(j);
  stringIt=cluster.begin();
}

void CloseUnique::step() 
{
  stringIt++;
  if (stringIt == cluster.end()) {
    if (samplePair.empty()) {
      sampleIdx++;
      if (sampleIdx < max(1u, 
                          static_cast<unsigned>(static_cast<float>(samplePer) / 
                                                100 * data->size())))
        buildSample();
      else
        return;
    }
    unsigned i = samplePair.begin()->index;
    samplePair.erase(samplePair.begin());
    cluster = clusters->getCluster(i);
    stringIt = cluster.begin();
  }
}

bool CloseUnique::hasNext() const 
{
  if (sampleIdx < sampleMax && sample.hasNext())
    return true;
  return false;
}

PPDTriple CloseUnique::next() 
{
  const unsigned q=sampleIdx;
  const unsigned p=cluster.getPivot();
  const unsigned s=*stringIt;

  PPDTriple triple=PPDTriple(q,
                             p,
                             s,
                             SimVect((*data)[q], (*data)[p]),
                             SimVect((*data)[p], (*data)[s]),
                             SimDist((*data)[q], (*data)[s]));

  // increment
  step();

  return triple;
}

void CloseUnique::buildSample() 
{
  unsigned sampleCrt = 0; 
  bool done = false;
  set<SimVect>
    newVect, 
    diffVect;

  while (!done) {
    if (!sample.hasNext())
      break;
    sampleCrt = sample.generate();
    samplePair.clear();
    newVect.clear();

    for (unsigned i = 0; i < clusters->sizeCluster(); i++) {
      const unsigned p = clusters->getCluster(i).getPivot();
      SimVect vect = SimVect((*data)[sampleCrt], (*data)[p]);
      if (samplePair.size() < queueSize || 
          vect.getDist() < samplePair.begin()->vect.getDist()) {
        samplePair.insert(IndexEntryVect(i, vect));
        newVect.insert(vect);
        if (samplePair.size() > queueSize) {
          samplePair.erase(samplePair.begin());
          newVect.erase(vect);
        }
      }
    }

    diffVect.clear();
    set_difference(newVect.begin(), newVect.end(), 
                   uniqueVect.begin(), uniqueVect.end(), 
                   inserter(diffVect, diffVect.begin()));
    if (diffVect.size() >= uniqueNo) 
      done = true;
  }
  
  sample.push_back(sampleCrt);
  copy(diffVect.begin(), diffVect.end(), inserter(uniqueVect, uniqueVect.begin()));
}

unsigned CloseUnique::countUnique() 
{
  unsigned sampleCrt = 0; 
  set<SimVect>
    diffVect;

  for (unsigned j = 0; j < data->size(); j++) {      
    sampleCrt = j;

    for (unsigned i = 0; i < clusters->sizeCluster(); i++) {
      const unsigned p = clusters->getCluster(i).getPivot();
      SimVect vect = SimVect((*data)[sampleCrt], (*data)[p]);

      diffVect.insert(vect);	
    }
  }
  
  return diffVect.size();
}
