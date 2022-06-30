//  
// $Id: RETree.cpp 5027 2010-02-18 19:41:48Z rares $
//
// RETree.cpp : Defines the entry point for the console application.
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

#include "stdafx.h"
#include <fstream>
#include <math.h>
#include <string.h>
#include <iostream.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <windows.h> 
#include "parameters.h"
#include "index.h"
#include "distance.h"

using namespace std;

double totalEDTime;

//int pruneinter;
//int pruneleaf;

//ofstream os;

struct Record* recordset;

int numofNodes;


int compare( const void *arg1, const void *arg2 )
{
   /* Compare all of both strings: */
	if( ((struct Record *)arg1)->numVal > ((struct Record *) arg2)->numVal)
		return 1;
	else if( ((struct Record *)arg1)->numVal < ((struct Record *) arg2)->numVal)
		return -1;
	else
		return 0;
   
}

void purify(char* incoming)
{
	int length = strlen(incoming);

	int counter = 0;

	for(int i=0; i<length; i++)
	{
		if( !(incoming[i]>='a' && incoming[i]<='z') )  
		{
			incoming[i] = '~';
		}
		
	}
	
	return;

}

void RecordToQuery(float num, float nDelta, char* str, int sDelta, struct Query *q)
{
	q->numVal = num;
	q->numDelta = nDelta;
	strcpy(q->strVal, str);
	q->strDelta = sDelta;
	
	return;
}



struct Node* merge(struct Node* nodes[], int startIndex, int endIndex, bool first)
{
	struct Node * n = RTreeNewNode();

	numofNodes++;
	
	struct Node *old;
	int i, j;
	
	if(first == true)
	{
		n->level = 1; //one level up

		//for each leaf node, combine its num and str val, put in one branch of the parent node
		for(i = 0; i < endIndex - startIndex + 1; i++)
		{
			old = nodes[startIndex + i];

			n->branch[i].child = old;
			n->count++;
			
			n->branch[i].rect.boundary[0] = old->records[0]->numVal;
			n->branch[i].rect.boundary[NUMDIMS - 1] = old->records[0]->numVal;

			for(j = 0; j < old->count; j++)
			{
				//update the numerical range
				if(old->records[j]->numVal < n->branch[i].rect.boundary[0])
					n->branch[i].rect.boundary[0] = old->records[j]->numVal;
				if(old->records[j]->numVal > n->branch[i].rect.boundary[NUMDIMS - 1])
					n->branch[i].rect.boundary[NUMDIMS - 1] = old->records[j]->numVal;

				n->branch[i].rect.realtrie->AddToTrie(old->records[j]->strVal);
			}

			n->branch[i].rect.realtrie->assignDepth();

			(*n->branch[i].rect.trie) = n->branch[i].rect.realtrie->Trie2Str(n->branch[i].rect.realtrie->root);

		}

	}
	else
	{
		n->level = nodes[0]->level + 1; //one level up

		//for each leaf node, combine its num and str val, put in one branch of the parent node
		for(i = 0; i < endIndex - startIndex + 1; i++)
		{
			old = nodes[startIndex + i];

			n->branch[i].child = old;
			n->count++;
			
			n->branch[i].rect.boundary[0] = old->branch[0].rect.boundary[0];
			n->branch[i].rect.boundary[NUMDIMS - 1] = old->branch[0].rect.boundary[NUMDIMS - 1];

			for(j = 0; j < old->count; j++)
			{
				//update the numerical range
				if(old->branch[j].rect.boundary[0] < n->branch[i].rect.boundary[0])
					n->branch[i].rect.boundary[0] = old->branch[j].rect.boundary[0];
				if(old->branch[j].rect.boundary[NUMDIMS - 1] > n->branch[i].rect.boundary[NUMDIMS - 1])
					n->branch[i].rect.boundary[NUMDIMS - 1] = old->branch[j].rect.boundary[NUMDIMS - 1];

				n->branch[i].rect.realtrie->MergeTries(old->branch[j].rect.realtrie->root);
			}

			n->branch[i].rect.realtrie->assignDepth();

			(*n->branch[i].rect.trie) = n->branch[i].rect.realtrie->Trie2Str(n->branch[i].rect.realtrie->root);

		}
	}

