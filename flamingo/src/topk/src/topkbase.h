/*
  $Id: topkbase.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 07/17/2009
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkbase_h_
#define _topkbase_h_

#include <algorithm>

#include "topkindex.h"

namespace Topk
{
  namespace Base
  {
    template< 
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class OutputIterator> 
    void getTopk(
      const RandomAccessIterator1 data, 
      uint noData, 
      const RandomAccessIterator2 weights, 
      const QueryGrams &que,
      OutputIterator topk)
    {
      std::multiset<Answer, std::less<Answer> > ansTopk;
      std::set<uint> gramQueSet(que.str.begin(), que.str.end());

      for (uint i = 0; i < noData; ++i) {
        std::set<uint> gramDataSet(data[i].begin(), data[i].end()),
          gramInter;

        std::set_intersection(
          gramQueSet.begin(), 
          gramQueSet.end(), 
          gramDataSet.begin(), 
          gramDataSet.end(), 
          inserter(gramInter, gramInter.begin()));

        if (gramInter.size()) {
          float sim = 
            dynamic_cast<const SimMetricJacc*>(&que.sim)->operator()(
              data[i].size(), que.str.size(), gramInter.size());

          ansTopk.insert(Answer(i, score(sim, weights[i])));
          if (ansTopk.size() > que.k)
            ansTopk.erase(ansTopk.begin());
        }
        
      }

      for (std::multiset<Answer, std::less<Answer> >::const_reverse_iterator a = 
             ansTopk.rbegin(); a != ansTopk.rend(); ++a) {
        *topk++ = a->id;
      }
    }
  }
}

#endif
