/*
  $Id: unittest.cc 5714 2010-09-09 03:51:02Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 04/15/2008
  Author: Rares Vernica <rares (at) ics.uci.edu> 
*/

#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>

#include "simmetric.h"

using namespace std;

void testSimMetric() 
{
  uint t = 0;
  GramGen *g = new GramGenFixedLen(1);

  SimMetric *sim = new SimMetricEd(*g);

  vector<string> s;
  s.push_back("abc");
  s.push_back("ab");
  s.push_back("ac");
  s.push_back("bc");
  s.push_back("a");
  s.push_back("b");
  s.push_back("c");
  s.push_back("abcdef");
  s.push_back("xyz");
  s.push_back("bac");
  s.push_back("acb");
  s.push_back("ba");
  
  const uint n = 12;
  uint ed[][n] = 
    { {0, 1, 1, 1, 2, 2, 2, 3, 3, 2, 2, 2},
      {1, 0, 1, 2, 1, 1, 2, 4, 3, 2, 1, 2},
      {1, 1, 0, 1, 1, 2, 1, 4, 3, 1, 1, 2},
      {1, 2, 1, 0, 2, 1, 1, 4, 3, 1, 2, 1},
      {2, 1, 1, 2, 0, 1, 1, 5, 3, 2, 2, 1},
      {2, 1, 2, 1, 1, 0, 1, 5, 3, 2, 2, 1},
      {2, 2, 1, 1, 1, 1, 0, 5, 3, 2, 2, 2},
      {3, 4, 4, 4, 5, 5, 5, 0, 6, 5, 4, 5},
      {3, 3, 3, 3, 3, 3, 3, 6, 0, 3, 3, 3},
      {2, 2, 1, 1, 2, 2, 2, 5, 3, 0, 2, 1},
      {2, 1, 1, 2, 2, 2, 2, 4, 3, 2, 0, 3},
      {2, 2, 2, 1, 1, 1, 2, 5, 3, 1, 3, 0}};

  for (uint i = 0; i < n; i++)
    for (uint j = 0; j < n; j++) {
      // cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
      //      << ed[i][j] << " " << sim->operator()(s[i], s[j]) << endl;
      assert(sim->operator()(s[i], s[j]) == ed[i][j]); t++;
      assert(sim->operator()(s[i], s[j], ed[i][j])); t++;
      if (ed[i][j] > 0)
        assert(!sim->operator()(s[i], s[j], ed[i][j] - 1)); t++;
    } 

  delete sim;

  sim = new SimMetricEdNorm(*g);

  for (uint i = 0; i < n; i++)
    for (uint j = 0; j < n; j++) {
      // cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
      //      << (1 - 
      //          static_cast<float>(ed[i][j]) / 
      //          max(s[i].length(), s[j].length()))
      //      << " " << sim->operator()(s[i], s[j]) << endl;
      assert(sim->operator()(s[i], s[j]) - 1 - static_cast<float>(ed[i][j]) / 
             max(s[i].length(), s[j].length()) <= 
             numeric_limits<float>::epsilon()); t++;
      assert(sim->operator()(s[i], s[j], 
                             1 - static_cast<float>(ed[i][j]) / 
                             max(s[i].length(), s[j].length()) - 
                             numeric_limits<float>::epsilon())); t++;
      if (ed[i][j] > 0)
        assert(!sim->operator()(s[i], s[j], 
                                1 - static_cast<float>(ed[i][j]) / 
                                max(s[i].length(), s[j].length()) + 
                                numeric_limits<float>::epsilon())); t++;
    }

  uint edSwap[][n] = 
    { {0, 1, 1, 1, 2, 2, 2, 3, 3, 1, 1, 2},
      {1, 0, 1, 2, 1, 1, 2, 4, 3, 2, 1, 1},
      {1, 1, 0, 1, 1, 2, 1, 4, 3, 1, 1, 2},
      {1, 2, 1, 0, 2, 1, 1, 4, 3, 1, 2, 1},
      {2, 1, 1, 2, 0, 1, 1, 5, 3, 2, 2, 1},
      {2, 1, 2, 1, 1, 0, 1, 5, 3, 2, 2, 1},
      {2, 2, 1, 1, 1, 1, 0, 5, 3, 2, 2, 2},
      {3, 4, 4, 4, 5, 5, 5, 0, 6, 4, 4, 5},
      {3, 3, 3, 3, 3, 3, 3, 6, 0, 3, 3, 3},
      {1, 2, 1, 1, 2, 2, 2, 4, 3, 0, 2, 1},
      {1, 1, 1, 2, 2, 2, 2, 4, 3, 2, 0, 3},
      {2, 1, 2, 1, 1, 1, 2, 5, 3, 1, 3, 0}};

  sim = new SimMetricEdSwap(*g);

  for (uint i = 0; i < n; i++)
    for (uint j = 0; j < n; j++) {
      // cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
      //      << edSwap[i][j] << " " << sim->operator()(s[i], s[j]) << endl;
      assert(sim->operator()(s[i], s[j]) == edSwap[i][j]); t++;
      assert(sim->operator()(s[i], s[j], edSwap[i][j])); t++;
      if (edSwap[i][j] > 0)
        assert(!sim->operator()(s[i], s[j], edSwap[i][j] - 1)); t++;
    } 
  
  sim = new SimMetricJacc(*g);

  float jc[][n] = 
    { {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3,
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {2. / 3, 1,      1. / 3, 1. / 3,     .5,
           .5,      0, 1. / 3,      0, 2. / 3, 2. / 3,      1},
      {2. / 3, 1. / 3,      1, 1. / 3,     .5,
            0,     .5, 1. / 3,      0, 2. / 3, 2. / 3, 1. / 3},
      {2. / 3, 1. / 3, 1. / 3,      1,      0,
           .5,     .5, 1. / 3,      0, 2. / 3, 2. / 3, 1. / 3},
      {1. / 3,     .5,     .5,      0,      1,
            0,      0, 1. / 6,      0, 1. / 3, 1. / 3,     .5},
      {1. / 3,     .5,      0,     .5,      0,
            1,      0, 1. / 6,      0, 1. / 3, 1. / 3,     .5},
      {1. / 3,      0,     .5,     .5,      0,
            0,      1, 1. / 6,      0, 1. / 3, 1. / 3,      0},
      {    .5, 1. / 3, 1. / 3, 1. / 3, 1. / 6,
       1. / 6, 1. / 6,      1,      0,     .5,     .5, 1. / 3},
      {     0,      0,      0,      0,      0,
            0,      0,      0,      1,      0,      0,      0},
      {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3, 
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3,
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {2. / 3,      1, 1. / 3, 1. / 3,     .5,
           .5,      0, 1. / 3,      0, 2. / 3, 2. / 3,      1}};

  for (uint i = 0; i < n; i++)
    for (uint j = 0; j < n; j++) {
      // cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
      //      << jc[i][j] << " " << sim->operator()(s[i], s[j]) << endl;
      assert(sim->operator()(s[i], s[j]) == jc[i][j]); t++;
      assert(sim->operator()(s[i], s[j], jc[i][j])); t++;
      assert(!sim->operator()(s[i], s[j], jc[i][j] + numeric_limits<float>::epsilon())); t++;
    }

  cout << "SimMetric (" << t << ")" << endl;
}

int main() 
{  
  cout << "test..." << endl;

  testSimMetric();
  
  cout << "OK" << endl;

  return 0;
}
