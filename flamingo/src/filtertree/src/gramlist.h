/*
  $Id: gramlist.h 5783 2010-10-21 23:12:04Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
          Shengyue Ji <shengyuj (at) ics.uci.edu>
*/

#ifndef _gramlist_h_
#define _gramlist_h_

#include <fstream>
#include <vector>

using namespace std;

template <typename InvList = vector<unsigned> >
class GramList {
 public:  
  virtual InvList* getArray(fstream* invListsFile = NULL) = 0;
  virtual ~GramList() {};
  virtual void free() = 0;
  virtual void clear() = 0;
};

template<class InvList = vector<unsigned> >
class QueryGramList {
public:
  unsigned gramCode;
  GramList<InvList>* gl;  

  QueryGramList(unsigned gramCode, GramList<InvList>* gl)
    : gramCode(gramCode), gl(gl) {}
};

#endif
