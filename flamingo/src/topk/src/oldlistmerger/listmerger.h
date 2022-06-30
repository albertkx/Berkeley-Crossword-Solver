/*  
 $Id: listmerger.h 5716 2010-09-09 04:27:56Z abehm $ 

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Author: Chen Li, Jiaheng Lu, and Yiming Lu, Alexander Behm
 Date: 05/11/2007
*/
 	
#ifndef _listsmerger_h_
#define _listsmerger_h_

#include <vector>

#include "util/src/array.h"
#include "counttable.h"
#include "utilities.h"

using namespace std;

enum DataSet {URL, DBLP, IMDB, Google};
 
template <class ListsMergerConcrete, class InvList>
class ListsMerger {
 protected:
  bool hasDuplicateLists;

 public:

  ListsMerger(bool hasDuplicateLists = false){ this->hasDuplicateLists = hasDuplicateLists; }
  
  // the lists are assumed to be sorted in an ascending order
  void merge(vector<InvList*> &arrays, 
	     const unsigned threshold, // threshold of count
	     vector<unsigned> &results) {
    static_cast<ListsMergerConcrete*>(this)->merge_Impl(arrays, threshold, results);
  }
  
  string getName() {
    return "ListsMerger";
  }

  ~ListsMerger() {};
}; 

#endif
