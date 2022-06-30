//  
// $Id: Trie.cpp 5027 2010-02-18 19:41:48Z rares $
//
// Trie.cpp: implementation of the Trie class.
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
#include <string>
#include <vector>
//#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <iostream.h>
//#include "parameters.h"
#include <math.h>
#include <time.h>
#include "NFA.h"
#include "NFAMatch.h"

using namespace std;


#include "Trie.h"

extern double totalEDTime;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Trie::Trie()
{
	root = TrieNewNode();
	//TrieInitNode(root);

	setName(root->names, EPSILON);

//	memset(content, ' ', TRIELEN);
}

Trie::~Trie()
{
	TrieFreeNode(root);
}

struct TrieNode * Trie::TrieNewNode()
{
	register struct TrieNode *n;

	//n = new Node;
	n = (struct TrieNode*)malloc(sizeof(struct TrieNode));
	assert(n);
	TrieInitNode(n);
	return n;
}

void Trie::TrieInitNode(struct TrieNode *N)
{
	register struct TrieNode *n = N;
	register int i;

	n->count = 0;

	n->isCompressed = false;

	n->names = 0;

	n->com.size = 0;
	n->com.depth = 0;

	for (i = 0; i < ALPH_SIZE; i++)
		n->child[i] = NULL;

	n->parent = NULL;
	
}

void Trie::TrieFreeNode(struct TrieNode *p)
{
	int i;
	assert(p);
	//delete p;
	if(p->count == 0)
	{
		p->parent = NULL;
		free(p);
	}
	else
	{
		for(i = 0; i < ALPH_SIZE; i++)
		{
			if(p->child[i] != NULL)
			{
				TrieFreeNode(p->child[i]);
				p->child[i] = NULL;
			}
		}
		free(p);
	}
	return;
}

void Trie::TrieClearTrie()
{
	TrieFreeNode(root);

	root = TrieNewNode();
	setName(root->names, EPSILON);
}

struct TrieNode* Trie::CopyTrie(struct TrieNode* p, struct TrieNode* parent)
{
	int i;

	struct TrieNode* newnode = TrieNewNode();
	newnode->names = p->names;
	newnode->isCompressed = p->isCompressed;
	newnode->count = p->count;
	newnode->depth = p->depth;
	newnode->com.depth = p->com.depth;
	newnode->com.size = p->com.size;
	newnode->parent = parent;

	if(newnode->count == 0)
		return newnode;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(p->child[i] != NULL)
			newnode->child[i] = CopyTrie(p->child[i], newnode);
	}
	return newnode;

}


string Trie::Trie2Str(struct TrieNode *root)
{
	string result;
	oneNode2Str(root, result);
	return result;

}

void Trie::oneNode2Str(struct TrieNode *node, string &result){
	int i;
	char name;
	if(node->count==0){
		if(node->isCompressed == false)
		{
			name = getSingleName(node->names);
			result.append(&name, 1);
		}
		else
		{
			result.append("[");
			
			char alpha[10];
			sprintf(alpha, "%d", node->names);
			result.append(alpha);


			result.append(",");

			char temp[2];
			//itoa(node->com.depth, temp, 10);
			sprintf(temp, "%d", node->com.depth);

			result.append(temp);

			result.append("]");
		}
		return;
	}
	
	else{
		//result.append(&node->name, 1);
		if(node->isCompressed == false)
		{
			name = getSingleName(node->names);
			result.append(&name, 1);
		}
		else
		{
			result.append("[");
			
			char alpha[10];
			sprintf(alpha, "%d", node->names);
			result.append(alpha);

			result.append(",");

			char temp[2];
			//itoa(node->com.depth, temp, 10);
			sprintf(temp, "%d", node->com.depth);

			result.append(temp);

			result.append("]");
		}

		if(node->count!=1)
			result.append("(");
		int counter = 0;
		for(i=0; i<ALPH_SIZE; i++){
			//append(result, node->name);
			if(node->child[i] != NULL)
			{
				oneNode2Str(node->child[i], result);

				if(counter != node->count - 1){
					result.append("|");
				}

				counter++;
			}
		}
		if(node->count!=1)
			result.append(")");
	}
	return;
}


