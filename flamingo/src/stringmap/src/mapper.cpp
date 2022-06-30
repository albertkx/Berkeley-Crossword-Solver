//
// $Id: mapper.cpp 5770 2010-10-19 06:38:25Z abehm $
//
//        mapper.cpp
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
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <cassert>

#include "mapper.h"
#include "editdistance.h"

using namespace std;

bool getDistanceFlag = false;
static EditDistance f;

int ed(string s1, string s2)
{
  return f.LD(s1.c_str(), s2.c_str());
}

bool SimilarString(string s1, string s2, int threshold)
{
  return f.similar(s1.c_str(), s2.c_str(), threshold);
}

// compute the distance between two strings based on the first
// "dimLimit" (excluding) dimensions.  "coordinates1" and
// "coordinates2" store there coordinates of the two strings up to
// "dimLimit - 1"
static double getDistance(string s1, const Coordinates &coordinates1, 
			  string s2, const Coordinates &coordinates2,
			  int dimLimit)
{
  //if (getDistanceFlag)
  //cout << "In getDistance(). s1 = " << s1 << "; s2 = " << s2 << endl;
  double dist = ed(s1, s2);

  for (int i = 0; i < dimLimit; i ++)
    {
      double x = fabs(coordinates1[i] - coordinates2[i]);
      dist = sqrt(fabs(x * x - dist * dist));
    }

  return dist;
}

Mapper::Mapper(vector<string> &stringVector, int dimensionality)
{
  this->stringVector   = stringVector;
  this->dimensionality = dimensionality;
  this->size           = stringVector.size();

  // make sure those vectors have enough space
  coordinatesVector.resize(size);
  for (register int i = 0; i < size; i ++)
    coordinatesVector[i].resize(dimensionality);
  pivotsA.resize(dimensionality);
  pivotsB.resize(dimensionality);
}


// read the strings, their coordinates, and pivots from a file
Mapper::Mapper(const string dataFileName)
{
  ifstream dataFile(dataFileName.c_str(), ios::in | ios::binary);

  if (!dataFile)
    {
      cout << "Failed to open " << dataFileName << " to read." << endl;
      exit(1);
    }

  // dimensionality
  dataFile.read((char *)&dimensionality, sizeof(dimensionality));

  // strings and coordinates
  dataFile.read((char *)&size, sizeof(size));
  coordinatesVector.resize(size);

  char buffer[200];
  for (register int i = 0; i < size; i ++)
    {
      // string
      int len;
      dataFile.read((char *)&len, sizeof(len));
      dataFile.read(buffer, len);
      buffer[len] = '\0';
      stringVector.push_back(buffer);

      // coordinates
      for (register int j = 0; j < dimensionality; j ++)
	{
	  double coord;
	  dataFile.read((char *)&coord, sizeof(coord));
	  coordinatesVector[i].push_back(coord);
	}
    }

  // pivotsA
  for (register int i = 0; i < dimensionality; i ++)
    {
      int id;
      dataFile.read((char *)&id, sizeof(id));
      pivotsA.push_back(id);
    }

  // pivotsB
  for (register int i = 0; i < dimensionality; i ++)
    {
      int id;
      dataFile.read((char *)&id, sizeof(id));
      pivotsB.push_back(id);
    }

  dataFile.close();
}

void Mapper::saveToFile(const string dataFileName)
{
  ofstream dataFile(dataFileName.c_str(), ios::out | ios::binary);

  if (!dataFile)
    {
      cout << "Failed to open " << dataFileName << " to write." << endl;
      exit(1);
    }

  // dimensionality
  dataFile.write((char *)&dimensionality, sizeof(dimensionality));

  // strings and coordinates
  size = stringVector.size();
  dataFile.write((char *)&size, sizeof(size));

  assert(stringVector.size() == coordinatesVector.size());
  for (register int i = 0; i < stringVector.size(); i ++)
    {
      const char *s = stringVector[i].c_str();
      int len = strlen(s);
      dataFile.write((char *)&len, sizeof(len));
      dataFile.write(s, len);

      assert(dimensionality == coordinatesVector[i].size());
      for (register int j = 0; j < coordinatesVector[i].size(); j ++)
	{
	  double coord = coordinatesVector[i][j];
	  dataFile.write((char *)&coord, sizeof(coord));
	}
    }

  // pivotsA
  assert(pivotsA.size() == dimensionality);
  for (register int i = 0; i < pivotsA.size(); i ++)
    {
      int id = pivotsA[i];
      dataFile.write((char *)&id, sizeof(id));
    }

  // pivotsB
  assert(pivotsB.size() == dimensionality);
  for (register int i = 0; i < pivotsB.size(); i ++)
    {
      int id = pivotsB[i];
      dataFile.write((char *)&id, sizeof(id));
    }

  dataFile.close();
}

Mapper::~Mapper()
{
}

vector<string> Mapper::getStrings()
{
  return stringVector;
}

// find a point farthest to the point id using distances up to "dimLimit"
int Mapper::findFarthestPoint(int id, int dimLimit)
{
  int farthest = 0;
  double longestdist = 0;

  for (int i = 0; i < size; i ++)
    {
      double dist = getDistance(stringVector[i], coordinatesVector[i],
				stringVector[id], coordinatesVector[id],
				dimLimit);
      if(dist > longestdist)
	{
	  farthest = i;
	  longestdist = dist;
	}
    }

  return farthest;
}