	return n;
}


struct Node** constructParent(struct Node* nodes[], int number, bool first)
{

	int parentNum = (int)ceil((double)number/MAXCARD);
	struct Node** parents = new struct Node*[parentNum];
	for(int i=0; i<parentNum; i++){
		if(i != parentNum -1){
			parents[i]= merge(nodes, i*MAXCARD, (i+1)*MAXCARD - 1, first);
		}
		else{
			parents[i]= merge(nodes, i*MAXCARD, number-1, first);
		}
	}
	return parents;
}

struct Node* constructTree(struct Node* leafNodes[], int number)
{
	struct Node** nodes ;
	nodes = leafNodes;
	
	bool first = true;
	
	while( number > 1)
	{
		if(first)
		{
			nodes = constructParent(nodes, number, first);
			first = false;
		}
		else
		{
			nodes = constructParent(nodes, number, first);
		}
		number = (int)ceil((double)number/MAXCARD);
	}
	return nodes[0];
}

void CompressTrieNode(struct Node* p, int method)
{
	int i;
	if(p->level == 0)
	{
		return;
	}
	

	for(i = 0; i < p->count; i++)
	{
		int before = p->branch[i].rect.realtrie->getNumofNodes(p->branch[i].rect.realtrie->root);

		if(method == TOPDOWN)
			p->branch[i].rect.realtrie->CompressTrieTopDown();
		else
			p->branch[i].rect.realtrie->CompressTrieBottomUp();

		*p->branch[i].rect.trie = p->branch[i].rect.realtrie->Trie2Str( p->branch[i].rect.realtrie->root );

		int after = p->branch[i].rect.realtrie->getNumofNodes(p->branch[i].rect.realtrie->root);

		p->branch[i].rect.compratio = (float)after / (float)before;

		//convert the trie into a nfa and store it
		//p->branch[i].rect.nfa = new NFA();
		//p->branch[i].rect.nfa->ConvertFromTrie(p->branch[i].rect.realtrie);
		//end convert to nfa

		//if(p->level == 1)
		//	cout << "number of nodes: " << after << endl;

		if(p->branch[i].child != NULL)
			CompressTrieNode(p->branch[i].child, method);
	}

	return;
}


struct Node* loadRETree(int compressmethod)
{
	int i, j; //, subindex;
	//Trie* trie = new Trie();

	ifstream inf(DATAFILE);
	if (!inf) {
		cout << "Error, could not open file: " << DATAFILE << endl;
		exit(1);
    }
	//struct Node * RTreeNewNode()
	char temp[MAXLEN];

	for(i = 0; i < SIZES; i++)
	{
		memset(temp, ' ', MAXLEN);
		inf >> temp;

		for(j = 0; j < MAXLEN; j++)
		{
			if(temp[j] == '\0')
				break;

			temp[j] = tolower(temp[j]);
		}
		//purify( _strlwr(temp) );	
		purify(temp);
		
		strcpy(recordset[i].strVal, temp);
		
		inf >> recordset[i].numVal;
	}
	
	inf.close();
	
	qsort(recordset, SIZES, sizeof(struct Record), compare);

	int id = 0;
	struct Node** leafnodes = new struct Node*[LEAFNODES];
	
	for(i = 0; i < LEAFNODES; i++)
	{
		struct Node * n = RTreeNewNode();
		
		leafnodes[i] = n;

		n->level = 0;

		n->isLeaf = true;
		
		for(j = 0; j < LEAFCARD && id < SIZES; j++)
		{
			n->records[j] = &recordset[id];
			
			id++;
		}

		n->count = j;
	}

	struct Node* result;

	result = constructTree(leafnodes, LEAFNODES);

	CompressTrieNode(result, compressmethod);

	//ConvertTrieToNFA(result);
	
	return result;

}


//RETree Query
bool QueryNumOverlap(struct Query *q, struct Rect *s)
{
	if(q->numVal - q->numDelta > s->boundary[NUMDIMS - 1] ||
	   s->boundary[0] > q->numVal + q->numDelta)
	{
		return false;
	}

	return true;
}

