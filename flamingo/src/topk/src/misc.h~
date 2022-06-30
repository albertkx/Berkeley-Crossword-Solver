/*
  $Id: misc.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/23/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ext/numeric>

#include "common/src/typedef.h"

template<
  class RandomAccessIterator, 
  class OutputIterator>
void selectRandom(
  RandomAccessIterator in, 
  RandomAccessIterator in_end,
  OutputIterator out, 
  uint n) 
{
  uint in_size = in_end - in;

  if (n > in_size) {
    std::cerr << "selectRandom::selection size cannot be greater than input size" 
              << std::endl;
    exit(EXIT_FAILURE);
  }

  uint *idx = new uint[in_size];
  __gnu_cxx::iota(idx, idx + in_size, 0);
  std::random_shuffle(idx, idx + in_size);

  for (uint i = 0; i < n; i++, ++out)
    out = in[idx[i]];

  delete idx;
}
