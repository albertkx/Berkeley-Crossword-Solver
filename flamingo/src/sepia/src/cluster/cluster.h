/*
  $Id: cluster.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _cluster_h_
#define _cluster_h_

#include <limits>
#include <set>

#include "simfunc/simdist.h"
#include "ftable.h"

using namespace std;

typedef set<unsigned> ContCluster;

class Cluster {
private:
  ContCluster *index;
  unsigned pivot; // index in the data vector
  SimType radius;
  FTable *ftable;

public:
  Cluster():
    index(new ContCluster), pivot(0), radius(numeric_limits<SimType>::min()), 
    ftable(new FTable()) {}
  Cluster(const Cluster &c);
  ~Cluster() { delete index; delete ftable; }

  Cluster& operator=(const Cluster &c);
  bool operator==(const Cluster &c) const;

  void setPivot(const unsigned &p) { pivot=p; }
  void setRadius(const SimType r) { radius=r; }

  const unsigned getPivot() const { return pivot; }
  const SimType getRadius() const { return radius; }
  const FTable& getFTable() const { return *ftable; }

  const size_t size() const { return this->index->size(); }
  ContCluster::const_iterator begin() const { return index->begin(); }
  ContCluster::const_iterator end() const { return index->end(); }
  void clear() { index->clear(); radius = 0; }

  void buildFTable(const vector<string> *data);
  bool improve(const vector<string> &data);  

  ContCluster::const_iterator find(unsigned idx) const { return index->find(idx); }
  void insert(unsigned idx) { index->insert(idx); }
  void insert(unsigned idx, const SimVect vect) { 
    insert(idx); ftable->insert(vect); }
  void erase(unsigned idx, const SimVect vect) { 
    index->erase(idx); ftable->erase(vect); }

  friend ostream& operator<<(ostream &out, const Cluster &c);
  friend istream& operator>>(istream &in, Cluster &c);
};

typedef vector<Cluster> VectCluster;
typedef VectCluster::iterator VectClusterIt;

#endif
