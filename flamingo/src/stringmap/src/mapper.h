//
// $Id: mapper.h 5770 2010-10-19 06:38:25Z abehm $
//
//        mapper.h
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


#ifndef __MAPPER_H__
#define __MAPPER_H__

#include <string>
#include <vector>

using namespace std;

typedef vector<double> Coordinates;

// compute the edit distance between two strings
int ed(string s1, string s2);
bool SimilarString(string s1, string s2, int threshold);

class Mapper
{
 public:
  Mapper(vector<string> &, int dimensionality);
  Mapper(const string dataFileName);
  ~Mapper();

  void map(); // map strings to a Euclidiean space
  vector<string> getStrings();

  void saveToFile(const string dataFileName); // save data to a file

  // compute the new coordinates for a given string
  Coordinates mappedCoordinates(string s);

  // compute the distance of two strings in the mapped space
  double mappedDistance(const string s1, const string s2);

  double getNewThreshold(const vector<string> &stringVector1,
			 const vector<string> &stringVector2,
			 const int edThreshold);

  double getNewThreshold(const vector<string> &stringVector1, const double percetange1,
			 const vector<string> &stringVector2, const double percentages,
			 const int edThreshold);

 private:
  vector<string> stringVector;
  vector< Coordinates > coordinatesVector;

  int size;
  double threshold;
  int dimensionality;

  // these two vectors store the pairs of pivots
  vector<int> pivotsA;
  vector<int> pivotsB;

  //double getCoordindate(int stringID, int dimensionID);
  //void   setCoordindate(int stringID, int dimensionID, double coord);

  void   nextsm(int);

  // find a point farthest to the point id using distances up to "dimLimit"
  int  findFarthestPoint(int id, int dimLimit);
  void choosePivot(int dim);

};

#endif // __MAPPER_H__
