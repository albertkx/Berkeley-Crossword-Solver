/*
 $Id: storage.h 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 11/01/2009
 Author: Shengyue Ji <shengyuj (at) ics.uci.edu>
*/

#include <stdio.h>
#include <vector>
#include <stdint.h>
#include <tr1/unordered_map>

using namespace std;
using namespace tr1;

// Buffers managed by Storage should extend this
class Buffer
{
public:
    // identifier of the buffer
    uintptr_t id;

    // size of the Buffer in bytes (including self)
    size_t size;
};

class Storage
{
    // id of the root buffer
    uintptr_t rootId;

public:
    // memory storage should use this
    Storage();

    // disk storage should use this
    // memory storage can also use this to load from disk
    Storage(const char *filename);

    ~Storage();

    // allocate a Buffer of size on the storage and return the buffer
    Buffer *alloc(size_t size);

    // deallocate the Buffer by id from storage
    void dealloc(uintptr_t id);

    // return the Buffer identified by id
    // disk storage should allocate Buffer also
    // memory storage should directly return the Buffer
    Buffer *read(uintptr_t id);

    // write the Buffer identified by id to disk
    // memory storage should do nothing
    void write(const Buffer *buf);

    // free the Buffer returned from disk read and allocation
    // memory storage should do nothing
    void free(Buffer *buf);

    // return the buffer id of the root
    uintptr_t getRoot();

    // set the buffer id of the root
    // disk storage should also write to file
    void setRoot(uintptr_t id);

};

