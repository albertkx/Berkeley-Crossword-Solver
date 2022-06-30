/*
  $Id: test.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "test.h"

#include <cmath>
#include <cstdlib>

#include "freqest.h"

Test::Test(vector<string>* dat, Clusters* clust,
           SimType thresholdMin, SimType thresholdMax, bool doBuck):
  data(dat), clusters(clust), errorCorr(NULL), 
  thresholdMin(thresholdMin), thresholdMax(thresholdMax), doBuck(doBuck)
{
  if (doBuck) 
    for (unsigned i = 0; i < 20; i++)
      buck[i] = 0;
}

void Test::create(const set<unsigned> &setQuery) 
{
  cerr << "...build test" << endl;
  for (set<unsigned>::iterator it=setQuery.begin(); it!=setQuery.end(); it++) {
    SimType dist;

#if SIM_DIST == 1
    dist = static_cast<SimType>(rand() % (thresholdMax - thresholdMin + 1)
                                + thresholdMin);
#elif SIM_DIST == 2
    dist = static_cast<SimType>(rand() % 
                                static_cast<int>(thresholdMax * 10 -
                                                 thresholdMin * 10 + 1)) /
      10 + thresholdMin;
#endif

    Predicate p = Predicate((*data)[*it], dist);
    p.freq = freqRealFunc(*data, p);

    vectTest.push_back(p);
  }
}

void Test::test() 
{
  cerr << "...test" << endl;

  sumRelErr=0;
  sumRelErrCor=0;
  noRelErr = 0;

  for (unsigned i = 0; i < vectTest.size(); i++) {
    Predicate p=vectTest[i];
    float f, fCor;
    unsigned r;

    f = freqEstFunc(*data, *clusters, p); 

    r = p.freq;

    if (r != 0) {
      float  relErr = (f - r) / r;
      sumRelErr += fabs(relErr);
	
      if (errorCorr) {
        float rCor = errorCorr->getError(Record(p, f, r));

        if (rCor == -1)
          fCor = f * 2;
        else
          fCor = f / (rCor + 1);

        float relErrCor = (fCor - r) / r;

        sumRelErrCor += fabs(relErrCor);

        // buckets
        if (doBuck) {
          int idx = static_cast<int>(relErrCor * 4) + 5;
          buck[idx]++;
        }
      }

      noRelErr++;
    }
    else
      cerr << "...absolute error" << endl;
  }
}

ostream& Test::info(ostream &out) const
{
  out << "Test" << endl << "---" << endl;
  out << "test size\t" << vectTest.size() << endl;
  out << "threshold\t" << thresholdMin << " " << thresholdMax << endl;
  out << endl;
  
  int relativeError = static_cast<int>((sumRelErr / noRelErr) * 100);
  int relativeErrorCorrected = static_cast<int>((sumRelErrCor 
                                                 / noRelErr) * 100);

  out << "Relative error\t" << relativeError
      << "%" << endl;
  if (errorCorr) {
    out << "Relative error corrected\t" << relativeErrorCorrected
        << "%" << endl;
    out << "Relative error improvement\t" 
        << relativeError - relativeErrorCorrected
        << "%" << endl;
  }

  if (doBuck) {
    out << "Relative error buckets\t10" << endl;
    for (int i = 0; i < 20; i++)
      out << (-125 + i * 25) << "\t" 
          << static_cast<float>(buck[i]) / vectTest.size() << endl;
  }

  return out << endl;
}

ostream& operator<<(ostream &out, const Test &t) 
{
  return out << t.vectTest;
}

istream& operator>>(istream &in, Test &t) 
{
  return in >> t.vectTest;
}
