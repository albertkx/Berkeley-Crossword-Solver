/*
  $Id: clusters.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "clusters.h"
#include "ppdtable/allrand.h"
#include "ppdtable/closelex.h"
#include "ppdtable/closerand.h"
#include "ppdtable/closeunique.h"

#include <algorithm>

ostream& operator<<(ostream &out, const QueryVect &qv) 
{
  return out << qv.query << "\t" << qv.vect;
}

istream& operator>>(istream &in, QueryVect &qv)
{
  return in >> qv.query >> qv.vect;
}  

Clusters::Clusters(const vector<string> *d, unsigned clusterNo, 
                   SampleType sampleType, unsigned samplePer,
                   unsigned queueSize, unsigned uniqueNo): 
  data(d), clusterNo(clusterNo), sampleType(sampleType), 
  samplePer(samplePer), queueSize(queueSize), uniqueNo(uniqueNo), 
  clusters(new VectCluster), ppdtable(new PPDTable()),
  queryPivot(new ContQueryPivot())
{
}

Clusters::Clusters(const Clusters &cs): 
  data(cs.data), clusterNo(cs.clusterNo), sampleType(sampleType), 
  samplePer(samplePer), queueSize(queueSize), uniqueNo(uniqueNo)
{
  clusters = new VectCluster;
  *clusters = *cs.clusters;
  ppdtable = new PPDTable();
  *ppdtable = *cs.ppdtable;
  queryPivot = new ContQueryPivot();
  *queryPivot = *cs.queryPivot;
}

Clusters::~Clusters() 
{
  delete clusters;
  delete ppdtable;
  delete queryPivot;
}

Clusters& Clusters::operator=(const Clusters &cs) 
{
  if (this == &cs)
    return *this;
  *clusters=*cs.clusters;
  *ppdtable=*cs.ppdtable;
  *queryPivot = *cs.queryPivot;
  data=cs.data;
  return *this;
}

bool Clusters::operator==(const Clusters &c) const 
{
  if (this == &c)
    return true;
  if (data == c.data && 
      clusterNo == c.clusterNo && 
      sampleType == c.sampleType && 
      samplePer == c.samplePer && 
      queueSize == c.queueSize && 
      uniqueNo == c.uniqueNo && 
      *clusters == *c.clusters && 
      *ppdtable == *c.ppdtable // &&
//    *queryPivot == *c.queryPivot
    )
    return true;
  
  return false;
}

const size_t Clusters::sizeEntries() const
{
  size_t cntEntries = 0;
  for (VectCluster::const_iterator it = clusters->begin();
       it != clusters->end(); ++it)
    cntEntries += it->getFTable().size();
  return cntEntries;
}

void Clusters::buildFTables() 
{
  for (VectClusterIt i=clusters->begin(); i!=clusters->end(); i++)
    i->buildFTable(data);
}

void Clusters::buildPPDtable() 
{
  PPDSample *sampleEngine;
  switch (sampleType) {
  case ALL_RAND:
    sampleEngine = new AllRand(data, this, queryPivot, samplePer);
    break;
  case CLOSE_LEX:
    sampleEngine = new CloseLex(data, this, queryPivot, samplePer, queueSize);
    break;
  case CLOSE_UNIQUE:
    sampleEngine = new CloseUnique(data, this, queryPivot, samplePer, queueSize,
                                   uniqueNo);
    break;
  default: // CLOSE_RAND:
    sampleEngine = new CloseRand(data, this, queryPivot, samplePer, queueSize);
    break;
  }

  unsigned i = 0;

  cerr << "ppdtable"; cerr.flush();

  while (sampleEngine->hasNext()) {

    if (i++ % 1000000 == 0) {
      cerr << '.'; cerr.flush();
    }

    PPDTriple triple = sampleEngine->next();
    PPDEntry entry = PPDEntry(triple.vect1, triple.vect2,
                              SimUpperOrEqual(triple.dist));
    ppdtable->insert(entry);   
  }

  cerr << "OK" << endl;

  delete sampleEngine;
}

ostream& operator<<(ostream &out, const Clusters &cs) 
{
  out << static_cast<unsigned>(cs.clusters->size()) << endl << endl;
  copy(cs.clusters->begin(), cs.clusters->end(), 
       ostream_iterator<Cluster>(out, "\n"));
  return out;
}

istream& operator>>(istream &in, Clusters &cs) 
{
  unsigned n;
  in>>n;
  for (unsigned i=0; i<n; i++) {
    Cluster cluster;			
    in>>cluster;
    cs.clusters->push_back(cluster);
  }
  return in;
}

typedef map<SimType, unsigned> MapSimDist;

ostream& operator<<(ostream &out, const MapSimDist &map) 
{
  for (MapSimDist::const_iterator it = map.begin(); it != map.end(); ++it) 
    {
      out << it->first << " " << it->second << endl;
    }
  return out;
}

void Clusters::hist(ostream &out, const vector<string> &d) const 
{
  MapSimDist globalHist, localHist;
  for (VectClusterIt cluster = clusters->begin();
       cluster!= clusters->end(); ++cluster) {
    localHist.clear();
    unsigned pivot = cluster->getPivot();
    for (ContCluster::const_iterator index = cluster->begin();
         index != cluster->end(); ++index) {
      SimType dist = SimDist(d[pivot], d[*index]);	  
      ++localHist[SimUpperOrEqual(dist)];
    }

    for (MapSimDist::const_iterator it = localHist.begin();
         it != localHist.end(); ++it) 
      globalHist[it->first] += it->second; // / cluster->size();
  }

  out << globalHist;
}

void Clusters::stat(ostream &out, const vector<string> &d) const 
{
  float 
    sumAvg = 0, 
    sumRad = 0;
  for (VectClusterIt cluster = clusters->begin();
       cluster!= clusters->end(); ++cluster) {
    unsigned pivot = cluster->getPivot();
    SimType
      sumDist = 0;
    for (ContCluster::const_iterator index = cluster->begin();
         index != cluster->end(); ++index) {
      SimType dist = SimDist(d[pivot], d[*index]);	  
      sumDist += dist;
    }
    SimType
      avgDist = sumDist / cluster->size();
    sumAvg += avgDist;
    sumRad += cluster->getRadius();
  }

  out << sumAvg / sizeCluster() << "\t" << sumRad / sizeCluster() << "\t";
}

ostream& Clusters::info(ostream &out) 
{
  out << "Clusters" << endl << "---" << endl;
  out << "Clusters no\t" << clusterNo << endl;
  return out << endl;
}

bool Clusters::isPivot(unsigned idx) const
{
  for (vector<Cluster>::const_iterator it = clusters->begin(); 
       it != clusters->end(); ++it) 
    if (it->getPivot() == idx) 
      return true;
  return false;
}

void Clusters::insert(unsigned idx)
{
  SimType min = SimDist((*data)[clusters->begin()->getPivot()], (*data)[idx]);
  vector<Cluster>::iterator closest = clusters->begin();
  for (vector<Cluster>::iterator it = clusters->begin() + 1; 
       it != clusters->end(); ++it) {
    SimType crt = SimDist((*data)[it->getPivot()], (*data)[idx]);
    if (crt < min) {
      min = crt;
      closest = it;
    }
  }

  // "closest" is the closest cluster
  unsigned pivot = closest->getPivot();
  SimVect vect2 = SimVect( (*data)[pivot], (*data)[idx]);
  closest->insert(idx, vect2);

  SimType dist = vect2.getDist();
  if (dist > closest->getRadius()) 
    closest->setRadius(dist);

  for (ContQueryPivot::const_iterator it = queryPivot->find(pivot);
       it != queryPivot->end() && it->first == pivot; ++it) {
    SimVect vect1 = it->second.vect;
    SimType dist = SimDist((*data)[it->second.query], (*data)[idx]);
    PPDEntry entry = PPDEntry(vect1, vect2, SimUpperOrEqual(dist));
    ppdtable->insert(entry);
  }
}

class CmpDist
{
private:
  const vector<string> *data;
  string pivotStr;
public:
  CmpDist(const vector<string> *data, unsigned pivot): 
    data(data), pivotStr((*data)[pivot]) {}
  bool operator()(unsigned i, unsigned j) {
    return SimDist((*data)[i], pivotStr) < SimDist((*data)[j], pivotStr); }
};

void Clusters::erase(unsigned idx) 
{
  vector<Cluster>::iterator contain;
  for (contain = clusters->begin(); contain != clusters->end(); ++contain) 
    if (contain->find(idx) != contain->end()) 
      break;
  if (contain == clusters->end()) {
    cerr << "Clusters::erase index not found\t" << idx << endl;
    return;
  }  
  unsigned pivot = contain->getPivot();
  SimVect vect2 = SimVect((*data)[pivot], (*data)[idx]);
  contain->erase(idx, vect2);

  if (contain->getRadius() == vect2.getDist())
    // update radius
    contain->setRadius(SimDist((*data)[pivot], 
                               (*data)[*max_element(contain->begin(), 
                                                    contain->end(),
                                                    CmpDist(data, pivot))]));

  for (ContQueryPivot::const_iterator it = queryPivot->find(pivot);
       it != queryPivot->end() && it->first == pivot; ++it) {
    SimVect vect1 = it->second.vect;
    SimType dist = SimDist((*data)[it->second.query], (*data)[idx]);
    PPDEntry entry = PPDEntry(vect1, vect2, SimUpperOrEqual(dist));
    ppdtable->erase(entry);
  }
}

void Clusters::readClusters(const string &filename)
{
  ifstream file(filename.c_str(), ios::in);
  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file >> *this;
  file.close();
}

void Clusters::writeClusters(const string &filename) const
{
  ofstream file(filename.c_str(), ios::out);
  if (!file) {
    cerr << "can't open output file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file << *this;
  file.close();
}

void Clusters::readPPDtable(const string &filename)
{
  ifstream file(filename.c_str(), ios::in);
  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file >> *ppdtable;
  file.close();
}

void Clusters::writePPDtable(const string &filename) const
{
  ofstream file(filename.c_str(), ios::out);
  if (!file) {
    cerr << "can't open output file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file << *ppdtable;
  file.close();
}


ostream& operator<<(ostream &out, const ContQueryPivot &cont) 
{
  out << cont.size() << endl << endl;
  for(ContQueryPivot::const_iterator it = cont.begin(); it != cont.end(); ++it) 
    out << it->first << "\t" << it->second << endl;
  return out;
}

istream& operator>>(istream &in, ContQueryPivot &cont) 
{
  unsigned sz;
  in >> sz;
  for (unsigned i = 0; i < sz; i++) {
    unsigned pivot;
    QueryVect qv;
    in >> pivot >> qv;
    cont.insert(std::make_pair(pivot, qv));
  }
  return in;
}
