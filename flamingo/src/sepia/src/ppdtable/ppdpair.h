/*
  $Id: ppdpair.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ppdpair_h_
#define _ppdpair_h_

#include <set>

#include "simfunc/simdist.h"
#include "simfunc/simvect.h"

using namespace std;

class PPDPair {
public:
  SimVect vect1, vect2;
  SimType maxD;
  unsigned countP;

  PPDPair(SimVect v1, SimVect v2, SimType mD): 
    vect1(v1), vect2(v2), maxD(mD), countP(1) {}

  friend ostream& operator<<(ostream &out, const PPDPair &p);
  friend bool operator<(const PPDPair &left, const PPDPair &right);
};

typedef set<PPDPair> SetPPDPair;
typedef SetPPDPair::iterator SetPPDPairIt;

#endif
