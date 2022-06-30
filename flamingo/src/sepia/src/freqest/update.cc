/*
  $Id: update.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/23/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "update.h"

ostream& operator<<(ostream &out, const Update &u)
{
  return out << u.id << "\t" << u.str;
}
  
istream& operator>>(istream &in, Update &u)
{
  return in >> u.id >>  u.str;
}

typedef vector<Update> ContUpdates;

ostream& operator<<(ostream &out, const ContUpdates &c)
{
  out << c.size() << endl << endl;
  for (ContUpdates::const_iterator it = c.begin(); it != c.end(); ++it)
    out << *it << endl;
  return out;
}

istream& operator>>(istream &in, ContUpdates &c)
{
  unsigned sz;
  in >> sz;
  for (unsigned i = 0; i < sz; i++) {
    Update u;
    in >> u;
    c.push_back(u);
  }
  return in;
}
