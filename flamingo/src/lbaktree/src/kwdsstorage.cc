/*
 $Id: kwdsstorage.cc 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 08/19/2010
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include "kwdsstorage.h"

KeywordsFile::KeywordsFile(string fileName)
{
    keywordsFileName = fileName;
    buffer = 0;
}

void KeywordsFile::readIndex(string indexFileName, bool newFile)
{
    fstream indexFile;

    if(newFile)
    {
        indexFile.open(indexFileName.c_str(), ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
    }
    else
    {
        indexFile.open(indexFileName.c_str(), ios_base::in | ios_base::out | ios_base::binary);
        if(indexFile.is_open())
        {
            indexFile.seekg(0, ios_base::end);
            if(indexFile.tellg() / sizeof(IndexNode) > 0)
            {
                indexFile.seekg(0, ios_base::beg);
                IndexNode irec;
                while(!indexFile.eof())
                {
                    indexFile.read((char *)&irec, sizeof(irec));
                    indexMap[irec.id] = irec;
                }
            }
            indexFile.close();
        }
    }
}

void KeywordsFile::writeIndex()
{
    fstream indexFile;
    IndexNode irec;
    indexFile.open((keywordsFileName + ".idx").c_str(), ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
    if(indexFile.is_open())
    {
        unordered_map <uintptr_t, IndexNode>::iterator it;
        for (it = indexMap.begin(); it != indexMap.end(); ++it)
        {
            irec = (*it).second;
            indexFile.write((char *)&irec, sizeof(irec));
        }
        indexFile.close();
    }
}

IndexNode KeywordsFile::getIndexNode(uintptr_t id)
{
    return indexMap[id];
}

bool KeywordsFile::open(bool newFile)
{
    if(newFile)
    {
        keywordsFile.open((keywordsFileName + ".dat").c_str(), ios_base::in | ios_base::out | ios_base::binary | ios_base::trunc);
    }
    else
    {
        keywordsFile.open((keywordsFileName + ".dat").c_str(), ios_base::in | ios_base::out | ios_base::binary);
    }
    readIndex(keywordsFileName + ".idx", newFile);
    return keywordsFile.is_open();
}


KeywordsFile::~KeywordsFile()
{
    if(keywordsFile.is_open())
    {
        close();
    }
}

unordered_map <uintptr_t, IndexNode>::iterator KeywordsFile::begin()
{
    return indexMap.begin();
}

unordered_map <uintptr_t, IndexNode>::iterator KeywordsFile::end()
{
    return indexMap.end();
}

void KeywordsFile::close()
{
    writeIndex();
    keywordsFile.close();
    keywordsFile.clear();
    indexMap.clear();
}

bool KeywordsFile::write(string text, uintptr_t id, unsigned numKeywords)
{
    bool result = true;
    IndexNode irec;

    try
    {
        unordered_map<uintptr_t, IndexNode>::iterator it;
        it = indexMap.find(id);

        if(it != indexMap.end())
        {
            indexMap.erase(it);
            keywordsFile.seekp((it->second).position);
            irec.position = keywordsFile.tellp();
            irec.size = text.length();
            irec.id = id;
            irec.numKeywords = numKeywords;
            indexMap[id] = irec;
            keywordsFile.write(text.c_str(), (std::streamsize)text.length());
            keywordsFile.seekp(0, ios_base::end);
        }
        else
        {
            irec.position = keywordsFile.tellp();
            irec.size = text.length();
            irec.id = id;
            irec.numKeywords = numKeywords;
            indexMap[id] = irec;
            keywordsFile.write(text.c_str(), (std::streamsize)text.length());
        }
    }
    catch(char *ex)
    {
        cout << "Problem writing " << ex << endl;
        result = false;
    }

    return (result);
}

string KeywordsFile::read(uintptr_t id)
{
    IndexNode irec = indexMap[id];
    if(buffer)
    {
        delete [] buffer;
    }
    buffer = new char[irec.size + 1];
    keywordsFile.seekg(irec.position, ios_base::beg);
    keywordsFile.read(buffer, irec.size);
    buffer[irec.size] = '\0';
    return (string)buffer;
}

unsigned KeywordsFile::getRecordCount()
{
    return (unsigned)indexMap.size();
}
