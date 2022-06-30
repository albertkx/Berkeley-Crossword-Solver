/*
  $Id: simfuncs.cc 6077 2011-04-28 19:12:15Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 05/10/2007
  Author: Jiaheng Lu
          Rares Vernica 
*/

#include "simfuncs.h"

#include <cmath>
#include <fstream>
#include <iostream>


#include "misc.h"
#include "gram.h"

#include "output.h"

unsigned ed(const string &s1, const string &s2)
{ 
  unsigned i, iCrt, iPre, j;
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0)
    return m;
  if (m == 0)
    return n;

  unsigned d[2][m + 1];

  for (j = 0; j <= m; j++)
    d[0][j] = j;

  iCrt = 1;
  iPre = 0;
  for (i = 1; i <= n; i++) {
    d[iCrt][0] = i;
    for (j = 1; j <= m; j++)
      d[iCrt][j] = min(d[iPre][j] + 1, 
                       d[iCrt][j - 1] + 1, 
                       d[iPre][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    iPre = !iPre;
    iCrt = !iCrt;
  }
  
  return d[iPre][m];
}

bool substringed(const string &longS, const string &shortS, unsigned threshold)
{ 
 
  unsigned i, j;
  unsigned
    m = longS.length(), 
    n = shortS.length();

  if ((n == 0)|| (m == 0))
    return false;
  
  if (m<n) return false;

  unsigned d[n+1][m + 1];

  for (j = 0; j <= m; j++)
    d[0][j] = 0;

  for (i = 0; i <= n; i++)
    d[i][0] = i;

  
  for (i = 1; i <= n; i++) {
    for (j = 1; j <= m; j++)
      d[i][j] = min(d[i-1][j] + 1, 
		    d[i][j - 1] + 1, 
		    d[i-1][j - 1] + (longS[j-1] == shortS[i-1] ? 0 : 1));
   
  }
  
  for(unsigned k=0;k<m+1;k++)
    if (d[n][k] <= threshold)
      return true;

  return false;

}//end substringed

bool ed(const string &s1, const string &s2, unsigned threshold) 
{
  unsigned i, j, ii, jj;
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0)
    return m <= threshold;
  if (m == 0)
    return n <= threshold;
  if ((n > m && n - m > threshold) ||  
      (m > n &&  m - n > threshold))
    return false;

  unsigned d[n + 1][m + 1], dmin, dmax = threshold + 1;

  for (i = 0; i <= n; i++)
    d[i][0] = i;
  for (j = 1; j <= m; j++)
    d[0][j] = j;

  for (ii = 1; ii <= n; ii++) {
    dmin = dmax;
    for (j = 1; j <= min(ii, m); j++) {
      i = ii - j + 1;
      d[i][j] = min(d[i - 1][j] + 1,
                    d[i][j - 1] + 1,
                    d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
      dmin = min(dmin, d[i][j], d[i - 1][j]);
    }
    if (dmin > threshold)
      return false;
  }
  
  for (jj = 2; jj <= m; jj++) {
    dmin = dmax;
    for (j = jj; j <= min(n + jj - 1, m); j++) {
      i = n - (j - jj);
      d[i][j] = min(d[i - 1][j] + 1,
                    d[i][j - 1] + 1,
                    d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
      dmin = min(dmin, d[i][j], d[i - 1][j]);
    }
    if (dmin > threshold)
      return false;
  }

  return d[n][m] <= threshold;
}

unsigned edSwap(const string &s1, const string &s2)
{ 
  unsigned i, iCrt, iPre, j;
  unsigned
    n = s1.length(), 
    m = s2.length();
  unsigned d[2][m + 1];

  for (j = 0; j <= m; j++)
    d[0][j] = j;

  iCrt = 1;
  iPre = 0;
  for (i = 1; i <= n; i++) {
    d[iCrt][0] = i;
    for (j = 1; j <= m; j++)
      d[iCrt][j] = min(d[iPre][j] + 1,
                       d[iCrt][j - 1] + 1,
                       d[iPre][j - 1] + ((s1[i - 1] == s2[j - 1] ||
                                          (i > 1 &&
                                           j > 1 &&
                                           s1[i - 1] == s2[j - 2] &&
                                           s1[i - 2] == s2[j - 1])) ? 0 : 1));
    iPre = !iPre;
    iCrt = !iCrt;
  }
  
  return d[iPre][m];
}

float normalizedED(const string &s1, const string &s2)
{ 
  
   unsigned distance = ed(s1,s2);

  if (s1.size()<s2.size())
    return (1-distance*1.0/s2.size());
  else
    return (1-distance*1.0/s1.size());

}//end normalizedED

void decompose2GramsHashSet(const string &s, set<unsigned> &result, 
                            unsigned gramLength)
{
  vector<string> words;

  str2words(s, words, " ");


  for(unsigned i=0;i<words.size();i++) {
    vector<unsigned> res;
    str2grams(words.at(i),res,gramLength);
    
    for(unsigned i=0;i<res.size();i++)
      result.insert(res.at(i));
    
  }//end for
}//end decompose2GramsHashSet

float cosine(const string &s1, const string &s2, unsigned gramLength)
{
  set<string> gramSet1;
  str2grams(s1, gramSet1,gramLength);
 
  set<string> gramSet2;
  str2grams(s2, gramSet2,gramLength);
 
  unsigned news1Size = gramSet1.size();
  unsigned news2Size = gramSet2.size();
  gramSet1.insert(gramSet2.begin(),gramSet2.end());
  unsigned unionSize = gramSet1.size();
  unsigned intersectionSize 
	= news1Size+news2Size-unionSize;
 
  double d =
	intersectionSize*1.0/sqrt(news1Size*news2Size);

  return d;
}//end cosine

//return the jaccard similarity coefficient

float jaccard(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<string> s1Gram, s2Gram, sUni;
  str2grams(s1, s1Gram, q);
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d;
}

float jaccard(const set<string> &s1Gram, const string &s2, unsigned q)
{
  unsigned
    n = s1Gram.size(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<string> s2Gram, sUni;
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d; 
}

float jaccardBag(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  multiset<string> s1Gram, s2Gram, sUni;
  str2grams(s1, s1Gram, q);
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
 
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d;
}

float jaccardBag(const multiset<string> &s1Gram, const string &s2, 
                            unsigned q)
{
  unsigned
    n = s1Gram.size(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  multiset<string> s2Gram, sUni;
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d; 
}

float jaccardNoPrePost(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<string> s1Gram, s2Gram, sUni;
  str2gramsNoPrePost(s1, s1Gram, q);
  str2gramsNoPrePost(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d;
}

float jaccardNoPrePost(const set<string> &s1Gram, const string &s2, unsigned q)
{
  unsigned
    n = s1Gram.size(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<string> s2Gram, sUni;
  str2gramsNoPrePost(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d; 
}

float jaccardHash(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<unsigned> s1Gram, s2Gram, sUni;
  str2grams(s1, s1Gram, q);
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d;
}

float jaccardHashBag(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  multiset<unsigned> s1Gram, s2Gram, sUni;
  str2grams(s1, s1Gram, q);
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));  

  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();

  /*
  cout << s1Gram.size() << " " << s1Gram << endl;
  cout << s2Gram.size() << " " << s2Gram << endl;
  cout << sUni.size() << " " << sUni << endl;
  cout << interSize << endl;
  cout << d << endl;
  */

  return d; 
}

float jaccardHashBag(const multiset<unsigned> &s1Gram, const string &s2,
                     unsigned q)
{
  unsigned
    n = s1Gram.size(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  multiset<unsigned> s2Gram, sUni;
  str2grams(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d; 
}

float jaccardHashNoPrePost(const string &s1, const string &s2, unsigned q)
{
  unsigned
    n = s1.length(), 
    m = s2.length();

  if (n == 0 || m == 0)
    return 0;

  set<unsigned> s1Gram, s2Gram, sUni;
  str2gramsNoPrePost(s1, s1Gram, q);
  str2gramsNoPrePost(s2, s2Gram, q);

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));
  
  unsigned interSize  = s1Gram.size() + s2Gram.size() - sUni.size();
  
  float d =  static_cast<float>(interSize) / sUni.size();
  
  return d;
}

float dice(const string &s1, const string &s2, unsigned gramLength  )
{
  set<string> gramSet1;
  str2grams(s1, gramSet1,gramLength);
 
  set<string> gramSet2;
  str2grams(s2, gramSet2,gramLength);
 
  unsigned news1Size = gramSet1.size();
  unsigned news2Size = gramSet2.size();
  gramSet1.insert(gramSet2.begin(),gramSet2.end());
  unsigned unionSize = gramSet1.size();
  unsigned intersectionSize 
	= news1Size+news2Size-unionSize;
 
  float d =
      (2.0*intersectionSize)/(news1Size+news2Size);

  return d;
}//end dice
