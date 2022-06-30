/*
  $Id: closelex.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _closelex_h_
#define _closelex_h_

#include "ppdsample.h"
#include "indexentry.h"

class CloseLex: public PPDSample
{
private:
  const unsigned queueSize;
  vector<unsigned>::const_iterator sampleIt;
  ContIndexEntry samplePair;
  vector<unsigned> index;
  Cluster cluster;
  ContCluster::const_iterator stringIt;

public:
  CloseLex(const vector<string> *d, Clusters *c, ContQueryPivot *q,
           const unsigned samplePer, const unsigned queueSize);
  bool hasNext() const;
  PPDTriple next();

private:
  void buildQueue();
  void step();
};

#endif
