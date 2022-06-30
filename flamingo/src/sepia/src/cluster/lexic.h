/*
  $Id: lexic.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _lexic_h_
#define _lexic_h_

#include "clusters.h"

// the data set needs to be sorted

class Lexic: public Clusters {
public:
  Lexic(const vector<string> *d, const unsigned clusterNo, 
        const SampleType sampleType, const unsigned samplePer = 20,
        const unsigned queueSize = 10, const unsigned uniqueNo = 10);

  void buildClusters();

  ostream& info(ostream& out);
};

#endif
