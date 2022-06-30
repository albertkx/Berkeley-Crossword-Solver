/*
  $Id: freqest.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _freqest_h_
#define _freqest_h_

#include "cluster/clusters.h"
#include "predicate.h"

float freqEstFunc(const vector<string> &d, const Clusters &cs, Predicate p);
float freqEstPPDOnlineFunc(const vector<string> &d, const Clusters &cs,
                           Predicate p);
unsigned freqRealFunc(const vector<string> &d, Predicate p);
unsigned freqRealFunc(const vector<string> &d, Predicate p, vector<string> &r);
float freqPPDFunc(const vector<string> &d, const Clusters &cs, Predicate p);

#endif
