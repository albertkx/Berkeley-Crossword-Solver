/*
  $Id: topkindex.v1.h 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 04/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkindex_v1_h_
#define _topkindex_v1_h_

#include "foreign_util.h"
#include "common/src/gramgen.h"
#include "common/src/tr1_local.h"

namespace Topk {
  // assumes data elements are ordered decreasing by weight
  struct Index_v1 
  {
    uint lenMax, noGramsMin;

    std::tr1::unordered_map<uint, Array<uint>*> lists;

    Index_v1() {}
    Index_v1(const std::string &filename) { load(filename); }
    ~Index_v1();

    template<class InputIterator> 
    void build(InputIterator begin, InputIterator end, const GramGen &gramGen);

    void load(const std::string &filename);

    void save(const std::string &filename)
      const;
  };

  typedef std::vector<Array<uint>*> IndexQuery_v1;

  template<class InputIterator> 
  void Index_v1::build(
    InputIterator data,
    InputIterator data_end, 
    const GramGen &gramGen)
  {
    lenMax = 0;
    noGramsMin = ~0;

    std::tr1::unordered_map<uint, std::vector<uint>* > idx;

    for (uint id = 0; data != data_end; ++data, id++) {

      uint len = data->length();
      if (len > lenMax)
        lenMax = len;
    
      std::set<uint> grams;            // unique grams
      gramGen.decompose(*data, grams);

      uint noGrams = grams.size();
      if (noGrams < noGramsMin)
        noGramsMin = noGrams;

      for (std::set<uint>::const_iterator gram = grams.begin(); 
           gram != grams.end(); ++gram) {

        std::vector<uint> *lst;
      
        std::tr1::unordered_map<uint, std::vector<uint>*>::const_iterator pos = 
          idx.find(*gram);
        if (pos == idx.end()) {
          lst = new std::vector<uint>; // needs to be freed (deleted) later
          idx.insert(make_pair(*gram, lst));
        }
        else
          lst = pos->second;
        lst->push_back(id);
      }
    }

    for(std::tr1::unordered_map<uint, std::vector<uint>*>::const_iterator pos = 
          idx.begin(); pos != idx.end(); ++pos) {
      Array<uint>* lst = new Array<uint>(pos->second->size());
      for(std::vector<uint> ::const_iterator it = pos->second->begin();
          it != pos->second->end(); ++it)
        lst->append(*it);
      lists.insert(std::make_pair(pos->first, lst));
    }  
  }
}

#endif
