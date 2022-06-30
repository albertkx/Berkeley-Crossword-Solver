//  
// $Id: NFANode.h 4143 2008-12-08 23:23:55Z abehm $
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
#ifndef _NFANODE_
#define _NFANODE_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NFATransition.h"

#include <vector>
using namespace std;

class NFATransition;


class NFANode
{
public:
	
	/**A flag denoting whether the state is final or not.*/

	bool finalState;
       
	/**A flag denoting whether the state is init or not.*/
    bool initState;

        
	/**A vector of transitions. to next node*/
	vector<NFATransition*> branches;

    /**A vector of transitions. point to previous node*/
    vector<NFATransition*> backBranches;

    /**A vector of transitions. point to down node*/
    vector<NFATransition*> downBranches;

    /**A vector of transitions. point to up node*/
    vector<NFATransition*> upBranches;

    /**The number in the nodes vector (used for optimiation).*/
	int nodeNumber;

	/**Level of the node in the trie */
	int level;

    /**The name of the node.*/
	char name;



	NFANode (char name, bool finalState, bool initState);
	NFANode (char name, bool finalState, bool initState, int level);
	virtual ~NFANode();

	void printMeAll();
	void printMe();

};

#endif
