/*
  $Id: stringshuffler.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
  
  Redistribution of this file is permitted under the terms of the BSD license

  Read strings from a file.  Shuffle them randomly.  Output the strings to
  a new file.

  Date: 05/11/2007
  Author: Chen Li <chenli (at) ics.uci.edu>
*/

#include <algorithm>
#include <iostream>
#include <fstream>
#include "input.h"

using namespace std;

void ShuffleStrings(const string &inputFileName,
		    const string &outputFileName,
		    const unsigned maxStringNum,
		    const unsigned strLengthLimit)
{
  // read string data
  vector<string> strings;
  readString(strings, inputFileName, maxStringNum, strLengthLimit);

  // create an array of ids and shuffle them
  unsigned stringNum = strings.size();
  unsigned ids[stringNum];
  for (register unsigned i = 0; i < stringNum; i ++)
    ids[i] = i;
  random_shuffle(ids, ids + stringNum);

  // output the shuffled strings
  ofstream outputFile;
  outputFile.open(outputFileName.c_str());
  for (register unsigned i = 0; i < stringNum; i ++) {
    int id = ids[i];
    outputFile << strings.at(id) << endl;
  }
  outputFile.close();
}
