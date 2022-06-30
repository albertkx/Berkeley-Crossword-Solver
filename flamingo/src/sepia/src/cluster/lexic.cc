/*
  $Id: lexic.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "lexic.h"

#include <algorithm>

class LexicCmp
{
private:
  const vector<string> *data;
public:
  LexicCmp(const vector<string> *d): data(d) {}
  bool operator()(unsigned a, unsigned b) const { return (*data)[a] < (*data)[b]; }
};  

Lexic::Lexic(const vector<string> *d, const unsigned clusterNo, 
             const SampleType sampleType, const unsigned samplePer,
             const unsigned queueSize, const unsigned uniqueNo): 
  Clusters(d, clusterNo, sampleType, samplePer, queueSize, uniqueNo)
{  
  for (unsigned i = 0; i < clusterNo; i++)
    clusters->push_back(Cluster());
}

void Lexic::buildClusters() 
{
  vector<unsigned> dId(data->size());
  for (unsigned i = 0; i < data->size(); i++) dId[i] = i;

  sort(dId.begin(), dId.end(), LexicCmp(data));

  const unsigned n = static_cast<unsigned>(data->size());
  const unsigned nclust = n/clusterNo;
  for (unsigned i = 0; i < clusterNo; i++) {
    const unsigned start = i*nclust;
    const unsigned end = (i == clusterNo? n:(i + 1) * nclust);
    const unsigned p = start + (end - start)/2;
    (*clusters)[i].setPivot(dId[p]);
    SimType radius = (*clusters)[i].getRadius();
    for (unsigned j = start; j < end; j++) {
      (*clusters)[i].insert(dId[j]);
      radius = max(radius, SimDist((*data)[dId[p]], (*data)[dId[j]]));
    }
    (*clusters)[i].setRadius(radius);
  }
}

ostream& Lexic::info(ostream& out)
{
  Clusters::info(out);
  out << "Cluster method\tLexic" << endl;
  return out << endl;
}
