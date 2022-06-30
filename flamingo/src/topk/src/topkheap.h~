/*
  $Id: topkheap.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 02/20/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkheap_h_
#define _topkheap_h_

#include <algorithm>
#include <cmath>
#include <map>

#include "common/src/tr1_local.h"
#include "util/src/querygroup.h"
#include "util/src/querygrams.h"
#include "util/src/misc.h"
#include "topkindex.h"

#ifndef _WIN32 // no timer needed for win32 build
#include "util/src/time.h"
#endif

#include "util/src/debug.h"
#include <iostream>

namespace Topk
{
  namespace Heap
  {
    // SEARCH_API is necessary for search.dll (Visual C++)
    SEARCH_API extern float nLongParam;

    struct _SetNoOperation
    {
      void* find(uint) { return 0; }
      void* end() { return 0; }
      bool empty() { return true; }
    };

    inline bool _greaterLen(range<uint*> &a, range<uint*> &b)
    {
      return (a._end - a._begin) > (b._end - b._begin);
    }

    inline bool _greaterId(
      std::list<range<uint*> >::iterator &a,
      std::list<range<uint*> >::iterator &b)
    {
      return *a->_begin > *b->_begin;
    }

    template<
      class RandomAccessIterator1,
      class RandomAccessIterator2,
      class Set>
    void _doDivideSkip(
      const RandomAccessIterator1 data,
      const RandomAccessIterator2 weights,
      const Index &idx,
      const Query &que,
      IndexQuery &idxQue,
      Cand *topkBuf,
      Set &checked,
      float simMax = 1,
      float whtMin = std::numeric_limits<float>::min(),
      uint thrMin = 1,
      uint thrChecked = std::numeric_limits<uint>::max())
    {
      const uint batchIter = 10;

      uint nLists = idxQue.lists.size();

      // order lists by length
      idxQue.lists.sort(_greaterLen);
      
      // compute nLong
      std::list<range<uint*> >::iterator listLongest = idxQue.lists.begin();
      uint lenLongest = listLongest->_end - listLongest->_begin;
      uint nLong =
        static_cast<uint>(floor(thrMin / (nLongParam * log2(lenLongest) + 1)));

      std::list<range<uint*> >::iterator listsShortBeg = idxQue.lists.begin();
      for (uint i = 0; i < nLong; i++)
        ++listsShortBeg;
      
      // build heap
      std::list<range<uint*> >::iterator *heap = 
        new std::list<range<uint*> >::iterator[nLists];
      uint heapSz = 0;
      for (std::list<range<uint*> >::iterator ls = listsShortBeg;
           ls != idxQue.lists.end(); ++ls) {
        heap[heapSz++] = ls;
      }
      std::make_heap(heap, heap + heapSz, _greaterId); // min-heap

      std::list<std::list<range<uint*> >::iterator> popped;

      uint iter = 0;
      while (heapSz > 0) {

#ifdef DEBUG
#ifndef _WIN32
        ++thrFreq[thrMin];
#endif
#endif

        // pop head and all equal ids
        uint id = *(**heap)._begin;
        uint freq = 0;

        while (heapSz > 0 && id == *(**heap)._begin) {
          ++freq;
          popped.push_back(*heap);
          std::pop_heap(heap, heap + (heapSz--), _greaterId);
        }

        if (freq >= thrMin - nLong) {
          if (checked.empty() || checked.find(id) == checked.end()) {
            if (freq < thrMin)
              // lookup in long lists
              for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                   ls != listsShortBeg;) {
                if (weights[*ls->_begin] >= whtMin) {
                  ls->_begin = std::lower_bound(ls->_begin, ls->_end, id);
                  if (ls->_begin != ls->_end && *ls->_begin == id) {
                    ++freq;
                    ++ls->_begin;
                  }
                  if (ls->_begin == ls->_end) {
                    ls = idxQue.lists.erase(ls);
                    --nLong;
                    --nLists;
                  }
                  else
                    ++ls;
                }
                else {
                  ls = idxQue.lists.erase(ls);
                  --nLong;
                  --nLists;
                }
              }

            if (freq >= thrMin) {
              // good result

              // TIME_START;

              float sc = score(
                que.sim(que.str, data[id]), 
                // dynamic_cast<const SimMetricJacc*>(&que.sim)->operator()(
                //   que.sim.gramGen.getNumGrams(data[id]), que.noGrams, freq),
                weights[id]);

              // TIME_END;
              // simTime += _tms;
              STAT_INC(simComp);
#ifdef DEBUG
#ifndef _WIN32
              ++thrFreqSim[thrMin];
#endif
#endif

              if (sc > topkBuf[0].score) {
                std::pop_heap(topkBuf, topkBuf + que.k);
                topkBuf[que.k - 1] = Cand(id, sc);
                std::push_heap(topkBuf, topkBuf + que.k);
                whtMin = scoreInverseWht(topkBuf[0].score, simMax);
              }
            }
          }
          // advance on the popped lists
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            ++ls->_begin;
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }
        else {
          // pop more elements from heap and skip nodes
          while (heapSz > 1 && freq < thrMin - nLong - 1) {
            ++freq;
            popped.push_back(*heap);
            std::pop_heap(heap, heap + (heapSz--), _greaterId); // pop
          }

          uint idHead;
          if (heapSz > 0) {
            idHead = *(**heap)._begin;
          }
          else {
            idHead = id + 1;
          }
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            /*
             * cannot "++ls->_begin" because "ls->_begin" might be equal to
             * the top of the heap. that is, in the prevous skip we
             * popped an element equal to the current top of the heap.
             * 
             * e.g.:  1  10 10   thr = 3
             *        10 .. ..
             *
             * after 1 is popped we skip one more element, we skip 10
             * on list 2. so, we should not mover over it here.
             */
            ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead); // skip
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }
        
        if (topkBuf[0].score > std::numeric_limits<float>::min() && 
            (++iter) % batchIter == 0) {
          // recompute maximum possible weight and minimum
          // necessary threshold
          float whtMax = weights[id];
          thrMin = que.sim.getNoGramsMin(
            que.len,
            idx.noGramsMin, 
            que.noGrams, 
            scoreInverseSim(topkBuf[0].score, whtMax) +  
            std::numeric_limits<float>::epsilon());
          uint thrMax = std::min(thrChecked - 1, nLists);

          if (thrMin > thrMax || whtMin > whtMax) {
            OUTPUT(">>>", "breaking");
            OUTPUT("thrMin > thrMax", thrMin > thrMax);
            OUTPUT("whtMin > whtMax", whtMin > whtMax);
            OUTPUT("thrMin", thrMin);
            OUTPUT("thrMax", thrMax);
            OUTPUT("whtMin", whtMin);
            OUTPUT("whtMax", whtMax);
            OUTPUT("<<<", "breaking");
          }

          if (thrMin > thrMax) break;
          if (whtMin > whtMax) break;
                  
          // lenLongest might be outdated, but OK
          uint nLongNew =
            static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));

          uint idHead = *(**heap)._begin;
          if (nLongNew != nLong) { // recompute nLong
            // advance to current top of the heap on the long lists
            for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                 ls != listsShortBeg;) {
              ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead);
              if (ls->_begin == ls->_end) {
                ls = idxQue.lists.erase(ls);
                --nLong;
                --nLists;
              }
              else
                ++ls;
            }

            // reorder lists by length
            idxQue.lists.sort(_greaterLen);
                    
            // recompute nLong
            listLongest = idxQue.lists.begin();
            lenLongest = listLongest->_end - listLongest->_begin;
            nLong = static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
                    
            listsShortBeg = idxQue.lists.begin();
            for (uint i = 0; i < nLong; i++)
              ++listsShortBeg;
                
            // rebuild heap
            heapSz = 0;
            for (std::list<range<uint*> >::iterator ls = listsShortBeg;
                 ls != idxQue.lists.end(); ++ls)
              heap[heapSz++] = ls;
            std::make_heap(heap, heap + heapSz, _greaterId); // min-heap
          }
        }    
      }

      OUTPUT("heapSz", heapSz);

      delete [] heap;
    }

    template<
      class RandomAccessIterator1,
      class RandomAccessIterator2,
      class Set>
    void _doDivideSkip(
      const RandomAccessIterator1 data,
      const RandomAccessIterator2 weights,
      const Index &idx,
      const QueryGrams &que,
      IndexQuery &idxQue,
      Cand *topkBuf,
      Set &checked,
      float simMax = 1,
      float whtMin = std::numeric_limits<float>::min(),
      uint thrMin = 1,
      uint thrChecked = std::numeric_limits<uint>::max())
    {
      const uint batchIter = 10;

      uint nLists = idxQue.lists.size();

      // order lists by length
      idxQue.lists.sort(_greaterLen);
      
      // compute nLong
      std::list<range<uint*> >::iterator listLongest = idxQue.lists.begin();
      uint lenLongest = listLongest->_end - listLongest->_begin;
      uint nLong =
        static_cast<uint>(floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
      
      std::list<range<uint*> >::iterator listsShortBeg = idxQue.lists.begin();
      for (uint i = 0; i < nLong; i++)
        ++listsShortBeg;
      
      // build heap
      std::list<range<uint*> >::iterator *heap = 
        new std::list<range<uint*> >::iterator[nLists];
      uint heapSz = 0;
      for (std::list<range<uint*> >::iterator ls = listsShortBeg;
           ls != idxQue.lists.end(); ++ls)
        heap[heapSz++] = ls;
      std::make_heap(heap, heap + heapSz, _greaterId); // min-heap

      std::list<std::list<range<uint*> >::iterator> popped;

      uint iter = 0;
      while (heapSz > 0) {

#ifdef DEBUG
        ++thrFreq[thrMin];
#endif

        // pop head and all equal ids
        uint id = *(**heap)._begin;
        uint freq = 0;

        while (heapSz > 0 && id == *(**heap)._begin) {
          ++freq;
          popped.push_back(*heap);
          std::pop_heap(heap, heap + (heapSz--), _greaterId);
        }

        if (freq >= thrMin - nLong) {
          if (checked.empty() || checked.find(id) == checked.end()) {
            // lookup in long lists
            for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                 ls != listsShortBeg;) {
              if (weights[*ls->_begin] >= whtMin) {
                ls->_begin = std::lower_bound(ls->_begin, ls->_end, id);
                if (ls->_begin != ls->_end && *ls->_begin == id) {
                  ++freq;
                  ++ls->_begin;
                }
                if (ls->_begin == ls->_end) {
                  ls = idxQue.lists.erase(ls);
                  --nLong;
                  --nLists;
                }
                else
                  ++ls;
              }
              else {
                ls = idxQue.lists.erase(ls);
                --nLong;
                --nLists;
              }
            }

            if (freq >= thrMin) {
              // good result

              // TIME_START;

              float sc = score(
                que.sim(que.str, data[id]), 
                // dynamic_cast<const SimMetricJacc*>(&que.sim)->operator()(
                //   data[id].size(), que.noGrams, freq),
                weights[id]);

              // TIME_END;
              // simTime += _tms;
              STAT_INC(simComp);
#ifdef DEBUG
              ++thrFreqSim[thrMin];
#endif

              if (sc > topkBuf[0].score) {
                std::pop_heap(topkBuf, topkBuf + que.k);
                topkBuf[que.k - 1] = Cand(id, sc);
                std::push_heap(topkBuf, topkBuf + que.k);
                whtMin = scoreInverseWht(topkBuf[0].score, simMax);
              }
            }
          }
          // advance on the popped lists
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            ++ls->_begin;
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }
        else {
          // pop more elements from heap and skip nodes
          while (heapSz > 1 && freq < thrMin - nLong - 1) {
            ++freq;
            popped.push_back(*heap);
            std::pop_heap(heap, heap + (heapSz--), _greaterId); // pop
          }

          uint idHead = *(**heap)._begin;
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            /*
             * cannot "++ls->_begin" because "ls->_begin" might be equal to
             * the top of the heap. that is, in the prevous skip we
             * popped an element equal to the current top of the heap.
             * 
             * e.g.:  1  10 10   thr = 3
             *        10 .. ..
             *
             * after 1 is popped we skip one more element, we skip 10
             * on list 2. so, we should not mover over it here.
             */
            ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead); // skip
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }
        
        if (topkBuf[0].score > std::numeric_limits<float>::min() && 
            (++iter) % batchIter == 0) {
          // recompute maximum possible weight and minimum
          // necessary threshold
          float whtMax = weights[id];
          thrMin = que.sim.getNoGramsMin(
            que.len,
            idx.noGramsMin, 
            que.noGrams, 
            scoreInverseSim(topkBuf[0].score, whtMax) +  
            std::numeric_limits<float>::epsilon());
          uint thrMax = std::min(thrChecked - 1, nLists);

#ifdef DEBUG
          if (thrMin > thrMax || whtMin > whtMax) {
            OUTPUT(">>>", "breaking");
            OUTPUT("thrMin > thrMax", thrMin > thrMax);
            OUTPUT("whtMin > whtMax", whtMin > whtMax);
            OUTPUT("thrMin", thrMin);
            OUTPUT("thrMax", thrMax);
            OUTPUT("whtMin", whtMin);
            OUTPUT("whtMax", whtMax);
            OUTPUT("nLists", nLists);
            OUTPUT("que.len", que.len);
            OUTPUT("idx.noGramsMin", idx.noGramsMin);
            OUTPUT("que.noGrams", que.noGrams);
            OUTPUT("scoreInverseSim", 
                   scoreInverseSim(topkBuf[0].score, whtMax));
            OUTPUT("<<<", "breaking");
          }
#endif

          if (thrMin > thrMax)
            break;

          if (whtMin > whtMax)
            break;
                  
          // lenLongest might be outdated, but OK
          uint nLongNew =
            static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
                  
          uint idHead = *(**heap)._begin;
          if (nLongNew != nLong) { // recompute nLong
            // advance to current top of the heap on the long lists
            for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                 ls != listsShortBeg;) {
              ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead);
              if (ls->_begin == ls->_end) {
                ls = idxQue.lists.erase(ls);
                --nLong;
                --nLists;
              }
              else
                ++ls;
            }

            // reorder lists by length
            idxQue.lists.sort(_greaterLen);
                    
            // recompute nLong
            listLongest = idxQue.lists.begin();
            lenLongest = listLongest->_end - listLongest->_begin;
            nLong = static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
                    
            listsShortBeg = idxQue.lists.begin();
            for (uint i = 0; i < nLong; i++)
              ++listsShortBeg;
                
            // rebuild heap
            heapSz = 0;
            for (std::list<range<uint*> >::iterator ls = listsShortBeg;
                 ls != idxQue.lists.end(); ++ls)
              heap[heapSz++] = ls;
            std::make_heap(heap, heap + heapSz, _greaterId); // min-heap
          }
        }    
      }

      OUTPUT("heapSz", heapSz);

      delete [] heap;
    }

    template<
      class RandomAccessIterator1,
      class RandomAccessIterator2>
    void _doDivideSkipMulti(
      const RandomAccessIterator1 data,
      const RandomAccessIterator2 weights,
      const Index &idx,
      const QueryGroup &queGrp,
      IndexQuery &idxQue,
      Cand **topkBufs,
      float simMax = 1,
      float whtMin = std::numeric_limits<float>::min(),
      uint thrMin = 1,
      uint thrChecked = std::numeric_limits<uint>::max())
    {
      const uint batchIter = 10; // * queGrp.n;

      uint nLists = idxQue.lists.size();

      uint *thrMins = new uint[queGrp.n], 
        *thrMaxs = new uint[queGrp.n], 
        thrMinMax = thrMin;
      float *whtMins = new float[queGrp.n];

      std::fill(thrMins, thrMins + queGrp.n, thrMin);
      std::fill(thrMaxs, thrMaxs + queGrp.n, nLists);
      std::fill(whtMins, whtMins + queGrp.n, whtMin);

      std::tr1::unordered_set<uint> notKCand;
      for (uint i = 0; i < queGrp.n; ++i)
        notKCand.insert(i);

      // order lists by length
      idxQue.lists.sort(_greaterLen);
      
      // compute nLong
      std::list<range<uint*> >::iterator listLongest = idxQue.lists.begin();
      uint lenLongest = listLongest->_end - listLongest->_begin;
      uint nLong =
        static_cast<uint>(floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
      
      std::list<range<uint*> >::iterator listsShortBeg = idxQue.lists.begin();
      for (uint i = 0; i < nLong; i++)
        ++listsShortBeg;
      
      // build heap
      std::list<range<uint*> >::iterator
        *heap = new std::list<range<uint*> >::iterator[nLists];
      uint heapSz = 0;
      for (std::list<range<uint*> >::iterator ls = listsShortBeg;
           ls != idxQue.lists.end(); ++ls)
        heap[heapSz++] = ls;
      std::make_heap(heap, heap + heapSz, _greaterId); // min-heap

      std::list<std::list<range<uint*> >::iterator> popped;

      uint iter = 0;
      while (heapSz > 0) {

#ifdef DEBUG
#ifndef _WIN32
        ++thrFreq[thrMin];
        for (uint i = 0; i < queGrp.n; ++i)
          ++thrsFreq[i][thrMins[i]];
#endif
#endif

        // pop head and all equal ids
        uint id = *(**heap)._begin;
        uint freq = 0;

        while (heapSz > 0 && id == *(**heap)._begin) {
          ++freq;
          popped.push_back(*heap);
          std::pop_heap(heap, heap + (heapSz--), _greaterId);
        }

        if (freq >= thrMin - nLong) {
          if (freq < thrMinMax)
            // lookup in long lists
            for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                 ls != listsShortBeg;) {
              if (weights[*ls->_begin] >= whtMin) {
                ls->_begin = std::lower_bound(ls->_begin, ls->_end, id);
                if (ls->_begin != ls->_end && *ls->_begin == id) {
                  ++freq;
                  ++ls->_begin;
                }
                if (ls->_begin == ls->_end) {
                  ls = idxQue.lists.erase(ls);
                  --nLong;
                  --nLists;
                }
                else
                  ++ls;
              }
              else {
                ls = idxQue.lists.erase(ls);
                --nLong;
                --nLists;
              }
            }

          if (freq >= thrMin) {
            // good result

            for (uint i = 0; i < queGrp.n; ++i) {
              if (freq >= thrMins[i] && freq <= thrMaxs[i]) {

                // TIME_START;

                float sim = queGrp.sim(queGrp.strs[i], data[id]);

                // TIME_END;
                // simTime += _tms;
                STAT_INC(simComp);
#ifdef DEBUG
                ++thrsFreqSim[i][thrMins[i]];
#endif

                if (sim > 0 
                    // added to pass smoke tests, should be removed for perf
                    // SimMetricGramCount(queGrp.sim.gramGen)(
                    //   queGrp.strs[i], data[id]) > 0
                    ) {
                  float sc = score(sim, weights[id]);
                  if (sc > topkBufs[i][0].score) {
                    std::pop_heap(topkBufs[i], topkBufs[i] + queGrp.k);
                    topkBufs[i][queGrp.k - 1] = Cand(id, sc);
                    std::push_heap(
                      topkBufs[i], topkBufs[i] + queGrp.k);
                    if (topkBufs[i][0].score > 
                        std::numeric_limits<float>::min())
                      notKCand.erase(i);
                    whtMins[i] = scoreInverseWht(topkBufs[i][0].score, simMax);
                  }
                }
              }
              if (i == 0 || whtMins[i] < whtMin)
                whtMin = whtMins[i]; // set whtMin to the minium of the whtMins
            }
          }
          // advance on the popped lists
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            ++ls->_begin;
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }
        else {
          // pop more elements from heap and skip nodes
          while (heapSz > 1 && freq < thrMin - nLong - 1) {
            ++freq;
            popped.push_back(*heap);
            std::pop_heap(heap, heap + (heapSz--), _greaterId); // pop
          }

          uint idHead = *(**heap)._begin;
          for (std::list<std::list<range<uint*> >::iterator>::const_iterator
                 lsp = popped.begin(); lsp != popped.end(); ++lsp) {
            std::list<range<uint*> >::iterator ls = *lsp;
            /*
             * cannot "++ls->_begin" because "ls->_begin" might be equal to
             * the top of the heap. that is, in the prevous skip we
             * popped an element equal to the current top of the heap.
             * 
             * e.g.:  1  10 10   thr = 3
             *        10 .. ..
             *
             * after 1 is popped we skip one more element, we skip 10
             * on list 2. so, we should not mover over it here.
             */
            ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead); // skip
            if (ls->_begin == ls->_end) {
              if (ls == listsShortBeg)
                listsShortBeg = idxQue.lists.erase(ls);
              else
                idxQue.lists.erase(ls);
              --nLists;
            } 
            else {
              heap[heapSz] = ls;
              std::push_heap(heap, heap + (++heapSz), _greaterId);
            }
          }
          popped.clear();
        }

        if (notKCand.empty() && 
            (++iter) % batchIter == 0) {
          // recompute maximum possible weight and minimum
          // necessary threshold
          float whtMax = weights[id];
          
          // thrMin & thrMax
          for (uint i = 0; i < queGrp.n; ++i) {
            thrMins[i] = queGrp.sim.getNoGramsMin(
              queGrp.lens[i],
              idx.noGramsMin, 
              queGrp.nosGrams[i], 
              scoreInverseSim(topkBufs[i][0].score, whtMax) +  
              std::numeric_limits<float>::epsilon());
            thrMaxs[i] = queGrp.sim.getNoGramsMax(
              queGrp.lens[i],
              idx.noGramsMin, 
              queGrp.nosGrams[i], 
              scoreInverseSim(topkBufs[i][0].score, whtMax) +  
              std::numeric_limits<float>::epsilon());
            if (i == 0 || thrMins[i] < thrMin)
              thrMin = thrMins[i]; // set thrMin to the min of the thrMins
            if (i == 0 || thrMins[i] > thrMinMax)
              thrMinMax = thrMins[i]; // set thrMinMax to the max of the thrMins
          }

          uint thrMax = std::min(thrChecked - 1, nLists);

          if (thrMin > thrMax || whtMin > whtMax) {
            OUTPUT(">>>", "breaking");
            OUTPUT("thrMin > thrMax", thrMin > thrMax);
            OUTPUT("whtMin > whtMax", whtMin > whtMax);
            OUTPUT("thrMin", thrMin);
            OUTPUT("thrMax", thrMax);
            OUTPUT("whtMin", whtMin);
            OUTPUT("whtMax", whtMax);
            OUTPUT("<<<", "breaking");
          }

          if (thrMin > thrMax) break;
          if (whtMin > whtMax) break;
                  
          // lenLongest might be outdated, but OK
          uint nLongNew =
            static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
                  
          uint idHead = *(**heap)._begin;
          if (nLongNew != nLong) { // recompute nLong
            // advance to current top of the heap on the long lists
            for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin();
                 ls != listsShortBeg;) {
              ls->_begin = std::lower_bound(ls->_begin, ls->_end, idHead);
              if (ls->_begin == ls->_end) {
                ls = idxQue.lists.erase(ls);
                --nLong;
                --nLists;
              }
              else
                ++ls;
            }

            // reorder lists by length
            idxQue.lists.sort(_greaterLen);
                    
            // recompute nLong
            listLongest = idxQue.lists.begin();
            lenLongest = listLongest->_end - listLongest->_begin;
            nLong = static_cast<uint>(
              floor(thrMin / (nLongParam * log2(lenLongest) + 1)));
                    
            listsShortBeg = idxQue.lists.begin();
            for (uint i = 0; i < nLong; i++)
              ++listsShortBeg;
                
            // rebuild heap
            heapSz = 0;
            for (std::list<range<uint*> >::iterator ls = listsShortBeg;
                 ls != idxQue.lists.end(); ++ls)
              heap[heapSz++] = ls;
            std::make_heap(heap, heap + heapSz, _greaterId); // min-heap
          }
        }    
      }

      delete [] thrMins;
      delete [] thrMaxs;
      delete [] whtMins;
      delete [] heap;

      OUTPUT("heapSz", heapSz);
    }

    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2>  
    void getTopkCand(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index &idx, 
      const Query &que, 
      IndexQuery &idxQue, 
      Cand *topkBuf)
    {
      std::fill(topkBuf, topkBuf + que.k, Cand());
      _SetNoOperation checked;

      float thrMin = 0;
      if (que.threshold > 0) {
        thrMin = que.sim.getNoGramsMin(
          que.len, idx.noGramsMin, que.noGrams, que.threshold);  
      }
      OUTPUT("getTopkCand::thrQue", que.threshold);
      OUTPUT("getTopkCand::thrMin", thrMin);

      _doDivideSkip(
        data,
        weights,
        idx,
        que, 
        idxQue, 
        topkBuf, 
        checked, 
        1,
        std::numeric_limits<float>::min(),
        thrMin);

      std::sort_heap(topkBuf, topkBuf + que.k);
    }
    
    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2>  
    void getTopkCand(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index &idx, 
      const QueryGrams &que, 
      IndexQuery &idxQue, 
      Cand *topkBuf)
    {
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
    }
    
    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class OutputIterator>  
    void getTopk(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index &idx, 
      const Query &que, 
      IndexQuery &idxQue, 
      OutputIterator topk)
    {
      Cand *topkBuf = new Cand[que.k];
      getTopkCand(
        data, 
        weights, 
        idx, 
        que, 
        idxQue, 
        topkBuf);      

      for (uint i = 0; i < que.k; i++)
        *topk++ = topkBuf[i].id;

      delete [] topkBuf;
    }

    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class OutputIterator>  
    void getTopk(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index &idx, 
      const QueryGrams &que, 
      IndexQuery &idxQue, 
      OutputIterator topk)
    {
      Cand *topkBuf = new Cand[que.k];
      getTopkCand(
        data, 
        weights, 
        idx, 
        que, 
        idxQue, 
        topkBuf);      

      for (uint i = 0; i < que.k; i++)
        *topk++ = topkBuf[i].id;

      delete [] topkBuf;
    }

    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2,
      class OutputContainer>
    void getTopks(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights, 
      const Index &idx, 
      const QueryGroup &queGrp,
      IndexQuery &idxQue, 
      OutputContainer &topks)    // vector<vector<uint> >, uint** 
    {
      Cand **topkBufs = new Cand*[queGrp.n];
      for (uint i = 0; i < queGrp.n; ++i) {
        topkBufs[i] = new Cand[queGrp.k];
        std::fill(topkBufs[i], topkBufs[i] + queGrp.k, Cand());
      }
      
      _doDivideSkipMulti(
        data,
        weights,
        idx,
        queGrp, 
        idxQue, 
        topkBufs);

      for (uint i = 0; i < queGrp.n; ++i) {
        std::sort_heap(topkBufs[i], topkBufs[i] + queGrp.k);
        for (uint j = 0; j < queGrp.k; ++j)
          topks[i][j] = topkBufs[i][j].id;
        delete [] topkBufs[i];
      }
      delete [] topkBufs;
    }
  }
}

#endif

