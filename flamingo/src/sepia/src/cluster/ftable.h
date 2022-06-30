/*
  $Id: ftable.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/20/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ftable_h_
#define _ftable_h_

#include <iterator>
#include <map>

#include "simfunc/simvect.h"

using namespace std;

typedef map<SimVect, unsigned> ContFTable;

class FTable {
private:
  ContFTable *cont;
public:
  FTable(): cont(new ContFTable()) {}
  FTable(const FTable &t);
  ~FTable() { delete cont; }

  FTable& operator=(const FTable &t);
  bool operator==(const FTable &t) const;

  const size_t size() const { return cont->size(); }

  void insert(SimVect vect) { (*cont)[vect]++; }
  void erase(SimVect vect);

  const ContFTable::iterator begin() const { return cont->begin(); }
  const ContFTable::iterator end() const { return cont->end(); }

  friend ostream& operator<<(ostream &out, const FTable &t);
  friend istream& operator>>(istream &in, FTable &t);
};

#endif
