/*
  $Id: perftest.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 27/03/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <sys/time.h>

#include <iostream>

#include "sepia.h"
#include "util/src/input.h"

using namespace std;

int main() 
{
  const string 
    filenameDataset = "dataset.txt";

  vector<string> data;
  readString(data, filenameDataset);

  Sepia sepia = Sepia(data, 2, 3);
  
  sepia.build();

  string s = "delmare";
  unsigned ed = 3;
  float f, err;
  unsigned r, time;
  struct timeval tv1, tv2;
  struct timezone tz;
  
  gettimeofday(&tv1, &tz);
  f = sepia.getEstimateSelectivity(s,ed);
  gettimeofday(&tv2, &tz);
  time = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);
  r = sepia.getRealSelectivity(s, ed);
  
  if (r)
    err = (f - r) / r;
  else
    err = f;

  cout << f << " " << r << " " << err << " " << time / 1000 << "ms" << endl;
}





