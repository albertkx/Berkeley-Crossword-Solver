/*
  $Id: simdist.h 4928 2009-12-17 22:52:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _simdist_h_

// default value for SIM_DIST
#ifndef SIM_DIST
#define SIM_DIST 1
#endif

// default value for DATASET_LEN
#ifndef DATASET_LEN
#define DATASET_LEN 20
#endif

#define _simdist_h_ SIM_DIST
// 1: ed
// 2: jd

#include <string>
#include <vector>

using namespace std;


#if _simdist_h_ == 1

typedef unsigned SimType;

const unsigned SIM_TYPE_MIN = 0;
const unsigned SIM_TYPE_MAX = DATASET_LEN;

inline size_t SimType2size_t(const unsigned x) { return static_cast<size_t>(x%100); }
inline unsigned SimUpper(const unsigned x) { return x+1; }
inline unsigned SimUpperOrEqual(const unsigned x) { return x; }

#elif _simdist_h_ == 2

typedef float SimType;
float jdQ(const string &s1, const string &s2);

const float SIM_TYPE_MIN = 0;
const float SIM_TYPE_MAX = 1;
const unsigned SIM_TYPE_PREC = 100;

inline size_t SimType2size_t(const float x) { return static_cast<size_t>(x*100); }
inline float SimUpper(const float x) {
  return static_cast<float>(static_cast<unsigned>(x * SIM_TYPE_PREC) + 1) /
    SIM_TYPE_PREC;
}
inline float SimUpperOrEqual(const float x) {
  unsigned y=static_cast<unsigned>(x * SIM_TYPE_PREC);
  return (static_cast<float>(y) / SIM_TYPE_PREC == x)? x:
    static_cast<float>(y+1) / SIM_TYPE_PREC;
}

#endif

extern SimType (*SimDist)(const string&, const string&);

#endif