void Mapper::choosePivot(int dim)
{
  int seeda;
  int seedb = rand() % size;

  for (int k = 0; k < 5; k ++) //repeat the heuristic 5 times
    {
      seeda = findFarthestPoint(seedb, dim);
      seedb = findFarthestPoint(seeda, dim);
    }

  pivotsA[dim] = seeda;
  pivotsB[dim] = seedb;

  //cout << "Mapper::choosePivot -- dim = " << dim
  //<< " seeda = " << seeda << "; seedb = " << seedb << endl;
}

void Mapper::map()
{
  //  cout << "begin of Mapper.map()." << endl;
  srand((unsigned)time(NULL));
  nextsm(dimensionality);
}

void Mapper::nextsm(int kd)
{
  if ( kd <= 0 )
    return;

  //get the pivot points and record them
  int dim = dimensionality - kd;
  choosePivot(dim);

  // get the IDs of the two pivots of the current dimension
  int idA = pivotsA[dim];
  int idB = pivotsB[dim];

  // get the strings of the two pivots of the current dimension
  string stringA = stringVector[idA];
  string stringB = stringVector[idB];

  // get the coordinates of the two pivots of the current dimension
  Coordinates coordindatesA = coordinatesVector[idA];
  Coordinates coordindatesB = coordinatesVector[idB];

  // compute the distance between the two pivots up to dimension "dim"
  double dab = getDistance(stringA, coordindatesA,
			   stringB, coordindatesB,
			   dim);

  // for each string, compute its dim-th coordinate
  for (int i = 0; i < size; i ++)
    {
      double coord;
      if (dab == 0)
	coord = 0;
      else
	{
	  double x = getDistance(stringVector[i], coordinatesVector[i],
				 stringA, coordindatesA, 
				 dim);
	  double y = getDistance(stringVector[i], coordinatesVector[i],
				 stringB, coordindatesB, 
				 dim);
	  coord = (x * x + dab * dab - y * y)/(2 * dab);
	}

      // set the dim-th coordinate of this string
      coordinatesVector[i][dim] = coord;
    }

  nextsm(kd-1);
}

// compute the new coordinates for a given string
Coordinates Mapper::mappedCoordinates(string s)
{
  Coordinates coordinates;

  getDistanceFlag = true;
  for (int dim = 0; dim < dimensionality; dim ++)
    {
      int idA = pivotsA[dim];
      int idB = pivotsB[dim];

      double dab = getDistance(stringVector[idA], coordinatesVector[idA], 
			       stringVector[idB], coordinatesVector[idB], 
			       dim);

      double x = getDistance(s, coordinates,
			     stringVector[idA], coordinatesVector[idA], 
			     dim);

      double y = getDistance(s, coordinates,
			     stringVector[idB], coordinatesVector[idB], 
			     dim);
      
      double coord = (x * x + dab * dab - y * y)/(2 * dab);

      coordinates.push_back(coord);
    }
  
  return coordinates;
}

// compute the distance between two strings in the mapped space
double Mapper::mappedDistance(string s1, string s2)
{
  Coordinates coordinates1 = mappedCoordinates(s1);
  Coordinates coordinates2 = mappedCoordinates(s2);

  assert(coordinates1.size() == coordinates2.size());

  double dist = 0;
  for (int i = 0; i < dimensionality; i ++)
    {
      double x = coordinates1[i] - coordinates2[i];
      dist += x * x;
    }

  return sqrt(dist);
}

// return the new threshold in the Euclidean space
// by default, use 10% from each set
double Mapper::getNewThreshold(const vector<string> &stringVector1,
			       const vector<string> &stringVector2,
			       const int edThreshold)
{
  return getNewThreshold(stringVector1, 0.10,
			 stringVector2, 0.10,
			 edThreshold);
}

// return the new threshold in the Euclidean space
double Mapper::getNewThreshold(const vector<string> &stringVector1, const double percentage1,
			       const vector<string> &stringVector2, const double percentage2,
			       const int edThreshold)
{
  double newThreshold = 0;
  
  // make the ranomization deterministic
  srand(92697);

  // generate two partial lists
  vector<string> v1;
  for (register int i = 0; i < (int) stringVector1.size() * percentage1; i ++)
  {
    int id = rand() % stringVector1.size();
    //int id = i;
    v1.push_back(stringVector1[id]);
  }

  vector<string> v2;
  for (register int i = 0; i < (int) stringVector2.size() * percentage2; i ++)
  {
    int id = rand() % stringVector2.size();
    //int id = i; //!!!!
    v2.push_back(stringVector2[id]);
  }

  // use a nested loop to find the maximal mapped distance of similar pairs
  cout << "Trying to compute a new threshold using " << v1.size() 
       << " strings from the first set and " << v2.size() 
       << " from the second." << endl;

  for (register int i = 0; i < v1.size(); i ++)
    {
      string s1 = v1[i];
      for (register int j = 0; j < v2.size(); j ++)
	{
	  string s2 = v2[j];
	  int eddist = ed(s1, s2);
	  if (eddist > edThreshold) // not similar, keep searching
	    continue;

	  // ignore those identical-string pairs
	  if (eddist == 0)
	    continue;

	  // found one
	  double dist = mappedDistance(s1, s2);
	  //cout << "[" << s1 << "],[" << s2 << "] = " << dist << endl;
	  if(dist > newThreshold)
	    newThreshold = dist;
	}
    }

  if (newThreshold == 0)
    {
      cout << "Mapper::getNewThreshold(): failed to compute a new distance threshold."
	   << "Possible Reasons: didn't get enough samples from the two lists." << endl;
      exit(1);
    }

  cout << "Mapper::getNewThreshold() = " << newThreshold << endl;

  // increase the threshold slightly since the R-tree join uses "<" while
  // we want to use "<="
  return newThreshold + 1e-5;
}
