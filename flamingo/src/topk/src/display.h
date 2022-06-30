/*
  $Id: display.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 08/19/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _display_h_
#define _display_h_

#include "common/src/query.h"
#include "util/src/range.h"

#include <list>

struct DisplayListsInfo 
{
  range<uint*> r;
  uint *i, *p, *c;
  bool e, u;

  DisplayListsInfo(range<uint*> r):
    r(r), i(r._begin), p(r._begin), c(r._begin), u(false)
    {}
};

struct DisplayLists
{
  std::vector<std::string> gs;
  std::list<DisplayListsInfo> lists;
  uint g, gn, longest;

  void init(
    const std::list<range<uint *> > &initialLists, 
    const Query &query);

  void show(const std::list<range<uint *> > &crt);
};

#endif
