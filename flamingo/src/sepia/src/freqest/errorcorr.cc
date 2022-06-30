/*
  $Id: errorcorr.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/23/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "errorcorr.h"
#include "freqest.h"

#include <cstdlib>

bool ErrorCorr::operator==(const ErrorCorr &e) const 
{
  if (this == &e)
    return true;
  if (dataset == e.dataset && 
      *clusters == *e.clusters && 
      thresholdMin == e.thresholdMin && 
      thresholdMax == e.thresholdMax && 
      recordSize == e.recordSize && 
//       vectRecord == e.vectRecord && 
      avgLen == e.avgLen && 
      avgThr == e.avgThr && 
      avgEst == e.avgEst) {
    bool errEqual = true;
    for (unsigned i = 0; i < 8; i++)
      if (err[i] != e.err[i])
        errEqual = false;
    if (errEqual)
      return true;
  }
  return false;
}

void ErrorCorr::buildErrorCorr() 
{
  unsigned sumLen=0;
  SimType sumThr=0;
  float sumEst=0;
  
  for (VectRecordIt it=vectRecord.begin(); it!=vectRecord.end(); it++) {
    sumLen+=it->query.length();
    sumThr+=it->dist;
    sumEst+=it->est;
  }

  unsigned size=vectRecord.size();
  avgLen=static_cast<float>(sumLen)/size;
  avgThr=static_cast<float>(sumThr)/size;
  avgEst=sumEst/size;

  unsigned count[8];
  float sumErr[8];
  
  for (unsigned i=0; i<8; i++) {
    count[i]=0;
    sumErr[i]=0;
  }

  for (VectRecordIt it=vectRecord.begin(); it!=vectRecord.end(); it++) {
    unsigned pos=getPos(*it);
    count[pos]++;
    sumErr[pos]+=it->relErr; 
    // assumes that all the entries in the dataset have relErr
  }  

  for (unsigned i=0; i<8; i++) {
    if (count[i])
      err[i]=sumErr[i]/count[i];
    else
      err[i]=0;
  }  
}

void ErrorCorr::buildRecords() 
{
  const unsigned datasetSize = dataset->size();

  vector<unsigned> indexes;
  indexes.reserve(datasetSize);
  for (unsigned i = 0; i < datasetSize; i++) indexes[i] = i;

  cerr << "errcorr"; cerr.flush();

  for (unsigned i = 0; i < recordSize; i++) {
    if (i % 20 == 0) {
      cerr << '.'; cerr.flush();
    }

    unsigned index = rand() % (datasetSize - i);
    indexes[index] = indexes[datasetSize - i - 1];

#if SIM_DIST == 1
    SimType dist = static_cast<SimType>(rand() % 
                                        (thresholdMax -
                                         thresholdMin + 1) + thresholdMin);
#elif SIM_DIST == 2
    SimType dist = static_cast<SimType>(rand() % 
                                        static_cast<int>(thresholdMax * 10 - 
                                                         thresholdMin * 10 + 1)) / 
      10 + thresholdMin;
#endif

    Record r = Record((*dataset)[index], dist);
    r.real = freqRealFunc(*dataset, r);
    r.est = freqEstFunc(*dataset, *clusters, r);
    r.setRelErr(); // assumes that the query is form the dataset => real! = 0
    vectRecord.push_back(r);
  }

  cerr << "OK" << endl;
}

unsigned ErrorCorr::getPos(Record r) const
{
  unsigned pos=0;
  if (static_cast<float>(r.dist)<=avgThr)
    ;
  else 
    pos += 4;
  if (static_cast<float>(r.query.length())<=avgLen)
    ;
  else
    pos += 2;
  if (r.est<=avgEst)
    ;
  else
    pos += 1;
  return pos;
}

ostream& ErrorCorr::info(ostream& out) 
{
  out << "ErrorCorr" << endl << "---" << endl;
  out << "ErrorCorr record size\t" << recordSize << endl;
  return out << endl;
}

void ErrorCorr::readRecords(const string filename) 
{
  ifstream file(filename.c_str(), ios::in);
  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file >> vectRecord;
  file.close();
}

void ErrorCorr::writeRecords(const string filename) const 
{
  ofstream file(filename.c_str(), ios::out);
  if (!file) {
    cerr << "can't open output file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file << vectRecord;
  file.close();
}

void ErrorCorr::readErrorCorr(const string filename) 
{
  ifstream file(filename.c_str(), ios::in);
  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file >> *this;
  file.close();
}

void ErrorCorr::writeErrorCorr(const string filename) const 
{
  ofstream file(filename.c_str(), ios::out);
  if (!file) {
    cerr << "can't open output file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file << *this;
  file.close();  
}

ostream& operator<<(ostream& out, const ErrorCorr& errorCorr) 
{
  out << errorCorr.avgThr << endl;
  out << errorCorr.avgLen << endl;
  out << errorCorr.avgEst << endl << endl;
  for (unsigned i=0; i<8; i++) {
    out << errorCorr.err[i] << " ";
  }
  out << endl;
  return out;
}

istream& operator>>(istream& in, ErrorCorr& errorCorr) 
{
  in >> errorCorr.avgThr;
  in >> errorCorr.avgLen;
  in >> errorCorr.avgEst;
  for (unsigned i=0; i<8; i++) {
    in >> errorCorr.err[i];
  }
  return in;
}
