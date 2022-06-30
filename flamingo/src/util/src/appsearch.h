/*
  $Id: appsearch.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 03/16/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _appsearch_h_
#define _appsearch_h_

#include <set>
#include <string>
#include <vector>

using namespace std;

class AppSearch 
{
public:
  virtual ~AppSearch() {}
  virtual void build() = 0;
  virtual void search(const string &query, const unsigned editdist, 
                      vector<unsigned> &results) = 0;
  virtual void saveIndex(const string &filename) const = 0;
};

string getFilenameQuery(const string pathData, const string nameDataset, 
                        const unsigned queryCount);

string getFilenameAnswer(const string pathData, const string nameDataset, 
                         const unsigned queryCount, const unsigned editdist);

bool existQuery(const string pathData, const string nameDataset, 
                const unsigned queryCount);

void genQuery(const vector<string> &data, 
              const string pathData, const string nameDataset,
              const unsigned queryCount);

void readQuery(vector<unsigned> &query,
               const string pathData, const string nameDataset, 
               const unsigned queryCount);

void readAnswer(vector<set<unsigned> > &answer,
                const string pathData, const string nameDataset, 
                const unsigned queryCount, const unsigned editdist);

#endif
