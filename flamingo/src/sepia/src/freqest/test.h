/*
  $Id: test.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _test_h_
#define _test_h_

#include "errorcorr.h"

class Test 
{
private:
  vector<string> *data;
  Clusters *clusters;
  const ErrorCorr *errorCorr;
  SimType thresholdMin, thresholdMax;
  bool doBuck;

  VectPredicate vectTest;

  float sumRelErr, sumRelErrCor;
  unsigned noRelErr, buck[20];

public:
  Test(vector<string>* dat, Clusters* clust, 
       SimType thresholdMin, SimType thresholdMax, bool doBuck = false);

  void create(const set<unsigned> &setQuery);
  void test();

  size_t size() const { return vectTest.size(); }
  void setErrorCorr(const ErrorCorr &errorCorr) { this->errorCorr = &errorCorr; }

  ostream& info(ostream &out) const;

  friend ostream& operator<<(ostream &out, const Test &t);
  friend istream& operator>>(istream &in, Test &t);
};

#endif

