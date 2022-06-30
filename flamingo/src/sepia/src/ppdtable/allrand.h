/*
  $Id: allrand.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _allrand_h_
#define _allrand_h_

#include "ppdsample.h"

class AllRand: public PPDSample 
{
private:
  vector<unsigned>::const_iterator sampleIt;
  VectClusterIt clusterIt;
  ContCluster::const_iterator stringIt;

public:
  AllRand(const vector<string> *d, Clusters *c, ContQueryPivot *q, 
          const unsigned samplePer);
  bool hasNext() const;
  PPDTriple next();

private:
  void step();
};

#endif
