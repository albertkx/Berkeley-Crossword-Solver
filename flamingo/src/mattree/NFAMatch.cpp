//  
// $Id: NFAMatch.cpp 5027 2010-02-18 19:41:48Z rares $
//
// NFAMatch.cpp: implementation of the NFAMatch class.
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
#include "NFAMatch.h"
#include <iostream>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NFAMatch::~NFAMatch()
{
	//for(int i=0; i< size; i++)
	//	delete graph[i];

	//delete[] graph;
}

NFAMatch::NFAMatch(NFA* automaton) {
	this->automaton = automaton;
}
    


// return the edit distance between the RE and the string
int NFAMatch::match(string str){
	int score = 0;
    buildGraph(str);
    score = dynamicProg(str);
    //printMatr(str);
    return score;
}

bool NFAMatch::match(string str, int delta)
{
	bool score;
    buildGraph(str);
    score = dynamicProg(str, delta);
    //printMatr(str);
    return score;
}

void NFAMatch::buildGraph(string str){
	//graph = new NFA*[str.length()+1];
    automaton->prepareForGraph();
    //for(int i=0; i<=str.length(); i++){
	//	graph[i] = automaton->clone();
    //}
	this->size = str.length() + 1;

    //labelStepOne(str);
    //labelStepTwo(str);
    //labelStepThree(str);
    return ;
}

// if i=[1, M] and either s=0 or Y(s)!=epsilon
// There is a deletion edge (i-1, s) -> (i, s)
// with label [ai, epsilon]
void NFAMatch::labelStepOne(string str){
	NFANode* node;
    char labels[2];
    for(int i=1; i<=str.length(); i++){
		for(int s=0; s<automaton->nodeNum();s++){
			node = automaton->getNode(s);
            if(node->initState == true || node->name != EPSILON){
				NFATransition* edge = new NFATransition(getNode(i,s), getNode(i-1, s));

                // [ai, epsilon]
                labels[0] = str.at(i-1);
                labels[1] = EPSILON;
                edge->setLabels(labels);

                getNode(i-1, s)->downBranches.push_back(edge);
                getNode(i,s)->upBranches.push_back(edge);
            }
        }
    }
}

// if i=[0,M] and t->s = E
// then there is an insertion edge (i,t)->(i,s)
// with label [epsilon, Y(s)]
void NFAMatch::labelStepTwo(string str){
	NFANode *t, *s;
    NFATransition* edge;
    char labels[2];
    for(int i=0; i<=str.length(); i++){
		for(int nodeIndex=0; nodeIndex<automaton->nodeNum();nodeIndex++){
			t = getNode(i, nodeIndex);
			for(int edgeNum=0; edgeNum<t->branches.size(); edgeNum++){
				edge = (NFATransition*)t->branches[edgeNum];
                s = edge->outNode;
                labels[0] = EPSILON;
                labels[1] = s->name;
                edge->setLabels(labels);
            }
        }
    }
}

// if i=[1,M], t->s = E, and Y(s)!=epsilon,
// then there is a substitution edge (i-1,t)->(i,s)
// with label [ai, Y(s)]
void NFAMatch::labelStepThree(string str){
	NFANode *t, *s;
    NFATransition *edge, *newEdge;
    int tIndex, sIndex;
    char labels[2];
    for(int i=1; i<=str.length(); i++){
		for(tIndex=0; tIndex<automaton->nodeNum(); tIndex++){
			t = automaton->getNode(tIndex);
            for(int edgeNum=0; edgeNum<t->branches.size(); edgeNum++){
				edge = (NFATransition*)t->branches[edgeNum];
                s = edge->outNode;
                if (s->name != EPSILON){
					sIndex = s->nodeNumber;
                    newEdge = new NFATransition(getNode(i,sIndex), getNode(i-1, tIndex));
                    labels[0] = str.at(i-1);
                    labels[1] = s->name;
                    newEdge->setLabels(labels);
                    getNode(i-1, tIndex)->downBranches.push_back(newEdge);
                    getNode(i,sIndex)->upBranches.push_back(newEdge);
                }
            }
        }
	}
}


