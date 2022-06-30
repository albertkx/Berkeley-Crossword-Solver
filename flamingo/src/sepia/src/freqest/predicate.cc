/*
  $Id: predicate.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "predicate.h"

#include <iterator>

using namespace std;

istream& operator>>(istream &in, Predicate &p)
{
  char buf[DATASET_LEN];
  in.get(buf, DATASET_LEN, in.widen('\t'));
  p.query.assign(buf);
  in >> p.dist >> p.freq;
  in.getline(buf, DATASET_LEN, in.widen('\n'));
  return in;
}

istream& operator>>(istream &in, VectPredicate &v)
{
  unsigned n;
  in>>n;
  char buf[DATASET_LEN];
  in.getline(buf, DATASET_LEN, in.widen('\n'));
  in.getline(buf, DATASET_LEN, in.widen('\n'));
  for (unsigned i=0; i<n; i++) {
    Predicate p;
    in>>p;
    v.push_back(p);
  }
  return in;
}

ostream& operator<<(ostream &out, const Predicate &p) 
{
  return out<<p.query<<"\t"<<p.dist << "\t" << p.freq;
}

ostream& operator<<(ostream &out, const VectPredicate &v) 
{
  out<<static_cast<unsigned>(v.size())<<endl<<endl;
  copy(v.begin(), v.end(), ostream_iterator<Predicate>(out, "\n"));
  return out;
}
