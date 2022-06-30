/*
  $Id: stringrm.h 5718 2010-09-09 05:39:08Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _stringrm_h_
#define _stringrm_h_

#include <iostream>
#include <fstream>
#include <set>
#include <tr1/unordered_map>

#define UNUSED_DICT_SLOT 0xFFFFFFFF
#define UNUSED_NEXT_PTR 0xFFFFFFFF
#define UNUSED_BUFFER_ENTRY 0xFFFFFFFF
#define NO_SPACE_IN_BLOCK 0xFFFFFFFF
#define CHAR_MAXVAL 0xFF
#define UNUSED_FREE_SPACE_SLOT 0x00000000

using namespace std;

typedef struct {
  unsigned blockNum;
  unsigned slotNum;
} RecordID;

// manages free space at the block level
class FreeSpaceManager {
 private:
  // inner class
  class BlockWithSpace {
   public:
    unsigned blockNum;
    unsigned freeSpace;
    BlockWithSpace() {}
    BlockWithSpace(unsigned blockNum, unsigned freeSpace)
      : blockNum(blockNum), freeSpace(freeSpace) {}
    bool operator() (const BlockWithSpace& lhs, const BlockWithSpace& rhs) const {
      return lhs.blockNum < rhs.blockNum;
    }    
  };

  typedef set<BlockWithSpace, BlockWithSpace> FreeSpaceSet;

  unsigned maxEntries;
  streampos totalFreeSpace;
  FreeSpaceSet freeSpaceEntries;

public:
  FreeSpaceManager(const unsigned maxEntries = 2000)
    : maxEntries(maxEntries), totalFreeSpace(0) {}
  
  void insert(const unsigned blockNum, const unsigned freeSpace);
  unsigned getBlockWithSpace(unsigned freeSpace);
  void print();  
};

typedef struct {
  unsigned freeSpace;
  unsigned offset;
} FreeSpaceEntry;
int FreeSpaceEntryCmpFunc(const void * a, const void * b);

class BufferEntry {
public:
  BufferEntry* next;
  char* data;
  char val;  
  unsigned blockNum;
  bool dirty;
  
  void reset() {
    blockNum = UNUSED_BUFFER_ENTRY;
    val = (char)0;
    dirty = false;
  }
  
  BufferEntry(unsigned blockSize)
    : next(NULL), data(new char[blockSize]) {
    reset();
  }
  
  ~BufferEntry() {
    if(data) delete[] data;
  }
};

// buffer manager that implements the clock algorithm
class BufferManagerRM {
private:
  fstream* file;
  tr1::unordered_map<unsigned, BufferEntry*> cacheMap;
  BufferEntry* clockHand; // for implementing the clock algorithm
  unsigned bufferSlots;
  unsigned blockSize;
  
  void init();  
  void readDiskBlock(char* block, const unsigned blockNum);  
  void writeDiskBlock(const char* block, const unsigned blockNum);

public:
  BufferManagerRM(fstream* file, unsigned blockSize, unsigned bufferSlots = 10);  
  
  BufferEntry* getBlock(unsigned blockNum, bool appendBlock = false);
  void flushBuffer();

  bool inCache(unsigned blockNum);

  ~BufferManagerRM();
};

class StringRM {
private:  
  unsigned blockSize;
  unsigned numDictSlots;
  unsigned numFreeSpaceSlots;
  unsigned dictOffset;
  unsigned nextBlockOffset;
  unsigned lastFileBlock;
  unsigned bufferSlots;
  fstream file;
  FreeSpaceManager* freeSpaceManager;
  BufferManagerRM* bufferManager;
  
  void initDiskBlock(char* block);
  void addFreeSpaceEntry(char* block, const FreeSpaceEntry* entry);   
  void sortFreeSpaceEntries(char* block);
  unsigned getNextBlockNum(char* block);
  unsigned insertStringIntoBlock(BufferEntry* entry, const string& s);
  void insertStringIntoLastBlock(const string& s, RecordID& rid);
  
  void writeCollectionDescr(char* block);
  void readCollectionDescr(); 

  void initCache();
  
public:
  StringRM(unsigned bufferSlots = 1);
  void createStringCollection(const char* fileName, const unsigned blockSize = 4096, const unsigned avgstrlen = 50);
  void openStringCollection(const char* fileName, bool disableStreamBuffer = false); 
  void insertString(const string& s, RecordID& rid);
  void retrieveString(string& target, const RecordID& rid);
  void flushCache();

  bool inCache(const RecordID& rid);

  ~StringRM();
};

#endif
