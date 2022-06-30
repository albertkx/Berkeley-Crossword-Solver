/*
  $Id: duplicate.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _duplicate_h_
#define _duplicate_h_

#include <string>

using namespace std;

string trim(const string &s);
void EditDistModif(string &sDup, unsigned noChanges);
void insertDuplicates();

#endif
