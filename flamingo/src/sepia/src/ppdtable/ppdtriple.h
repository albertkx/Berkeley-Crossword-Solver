/*
  $Id: ppdtriple.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ppdtriple_h_
#define _ppdtriple_h_

#include <list>

#include "simfunc/simdist.h"
#include "simfunc/simvect.h"

using namespace std;

class PPDTriple {
public:
  unsigned query, pivot, str;
  SimVect vect1, vect2;
  SimType dist;

  PPDTriple(): query(0), pivot(0), str(0), dist(0) {}
  PPDTriple(unsigned q, unsigned p, unsigned s, SimVect v1, SimVect v2, SimType d): 
    query(q), pivot(p), str(s), vect1(v1), vect2(v2), dist(d) {}

  friend ostream& operator<<(ostream &out, const PPDTriple &t);
  friend istream& operator>>(istream &in, PPDTriple &t);
  friend bool operator<(const PPDTriple &left, const PPDTriple &right);
};

typedef list<PPDTriple> ListPPDTriple;
typedef ListPPDTriple::iterator ListPPDTripleIt;

#endif