struct TrieNode* Trie::Str2Trie(string input)
{
	struct TrieNode* n;
	n = TrieNewNode();
	//TrieInitNode(n);
	setName(n->names, EPSILON);

	input.erase(0,1); //remove the & at the beginning it's a epsilon state
	Str2oneNode(n, input);

	return n;
}

void Trie::Str2oneNode(struct TrieNode *node, string input)
{
	int pos = input.find_first_of('(');
	int i, j, k, subIndex;
	
	struct TrieNode *temp = node;
	int index;
	
	//No ()
	if(pos == -1) //there is no (), it's a single path
	{
		for(i=0; i<input.length(); i++)
		{
			struct TrieNode* n = TrieNewNode();
			//TrieInitNode(n);

			if(input[i] != '[')
			{
				setName(n->names, input[i]);
				index = getIndex(input[i]);
				temp->child[index] = n;

				n->parent = temp;
			
				temp->count++;

				temp = temp->child[index];
			}
			else
			{
				j = i+1;
				
				while(input[j] != ']')
				{
					j++;
				}

				char* substring = new char[j - i];
				
				for(k = 0; k < j-i; k++)
				{
					substring[k] = input[i + k + 1];
				}
				substring[j-i-1] = '\0';

				n->isCompressed = true;

				char* name = new char[j - i];
				
				for(k = 0; k < j-i-1; k++)
				{
					if(substring[k] != ',')
					{
						name[k] = substring[k];
						//setName(n->names, substring[k]);
						
						//n->com.size++;
					}
					else
						break;
				}
				name[k] = '\0';

				n->names = atoi(name);
				for(subIndex = 0; subIndex < ALPH_SIZE; subIndex ++)
				{
					if(getNameExist(n->names, subIndex) == true)
						n->com.size++;
				}
				
				n->com.depth = atoi(substring + k + 1);

				temp->child[ALPH_SIZE - 1] = n; //38 for compressed node
				n->parent = temp;

				temp->count++;

				temp = temp->child[ALPH_SIZE - 1];

				i = j;  //skip the part between [ ]

				delete[] substring;
				delete[] name;
			}
		}

		return;
	}

	//There is ()
	if(pos > 0) //there are chars that are in a single path from the parent
	{
		for(i=0; i<pos; i++)
		{
			struct TrieNode* n = TrieNewNode();
			//TrieInitNode(n);
			
			if(input[i] != '[')
			{
				setName(n->names, input[i]);
				index = getIndex(input[i]);
				temp->child[index] = n;
				n->parent = temp;
			
				temp->count++;

				temp = temp->child[index];
			}
			else
			{
				j = i+1;
				
				while(input[j] != ']')
				{
					j++;
				}

				char* substring = new char[j - i];
				
				for(k = 0; k < j-i; k++)
				{
					substring[k] = input[i + k + 1];
				}
				substring[j-i-1] = '\0';

				n->isCompressed = true;
				
				char* name = new char[j-i];

				for(k = 0; k < j-i-1; k++)
				{
					if(substring[k] != ',')
					{
						name[k] = substring[k];
						//setName(n->names, substring[k]);

						//n->com.size++;
					}
					else
						break;
				}
				name[k] = '\0';

				n->names = atoi(name);
				for(subIndex = 0; subIndex < ALPH_SIZE; subIndex ++)
				{
					if(getNameExist(n->names, subIndex) == true)
						n->com.size++;
				}
				
				n->com.depth = atoi(substring + k + 1);

				temp->child[ALPH_SIZE - 1] = n; //ALPH_SIZE - 1 for compressed node
				n->parent = temp;

				temp->count++;

				temp = temp->child[ALPH_SIZE - 1];

				i = j;  //skip the part between [ ]

				delete[] substring;
				delete[] name;
			}

		}
		
		input.erase(0,pos); //erase everything before the first (, including the (
	}

	input.erase(0, 1);

	if(input.length() == 0) return;

	int leftcount = 0;
	int cursor = 0;

	for(i = 0; i < input.length(); i++)
	{
		if(input[i] == '|' && leftcount == 0)
		{
			string input2 = input.substr(cursor, i - cursor);
			Str2oneNode(temp, input2);
			cursor = i+1;
		}
		else if(input[i] == '(')
		{
			leftcount ++;
		}
		else if(input[i] == ')' && leftcount > 0)
		{
			leftcount --;
		}
		else if(input[i] == ')' && leftcount == 0) //last branch here
		{
			string input3 = input.substr(cursor, i - cursor);
			Str2oneNode(temp, input3);
			break;
		}
	}
	assert(i == input.length() - 1);

	return;

}

