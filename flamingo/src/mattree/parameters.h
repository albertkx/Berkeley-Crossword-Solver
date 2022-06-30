//  
// $Id: parameters.h 4143 2008-12-08 23:23:55Z abehm $
//
//  Copyright (C) 2004 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: October, 2004
//
// Author: 
//          Liang Jin (liangj (at) ics.uci.edu)
//

#ifndef _PARAMETERS_
#define _PARAMETERS_

#include <math.h>

//#define LEAFNODES (int)ceil((double)SIZES/MAXCARD)
#define LEAFNODES (int)ceil((double)SIZES/LEAFCARD)

const int MAXLEN = 100;

const int DIMNUM = 0;
const int DIMSTR = 1;

/* PGSIZE is normally the natural page size of the machine */

//const int PGSIZE = 512;
//const int TRIELEN =	500;
const int PGSIZE = 256;
const int TRIELEN =	1000;

const int PAGESIZE = 8192; //for size calculation only

//const int PGSIZE = 284;
//const int TRIELEN =	900;

const int K = 400;

const int STRDELTA = 3;

const int NUMDELTA = 4;

const int SIZES = 80000;

const int NUMDIMS =	2;	/* number of dimensions */
#define NUMSIDES 2*NUMDIMS

const int ALPH_SIZE = 29;

const char EPSILON = '&';
const int INVALID =	-10000;

#define DATAFILE "data.txt"
#define QUERYFILE "query.txt"

const int TOPDOWN = 1;
const int BOTTOMUP = 2;

const int MINIMUM = -9999;

const int NUMQUERY = 10;

const int TO = 1;
const int BACK = 2;
const int UP = 3;
const int DOWN = 4;

const float CRDELTA = 0;



#endif
