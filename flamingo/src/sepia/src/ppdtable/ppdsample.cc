/*
  $Id: ppdsample.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ppdsample.h"

ostream& PPDSample::info(ostream &out)
{
  out << "PPDTable" << endl << "---" << endl;
  out << "PPDTable sample size\t" << samplePer << "%" << endl;
  return out << endl;
}
