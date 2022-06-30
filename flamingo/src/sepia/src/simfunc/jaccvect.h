/*
  $Id: jaccvect.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _jaccvect_h_
#define _jaccvect_h_

#include <iostream>

using namespace std;

class JaccVect {
  // <intersection, union, editdistance>
public:
  unsigned inte, unio, edit;

  JaccVect(): inte(0), unio(0), edit(0) {}
  JaccVect(unsigned i, unsigned u, unsigned e): inte(i), unio(u), edit(e) {}
  JaccVect(const string &a, const string &b);

  bool operator==(const JaccVect &j) const {
    return this == &j || (inte == j.inte && unio == j.unio && edit == j.edit); }

  operator size_t() const {
    return static_cast<size_t>((inte % 10) * 100 + (unio % 10) * 10 + edit % 10); }

  float getDist() const { return 1-static_cast<float>(inte) / unio; }

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const JaccVect &ev);
  friend istream& operator>>(istream &in, JaccVect &ev);
  friend bool operator<(const JaccVect &left, const JaccVect &right);
  friend bool operator!=(const JaccVect &left, const JaccVect &right);
};

#endif
