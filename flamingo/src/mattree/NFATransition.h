//
// NFATransition.h: interface for the NFATransition class.
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

#ifndef _NFATRAN_
#define _NFATRAN_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NFANode.h"

class NFANode;

class NFATransition  
{
public:
	
	char labels[2];
	NFANode* outNode;
	NFANode* fromNode;

	NFATransition (char label[], NFANode* outNode);
	NFATransition (char label[], NFANode* outNode, NFANode* fromNode);
	NFATransition (NFANode* toNode, NFANode* fromNode);
	virtual ~NFATransition();

	void setLabels(char labels[]);
	char* getLabels();
	void printMe(int type);

};

#endif 

