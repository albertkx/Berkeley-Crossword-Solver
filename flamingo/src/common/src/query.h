/*
  $Id: query.h 6074 2011-04-26 03:52:15Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the 
  BSD license.
  
  Date: 04/22/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _query_h_
#define _query_h_

#include "typedef.h"
#include "simmetric.h"

typedef enum
{
  QueryNotDefined = 0,
  QueryRange = 1, 
  QueryRanking = 2,
  QueryTopk = 3
} QueryType;

class Query 
{
public:
  const std::string str;
  const uint len;
  uint noGrams;                 // temporary not const, see setNoGrams
  SimMetric &sim;
  
  float threshold;
  const uint k;
  const QueryType type;

  Query(std::string str, SimMetric &sim, float threshold):
    str(str), 
    len(str.length()), 
    noGrams(sim.gramGen.getNumGrams(str)), 
    sim(sim),
    threshold(threshold), 
    k(0), 
    type(QueryRange) 
    {
      setNoGrams();
    }

  Query(std::string str, SimMetric &sim, uint k):
    str(str), 
    len(str.length()), 
    noGrams(sim.gramGen.getNumGrams(str)), 
    sim(sim),
    threshold(0), 
    k(k), 
    type(QueryRanking) 
    {
      setNoGrams();
    }

  Query(
    std::string str, 
    SimMetric &sim, 
    float threshold, 
    uint k, 
    QueryType type):
    str(str), 
    len(str.length()), 
    noGrams(sim.gramGen.getNumGrams(str)), 
    sim(sim), 
    threshold(threshold), 
    k(k), 
    type(type) 
    {
      setNoGrams();
    }

    Query(
    std::string str, 
    SimMetric &sim, 
    float k, 
    QueryType type):
    str(str), 
    len(str.length()), 
    noGrams(sim.gramGen.getNumGrams(str)), 
    sim(sim), 
    threshold(k), 
    k(k), 
    type(type) 
    {
      setNoGrams();
    }

private:
  // temporary, until Alex implements the set & bag semantics in GramGen
  // set semantics !
  void setNoGrams() 
    {
      std::set<uint> g;
      sim.gramGen.decompose(str, g);
      noGrams = g.size();
    }
};

#endif
