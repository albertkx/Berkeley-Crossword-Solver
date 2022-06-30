/*
  $Id: topksearch.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 04/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topksearch.h"
#include "util/src/io.h"

#include <cassert>

using namespace std;
using namespace tr1;

namespace Topk 
{
  const uint Search::k = 3;
  const float Search::thr = .7;
  
  void Search::save(const string &filename) const 
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

  void Search::save(ofstream &file) const 
  {
    topkIndex.save(file);

    uint sz = kwds.size();
    file.write(reinterpret_cast<const char*>(&sz), sizeof(uint));

    for (vector<KwdInfo*>::const_iterator kwd = kwds.begin();
         kwd != kwds.end();
         ++kwd) {
      uchar ln = static_cast<uchar>((*kwd)->kwd.length());      
      file.write(reinterpret_cast<const char*>(&ln), sizeof(uchar));
      file.write((*kwd)->kwd.c_str(), (*kwd)->kwd.length());
      file.write(reinterpret_cast<const char*>(&(*kwd)->count), sizeof(int));
    }
  }

  void Search::load(const string &filename) 
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

  void Search::load(ifstream &file) 
  {
    topkIndex.load(file);

    free();
    kwds.clear();
    kwdToKwdInfo.clear();

    uint sz;
    file.read(reinterpret_cast<char*>(&sz), sizeof(uint));

    char *buf = new char[numeric_limits<uchar>::max()];
    for (uint i = 0; i < sz; i++) {
      uchar ln;
      file.read(reinterpret_cast<char*>(&ln), sizeof(uchar));
      file.read(buf, ln);

      int count;
      file.read(reinterpret_cast<char*>(&count), sizeof(int));
      
      string kwd(buf, ln);
      KwdInfo *kwdInfo = new KwdInfo(kwd, count);
      
      kwds.push_back(kwdInfo);
      kwdToKwdInfo.insert(make_pair(kwd, kwdInfo));
    }
    delete [] buf;

    updateWeights();
  }

  void Search::updateWeights()
  {
    int countMin = *min_element(countsBegin(), countsEnd(), lessIgnoreLessOne);
    transform(
      countsBegin(), 
      countsEnd(), 
      weightsBegin(), 
      bind2nd(ptr_fun(weight), countMin));
  }

  void Search::free()
  {
    for (std::vector<KwdInfo*>::const_iterator kwd = kwds.begin();
         kwd != kwds.end();
         ++kwd) {
      delete *kwd;
    }
  }

  ostream& operator<<(ostream& output, const KwdInfo& k)
  {
    return output << k.kwd << ", " << k.count << ", " << k.weight;
  }

  ostream& operator<<(ostream& output, const Search& s)
  {
    output << "== = search = ==" << endl;
    output << "-- - kwds - --" << endl;
    for (vector<KwdInfo*>::const_iterator kwd = s.kwds.begin();
         kwd != s.kwds.end();
         ++kwd) {
      output << **kwd << endl;
    }
    
    output << "== == ==" << endl;
    return output;
  }

  bool operator==(const Search& s1, const Search& s2) 
  {
    if (&s1 == &s2) {
      return true;
    }

    if (s1.kwds.size() != s2.kwds.size()) {
      return false;
    }

    vector<KwdInfo*>::const_iterator i1, i2;
    for (i1 = s1.kwds.begin(), i2 = s2.kwds.begin();
         i1 != s1.kwds.end() && i2 != s2.kwds.end();
         ++i1, ++i2) {
      if (**i1 != **i2) {
        return false;
      }      
    }
    if (i1 != s1.kwds.end() || i2 != s2.kwds.end()) {
      return false;
    }

    assert(s1.kwds.size() == s1.kwdToKwdInfo.size());
    assert(s2.kwds.size() == s2.kwdToKwdInfo.size());

    for (unordered_map<string, KwdInfo*>::const_iterator 
           p1 = s1.kwdToKwdInfo.begin();
         p1 != s1.kwdToKwdInfo.end();
         ++p1) {
      unordered_map<string, KwdInfo*>::const_iterator p2 = 
        s2.kwdToKwdInfo.find(p1->first);
      if (p2 == s2.kwdToKwdInfo.end()) {
        return false;
      }
      if (*p1->second != *p2->second) {
        return false;
      }
    }
      
    return true;
  }
}
