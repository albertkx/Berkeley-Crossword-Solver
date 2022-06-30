//  
// $Id: NFA.h 4143 2008-12-08 23:23:55Z abehm $
//
// NFA.h: interface for the NFA class.
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

#ifndef _NFA_
#define _NFA_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NFANode.h"
#include "NFATransition.h"
#include "parameters.h"
#include "Trie.h"

class Trie;
struct TrieNode;

class NFA  
{
public:
	
	/**The number of states in the NFA.*/
	int Q;

	/**Epsilon constant.*/
	

    /**The collection of nodes.*/
	vector<NFANode*> nodes;

	/**The final state nodes. (FI)*/
	NFANode* finalStateNode;

    /**The initial state nodes. (CIDA)*/
    NFANode* initStateNode;

    /**Flag denoting whether node numbers have been assigned (used in reset).*/
    bool nodeNumbersAssigned;


	NFA();
	virtual ~NFA();

	void assignNodeNumbers ();
	NFA* clone();
	void insertHead(NFANode* newInitNode);
	void insertHead(char character);
	void printMe();
	void prepareForGraph();
	NFANode* getNode(int index);
	int nodeNum();
	void ConvertFromTrie(Trie* trie);
	void ConvertTrie(NFANode* parent, TrieNode* node, NFANode* finalstate, int level, Trie* trie);
	void insertTo(NFANode* node, int level);


	// add a horizontal edge
	static void NFA::addHoriEdge(NFANode* fromNode, NFANode* toNode)
	{
		NFATransition* edge = new NFATransition(toNode, fromNode);
		fromNode->branches.push_back(edge);
		toNode->backBranches.push_back(edge);
	}
		
	// add a vertical edge
	static void NFA::addVetiEdge(NFANode* fromNode, NFANode* toNode)
	{
		NFATransition* edge = new NFATransition(toNode, fromNode);
		fromNode->downBranches.push_back(edge);
		toNode->upBranches.push_back(edge);
	}

};

#endif 