void Trie::AddToTrie(string s)
{
	int lcv, index, character;
	s.append("&");
	 
    struct TrieNode *t = root;
     
 
    // loop over the length of the string to add and traverse the trie...
    for(lcv=0; lcv < s.length(); lcv++) {
 
		character = s[lcv]; // the character in s we are processing...
		index = getIndex(character);

 		// is there a child node for this character?
		if (t->child[index] == NULL) {
 
 			// if not, make one!
			t->child[index] = TrieNewNode();

			t->child[index]->parent = t;
			//TrieInitNode(t->child[index]);
            setName(t->child[index]->names, character);
			
			t->count++;
        }
 
        // there is another string under this node...
        //t->child[index]->count++;
 
        // move to it this node... and loop
        t = t->child[index];
    }
}

int Trie::getIndex(int character)
{
	int index;
	
	if(character >= 'a' && character <= 'z')
		index = character - 'a';
	else if(character == '~')
		index = 26;
	else if(character == '&')
		index = 27;
	else
		index = 28; //for compressed node

	return index;
}

char Trie::getCharFromIndex(int index)
{
	char character;
	
	if(index >= 'a'-'a' && index <= 'z'-'a')
		character = index + 'a';
	else if(index == 26)
		character = '~';
	else if(index == 27)
		character = '&';
	else
		character = '\0'; //for compressed node

	return character;
}

void Trie::MergeTries(struct TrieNode* t2)
{
	string insertstr;

	InsertStrs(t2, insertstr);
	
	return;
}

void Trie::InsertStrs(struct TrieNode* t2, string insert)
{
	int i;
	char name;
	if(t2->count == 0) 
	{
		insert.erase(insert.length() - 1, 1); //get rid of the last &, since the insert will append it

		AddToTrie(insert);
		return;
	}

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(t2->child[i] != NULL)
		{
			name = getSingleName(t2->child[i]->names);
			insert.append(&name, 1);
			InsertStrs(t2->child[i], insert);
			insert.erase(insert.length() - 1, 1);
		}

		
	}
}

long Trie::getSize(struct TrieNode *p)
{
	assert(p);
	string s;
	s = Trie2Str(p);

	return s.length();
}

double Trie::getNumofStr(struct TrieNode* p)
{
	assert(p);
	
	double counter = 0.0;

	getNum(p, counter);

	return counter;
}

bool Trie::getNum(struct TrieNode * p, double &counter)
{
	int i;

	double temp;

	if(p->count == 0) // leaf node
	{
		if(p->isCompressed == false) // the node is compressed
			counter ++;
		else //not compressed
		{
			temp = (double)pow(p->com.size, p->com.depth);
			if(temp >= 0.0)
				counter += (double)pow(p->com.size, p->com.depth);
		}
		return true;
	}
	else //interior node
	{
		for(i = 0; i < ALPH_SIZE; i++)
		{
			if(p->child[i] != NULL)
			{
				getNum(p->child[i], counter);
			}
		}
	}

	return true;

}

void Trie::assignDepth()
{
	assert(root);

	root->depth = assignOneDepth(root);
}

int Trie::assignOneDepth(struct TrieNode* p)
{
	if(p->count == 0) //leaf node
	{
		p->depth = 0;
		return 0;
	}
	int i;
	bool first=true;
	int depth, temp;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(p->child[i] != NULL)
		{
			if(first)
			{
				depth = assignOneDepth(p->child[i]) + 1;
				first = false;
			}
			else
			{
				temp = assignOneDepth(p->child[i]) + 1;
				if(temp > depth)
					depth = temp;
			}

		}
	}
	
	p->depth = depth;

	return depth;
}

