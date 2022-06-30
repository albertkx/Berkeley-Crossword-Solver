/*
  $Id: closeunique.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _closeunique_h_
#define _closeunique_h_

#include "ppdsample.h"
#include "indexentry.h"

class CloseUnique: public PPDSample 
{
private: 
  const unsigned queueSize, uniqueNo;
  unsigned sampleIdx, sampleMax;
  ContIndexEntryVect samplePair;
  Cluster cluster;
  ContCluster::const_iterator stringIt;
  set<SimVect> uniqueVect;

public:
  CloseUnique(const vector<string> *d, Clusters *c, ContQueryPivot *q, 
              const unsigned samplePer,
              const unsigned queueSize, const unsigned uniqueNo);
  bool hasNext() const;
  PPDTriple next();

private:
  void buildSample();
  unsigned countUnique();
  void step();
};

#endif
