/*
  $Id: gramgen.h Tue Apr 05 10:20:24 PDT 2008 abehm$

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of BSD license.
    
  Date: 04/29/2009
  Author: Rares Vernica <rares (at) ics.uci.edu> 
  
  This file is needed in oder to easily cross-compile the code that uses 
  TR1 functions/classes between Linux and Visual C++. Linux puts the TR1 
  headers in tr1 directory while Visual C++ does not. The developer is 
  advised to include this header whenever he/she needs a TR1 header. If a
  specific header is not included by this file, it can be added to it.
*/

#include <functional>
#include <unordered_map>
#include <unordered_set>
