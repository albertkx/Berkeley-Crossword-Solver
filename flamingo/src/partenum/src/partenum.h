/*
  $Id: partenum.h 5769 2010-10-19 06:17:00Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  The implementation of the PartEnum algorithm invented by Microsoft
  researchers is limited to non commercial use, which would be covered
  under the royalty free covenant that Microsoft made public.

  Date: 01/31/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _partenum_h_
#define _partenum_h_

#include <map>
#include <utility>
#include <vector>

#include <tr1/unordered_map>

#include "util/src/appsearch.h"
#include "util/src/gram.h"

using namespace std;
using namespace tr1;

typedef unordered_map<unsigned, unsigned*> SigsBucket;

bool operator==(const SigsBucket &b1, const SigsBucket &b2);

class PartEnum: public AppSearch
{
public:
  PartEnum(const vector<string> &data, 
           unsigned q, 
           unsigned editdist, 
           unsigned n1, 
           unsigned n2);

  PartEnum(const vector<string> &data, 
           const string &filename);
  // filename is used as a prefix for multiple filenames

  void build();
  void saveIndex(const string &filename) const;
  // filename is used as a prefix for multiple filenames

  void search(const string &query, vector<unsigned> &results);
  void search(const string &query, const unsigned editdist,
              vector<unsigned> &results);

  ~PartEnum();

  unsigned getQ() const { return gramId.getQ(); }
  unsigned getEditdist() const { return k / gramId.getQ() / 2; }
  unsigned getN1() const { return n1; }
  unsigned getN2() const { return n2; }
  unsigned getK() const { return k; }
  unsigned getK2() const { return k2; }
  unsigned getSignlen() const { return siglen; }

  void sign(const string &s, vector<unsigned> &sig) const;
  void sign(const string &s, unsigned *sig) const;

  bool operator==(const PartEnum &h) const;

private:
  const vector<string> *data;
  GramId gramId;
  unsigned k, k2, n1, n2;
  vector<vector<unsigned> > subs;
  unsigned datalen, siglen;
  SigsBucket *buckets;
  
  static const unsigned siglenMax;
  static const string paramSuffix, signSuffix;

  void loadIndex(const string &filenamePrefix);
  bool consistIndex(const string &filenamePrefix) const;

  unsigned begin(unsigned i, unsigned j) const { 
    return gramId.getN() * (n2 * i + j) / n1 / n2; }
  unsigned end(unsigned i, unsigned j) const { 
    return gramId.getN() * (n2 * i + j + 1) / n1 / n2; }
};

#endif
