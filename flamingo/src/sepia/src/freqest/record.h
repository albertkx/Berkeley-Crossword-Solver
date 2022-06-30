/*
  $Id: record.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/21/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _record_h_
#define _record_h_

#include "predicate.h"

class Record: public Predicate 
{
public:
  float est, relErr;
  unsigned real;
  unsigned cntCluster, cntPPDpair;

  Record(): Predicate(), est(0), relErr(-1), real(0) {}
  Record(string q, SimType d): Predicate(q, d), est(0), relErr(-1), real(0) {}
  Record(string q, SimType d, float e, unsigned r): 
    Predicate(q, d), est(e), real(r) {
    setRelErr(); }
  Record(string q, SimType d, float e, unsigned r, unsigned cntC, unsigned cntP): 
    Predicate(q, d), est(e), real(r), cntCluster(cntC), cntPPDpair(cntP) {
    setRelErr(); }
  Record(Predicate p, float e):
    Predicate(p), est(e), relErr(-1), real(0) {}
  Record(Predicate p, float e, unsigned r): Predicate(p), est(e), real(r) {
    setRelErr(); }
  Record(Predicate p, float e, unsigned r, unsigned cntC, unsigned cntP):
    Predicate(p), est(e), real(r), cntCluster(cntC), cntPPDpair(cntP) {
    setRelErr();
  }

  bool operator==(const Record &r) const {
    return this == &r || 
      (est == r.est && relErr == r.relErr && real == r.real && 
       cntCluster == r.cntCluster && cntPPDpair == r.cntPPDpair); 
  }

  void setRelErr();

  ostream& toWeka(ostream &out);

  friend ostream& operator<<(ostream &out, const Record &r);
  friend istream& operator>>(istream &in, Record &r);
};

typedef vector<Record> VectRecord;
typedef VectRecord::iterator VectRecordIt;

ostream& operator<<(ostream &out, const VectRecord &v);
istream& operator>>(istream &in, VectRecord &v);
ostream& vectRecordToWeka(ostream  &out,  VectRecord  &v);

inline float recordGetEst(Record r) { return r.est; }
inline float recordGetLen(Record r) { return static_cast<float>(r.query.length()); }
inline float recordGetThr(Record r) { return static_cast<float>(r.dist); }

typedef float recordFunc(Record);

#endif