int Trie::getAlpha(struct TrieNode *p)
{
	assert(p);

	int i;

	bool existing[ALPH_SIZE];

	for(i=0; i<ALPH_SIZE; i++)
		existing[i] = false;
	
	for(i=0; i<ALPH_SIZE; i++)
		if(p->child[i] != NULL)
			getAlphaHelper(p->child[i], existing);

	int counter = 0;
	for(i=0; i<ALPH_SIZE; i++)
		if(existing[i] == true)
			counter++;

	if(existing[27] == true) counter--; //don't count the $

	return counter;
}

bool* Trie::getAlpha(struct TrieNode *p, bool bo)
{
	assert(p);

	int i;

	bool* existing = new bool[ALPH_SIZE];

	for(i=0; i<ALPH_SIZE; i++)
		existing[i] = false;
	
	for(i=0; i<ALPH_SIZE; i++)
		if(p->child[i] != NULL)
			getAlphaHelper(p->child[i], existing);

	if(existing[27] == true) existing[27] = false;

	
	return existing;
}

void Trie::getAlphaHelper(struct TrieNode* p, bool* existing)
{
	int index;
	int i;
	if(p->count == 0) //leaf node
	{
		if(p->isCompressed == false)
		{
			index = getIndex(getSingleName(p->names));
			existing[index] = true;
		}
		else
		{
			for(i = 0; i < ALPH_SIZE; i++)
				if(getNameExist(p->names, i) == true)
					existing[i] = true; 
		}
		return;
	}
	
	index = getIndex(getSingleName(p->names));
	existing[index] = true;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(p->child[i] != NULL)
			getAlphaHelper(p->child[i], existing);
	}
	
	return;
}


//----------------------------------------------------
//---------------Bottom Up Compression----------------
//----------------------------------------------------

void Trie::getCandidateBottom(struct TrieNode* p, vector<BottomElem*> &list)
{
	//insert all the depth 2 nodes into the vector as candidate list
	if(p->depth == 2)
	{
		struct BottomElem *ce = new struct BottomElem;
		ce->n = p;

		int numofchars = getAlpha(p);
		ce->newStrs = (double)pow(numofchars, p->depth - 1);
		ce->oldStrs = getNumofStr(p);
		ce->newSize = numofchars + (int)floor(log10(p->depth-1)+1) + 3; //[d,1], numofchars for d, floor() for 1, 3 for [,]
		ce->oldSize = Trie2Str(p).length() - 1; //(d$) -1 minus itself. Trie2Str() include p->name

		if(ce->newStrs < 0 || ce->oldStrs == INVALID)
			ce->cost = INVALID;
		else
			ce->cost = ce->newStrs - ce->oldStrs;

		ce->benefit = ce->oldSize - ce->newSize;

		list.push_back(ce);

		return;
	}

	if(p->depth == 0 || p->depth == 1)
		return; // for compressed node... doesn't count for candidate?

	for(int i=0; i<ALPH_SIZE; i++)
	{
		if(p->child[i] != NULL)
			getCandidateBottom(p->child[i], list);
	}
	
	return;

}

