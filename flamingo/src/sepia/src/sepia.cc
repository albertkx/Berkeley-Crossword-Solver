/*
  $Id: sepia.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "sepia.h"

#include <sys/stat.h>

#include "cluster/lexic.h"
#include "cluster/medoids.h"
#include "freqest/freqest.h"
#include "util/src/input.h"
#include "util/src/misc.h"

const string Sepia::PARAM_SUFFIX = ".sep.param.bin";
const string Sepia::CLUST_SUFFIX = ".sep.clust.txt";
const string Sepia::PPD_SUFFIX = ".sep.pdd.txt";
const string Sepia::ERR_SUFFIX = ".sep.err.txt";

Sepia::Sepia(const vector<string> &dataset, 
             unsigned thresholdMin, unsigned thresholdMax):
  dataset(&dataset),
  thresholdMin(thresholdMin),
  thresholdMax(thresholdMax), 
  clustNo(1000),
  recNo(300),
  clustType(MEDOIDS_IMP),
  sampleType(CLOSE_RAND),
  samplePer(20), 
  sampleSizeMul(5),
  sampleSets(10), 
  sampleSteps(10),
  queueSize(10),
  uniqueNo(10)
{
}

Sepia::Sepia(const vector<string> &dataset, 
             unsigned thresholdMin, unsigned thresholdMax,
             unsigned clustNo, unsigned recNo,
             ClusterType clustType, SampleType sampleType,
             unsigned samplePer, unsigned sampleSizeMul,
             unsigned sampleSets, unsigned sampleSteps,
             unsigned queueSize, unsigned uniqueNo):
  dataset(&dataset),
  thresholdMin(thresholdMin),
  thresholdMax(thresholdMax), 
  clustNo(clustNo),
  recNo(recNo),
  clustType(clustType),
  sampleType(sampleType),
  samplePer(samplePer), 
  sampleSizeMul(sampleSizeMul),
  sampleSets(sampleSets), 
  sampleSteps(sampleSteps),
  queueSize(queueSize),
  uniqueNo(uniqueNo)
{
}

Sepia::Sepia(const vector<string> &dataset, 
             const string &filenamePrefix):
  dataset(&dataset)
{
  loadData(filenamePrefix);
}

void Sepia::build()
{
  // clusters
  switch (clustType) {
  case LEXIC:
    clust = new Lexic(dataset, clustNo, sampleType, samplePer, queueSize, 
                      uniqueNo);
    break;
  case MEDOIDS:
    clust = new Medoids(dataset, clustNo, sampleType, samplePer, queueSize, 
                        uniqueNo, 1, sampleSizeMul, sampleSets, sampleSteps);
    break;
  case MEDOIDS_IMP:
    clust = new Medoids(dataset, clustNo, sampleType, samplePer, queueSize, 
                        uniqueNo, 7, sampleSizeMul, sampleSets, sampleSteps);
    break;
  }
  clust->buildClusters();
  clust->buildFTables();

  // ppdtable
  clust->buildPPDtable();

  // errorcorr
  err = new ErrorCorr(dataset, clust, thresholdMin, thresholdMax, recNo);
  err->buildRecords();
  err->buildErrorCorr();
}

void Sepia::saveData(const string &filenamePrefix) const
{
  // save param
  string filenameParam = filenamePrefix + PARAM_SUFFIX;
  ofstream fileParam(filenameParam.c_str(), ios::out | ios::binary);
  if (!fileParam) {
    cerr << "can't open output file \"" << filenameParam << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  fileParam.write(reinterpret_cast<const char*>(&thresholdMin), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&thresholdMax), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);
  
  fileParam.write(reinterpret_cast<const char*>(&clustNo), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&clustType), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&sampleType), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&samplePer), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&sampleSizeMul), 
                  sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&sampleSets), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&sampleSteps), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&queueSize), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&uniqueNo), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.write(reinterpret_cast<const char*>(&recNo), sizeof(unsigned));
  if (fileParam.fail()) writeerror(filenameParam);

  fileParam.close();

  clust->writeClusters(filenamePrefix + CLUST_SUFFIX);
  clust->writePPDtable(filenamePrefix + PPD_SUFFIX);
  err->writeErrorCorr(filenamePrefix + ERR_SUFFIX);
}

void Sepia::loadData(const string &filenamePrefix) 
{
  if (!consistData(filenamePrefix)) {
    cerr << "data files are not consistent" << endl;
    exit(EXIT_FAILURE);
  }

  // load param
  string filenameParam = filenamePrefix + PARAM_SUFFIX;
  ifstream fileParam(filenameParam.c_str(), ios::in | ios::binary);
  if (!fileParam) {
    cerr << "can't open input file \"" << filenameParam << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  fileParam.read(reinterpret_cast<char*>(&thresholdMin), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&thresholdMax), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);
  
  fileParam.read(reinterpret_cast<char*>(&clustNo), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&clustType), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&sampleType), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&samplePer), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&sampleSizeMul), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&sampleSets), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&sampleSteps), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&queueSize), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&uniqueNo), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.read(reinterpret_cast<char*>(&recNo), sizeof(unsigned));
  if (fileParam.fail()) readerror(filenameParam);

  fileParam.close();

  // clusters
  clust = new Clusters(dataset, clustNo, sampleType, samplePer, queueSize, 
                       uniqueNo);
  clust->readClusters(filenamePrefix + CLUST_SUFFIX);

  // ppdtable
  clust->readPPDtable(filenamePrefix + PPD_SUFFIX);

  // errorcorr
  err = new ErrorCorr(dataset, clust, thresholdMin, thresholdMax, recNo);
  err->readErrorCorr(filenamePrefix + ERR_SUFFIX);
}

bool Sepia::consistData(const string &filenamePrefix) const
{
  const string
    filenameParam = filenamePrefix + PARAM_SUFFIX, 
    filenameClust = filenamePrefix + CLUST_SUFFIX, 
    filenamePPD = filenamePrefix + PPD_SUFFIX, 
    filenameErr = filenamePrefix + ERR_SUFFIX;

  struct stat attribParam, attribClust, attribPPD, attribErr;      

  if (stat(filenameParam.c_str(), &attribParam)) {
    cerr << "can't stat file \"" << filenameParam << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  if (stat(filenameClust.c_str(), &attribClust)) {
    cerr << "can't stat file \"" << filenameClust << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  if (stat(filenamePPD.c_str(), &attribPPD)) {
    cerr << "can't stat file \"" << filenamePPD << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  if (stat(filenameErr.c_str(), &attribErr)) {
    cerr << "can't stat file \"" << filenameErr << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  if (attribParam.st_mtime > attribClust.st_mtime)
    return false;
  if (attribClust.st_mtime > attribPPD.st_mtime)
    return false;
  if (attribPPD.st_mtime > attribErr.st_mtime)
    return false;

  return true;
}

float Sepia::getEstimateSelectivity(const string &query, 
                                    const unsigned editdist) const 
{
  Predicate p = Predicate(query, editdist);
  float f = freqEstFunc(*dataset, *clust, p);
  float rCor = err->getError(Record(p, f));
  if (rCor == -1)
    return f * 2;
  else
    return f / (rCor + 1);
}

unsigned Sepia::getRealSelectivity(const string &query, 
                                   const unsigned editdist) const 
{
  return freqRealFunc(*dataset, Predicate(query, editdist));
}

bool Sepia::operator==(const Sepia &s) const 
{
  if (this == &s)
    return true;
  if (dataset == s.dataset && 
      thresholdMin == s.thresholdMin && 
      thresholdMax == s.thresholdMax && 
      clustNo == s.clustNo && 
      clustType == s.clustType && 
      sampleType == s.sampleType && 
      samplePer == s.samplePer && 
      sampleSizeMul == s.sampleSizeMul && 
      sampleSets == s.sampleSets && 
      sampleSteps == s.sampleSteps && 
      queueSize == s.queueSize && 
      uniqueNo == s.uniqueNo && 
      recNo == s.recNo && 
      *clust == *s.clust && 
      *err == *s.err)
    return true;

  return false;
}
