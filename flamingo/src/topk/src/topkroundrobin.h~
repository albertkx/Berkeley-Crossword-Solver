/*
  $Id: topkroundrobin.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/11/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topkroundrobin_h_
#define _topkroundrobin_h_

#include <algorithm>
#include <list>

#include "topkindex.h"

namespace Topk 
{
  namespace RoundRobin 
  {
    struct _TopkEl 
    {
      float min;
      std::tr1::unordered_map<uint, std::list<_TopkEl>::iterator>::iterator ptr;
    };

    typedef std::list<_TopkEl> _TopkList;
    typedef std::tr1::unordered_map<uint, _TopkList::iterator> _TopkMap;

    inline bool _less_equal_prev(const std::pair<uint*, uint> r, uint v) 
    {
      return r.second < v;
    }

    bool _updateTopk(
      const Query &que,
      _TopkMap &topkMap, 
      _TopkList &topkList, 
      uint id, 
      float sc);

    template<
      class RandomAccessIterator1, 
      class RandomAccessIterator2, 
      class RandomAccessIterator3, 
      class OutputIterator> 
    void getTopk(
      const RandomAccessIterator1 data, 
      const RandomAccessIterator2 weights,
      const RandomAccessIterator3 nograms, 
      const Index &idx, 
      const Query &que, 
      IndexQuery &idxQue, 
      OutputIterator topk)
    {
      std::tr1::unordered_map<uint*, uint> prev;
      std::map<uint, std::pair<uint, uint> > cands;
      std::list<std::list<range<uint*> >::iterator> skip;

      _TopkMap topkMap;
      struct _TopkEl el;
      el.min = 0;
      el.ptr = topkMap.end();
      _TopkList topkList(que.k, el);
  
      bool firstRound = true;

      // while there are lists with elements inside
      while (!idxQue.lists.empty()) {

        uint 
          idMin = ~0u, 
          idMax = 0;

        // === get next element from each list === 
        for (std::list<range<uint*> >::iterator ls = idxQue.lists.begin(); 
             ls != idxQue.lists.end();) {

          uint id = *ls->_begin;

          if (id < idMin) idMin = id;
          if (id > idMax) idMax = id;

          // === process current id ===
          std::map<uint, std::pair<uint, uint> >::iterator 
            pos = cands.lower_bound(id);
          std::pair<uint, uint> cand;

          bool isNew = pos == cands.end() || pos->first != id;

          if (isNew) {
            cand.first = 1;
            cand.second = std::count_if(
              prev.begin(), 
              prev.end(), 
              std::bind2nd(std::ptr_fun(_less_equal_prev), id)) + 
              (firstRound ? std::distance(ls, idxQue.lists.end()) : 0);
          }
          else {
            ++pos->second.first;
            cand = pos->second;
          }

          bool doInsert, doDiscard;
          doInsert = doDiscard = false;
      
          float scMin, scMax, sck;
          scMin = score(
            que.sim.getSimMin(que.noGrams, nograms[id], cand.first), 
            weights[id]);       // score min
          scMax = score(
            que.sim.getSimMax(que.len, que.noGrams, nograms[id], cand.second),
            weights[id]);             // score max
          sck = topkList.front().min; // score min kth in topk

          if (scMax > sck || (scMax == sck && scMin == sck)) {
            if (cand.first == cand.second) {
              doDiscard = !isNew;
              scMin = score(que.sim(que.str, data[id]), weights[id]); 
                                // score real
            }
            else
              doInsert = isNew;
            if (scMin >= sck)
              _updateTopk(que, topkMap, topkList, id, scMin);
          }
          else {
            doDiscard = !isNew;
            if (ls->_begin + 1 != ls->_end) 
              skip.push_back(ls);
          }

          if (doInsert)
            cands.insert(pos, make_pair(id, cand));
          if (doDiscard)
            cands.erase(pos);

          // === process affected candidates ===      
          for (std::map<uint, std::pair<uint, uint> >::iterator 
                 c = firstRound ? 
                 cands.begin() : cands.upper_bound(prev[ls->_end]);
               c != (ls->_begin + 1 == ls->_end ? 
                     cands.end() : cands.lower_bound(id));) {
            uint idc = c->first;

            if (id == idc) {
              ++c;
              continue;
            }

            --c->second.second;

            cand = c->second;
            doDiscard = false;

            scMin = score(
              que.sim.getSimMin(que.noGrams, nograms[id], cand.first), 
              weights[idc]);    // score min
            scMax = score(
              que.sim.getSimMax(
                que.len, que.noGrams, nograms[id], cand.second), 
              weights[idc]);            // score max
            sck = topkList.front().min; // score min kth in topk

            if (scMax > sck || (scMax == sck && scMin == sck)) {
              if (cand.first == cand.second) {
                doDiscard = true;
                scMin = score(
                  que.sim(que.str, data[idc]), weights[idc]); // score real
                if (scMin >= sck)
                  _updateTopk(que, topkMap, topkList, idc, scMin);
              } 
            }
            else {
              doDiscard = true;
              if (idMin <= idc && idc <= idMax)
                for (std::list<range<uint*> >::iterator lsc = idxQue.lists.begin();
                     lsc != ls; ++lsc)
                  if (prev[lsc->_end] == idc && lsc->_begin + 1 != lsc->_end)
                    skip.push_back(lsc);
            }

            if (doDiscard)
              cands.erase(c++);
            else
              ++c;
          }

          // go to next element on the list
          ++ls->_begin;
          
          if (ls->_begin == ls->_end) { // remove list if depleted
            prev.erase(ls->_end);
            ls = idxQue.lists.erase(ls);
          }
          else {                // set previous and move to next list

            prev[ls->_end] = id;
            ++ls;
          }
        }
    
        // === check viability of unupdated candidates === 
        for (std::map<uint, std::pair<uint, uint> >::iterator 
               c = cands.upper_bound(idMax); c != cands.end();) {

          uint id = c->first;
          std::pair<uint, uint> cand = c->second;

          float scMin, scMax, sck;
          scMin = score(
            que.sim.getSimMin(que.noGrams, nograms[id], cand.first), 
            weights[id]);       // score min
          scMax = score(
            que.sim.getSimMax(que.len, que.noGrams, nograms[id], cand.second), 
            weights[id]);             // score max
          sck = topkList.front().min; // score min kth in topk

          if (scMax < sck || (scMax == sck && scMin < sck))
            cands.erase(c++);
          else
            ++c;
        }
  
        // === skip  === 
        for (std::list<std::list<range<uint*> >::iterator>::iterator 
               s = skip.begin(); s != skip.end(); ++s) {

          std::list<range<uint *> >::iterator ls = *s;
          uint idCandMin = cands.begin()->first;

          if (ls->_begin == ls->_end || *ls->_begin >= idCandMin)
            continue;
            
          ls->_begin = std::lower_bound(ls->_begin, ls->_end, idCandMin);

          if (ls->_begin == ls->_end) { // skiped to the end

            std::tr1::unordered_map<uint*, uint>::iterator 
              pos = prev.find(ls->_end);

            for (std::map<uint, std::pair<uint, uint> >::iterator
                   c = cands.upper_bound(pos->second); c != cands.end();) {

              --c->second.second;

              uint id = c->first;
              std::pair<uint, uint> cand = c->second;
              bool doDiscard = false;

              float scMin, scMax, sck;
              scMin = score(
                que.sim.getSimMin(que.noGrams, nograms[id], cand.first), 
                weights[id]);   // score min
              scMax = score(
                que.sim.getSimMax(que.len, que.noGrams, nograms[id], cand.second), 
                weights[id]);             // score max
              sck = topkList.front().min; // score min kth in topk

              if (scMax > sck || (scMax == sck && scMin == sck)) {
                if (cand.first == cand.second) {
                  doDiscard = true;
                  scMin = score(
                    que.sim(que.str, data[id]), weights[id]); // score real
                  if (scMin >= sck)
                    _updateTopk(que, topkMap, topkList, id, scMin);
                } 
              }
              else
                doDiscard = true;
                  
              if (doDiscard)
                cands.erase(c++);
              else
                ++c;
            }
                
            prev.erase(pos);
            idxQue.lists.erase(ls);
          }
        }
        skip.clear();
    
        // === stop === 
        if (cands.empty() && 
            topkList.front().min > 
            score(
              que.sim.getSimMax(
                que.len, que.noGrams, idxQue.lists.size(), idxQue.lists.size()), 
              weights[idMin]))
          break;

        if (firstRound)
          firstRound = false;
      }

      for (_TopkList::const_reverse_iterator i = topkList.rbegin(); 
           i != topkList.rend(); ++i)
        *topk++ = i->ptr->first;
    }
  }
}

#endif
