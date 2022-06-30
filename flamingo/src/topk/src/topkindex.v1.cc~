/*
  $Id: topkindex.v1.cc 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 04/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topkindex.v1.h"
#include "util/src/io.h"

using namespace std;
using namespace tr1;

namespace Topk 
{
  Index_v1::~Index_v1() 
  {
    for (unordered_map<uint, Array<uint>*>::const_iterator it = lists.begin();
         it != lists.end(); ++it)
      delete it->second;
  }

  void Index_v1::load(const string &filename) 
  {
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);

    if (!file) {
      std::cerr << "can't open input file \"" << filename << "\"" << std::endl;
      exit(EXIT_FAILURE);
    }

    READING_FILE(filename);
    std::istream_iterator<binary_io<uint> > i(file);

    lenMax = *i++;
    noGramsMin = *i++;

    while (i != std::istream_iterator<binary_io<uint> >()) {
      uint gram = *i++;
      uint size = *i++;

      Array<uint> *lst = new Array<uint>(size); 
      for (uint j = 0; j < size; ++j) {
        uint id = *i++;

        lst->append(id);
      }
      lists.insert(make_pair(gram, lst));
    }
    READING_DONE();
  }

  void Index_v1::save(const string &filename) 
    const 
  {  
    std::ofstream file(filename.c_str(), std::ios::out | std::ios::binary);

    if (!file) {
      std::cerr << "can't open output file \"" << filename << "\"" << std::endl;
      exit(EXIT_FAILURE);
    }

    WRITING_FILE(filename);
    std::ostream_iterator<binary_io<uint> > o(file);

    *o++ = lenMax;
    *o++ = noGramsMin;

    for (unordered_map<uint, Array<uint>*>::const_iterator lst = lists.begin();
         lst != lists.end(); ++lst) {
      *o++ = lst->first;    
      *o++ = lst->second->size();
      for (uint i = 0; i < lst->second->size(); ++i)
        *o++= lst->second->at(i);
    }

    file.close();
    WRITING_DONE();
  }
}
