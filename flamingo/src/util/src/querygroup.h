/*
  $Id: querygroup.h 5775 2010-10-20 01:33:58Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license.

  Date: 11/07/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _querygroup_h_
#define _querygroup_h_

#include "common/src/simmetric.h"

#include <string>

class QueryGroup
{
public:
  SimMetric &sim;  
  const uint k;
  const uint n;

  std::string* strs;
  uint* lens;
  uint* nosGrams;

  template<class InputIterator> 
  QueryGroup(
    InputIterator ss, uint n, SimMetric &sim, uint k):
    sim(sim), 
    k(k), 
    n(n)
  {
    strs = new std::string[n];
    lens = new uint[n];
    nosGrams = new uint[n];

    for (uint i = 0; i < n; ++i) {
      strs[i] = *ss;
      lens[i] = ss->length();
      nosGrams[i] = sim.gramGen.getNumGrams(*ss);
      ++ss;
    }
  }
};

#endif
