/*
  $Id: simvect.h 4928 2009-12-17 22:52:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _simvect_h_

// default value for SIM_VECT
#ifndef SIM_VECT
#define SIM_VECT 1
#endif

#define _simvect_h_

#if SIM_DIST ==  1
#include "editvect.h"

#if SIM_VECT == 1
typedef EditVect SimVect;

#elif SIM_VECT == 2
typedef EditVectID SimVect;

#elif SIM_VECT == 3
typedef EditVectIS SimVect;

#elif SIM_VECT == 4
typedef EditVectDS SimVect;

#elif SIM_VECT == 5
typedef EditVectIDS SimVect;

#endif

#elif SIM_DIST == 2
#include "jaccvect.h"
typedef JaccVect SimVect;
#endif

#endif