void Trie::CompressTrieBottomUp()
{
	vector<BottomElem*> candid;

	getCandidateBottom(root, candid);

	//candid = getCandidateList(); // get all the leaf nodes

	int size, i;

	size = candid.size();

	double originalsize = Trie2Str(root).length();
	
	struct BottomElem *ce;
	struct TrieNode *parent;

	//double benefitratio, tempratio;
	double maxbenefit;
	double mincost, tempcost;
	
	int id;
	vector<BottomElem*>::iterator it;

	while(originalsize > TRIELEN && candid.size() > 0)
	{
		//compress here
		//find the best one to compress
		id = -1;
		
		size = candid.size();

		for(i = 0; i < size; i++)
		{
			if( candid[i]->cost != INVALID)
			{
				mincost = candid[i]->cost;
				maxbenefit = candid[i]->benefit;
				id = i;
				break;
			}
			else
			{
				mincost = -INVALID;
				maxbenefit = candid[i]->benefit;
				id = i;
				break;
			}
		}

		for( i++ ; i < size; i++)
		{
			if(candid[i]->cost != INVALID)
			{
				if(candid[i]->cost < mincost)
				{
					mincost = candid[i]->cost;
					maxbenefit = candid[i]->benefit;
					id = i;
				}
				else if(candid[i]->cost == mincost)
				{
					if(candid[i]->benefit > maxbenefit)
					{
						mincost = candid[i]->cost;
						maxbenefit = candid[i]->benefit;
						id = i;
					}
				}
			}
			else
			{
				tempcost = -INVALID;
				if(tempcost < mincost)
				{
					mincost = tempcost;
					maxbenefit = candid[i]->benefit;
					id = i;
				}
				else if(tempcost == mincost)
				{
					if(candid[i]->benefit > maxbenefit)
					{
						mincost = tempcost;
						maxbenefit = candid[i]->benefit;
						id = i;
					}
				}

			}
		}

		if(id == -1) break;
		
		ce = candid[id];
		parent = ce->n->parent;

		//compress the subtree, attach it as the child of ce->n
		bool* existing;
		existing = getAlpha(ce->n, true);

		struct TrieNode* newnode;
		newnode = TrieNewNode();

		newnode->count = 0;
		newnode->isCompressed = true;
		newnode->depth = ce->n->depth - 1;
		newnode->com.depth = ce->n->depth - 1;
		newnode->com.size = 0;

		for(i = 0; i < ALPH_SIZE; i++)
		{
			if(existing[i] == true)
			{
				setName(newnode->names, getCharFromIndex(i));
				newnode->com.size ++;
			}
		}

		ce->n->child[ALPH_SIZE - 1] = newnode;
		for(i = 0; i < ALPH_SIZE - 1; i++)
		{
			if(ce->n->child[i] != NULL)
				TrieFreeNode(ce->n->child[i]);
			
			ce->n->child[i] = NULL;
		}
		ce->n->count = 1;
		newnode->parent = ce->n;

		//remove the entry from the candidate list. insert its parent
		it = candid.begin();
		for(i = 0; i < id; i++)
			it++;
		
		candid.erase(it);

	    if(parent != NULL)
		{
			bool found = false;

			for(it = candid.begin(); it != candid.end(); ++it)
			{
				if((*it)->n == parent)
				{
					found = true;
					break;
				}
			}

			if(found == false)//if the parent doesn't exist in the list, insert its parent
			{
				struct BottomElem *par = new struct BottomElem;
				par->n = parent;
				int numofchars = getAlpha(par->n);
				par->newStrs = (double)pow(numofchars, par->n->depth - 1);
				par->oldStrs = getNumofStr(par->n);
				par->newSize = numofchars + (int)floor(log10(par->n->depth-1)+1) + 3; //[d,1], numofchars for d, floor() for 1, 3 for [,]
				par->oldSize = Trie2Str(par->n).length() - 1; //(d$) -1 minus itself. Trie2Str() include p->name

				if(par->newStrs < 0 || par->oldStrs == INVALID)
					par->cost = INVALID;
				else
					par->cost = par->newStrs - par->oldStrs;

				par->benefit = par->oldSize - par->newSize;

				candid.push_back(par);

			}
		}
		
		originalsize -= maxbenefit;
	}
	
	//final cleanup
	for(i = 0; i < candid.size(); i++)
		delete candid[i];
	//end
	return;
}

//--------------------------------------------------------
//---------------Top Down Compression---------------------
//--------------------------------------------------------

