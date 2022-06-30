/*
  $Id: unittest.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 10/22/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topk.h"
#include "topksearch.h"
#include "util/src/output.h"
#include "util/src/io.h"

#include <iomanip>

using namespace std;

void testTopk()
{
  uint t = 0;

  /*

    +----+------+      +----+ +----+ +----+ +----+ 
    |  0 |      |      | #a | | ab | | bc | | c$ | 
    +----+------+      +----+ +----+ +----+ +----+ 
    |  1 | a    |      |  1 | |  3 | |  8 | |  8 | 
    +----+------+      +----+ +----+ +----+ +----+ 
    |  2 |      |      |  5 | |  4 | | 10 | | 10 | 
    +----+------+      +----+ +----+ +----+ +----+ 
    |  3 | xab  |      |  8 | |  5 | | 13 | | 13 | 
    +----+------+      +----+ +----+ +----+ +----+ 
    |  4 | xab  |                    | 15 | | 15 | 
    +----+------+                    +----+ +----+ 
    |  5 | ab   | 
    +----+------+ 
    |  6 |      | 
    +----+------+ 
    |  7 |      | 
    +----+------+ 
    |  8 | axbc | 
    +----+------+ 
    |  9 |      | 
    +----+------+ 
    | 10 | bc   | 
    +----+------+ 
    | 11 |      | 
    +----+------+ 
    | 12 |      | 
    +----+------+ 
    | 13 | bc   | 
    +----+------+ 
    | 14 |      | 
    +----+------+ 
    | 15 | bc   | 
    +----+------+ 
    | 16 |      | 
    +----+------+ 
    | 17 |      | 
    +----+------+ 
    | 18 |      | 
    +----+------+ 
    | 19 |      | 
    +----+------+ 

  */

  GramGenFixedLen gramGen(2);
  SimMetricJacc simMetric(gramGen);
  Query query("abc", simMetric, static_cast<uint>(1));
  uint topk;

  string data[] = {
    "", "a", "", "xab", "xab", 
    "ab", "", "", "axbc", "",
    "bc", "",  "", "bc", "", 
    "bc", "", "", "", ""};
  uint noData = sizeof(data) / sizeof(string);

  float *weights = new float[noData];
  uint *nograms = new uint[noData];

  for (uint i = 0; i < noData; i++) {
    // weights[i] = log(i + 1) / log(noData);
    weights[i] = 1 - log(static_cast<float>(i + 1)) / log(static_cast<float>(noData));
    nograms[i] = gramGen.getNumGrams(data[i]);
  }

  cout << fixed << setprecision(2);
  for (uint i = 0; i < noData; i++)
    cout << left << setw(4) << data[i] << ' '
         << right << simMetric(query.str, data[i]) << " + "
         << weights[i] << " = "
         << Topk::score(simMetric(query.str, data[i]), weights[i]) << endl;

  Topk::Index topkIndex;
  topkIndex.build(data, data + sizeof(data) / sizeof(string), gramGen);
  Topk::IndexQuery *topkIndexQuery;

  topkIndexQuery = new Topk::IndexQuery(topkIndex, query);
  Topk::Heap::getTopk(
    data, 
    weights, 
    topkIndex, 
    query, 
    *topkIndexQuery, 
    &topk);
  delete topkIndexQuery;
  
  assert(topk == 1); t++; // 1, 8

  topkIndexQuery = new Topk::IndexQuery(topkIndex, query);
  Topk::RoundRobin::getTopk(
    data, 
    weights, 
    nograms, 
    topkIndex, 
    query, 
    *topkIndexQuery, 
    &topk);
  delete topkIndexQuery;
  
  assert(topk == 1); t++; // 1, 8

  delete [] weights;
  delete [] nograms;

  cout << "topk (" << t << ")" << endl;
}

void testSearch()
{
  uint t = 0;

  vector<string> res;
  ofstream log("out.log", ios::out | ios::app);

  Topk::Search s(cout);
  cout << s;
  
  {
    string kwds[] = {"ABC", "xyz"};
    int counts[] = {1, 2};
    
    s.updateKwd(kwds, kwds + sizeof(kwds) / sizeof(string), counts);
    cout << s;
  }

  {
    string kwds[] = {"bcd", "xyz", "abc"};
    int counts[] = {3, -1, -1};
  
    s.updateKwd(kwds, kwds + sizeof(kwds) / sizeof(string), counts);
    cout << s;
  }

  {
    string kwds[] = {"bc", "xz"};

    s.searchKwd(
      kwds, kwds + sizeof(kwds) / sizeof(string), back_inserter(res));

    cout << res << endl;  
    res.clear();
  }
  
  string fname = "idx.bin";
  s.save(fname);

  Topk::Search s2(log);
  s2.load(fname);
  cout << s;
  assert(s == s2); t++;

  remove(fname.c_str());

  {
    string kwds[] = {"bc", "xz"};

    s2.searchKwd(
      kwds, kwds + sizeof(kwds) / sizeof(string), back_inserter(res));

    cout << res << endl;  
    res.clear();
  }

  {
    string kwds[] = {"abc", "xyz", "bcd"};
    int counts[] = {1, 1, -2};
    
    s2.updateKwd(kwds, kwds + sizeof(kwds) / sizeof(string), counts);
    cout << s2;
  }
  
  cout << "search (" << t << ")" << endl;  
}

void searchWords() 
{
  vector<string> lines, words;
  vector<int> counts;
  
  string filename = "words.txt";
  ifstream file(filename.c_str(), std::ios::in);

  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << std::endl;
    exit(EXIT_FAILURE);
  }

  READING_FILE(filename);
  while (!file.eof()) {
    string word;
    file >> word;
    words.push_back(word);
    
    int count;
    file >> count;
    counts.push_back(count);
  }
  READING_DONE();

  ofstream log("/dev/null", ios::out | ios::app);
  Topk::Search s(log);
  s.updateKwd(words, counts);
  
  words.clear();
  counts.clear();
  
  vector<string> result;
  
  words.push_back("thank");
  words.push_back("aaa");
  words.push_back("dsalfjalsjgfkldsh");
  
  s.searchKwd(words, result);
  
  cout << "query: " << words << endl;
  cout << "result: " << result << endl;;
}

int main()
{
  cout << "test..." << endl;

  testTopk();
  testSearch();
  // searchWords();
    
  cout << "OK" << endl;
}
