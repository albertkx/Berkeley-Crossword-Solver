//  
// $Id: Trie.h 4143 2008-12-08 23:23:55Z abehm $
//
// Trie.h: interface for the Trie class.
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

#ifndef _TRIE_
#define _TRIE_


#include <string>
#include <vector>

#include "parameters.h"
#include "NFA.h"

using namespace std;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class NFA;

struct Compressed
{
	int size;
	int depth;
};

struct BottomElem
{
	struct TrieNode *n;
	double oldStrs;//# of string
	double newStrs;
	double cost;
	double oldSize;//length of the trie representation
	double newSize;
	double benefit;
};

struct TopElem
{
	struct TrieNode *old;
	struct TrieNode *copy;
	struct TrieNode *subtree;
	double oldStrs;//# of string
	double newStrs;
	double cost;
	double oldSize;//length of the trie representation
	double newSize;
	double benefit;
};


struct TrieNode
{
	int names;
	struct Compressed com;
	bool isCompressed;
	int count;  //if count = 0, leaf node
	int depth;
	struct TrieNode *child[ALPH_SIZE];
	struct TrieNode *parent; // if parent = NULL, root node
};

class Trie  
{
public:
	bool getNameExist(int names, int index);
	char getSingleName(int names);
	void setName(int& names, char character);
	
	struct TrieNode * root;

	Trie();
	virtual ~Trie();
	
	string Trie2Str(struct TrieNode *root);
	//char* Trie2Str(struct TrieNode *root);
	void oneNode2Str(struct TrieNode *node, string &result);
	//void oneNode2Str(struct TrieNode *node, char* result, int &pos);
	struct TrieNode* Str2Trie(string input);
	void Str2oneNode(struct TrieNode *node, string input);
	struct TrieNode * Trie::TrieNewNode();
	void TrieInitNode(struct TrieNode *N);
	void TrieFreeNode(struct TrieNode *p);
	void TrieClearTrie();
	bool getNum(struct TrieNode * p, double &counter);
	double getNumofStr(struct TrieNode *p);
	long getSize(struct TrieNode *p);

	void MergeTries(struct TrieNode* t2);
	void InsertStrs(struct TrieNode* t2, string insert);

	void assignDepth();
	int assignOneDepth(struct TrieNode* p);

	void getCandidateBottom(struct TrieNode* p, vector<BottomElem*> &list);
	//vector<BottomElem> getCandidateList();

	void getAlphaHelper(struct TrieNode* p, bool* existing);
	int getAlpha(struct TrieNode *p);
	bool* getAlpha(struct TrieNode *p, bool bo);
	
	void CompressTrieBottomUp();
	void CompressTrieTopDown();

	struct TrieNode* CopyTrie(struct TrieNode* p, struct TrieNode* parent);
	struct TopElem* getCandidateTop(struct TrieNode* pOld, struct TrieNode* pCopy);

	//int getED(string str);
	int getED(string str, NFA* nfa);
	bool getED(string str, int delta);

	int getNumofNodes(struct TrieNode* p);
	void AddToTrie(string s);
	int getIndex(int character);
	char getCharFromIndex(int index);
	

};


#endif

