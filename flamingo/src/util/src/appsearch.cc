/*
  $Id: appsearch.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.

  Date: 03/16/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "appsearch.h"

#include <sys/stat.h>

#include <fstream>
#include <iostream>

#include "simfuncs.h"
#include "input.h"
#include "misc.h"

const unsigned editdistMin = 1, editdistMax = 3;

string getFilenameQuery(const string pathData, const string nameDataset, 
                        const unsigned queryCount)
{
  return pathData + removeExt(nameDataset) + 
    ".query." + utosh(queryCount) + ".bin";
}

string getFilenameAnswer(const string pathData, const string nameDataset, 
                         const unsigned queryCount, const unsigned editdist)
{
  return pathData + removeExt(nameDataset) +
    ".answer." + utosh(queryCount) + "." + utos(editdist) + ".bin";
}


bool existQuery(const string pathData, const string nameDataset, 
                const unsigned queryCount)
{
  const string filenameQuery = getFilenameQuery(pathData, nameDataset, 
                                                queryCount);
  string filenameAnswer[editdistMax - editdistMin + 1];
  unsigned editdist;
  for (editdist = editdistMin; editdist <= editdistMax; editdist++)
    filenameAnswer[editdist - editdistMin] = 
      getFilenameAnswer(pathData, nameDataset, queryCount, editdist);
  
  ifstream fileQue(filenameQuery.c_str(), ios::in | ios::binary);
  if (!fileQue)
    return false;
  fileQue.close();

  for (editdist = editdistMin; editdist <= editdistMax; editdist++) {
    ifstream fileAns(filenameAnswer[editdist - editdistMin].c_str(), 
                     ios::in | ios::binary);
    if (!fileAns)
      return false;
    fileAns.close();
  }

  struct stat attribQue, attribAns;      
  stat(filenameQuery.c_str(), &attribQue);
  for (editdist = editdistMin; editdist <= editdistMax; editdist++) {
    stat(filenameAnswer[editdist - editdistMin].c_str(), &attribAns);
    if (attribQue.st_mtime > attribAns.st_mtime)
      return false;
  }
  
  return true;
}

void genQuery(const vector<string> &data, 
              const string pathData, const string nameDataset, 
              const unsigned queryCount)
{
  cerr << "que gen"; cerr.flush();

  // query
  const string filenameQuery = getFilenameQuery(pathData, nameDataset, 
                                                queryCount);
  ofstream fileQuery(filenameQuery.c_str(), ios::out | ios::binary);
  if (!fileQuery) {
    cerr << "can't open output file \"" << filenameQuery << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  vector<unsigned> query;
  vector<unsigned> queryA(data.size());
  for (unsigned i = 0; i < queryA.size(); i++)
    queryA[i] = i;
  random_shuffle(queryA.begin(), queryA.end());
    
  for (unsigned i = 0; i < queryCount; i++) {
    query.push_back(queryA[i]);
    fileQuery.write(reinterpret_cast<char*>(&queryA[i]), sizeof(unsigned));
  }
  fileQuery.close();  

  // answer
  for (unsigned editdist = editdistMin; editdist <= editdistMax; editdist++) {
    string filenameAnswer = getFilenameAnswer(pathData, nameDataset,
                                              queryCount, editdist);
    ofstream fileAns(filenameAnswer.c_str(), ios::out | ios::binary);
    if (!fileAns) {
      cerr << "can't open output file \"" << filenameAnswer << "\"" << endl;
      exit(EXIT_FAILURE);
    }

    vector<unsigned> ans;
    unsigned e, k = 0;
    for (vector<unsigned>::const_iterator it = query.begin();
         it != query.end(); ++it) {

      if (k % 10 == 0) {
        cerr << '.'; cerr.flush();
      }
      k++;
    
      ans.clear();
      for (unsigned i = 0; i < data.size(); i++)
        if (ed(data[*it], data[i], editdist))
          ans.push_back(i);

      e = ans.size();
      fileAns.write(reinterpret_cast<char*>(&e), sizeof(unsigned));
      for (vector<unsigned>::const_iterator jt = ans.begin(); jt != ans.end(); ++jt)
        fileAns.write(reinterpret_cast<const char*>(&*jt), sizeof(unsigned));
    }
    fileAns.close();
  }  

  cerr << "OK" << endl;
}

void readQuery(vector<unsigned> &query,
               const string pathData, const string nameDataset, 
               const unsigned queryCount)
{
  const string filenameQuery = getFilenameQuery(pathData, nameDataset,
                                                queryCount);
  cerr << "que " << filenameQuery << endl;
  readBin(query, filenameQuery);
}

void readAnswer(vector<set<unsigned> > &answer,
                const string pathData, const string nameDataset, 
                const unsigned queryCount, const unsigned editdist)
{
  const string filenameAnswer = getFilenameAnswer(pathData, nameDataset,
                                                  queryCount, editdist);
  cerr << "ans " << filenameAnswer << endl;

  ifstream fileAnswer(filenameAnswer.c_str(), ios::in | ios::binary);
  if (!fileAnswer) {
    cerr << "can't open input file \"" << filenameAnswer << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  set<unsigned> ans;
  unsigned n, e;

  while (true) {
    fileAnswer.read(reinterpret_cast<char*>(&n), sizeof(unsigned));
    if (fileAnswer.eof())
      break;

    ans.clear();
    for (unsigned i = 0; i < n; i++) {
      fileAnswer.read(reinterpret_cast<char*>(&e), sizeof(unsigned));
      ans.insert(e);
    }

    answer.push_back(ans);
  }

  fileAnswer.close();
}
