/*
  $Id: medoids.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "medoids.h"

#include <cstdlib>

Medoids::Medoids(const vector<string>* d, unsigned clusterNo, 
                 SampleType sampleType, unsigned samplePer,
                 unsigned queueSize, unsigned uniqueNo, 
                 unsigned ver, unsigned sampleSizeMul,
                 unsigned sampleSets, unsigned sampleSteps):
  Clusters(d, clusterNo, sampleType, samplePer, queueSize, uniqueNo),
  ver(ver), sampleSizeMul(sampleSizeMul), sampleSets(sampleSets),
  sampleSteps(sampleSteps)
{
  for (unsigned i = 0; i < clusterNo; i++)
    clusters->push_back(Cluster());

  sampleSize = sampleSizeMul * clusterNo;
  if (sampleSize > d->size()) 
    sampleSize = d->size();

  time_t ltime;
  time(&ltime);
  srand(static_cast<unsigned int>(ltime));
}

void Medoids::buildClusters() 
{
  cerr << "cluster"; cerr.flush();
  
  switch (ver) {
  case 1:
    buildClustersVer1();
    break;
  case 2:
    buildClustersVer2();
    break;
  case 3:
    buildClustersVer3();
    break;
  case 5:
    buildClustersVer5();
    break;
  case 6:
    buildClustersVer6();
  case 7:
    buildClustersVer7();
    break;
  } 

  cerr << "OK" << endl;
}

void Medoids::buildClustersVer1() 
{
  const unsigned dataSize = static_cast<unsigned>(data->size());

  unsigned *pivot = new unsigned[clusterNo];
  unsigned *pivotFinal = new unsigned[clusterNo];
  unsigned *sample = new unsigned[sampleSize];

  set<unsigned>
    *idSet = new set<unsigned>, 
    *dataSet = new set<unsigned>;

  SimType minSumDist = 0;

  for (unsigned l = 0; l < sampleSets; l++) {
    cerr << '.'; cerr.flush();

    // ---=== SAMPLE ===---
    // select sample records
    for (unsigned i = 0; i < dataSize; i++) dataSet->insert(i);
    for (unsigned i = 0; i < sampleSize; i++) {
      set<unsigned>::iterator it = dataSet->
        lower_bound(rand() % static_cast<unsigned>(dataSet->size()));
      unsigned id = *it;
      sample[i] = id;
      dataSet->erase(it);
    }

    for (unsigned k = 0; k < sampleSteps; k++){// ---=== BUILD ===---
      // select random pivots
      idSet->clear();
      for (unsigned i = 0; idSet->size() < clusterNo;) { // BUILD (select pivots)
        unsigned j = sample[rand()%sampleSize];
        if (idSet->insert(j).second)
          pivot[i++] = j;
      }
		
      // assign each sample element to a pivot (to a cluster) and
      // compute the sum of dissimilarities  (sum of objective functions)
      SimType sumDist = 0;
      for (unsigned i = 0; i < sampleSize; i++) {
        // find the best pivot
        SimType minDist = 0;
        for (unsigned j = 0; j < clusterNo; j++) {
          SimType dist = SimDist((*data)[sample[i]], (*data)[pivot[j]]);
          if (j == 0 || dist < minDist)
            minDist = dist;
        }
        sumDist+=minDist;
      }

      if ((l == 0 && k == 0) || sumDist < minSumDist) {
        minSumDist = sumDist;
        unsigned *pivotExtra;
        pivotExtra = pivotFinal; pivotFinal = pivot; pivot = pivotExtra;
      }
    }
  }

  delete dataSet;
  delete idSet;
  delete [] sample;

  // set final pivots as pivots
  for (unsigned i = 0; i < clusterNo; i++)
    (*clusters)[i].setPivot(pivotFinal[i]);

  delete [] pivotFinal;
  delete [] pivot;

  assignClusters();
}

void Medoids::buildClustersVer2() 
{
  const unsigned dataSize = static_cast<unsigned>(data->size());

  unsigned *pivot = new unsigned[clusterNo];
  unsigned *pivotFinal = new unsigned[clusterNo];

  set<unsigned> *idSet = new set<unsigned>;

  SimType minSumDist = 0;
  for (unsigned k = 0; k < sampleSteps; k++){
    cerr << '.'; cerr.flush();

    // ---=== BUILD ===---
    // select random pivots
    idSet->clear();
    for (unsigned i = 0; idSet->size() < clusterNo;) { // BUILD (select pivots)
      unsigned j = rand()%dataSize;
      if (idSet->insert(j).second)
        pivot[i++] = j;
    }
	
    // assign each sample element to a pivot (to a cluster) and
    // compute the sum of dissimilarities  (sum of objective functions)
    SimType sumDist = 0;
    for (unsigned i = 0; i < dataSize; i++) {
      // find the best pivot
      SimType minDist = 0;
      for (unsigned j = 0; j < clusterNo; j++) {
        SimType dist = SimDist((*data)[i], (*data)[pivot[j]]);
        if (j == 0 || dist < minDist)
          minDist = dist;
      }
      sumDist+=minDist;
    }

    if (k == 0 || sumDist < minSumDist) {
      minSumDist = sumDist;
      unsigned *pivotExtra;
      pivotExtra = pivotFinal; pivotFinal = pivot; pivot = pivotExtra;
    }
  }

  delete idSet;

  // set final pivots as pivots
  for (unsigned i = 0; i < clusterNo; i++)
    (*clusters)[i].setPivot(pivotFinal[i]);

  delete [] pivotFinal;
  delete [] pivot;

  assignClusters();
}

void Medoids::buildClustersVer3() 
{
  const unsigned dataSize = static_cast<unsigned>(data->size());

  unsigned *pivot = new unsigned[clusterNo];
  unsigned *pivotFinal = new unsigned[clusterNo];
  unsigned *sample = new unsigned[sampleSize];

  set<unsigned> *idSet = new set<unsigned>;

  SimType minSumDist = 0;
  for (unsigned k = 0; k < sampleSteps; k++){
    cerr << '.'; cerr.flush();

    // ---=== SAMPLE ===---
    // select sample records
    idSet->clear();

    if (k != 0) { // we already have a set of pivotFinal
      // add the set of pivotFinal to the sample
      for (unsigned i = 0; i < clusterNo; i++) {
        idSet->insert(pivotFinal[i]);
        sample[i] = pivotFinal[i];
      }
    }

    for (unsigned i = idSet->size(); idSet->size() < sampleSize;) { // 1st SAMPLE
      unsigned j = rand()%dataSize;
      if (idSet->insert(j).second)
        sample[i++] = j;
    }

    // ---=== BUILD ===---	
    //     idSet->clear();
    //     for (unsigned i = 0; i < sampleSize; i++) idSet->insert(sample[i]);

    // first pivot
    unsigned pivotId = 0;
    SimType minSumDistPivot = 0;
    for (set<unsigned>::iterator i = idSet->begin(); i != idSet->end(); i++) {
      SimType sumDist = 0;
      for (set<unsigned>::iterator j = idSet->begin(); j != idSet->end(); j++)
        sumDist+=SimDist((*data)[*i], (*data)[*j]);
      if (i == idSet->begin() || sumDist < minSumDistPivot) {
        pivotId = *i;
        minSumDistPivot = sumDist;
      }
    }
    pivot[0] = pivotId; // we have the first
    idSet->erase(pivotId);

    // rest of the pivots
    for (unsigned m = 1; m < clusterNo; m++) {
      SimType maxSumCji = 0;
      unsigned pivotId = 0; // the best unselected object - the new pivot

      for (set<unsigned>::iterator i = idSet->begin(); i != idSet->end(); i++) {
        SimType sumCji = 0;
        for (set<unsigned>::iterator j = idSet->begin(); j != idSet->end(); j++) {
          // compute Dj
          SimType Dj = 0;
          for (unsigned l = 0; l < m; l++) {
            SimType dist = SimDist((*data)[*j], (*data)[pivot[l]]);
            if (l == 0 || dist < Dj) {
              Dj = dist;
            }
          }

          // compute Cji
          SimType dist = SimDist((*data)[*j], (*data)[*i]);
          SimType Cji = Dj>dist?Dj-dist:0;

          // add to total
          sumCji+=Cji;
        }

        if (i == idSet->begin() || sumCji>maxSumCji) {
          maxSumCji = sumCji;
          pivotId = *i;
        }
      }

      pivot[m] = pivotId;
      idSet->erase(pivotId);
    }

    // assign each sample element to a pivot (to a cluster) and
    // compute the sum of dissimilarities  (sum of objective functions)
    SimType sumDist = 0;
    for (unsigned i = 0; i < sampleSize; i++) {
      // find the best pivot
      SimType minDist = 0;
      for (unsigned j = 0; j < clusterNo; j++) {
        SimType dist = SimDist((*data)[sample[i]], (*data)[pivot[j]]);
        if (j == 0 || dist < minDist)
          minDist = dist;
      }
      sumDist+=minDist;
    }

    if (k == 0 || sumDist < minSumDist) {
      minSumDist = sumDist;
      unsigned *pivotExtra;
      pivotExtra = pivotFinal; pivotFinal = pivot; pivot = pivotExtra;
    }
  }

  delete idSet;
  delete [] sample;

  // set final pivots as pivots
  for (unsigned i = 0; i < clusterNo; i++)
    (*clusters)[i].setPivot(pivotFinal[i]);

  delete [] pivotFinal;
  delete [] pivot;
	
  assignClusters();	
}

void Medoids::buildClustersVer5() 
{ 
  const unsigned dataSize = data->size();
  vector<unsigned> *pivots = new vector<unsigned>(clusterNo);
  SimType sumMin = 0;
  for (unsigned i = 0; i < dataSize; i++) {
    SimType sumCrt = 0;
    for (unsigned j = 0; j < dataSize; j++) {
      if (i == j) continue;
      sumCrt += SimDist((*data)[i], (*data)[j]);
    }
    if (i == 0 || sumCrt < sumMin) {
      sumMin = sumCrt;
      (*pivots)[0] = i;
    }
  }
  cerr << '.'; cerr.flush();

  vector<unsigned> *assigned = new vector<unsigned>();
  assigned->reserve(dataSize);
  vector<SimType> *distances = new vector<SimType>();
  distances->reserve(dataSize);
  for (unsigned i = 0; i < dataSize; i++) {
    (*assigned)[i] = 0;
    (*distances)[i] = SimDist((*data)[(*pivots)[0]], (*data)[i]);
  }
  cerr << '.'; cerr.flush();

  for (unsigned i = 1; i < clusterNo; i++) {
    SimType maxGain = 0;
    for (unsigned j = 0; j < dataSize; j++) {
      unsigned k;
      for (k = 0; k < i && (*pivots)[k] != j; k++);
      if (k != i) continue;

      SimType crtGain = 0;
      for (k = 0; k < dataSize; k++) {
        SimType crtDist = SimDist((*data)[j], (*data)[k]);
        if (crtDist < (*distances)[k]) crtGain += (*distances)[k] - crtDist;
      }
      if (crtGain > maxGain) {
        maxGain = crtGain;
        (*pivots)[i] = j;
      }
    }
    for (unsigned j = 0; j < dataSize; j++) {
      SimType newDist = SimDist((*data)[(*pivots)[i]], (*data)[j]);
      if (newDist < (*distances)[j]) {
        (*assigned)[j] = i;
        (*distances)[j] = newDist;
      }
    }
  }
  cerr << '.'; cerr.flush();

  for (unsigned i = 0; i < clusterNo; i++) 
    (*clusters)[i].setPivot((*pivots)[i]);
  for (unsigned i = 0; i < dataSize; i++) {
    unsigned clusterId = (*assigned)[i];
    (*clusters)[clusterId].insert(i);
    (*clusters)[clusterId].setRadius(max((*clusters)[clusterId].getRadius(),
                                         (*distances)[i]));
  }
  delete distances;
  delete assigned;
  delete pivots;
}

void Medoids::buildClustersVer6() 
{
  const unsigned dataSize = data->size();
  set<unsigned> *pivots = new set<unsigned>();

  time_t ltime;
  time(&ltime);
  srand(static_cast<unsigned>(ltime));

  while (pivots->size() < clusterNo) pivots->insert(rand() % dataSize);
  
  unsigned i = 0;
  for (set<unsigned>::iterator it = pivots->begin(); 
       it != pivots->end(); ++it) (*clusters)[i++].setPivot(*it);

  delete pivots;

  assignClusters();

  for (unsigned i = 0; i < sampleSteps; i++) {
    cerr << '.'; cerr.flush();
    
    bool changed = false;
    for (VectClusterIt it = clusters->begin(); it != clusters->end(); ++it) 
      if (it->improve(*data)) changed = true;

    if (changed) {
      for (VectClusterIt it = clusters->begin(); 
           it != clusters->end(); ++it) it->clear();
      assignClusters();
    }
    else break;
  }
}

void Medoids::buildClustersVer7() 
{
  const unsigned dataSize = data->size();

  time_t ltime;
  time(&ltime);
  srand(static_cast<unsigned>(ltime));

  Sample
    samples = Sample(sampleSize, dataSize),
    pivots = Sample(clusterNo, sampleSize);

  unsigned i = 0;
  for (vector<unsigned>::const_iterator it = pivots.begin(); 
       it != pivots.end(); ++it)
    (*clusters)[i++].setPivot(samples[*it]);

  assignClusters(samples.begin(), samples.end());

  for (unsigned i = 0; i < sampleSteps; i++) {
    cerr << '.'; cerr.flush();

    bool changed = false;
    for (VectClusterIt it = clusters->begin(); it != clusters->end(); ++it) 
      if (it->improve(*data)) changed = true;

    if (changed) {
      for (VectClusterIt it = clusters->begin(); 
           it != clusters->end(); ++it) it->clear();
      assignClusters(samples.begin(), samples.end());
    }
    else break;
  }

  assignClustersMinus(samples.begin(), samples.end());
}

void Medoids::assignClusters() 
{
  const unsigned dataSize = data->size();

  cerr << '.'; cerr.flush();

  // assign each data element to a pivot (to a cluster)
  for (unsigned i = 0; i < dataSize; i++) {
    SimType minDist = 0;
    unsigned clusterI = 0;
		
    // find the best pivot
    for (unsigned j = 0; j < clusterNo; j++) {
      if (i == (*clusters)[j].getPivot()) {
        minDist = 0;
        clusterI = j;
        break;
      }
      SimType dist = SimDist((*data)[i], (*data)[(*clusters)[j].getPivot()]);
      if (j == 0 || dist < minDist) {
        minDist = dist;
        clusterI = j;
      }	
    }

    (*clusters)[clusterI].insert(i);
    (*clusters)[clusterI].setRadius(max((*clusters)[clusterI].getRadius(), 
                                        minDist));
  }
}

void Medoids::assignClusters(vector<unsigned>::const_iterator begin, 
                             vector<unsigned>::const_iterator end)
{
  cerr << '.'; cerr.flush();

  // assign each data element to a pivot (to a cluster)
  for (vector<unsigned>::const_iterator i = begin; i != end; ++i) {
    SimType minDist = 0;
    unsigned clusterI = 0;
		
    // find the best pivot
    for (unsigned j = 0; j < clusterNo; j++) {
      if (*i == (*clusters)[j].getPivot()) {
        minDist = 0;
        clusterI = j;
        break;
      }
      SimType dist = SimDist((*data)[*i], (*data)[(*clusters)[j].getPivot()]);
      if (j == 0 || dist < minDist) {
        minDist = dist;
        clusterI = j;
      }	
    }

    (*clusters)[clusterI].insert(*i);
    (*clusters)[clusterI].setRadius(max((*clusters)[clusterI].getRadius(), 
                                        minDist));
  }
}

void Medoids::assignClustersMinus(vector<unsigned>::const_iterator begin, 
                                  vector<unsigned>::const_iterator end)
{
  cerr << '.'; cerr.flush();

  set<unsigned> samples = set<unsigned>(begin, end);

  const unsigned dataSize = data->size();

  // assign each data element to a pivot (to a cluster)
  for (unsigned i = 0; i < dataSize; i++) {
    if (samples.find(i) != samples.end()) continue;

    SimType minDist = 0;
    unsigned clusterI = 0;
		
    // find the best pivot
    for (unsigned j = 0; j < clusterNo; j++) {
      if (i == (*clusters)[j].getPivot()) {
        minDist = 0;
        clusterI = j;
        break;
      }
      SimType dist = SimDist((*data)[i], (*data)[(*clusters)[j].getPivot()]);
      if (j == 0 || dist < minDist) {
        minDist = dist;
        clusterI = j;
      }	
    }

    (*clusters)[clusterI].insert(i);
    (*clusters)[clusterI].setRadius(max((*clusters)[clusterI].getRadius(), 
                                        minDist));
  }
}

ostream& Medoids::info(ostream &out)
{
  Clusters::info(out);
  out << "Cluster method\tMedoids " << ver << endl;
  out << "sample size\t" << sampleSizeMul << endl;
  out << "sample sets\t" << sampleSets << endl;
  out << "sample steps\t" << sampleSteps << endl;
  return out << endl;
}

ostream& output(ostream &out, const unsigned* p, const unsigned k, 
                const vector<string> &d)
{
  for (unsigned i = 0; i < k; i++)
    out << '[' << i << "]\t" << p[i] << "\t(" << d[p[i]] << ')' << endl;
  return out;
}

ostream& output(ostream &out, const Closest* c, const unsigned nK, 
                const unsigned* p, const unsigned* np) 
{
  for (unsigned i = 0; i < nK; i++)
    out << np[i] << ":\t(" << p[c[i].d] << ',' << p[c[i].e] << ')' << endl;
  return out;
}
