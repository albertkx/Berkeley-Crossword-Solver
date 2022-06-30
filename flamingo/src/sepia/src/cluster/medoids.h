/*
  $Id: medoids.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _medoids_h_
#define _medoids_h_

#include "clusters.h"

// 1: random sample, random pivots
// 2: sample is the full data
// 3: swap step is used
// 5: PAM: Partitioning Around Medoids; k-Medoids algorithm 
// 6: ML - Medoids
// 7: ML - Medoids, Sample

class Medoids: public Clusters {
private:
  unsigned ver, sampleSizeMul, sampleSets, sampleSteps, sampleSize;

public:
  Medoids(const vector<string>* d, unsigned clusterNo, 
          SampleType sampleType, unsigned samplePer = 20,
          unsigned queueSize = 10, unsigned uniqueNo = 10, 
          unsigned ver = 7, unsigned sampleSizeMul = 5,
          unsigned sampleSets = 10, unsigned sampleSteps = 10);

//   bool operator==(const Medoids &c) const;

  void buildClusters();

  ostream& info(ostream &out);

private:
  void buildClustersVer1();
  void buildClustersVer2();
  void buildClustersVer3();
  void buildClustersVer5();
  void buildClustersVer6();
  void buildClustersVer7();
  void assignClusters();
  void assignClusters(vector<unsigned>::const_iterator begin, 
                      vector<unsigned>::const_iterator end);
  void assignClustersMinus(vector<unsigned>::const_iterator begin, 
                           vector<unsigned>::const_iterator end);
};

typedef struct { unsigned int d, e; } Closest;

typedef vector<Closest> VectClosest;
typedef VectClosest::iterator VectClosestIt;

ostream& output(ostream &out, const unsigned* p, const unsigned k, 
                const vector<string> &d);
ostream& output(ostream &out, const Closest* c, const unsigned nK, 
                const unsigned* p, const unsigned* np);

#endif
