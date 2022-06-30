//  
// $Id: NFANode.cpp 5027 2010-02-18 19:41:48Z rares $
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
//          Liang Jin <liangj (at) ics.uci.edu>
//

#include "stdafx.h"
#include "NFANode.h"
#include <string>
#include <iostream>
#include "parameters.h"
//#include <vector>

NFANode::NFANode (char name, bool finalState, bool initState)
{
	this->name = name;
	this->finalState = finalState;
    this->initState = initState;
	//branches = new vector();
    //backBranches = new vector();
    //downBranches = new vector();
    //upBranches = new vector();
}

NFANode::NFANode (char name, bool finalState, bool initState, int level)
{
	this->name = name;
	this->finalState = finalState;
    this->initState = initState;
	this->level = level;
	//branches = new vector();
    //backBranches = new vector();
    //downBranches = new vector();
    //upBranches = new vector();
}


NFANode::~NFANode ()
{
	int i;
	for(i = 0; i < this->branches.size(); i++)
		delete (NFATransition*)branches[i];
	
	//for(i = 0; i < this->backBranches.size(); i++)
	//	delete (NFATransition*)backBranches[i];

	for(i = 0; i < this->upBranches.size(); i++)
		delete (NFATransition*)upBranches[i];

	//for(i = 0; i < this->downBranches.size(); i++)
	//	delete (NFATransition*)downBranches[i];
}

void NFANode::printMe()
{
	int i;

    cout << "**********Node Info********************" << endl;
    cout << "number\t\t" << nodeNumber << "\t";
    if ( initState){
        cout << "init state\t";
    }
    if (finalState){
        cout << "final state\t";
    }
    cout << endl;
    cout << "name\t\t"+name << endl;

    cout << "to edge:\t\t" << endl;
    for(i=0; i<branches.size(); i++){
		((NFATransition*)branches[i])->printMe(TO);
    }

    cout << "back edge:\t\t" << endl;
    for(i=0; i<backBranches.size(); i++){

        ((NFATransition*)backBranches[i])->printMe(BACK);
    }
    cout << endl;
}


void NFANode::printMeAll()
{
	int i;
    cout << "**********Node Info********************" << endl;
    cout << "number\t\t" << nodeNumber << "\t";
    if ( initState){
		cout << "init state\t";
    }
    if (finalState){
        cout << "final state\t";
    }
    cout << endl;
    cout << "name\t\t"+name << endl;

    cout << "to edge:\t\t" << endl;
    for(i=0; i<branches.size(); i++){
        ((NFATransition*)branches[i])->printMe(TO);
    }

    cout << "back edge:\t\t" << endl;
    for(i=0; i<backBranches.size(); i++){

        ((NFATransition*)backBranches[i])->printMe(BACK);
    }

    cout << "down edge:\t\t" << endl;
    for(i=0; i<downBranches.size(); i++){
        ((NFATransition*)downBranches[i])->printMe(DOWN);
    }

    cout << "up edge:\t\t" << endl;
    for(i=0; i<upBranches.size(); i++){
        ((NFATransition*)upBranches[i])->printMe(UP);
    }
    cout << endl;
}
