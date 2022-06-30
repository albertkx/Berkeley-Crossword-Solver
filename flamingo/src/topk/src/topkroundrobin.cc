/*
  $Id: topkroundrobin.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/09/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "topkroundrobin.h"

using namespace std;

namespace Topk 
{
  namespace RoundRobin 
  {
    void _updateTopkList(_TopkList &topkList, _TopkList::iterator pos);

    void _updateTopkList(_TopkList &topkList, _TopkList::iterator pos) 
    {
      _TopkList::iterator posNext = pos;
      ++posNext;

      if (posNext != topkList.end()) {
        while (posNext != topkList.end() && posNext->min < pos->min)
          ++posNext;
        topkList.splice(posNext, topkList, pos);
      }
    }

    bool _updateTopk(
      const Query &que,
      _TopkMap &topkMap, 
      _TopkList &topkList, 
      uint id, 
      float sc) 
    {
      _TopkMap::const_iterator pos = topkMap.find(id);
      if (pos != topkMap.end()) { // update and resort
        _TopkList::iterator ps = pos->second;
        ps->min = sc;
          
        _updateTopkList(topkList, ps);
      }
      else if (sc > topkList.front().min) { // delete, insert, and resort
        _TopkMap::iterator pos = topkList.front().ptr;
        if (pos != topkMap.end())
          topkMap.erase(topkList.front().ptr);
        topkList.pop_front();
          
        pair<_TopkMap::iterator, bool>
          st = topkMap.insert(make_pair(id, topkList.begin()));
        struct _TopkEl t;
        t.min = sc;
        t.ptr = st.first;
        topkList.push_front(t);
        st.first->second = topkList.begin();
    
        _updateTopkList(topkList, topkList.begin());
      }
      else
        return false;
      return true;
    }
  }
}
