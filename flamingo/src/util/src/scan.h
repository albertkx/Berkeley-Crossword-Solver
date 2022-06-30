/*
  $Id: scan.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 03/19/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _scan_h_
#define _scan_h_

#include <iostream>
#include <string>
#include <vector>

#include "appsearch.h"

using namespace std;

class Scan: public AppSearch 
{
private:
  unsigned editdist;
  const vector<string> *data;
public:
  Scan(const unsigned editdist, const vector<string> *data):
    editdist(editdist), data(data) {}
  void build() {}
  void search(const string &query, const unsigned editdist, 
              vector<unsigned> &results);
  void saveIndex(const string &filename) const {
    cerr << "Scan::saveIndex Not Implemented" << endl; }
};

#endif
