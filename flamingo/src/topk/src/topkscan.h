/*
  $Id: topkscan.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 11/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkscan_h_
#define _topkscan_h_

#include <algorithm>
#include <cmath>

#include "common/src/tr1_local.h"
#include "topkindex.h"

namespace Topk
{
  namespace Scan
  {
    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2>  
    void getTopkCand(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      uint nData, 
      const Query &que, 
      Cand *topkBuf)
    {
      std::fill(topkBuf, topkBuf + que.k, Cand());
      
      float whtMin = std::numeric_limits<float>::min();
      for (uint id = 0; id < nData; ++id)
        if (weights[id] >= whtMin) {
          float sc = score(que.sim(que.str, data[id]), weights[id]);
          if (sc > topkBuf[0].score) {
            std::pop_heap(topkBuf, topkBuf + que.k);
            topkBuf[que.k - 1] = Cand(id, sc);
            std::push_heap(topkBuf, topkBuf + que.k);
            whtMin = scoreInverseWht(topkBuf[0].score, 1);
          }
        }

      std::sort_heap(topkBuf, topkBuf + que.k);
    }
  }
}

#endif

