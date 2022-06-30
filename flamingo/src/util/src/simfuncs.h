/*
  $Id: simfuncs.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 10/05/2007
  Author: Jiaheng Lu
          Rares Vernica 
  
*/

#ifndef _simfuncs_h_
#define _simfuncs_h_

#include <string> 
#include <set>
#include <vector>

using namespace std;

unsigned ed(const string &s1, const string &s2);
bool ed(const string &s1, const string &s2, unsigned threshold);

unsigned edSwap(const string &s1, const string &s2);

bool  substringed(const string &longS, const string &shortS, unsigned threshold);

float normalizedED(const string &s1, const string &s2);

float cosine(const string &s1, const string &s2, unsigned gramLength = 3);

// default is with Prefix and Suffix characters
float jaccard(const string &s1, const string &s2, unsigned gramLength = 3);
float jaccard(const set<string> &s1Gram, const string &s2, unsigned gramLength = 3);
float jaccardBag(const string &s1, const string &s2, unsigned gramLength = 3);
float jaccardBag(const multiset<string> &s1Gram, const string &s2, 
                 unsigned gramLength = 3);
float jaccardNoPrePost(const string &s1, const string &s2, unsigned gramLength = 3);
float jaccardNoPrePost(const set<string> &s1Gram, const string &s2, 
                       unsigned gramLength = 3);

float jaccardHash(const string &s1, const string &s2, unsigned gramLength = 3);
float jaccardHash(const set<unsigned> &s1Gram, const string &s2, 
                  unsigned gramLength = 3);
float jaccardHashBag(const string &s1, const string &s2, unsigned gramLength = 3);
float jaccardHashBag(const multiset<unsigned> &s1Gram, const string &s2, 
                     unsigned gramLength = 3);
float jaccardNoPrePostHash(const string &s1, const string &s2, 
                           unsigned gramLength = 3);
float jaccardNoPrePostHash(const set<unsigned> &s1Gram, const string &s2,
                           unsigned gramLength = 3);

float dice(const string &s1, const string &s2, unsigned gramLength = 3);

void  decompose2GramsHashSet(const string &s, set<unsigned> &result,
                             unsigned gramLength);
#endif