void Trie::CompressTrieTopDown()
{
	int i, j, id, index;

	vector<TopElem*> candidates;

	struct TrieNode* newroot;
	newroot = TrieNewNode();
	setName(newroot->names, EPSILON);
	newroot->depth = root->depth;

	//construct the initial result tree
	bool* existing;
	
	existing = getAlpha(root, true);
	struct TrieNode *temp = TrieNewNode();
	temp->isCompressed = true;
	temp->count = 0;
	temp->depth = root->depth - 1;
	temp->com.size = 0;
	temp->com.depth = temp->depth;

	for(j = 0; j < ALPH_SIZE; j++)
	{
		if(existing[j] == true)
		{
			setName(temp->names, getCharFromIndex(j));
			temp->com.size++;
		}
	}

	newroot->count = 1;
	newroot->child[ALPH_SIZE-1] = temp;
	temp->parent = newroot;

	struct TopElem *te;
	te = getCandidateTop(root, newroot);

	candidates.push_back(te);

	double originalsize = Trie2Str(newroot).length();
	double mincost = 0;
	double tempcost;
	double maxbenefit;

	//process the candidate list
	vector<TopElem*>::iterator it;
	//double benefitratio, tempratio;
	struct TopElem  te1;
	bool first = true;

	int numOfCenters = 1;

	while(candidates.size() > 0 && /*originalsize < TRIELEN*/ numOfCenters <= K) 
	{

		id = -1;
			
		bool start = true;

		for(i = 0; i < candidates.size(); i++)
		{
			//if(candidates[i]->cost + originalsize < TRIELEN)
			if(candidates[i]->cost + numOfCenters <= K)
			{
				
				if(start)
				{
					mincost = candidates[i]->cost;
					maxbenefit = candidates[i]->benefit;
					id = i;
					start = false;
				}
				else
				{
					tempcost = candidates[i]->cost;

					if(tempcost < mincost)
					{
						mincost = tempcost;
						maxbenefit = candidates[i]->benefit;
						id = i;
					}
					else if(tempcost == mincost)
					{
						if(candidates[i]->benefit > maxbenefit)
						{
							mincost = tempcost;
							maxbenefit = candidates[i]->benefit;
							id = i;
						}
					}
				}
			}
		}//for

		if(id == -1) break;

		te1.copy = candidates[id]->copy;
		te1.old = candidates[id]->old;
		te1.subtree = candidates[id]->subtree;
		te1.newSize = candidates[id]->newSize;
		te1.newStrs = candidates[id]->newStrs;
		te1.oldSize = candidates[id]->oldSize;
		te1.oldStrs = candidates[id]->oldStrs;
		te1.cost = candidates[id]->cost;
		te1.benefit = candidates[id]->benefit;

		it = candidates.begin();
		for(i = 0; i < id; i++)
			it++;
		
		candidates.erase(it);

		int test = candidates.size();

		if(first)
		{
			//add for clean up ==== liang
			TrieFreeNode(newroot);
			//end
			newroot = te1.subtree;
			te1.copy = newroot;
			first = false;
		}
		else
		{
			te1.subtree->parent = te1.copy->parent;

			index = getIndex(getSingleName(te1.copy->names));
			te1.copy->parent->child[index] = te1.subtree;

			//add for clean up ==== liang
			TrieFreeNode(te1.copy);
			//end

			//te1.copy = te1.copy->parent->child[index];
			te1.copy = te1.subtree;
		}

		for(i = 0; i < ALPH_SIZE; i++)
		{
			if(te1.old->child[i] != NULL)
			{
				if(te1.old->child[i]->depth != 1) //if depth=1, no furthur decompression possible
				{
					struct TopElem *te2;
					te2 = getCandidateTop(te1.old->child[i], te1.copy->child[i]);
					candidates.push_back(te2);
				}
			}
		}
		//remove the entry from the candidate list. insert its children
		
		
		//originalsize += mincost;
		numOfCenters += mincost;

	}//while

	//add for clean up ===== liang
	TrieFreeNode(root);

	for(i = 0; i < candidates.size(); i++)
	{
		TrieFreeNode(candidates[i]->subtree);
		delete candidates[i];
	}

	root = newroot;

	return;
}

struct TopElem* Trie::getCandidateTop(struct TrieNode* pOld, struct TrieNode* pCopy)
{
	int i, j;
	bool* existing;
	
	//construct the future subtree if we uncompress the node
	struct TrieNode* sub = TrieNewNode();
	sub->names = pOld->names;
	sub->depth = pOld->depth;

