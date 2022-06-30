/*
  $Id: gramlistsimple.h 5783 2010-10-21 23:12:04Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _gramlistsimple_h_
#define _gramlistsimple_h_

#include "gramlist.h"

template<typename InvList = vector<unsigned> >
class GramListSimple : public GramList<InvList> {
 protected:
  InvList invertedList;
  
 public:
  GramListSimple() {};
  InvList* getArray(fstream* invListsFile = NULL) { return &invertedList; }
  ~GramListSimple() {};
  void free() { delete this; }
  void clear() { }
};

#endif
