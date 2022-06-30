//  
// $Id: NFA.cpp 5027 2010-02-18 19:41:48Z rares $
//
// NFA.cpp: implementation of the NFA class.
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
#include "NFA.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

NFA::NFA()
{
	this->Q = 0;
	nodeNumbersAssigned = false;
}

NFA::~NFA()
{
	for(int i = 0; i < nodes.size(); i++)
		delete (NFANode*)nodes[i];
}

void NFA::assignNodeNumbers ()
{
		//Assigning node numbers.
	for (int index = 0; index < nodes.size(); index++)
	{
		NFANode* node = (NFANode*) nodes[index];
		node->nodeNumber = index;
	}
}


/**
	 * Performs a deep copy of the automaton.
	 *
	 * @return a deep copy of the automaton.
	 */
NFA* NFA::clone()
{
    //test("clone()");
	assignNodeNumbers ();

	//Copying nodes.
	vector<NFANode*> newNodes;
	NFANode* newFinalStateNode;// = new NFANode(EPSILON, true, false);
    NFANode* newInitStateNode;// = new NFANode(EPSILON, false, true);

	int index, subindex;

	for (index = 0; index < nodes.size (); index++)
	{
		NFANode* node = (NFANode*) nodes[index];
		NFANode* newNode = new NFANode (node->name, node->finalState, node->initState);
        newNode->nodeNumber = node->nodeNumber;
		newNode->level = node->level;
		newNodes.push_back (newNode);
		if (node->finalState)
	        newFinalStateNode = newNode;
        if (node->initState)
            newInitStateNode = newNode;

	}

	//Copying transitions.
	for (index = 0; index < nodes.size (); index++)
	{
		NFANode* node = (NFANode*) nodes[index];
		NFANode* newNode = (NFANode*) newNodes[index];
		int node_branches_size = node->branches.size ();
		for (subindex = 0; subindex < node_branches_size; subindex++)
		{
			NFATransition* transition = (NFATransition*) node->branches[subindex];

            NFATransition* edge = new NFATransition ((NFANode*) newNodes[transition->outNode->nodeNumber], newNode);
			newNode->branches.push_back (edge);
            ((NFANode*) newNodes[transition->outNode->nodeNumber])->backBranches.push_back(edge);
		}


        node_branches_size = node->downBranches.size ();
		for (subindex = 0; subindex < node_branches_size; subindex++)
		{
			NFATransition* transition = (NFATransition*) node->downBranches[subindex];

			NFATransition* edge = new NFATransition ((NFANode*) newNodes[transition->outNode->nodeNumber] , newNode);
			newNode->downBranches.push_back (edge);
            ((NFANode*) newNodes[transition->outNode->nodeNumber])->upBranches.push_back(edge);

		}
	}

                //Creating new NFA.
	NFA* output = new NFA ();
	output->Q = Q;
	output->nodes = newNodes;
	output->finalStateNode = newFinalStateNode;
    output->initStateNode = newInitStateNode;
	//System.arraycopy (specialInAlphabet, 0, output.specialInAlphabet, 0, specialInAlphabet.length);

	return output;
}

void NFA::insertHead(NFANode* newInitNode){

    initStateNode->initState = false;
    NFATransition* edge = new NFATransition(initStateNode,newInitNode );
    newInitNode->branches.push_back(edge);
    initStateNode->backBranches.push_back(edge);

    initStateNode = newInitNode;
    nodes.insert(nodes.begin(), newInitNode);
    Q++;
	//no recalculation of the number?
}

void NFA::insertHead(char character){
    NFANode* newInitNode = new NFANode(character, false, true, 0);
    initStateNode->initState = false;

    NFATransition* edge = new NFATransition(initStateNode,newInitNode );
    newInitNode->branches.push_back(edge);
    initStateNode->backBranches.push_back(edge);

    initStateNode = newInitNode;
    nodes.insert(nodes.begin(), newInitNode);
    Q++;
}

