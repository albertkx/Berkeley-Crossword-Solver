//
// $Id: stringmap.h 5770 2010-10-19 06:38:25Z abehm $
//
//        stringmap.h
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
//          Chen Li (chenli (at) ics.uci.edu)
//          Liang Jin (liangj (at) ics.uci.edu)
//

#ifndef __STRINGMAP_H__
#define __STRINGMAP_H__

#include <string>
#include <vector>

using namespace std;

class Mapper;
class Rtree;
class Rect;

//int ed(string s1, string s2); // compute the edit distance between two strings

class StringMap
{
 public:
  StringMap(vector<string> &);
  StringMap(const string mapperFileName);
  ~StringMap();

   // map the passed strings to a Euclidean space. The dimensionality is
   // defined as "NUMDIMS" in "rtreeparams.h"
   void Map();
   void saveMapperToFile(const string mapperFileName); // save data to a file

   vector<string> getStrings();

   // sample portions of two string lists to compute their new maximal
   // Euclidean distance in the new space
   float getNewThreshold(const vector<string> &stringVector1, const double percetange1,
			 const vector<string> &stringVector2, const double percentage2,
			 const int edThreshold);

   // treat the list as two lists (self join)
   float getNewThreshold(const vector<string> &stringVector, const double percetange,
			 const int edThreshold);

   // join the two string lists using the old and new distances. return
   // the number of similar pairs found. "countingDuplicates" indicates
   // whether we should consider those identical-string pairs
   int Join(const vector<string> &stringVector1,
	    const vector<string> &stringVector2,
	    const int edThreshold,
	    float newThreshold, 
	    const string resultFileName,
	    const bool countingDuplicates);

   // compute the real number of similar pairs. 
   // "countingDuplicates" indicates whether we should consider those identical-string pairs
   int SimilarPairs(const vector<string> &stringVector1,
		    const vector<string> &stringVector2,
		    int edThreshold,
		    bool countingDuplicates);

   // construct an RTree to support single-string queries
   void ConstructSearchRtree(const vector<string> &stringVector, string rtreeFileName);

   // load the saved search R-tree, assuming their strings are those in
   //the mapper
   void LoadSearchRTree(const vector<string> &searchStrings,
			string rtreeFileName);

   // find strings similar to a given string using StringMap. return
   // the number of similar strings found. "countingDuplicates" indicates
   // whether we should consider those identical-string pairs
   int Search(const string queryString,
	      int edThreshold,
	      float newThreshold,
	      bool countingDuplicates);

   int SimilarStrings(const string queryString,
		      const vector<string> &stringVector,
		      const int edThreshold,
		      bool countingDuplicates);
 private:
   Mapper *mapper;
   Rtree *rtreeSearch;
   const vector<string> *searchStrings;

   void ConstructRect(string s, Rect &rect);

   void ConstructRTree(const vector<string> &stringVector, Rtree *&rtree,
		       string rtreeFileName);

   int RTreeJoin(const vector<string> &stringVector1,
		 const vector<string> &stringVector2,
		 const int edThreshold,
		 Rtree *rtree1, 
		 Rtree *rtree2,
		 float newThreshold,
		 const bool countingDuplicates,
		 const string resultFileName);
};

#endif // __STRINGMAP_H__
