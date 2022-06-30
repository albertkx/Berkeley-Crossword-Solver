/*
  $Id: example.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 27/03/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <iostream>

#include "sepia.h"
#include "util/src/input.h"
#include "util/src/misc.h"

using namespace std;

int main() 
{
  const string 
    filenameData = "dataset.txt", 
    filenameSepia = "sepia";

  vector<string> data;
  readString(data, filenameData);

  Sepia sepia = Sepia(data, 2, 3, 10, 10);

  // 1. pre-processing (generate signatures)

  sepia.build();

  // 2. save

  sepia.saveData(filenameSepia);

  // 3. use

  string que = "delmare";
  unsigned ed = 2;

  cout << "query:\t" << que << endl;
  cout << "edit-dist:\t" << ed << endl;
  cout << "estimate selectivity:\t" << sepia.getEstimateSelectivity(que, ed)
       << endl;  
  cout << "real selectivity:\t" << sepia.getRealSelectivity(que, ed) << endl;
}

