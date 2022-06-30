/*
  $Id: ppdentry.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ppdentry_h_
#define _ppdentry_h_

#include <set>

#include "simfunc/simdist.h"
#include "simfunc/simvect.h"

using namespace std;

#if SIM_DIST == 1
const unsigned DIST_THRESHOLD = 5;
#elif SIM_DIST == 2
const float DIST_THRESHOLD = static_cast<float>(0.96);
#endif

class PPDEntry {
public:
  SimVect vect1, vect2;
  SimType distM;
  unsigned countE;
  float fract;

  PPDEntry(): distM(0), countE(0), fract(0) {}
  PPDEntry(SimVect v1, SimVect v2, SimType d, unsigned c = 1, float f = 0);

  bool operator==(const PPDEntry &e) const;

  operator size_t() const {
    return static_cast<size_t>(vect1) * 100000 + 
      static_cast<size_t>(vect2) * 100 + SimType2size_t(distM); }

#if SIM_DIST == 1 && SIM_VECT == 1
  void serialize(ofstream &out) const;
  void deserialize(ifstream &in);
#endif

  friend ostream& operator<<(ostream &out, const PPDEntry &e);
  friend istream& operator>>(istream &in, PPDEntry &e);
  friend bool operator<(const PPDEntry &left, const PPDEntry &right);
};

typedef set<PPDEntry> ContPPDEntry; 
/* !!! FreqEst assumes that ContPPDEntry is an ordered container !!!
   !!! PPDTable assumes that ContPPDEntry is an ordered container !!!
   !!! PPDTable assumes that ContPPDEntry is a container where 
   remove does not imvalidate iterators !!!
*/
typedef ContPPDEntry::iterator ContPPDEntryIt;

ostream& operator<<(ostream &out, const ContPPDEntry &c);
istream& operator>>(istream &in, ContPPDEntry &c);

#endif
