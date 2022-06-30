/*
 $Id: kwdsstorage.h 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 08/19/2010
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdint.h>
#include <tr1/unordered_map>

using namespace std;
using namespace tr1;

struct IndexNode
{
    uintptr_t position;
    size_t size;
    uintptr_t id;
    unsigned numKeywords;
};

class KeywordsFile
{
private:
    char *buffer;
    fstream keywordsFile;
    string keywordsFileName;
    unordered_map<uintptr_t, IndexNode> indexMap;

    void readIndex(string indexFileName, bool newFile);
    void writeIndex();
public:
    KeywordsFile(string fileName);
    ~KeywordsFile();
    unordered_map <uintptr_t, IndexNode>::iterator begin();
    unordered_map <uintptr_t, IndexNode>::iterator end();
    IndexNode getIndexNode(uintptr_t id);
    bool open(bool newFile = false);
    string read(uintptr_t id);
    bool write(string text, uintptr_t id, unsigned numKeywords);
    void close();
    unsigned getRecordCount();
};
