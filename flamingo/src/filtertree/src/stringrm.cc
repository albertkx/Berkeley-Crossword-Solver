/*
  $Id: stringrm.cc 5718 2010-09-09 05:39:08Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include <cstring>
#include <algorithm>

#include "stringrm.h"

int FreeSpaceEntryCmpFunc(const void * a, const void * b) {
  if( ((FreeSpaceEntry*)a)->freeSpace > ((FreeSpaceEntry*)b)->freeSpace ) return -1;
  if( ((FreeSpaceEntry*)a)->freeSpace < ((FreeSpaceEntry*)b)->freeSpace ) return 1;
  if( ((FreeSpaceEntry*)a)->freeSpace == ((FreeSpaceEntry*)b)->freeSpace ) return 0;
  return 0;
}

void 
FreeSpaceManager::
insert(const unsigned blockNum, const unsigned freeSpace) {
  BlockWithSpace entry(blockNum, freeSpace);
  FreeSpaceSet::iterator iter = freeSpaceEntries.find(entry);
  if(iter == freeSpaceEntries.end()) {
    if(freeSpaceEntries.size() < maxEntries) {
      freeSpaceEntries.insert(entry);
      totalFreeSpace += freeSpace;
    }
    else {
      streampos avgFreeSpace = (totalFreeSpace / freeSpaceEntries.size());
      if(freeSpace >  avgFreeSpace) {
	for(FreeSpaceSet::iterator del_iter = freeSpaceEntries.begin();	
	    del_iter != freeSpaceEntries.end();
	    del_iter++) {
	  if(del_iter->freeSpace < avgFreeSpace) {
	    totalFreeSpace -= del_iter->freeSpace;	  
	    freeSpaceEntries.erase(del_iter);	      
	    break;
	  }
	}
	freeSpaceEntries.insert(entry);	  
	totalFreeSpace += freeSpace;
      }
    }
  }
  else {
    totalFreeSpace -= iter->freeSpace;
      freeSpaceEntries.erase(iter);
      freeSpaceEntries.insert(entry);	  
      totalFreeSpace += freeSpace;
  }
}

unsigned 
FreeSpaceManager::
getBlockWithSpace(unsigned freeSpace) {
  for(FreeSpaceSet::const_iterator iter = freeSpaceEntries.begin();
      iter != freeSpaceEntries.end();
      iter++) {
    if(iter->freeSpace >= freeSpace) return iter->blockNum;
  }
  return NO_SPACE_IN_BLOCK;
}

void 
FreeSpaceManager::
print() {
  cout << "TOTAL FREE SPACE: " << totalFreeSpace << endl;
  cout << "AVG FREE SPACE: " << (totalFreeSpace / freeSpaceEntries.size()) << endl;
  for(FreeSpaceSet::const_iterator iter = freeSpaceEntries.begin();
      iter != freeSpaceEntries.end();
      iter++) {
    cout << iter->blockNum << " " << iter->freeSpace << endl;      
  }
  cout << "--------------" << endl;
}


BufferManagerRM::
BufferManagerRM(fstream* file, unsigned blockSize, unsigned bufferSlots)
  : file(file), clockHand(NULL), bufferSlots(bufferSlots), blockSize(blockSize) {
  init();  
}

void 
BufferManagerRM::
init() {
  BufferEntry* runner = new BufferEntry(blockSize);
  clockHand = runner;
  for(unsigned i = 0; i < bufferSlots-1; i++) {
    runner->next = new BufferEntry(blockSize);
    runner = runner->next;
  }
  runner->next = clockHand; // close the circle
}

void 
BufferManagerRM::
readDiskBlock(char* block, const unsigned blockNum) {
  file->seekg(blockNum * blockSize);
  file->read(block, blockSize);
}

void 
BufferManagerRM::
writeDiskBlock(const char* block, const unsigned blockNum) {
  file->seekp(blockNum * blockSize);
  file->write(block, blockSize);
}


BufferEntry* 
BufferManagerRM::
getBlock(unsigned blockNum, bool appendBlock) {    
  tr1::unordered_map<unsigned, BufferEntry*>::iterator iter = cacheMap.find(blockNum);
  if(iter == cacheMap.end()) {

    // find entry to replace, always start with next
    while(clockHand->val != 0) {
      clockHand->val = 0;
      clockHand = clockHand->next;
    }
    
    // first make the old buffer slot persistent
    if(clockHand->dirty) writeDiskBlock(clockHand->data, clockHand->blockNum);
    cacheMap.erase(clockHand->blockNum);
        
    if(!appendBlock) readDiskBlock(clockHand->data, blockNum);
    clockHand->blockNum = blockNum;
    clockHand->val = 1;
    clockHand->dirty = false;
    cacheMap[blockNum] = clockHand;          
    
    return clockHand;   
  }
  else {    
    iter->second->val = 1;
    return iter->second;
  }
}

bool
BufferManagerRM::
inCache(unsigned blockNum) {
  return cacheMap.find(blockNum) != cacheMap.end();
}

void 
BufferManagerRM::
flushBuffer() {  
  BufferEntry* runner = clockHand;
  while(runner->next != clockHand) {
    if(runner->blockNum != UNUSED_BUFFER_ENTRY) writeDiskBlock(runner->data, runner->blockNum); 
    runner = runner->next;
  }   
  if(runner->blockNum != UNUSED_BUFFER_ENTRY) writeDiskBlock(runner->data, runner->blockNum);  
  cacheMap.clear();
}

BufferManagerRM::
~BufferManagerRM() {
  BufferEntry* runner = clockHand;
  while(runner->next != clockHand) {
    BufferEntry* next = runner->next;
    delete runner;
    runner = next;
  }
  delete runner;
}

void 
StringRM::
initDiskBlock(char* block) {
  memset(block, 0, blockSize); // set whole block to zeros
  // set dictionary entries to unused
  memset(block + dictOffset, CHAR_MAXVAL, numDictSlots * sizeof(unsigned));
  // set next block pointer to unused
  memset(block + dictOffset + numDictSlots * sizeof(unsigned), CHAR_MAXVAL, sizeof(unsigned));    
  // set free space entry
  FreeSpaceEntry entry;
  entry.freeSpace = blockSize - sizeof(unsigned) - numDictSlots * sizeof(unsigned) - numFreeSpaceSlots * sizeof(FreeSpaceEntry);
  entry.offset = numFreeSpaceSlots * sizeof(FreeSpaceEntry);
  addFreeSpaceEntry(block, &entry);   
}

void 
StringRM::
addFreeSpaceEntry(char* block, const FreeSpaceEntry* entry) {
  unsigned i;
  for(i = 0; i < numFreeSpaceSlots; i++) {
    FreeSpaceEntry* tmp = (FreeSpaceEntry*)(block + i*sizeof(FreeSpaceEntry));
    if(tmp->freeSpace == UNUSED_FREE_SPACE_SLOT) {
      memcpy(tmp, entry, sizeof(FreeSpaceEntry));
      break;
    }
  }
  if(i == numFreeSpaceSlots) {
    cout << "ERROR: could not add free space entry" << endl;
    // TODO: probably should schedule a re-org of this page here
  }
  // always sort free space entries
  sortFreeSpaceEntries(block);
}

void 
StringRM::
sortFreeSpaceEntries(char* block) {
  qsort(block, numFreeSpaceSlots, sizeof(FreeSpaceEntry), FreeSpaceEntryCmpFunc);
}

unsigned 
StringRM::
getNextBlockNum(char* block) {
  return *(block + nextBlockOffset);
}

unsigned 
StringRM::
insertStringIntoBlock(BufferEntry* bufferEntry, const string& s) {
  // implement first fit policy
  char* block = bufferEntry->data;
  FreeSpaceEntry* entry = (FreeSpaceEntry*)block;
  if(entry->freeSpace < s.size()+sizeof(unsigned)) return NO_SPACE_IN_BLOCK; //means there is not enough space    
  // try to add entry in dictionary
  unsigned insertedSlot = 0;
  unsigned slotCounter;
  for(slotCounter = 0; slotCounter < numDictSlots; slotCounter++) {
    unsigned* dictEntry = (unsigned*)(block + dictOffset + slotCounter * sizeof(unsigned));
    if(*dictEntry == UNUSED_DICT_SLOT) {
      *dictEntry = entry->offset;
      insertedSlot = slotCounter;
      break;	
    }
  }
  if(slotCounter == numDictSlots) return NO_SPACE_IN_BLOCK;
  
  // add data to block
  unsigned strLength = s.size();
  const char* rawData = s.c_str();
  memcpy(block + entry->offset, &strLength, sizeof(unsigned)); 
  memcpy(block + entry->offset + sizeof(unsigned), rawData, strLength);
  
  // modify free space entry, TODO: merge adjacent free space blocks
  entry->freeSpace = entry->freeSpace - strLength - 4;
  entry->offset = entry->offset + strLength + 4;
  sortFreeSpaceEntries(block);    
  
  // set dirty bit
  bufferEntry->dirty = true;
  
  return insertedSlot;
}

void 
StringRM::
insertStringIntoLastBlock(const string& s, RecordID& rid) {
  unsigned retVal = 0;
  BufferEntry* block = bufferManager->getBlock(lastFileBlock);
  retVal = insertStringIntoBlock(block, s);
  if(retVal == NO_SPACE_IN_BLOCK) {    
    lastFileBlock++;
    block = bufferManager->getBlock(lastFileBlock, true);        
    initDiskBlock(block->data);
    retVal = insertStringIntoBlock(block, s);
  }
  rid.blockNum = lastFileBlock;
  rid.slotNum = retVal;
}

void 
StringRM::
retrieveString(string& target, const RecordID& rid) {     
  BufferEntry* entry = bufferManager->getBlock(rid.blockNum);
  char* block = entry->data;
  unsigned strOffset;
  memcpy(&strOffset, block + dictOffset + rid.slotNum * sizeof(unsigned), sizeof(unsigned));
  unsigned strLen;
  memcpy(&strLen, block + strOffset, sizeof(unsigned));
  target.assign(block + strOffset + sizeof(unsigned), strLen);    
}

void 
StringRM::
insertString(const string& s, RecordID& rid) {  
  unsigned freeBlock = freeSpaceManager->getBlockWithSpace(s.size() + sizeof(unsigned));  
  if(freeBlock == NO_SPACE_IN_BLOCK) {
    insertStringIntoLastBlock(s, rid);
  }
  else {
    BufferEntry* block = bufferManager->getBlock(freeBlock);
    insertStringIntoBlock(block, s);
  }
}

StringRM::
StringRM(unsigned bufferSlots)
  : blockSize(0), numDictSlots(0), numFreeSpaceSlots(0), dictOffset(0),
    nextBlockOffset(0), lastFileBlock(1), bufferSlots(bufferSlots),
    freeSpaceManager(NULL), bufferManager(NULL) {}

void 
StringRM::
createStringCollection(const char* fileName, const unsigned blockSize, const unsigned avgstrlen) {
  if(file.is_open()) file.close();
  file.open(fileName, ios::in | ios::out | fstream::trunc);
  if(!file.is_open()) cout << "ERROR: COULD NOT OPEN FILE: " << fileName << endl;
  this->blockSize = blockSize;
  this->numDictSlots = blockSize / (avgstrlen + sizeof(unsigned));
  this->numFreeSpaceSlots = this->numDictSlots / 2;
  this->dictOffset = blockSize - sizeof(unsigned) - numDictSlots * sizeof(unsigned);
  this->nextBlockOffset = blockSize - sizeof(unsigned);
  this->lastFileBlock = 1; 
  if(freeSpaceManager) delete freeSpaceManager;
  freeSpaceManager = new FreeSpaceManager();
  if(bufferManager) delete bufferManager;
  bufferManager = new BufferManagerRM(&file, blockSize, bufferSlots); 
  BufferEntry* entry = bufferManager->getBlock(0, true);
  writeCollectionDescr(entry->data);
  entry->dirty = true;
  entry = bufferManager->getBlock(1, true);
  initDiskBlock(entry->data);
  entry->dirty = true;
  bufferManager->flushBuffer();
  file.close();
}

void 
StringRM::
openStringCollection(const char* fileName, bool disableStreamBuffer) {
  if(file.is_open()) file.close();
  if(disableStreamBuffer) file.rdbuf()->pubsetbuf(0, 0);
  file.open(fileName, ios::in | ios::out);
  if(!file.is_open()) cout << "ERROR: COULD NOT OPEN FILE: " << fileName << endl;
  readCollectionDescr();
  if(freeSpaceManager) delete freeSpaceManager;
  freeSpaceManager = new FreeSpaceManager();  
  if(bufferManager) delete bufferManager;
  bufferManager = new BufferManagerRM(&file, blockSize, bufferSlots);
}

void 
StringRM::
writeCollectionDescr(char* block) {
  char* runner = block;
  memset(block, 0, this->blockSize);
  memcpy(runner, (void*)&this->blockSize, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, (void*)&this->numDictSlots, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, &this->numFreeSpaceSlots, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, &this->dictOffset, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, &this->nextBlockOffset, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, &this->lastFileBlock, sizeof(unsigned));
  runner += sizeof(unsigned);
  memcpy(runner, &this->bufferSlots, sizeof(unsigned));
}

void
StringRM::
readCollectionDescr() {  
  file.seekp(0);
  file.read((char*)&this->blockSize, sizeof(unsigned));
  file.read((char*)&this->numDictSlots, sizeof(unsigned));
  file.read((char*)&this->numFreeSpaceSlots, sizeof(unsigned));
  file.read((char*)&this->dictOffset, sizeof(unsigned));
  file.read((char*)&this->nextBlockOffset, sizeof(unsigned));
  file.read((char*)&this->lastFileBlock, sizeof(unsigned));
  file.read((char*)&this->bufferSlots, sizeof(unsigned));
}

// forces flushing of cache to disk
void 
StringRM::
flushCache() {
  // update the meta information
  BufferEntry* descrEntry = bufferManager->getBlock(0);
  writeCollectionDescr(descrEntry->data);
  // flush dirty pages
  bufferManager->flushBuffer();  
}

bool
StringRM::
inCache(const RecordID& rid) {     
  return bufferManager->inCache(rid.blockNum);
}

StringRM::
~StringRM() {
  if(file.is_open()) file.close();
  if(freeSpaceManager) delete freeSpaceManager;
  if(bufferManager) {
    bufferManager->flushBuffer();
    delete bufferManager;
  }
}
