/*
  $Id: topkheap.v1.cc 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/20/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topkheap.v1.h"

namespace Topk 
{
  namespace Heap_v1
  {
    void _sortListsBySize(
      const std::vector<Array<uint>*> &allLists,
      const uint *const pointersIndex, 
      uint sortedIndex [])
    {
      std::vector<Array<uint>*>::const_iterator ite = allLists.begin();

      uint *sizeHeap = new uint[allLists.size()]; 
      uint *indexHeap = new uint[allLists.size()];

      uint index = 0;

      uint heapSize = 0;

      for (uint i = 0; i < allLists.size(); i++) {
        uint sizeOfList = allLists[i]->size() - pointersIndex[i];
    
        heapInsert(sizeOfList,index++,sizeHeap,indexHeap,heapSize);
    
      }

      index = 0;

      while (heapSize > 0) {
        sortedIndex[index++] = indexHeap[0];
        heapDelete(sizeHeap,indexHeap,heapSize);
      }

      delete [] sizeHeap;
      delete [] indexHeap;
    }
  }
}
