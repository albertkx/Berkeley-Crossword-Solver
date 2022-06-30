/*
  $Id: topkheap.v1.h 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/20/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkheap_v1_h_
#define _topkheap_v1_h_

#include <cstring>

#include "topkindex.h"
#include "topkindex.v1.h"
#include "topkheap.h"
#include "foreign_listmerger.h"

extern float heap_c1;

namespace Topk
{
  namespace Heap_v1
  {
    struct _SetNoOperation 
    {
      void* find(uint) { return 0; }
      void* end() { return 0; }
    };

    void _sortListsBySize(
      const std::vector<Array<uint>*> &allLists,
      const uint *const pointersIndex, 
      uint sortedIndex []);    

    template<class RandomAccessIterator>
    void _insertHeap(
      uint dataHeap[],
      uint indexHeap[],
      uint &heapSize,
      const std::vector<Array<uint>*> &lists,
      uint pointersIndexList[],			  
      uint vectorIndexContainer[],
      uint containerSize, 
      RandomAccessIterator weights, 
      float whtMin)
    {
      for(uint i=0; i<containerSize; i++) {
        uint index = vectorIndexContainer[i];
        uint position = pointersIndexList[index];
        uint newData = (lists.at(index))->at(position);
        if (newData != ~0u && weights[newData] < whtMin)
          newData = ~0;
        heapInsert(newData, index, dataHeap, indexHeap, heapSize);  
      }//end for
    }//end _insertHeap

    template<class RandomAccessIterator> 
    void _makeHeap(
      const std::vector<Array<uint>*> &lists, 
      const uint *const pointersIndex, 
      const bool *const longListMask, 
      uint dataHeap[],
      uint indexHeap[],
      uint &size, 
      RandomAccessIterator weights, 
      float whtMin)
    {
      size = 0;
      for(uint i=0; i< lists.size(); i++)
        if (!longListMask[i]) {	
          uint t = lists.at(i)->at(pointersIndex[i]);	
          if (t != ~0u && weights[t] < whtMin)
            t = ~0;
          heapInsert(t, i, dataHeap, indexHeap, size);
        }//end if
    }//end _makeHeap

    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class Set>
    void _doDivideSkip(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index_v1 &idx, 
      const Query &que, 
      const IndexQuery_v1 &idxQue,
      Cand *topkBuf, 
      Set &checked, 
      float simMaxPossible = 1, 
      float whtMinNeeded = std::numeric_limits<float>::min(), 
      uint thresholdMinNeeded = 1, 
      uint thresholdChecked = std::numeric_limits<uint>::max()) 
    {
      const uint MAXUnsigned = ~0;
      const uint numberOfInvertedList = idxQue.size();
      const uint batchThreshold = 10;
      const uint batchLongLists = 3;
      const uint thresholdMax = std::min(thresholdChecked - 1, numberOfInvertedList);

      uint threshold = thresholdMinNeeded;
      uint longListsSize = 0, longListsSizeNew;
  
      uint *pointersIndex = new uint[numberOfInvertedList];
      memset(pointersIndex, 0, sizeof(uint) * numberOfInvertedList);

      uint *sortedIndex = new uint[numberOfInvertedList];
      _sortListsBySize(idxQue, pointersIndex, sortedIndex);
      uint longestSize = idxQue[sortedIndex[numberOfInvertedList - 1]]->size();

      bool *longListMask = new bool[numberOfInvertedList];
      memset(longListMask, 0, sizeof(bool) * numberOfInvertedList);  

      addMAXUnsigned2EachList(idxQue, MAXUnsigned);

      uint *dataHeap;
      uint *indexHeap;
      dataHeap = new uint[numberOfInvertedList];
      indexHeap = new uint[numberOfInvertedList];
        
      uint sizeOfHeaps;
      _makeHeap(
        idxQue, 
        pointersIndex, 
        longListMask, 
        dataHeap, 
        indexHeap, 
        sizeOfHeaps, 
        weights,
        whtMinNeeded);
  
      // Container of vector indexes which should be moved to the next position
      uint *vectorIndexContainer;
      vectorIndexContainer = new uint[numberOfInvertedList];
      uint containerSize = 0;

      uint iteration = 0;
      while (dataHeap[0] < MAXUnsigned) {
     
        // Check if we can get the result 
        containerSize = 0;  
        
        uint minData =  dataHeap[0];
        while (minData == dataHeap[0] && 
               containerSize < numberOfInvertedList - longListsSize) {
          vectorIndexContainer[containerSize++] = indexHeap[0];
          heapDelete(dataHeap, indexHeap, sizeOfHeaps);
        }//end while

        if (containerSize >= threshold - longListsSize) {
          // we got the result

          uint freq = containerSize;

          if (freq < threshold)
            // lookup in the long lists
            for (uint i = 0; i < numberOfInvertedList; i++)
              if (longListMask[i] && 
                  weights[pointersIndex[i]] >= whtMinNeeded) {
                uint pos = idxQue[i]->binarySearch(minData, pointersIndex[i]);
                pointersIndex[i] = pos;
                if (pos < idxQue[i]->size() && idxQue[i]->at(pos) == minData)
                  freq++;
              }

          if (freq >= threshold && checked.find(minData) == checked.end()) {
            // good result
            uint id = minData;
            float sc = score(que.sim(que.str, data[id]), weights[id]);

            if (sc > topkBuf[0].score) {
              std::pop_heap(topkBuf, topkBuf + que.k);
              topkBuf[que.k - 1] = Cand(id, sc);
              std::push_heap(topkBuf, topkBuf + que.k);
              whtMinNeeded = scoreInverseWht(topkBuf[0].score, simMaxPossible);
            }
          }

          // recompute threshold
          if (topkBuf[0].score > std::numeric_limits<float>::min() && 
              (iteration++) % batchThreshold == 0) {
            float whtMaxFront = weights[minData];
            threshold = que.sim.getNoGramsMin(
              que.len,
              idx.noGramsMin, 
              que.noGrams, 
              scoreInverseSim(topkBuf[0].score, whtMaxFront));

            if (threshold > thresholdMax) break;
            if (whtMinNeeded > whtMaxFront) break;
        
            // longListsSizeNew = threshold - 1;
            longListsSizeNew = 
              static_cast<uint>(
                floor(
                  threshold / 
                  (Topk::Heap::nLongParam * log2(longestSize) + 1)));
        
            if (longListsSizeNew > longListsSize && 
                longListsSizeNew - longListsSize >= batchLongLists) {
              /* 
               * move pointersIndex for the popped lists
               * resort lists by length
               * set the longListMask for the longListsSizeNew longest lists
               * create a new heap with the heads of the non-long lists
               * continue
               */

              for(uint i=0; i < containerSize; i++) {
                uint j = vectorIndexContainer[i];
                pointersIndex[j]++ ;
              }//end for
          
              _sortListsBySize(idxQue, pointersIndex, sortedIndex);
              longestSize = idxQue[sortedIndex[numberOfInvertedList - 1]]->size();
          
              memset(longListMask, 0, sizeof(bool) * numberOfInvertedList);
              for (uint i = numberOfInvertedList - longListsSizeNew; 
                   i < numberOfInvertedList; i++)
                longListMask[sortedIndex[i]] = true;

              _makeHeap(
                idxQue, 
                pointersIndex, 
                longListMask, 
                dataHeap, 
                indexHeap, 
                sizeOfHeaps, 
                weights, 
                whtMinNeeded);

              longListsSize = longListsSizeNew;

              continue;
            }
          }

        // }
          
          //move to the next element
          for(uint i=0; i < containerSize; i++) {
            uint j = vectorIndexContainer[i];
            pointersIndex[j]++ ;
          }//end for


          _insertHeap(
            dataHeap, 
            indexHeap, 
            sizeOfHeaps, 
            idxQue, 
            pointersIndex, 
            vectorIndexContainer, 
            containerSize, 
            weights, 
            whtMinNeeded);

          continue;

        }//end if (freq >= threshold)
    
        // pop more elements from heap
        // and skip nodes

        while (containerSize < threshold - 1 - longListsSize) {
          vectorIndexContainer[containerSize++] = indexHeap[0];
          heapDelete(dataHeap, indexHeap, sizeOfHeaps);
        }//end while

        skipNodes(
          idxQue, 
          vectorIndexContainer, 
          containerSize, 
          dataHeap[0], 
          pointersIndex);

        _insertHeap(
          dataHeap, 
          indexHeap, 
          sizeOfHeaps, 
          idxQue, 
          pointersIndex,
          vectorIndexContainer, 
          containerSize, 
          weights, 
          whtMinNeeded);

      } //end while ( thresholdHeap[0]  < MAX)

      delete [] dataHeap;
      delete [] indexHeap;
      delete [] vectorIndexContainer;
      delete [] pointersIndex;
      delete [] sortedIndex;
      delete [] longListMask;

      deleteMAXUnsignedfromEachList(idxQue);
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
      const IndexQuery_v1 &idxQue, 
      OutputIterator topk)
    {
      Cand *topkBuf = new Cand[que.k];
      std::fill(topkBuf, topkBuf + que.k, Cand());
      _SetNoOperation checked;

      _doDivideSkip(
        data, 
        weights, 
        idx, 
        que, 
        idxQue, 
        topkBuf, 
        checked);
  
      std::sort_heap(topkBuf, topkBuf + que.k);
      for (uint i = 0; i < que.k; i++)
        *topk++ = topkBuf[i].id;
      delete [] topkBuf;
    }
  }
}

#endif

