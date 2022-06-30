/*
  $Id: cluster.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "cluster.h"

Cluster::Cluster(const Cluster &c): pivot(c.pivot), radius(c.radius)
{
  index = new ContCluster;
  *index = *c.index;
  ftable = new FTable;
  *ftable = *c.ftable;
}

Cluster& Cluster::operator=(const Cluster &c)
{
  if (this == &c)
    return *this;
  *index = *c.index;
  *ftable = *c.ftable;
  pivot = c.pivot;
  radius = c.radius;
  return *this;
}

bool Cluster::operator==(const Cluster &c) const 
{
  if (this == &c)
    return true;
  if (*index == *c.index && 
      pivot == c.pivot && 
      radius == c.radius && 
      *ftable == *c.ftable)
    return true;
  return false;
}

void Cluster::buildFTable(const vector<string> *data)
{
  for(ContCluster::const_iterator i = index->begin(); i != index->end(); i++)
    ftable->insert(SimVect((*data)[pivot], (*data)[*i]));
}

bool Cluster::improve(const vector<string> &data) 
{
  SimType minSum = 0;
  unsigned newPivot = 0;
  for (ContCluster::const_iterator i = index->begin(); i != index->end(); ++i) {
    SimType crtSum = 0;
    for (ContCluster::const_iterator j = index->begin(); j != index->end(); ++j)
      crtSum += SimDist(data[*i], data[*j]);
    if (i == index->begin() || crtSum < minSum) {
      minSum = crtSum;
      newPivot = *i;
    }
  }
  if (newPivot != pivot) {
    pivot = newPivot;
    return true;
  }
  return false;
}

ostream& operator<<(ostream &out, const Cluster &c)
{
  out << static_cast<unsigned>(c.index->size()) << endl;
  copy(c.index->begin(), c.index->end(), ostream_iterator<unsigned> (out, "\n"));
  out << endl << c.pivot << endl << endl;
  out << *c.ftable;
  out << endl << fixed << c.radius << endl;
  return out;
}

istream& operator>>(istream &in, Cluster &c)
{
  /* cluster input:
     - n: number of elements
     - \n
     - v: elements
     - \n
     - p: pivot
     - \n
     - ftable
     - \n
     - r: radius
     - \n
  */
  unsigned n;
  in>>n;
  //return in;
  for (unsigned i=0; i<n; i++) {
    unsigned v;
    in>>v;
    c.index->insert(v);
    //if (i==10) return in;
  }
  in>>c.pivot;
  in >> *c.ftable;
  in>>c.radius;
  return in;
}
