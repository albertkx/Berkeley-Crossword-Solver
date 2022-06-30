/*
  $Id: topk.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/20/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topk_h_
#define _topk_h_

#include "topkbase.h"
#include "topkiterative.h"
#include "topkroundrobin.h"
#include "topkheap.h"
#include "topkheap.v1.h"
#include "topktwophase.h"
#include "topkscan.h"

namespace Topk
{
  enum Alg 
  {
    AlgNotDefined = 0,
    AlgBase, 
    AlgIterative, 
    AlgRoundRobin, 
    AlgHeap, 
    AlgHeap_v1, 
    AlgTwoPhase
  };

  std::string getAlgName(Alg alg);
  uint getAlgVer(Alg alg);

  template<
    class RandomAccessIterator1, 
    class RandomAccessIterator2,
    class RandomAccessIterator3, 
    class OutputIterator>
  void runAlg(
    Alg alg, 
    const RandomAccessIterator1 data, 
    const RandomAccessIterator2 weights,
    const RandomAccessIterator3 nograms, 
    const Index &index, 
    const Query &query, 
    IndexQuery &indexQuery, 
    OutputIterator topk) 
  { 
    switch (alg) {
    case AlgRoundRobin:
      RoundRobin::getTopk(data, weights, nograms, index, query, indexQuery, topk); 
      break;
    case AlgHeap:
      Heap::getTopk(data, weights, index, query, indexQuery, topk); 
      break;
    default:
      break;
    }
  }

  template<
    class RandomAccessIterator1, 
    class RandomAccessIterator2, 
    class RandomAccessIterator3,
    class OutputIterator>
  void runAlg_v1(
    Alg alg, 
    const RandomAccessIterator1 data, 
    const RandomAccessIterator2 weights,
    const RandomAccessIterator3 nograms, 
    const Index_v1 &index_v1, 
    const Query &query, 
    IndexQuery_v1 &indexQuery_v1, 
    OutputIterator topk) 
  { 
    switch (alg) {
    case AlgIterative:
      Iterative::getTopk(data, weights, index_v1, query, indexQuery_v1, topk); 
      break;
    case AlgHeap_v1:
      Heap_v1::getTopk(data, weights, index_v1, query, indexQuery_v1, topk); 
      break;
    case AlgTwoPhase:
      TwoPhase::getTopk(data, weights, index_v1, query, indexQuery_v1, topk); 
      break;
    default:
      break;
    }
  }
}

#endif
