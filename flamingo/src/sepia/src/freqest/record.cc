/*
  $Id: record.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/21/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "record.h"

#include <iterator>

using namespace std;

void Record::setRelErr() 
{
  if (real!=0) {
    relErr=static_cast<float>(est-real)/real;
//     if (relErr<0) {
//       relErr-=relErr;
//     }
  }
  else {
    relErr=-1;
  }
} 

ostream& Record::toWeka(ostream &out) 
{
  out << query.length() << "," << dist << "," << est;
  out << "," << cntCluster << "," << cntPPDpair;
  out << "," << relErr;
  return out;
}

ostream& operator<<(ostream &out, const Record &r) 
{
  out << r.query << "\t" << r.dist << "\t" << r.real << "\t" << r.est << "\t" 
      << r.relErr << "\t" << r.cntCluster << "\t" << r.cntPPDpair;
  return out;
}

istream& operator>>(istream &in, Record &r) 
{
  in >> r.query >> r.dist >> r.real >> r.est >> r.relErr;
  in >> r.cntCluster >> r.cntPPDpair;
  return in;
}


istream& operator>>(istream &in, VectRecord &v) 
{
  unsigned n;
  in>>n;
  for (unsigned i=0; i<n; i++) {
    Record p;
    in>>p;
    v.push_back(p);
  }
  return in;
}

ostream& operator<<(ostream &out, const VectRecord &v) 
{
  out<<static_cast<unsigned>(v.size())<<endl<<endl;
  copy(v.begin(), v.end(), ostream_iterator<Record>(out, "\n"));
  return out;
}

ostream& VectRecordToWeka(ostream &out, VectRecord &v) 
{
  string wekaHeader = "\
@relation 'records'\n\
@attribute predicate_string_length integer\n\
@attribute predicate_threshold integer\n\
@attribute selectivity_estimation real\n\
@attribute count_clusters integer\n\
@attribute count_ppdpairs integer\n\
@attribute relative_error real\n\
@data\n";
  
  out << wekaHeader;
  for (VectRecordIt it = v.begin(); it != v.end(); it++) {
    it->toWeka(out);
    out << endl;
  }
  return out;
}
