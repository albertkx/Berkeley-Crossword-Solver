/*
  $Id: topkindex.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 04/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkindex_h_
#define _topkindex_h_

#include <limits>
#include <list>

#include "common/src/query.h"
#include "common/src/tr1_local.h"
#include "util/src/querygroup.h"
#include "util/src/querygrams.h"
#include "util/src/range.h"

#ifdef _WIN32
// Necessary for search.dll (Visual C++)
#include "search/search_dll.h"
#else
#define SEARCH_API
#endif

// #define DEBUG
// #define DEBUG_STAT
// #define DEBUG_L2
#include "util/src/debug.h"

#ifdef DEBUG
#include <map>
extern uint simComp;
extern float simTime;
extern std::map<uint, uint> thrFreq;
extern std::map<uint, uint> thrFreqSim;
extern std::map<uint, std::map<uint, uint> > thrsFreq;
extern std::map<uint, std::map<uint, uint> > thrsFreqSim;
#endif

namespace Topk
{
  //=== Score 1 === 
  inline float score(float sim, float wht)
  { 
    return sim + wht;
  }

  inline float scoreInverseSim(float score, float wht)
  {
    return score - wht;
  }

  inline float scoreInverseWht(float score, float sim)
  {
    return score - sim;
  }

  //=== Score 2 === 
  /*
  inline float score(float sim, float wht)
  { 
    return sim * wht;
  }

  inline float scoreInverseSim(float score, float wht)
  {
    return wht ? score / wht : std::numeric_limits<float>::max();
  }

  inline float scoreInverseWht(float score, float sim)
  {
    return sim ? score / sim : std::numeric_limits<float>::max();
  }
  */

  //=== Score 3 -- no weight === 
  /*
  inline float score(float sim, float wht)
  {
    return sim;
  }

  inline float scoreInverseSim(float score, float wht)
  {
    return score;
  }

  inline float scoreInverseWht(float score, float sim)
  {
    return score - sim;
  }
  */

  // assumes data elements are ordered decreasing by weight
  struct Index 
  {
    uint lenMax, noGramsMin;

    std::tr1::unordered_map<uint, range<uint*> > lists;

    Index() {}
    Index(const std::string &filename) { load(filename); }
    ~Index();

    void clear();

    template<class InputIterator> 
    void build(InputIterator begin, InputIterator end, const GramGen &gramGen);

    template<class InputIterator>
    void buildFromGrams(
      InputIterator begin, InputIterator end, const GramGen &gramGen);

    void load(const std::string &filename);

    void save(const std::string &filename)
      const;

    void load(std::ifstream &file);

    void save(std::ofstream &file)
      const;
  };

  struct IndexQuery 
  {
    // uint size;
    std::list<range<uint*> > lists;

    // all the lists should have at least one element

    // SEARCH_API is necessary for search.dll (Visual C++)
    SEARCH_API IndexQuery(const Index &idx, const Query &query);
    IndexQuery(const Index &idx, const QueryGroup &queryGroup);
    IndexQuery(const Index &idx, const QueryGrams &queryGrams);
  };

  struct Cand
  {
    uint id;
    float score;
  
    Cand(
      uint id = std::numeric_limits<uint>::max(), 
      float score = std::numeric_limits<float>::min()):
      id(id), 
      score(score)
      {}

    bool operator<(const Cand &x) const { return score > x.score; }
  };

  template<class InputIterator> 
  void Index::build(
    InputIterator data,
    InputIterator data_end, 
    const GramGen &gramGen)
  {
    clear();
    
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
      uint sz = pos->second->size();
      uint* lst = new uint[sz];
      copy(pos->second->begin(), pos->second->end(), lst);
      lists.insert(std::make_pair(pos->first, make_range(lst, lst + sz)));
      delete pos->second;
    }    
  }

  template<class InputIterator> 
  void Index::buildFromGrams(
    InputIterator data,
    InputIterator data_end, 
    const GramGen &gramGen)
  {
    std::tr1::unordered_map<uint, std::vector<uint>* > idx;

    for (uint id = 0; data != data_end; ++data, id++) {
      uint noGrams = data->size();
      if (noGrams < noGramsMin)
        noGramsMin = noGrams;

      for (std::vector<uint>::const_iterator gram = data->begin(); 
           gram != data->end(); ++gram) {

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
      uint sz = pos->second->size();
      uint* lst = new uint[sz];
      copy(pos->second->begin(), pos->second->end(), lst);
      lists.insert(std::make_pair(pos->first, make_range(lst, lst + sz)));
      delete pos->second;
    }    
  }

  struct Answer 
  {
    uint id;
    float score;
    
    Answer(uint id, float score): 
      id(id), 
      score(score) 
    {}
    
    bool operator<(const Answer &a) const { return score < a.score; }
  };
}

#endif
