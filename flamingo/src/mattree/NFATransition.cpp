//
// NFATransition.cpp: implementation of the NFATransition class.
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
#include "NFATransition.h"
#include <string>
#include <iostream>
#include "parameters.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


NFATransition::NFATransition (char label[], NFANode* outNode)
{
	//this.labels = new char[2];
    this->labels[0] = label[0];
    this->labels[1] = label[1];
	this->outNode = outNode;
}

NFATransition::NFATransition (char label[], NFANode* outNode, NFANode* fromNode)
{
    //this.labels = new char[2];
    this->labels[0] = label[0];
    this->labels[1] = label[1];
	this->outNode = outNode;
    this->fromNode = fromNode;
}
       
NFATransition::NFATransition (NFANode* toNode, NFANode* fromNode)
{
	//this.labels = new char[2];
	this->outNode = toNode;
    this->fromNode = fromNode;
}

NFATransition::~NFATransition()
{

}


void NFATransition::setLabels(char labels[])
{
    this->labels[0] = labels[0];
    this->labels[1] = labels[1];
}


char* NFATransition::getLabels()
{
    return labels;
}


void NFATransition::printMe(int type){
    string part1;
    string part2;
    if(type==TO){
		part1 = fromNode->nodeNumber + " --[";
        part2 = "]-> "+outNode->nodeNumber;
    }
    else if(type==BACK){
        part1 = outNode->nodeNumber+" <--[";
        part2 = "]-- "+fromNode->nodeNumber;
    }
    else if(type == DOWN){
        part1 = fromNode->nodeNumber+" \\--[";
        part2 = "]--> "+outNode->nodeNumber;
    }
    else if(type == UP){
        part1 = outNode->nodeNumber+" <--[";
        part2 = "]--\\ "+fromNode->nodeNumber;
    }
    cout << part1;
    if(labels[0]==EPSILON){
        cout << "& ";
    }
    else{
        cout << labels[0]+" ";
    }

    if(labels[1]==EPSILON){
        cout << "&";
    }
    else{
        cout << labels[1];
    }
    cout << part2;

}
