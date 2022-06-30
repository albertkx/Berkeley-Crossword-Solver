/*
  $Id: compressionargs.h 3456 2008-06-26 00:45:48Z rares $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  Academic BSD license.
  
  Date: 04/28/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _compressionargs_h_
#define _compressionargs_h_

#include <set>

#include "typedef.h"

class CompressionArgs {
 public:
  bool holesOptimized;
  std::set<uint>* blackList;
  
  CompressionArgs() { 
    blackList = NULL;
    holesOptimized = false;
  }
};

#endif
