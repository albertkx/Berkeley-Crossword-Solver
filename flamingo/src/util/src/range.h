/*
  $Id: range.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license.

  Date: 07/19/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _range_h_
#define _range_h_

#include <iterator>

template<class T>
struct range
{
  T _begin, _end;

  range(const T& begin = T(), const T& end = T()): 
    _begin(begin), 
    _end(end)
  {}
  
  template<class U>
  range(const range<U>& p):
    _begin(p._begin), _end(p._end)
  {}

  size_t size()
    const
  { return std::distance(_begin, _end); }

  const T& begin() const 
  { return _begin; }

  const T& end() const 
  { return _end; }

  typedef T iterator;
  typedef T const_iterator;
};

template<class T>
inline bool operator==(const range<T>& x, const range<T>& y) 
{ return x._begin == y._begin && x._end == y._end; }

template<class T> 
inline bool operator!=(const range<T>& x, const range<T>& y)
{ return !(x == y); }

template<class T>
inline range<T> make_range(T x, T y) { return range<T>(x, y); }

#endif
