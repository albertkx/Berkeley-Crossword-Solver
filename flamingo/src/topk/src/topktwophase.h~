/*
  $Id: topktwophase.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/09/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topktwophase_h_
#define _topktwophase_h_

#include "topkiterative.h"
#include "topkheap.v1.h"

extern uint thresholdInit;

namespace Topk
{
  namespace TwoPhase
  {
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
      OutputIterator topk)
    {
      // assume threshold > 0
      const uint threshold = thresholdInit;

      float whtMinNeeded = 0;
      float whtMaxPossible = Iterative::_getWhtMax(weights, idxQue);
      uint thresholdMinNeeded = 1;

      // for Phase 2
      const float simMaxPossible = que.sim.getSimMax(
        que.len, que.noGrams, threshold - 1, threshold - 1);

      std::tr1::unordered_set<uint> checked;
      Cand *topkBuf = new Cand[que.k];
      std::fill(topkBuf, topkBuf + que.k, Cand());

      // struct timeval tStart, tEnd1, tEnd2;
      // gettimeofday(&tStart, 0);

      // === Phase 1 === Range Search -- Divide Skip
      if (threshold <= que.noGrams) {
        std::vector<uint> result;
        DivideSkipMerger<> mergeLists;

        mergeLists.merge(idxQue, threshold, result);
        Iterative::_selectTopk(data, weights, que, result, topkBuf, checked);

        const float topkLastScore = topkBuf[0].score;
        whtMinNeeded = scoreInverseWht(topkLastScore, simMaxPossible);
        thresholdMinNeeded = que.sim.getNoGramsMin(
          idx.lenMax, 
          idx.noGramsMin, 
          que.noGrams, 
          scoreInverseSim(topkLastScore, whtMaxPossible));
      }
 
      // gettimeofday(&tEnd1, 0);

      // === Phase 2 === Lists Traversal -- Divide Skip (TopkHeap)
      if (thresholdMinNeeded < threshold && whtMinNeeded <= whtMaxPossible)
        Heap_v1::_doDivideSkip(
          data, 
          weights, 
          idx, 
          que, 
          idxQue, 
          topkBuf,
          checked,
          simMaxPossible, 
          whtMinNeeded, 
          thresholdMinNeeded, 
          threshold);

      // gettimeofday(&tEnd2, 0);
      // float _tms = (tEnd1.tv_sec  - tStart.tv_sec ) * 1000 + 
      //   (tEnd1.tv_usec - tStart.tv_usec) / 1000., 
      //   _tms2 = (tEnd2.tv_sec  - tEnd1.tv_sec ) * 1000 + 
      //   (tEnd2.tv_usec - tEnd1.tv_usec) / 1000.;
      // std::cout << que.noGrams + 1 - threshold << " " 
      //           << _tms << " " << _tms2 << std::endl;

      std::sort_heap(topkBuf, topkBuf + que.k);
      for (uint i = 0; i < que.k; i++)
        *topk++ = topkBuf[i].id;
      delete [] topkBuf;
    }
  }
}

#endif
