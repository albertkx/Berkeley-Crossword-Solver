/*
  $Id: ppdpair.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ppdpair.h"

ostream& operator<<(ostream &out, const PPDPair &p) {
  return out << '[' << p.vect1 << ',' << p.vect2 << ','
             << p.maxD << ',' << p.countP << ']';
}

bool operator<(const PPDPair &left, const PPDPair &right) {
  if (&left == &right)
    return false;
  if (left.vect1 != right.vect1)
    return left.vect1 < right.vect1;
  return left.vect2 < right.vect2;
}
