/* 
   $Id: showutil.h 5716 2010-09-09 04:27:56Z abehm $

   Copyright (C) 2010 by The Regents of the University of California
	
   Redistribution of this file is permitted under
   the terms of the BSD license.
   
   Date: 04/20/2007
   Author: Jiaheng Lu
*/
 	
#ifndef _showutil_h_
#define _showutil_h_

#include <vector>
#include <map>
#include <set>

#include "util/src/array.h"

using namespace std;

void showArrayUnsignedLists(const vector<Array<unsigned> *> &lists);

void showArrayUnsigned(Array<unsigned> *v);

void showVectorUnsigned(vector<unsigned> *v);

void printVectorUnsigned(const vector<unsigned> *v, unsigned count);

void printArrayUnsigned(const unsigned *array, unsigned count);

void showUnsignedSet( set<unsigned,less<unsigned> > *s);

void showStringVector(const vector<string> &v);

#endif
