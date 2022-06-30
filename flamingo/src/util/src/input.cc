/*
  $Id: input.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 02/16/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "input.h"

void readString(vector<string> &data, const string &filenameData, unsigned count, 
                unsigned maxLineLen)
{
  ifstream fileData(filenameData.c_str());
  if (!fileData) {
    cerr << "can't open input file \"" << filenameData << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  READING_FILE(filenameData);
  char *line = new char[maxLineLen + 1];
  bool isIgnore = false;

  while (true) {
    fileData.getline(line, maxLineLen + 1);
    if (fileData.eof())
      break;
    if (fileData.rdstate() & ios::failbit) {
      isIgnore = true;
      while (fileData.rdstate() & ios::failbit) {      
        fileData.clear(fileData.rdstate() & ~ios::failbit);
        fileData.getline(line, maxLineLen);
      }
      cerr << "open reading input file \"" << filenameData << "\"" << endl
           << "line length might exceed " << maxLineLen << " characters" << endl;
      exit(EXIT_FAILURE);
    }
    else
      data.push_back(string(line));
    if (count != 0 && data.size() == count)
      break;
  }
  READING_DONE();
  fileData.close();

  if (isIgnore)
    cerr << "WARNING" << endl 
         << "some lines in the file exceeded " << maxLineLen 
         << " characters and were ignored" << endl;

  delete [] line;
}

bool existFileBin(const string &filename)  
{
  ifstream file(filename.c_str(), ios::in | ios::binary);
  if (!file) 
    return false;
  file.close();
  return true;
}

bool existFile(const string &filename) 
{
  ifstream file(filename.c_str(), ios::in);
  if (!file) 
    return false;
  file.close();
  return true;
}
