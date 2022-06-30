//  
// $Id: NFAMatch.h 4143 2008-12-08 23:23:55Z abehm $
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

#ifndef _NFAMATCH_
#define _NFAMATCH_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NFA.h"

class NFAMatch  
{
public:

	NFA* automaton;
    NFA** graph;
    int** C;
	int size;
    

	NFAMatch(NFA* automaton);
	virtual ~NFAMatch();

	int match(string str);
	bool match(string str, int delta);
	void buildGraph(string str);
	void labelStepOne(string str);
	void labelStepTwo(string str);
	void labelStepThree(string str);
	int dynamicProg(string str);
	bool dynamicProg(string str, int threshold);

	NFANode* getNode(int i, int s);
	void printGraph(int i);
	void resetC(int M, int N, int num);
	int max(int i,  vector<NFATransition*> edgeArr);
	int InsertionMax(int i, NFANode* node);
	int SubstitutionMax(int i, NFANode* node, string str);

	int scoreLabel(char labels[]);
	int scoreLabel(char label0, char label1);
	int maxOfThree(int num1, int num2, int num3);
	void printMatr(string str);
};

#endif

