/*
  $Id: topkindex.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 04/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topkindex.h"
#include "util/src/io.h"

using namespace std;
using namespace tr1;

#ifdef DEBUG
uint simComp = 0;
float simTime = 0;
map<uint, uint> thrFreq;
map<uint, uint> thrFreqSim;
map<uint, map<uint, uint> > thrsFreq;
map<uint, map<uint, uint> > thrsFreqSim;
#endif

namespace Topk 
{
  Index::~Index() 
  {
    clear();
  }

  void Index::clear() 
  {
    for (unordered_map<uint, range<uint*> >::const_iterator it = lists.begin();
         it != lists.end(); 
         ++it)
      delete [] it->second._begin;
    lists.clear();
  }

  void Index::save(const string &filename) 
    const
  {
    ofstream file(filename.c_str(), ios::out | ios::binary);

    if (!file) {
      cerr << "can't open output file \"" << filename << "\"" << endl;
      exit(EXIT_FAILURE);
    }

    WRITING_FILE(filename);
    save(file);
    file.close();
    WRITING_DONE();
  }

  void Index::save(ofstream &file)
    const
  {
    ostream_iterator<binary_io<uint> > out(file);

    *out++ = lenMax;
    *out++ = noGramsMin;
    *out++ = lists.size();

    for (unordered_map<uint, range<uint*> >::const_iterator lst = lists.begin();
         lst != lists.end(); ++lst) {
      *out++ = lst->first;    
      *out++ = lst->second._end - lst->second._begin;

      copy(lst->second._begin, lst->second._end, out);
    }
  }

  void Index::load(const string &filename)
  {
    ifstream file(filename.c_str(), ios::in | ios::binary);

    if (!file) {
      cerr << "can't open input file \"" << filename << "\"" << endl;
      exit(EXIT_FAILURE);
    }

    READING_FILE(filename);
    load(file);
    file.close();
    READING_DONE();
  }

  void Index::load(ifstream &file)
  {
    clear();

    istream_iterator<binary_io<uint> > in(file);

    uint nList;

    lenMax = *in++;
    noGramsMin = *in++;
    nList = *in;

    for (uint list = 0; list < nList; ++list) {
      ++in;
      uint gram = *in++;
      uint size = *in;

      uint *lst = new uint[size];
      for (uint j = 0; j < size; ++j) {
        ++in;
        lst[j] = *in;
      }
        
      lists.insert(make_pair(gram, make_range(lst, lst + size)));
    }
  }

  IndexQuery::IndexQuery(const Index &idx, const Query &que) 
  {
    set<uint> grams;
    que.sim.gramGen.decompose(que.str, grams);
 
    for (set<uint>::const_iterator gram = grams.begin(); 
         gram != grams.end(); ++gram) {
      unordered_map<uint, range<uint*> >::const_iterator 
        ls = idx.lists.find(*gram);
      if (ls != idx.lists.end()) {
        lists.push_back(ls->second);
      }
    }
  }

  IndexQuery::IndexQuery(const Index &idx, const QueryGroup &queGrp) 
  {
    set<uint> grams;

    for (uint i = 0; i < queGrp.n; ++i)
      queGrp.sim.gramGen.decompose(queGrp.strs[i], grams);
    
    OUTPUT("no grams ", grams.size());

    for (set<uint>::const_iterator gram = grams.begin(); 
         gram != grams.end(); ++gram) {
      // cout << *gram << " ";
      unordered_map<uint, range<uint*> >::const_iterator 
        ls = idx.lists.find(*gram);
      if (ls != idx.lists.end())
        lists.push_back(ls->second);
    }

    // cout << endl;

  }

  IndexQuery::IndexQuery(const Index &idx, const QueryGrams &que) 
  {
    OUTPUT("no grams ", que.str.size());

    for (vector<uint>::const_iterator gram = que.str.begin(); 
         gram != que.str.end(); ++gram) {
      // cout << *gram << " ";
      unordered_map<uint, range<uint*> >::const_iterator 
        ls = idx.lists.find(*gram);
      if (ls != idx.lists.end())
        lists.push_back(ls->second);
    }

    // cout << endl;

  }
}
