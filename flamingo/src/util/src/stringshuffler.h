/*
  $Id: shrinkshuffle.cc 1267 2007-05-15 21:40:43Z chenli $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD license

  Read strings from a file.  Shuffle them randomly.  Output the strings to
  a new file.

  Date: 05/11/2007
  Author: Chen Li <chenli (at) ics.uci.edu>
*/

#ifndef _stringshuffler_h_
#define _stringshuffler_h_

#include <string>
using namespace std;

// Read strings from a file.  Shuffle them randomly.  Output the strings to
// a new file. Ignore strings whose length is greater than the given
// limit.  The function was tested on 2 million URLs and was OK. But it
// failed on a larger data set.
void ShuffleStrings(const string &inputFileName,
		    const string &outputFileName,
		    const unsigned maxStringNum,
		    const unsigned strLengthLimit);
#endif
