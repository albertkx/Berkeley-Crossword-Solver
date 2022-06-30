/*
  $Id: misc.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD License.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _misc_h_
#define _misc_h_

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>

#include <tr1/unordered_map>

using namespace std;
using namespace tr1;

// x^y
unsigned pow(unsigned x, unsigned y);

// all possible subsets of k elements from 0..n-1
vector<vector<unsigned> > subsets(unsigned n, unsigned k);

template<class T> 
T min(T x, T y, T z) {
  return (x < y ? (x < z ? x : z) : (y < z ? y : z));
}

// unsigned to string
template<typename T> 
string utos(T i) 
{
  stringstream s;
  s << i;
  return s.str();
}

string utosh(unsigned i);       // human representation

class UnsignedSeq 
{
private:
  unsigned value;
public:
  UnsignedSeq(unsigned initialValue = 0);
  unsigned operator() ();
};

string removeExt(const string filename);

void writeerror(const string filename);
void readerror(const string filename);

template <class Key, class Ty, class Hash, class Pred, class Alloc>
bool equalElements(const unordered_map<Key, Ty, Hash, Pred, Alloc> &a, 
                   const unordered_map<Key, Ty, Hash, Pred, Alloc> &b) 
{
  typename unordered_map<Key, Ty, Hash, Pred, Alloc>::const_iterator it, pos;
  for (it = a.begin(); it != a.end(); ++it) {
    pos = b.find(it->first);
    if (pos == b.end() || pos->second != it->second)
      return false;
  }
  for (it = b.begin(); it != b.end(); ++it) {
    pos = a.find(it->first);
    if (pos == a.end() || pos->second != it->second)
      return false;
  }
  return true;
}

template <class T>
void selectRandom(const vector<T> &data, vector<T> &selection, unsigned n) 
{
  vector<unsigned> idx(data.size());
  unsigned i;
  for (i = 0; i < data.size(); i++)
    idx[i] = i;
  random_shuffle(idx.begin(), idx.end());
  for (i = 0; i < n; i++)
    selection.push_back(data[idx[i]]);
}

// generates a zipfian distribution for distinctValues with skew value zipfSkew
// sorted value frequencies are stored in valFreqs, caller must ensure proper memory allocation
void genZipfDist(unsigned distinctValues, unsigned zipfSkew, double valFreqs[]);

// takes inputFile and reades it line-by-line
// creates random duplicates with minErrors to maxErrors edit operations applied until targetSize is reached
// all lines are written to targetFile
void enlargeDataset(const char* targetFile, 
		    const char* inputFile, 
		    unsigned targetSize,
		    unsigned minErrors,
		    unsigned maxErrors, 
		    unsigned trueRand);

// compute slope and intercept of line using linear regression
void linearRegression(const vector<float>& xvals, const vector<float>& yvals, float& slope, float& intercept);

// simple implementation of binomial distribution
// for cumulative version, no optimization using beta function ar anything - just a simple summation
float binomialDistrib(unsigned n, unsigned k, float p, bool cumulative);

// check if a file exists
bool fileExists(const char* fileName);

// returns true if success, false if no success
bool getRandomFileName(string& target, unsigned maxAttempts = 100);

#endif
