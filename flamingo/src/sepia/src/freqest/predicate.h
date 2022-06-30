/*
  $Id: predicate.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _predicate_h_
#define _predicate_h_

#include <iostream>

#include "simfunc/simdist.h"

using namespace std;

class Predicate 
{
public:
  string query;
  SimType dist;
  unsigned freq;

  Predicate(): query(""), dist(SIM_TYPE_MIN) {}
  Predicate(const string &q, SimType d): query(q), dist(d) {}

  friend ostream& operator<<(ostream &out, const Predicate &p);
  friend istream& operator>>(istream &in, Predicate &p);
};

typedef vector<Predicate> VectPredicate;
typedef VectPredicate::iterator VectPredicateIt;

istream& operator>>(istream &in, VectPredicate &v);
ostream& operator<<(ostream &out, const VectPredicate &v);

#endif
