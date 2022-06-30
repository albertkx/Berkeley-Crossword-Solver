/*
  $Id: clusters.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _clusters_h_
#define _clusters_h_

#include "src/sample.h"
#include "src/ppdtable/ppdtable.h"
#include "cluster.h"

typedef enum {LEXIC, MEDOIDS, MEDOIDS_IMP} ClusterType;

typedef enum {ALL_RAND, CLOSE_LEX, CLOSE_RAND, CLOSE_UNIQUE} SampleType;

class QueryVect
{
public:
  unsigned query;
  SimVect vect;
  QueryVect() {}
  QueryVect(unsigned query, SimVect vect): query(query), vect(vect) {}

  bool operator==(const QueryVect &q) const {
    return this == &q || (query == q.query && vect == q.vect); }

  friend ostream& operator<<(ostream& out, const QueryVect& qv);
  friend istream& operator>>(istream& in, QueryVect& qv);
};

typedef multimap<unsigned, QueryVect> ContQueryPivot;

class Clusters 
{
protected:
  const vector<string> *data;
  const unsigned clusterNo;
  const SampleType sampleType;
  const unsigned samplePer;
  const unsigned queueSize, uniqueNo;
  VectCluster *clusters;
  PPDTable *ppdtable;
  ContQueryPivot *queryPivot;

public:
  Clusters(const vector<string> *d, unsigned clusterNo, 
           SampleType sampleType, unsigned samplePer,
           unsigned queueSize, unsigned uniqueNo);

  virtual void buildClusters() {};
  void buildFTables();
  void buildPPDtable();

  void readClusters(const string &filename);
  void writeClusters(const string &filename) const;

  void readPPDtable(const string &filename);
  void writePPDtable(const string &filename) const;

  Clusters(const Clusters &cs);
  virtual ~Clusters();

  Clusters& operator=(const Clusters &cs);
  bool operator==(const Clusters &c) const;

  const size_t sizeCluster() const { return clusters->size(); }
  const size_t sizeEntries() const;
  const Cluster& getCluster(unsigned i) const { return (*clusters)[i]; }
  PPDTable& getPPDtable() const { return *ppdtable; }
  ContQueryPivot& getQueryPivot() const { return *queryPivot; }

  void push_backCluster(const Cluster &c) { this->clusters->push_back(c); }

  const VectClusterIt beginCluster() const { return clusters->begin(); }
  const VectClusterIt endCluster() const { return clusters->end(); }

  const ContPPDEntryIt beginPPDtable() const { return ppdtable->beginTable(); }
  const ContPPDEntryIt endPPDtable() const { return ppdtable->endTable(); }
  const ContPPDEntryIt findPPDtable(const PPDEntry &e) const { 
    return ppdtable->findTable(e); }
  const ContPPDEntryIt lower_boundPPDtable(const PPDEntry &e) const {
    return ppdtable->lower_boundTable(e); }
  const ContPPDEntryIt upper_boundPPDtable(const PPDEntry &e) const {
    return ppdtable->upper_boundTable(e); }

  bool isPivot(unsigned idx) const;
  void insert(unsigned idx);
  void erase(unsigned idx);

  void hist(ostream &out, const vector<string> &data) const;
  void stat(ostream &out, const vector<string> &data) const;
  ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const Clusters &cs);
  friend istream& operator>>(istream &in, Clusters &cs);
};

ostream& operator<<(ostream &out, const ContQueryPivot &cont);
istream& operator>>(istream &in, ContQueryPivot &cont);

#endif
