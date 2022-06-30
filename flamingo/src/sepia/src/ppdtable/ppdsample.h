/*
  $Id: ppdsample.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ppdsample_h_
#define _ppdsample_h_

#include "cluster/clusters.h"
#include "ppdtriple.h"

class PPDSample 
{
protected:
  Sample sample;
  const vector<string> *data;
  Clusters *clusters;
  ContQueryPivot *queryPivot;

  const unsigned samplePer;   // (20)

public:
  PPDSample(const vector<string> *d, Clusters *c, ContQueryPivot *q,
            const unsigned samplePer = 20): 
    data(d), clusters(c), queryPivot(q), samplePer(samplePer) {}
  virtual ~PPDSample() {};

  virtual bool hasNext() const = 0;
  virtual PPDTriple next() = 0;

  ostream& info(ostream& out);
};

#endif
