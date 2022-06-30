/*  
    $Id: mergeoptmerger.cc 5716 2010-09-09 04:27:56Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    This imeplementation merges multiple lists by 
    the algorithm proposed in SIGMOD'04.
    "Effiicent set joins on similarity predicates"
    Separate lists to two disjoint sets
    according to the lengths of lists.
    Heap-merge short lists and
    binary search for longer lists.

    The lists are assumed to
    be sorted in an ascending order.

    Author: Jiaheng Lu 
    Date: 05/16/2007

*/

#include "mergeoptmerger.h"