	int sizeincrease = 0;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if(pOld->child[i] != NULL)
		{
			//temp = te.old->child[i];

			struct TrieNode* children = TrieNewNode();
			children->names = pOld->child[i]->names;
			//setName(children->names, getSingleName(pOld->child[i]->names));
			children->isCompressed = false;
			children->count = 1;
			children->depth = pOld->child[i]->depth;
			children->parent = sub;
			
			sub->child[i] = children;
			sub->count++;
			
			struct TrieNode* grandchildren = TrieNewNode();
			if(pOld->child[i]->depth == 1) // if there is no furthur data, just attach a '&'
			{
				//struct TrieNode* grandchildren = TrieNewNode();
				setName(grandchildren->names, EPSILON);
				grandchildren->depth = 0;
				grandchildren->isCompressed = false;
				grandchildren->count = 0;

				children->child[ALPH_SIZE-2] = grandchildren;
				grandchildren->parent = children;
			}
			else // if there is furthur data, attach the compressed node
			{
				sizeincrease++;

				existing = getAlpha(pOld->child[i], true);

				//struct TrieNode* grandchildren = TrieNewNode();
				grandchildren->isCompressed = true;
				grandchildren->count = 0;
				grandchildren->depth = pOld->child[i]->depth - 1;
				grandchildren->com.size = 0;
				grandchildren->com.depth = grandchildren->depth;

				for(j = 0; j < ALPH_SIZE; j++)
				{
					if(existing[j] == true)
					{
						setName(grandchildren->names, getCharFromIndex(j));
						grandchildren->com.size++;
					}
				}

				children->child[ALPH_SIZE-1] = grandchildren;
				grandchildren->parent = children;
			}

		}
	}

	
	struct TopElem* te = new struct TopElem;

	te->old = pOld;
	te->copy = pCopy;
	te->subtree = sub;

	int numofchars = pCopy->child[ALPH_SIZE-1]->com.size;
	
	te->newStrs = getNumofStr(sub);
	te->oldStrs = (double)pow(numofchars, pCopy->depth - 1);
	
	if(te->newStrs == INVALID || te->oldStrs < 0)
		te->benefit = INVALID;
	else
		te->benefit = te->oldStrs - te->newStrs;

	te->cost = sub->count;
	

	return te;

}


//get the edit distance between the trie and a string
int Trie::getED(string str, NFA* nfa)
{
	
	clock_t tv1, tv2;
	double time;
	tv1 = clock();
  
  	NFAMatch* nm = new NFAMatch(nfa);

	int distance;
	distance = nm->match(str);
	//bool good = nm->match(str, 2);

	delete nm;
		
	tv2 = clock();
	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
	totalEDTime += time;
	

	return distance; 
}

bool Trie::getED(string str, int delta)
{
	clock_t tv1, tv2;
	double time;
	tv1 = clock();
  
	NFA* nfa = new NFA();
	nfa->ConvertFromTrie(this);

  	NFAMatch* nm = new NFAMatch(nfa);

	//int distance;
	//distance = nm->match(str);
	bool good = nm->match(str, delta);

	delete nm;
	

	
	tv2 = clock();
	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
	totalEDTime += time;
	
	delete nfa;

	return good; 
}


int Trie::getNumofNodes(struct TrieNode* p)
{
	int i, count;
	assert(p);
	
	if(p->count == 0)
	{
		return 1;		
	}
	else
	{
		count = 0;
		for(i = 0; i < ALPH_SIZE; i++)
		{
			if(p->child[i] != NULL)
			{
				count += getNumofNodes(p->child[i]);
				
			}
		}
		count++;
		return count;
	}
}

void Trie::setName(int &names, char character)
{
	int index = getIndex(character);
	int oper = 1;
	oper = oper << index;

	names = names | oper;

	return;
}

char Trie::getSingleName(int names)
{
	int i;
	int oper = 1;

	for(i = 0; i < ALPH_SIZE; i++)
	{
		if((oper & names) != 0)
			break;
		
		oper = oper << 1;
	}

	char temp = getCharFromIndex(i);

	return temp;
}

bool Trie::getNameExist(int names, int index)
{
	int oper = 1;

	oper = oper << index;

	if((oper & names) != 0)
		return true;
	else
		return false;
}
