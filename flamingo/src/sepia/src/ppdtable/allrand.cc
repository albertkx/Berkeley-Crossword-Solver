/*
  $Id: allrand.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "allrand.h"

AllRand::AllRand(const vector<string> *d, Clusters *c, ContQueryPivot *q, 
                 const unsigned samplePer): 
  PPDSample(d, c, q, samplePer) 
{
  // init - sample
  sample = Sample(max(1u, 
                      static_cast<unsigned>(static_cast<float>(samplePer) / 
                                            100 * data->size())), 
                  data->size());

  // init - loop
  sampleIt = sample.begin();  
  clusterIt = clusters->beginCluster();
  stringIt = clusterIt->begin();
}

void AllRand::step() 
{
  stringIt++;
  if (stringIt == clusterIt->end()) {
    ++clusterIt;
    if (clusterIt == clusters->endCluster())
      {
        sampleIt++;
        if (sampleIt == sample.end()) 
          return;
        clusterIt = clusters->beginCluster();
      }
    stringIt = clusterIt->begin();
  }
}

bool AllRand::hasNext() const 
{
  if (sampleIt != sample.end()) 
    return true;
  return false;
}

PPDTriple AllRand::next() 
{
  const unsigned q = *sampleIt;
  const unsigned p = clusterIt->getPivot();
  const unsigned s = *stringIt;

  PPDTriple triple = PPDTriple(q,
                               p,
                               s,
                               SimVect((*data)[q], (*data)[p]),
                               SimVect((*data)[p], (*data)[s]),
                               SimDist((*data)[q], (*data)[s]));

  // increment
  step();

  return triple;
}