bool QueryOverlap(struct Query *Q, struct Rect *S)
{
	register struct Rect *s = S;
	register struct Query *q = Q;

	//add for debug
	if(s->compratio < CRDELTA)
		return QueryNumOverlap(q, s);
	//end for debug

	assert(q && s);

		
	if(q->numVal - q->numDelta > s->boundary[NUMDIMS - 1] ||
	   s->boundary[0] > q->numVal + q->numDelta)
	{
		return false;
	}
	
	bool result = (s->realtrie)->getED(q->strVal, q->strDelta);

	return result;
}


bool QueryOverlap(struct Query *q, struct Record *r)
{
	assert(q && r);

	
	if(q->numVal - q->numDelta > r->numVal ||
	   r->numVal > q->numVal + q->numDelta)
	{
		return false;
	}
	
	Distance1 dis;
	int result = dis.LD(q->strVal, r->strVal);

	if(result <= q->strDelta)
		return true;
	else
		return false;
}


int RETreeQueryHelper(struct Node *r, struct Query* qr, long* IOCount)
{
	register struct Node *n = r;
	register struct Query *q = qr;
	int i;

	assert(n);
	assert(n->level >= 0);
	assert(q);

	int hitCount = 0;

	(*IOCount)++;
	Sleep(10);

	if (n->level > 0) // this is an internal node in the tree 
	{


		for (i = 0; i < n->count; i++)
		{
			//add for reporting result
			//clock_t tv1, tv2;
			//double time;
			//tv1 = clock();
			
			//os << "Level at " << n->level << endl;

			//without level filtering
			//if (n->branch[i].child != NULL && QueryOverlap(q, &n->branch[i].rect))
			//{
			//	tv2 = clock();
			//	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
			//	os << "Time for edit distance calculation is: " << time << " miliseconds. visit it" << endl;
			//	os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
			//	os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;
			//
			//	hitCount += RETreeQueryHelper(n->branch[i].child, q, IOCount);
			//}
			//else
			//{
			//	tv2 = clock();
			//	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
			//	os << "Time for edit distance calculation is: " << time << " miliseconds. prune it" << endl;
			//	os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
			//	os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;
			//
			//	pruneinter ++;
			//}

			//add for level filtering
			if(n->level > 1)
			{
				if (n->branch[i].child != NULL && QueryNumOverlap(q, &n->branch[i].rect))
				{
					//tv2 = clock();
					//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
					//os << "Time for edit distance calculation is: " << time << " miliseconds. visit it" << endl;
					//os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
					//os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;
				
					hitCount += RETreeQueryHelper(n->branch[i].child, q, IOCount);
				}
				else
				{
					//tv2 = clock();
					//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
					//os << "Time for edit distance calculation is: " << time << " miliseconds. prune it" << endl;
					//os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
					//os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;

					//os << "prune one at level: " << n->branch[i].child->level << endl;
					//pruneinter ++;
				}
			}
			else
			{
				if (n->branch[i].child != NULL && QueryOverlap(q, &n->branch[i].rect))
				{
					//tv2 = clock();
					//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
					//os << "Time for edit distance calculation is: " << time << " miliseconds. visit it" << endl;
					//os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
					//os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;
				
					hitCount += RETreeQueryHelper(n->branch[i].child, q, IOCount);
				}
				else
				{
					//tv2 = clock();
					//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
					//os << "Time for edit distance calculation is: " << time << " miliseconds. prune it" << endl;
					//os << "Number of nodes in the trie: " << n->branch[i].rect.realtrie->getNumofNodes(n->branch[i].rect.realtrie->root) << endl;
					//os << "Compress ratio is: " << n->branch[i].rect.compratio << endl  << endl;

					//os << "prune one at level: " << n->branch[i].child->level << endl;
					//pruneinter ++;
				}
			}
			//end for level filtering
		}

	}
	else // this is a leaf node 
	{
		for (i = 0; i < n->count; i++)
		{
			//clock_t tv1, tv2;
			//double time;
			//tv1 = clock();
			
			//os << "Level at " << n->level << " leaf node" << endl;

			if (n->records[i] != NULL && QueryOverlap(q,n->records[i]))
			{
				//tv2 = clock();
				//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
				//os << "Time for edit distance calculation is: " << time << " miliseconds. get one answer" << endl;
			
				hitCount++;
				cout << "hit one: " << endl;
				
				cout << n->records[i]->numVal << "\t" << n->records[i]->strVal << endl;
			}
			else
			{
				//tv2 = clock();
				//time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
				//os << "Time for edit distance calculation is: " << time << " miliseconds. prune it" << endl;
				
				//os << "prune one at leaf level" << endl << endl;
				//pruneleaf ++;
			}
		}
	}
	return hitCount;
}

