/*
  $Id: topkiterative.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 03/03/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkiterative_h_
#define _topkiterative_h_

#include "common/src/tr1_local.h"
#include "topkindex.h"
#include "topkindex.v1.h"
#include "foreign_listmerger.h"

extern uint thresholdInit;
extern bool thresholdPerf;
extern uint factorK;
extern float time1;

namespace Topk
{
  namespace Iterative
  {
    template<class RandomAccessIterator> 
    float _getWhtMax(
      const RandomAccessIterator weights, 
      const IndexQuery_v1 &idxQue) 
    {
      float whtMaxPossible = 0;
      for (IndexQuery_v1::const_iterator i = idxQue.begin(); 
           i != idxQue.end(); ++i) {
        const float whtFirst = weights[(**i)[0]];
        if (whtFirst > whtMaxPossible)
          whtMaxPossible = whtFirst;
      }
      return whtMaxPossible;
    }

    template <class RandomAccessIterator1, class RandomAccessIterator2> 
    void _selectTopk(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights,
      const Query &que, 
      const std::vector<uint> result, 
      Cand *topkBuf,
      std::tr1::unordered_set<uint> &checked)
    {
      for (std::vector<uint>::const_iterator i = result.begin(); 
           i != result.end(); ++i) {
        const uint id = *i;
        if (checked.insert(id).second) { // insert succeeded
          const float sc = score(que.sim(que.str, data[id]), weights[id]);
          if (sc > topkBuf[0].score) {
            std::pop_heap(topkBuf, topkBuf + que.k);
            topkBuf[que.k - 1] = Cand(id, sc);
            std::push_heap(topkBuf, topkBuf + que.k);
          }
        }
      }
    }

    template< 
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class OutputIterator> 
    void getTopk(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index_v1 &idx, 
      const Query &que,
      IndexQuery_v1 &idxQue, 
      OutputIterator &topk)
    {
      // const uint delta = static_cast<uint>(log2(que.k)) + 2;
      // const uint thresholdStart = que.noGrams > delta ? que.noGrams - delta : 1;

      const uint thresholdStart = thresholdInit;
      const uint thresholdStep = 1;

      uint threshold = thresholdStart;
      std::vector<uint> result;
      DivideSkipMerger<> mergeLists;

      while (result.size() < factorK * que.k && threshold > 0) {
        result.clear();
        mergeLists.merge(idxQue, threshold, result);
        threshold -= thresholdStep;
      }
      threshold += thresholdStep;

      // *topk++ = result.size();  // revert "for(uint i = 0" bellow

      Cand *topkBuf = new Cand[que.k];
      std::fill(topkBuf, topkBuf + que.k, Cand());
      std::tr1::unordered_set<uint> checked;
  
      _selectTopk(data, weights, que, result, topkBuf, checked);
        
      const float topkLastScore = topkBuf[0].score;  
      const float whtMaxPossible = _getWhtMax(weights, idxQue);
      const uint thresholdMinNeeded = que.sim.getNoGramsMin(
        idx.lenMax, 
        idx.noGramsMin, 
        que.noGrams, 
        scoreInverseSim(topkLastScore, whtMaxPossible));
  
      if (thresholdMinNeeded < threshold) {
        result.clear();
        mergeLists.merge(idxQue, thresholdMinNeeded, result);
        _selectTopk(data, weights, que, result, topkBuf, checked);
      }
      else
        thresholdPerf = true;

      std::sort_heap(topkBuf, topkBuf + que.k);
      for (uint i = 0; i < que.k; i++)
      // for (uint i = 1; i < que.k; i++) // for storing the result size above
        *topk++ = topkBuf[i].id;
      delete [] topkBuf;
    }
  }
}

#endif
