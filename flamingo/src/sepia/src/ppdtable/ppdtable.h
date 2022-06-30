/*
  $Id: ppdtable.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _ppdtable_h_
#define _ppdtable_h_

#include <iostream>

#include "ppdentry.h"

using namespace std;

class PPDTable 
{
protected:
  ContPPDEntry *table;
public:
  PPDTable(): table(new ContPPDEntry) {}
  PPDTable(const PPDTable &t);

  ~PPDTable() { delete table; } 

  PPDTable& operator=(const PPDTable &t);
  bool operator==(const PPDTable &t) const;

  const size_t sizeTable() const { return table->size(); }

  void insert(const PPDEntry &e);
  void erase(const PPDEntry &e);
  void pruneMax();

  const ContPPDEntryIt beginTable() const { return table->begin(); }
  const ContPPDEntryIt endTable() const { return table->end(); }
  const ContPPDEntryIt findTable(const PPDEntry &e) const {
    return table->find(e); } 
  const ContPPDEntryIt lower_boundTable(const PPDEntry &e) const {
    return table->lower_bound(e); }
  const ContPPDEntryIt upper_boundTable(const PPDEntry &e) const {
    return table->upper_bound(e); }

#if SIM_DIST == 1 && SIM_VECT == 1
  void serialize(ofstream &out);
  void deserialize(ifstream &in);
#endif

  friend ostream& operator<<(ostream &out, const PPDTable &t);
  friend istream& operator>>(istream &in, PPDTable &t);
};

#endif
