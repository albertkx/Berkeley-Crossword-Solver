/*
  $Id: indexentry.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _indexentry_h_
#define _indexentry_h_

#include <set>

#include "simfunc/simdist.h"
#include "simfunc/simvect.h"

using namespace std;

class IndexEntry
{
public:
  unsigned index;
  SimType dist;
 
  IndexEntry(unsigned i, SimType d): index(i), dist(d) {}

  friend ostream& operator<<(ostream &out, const IndexEntry &e);
  friend bool operator<(const IndexEntry &left, const IndexEntry &right);
};

typedef set<IndexEntry> ContIndexEntry;

class IndexEntryVect
{
public:
  unsigned index;
  SimVect vect;
 
  IndexEntryVect(unsigned i, SimVect v): index(i), vect(v) {}

  friend bool operator<(const IndexEntryVect &left, const IndexEntryVect &right);
};

typedef set<IndexEntryVect> ContIndexEntryVect;

#endif