int RETreeQuery(struct Node* root, struct Query* qr, long* IOCount)
{
	register struct Node *n = root;
	register struct Query *q = qr; // NOTE: Suspected bug was R sent in as Node* and cast to Rect* here. Fix not yet tested.
	register int hitCount = 0;
	//register int i;

	assert(n);
	assert(n->level >= 0);
	assert(q);

	(*IOCount) = 0;
	hitCount = RETreeQueryHelper(n, q, IOCount);

	return hitCount;
}




void DeleteRETree(struct Node* root)
{
	struct Node* n = root;
	assert(n);
	assert(n->level >= 0);

	if (n->level > 0) /* this is an internal node in the tree */
	{

		for (int i = 0; i < n->count; i++)
			if (n->branch[i].child != NULL)
			{
				DeleteRETree(n->branch[i].child);
			}

		RTreeFreeNode(n);
	}
	else /* this is a leaf node */
	{
		RTreeFreeLeafNode(n);
	}
	return;
}

int main(int argc, char* argv[])
{
	cout << "Internal fanout: " << MAXCARD <<endl;
	cout << "LeafCard: " << LEAFCARD << endl;
	//cout << sizeof(struct Branch) << endl;

	srand ( time(NULL) );

	numofNodes = 0;

	recordset = new struct Record[SIZES];

	totalEDTime = 0.0;

	clock_t tv1, tv2;
	double time;
	tv1 = clock();
  
  	struct Node* root;

	
	root = loadRETree(TOPDOWN);

	
	tv2 = clock();
	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);

	cout << "Time for construction is: " << time << "miliseconds" << endl;

	cout << "numofNodes: " << numofNodes << "\tLEAFNODES:" << LEAFNODES << "\tsizeofRecord" << sizeof(struct Record) << endl;
	cout << "The size of the structure is:" << numofNodes * PAGESIZE + LEAFNODES * sizeof(struct Record) << " bytes" << endl << endl;

	long totalio;
	totalio = 0;
	
	ifstream inf(QUERYFILE);
	if (!inf) {
		cout << "Error, could not open file: " << QUERYFILE << endl;
		exit(1);
    }
	
	char temp[MAXLEN];
	int num;
	
	tv1 = clock();
	
	for(int i=0; i<NUMQUERY; i++)
	{
		struct Query qr;
		
		memset(temp, ' ', MAXLEN);
		inf >> temp;

		purify(temp);
		
		inf >> num;
	
		RecordToQuery(num, NUMDELTA, temp, STRDELTA, &qr);
			
		cout << "The query is: " << num << "\t" << temp << endl;

		long IO = 0;

		int	nhits = RETreeQuery(root, &qr, &IO);
		
		printf("Search resulted in %d hits\n\n", nhits);
		
		totalio += IO;

	}

	inf.close();
		
	//os << "total interior nodes pruned: " << pruneinter << endl;
	//os << "total leaf nodes pruned: " << pruneleaf << endl;

	//os.close();

	tv2 = clock();
	time = (tv2 - tv1)/(CLOCKS_PER_SEC / (double) 1000.0);
	cout << "Time for total running time is: " << time << "miliseconds" << endl;
	cout << "Average query time is: " << time/NUMQUERY << "miliseconds" << endl;

	cout << "Total IO is: " << totalio << endl;
	cout << "Average IO is: " << totalio/NUMQUERY << endl;

	cout << "Total time for edit distance is:" << totalEDTime << "miliseconds" << endl;

	DeleteRETree(root);

	return 0;

}

