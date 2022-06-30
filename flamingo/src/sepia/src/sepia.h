/*
  $Id: sepia.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _selest_h_
#define _selest_h_

#include "cluster/clusters.h"
#include "freqest/errorcorr.h"

class Sepia
{
public:
  Sepia(const vector<string> &dataset, 
        unsigned thresholdMin,
        unsigned thresholdMax);
  Sepia(const vector<string> &dataset,
        const string &filename); 
  // filename is used as a prefix for multiple filenames

  void build();
  void saveData(const string &filename) const;
  // filename is used as a prefix for multiple filenames
  
  float getEstimateSelectivity(const string &query, unsigned editdist) const;

  // advanced constructor
  Sepia(const vector<string> &dataset, 
        unsigned thresholdMin,
        unsigned thresholdMax,
        unsigned clustNo, // default: 100
        unsigned recNo = 300,
        ClusterType clustType = MEDOIDS_IMP,
        SampleType sampleType = CLOSE_RAND,
        unsigned samplePer = 20,
        unsigned sampleSizeMul = 5,
        unsigned sampleSets = 10,
        unsigned sampleSteps = 10,
        unsigned queueSize = 10,
        unsigned uniqueNo = 10);

  bool operator==(const Sepia &s) const;

  unsigned getRealSelectivity(const string &query, unsigned editdist) const;

  Clusters* getClusters() const { return clust; }

  ~Sepia() { delete clust; delete err; }

private:
  const vector<string> *dataset;
  unsigned thresholdMin, thresholdMax;
  unsigned clustNo, recNo;
  ClusterType clustType;
  SampleType sampleType;
  unsigned  samplePer, sampleSizeMul, sampleSets, sampleSteps, queueSize,
    uniqueNo;

  Clusters *clust;
  ErrorCorr *err;

  static const string PARAM_SUFFIX, CLUST_SUFFIX, PPD_SUFFIX, ERR_SUFFIX;

  void loadData(const string &filenamePrefix);
  bool consistData(const string &filenamePrefix) const;
};

#endif
