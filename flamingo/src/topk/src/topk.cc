/*
  $Id: topk.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/20/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topk.h"

using namespace std;

namespace Topk 
{
  string getAlgName(Alg alg) 
  {
    switch (alg) {
    case AlgBase:
      return "Base";
    case AlgIterative:
      return "Iterative";
    case AlgRoundRobin:
      return "RoundRobin";
    case AlgHeap:
      return "Heap";
    case AlgHeap_v1:
      return "Heap v1";
    case AlgTwoPhase:
      return "TwoPhase";
    default:
      return "Unknown";
    }
  }

  uint getAlgVer(Alg alg) {
    switch (alg) {
    case AlgRoundRobin:
    case AlgHeap:
      return 0;
    case AlgIterative:
    case AlgHeap_v1:
    case AlgTwoPhase:
      return 1;
    default:
      return 0;
    }
  }
}
