/*
  $Id: errorcorr.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/23/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _errorcorr_h_
#define _errorcorr_h_

#include "record.h"
#include "cluster/clusters.h"

class ErrorCorr 
{
private:
  const vector<string> *dataset;
  const Clusters *clusters;
  SimType thresholdMin, thresholdMax;
  unsigned recordSize;
  VectRecord vectRecord;
  float avgLen, avgThr, avgEst;
  float err[8];
  
public:
  ErrorCorr(const vector<string> *dataset, 
            const Clusters *clusters,
            SimType thresholdMin, SimType thresholdMax, 
            unsigned recordSize = 300): 
    dataset(dataset), clusters(clusters), 
    thresholdMin(thresholdMin), thresholdMax(thresholdMax), 
    recordSize(min((size_t)recordSize, dataset->size() / 3)) 
  {}

  bool operator==(const ErrorCorr &e) const;

  float getError(Record r) const { return err[getPos(r)]; }

  void buildRecords();
  void buildErrorCorr();

  void writeRecords(const string filename) const;
  void readRecords(const string filename);
  void writeErrorCorr(const string filename) const;
  void readErrorCorr(const string filename);

  ostream& info(ostream &out);

private:
  unsigned getPos(Record r) const;

  friend ostream& operator<<(ostream &out, const ErrorCorr &errorCorr);
  friend istream& operator>>(istream &in, ErrorCorr &errorCorr);
};

#endif
