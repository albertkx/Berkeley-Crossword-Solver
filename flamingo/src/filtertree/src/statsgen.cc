/*
  $Id: statsgen.cc 5765 2010-10-19 04:36:20Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include "statsgen.h"
#include "util/src/input.h"
#include "util/src/misc.h"

StatsGenConfig::
StatsGenConfig() {
  gramGen = NULL;
  simMetric = NULL;
  simThresh = 0.0f;

  dictSizeStart = 0;
  dictSizeStop = 1;
  dictSizeStep = 0;  
  fanout = 0;
  maxStrLen = 0;
  autoPartFilter = false;
  
  numberQueries = 0;  
  distinctQueries = 0;
  queriesDistrib = QD_UNIFORM;
  zipfSkew = 1;
  numberRuns = 1;  

  qt = QueryRange;
  
  pmf = PMF_NONE;

  lcCharNum = 0;
  hcBuckets = 0;

  outputFlags = OF_WORKLOADSTATS | OF_QUERYRESULTS | OF_QUERYSTATS;  
  outputFilePrefix = string("output");
  
  reductionStart = 0;
  reductionStop = 0;
  reductionStep = 1;
  dictSamplingFrac = 0;
  queriesSamplingFrac = 0;
  sampleRatioCost = false;
  
  dropCachesBeforeEachQuery = false;
  dropCachesBeforeBuildingIndex = false;
  disableStreamBuffer = false;  
  runBuffer = 500000;
  scatteredOrg = false;
  pho = PHO_NONE; 
  avgStrLen = 0;
  bufferSlots = 0; 
}

void 
StatsGenConfig::
clearFilters() { 
  for(unsigned i = 0; i < filterTypes.size(); i++) {
    AbstractFilter* f = filterTypes.at(i);
    if(f) delete f;
  }
  filterTypes.clear();
}  

template<>
StringContainerVector*
createDefaultStringContainer(StatsGenConfig* config) {
  return new StringContainerVector(config->pho, true); 
}

template<>
StringContainerVector*
openStringContainer(StatsGenConfig* config) {
  cout << "ERROR: cannot open existing StringContainerVector" << endl;
  return new StringContainerVector(config->pho, true);
}

template<>
StringContainerRM*
createDefaultStringContainer(StatsGenConfig* config) {
  StringContainerRM* container = new StringContainerRM(config->pho, true);
  container->createContainer("default.rm", 8192, config->avgStrLen);
  container->openContainer("default.rm", config->disableStreamBuffer);
  return container;
}


template<>
StringContainerRM*
openStringContainer(StatsGenConfig* config) {
  StringContainerRM* strContainer = new StringContainerRM(false);
  strContainer->openContainer(config->existingStringContainer.c_str(), config->disableStreamBuffer);
  return strContainer;
}
