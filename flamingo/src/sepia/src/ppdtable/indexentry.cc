/*
  $Id: indexentry.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "indexentry.h"

bool operator<(const IndexEntry &left, const IndexEntry &right) 
{
  if (&left == &right)
    return false;
  if (left.dist != right.dist) 
    {
      return left.dist < right.dist;
    }
  return left.index < right.index;
}

ostream& operator<<(ostream &out, const IndexEntry &e)
{
  return out << e.index << "\t" << e.dist;
}

bool operator<(const IndexEntryVect &left, const IndexEntryVect &right) 
{
  if (&left == &right)
    return false;
  return left.vect.getDist() < right.vect.getDist();
}
