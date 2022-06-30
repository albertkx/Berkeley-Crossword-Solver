/*
  $Id: output.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "output.h"
#include "debug.h"

void writeString(const vector<string> &data, const string &filenameData)
{
  ofstream fileData(filenameData.c_str(), ios::out);
  if (!fileData) {
    cerr << "can't open output file \"" << filenameData << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  WRITING_FILE(filenameData);
  for (vector<string>::const_iterator it = data.begin();
       it !=  data.end(); ++it)
    fileData << *it << endl;
    
  fileData.close();
  WRITING_DONE();
}
