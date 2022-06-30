//
// $Id: stringmap_unittest.cpp 5770 2010-10-19 06:38:25Z abehm $
//
//        stringmap_unittest.cpp                                       
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: 
//          Chen Li <chenli (at) ics.uci.edu>
//          Liang Jin <liangj (at) ics.uci.edu>
//

#include <fstream>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/times.h>
#include <unistd.h>

#include "stringmap.h"

using namespace std;


void readStrings(vector<string> &stringVector, string fileName)
{
  string line;
  ifstream infile(fileName.c_str());
  while (getline(infile, line, '\n'))
    stringVector.push_back (line);
  infile.close();

  cout << fileName << " has " << stringVector.size() << " strings." << endl;

  if (stringVector.size() == 0) {
    cout << "\"source.txt\" file does not exist or it is empty." << endl
         << "unzip \"source.zip\" and try again." << endl;
    exit(1);
  }
}


// testing the join implementation
bool JoinTest(int argc, char *argv[])
{
  string mapperFileName("mapper.bin");
  StringMap *sm;
  vector<string> stringVector1, stringVector2, allStrings;

  int useSavedFiles = false;
  cout << "Usage: " << argv[0] << " [load]" << endl;
  if (argc==2 && 0==strcmp(argv[1],"load")) // use old tree
    useSavedFiles = true;

  // read in the two lists of strings from two files
  string srcFileName1("source1.txt");
  readStrings(stringVector1, srcFileName1);

  string srcFileName2("source2.txt");
  readStrings(stringVector2, srcFileName2);
      
  if (useSavedFiles) // use the saved mapper
    {
      sm = new StringMap(mapperFileName);
      // we assume these read strings are the same as the those in the two files
      allStrings = sm->getStrings();
    }
  else // build a mapper from scratch
    {
      // combine them into one vector
      for (int i = 0; i < stringVector1.size(); i ++)
	allStrings.push_back(stringVector1[i]);
      for (int i = 0; i < stringVector2.size(); i ++)
	allStrings.push_back(stringVector2[i]);

      // create a StringMap
      sm = new StringMap(allStrings);

      // map the strings to a new Euclidean space
      sm->Map();
      sm->saveMapperToFile(mapperFileName);
    }

  int edThreshold = 2;

  // Compute the new threshold in the new Euclidean space by doing sampling.
  // The two percentages can be reduced (to save time) if the two lists
  // are large.
  float newThreshold = sm->getNewThreshold(stringVector1, 0.50,
					   stringVector2, 0.50,
					   edThreshold);

  // this flag indicates whether we need to counter duplicate-string pairs
  //bool countingDuplicates = false;
  bool countingDuplicates = true;

  // do an approximixate string join (using R-trees internally)
  int numberFoundPairs = sm->Join(stringVector1, stringVector2,
				  edThreshold,
				  newThreshold,
				  "result-join.txt",
				  countingDuplicates);

  // compute the recall of mapping these strings
  cout << "Computing recall: " << endl;
  int numberSimilarPairs = sm->SimilarPairs(stringVector1,
					    stringVector2,
					    edThreshold,
					    countingDuplicates);
  cout << " # of similar pairs: " << numberSimilarPairs << endl;
  cout << " # of similar pairs found by Stringmap: " << numberFoundPairs << endl;
  double recall = 1;
  if (numberSimilarPairs > 0)
    recall = (float)numberFoundPairs / (float) numberSimilarPairs;
  cout << " Recall is  " << recall << endl;

  delete sm;

  return true;
}

// testing the implementation to support search for a single string
bool SingleStringTest(int argc, char *argv[])
{
  // read in the two lists of strings from two files
  string mapperFileName("mapper.bin");
  StringMap *sm;
  vector<string> stringVector;

  int useSavedFiles = false;
  cout << "Usage: " << argv[0] << " [load]" << endl;
  if (argc==2 && 0==strcmp(argv[1],"load")) // use old tree
    useSavedFiles = true;

  if (useSavedFiles) // use the saved mapper
    {
      sm = new StringMap(mapperFileName);
      stringVector = sm->getStrings();
    }
  else // build a mapper from scratch
    {
      string srcFileName("source.txt");
      readStrings(stringVector, srcFileName);
      
      // create a StringMap
      sm = new StringMap(stringVector);
      
      // map the strings to a new Euclidean space
      sm->Map();
      sm->saveMapperToFile(mapperFileName);
    }

  int edThreshold = 2;
  // Compute the new threshold in the new Euclidean space by doing sampling.
  // The percentage can be reduced (to save time) if the data set is large
  float newThreshold = sm->getNewThreshold(stringVector, 0.5,
					   edThreshold);

  string rtreeFileName("rtreesearch.bin");
  if (useSavedFiles) // use the saved RTree
    {
      sm->LoadSearchRTree(stringVector, rtreeFileName);
    }
  else
    {
      cout << "Constructing an R-trees ...." << endl;
      sm->ConstructSearchRtree(stringVector, rtreeFileName);
    }

  // this flag indicates whether we need to counter duplicate-string pairs
  //bool countingDuplicates = false;
  bool countingDuplicates = true;

  double totalRecall = 0;
  int queryNumber = 50;
  float totalStringMapTime = 0;
  float totalNaiveTime = 0;
  clock_t startTime, endTime;
  float diffTime;
  struct tms buff;

  for (register int i = 0; i < queryNumber; i ++)
    {
      int id = rand() % stringVector.size();
      string queryString = stringVector[id];

      cout << "\nSearching for [" << queryString << "]" << endl
	   << "-----------------" << endl;
      startTime = times(&buff);
      int numberFoundStrings = sm->Search(queryString,
					  edThreshold,
					  newThreshold,
					  countingDuplicates);
      endTime = times(&buff);
      float stringMapTime = static_cast<float>(endTime - startTime) / sysconf(_SC_CLK_TCK);
      totalStringMapTime += stringMapTime;
      
      // compute the recall of mapping these strings
      // use a naive approach
      cout << "-----------------" << endl;
      startTime = times(&buff);
      int numberSimilarStrings = sm->SimilarStrings(queryString,
						    stringVector,
						    edThreshold,
						    countingDuplicates);
      endTime = times(&buff);
      float naiveTime = static_cast<float>(endTime - startTime) / sysconf(_SC_CLK_TCK);
      totalNaiveTime += naiveTime;
      
      double recall;
      if (numberSimilarStrings > 0)
	recall = (float)numberFoundStrings / (float) numberSimilarStrings;
      else
	recall = 1;

      cout << "Recall is " << numberFoundStrings << "/" << numberSimilarStrings
	   << " = " << recall
	   << ". StringMap time = "<< stringMapTime
	   << " seconds. Naive time " << naiveTime << " secs." << endl;

      totalRecall += recall;
    }

  cout << "\nAvg String Map time " << totalStringMapTime / queryNumber
       << ". Naive time " << totalNaiveTime / queryNumber << endl;
  cout << "Avg recall " << totalRecall / queryNumber << endl;

  delete sm;

  return true;
}


int main(int argc, char *argv[])
{
  //JoinTest(argc, argv);
  SingleStringTest(argc, argv);
}