void NFA::printMe(){
    assignNodeNumbers();
    for (int i=0; i<nodes.size(); i++){
		((NFANode*)nodes[i])->printMeAll();
    }
}



//  prepare before we build the graph
//  add a epsilon before all the node
//  then reorder the node number
void NFA::prepareForGraph()
{
	assignNodeNumbers();
    //this->insertHead(EPSILON);
	//assignNodeNumbers();
}

NFANode* NFA::getNode(int index)
{
    return (NFANode*)nodes[index];
}

int NFA::nodeNum()
{
	return nodes.size();
}
        

void NFA::ConvertFromTrie(Trie* trie)
{	
	int i, j, sIndex;

	NFANode* initstate = new NFANode(EPSILON, false, true, 1);
	nodes.push_back(initstate);
	initStateNode = initstate;
	Q++;
	
	NFANode* finalstate = new NFANode(EPSILON, true, false);

	TrieNode* trienode;

	NFANode* tempnode[ALPH_SIZE];
	for(i = 0; i < ALPH_SIZE; i++)
		tempnode[i] = NULL;

	int levels[ALPH_SIZE];

	char name;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(trie->root->child[i] != NULL)
		{
			trienode = trie->root->child[i];

			if(trienode->isCompressed == false)		//not a compressed node
			{
				name = trie->getSingleName(trienode->names);

				if(name != EPSILON)	//if it's not EPSILON
					levels[i] = 2;
				else							//if it's EPSILON, dont increase level
					levels[i] = 1;

				NFANode* newnode = new NFANode(name, false, false, levels[i]);
				
				NFATransition* edge = new NFATransition(newnode, initstate);

				newnode->backBranches.push_back(edge);
				initstate->branches.push_back(edge);

				//nodes.push_back(newnode);
				insertTo(newnode, levels[i]);
				Q++;

				tempnode[i] = newnode;
					

				if(trienode->count == 0) //if it's the leaf node, add a link to the final state
				{
					newnode->finalState = true;

					//NFATransition *newedge = new NFATransition(finalstate, newnode);

					//newnode->branches.push_back(newedge);
					//finalstate->backBranches.push_back(newedge);

					tempnode[i] = NULL;
				}				
								
			}
			else	//compressed node
			{

				NFANode* newparent = initstate;

				levels[i] = 2;

				for(j = 0; j < trienode->com.depth; j++)
				{
					
					//set the new node as final state
					NFANode* newnode3 = new NFANode(EPSILON, true, false, levels[i]);
					
					
					for(sIndex = 0; sIndex < ALPH_SIZE; sIndex++)
					{
						if(trie->getNameExist(trienode->names, sIndex) == true)
						{
							NFANode* newnode2 = new NFANode(trie->getCharFromIndex(sIndex), false, false, levels[i]);

							NFATransition* edge1 = new NFATransition(newnode2, newparent);
							newparent->branches.push_back(edge1);
							newnode2->backBranches.push_back(edge1);

							NFATransition* edge2 = new NFATransition(newnode3, newnode2);
							newnode2->branches.push_back(edge2);
							newnode3->backBranches.push_back(edge2);

							//nodes.push_back(newnode2);
							insertTo(newnode2, levels[i]);
							Q++;
						}
					}

					//nodes.push_back(newnode3);
					insertTo(newnode3, levels[i]);
					Q++;
					
					//add an edge to the final accepting state
					//NFATransition *newedge = new NFATransition(finalstate, newnode3);
					//newnode3->branches.push_back(newedge);
					//finalstate->backBranches.push_back(newedge);
					
					newparent= newnode3;
					levels[i]++;
				}
				
				tempnode[i] = newparent;
				levels[i]--;

				if(trienode->count == 0) //if it's the leaf node, add a link to the final state
				{
					tempnode[i] = NULL;
				}
			}
		}
	}
	
	for(i = 0; i < ALPH_SIZE; i++)
		if(trie->root->child[i] != NULL && tempnode[i] != NULL)
			ConvertTrie(tempnode[i], trie->root->child[i], finalstate, levels[i], trie);

	//finalstate->level = 100000;
	//nodes.push_back(finalstate);
	finalStateNode = finalstate;
	//Q++;

	return;

}

