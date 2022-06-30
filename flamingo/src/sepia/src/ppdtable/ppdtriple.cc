/*
  $Id: ppdtriple.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ppdtriple.h"

ostream& operator<<(ostream &out, const PPDTriple &t)
{
  return out << t.query << "  " << t.pivot << "  " << t.str << "  "
             << t.vect1 << "  " << t.vect2 << "  " << t.dist;
}

istream& operator>>(istream &in, PPDTriple &t)
{
  return in >> t.query >> t.pivot >> t.str >> t.vect1 >> t.vect2 >> t.dist;
}

bool operator<(const PPDTriple &left, const PPDTriple &right)
{
  if (&left == &right)
    return false;
  if (left.query != right.query)
    return left.query < right.query;
  if (left.pivot != right.pivot)
    return left.pivot < right.pivot;
  return left.str < right.str;
}
