/*
  $Id: speedtest.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license.

  Date: 03/16/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <sys/time.h>

#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "appsearch.h"
#include "input.h"
#include "scan.h"
#include "partenum/partenum.h"
// #include "filtertree_old/filtertree.h"

using namespace std;

void run(AppSearch &appsearch, const vector<string> &data, 
         const string pathData, const string nameDataset, 
         const unsigned editdist) 
{
  const unsigned queryCount = 200;
  
  cout << "que:\t" << queryCount << endl;

  if (!existQuery(pathData, nameDataset, queryCount))
    genQuery(data, pathData, nameDataset, queryCount);

  vector<unsigned> query;
  readQuery(query, pathData, nameDataset, queryCount);

  vector<set<unsigned> > answer;
  readAnswer(answer, pathData, nameDataset, queryCount, editdist);

  appsearch.build();

  struct timeval tv1, tv2;
  struct timezone tz;
  unsigned time = 0;

  cerr << "run"; cerr.flush();
  for (unsigned que = 0; que < queryCount; que++) {
    if (que % 10 == 0) {
      cerr << '.'; cerr.flush();
    }

    // search
    vector<unsigned> results;
    gettimeofday(&tv1, &tz);
    appsearch.search(data[query[que]], editdist, results);
    gettimeofday(&tv2, &tz);
    time += (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec);

    // integrity check
    set<unsigned> resultsSet(results.begin(), results.end());
    if (resultsSet != answer[que]) {
      cout << "integrity check failed for query " << que << endl;
      exit(1);
    }  
  }
  cerr << "OK" << endl;

  cout << "time:\t"
       << static_cast<double>(time) / queryCount / 1000 << "ms" << endl;
}

int main(int argc, char *argv[])
{
  const string usage = "specify technique ID to use\n\
\t0: scan\n\
\t1: partenum\n\
\t2: filtertree\n";

  if (argc < 2) {
    cerr << usage;
    exit(1);
  }
  
  const unsigned
    type = atoi(argv[1]), 
    editdist = 3, 
    q = 3;

  cerr << "speed..." << endl;
  cout << fixed << setprecision(4);
  cout << "ed:\t" << editdist << endl
       << "q:\t" << q << endl;

  const string
    pathData = "", 
    nameDataset =  "in.txt";

  cerr << "data " << pathData + nameDataset << endl;
  vector<string> data;
  readString(data, pathData + nameDataset);

  AppSearch *appsearch;
  
  if (type == 0) {
    cout << "Scan" << endl;
    appsearch = new Scan(editdist, &data);
  }
  else if (type == 1) {
    cout << "PartEnum" << endl;
    /* scan:    ed  q n1 n2
       1  1  3  2
       2  2  3  7
       3  1  7  2
       bucket:  ed  q n1 n2
       1  2  2  8
       2  2  3  8
       3  1  2  7
    */
    const unsigned n1 = 7, n2 = 2;
    cout << "n1:\t" << n1 << endl
         << "n2:\t" << n2 << endl;
    appsearch = new PartEnum(data, 
                             q, editdist, n1, n2);
  }
//   else if (type == 2) {
//     cout << "FilterTree" << endl;
//     appsearch = new FilterTree(q, &data);
//   }
  else {
    cerr << usage
         << "unknown ID " << argv[1] << endl;
    exit(1);
  }
  
  run(*appsearch, data, pathData, nameDataset, editdist);

  delete appsearch;
}
