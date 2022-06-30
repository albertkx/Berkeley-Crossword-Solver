/*
  $Id: foreign_listmerger.h 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/25/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "oldlistmerger/divideskipmerger.h"
#define _foreign_listmerger_h

#ifndef _foreign_listmerger_h
#define _foreign_listmerger_h

#include <vector>

#include "common/src/typedef.h"

void addMAXUnsigned2EachList(std::vector<Array<uint>*>, uint);
void deleteMAXUnsignedfromEachList(std::vector<Array<uint>*>);
void heapInsert(uint, uint, uint*, uint*, uint);
void heapDelete(uint*, uint*, uint);
void insertToHeaps(uint*, uint*, uint, std::vector<Array<uint>*>, uint*, uint*, uint);
void skipNodes(std::vector<Array<uint>*>, uint*, uint, uint, uint*);

struct DivideSkipMerger
{
  void merge(std::vector<Array<uint>*>, uint, std::vector<uint>);
}; 

#endif
