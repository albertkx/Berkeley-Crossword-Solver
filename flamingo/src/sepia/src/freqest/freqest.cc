/*
  $Id: freqest.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "freqest.h"

#include <sys/time.h>

float freqEstFunc(const vector<string> &d, const Clusters &cs, Predicate p)
{
  float est = 0;
  float fract;

  for (VectClusterIt i = cs.beginCluster(); i != cs.endCluster(); i++) {
    float estCluster = 0;
    SimVect v1 = SimVect(p.query, d[i->getPivot()]);
    SimType v1d = v1.getDist();

    if (v1d > i->getRadius() + p.dist)
      continue;

    for (ContFTable::const_iterator j = i->getFTable().begin(); 
         j != i->getFTable().end(); j++) {
      SimVect v2 = j->first;
      SimType v2d = v2.getDist();

      if (v1d + v2d <= p.dist) {
        estCluster += j->second;
        continue;
      }

      if (v1d > p.dist + v2d || v2d > p.dist + v1d)
        continue;

      PPDEntry e=PPDEntry(v1, v2, p.dist, 0, 0);

      ContPPDEntryIt itE = cs.findPPDtable(e);

      if (itE == cs.endPPDtable()) {
        itE = cs.lower_boundPPDtable(e);
        --itE;
        // !!! freqEst assumes that ContPPDEntry is an ordered container !!!
        if (itE->vect1 != v1 || itE->vect2 != v2)
          // !!! itE == cs.endPPDtable() || !!! || itE->distM>p.dist !!!
          continue;
      }
      
      fract = itE->fract;
      estCluster += fract * j->second;
    }
    est += estCluster;
  }

  return est;
}

float freqEstPPDOnlineFunc(const vector<string> &d, const Clusters &cs,
                           Predicate p)
{
  float est = 0;
  float fract;

  for (VectClusterIt i = cs.beginCluster(); i != cs.endCluster(); i++) {
    float estCluster = 0;
    SimVect v1 = SimVect(p.query, d[i->getPivot()]);
    SimType v1d = v1.getDist();

    if (v1d > i->getRadius() + p.dist)
      continue;

    for (ContFTable::const_iterator j = i->getFTable().begin(); 
         j != i->getFTable().end(); j++) {
      SimVect v2 = j->first;
      SimType v2d = v2.getDist();

      if (v1d + v2d <= p.dist) {
        estCluster += j->second;
        continue;
      }

      if (v1d > p.dist + v2d || v2d > p.dist + v1d)
        continue;

      PPDEntry e=PPDEntry(v1, v2, p.dist, 0, 0);

      ContPPDEntryIt itE = cs.findPPDtable(e);

      if (itE == cs.endPPDtable()) {
        itE = cs.lower_boundPPDtable(e);
        --itE;
        // !!! freqEst assumes that ContPPDEntry is an ordered container !!!
        if (itE->vect1 != v1 || itE->vect2 != v2)
          // !!! itE == cs.endPPDtable() || !!! || itE->distM>p.dist !!!
          continue;
      }

      ContPPDEntryIt itENext = itE;
      itENext++;
      while (itE->vect1 == itENext->vect1 && itE->vect2 == itENext->vect2)
        itENext++;
      itENext--;
      
      fract = static_cast<float>(itE->countE) / itENext->countE;
      
      estCluster += fract * j->second;
    }
    est += estCluster;
  }

  return est;
}

unsigned freqRealFunc(const vector<string> &d, Predicate p, vector<string> &r)
{
  for (vector<string>::const_iterator i=d.begin(); i!=d.end(); i++)
    if (SimDist(p.query, *i) <= p.dist)
      r.push_back(*i);
  return r.size();
}

unsigned freqRealFunc(const vector<string> &d, Predicate p)
{
  vector<string> r;
  return freqRealFunc(d, p, r);
}  

float freqPPDFunc(const vector<string> &d, const Clusters &cs, Predicate p)
{
  float count = 0;

  for (VectClusterIt i = cs.beginCluster(); i != cs.endCluster(); i++) {
    SimVect v1 = SimVect(p.query, d[i->getPivot()]);

    for (ContCluster::const_iterator j = i->begin(); j != i->end(); j++) {
      SimVect v2 = SimVect(d[i->getPivot()], d[*j]);
      PPDEntry e = PPDEntry(v1, v2, p.dist, 0, 0);

      ContPPDEntryIt itE = cs.findPPDtable(e);
      if (itE == cs.endPPDtable()) {
        itE = cs.lower_boundPPDtable(e); 
        --itE;
        // !!! freqEst assumes that ContPPDEntry is an ordered container !!!
        if (itE->vect1 != v1 || itE->vect2 != v2) {
          // !!! itE == cs.endPPDtable() || !!! || itE->distM>p.dist !!!
          continue;
        }				
      }
      count += itE->fract;
    }
  }
  return count;
}
