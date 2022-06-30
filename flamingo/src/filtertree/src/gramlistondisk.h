/*
  $Id: gramlistondisk.h 5751 2010-10-01 17:31:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _gramlistondisk_h_
#define _gramlistondisk_h_

#include "gramlist.h"

#include <iostream>
#include <cstring>

template <typename InvList = vector<unsigned> >
class GramListOnDisk : public GramList<InvList> {
private:
  InvList* invList;

public:
  unsigned listSize;
  streampos startOffset;
  
  GramListOnDisk()
    : invList(NULL), listSize(0), startOffset(0) {}
  
  GramListOnDisk(unsigned listSize, streampos& startOffset)
    : invList(NULL), listSize(listSize), startOffset(startOffset) {}
  
  InvList* getArray(fstream* invListsFile = NULL) {     
    if(invList) return invList;
    //cout << "NOT RETURNING IMMEDIATELY?!" << endl;
    if(!invListsFile) return NULL;    
    invList = new InvList(listSize);
    invList->reserve(listSize);
    invList->resize(listSize);
    unsigned bytes = listSize * sizeof(typename InvList::value_type);
    invListsFile->seekg(startOffset);
    invListsFile->read((char*)&invList->at(0), bytes);
    return invList;
  }
  
  // fill the array from the given get position of the file stream
  // old invList will be deleted
  // no disk seek is performed
  void fillArray(fstream* invListsFile) {
    if(invList) clear();
    invList = new InvList(listSize);
    invList->reserve(listSize);
    invList->resize(listSize);
    unsigned bytes = listSize * sizeof(typename InvList::value_type);
    if(invListsFile->tellg() != startOffset) 
      cout << "THERE IS A PROBLEM HERE" << endl;
    invListsFile->read((char*)&invList->at(0), bytes);
  }
  
  void fillArray(char* data) {
    if(invList) clear();
    invList = new InvList(listSize);
    invList->reserve(listSize);
    invList->resize(listSize);
    unsigned bytes = listSize * sizeof(typename InvList::value_type);
    memcpy((char*)&invList->at(0), (const void*)data, bytes);    
  }
  
  void clear() {
    if(invList) delete invList;
    invList = NULL;
  }

  void free() {
    clear();
    delete this;
  }

  ~GramListOnDisk() {
    clear();
  }
};

#endif