int NFAMatch::dynamicProg(string str){
	int M=str.length();
    int N=automaton->nodeNum();
	int i;

	C = new int*[M+1];

	for(i = 0; i < M+1; i++)
		C[i] = new int[N];

	//C = new int[M+1][N];
    resetC(M, N, MINIMUM);
    int sIndex;
    //NFATransition* edge;
    NFANode* node;


    C[0][0] = 0;
    
	for(sIndex=1; sIndex < M+1; sIndex++){
		C[sIndex][0] = C[sIndex-1][0] + scoreLabel(((NFATransition*)getNode(sIndex,0)->upBranches[0])->labels);
    }

    for(i=1; i<N; i++){
		C[0][i] = max(0, getNode(0,i)->backBranches);
        for(sIndex=1; sIndex < M+1; sIndex++){
			node = getNode(sIndex,i);
            C[sIndex][i] = max(sIndex, node->backBranches);
            if(node->name != EPSILON){

				int  max1 = max(sIndex-1, node->upBranches);
                int  max2 = C[sIndex-1][i] + scoreLabel(str.at(sIndex-1), EPSILON);
                C[sIndex][i] = maxOfThree(C[sIndex][i], max1, max2);
            }
        }       
	}

	int result = C[M][N-1];

	for(i = 0; i < M+1; i++)
		delete[] C[i];

	delete[] C;

    return result;
}

bool NFAMatch::dynamicProg(string str, int threshold){
	int M=str.length();
    int N=automaton->nodeNum();
	int i, j;

    C = new int*[M+1];

	for(i = 0; i < M+1; i++)
		C[i] = new int[N];

	
    resetC(M, N, MINIMUM);
    int sIndex;
    
    NFANode* node;

	//int delta = (-1) * threshold;
	int delta = threshold;
	int currentMax = MINIMUM;

	C[0][0] = 0;
	currentMax = 0;
  
	for(sIndex=1; sIndex < M+1; sIndex++){
		C[sIndex][0] = C[sIndex-1][0] + scoreLabel(str.at(sIndex-1), EPSILON);

		if(C[sIndex][0] > currentMax)
			currentMax = C[sIndex][0];
    }

	if(abs(currentMax) > delta)
	{
		for(i = 0; i < M+1; i++)
			delete[] C[i];

		delete[] C;

		return false;
	}

	i = 1;
    while(i < N){
		currentMax = MINIMUM;
		do
		{
			C[0][i] = InsertionMax(0, automaton->getNode(i));

			if(C[0][i] > currentMax)
				currentMax = C[0][i];

			for(sIndex=1; sIndex < M+1; sIndex++){
				node = automaton->getNode(i);
				C[sIndex][i] = InsertionMax(sIndex, node);//insertion
				if(node->name != EPSILON){

					int  max1 = SubstitutionMax(sIndex, node, str);//substitution

					int  max2 = C[sIndex-1][i] + scoreLabel(str.at(sIndex-1), EPSILON);//deletion
					C[sIndex][i] = maxOfThree(C[sIndex][i], max1, max2);
				}

				if(C[sIndex][i] > currentMax)
					currentMax = C[sIndex][i];
			}

			if(automaton->getNode(i)->finalState == true)
				if(abs(C[M][i]) <= delta)
				{
					for(j = 0; j < M+1; j++)
						delete[] C[j];

					delete[] C;
					
					return true;
				}
			i++;

		}
		while(i < N && automaton->getNode(i)->level == automaton->getNode(i-1)->level);

		if(abs(currentMax) > delta)
		{
			for(j = 0; j < M+1; j++)
				delete[] C[j];

			delete[] C;

			return false;
		}
	}

	int result = C[M][N-1];

	for(i = 0; i < M+1; i++)
		delete[] C[i];

	delete[] C;

	/*if(result >= delta) 
		return true;
	else
		return false;    */
	return false;
}



NFANode* NFAMatch::getNode(int i, int s){
	//return graph[i]->getNode(s);
	return automaton->getNode(s);
}

