/*
  $Id: unittest.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
          Jiaheng Lu
*/

#undef NDEBUG

#include <fstream>
#include <iostream>
#include <cassert>

#include "gram.h"
#include "misc.h"
#include "input.h"
#include "output.h"
#include "io.h"
#include "array.h"
#include "stringshuffler.h"
#include "simfuncs.h"

using namespace std;

void simFuncsTest()
{

  unsigned t = 0;
  
  vector<string> s;
  s.push_back("abc");
  s.push_back("ab");
  s.push_back("ac");
  s.push_back("bc");
  s.push_back("a");
  s.push_back("b");
  s.push_back("c");
  s.push_back("abcdef");
  s.push_back("xyz");
  s.push_back("bac");
  s.push_back("acb");
  s.push_back("ba");
  
  const unsigned n = 12;
  unsigned e[][n] = 
    { {0, 1, 1, 1, 2, 2, 2, 3l, 3, 2, 2, 2},
      {1, 0, 1, 2, 1, 1, 2, 4, 3, 2, 1, 2},
      {1, 1, 0, 1, 1, 2, 1, 4, 3, 1, 1, 2},
      {1, 2, 1, 0, 2, 1, 1, 4, 3, 1, 2, 1},
      {2, 1, 1, 2, 0, 1, 1, 5, 3, 2, 2, 1},
      {2, 1, 2, 1, 1, 0, 1, 5, 3, 2, 2, 1},
      {2, 2, 1, 1, 1, 1, 0, 5, 3, 2, 2, 2},
      {3, 4, 4, 4, 5, 5, 5, 0, 6, 5, 4, 5},
      {3, 3, 3, 3, 3, 3, 3, 6, 0, 3, 3, 3},
      {2, 2, 1, 1, 2, 2, 2, 5, 3, 0, 2, 1},
      {2, 1, 1, 2, 2, 2, 2, 4, 3, 2, 0, 3},
      {2, 2, 2, 1, 1, 1, 2, 5, 3, 1, 3, 0}};

  unsigned eSwap[][n] = 
    { {0, 1, 1, 1, 2, 2, 2, 3, 3, 1, 1, 2},
      {1, 0, 1, 2, 1, 1, 2, 4, 3, 2, 1, 1},
      {1, 1, 0, 1, 1, 2, 1, 4, 3, 1, 1, 2},
      {1, 2, 1, 0, 2, 1, 1, 4, 3, 1, 2, 1},
      {2, 1, 1, 2, 0, 1, 1, 5, 3, 2, 2, 1},
      {2, 1, 2, 1, 1, 0, 1, 5, 3, 2, 2, 1},
      {2, 2, 1, 1, 1, 1, 0, 5, 3, 2, 2, 2},
      {3, 4, 4, 4, 5, 5, 5, 0, 6, 4, 4, 5},
      {3, 3, 3, 3, 3, 3, 3, 6, 0, 3, 3, 3},
      {1, 2, 1, 1, 2, 2, 2, 4, 3, 0, 2, 1},
      {1, 1, 1, 2, 2, 2, 2, 4, 3, 2, 0, 3},
      {2, 1, 2, 1, 1, 1, 2, 5, 3, 1, 3, 0}};

  for (unsigned i = 0; i < n; i++)
    for (unsigned j = 0; j < n; j++) {
//       cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
//            << e[i][j] << endl;
      assert(ed(s[i], s[j]) == e[i][j]); t++;
    }  

  for (unsigned i = 0; i < n; i++)
    for (unsigned j = 0; j < n; j++) {
      assert(ed(s[i], s[j], e[i][j])); t++;
    }

  for (unsigned i = 0; i < n; i++)
    for (unsigned j = 0; j < n; j++) {
      if (e[i][j]) {
        assert(!ed(s[i], s[j], e[i][j] - 1)); t++;
      }
    }

  for (unsigned i = 0; i < n; i++)
    for (unsigned j = 0; j < n; j++) {
//       cout << i << " " << j << " " << s[i] << " " << s[j] << " " 
//            << eSwap[i][j] << " " << edSwap(s[i], s[j]) << endl;
      assert(edSwap(s[i], s[j]) == eSwap[i][j]); t++;
    }  

 
  cout << "ed (" << t << ")" << endl;

  t = 0;

  vector<string> s2;
  s2.push_back("abc");
  s2.push_back("ab");
  s2.push_back("ac");
  s2.push_back("bc");
  s2.push_back("a");
  s2.push_back("b");
  s2.push_back("c");
  s2.push_back("abcdef");
  s2.push_back("xyz");
  s2.push_back("bac");
  s2.push_back("acb");
  s2.push_back("ba");
  
  const unsigned n1 = 12;
  unsigned q = 1;
  float r1[][n1] = 
    { {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3,
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {2. / 3, 1,      1. / 3, 1. / 3,     .5,
           .5,      0, 1. / 3,      0, 2. / 3, 2. / 3,      1},
      {2. / 3, 1. / 3,      1, 1. / 3,     .5,
            0,     .5, 1. / 3,      0, 2. / 3, 2. / 3, 1. / 3},
      {2. / 3, 1. / 3, 1. / 3,      1,      0,
           .5,     .5, 1. / 3,      0, 2. / 3, 2. / 3, 1. / 3},
      {1. / 3,     .5,     .5,      0,      1,
            0,      0, 1. / 6,      0, 1. / 3, 1. / 3,     .5},
      {1. / 3,     .5,      0,     .5,      0,
            1,      0, 1. / 6,      0, 1. / 3, 1. / 3,     .5},
      {1. / 3,      0,     .5,     .5,      0,
            0,      1, 1. / 6,      0, 1. / 3, 1. / 3,      0},
      {    .5, 1. / 3, 1. / 3, 1. / 3, 1. / 6,
       1. / 6, 1. / 6,      1,      0,     .5,     .5, 1. / 3},
      {     0,      0,      0,      0,      0,
            0,      0,      0,      1,      0,      0,      0},
      {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3, 
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {     1, 2. / 3, 2. / 3, 2. / 3, 1. / 3,
       1. / 3, 1. / 3,     .5,      0,      1,      1, 2. / 3},
      {2. / 3,      1, 1. / 3, 1. / 3,     .5,
           .5,      0, 1. / 3,      0, 2. / 3, 2. / 3,      1}};

  unsigned n11 = n1;
  for (unsigned i = 0; i < n11; i++)
    for (unsigned j = 0; j < n11; j++) {
      //cout << i << " " << j << " " << s2[i] << " " << s2[j] << " " 
      //     << r1[i][j] << " " << jaccard(s[i], s[j], q) << endl;
      assert(jaccardNoPrePost(s2[i], s2[j], q) == r1[i][j]); t++;
    }  
  
  s2.clear();
  s2.push_back("abc");
  s2.push_back("ab");
  s2.push_back("ac");
  s2.push_back("bc");
  s2.push_back("abcdef");
  s2.push_back("xyz");
  s2.push_back("bac");
  s2.push_back("acb");
  s2.push_back("ba");
  
  q = 2;
  const unsigned n2 = 9; 
  float r2[][n2] = 
    { {     1,     .5,      0,     .5,     .4,     0,      0,      0,      0},
      {    .5,      1,      0,      0, 1. / 5,     0,      0,      0,      0},
      {     0,      0,      1,      0,      0,     0,     .5,     .5,      0},
      {    .5,      0,      0,      1, 1. / 5,     0,      0,      0,      0},
      {    .4, 1. / 5,      0, 1. / 5,      1,     0,      0,      0,      0},
      {     0,      0,      0,      0,      0,     1,      0,      0,      0},
      {     0,      0,     .5,      0,      0,     0,      1, 1. / 3,     .5},
      {     0,      0,     .5,      0,      0,     0, 1. / 3,      1,      0},
      {     0,      0,      0,      0,      0,     0,     .5,      0,      1}};

  unsigned n22 = n2;
  for (unsigned i = 0; i < n22; i++)
    for (unsigned j = 0; j < n22; j++) {
      //cout << i << " " << j << " " << s2[i] << " " << s2[j] << " " 
      //     << r2[i][j] << " " << jaccard(s2[i], s2[j], q) << endl;
      assert(jaccardNoPrePost(s2[i], s2[j], q) == r2[i][j]); t++;
    }  

  assert(substringed("abcdef", "def", 0) == true); t++;
  assert(substringed("abcdef", "abf", 0) == false); t++;
  assert(substringed("abcdef", "abf", 1) == true); t++;

  assert(substringed("university", "vers", 0) == true); t++;
  assert(substringed("university", "verz", 0) == false); t++;
  assert(substringed("university", "verz", 1) == true); t++;

  assert(substringed("university of california at irvine", "irvine", 0) == true); 
  t++;
  assert(substringed("university of california at irvine", "irivine", 0) == false);
  t++;
  assert(substringed("university of california at irvine", "irivine", 1) == true);
  t++;
  assert(substringed("university of california at irvine", "calxifoyrnia", 1) == 
         false); t++;
  assert(substringed("university of california at irvine", "calxifoyrnia", 2) == 
         true); t++;

  float d1 = normalizedED("test", "test" );

  float d2 = cosine( "test",  "test");

  float d3 = jaccardNoPrePost( "test",  "test");

  float d4 = dice( "test",  "test" );

  assert(d1 == 1.0); t++;
  assert(d2 == 1.0); t++;
  assert(d3 == 1.0); t++;
  assert(d4 == 1.0); t++;

  float d5 = normalizedED("test","test1" );
  float d6 = normalizedED("test","test11" );

  //cout<<"d1 is " <<d1 <<" ; d2 is "<<d2 <<endl;

  assert(d5 >0 && d5 <1 ); t++;
  assert(d6 >0 && d6 <1 ); t++;
  assert(d5>d6); t++;

  d1 = cosine("test","test1");
  d2 = cosine("test","test11");

  //cout<<"d1 is " <<d1 <<" ; d2 is "<<d2 <<endl;

  assert(d1 >0 && d1 <1 ); t++;
  assert(d2 >0 && d2 <1 ); t++;
  assert(d1>d2); t++;

  d1 = jaccardNoPrePost("test","test1" );
  d2 = jaccardNoPrePost("test","test11" );

  //cout<<"d1 is " <<d1 <<" ; d2 is "<<d2 <<endl;

  assert(d1 >0 && d1 <1 ); t++;
  assert(d2 >0 && d2 <1 ); t++;
  assert(d1>d2); t++;

  d1 = dice("test","test1");
  d2 = dice("test","test11" );

  //cout<<"d1 is " <<d1 <<" ; d2 is "<<d2 <<endl;

  assert(d1 >0 && d1 <1 ); t++;
  assert(d2 >0 && d2 <1 ); t++;
  assert(d1>d2); t++;

  cout << "jd (" << t << ")" << endl;
}

void stringShufflerTest()
{
  // generate an input file
  const string inputFile = "stringshufflerunittest.data";
  const string shuffledFile = "stringshufflerunittest.shuffled";
  
  ofstream testFile;
  testFile.open(inputFile.c_str());
  testFile << "This is line 1" << endl;
  testFile << "That is line 2" << endl;
  testFile << "aaaa" << endl;
  testFile << "bbbb" << endl;
  testFile.close();
  
  vector<string> strings;

  // test 1
  ShuffleStrings(inputFile,
		 shuffledFile,
		 4,  // max number that didn't fail: 2M
		 20);
  readString(strings, shuffledFile, 4, 20);
  assert(strings.size() == 4);

  // test 2
  strings.clear();
  ShuffleStrings(inputFile,
		 shuffledFile,
		 4,  // max number that didn't fail: 2M
		 5);
  readString(strings, shuffledFile, 4, 5);
  assert(strings.size() == 2);

  // delete the two files
  assert(system(("\\rm " + inputFile).c_str()));
  assert(system(("\\rm " + shuffledFile).c_str()));
 
}

void arrayTest()
{
  unsigned t = 0;

  Array<unsigned> *array = new Array<unsigned>(5, 10);
  array->append(10);
  array->append(20);
  array->append(30);
  array->append(40);
  array->append(50);
  array->append(60);
  array->append(70);
  array->append(80);

  assert(array->size() == 8); t++;

  assert(array->at(0) == 10); t++;
  assert(array->at(1) == 20); t++;
  assert(array->at(2) == 30); t++;
  assert(array->at(3) == 40); t++;
  assert(array->at(4) == 50); t++;
  assert(array->at(5) == 60); t++;
  assert(array->at(6) == 70); t++;
  assert(array->at(7) == 80); t++;

  assert(array->binarySearch(10) == 0); t++;
  assert(array->binarySearch(15) == 1); t++;
  assert(array->binarySearch(20) == 1); t++;
  assert(array->binarySearch(25) == 2); t++;
  assert(array->binarySearch(30) == 2); t++;
  assert(array->binarySearch(80) == 7); t++;
  assert(array->binarySearch(85) == 8); t++;
  assert(array->binarySearch(90) == 8); t++;

  assert(array->jumpIncRevSearch(10, 6) == 0); t++;
  assert(array->jumpIncRevSearch(15, 4) == 0); t++;
  assert(array->jumpIncRevSearch(20, 7) == 1); t++;
  assert(array->jumpIncRevSearch(25, 3) == 1); t++;
  assert(array->jumpIncRevSearch(30, 3) == 2); t++;
  assert(array->jumpIncRevSearch(80, 8) == 7); t++;
  assert(array->jumpIncRevSearch(85, 8) == 7); t++;
  assert(array->jumpIncRevSearch(90, 8) == 7); t++;

  assert(array->has(10)); t++;
  assert(array->has(20)); t++;
  assert(array->has(30)); t++;
  assert(array->has(40)); t++;
  assert(array->has(50)); t++;

  assert(!array->has(15)); t++;
  assert(!array->has(25)); t++;
  assert(!array->has(35)); t++;
  assert(!array->has(45)); t++;
  assert(!array->has(56)); t++;
  assert(!array->has(67)); t++;
  assert(!array->has(89)); t++;

  delete array;

  Array<double> *array2 = new Array<double>(5, 10);
  array2->append(10.0);
  array2->append(20.0);
  array2->append(30.0);
  array2->append(40.0);
  array2->append(50.0);
  array2->append(60.0);
  array2->append(70.0);
  array2->append(80.0);

  assert(array2->size() == 8); t++;

  assert(array2->at(0) == 10.0); t++;
  assert(array2->at(1) == 20.0); t++;
  assert(array2->at(2) == 30.0); t++;
  assert(array2->at(3) == 40.0); t++;
  assert(array2->at(4) == 50.0); t++;
  assert(array2->at(5) == 60.0); t++;
  assert(array2->at(6) == 70.0); t++;
  assert(array2->at(7) == 80.0); t++;

  array2->erase(3);
  assert(array2->at(0) == 10.0); t++;
  assert(array2->at(1) == 20.0); t++;
  assert(array2->at(2) == 30.0); t++;
  assert(array2->at(3) == 50.0); t++;
  assert(array2->at(4) == 60.0); t++;
  assert(array2->at(5) == 70.0); t++;
  assert(array2->at(6) == 80.0); t++;

  array2->insert(1, 35.5);
  assert(array2->at(0) == 10.0); t++;
  assert(array2->at(1) == 35.5); t++;
  assert(array2->at(2) == 20.0); t++;
  assert(array2->at(3) == 30.0); t++;
  assert(array2->at(4) == 50.0); t++;
  assert(array2->at(5) == 60.0); t++;
  assert(array2->at(6) == 70.0); t++;
  assert(array2->at(7) == 80.0); t++;

  delete array2;

  Array<double> *array3 = new Array<double>(5, 10);
  array3->append(10.0);
  array3->append(20.0);

  assert(array3->size() == 2); t++;

  array3->erase(1);
  array3->erase(0);

  assert(array3->size() == 0); t++;

  delete array3;

  Array<unsigned> array4;

  array4.append(1);
  array4.append(2);
  
  assert(array4[0] == 1); t++;
  assert(array4[1] == 2); t++;
  
  Array<unsigned> array5;

  array5.append(1);
  array5.append(2);
  
  assert(array4 == array5); t++;

  Array<unsigned> array6;
  
  array6.append(1);
  array6.append(2);
  array6.append(3);
  
  vector<unsigned> v_a, v_c;

  copy(array6.begin(), array6.end(), back_inserter(v_a));

  v_c.push_back(1);
  v_c.push_back(2);
  v_c.push_back(3);
  assert(v_a == v_c); t++;

  cout << "array (" << t << ")" << endl;  
}

void gramTest()
{
  unsigned t = 0;

  // str2grams, str2gramsHash, and grams2str
  vector<string> gramsCor, gramsRes;
  string strRes;
  gramsCor.push_back(string(1, PREFIXCHAR) + string(1, PREFIXCHAR) + "a");
  gramsCor.push_back(string(1, PREFIXCHAR) + "ab");
  gramsCor.push_back("abc");
  gramsCor.push_back("bc" + string(1, SUFFIXCHAR));
  gramsCor.push_back("c" + string(1, SUFFIXCHAR) + string(1, SUFFIXCHAR));
  str2grams("abc", gramsRes);
  assert(gramsRes == gramsCor); t++;
  grams2str(gramsCor, strRes);
  assert(strRes == "abc"); t++;

  vector<unsigned> gramsHash, gramsHashRes;
  for (vector<string>::const_iterator gram = gramsCor.begin();
       gram != gramsCor.end(); ++gram)
    gramsHash.push_back(hashString(*gram));
  
  str2grams("abc", gramsHashRes);
  assert(gramsHashRes == gramsHash); t++;  

  gramsCor.clear();
  gramsCor.push_back("%%%a");
  gramsCor.push_back("%%ab");
  gramsCor.push_back("%abc");
  gramsCor.push_back("abcd");
  gramsCor.push_back("bcd@");
  gramsCor.push_back("cd@@");
  gramsCor.push_back("d@@@");
  gramsRes.clear();
  str2grams("abcd", gramsRes, 4, '%', '@');
  assert(gramsRes == gramsCor); t++;
  grams2str(gramsCor, strRes, 4);
  assert(strRes == "abcd"); t++;

  gramsHash.clear();
  for (vector<string>::const_iterator gram = gramsCor.begin();
       gram != gramsCor.end(); ++gram)
    gramsHash.push_back(hashString(*gram));
  gramsHashRes.clear();
  str2grams("abcd", gramsHashRes, 4, '%', '@');
  assert(gramsHashRes == gramsHash); t++;

  // gram2id &  id2gram
  assert(gram2id("abc") == 207804); t++;
  id2gram(207804, strRes, 3);
  assert(strRes == "abc"); t++;

  // GramId
  GramId gid(3, PREFIXCHAR, SUFFIXCHAR, "abc", false);

  assert(gid.getId(string(1, PREFIXCHAR) + string(1, PREFIXCHAR)
                   + string(1, PREFIXCHAR)) == 0); t++;
  assert(gid.getId(string(1, PREFIXCHAR) + string(1, PREFIXCHAR) + "a") == 1); t++;
  assert(gid.getId("aaa") == 31); t++;
  assert(gid.getId("abc") == 38); t++;
  assert(gid.getId("cba") == 86); t++;

  assert(gid.getGram(0) == string(1, PREFIXCHAR) + string(1, PREFIXCHAR)
         + string(1, PREFIXCHAR)); t++;
  assert(gid.getGram(1) == string(1, PREFIXCHAR) + string(1, PREFIXCHAR) + "a"); t++;
  assert(gid.getGram(31) == "aaa"); t++;
  assert(gid.getGram(38) == "abc"); t++;
  assert(gid.getGram(86) == "cba"); t++;

  vector<unsigned> idsCor, idsRes;
  idsCor.clear();
  idsCor.push_back(1);
  idsCor.push_back(6);
  idsCor.push_back(32);
  idsCor.push_back(37);
  idsCor.push_back(63);
  idsCor.push_back(68);
  idsCor.push_back(94);
  idsCor.push_back(99);
  idsRes.clear();
  gid.getIds("aabbcc", idsRes);
  assert(idsRes == idsCor); t++;

  gramsCor.clear();
  gramsCor.push_back(string(1, PREFIXCHAR) + string(1, PREFIXCHAR) + "a");
  gramsCor.push_back(string(1, PREFIXCHAR) + "aa");
  gramsCor.push_back("aab");
  gramsCor.push_back("abb");
  gramsCor.push_back("bbc");
  gramsCor.push_back("bcc");
  gramsCor.push_back("cc" + string(1, SUFFIXCHAR));
  gramsCor.push_back("c" + string(1, SUFFIXCHAR) + string(1, SUFFIXCHAR));
  gramsRes.clear();
  gid.getGrams(idsCor, gramsRes);
  assert(gramsRes == gramsCor); t++;
  grams2str(gramsCor, strRes);
  assert(strRes == "aabbcc"); t++;

  string filenamePrefix = "dataset";
  GramId gidSave = GramId();
  gidSave.saveData(filenamePrefix);
  GramId gidLoad = GramId(filenamePrefix);
  assert(gidSave == gidLoad); t++;

  // str2words
  vector<string> words, wordsRes;
  words.push_back("abc");
  words.push_back("de");
  words.push_back("f");
  
  str2words("abc de f", wordsRes);
  assert(wordsRes == words); t++;
  wordsRes.clear();
  str2words("abc\tde\tf", wordsRes);
  assert(wordsRes == words); t++;
  wordsRes.clear();
  str2words("abc \tde\t f", wordsRes);
  assert(wordsRes == words); t++;
  wordsRes.clear();
  str2words("abc \t de\t \tf", wordsRes);
  assert(wordsRes == words); t++;
  wordsRes.clear();
  str2words("\t \tabc \t de\t \tf \t ", wordsRes);
  assert(wordsRes == words); t++;

  // WordIndex
  WordHash wordHash;
  vector<string> data;
  data.push_back("abc cd");
  data.push_back("cd def");
  data.push_back("abc");
  WordIndex::build(data, wordHash);

  Ids s;
  s.insert(0);
  s.insert(2);
  assert(wordHash["abc"] == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordHash["cd"] == s); t++;
  s.erase(0);
  assert(wordHash["def"] == s); t++;
  
  wordHash.clear();
  WordIndex::build("dataset.txt", wordHash);
  s.clear();
  s.insert(0);
  s.insert(2);
  assert(wordHash["abc"] == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordHash["cd"] == s); t++;
  s.erase(0);
  assert(wordHash["def"] == s); t++;

  WordIndex::save("dataset.words.txt", 
                  "dataset.ids.bin", wordHash);
  wordHash.clear();
  WordIndex::load("dataset.words.txt", 
                  "dataset.ids.bin", wordHash);
  s.clear();
  s.insert(0);
  s.insert(2);
  assert(wordHash["abc"] == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordHash["cd"] == s); t++;
  s.erase(0);
  assert(wordHash["def"] == s); t++;

  WordIds wordIds;
  WordKey wordKey;
  WordIndex::build(data, wordIds, wordKey);

  s.clear();
  s.insert(0);
  s.insert(2);
  assert(wordIds[wordKey["abc"]].second == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordIds[wordKey["cd"]].second == s); t++;
  s.erase(0);
  assert(wordIds[wordKey["def"]].second == s); t++;

  wordIds.clear();
  wordKey.clear();
  WordIndex::build("dataset.txt", wordIds, wordKey);
  s.clear();
  s.insert(0);
  s.insert(2);
  assert(wordIds[wordKey["abc"]].second == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordIds[wordKey["cd"]].second == s); t++;
  s.erase(0);
  assert(wordIds[wordKey["def"]].second == s); t++;

  WordIndex::save("dataset.wids.bin", "dataset.wkey.txt", 
                  wordIds, wordKey);
  wordIds.clear();
  wordKey.clear();
  WordIndex::load("dataset.wids.bin", "dataset.wkey.txt", 
                  wordIds, wordKey);
  s.clear();
  s.insert(0);
  s.insert(2);
  assert(wordIds[wordKey["abc"]].second == s); t++;
  s.erase(2);
  s.insert(1);
  assert(wordIds[wordKey["cd"]].second == s); t++;
  s.erase(0);
  assert(wordIds[wordKey["def"]].second == s); t++;

  vector<string> wordVectRes, wordVectCor;
  vector<Ids> idsVectRes, idsVectCor;
  WordKey wordPosMapRes, wordPosMapCor;
  wordVectCor.push_back("abc");
  wordVectCor.push_back("cd");
  wordVectCor.push_back("def");
  Ids ids;
  ids.insert(0);
  ids.insert(2);
  idsVectCor.push_back(ids);
  ids.clear();
  ids.insert(0);
  ids.insert(1);
  idsVectCor.push_back(ids);
  ids.clear();
  ids.insert(1);
  idsVectCor.push_back(ids);
  ids.clear();
  wordPosMapCor["abc"] = 0;
  wordPosMapCor["cd"] = 1;
  wordPosMapCor["def"] = 2;
  WordIndex::build(data, wordVectRes, idsVectRes, wordPosMapRes);
  assert(wordVectRes == wordVectCor); t++;
  assert(idsVectRes == idsVectCor); t++;
  for (WordKey::const_iterator el = wordPosMapRes.begin();
       el != wordPosMapRes.end(); ++el) {
    assert(wordPosMapCor.find(el->first) != wordPosMapCor.end()); 
    assert(wordPosMapCor[el->first] == el->second); 
  } t++;
  wordVectRes.clear();
  idsVectRes.clear();
  wordPosMapRes.clear();

  WordIndex::save("dataset", wordVectCor, idsVectCor, wordPosMapCor);  
  WordIndex::load("dataset", wordVectRes, idsVectRes, wordPosMapRes);
  assert(wordVectRes == wordVectCor); t++;
  assert(idsVectRes == idsVectCor); t++;
  for (WordKey::const_iterator el = wordPosMapRes.begin();
       el != wordPosMapRes.end(); ++el) {
    assert(wordPosMapCor.find(el->first) != wordPosMapCor.end()); 
    assert(wordPosMapCor[el->first] == el->second); 
  } t++;
  wordVectRes.clear();
  idsVectRes.clear();
  wordPosMapRes.clear();  

  cout << "gram (" << t << ")" << endl;
}

void miscTest()
{
  unsigned t = 0;

  assert(pow(static_cast<unsigned>(2), static_cast<unsigned>(0)) == 1); t++;
  assert(pow(static_cast<unsigned>(2), static_cast<unsigned>(1)) == 2); t++;
  assert(pow(static_cast<unsigned>(2), static_cast<unsigned>(2)) == 4); t++;

  vector<vector<unsigned> > subs;
  vector<unsigned> sub;
  sub.push_back(0);
  sub.push_back(1);
  subs.push_back(sub);
  sub[1] = 2;
  subs.push_back(sub);
  sub[0] = 1;
  sub[1] = 2;
  subs.push_back(sub);
  assert(subsets(3, 2) == subs); t++;
  
  subs.clear();
  sub.clear();
  sub.push_back(0);
  subs.push_back(sub);
  sub[0] = 1;
  subs.push_back(sub);
  sub[0] = 2;
  subs.push_back(sub);
  assert(subsets(3, 1) == subs); t++;

  assert(min(1, 2, 3) == 1); t++;
  assert(min(1, 3, 2) == 1); t++;
  assert(min(2, 1, 3) == 1); t++;
  assert(min(2, 3, 1) == 1); t++;
  assert(min(3, 1, 2) == 1); t++;
  assert(min(3, 2, 1) == 1); t++;
  assert(min(1, 2, 2) == 1); t++;
  assert(min(2, 1, 2) == 1); t++;
  assert(min(2, 2, 1) == 1); t++;

  assert(utos(10) == "10"); t++;
  assert(utosh(10) == "10"); t++;
  assert(utosh(1000) == "1k"); t++;

  UnsignedSeq s;
  assert(s() == 0); t++;
  assert(s() == 1); t++;
  s = UnsignedSeq(10);
  assert(s() == 10); t++;
  assert(s() == 11); t++;

  assert(removeExt("a.txt") == "a"); t++;
  assert(removeExt("a.bin") == "a"); t++;

  unordered_map<unsigned, string> a, b;
  a.insert(make_pair(1, "a"));
  a.insert(make_pair(2, "b"));
  a.insert(make_pair(3, "c"));
  b.insert(make_pair(1, "a"));
  b.insert(make_pair(2, "b"));
  b.insert(make_pair(3, "c"));
  
  assert(equalElements(a, b)); t++;
  
  cout << "misc (" << t << ")" << endl;
}

void createIdPosInvertedListsTest()
{
  vector<string> data;
 
  data.push_back("abcabc");
  data.push_back("abd");
  data.push_back("bcd");

  GramListMap idLists;
  GramListMap posLists;
			    
  createIdPosInvertedLists(data,false,idLists,posLists);

  cout<<"Now test for create (id,pos) pair :"<<endl;

  assert(idLists.size() == posLists.size());
  
  for (  GramListMap::iterator iterId = idLists.begin();
	   iterId != idLists.end(); ++iterId )
    {
      unsigned gram = iterId->first;
      
      //cout<<"Gram hash is "<<gram<<endl;
      
      GramListMap::iterator iterPos = posLists.find(gram);

      assert(iterPos != posLists.end());
      
      Array<unsigned> *posList = iterPos->second;
      Array<unsigned> *idList = iterId->second;

      assert(posList->size() == idList->size());

    //  for(unsigned i=0;i<posList->size();i++)
	//{
	  //cout<<" ("<<idList->at(i)<<","
	    //  <<posList->at(i)<<"), ";
      
	//}

   //   cout<<endl;
      
    }//end for


  //free space
  for (GramListMap::iterator iterId = idLists.begin();
       iterId != idLists.end(); ++iterId )
    delete iterId->second;

  for (GramListMap::iterator iterId = posLists.begin();
       iterId != posLists.end(); ++iterId )
    delete iterId->second;

  cout<<"...OK"<<endl;

}//end  createIdPosInvertedListsTest

void subedTest()
{
 
}//end subtexts

void ioTest() 
{
  unsigned t = 0;

  vector<double> data1;
  data1.push_back(1);
  data1.push_back(-2);
  data1.push_back(5);
  
  writeBin(data1, "iotest.bin");
  
  vector<double> data2;
  readBin(data2, "iotest.bin");
  
  assert(data1 == data2); t++;

  // === text read/write === 

  vector<string> s_o;

  s_o.push_back("a");
  s_o.push_back("b");
  s_o.push_back("c");

  writeTextFile<string>("test.txt", s_o.begin(), s_o.end());

  vector<string> s_i;

  readTextFile<string>("test.txt", back_inserter(s_i));


  assert(s_o == s_i); t++;


  vector<float> f_o;
  f_o.push_back(.1);
  f_o.push_back(.2);
  f_o.push_back(.3);

  writeTextFile<float>("test.txt", f_o.begin(), f_o.end());
  

  vector<float> f_i;

  readTextFile<float>("test.txt", back_inserter(f_i));

  
  assert(f_o == f_i); t++;

  
  remove("test.txt");


  // === binary read/write === 

  writeBinaryFile<float>("test.bin", f_o.begin(), f_o.end());
  

  f_i.clear();

  readBinaryFile<float>("test.bin", back_inserter(f_i));

  
  assert(f_o == f_i); t++;

  
  remove("test.bin");


  // === binary read -- iterator === 

  writeBinaryFile<float>("test.bin", f_o.begin(), f_o.end());

  ifstream *i = new ifstream("test.bin", ios::in | ios::binary);
  istream_iterator<binary_io<float> > j(*i);

  float f;
  f = *j;
  j++;
  assert(f == .1f); t++;
  
  float g;
  g = *j;
  assert(g == .2f); t++;
  
  f = *j;
  assert(f == .2f); t++;

  delete i;  


  remove("test.bin");


  cout << "io (" << t << ")" << endl;
}

int main() 
{
  cout << "test..." << endl;

  gramTest(); //gram.h
  miscTest(); //misc.h
  ioTest(); //io.h
  arrayTest(); //array.h
  simFuncsTest(); // simfuncs.h

  //  stringShufflerTest(); //stringshuffler.h
  //createIdPosInvertedListsTest(); // createIdPosInvertedLists.h
  
  cout << "OK" << endl;
}
