/*
  $Id: filtertreenode.h 5783 2010-10-21 23:12:04Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _filtertreenode_h_
#define _filtertreenode_h_

#include <vector>
#include <set>
#include <tr1/unordered_map>

#include "gramlist.h"
#include "gramlistondisk.h"
#include "common/src/filtertypes.h"
#include "common/src/gramgen.h"
#include "util/src/array.h"

template<class InvList> class FilterTreeNode;
template<class InvList> class KeyNodePair;

template<class InvList = vector<unsigned> >
class KeyNodePair {
public:
  unsigned key;
  FilterTreeNode<InvList>* node;
  
  KeyNodePair(unsigned key, FilterTreeNode<InvList>* node) 
    : key(key), node(node) {}
  
  bool operator<(const KeyNodePair& knp) {
    return key < knp.key;
  }
};

template <class InvList = vector<unsigned> >
class FilterTreeNode {
public:
  typedef unordered_map<unsigned, GramList<InvList>* > GramMap;
  
  GramMap gramMap;
  GramList<InvList>* distinctStringIDs;
  vector<KeyNodePair<InvList> > children; // sorted by pair.first
  set<unsigned> gramBlackList; // set of hole grams for compressed index
  
  FilterTreeNode() : distinctStringIDs(NULL) {}
  
  void getQueryGramLists(const string& query, GramGen& gramGen, vector<QueryGramList<InvList>* >* gramLists); 
  void getQueryGramLists(const vector<unsigned>& gramCodes, vector<QueryGramList<InvList>* >* gramLists);
  void getInvertedLists(const string& query, const GramGen& gramGen, vector<InvList*> &invertedLists);
  void getInvertedLists(const vector<unsigned>& gramCodes, vector<InvList*> &invertedLists);
  
  // place all leaves below input node into result vector
  static void getSubTreeLeaves(FilterTreeNode<InvList>* node, vector<FilterTreeNode<InvList>* >& leaves);
  
  // get size of subtree (including input node), without counting inverted lists
  static unsigned getSubTreeSize(FilterTreeNode<InvList>* node);  

  // get size of inverted lists below subtree (including input node)
  static unsigned getSubTreeInvertedListsSize(FilterTreeNode<InvList>* node);
  
  // get size of this node
  unsigned getSize();
  
  // get size of inverted lists of this node
  unsigned getInvertedListsSize();  
  
  bool isLeaf() { return children.size() == 0; }

  ~FilterTreeNode();
};

template<class InvList>
FilterTreeNode<InvList>::
~FilterTreeNode() { 
  for(typename GramMap::iterator iter = gramMap.begin(); iter != gramMap.end(); iter++) {
    GramList<InvList>* tmp = iter->second;
    if(tmp) tmp->free();
  }
  if(distinctStringIDs) distinctStringIDs->free();
}

template<class InvList>
void 
FilterTreeNode<InvList>::
getInvertedLists(const string& query, const GramGen& gramGen, vector<InvList*> &invertedLists) {
  vector<unsigned> gramCodes;
  gramGen.decompose(query, gramCodes);      
  getInvertedLists(gramCodes, invertedLists);       
}

template<class InvList>
void 
FilterTreeNode<InvList>::
getInvertedLists(const vector<unsigned>& gramCodes, vector<InvList*> &invertedLists) {
  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes[i];
    if(gramMap.find(gramCode) != gramMap.end()) {
      InvList* tmp = gramMap[gramCode]->getArray();
      invertedLists.push_back(tmp);
    }
  }  
}

template<class InvList>
void 
FilterTreeNode<InvList>::
getQueryGramLists(const string& query, GramGen& gramGen, vector<QueryGramList<InvList>* >* queryGramLists) {
  vector<unsigned> gramCodes;
  gramGen.decompose(query, gramCodes);      
  getGramLists(gramCodes, queryGramLists);       
}

template<class InvList>
void 
FilterTreeNode<InvList>::
getQueryGramLists(const vector<unsigned>& gramCodes, vector<QueryGramList<InvList>* >* queryGramLists) {
  for(unsigned i = 0; i < gramCodes.size(); i++) {
    unsigned gramCode = gramCodes[i];
    if(gramMap.find(gramCode) != gramMap.end()) {
      queryGramLists->push_back( new QueryGramList<InvList>(gramCode, gramMap[gramCode]) );
    }
  }
}

template<class InvList>
void
FilterTreeNode<InvList>::
getSubTreeLeaves(FilterTreeNode<InvList>* node, vector<FilterTreeNode<InvList>* >& leaves) {
  if(node->isLeaf()) leaves.push_back(node);
  else {    
    vector<KeyNodePair<InvList> >& children = node->children;
    for(unsigned i = 0; i < children.size(); i++)
      FilterTreeNode::getSubTreeLeaves(children[i].node, leaves);
  }
}

template<class InvList>
unsigned 
FilterTreeNode<InvList>::
getSubTreeSize(FilterTreeNode<InvList>* node) {
  unsigned size = node->getSize();
  vector<KeyNodePair<InvList> >& children = node->children;
  for(unsigned i = 0; i < children.size(); i++)
    size += FilterTreeNode::getSubTreeSize(children[i].node);
  return size;
}
  
template<class InvList>
unsigned 
FilterTreeNode<InvList>::
getSubTreeInvertedListsSize(FilterTreeNode<InvList>* node) {
  unsigned size = node->getInvertedListsSize();
  vector<KeyNodePair<InvList> >& children = node->children;
  for(unsigned i = 0; i < children.size(); i++)
    size += FilterTreeNode::getSubTreeInvertedListsSize(children[i].node);
  return size;
}
  
template<class InvList>
unsigned 
FilterTreeNode<InvList>::
getSize() {
  unsigned size = 0;
  size += sizeof(GramMap);
  size += sizeof(GramList<vector<unsigned> >*);
  size += sizeof(vector<KeyNodePair<InvList> >);
  size += sizeof(pair<unsigned, FilterTreeNode>) * children.size(); 
  size += sizeof(InvList);
  size += gramMap.size() * (sizeof(unsigned) + sizeof(GramListOnDisk<InvList>));
  return size;
}
  
template<class InvList>
unsigned 
FilterTreeNode<InvList>::
getInvertedListsSize() {
  if(!isLeaf()) return 0;
  unsigned size = 0;
  typename GramMap::const_iterator citer;
  for(citer = gramMap.begin(); citer != gramMap.end(); citer++)
    size += citer->second->getArray()->size();
  size += sizeof(unsigned) * distinctStringIDs->getArray()->size();
  return size;
}  

#endif
