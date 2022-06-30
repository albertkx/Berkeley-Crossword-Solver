/*
  $Id: example.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/16/2010
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topk.h"

using namespace std;

int main()
{
  /*
   * -- - Data - --
   */
  string data[]    = {"ab", "abc", "xab", "axbc", "bc"};
  float  weights[] = {  .1,    .2,    .4,     .2,   .3};
  string que = "abc";
  uint n = 5, k = 3;

  cout << "Data:" << endl << "---" << endl;
  for (uint i = 0; i < n; i++) {
    cout << data[i] << "\t" << weights[i] << endl;
  }
  cout << endl;

  /*
   * -- - Index - --
   */
  GramGenFixedLen gramGen(2);   // gram length 2
  Topk::Index topkIndex;
  topkIndex.build(data, data + n, gramGen);
  // topkIndex.save("index.bin");
  // topkIndex.load("index.bin");

  /*
   * -- - Query - --
   */
  SimMetricEdNorm simMetric(gramGen); // edit distance
  Query query(que, simMetric, k);
  Topk::IndexQuery topkIndexQuery(topkIndex, query);

  uint topk[k];
  Topk::Heap::getTopk(data, weights, topkIndex, query, topkIndexQuery, topk);

  cout << "Query: " << query.str << ", k:" << k << endl << endl;
  cout << "Results:" << endl << "---" << endl;
  for (uint i = 0; i < k; i++) {
    cout << i + 1 << " " << data[topk[i]] << endl;
  }

  return 0;
}