void NFAMatch::printGraph(int i){
    //graph[i]->printMe();
}

void NFAMatch::resetC(int M, int N, int num){
    for(int i=0; i<M+1; i++){
		for(int j=0; j<N; j++){
			C[i][j] = num;
        }
    }
    return ;
}

//  for all nodeIndex as fromNode in the edgeArr
//  line i, the max value that {C[i][nodeIndex] + score(edge.label)
int NFAMatch::max(int i,  vector<NFATransition*> edgeArr){
    NFATransition* edge;
    int tIndex;
    int max = MINIMUM;
    for(int edgeNum=0; edgeNum<edgeArr.size(); edgeNum++){
		edge = (NFATransition*)edgeArr[edgeNum];
        tIndex = edge->fromNode->nodeNumber;
        //  only consider edge in topological order
        //  since we set all unexamed node score to be MINIMUM
        //  then it is of no problem
        if(true){
			if(C[i][tIndex] + scoreLabel(edge->labels) > max){
				max = C[i][tIndex] + scoreLabel(edge->labels);
            }
        }
    }
    return max;
}

int NFAMatch::InsertionMax(int i, NFANode* node)
{
	NFATransition* edge;
	int tIndex;
	int max = MINIMUM;

	char labels[2];
	labels[0] = EPSILON;
	labels[1] = node->name;

	for(int edgeNum = 0; edgeNum < node->backBranches.size(); edgeNum++)
	{
		edge = (NFATransition*)node->backBranches[edgeNum];
		tIndex = edge->fromNode->nodeNumber;

		if(C[i][tIndex] + scoreLabel(labels) > max)
		{
			max = C[i][tIndex] + scoreLabel(labels);
		}
	}
	return max;
}

int NFAMatch::SubstitutionMax(int i, NFANode* node, string str)
{
	NFATransition* edge;
	int tIndex;
	int max = MINIMUM;

	char labels[2];
	labels[0] = str.at(i-1);
	labels[1] = node->name;

	for(int edgeNum = 0; edgeNum < node->backBranches.size(); edgeNum++)
	{
		edge = (NFATransition*)node->backBranches[edgeNum];
		tIndex = edge->fromNode->nodeNumber;

		if(C[i - 1][tIndex] + scoreLabel(labels) > max)
		{
			max = C[i - 1][tIndex] + scoreLabel(labels);
		}
	}
	return max;
}

//  this is a user-speficed scoring function for the label
//  [epsilon, a] insertion  -------       -1
//  [a, epsilon] deletion   -------       -1
//  [a, b]       substitution -----       -1
//  [a, a]  [epsilon, epsilon] ----       0
int NFAMatch::scoreLabel(char labels[]){
    if(labels[0] == labels[1]){
		return 0;
    }
    else{
		return -1;
    }
}


int NFAMatch::scoreLabel(char label0, char label1){
    if(label0 == label1){
		return 0;
    }
    else{
		return -1;
    }
}


int NFAMatch::maxOfThree(int num1, int num2, int num3){
    if(num1>=num2){
		if(num1>=num3){
			return num1;
        }
		else {
			return num3;
		}
    }
    else{
		if(num2>=num3){
			return num2;
        }
        else{
			return num3;
        }
    }
}


void NFAMatch::printMatr(string str){
    int M=str.length();
    int N=automaton->nodeNum();
	for(int i=0; i<=M; i++){
		for(int j=0; j<N; j++){
			cout << C[i][j] << "\t";
		}
		cout << endl;
	}
	cout << endl;
	cout << endl;
}

/*static void main (String [] args) throws Exception
{
    if(args.length <2){
		System.out.println("Usage: java NFAMatch RE String");
        System.exit(-1);
    }
    String reStr = args[0];
    String str = args[1];
    System.out.println("The RE is "+reStr);
    System.out.println("The string is "+str);
    RE testRE1 = new RE(reStr, true);
    NFA nfa= testRE1.getAutomaton();
    //nfa.printMe();
    NFAMatch nm = new NFAMatch(nfa);
    System.out.println(nm.match(str));
}*/
