//
// $Id: split_q.h 5770 2010-10-19 06:38:25Z abehm $
//
// split_q.h
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: Michael Ortega-Binderberger (miki (at) ics.uci.edu)
//          Liang Jin (liangj (at) ics.uci.edu)
//          Chen Li (chenli (at) ics.uci.edu)
//


/*-----------------------------------------------------------------------------
| Definitions and global variables.
-----------------------------------------------------------------------------*/

#define METHODS 1

struct Branch BranchBuf[NODECARD+1];
struct Rect CoverSplit;
RectReal CoverSplitArea;

/* variables for finding a partition */
struct PartitionVars
{
	int partition[NODECARD+1];
	int taken[NODECARD+1];
	int count[2];
	struct Rect cover[2];
	RectReal area[2];
} Partitions[METHODS];
