/*
  $Id: closelex.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "closelex.h"

#include <algorithm>

class CmpIdx
{
private:
  const vector<string> *data;
public:
  CmpIdx(const vector<string> *d): data(d) {}
  bool operator()(unsigned idx1, unsigned idx2) { 
    return (*data)[idx1] < (*data)[idx2]; }
};

CloseLex::CloseLex(const vector<string> *d, Clusters *c, ContQueryPivot *q, 
                   const unsigned samplePer, const unsigned queueSize):
  PPDSample(d, c, q, samplePer), queueSize(queueSize)
{
  // init - sample
  index.reserve(d->size());
  for (unsigned i = 0; i < d->size(); i++) index.push_back(i);
  sort(index.begin(), index.end(), CmpIdx(data));
  sample = Sample(max(1u, 
                      static_cast<unsigned>(static_cast<float>(samplePer) / 
                                            100 * data->size())), 
                  data->size());

  // init - loop
  sampleIt=sample.begin();
  buildQueue();
  unsigned j = samplePair.begin()->index;
  samplePair.erase(samplePair.begin());
  cluster = clusters->getCluster(j);
  stringIt = cluster.begin();
}

void CloseLex::step() 
{
  stringIt++;
  if (stringIt == cluster.end()) {
    if (samplePair.empty()) {
      sampleIt++;
      if (sampleIt == sample.end()) 
        return;
      buildQueue();
    }
    unsigned i = samplePair.begin()->index;
    samplePair.erase(samplePair.begin());
    cluster = clusters->getCluster(i);
    stringIt = cluster.begin();
  }
}

bool CloseLex::hasNext() const 
{
  if (sampleIt != sample.end())
    return true;
  return false;
}

PPDTriple CloseLex::next() 
{
  const unsigned q = index[*sampleIt];
  const unsigned p = cluster.getPivot();
  const unsigned s = *stringIt;

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

void CloseLex::buildQueue() 
{
  for (unsigned i = 0; i < clusters->sizeCluster(); i++) {
    const unsigned p = clusters->getCluster(i).getPivot();
    const unsigned q = *sampleIt;
    SimType dist = SimDist((*data)[q], (*data)[p]);
    if (samplePair.size() < queueSize || dist<samplePair.begin()->dist) {
      samplePair.insert(IndexEntry(i, dist));
      if (samplePair.size() > queueSize)
        samplePair.erase(samplePair.begin());
    }
  }
}