void NFA::ConvertTrie(NFANode* parent, TrieNode* node, NFANode* finalstate, int level, Trie* trie)
{
	int i, j, sIndex;

	TrieNode* datanode;

	NFANode* parents[ALPH_SIZE];
	for(i = 0; i < ALPH_SIZE; i++)
		parents[i] = NULL;

	int levels[ALPH_SIZE];

	char name;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(node->child[i] != NULL)
		{
			datanode = node->child[i];

			if(datanode->isCompressed == false)		//not a compressed node
			{
				name = trie->getSingleName(datanode->names);

				if(name != EPSILON)
					levels[i] = level + 1;
				else
					levels[i] = level;
				
				NFANode* newnode = new NFANode(name, false, false, levels[i]);
					
				NFATransition* edge = new NFATransition(newnode, parent);

				newnode->backBranches.push_back(edge);
				parent->branches.push_back(edge);

				//nodes.push_back(newnode);
				insertTo(newnode, levels[i]);
				Q++;

				parents[i] = newnode;

				if(datanode->count == 0) //if it's the leaf node, add a link to the final state
				{
					//NFATransition *newedge = new NFATransition(finalstate, newnode);

					//newnode->branches.push_back(newedge);
					//finalstate->backBranches.push_back(newedge);
					newnode->finalState = true;
					
					parents[i] = NULL;
				}
				
			}
			else	//compressed node
			{
				//NFANode* newnode1 = new NFANode(EPSILON, true, false, level);
				//NFATransition* edge4 = new NFATransition(newnode1, parent);
				//parent->branches.push_back(edge4);
				//newnode1->backBranches.push_back(edge4);

				NFANode* newparent = parent;
				//NFANode* newparent = newnode1;

				levels[i] = level + 1;

				for(j = 0; j < datanode->com.depth; j++)
				{
					NFANode* newnode3 = new NFANode(EPSILON, true, false, levels[i]);
					
					for(sIndex = 0; sIndex < ALPH_SIZE; sIndex++)
					{
						if(trie->getNameExist(datanode->names, sIndex) == true)
						{
							NFANode* newnode2 = new NFANode(trie->getCharFromIndex(sIndex), false, false, levels[i]);

							NFATransition* edge1 = new NFATransition(newnode2, newparent);
							newparent->branches.push_back(edge1);
							newnode2->backBranches.push_back(edge1);

							NFATransition* edge2 = new NFATransition(newnode3, newnode2);
							newnode2->branches.push_back(edge2);
							newnode3->backBranches.push_back(edge2);

							//nodes.push_back(newnode2);
							insertTo(newnode2, levels[i]);
							Q++;
						}
					}

					//nodes.push_back(newnode3);
					insertTo(newnode3, levels[i]);
					Q++;
					
					//add an edge to the final accepting state
					//NFATransition *newedge = new NFATransition(finalstate, newnode3);
					//newnode3->branches.push_back(newedge);
					//finalstate->backBranches.push_back(newedge);
					
					newparent = newnode3;
					levels[i]++;
				}

				parents[i] = newparent;
				levels[i]--;

				if(datanode->count == 0)
				{
					parents[i] = NULL;
				}
				
			}
		}
	}

	for( i = 0; i < ALPH_SIZE; i++)
	{
		if(node->child[i] != NULL && parents[i] != NULL)
		{
			ConvertTrie(parents[i], node->child[i], finalstate, levels[i], trie);
		}
	}

	return;
}

void NFA::insertTo(NFANode *node, int level)
{
	int i, size;
	size = nodes.size();

	vector<NFANode*>::iterator it;
	it = nodes.begin();
	
	for(i = 0; i < size; i++)
	{
		if( ((NFANode*)nodes[i])->level <= level)
			it++;
		else
			break;
	}
		
	nodes.insert(it, 1, node);
	
	return;
}
