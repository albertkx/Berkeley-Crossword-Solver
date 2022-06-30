//
// $Id: stringmap.cpp 5770 2010-10-19 06:38:25Z abehm $
//
//        stringmap.cpp
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
#include <string>
#include <vector>

#include "rtreeparams.h"
#include "editdistance.h"
#include "index.h"
#include "mapper.h"

#include "stringmap.h"

using namespace std;

#define DUMMYMAXNUM (1000000)


//------------------------------------------------------------
// Constructor
//------------------------------------------------------------
StringMap::StringMap(vector<string> &allStrings)
{
  mapper = new Mapper(allStrings, NUMDIMS);
  rtreeSearch = NULL;
}

StringMap::StringMap(const string mapperFileName)
{
  mapper = new Mapper(mapperFileName);
  rtreeSearch = NULL;
}

vector<string> StringMap::getStrings()
{
  return mapper->getStrings();
}

//------------------------------------------------------------
// Destructor
//------------------------------------------------------------
StringMap::~StringMap()
{
  if (rtreeSearch != NULL) delete rtreeSearch;
  if (mapper != NULL) delete mapper;
}

void StringMap::Map()
{
  cout << "Mapper.map() ... " << endl << endl;
  mapper->map();
}

// save data to a file
void StringMap::saveMapperToFile(const string mapperFileName)
{
  mapper->saveToFile(mapperFileName);
}

float StringMap::getNewThreshold(const vector<string> &stringVector, const double percentage,
				  const int edThreshold)
{
  return getNewThreshold(stringVector, percentage,
			 stringVector, percentage,
			 edThreshold);
  
}

float StringMap::getNewThreshold(const vector<string> &stringVector1, const double percentage1,
				  const vector<string> &stringVector2, const double percentage2,
				  const int edThreshold)
{
  cout << "StringMap.getNewThreshold() ... " << endl << endl;
  float newThreshold = mapper->getNewThreshold(stringVector1, percentage1,
						stringVector2, percentage2,
						edThreshold);
  return newThreshold;
}


int StringMap::Join(const vector<string> &stringVector1,
		    const vector<string> &stringVector2,
		    const int edThreshold, 
		    float newThreshold, 
		    const string resultFileName,
		    const bool countingDuplicates)
{
  cout << "Constructing two R-trees ...." << endl << endl;
  Rtree *rtree1 = new Rtree;
  ConstructRTree(stringVector1, rtree1, "rtree1.bin");

  Rtree *rtree2 = new Rtree;
  ConstructRTree(stringVector2, rtree2, "rtree2.bin");

  cout << "Querying the R-trees ...." << endl << endl;
  int numberFoundPairs = RTreeJoin(stringVector1,
				   stringVector2,
				   edThreshold,
				   rtree1,
				   rtree2, 
				   newThreshold,
				   countingDuplicates,
				   resultFileName
				   );

  cout << "Free space ... " << endl;
  delete rtree2;
  delete rtree1;
  cout << "Done. Results are in result.txt." << endl << endl;

  return numberFoundPairs;
}

void StringMap::ConstructRect(string s, Rect &rect)
{
  Coordinates coordinates = mapper->mappedCoordinates(s);

  //cout << "string " << id 
  //<< " (" << stringVector[id] << ") is mapped to:\t";
  for (register int i = 0; i < NUMDIMS; i++) 
    {
      rect.boundary[i] = coordinates[i];
      rect.boundary[i + NUMDIMS] = coordinates[i];
      //cout << rect.boundary[i] << "\t";
    }
}


void StringMap::ConstructRTree(const vector<string> &stringVector, Rtree *&rtree,
			       string persistentFile)
{

  for (register int id = 0; id < stringVector.size(); id ++)
    {
      Rect rect;
      ConstructRect(stringVector[id], rect);

      //for (register int i = 0; i < NUMDIMS * 2; i++) 
      //cout << rect.boundary[i] << "\t";
      //cout << endl;

      // for some reason the R-tree implementation doesn't like an ID of
      // 0, so we have to replace 0 with a dummy maximual number
      int tid = id;
      if (id == 0) tid = DUMMYMAXNUM;
      rtree->RTreeInsertRect(&rect, tid);
    }

  // for some reason, this R-tree implementation requires us to dump the R-tree
  // file and then reload it from disk. Let's do it to make it happy.
  rtree->RTreeDump((char *)persistentFile.c_str()); // now, make the tree persistent
  delete rtree;
  rtree = new Rtree;
  rtree->RTreeLoad((char *)persistentFile.c_str());
}

// ------------------------------------------------------------
// Return the # of similar pairs
// ------------------------------------------------------------
int StringMap::RTreeJoin(const vector<string> &stringVector1,
			 const vector<string> &stringVector2,
			 const int edThreshold,
			 Rtree *rtree1, 
			 Rtree *rtree2,
			 float newThreshold,
			 const bool countingDuplicates,
			 const string resultFileName)
{
  DistanceFunction df;
  join_pair_result join_result;
  int numberFoundPairs = 0;

  // parameters for collecting performance info:
  long  disk_acc1[10], disk_acc2[10], cpu_comps[10], total_cpus;
  float distance = 0.0;
  int   result=1;

  //float delta = DELTA;  // maximum mapped distance between pairs
  // open the join
  rtree1->SearchJoinOpen(rtree2, &df, &newThreshold,
			 disk_acc1, 10, disk_acc2, 10,
			 cpu_comps, 10, &total_cpus);

  int falsepositive=0;
  ofstream out;
  out.open(resultFileName.c_str());
  while (rtree1->SearchJoinNext(distance, join_result) == 1)
    {
      int id1 = join_result.first.the_id;
      int id2 = join_result.second.the_id;

      // we have to convert a dummy maximal number back to 0 since we have
      // replaced id 0 with this dummy number when constructing the R-trees
      if( id1 == DUMMYMAXNUM ) id1 = 0;
      if( id2 == DUMMYMAXNUM ) id2 = 0;

      /*int eddist = ed(stringVector1[id1], stringVector2[id2]);
	if (eddist <= edThreshold
	&& (countingDuplicates || eddist > 0)) // check if need to ignore those identical-string pairs*/
      if (SimilarString(stringVector1[id1], stringVector2[id2], edThreshold)
	  && (countingDuplicates || stringVector1[id1] != stringVector2[id2])) // check if need to ignore those identical-string pairs
	{
	  out << stringVector1[id1] << endl;
	  out << stringVector2[id2] << endl << endl;
	  numberFoundPairs ++;

	  double newDist = mapper->mappedDistance(stringVector1[id1], stringVector2[id2]);
	  //cout << "Found: [" << stringVector1[id1] << "],[" 
	  //<< stringVector2[id2] << "] = " << newDist << endl;
	}
      else
	falsepositive++;
  }
  out.close();

  rtree1->SearchJoinClose(); // cleanup


  /*out.open(ERRORFILE);
  out<<falsepositive;
  out.close();*/
 
 return numberFoundPairs;
}

// compute the real number of similar pairs
int StringMap::SimilarPairs(const vector<string> &stringVector1,
			    const vector<string> &stringVector2,
			    int edThreshold,
			    bool countingDuplicates)
{
  int count = 0;

  cout << "Computing # of similar pairs. Be patient, since it may take O(n^2) time ... " << endl << endl;
  for (int i = 0; i < stringVector1.size(); i ++)
    for (int j = 0; j < stringVector2.size(); j ++)
      {
	/*int eddist = ed(stringVector1[i], stringVector2[j]);
	if (eddist <= edThreshold
	&& (countingDuplicates || eddist > 0)) // check if need to ignore those identical-string pairs*/
      if (SimilarString(stringVector1[i], stringVector2[j], edThreshold)
	  && (countingDuplicates || stringVector1[i] != stringVector2[j])) // check if need to ignore those identical-string pairs
	{
	  //cout << stringVector1[i] << " || " << stringVector2[j] << endl;
	  count ++;
	}
      }

  return count;
}



void StringMap::ConstructSearchRtree(const vector<string> &searchStrings,
				     string rtreeFileName)
{
  rtreeSearch = new Rtree;
  ConstructRTree(searchStrings, rtreeSearch, rtreeFileName);
  this->searchStrings = &searchStrings;
}

void StringMap::LoadSearchRTree(const vector<string> &searchStrings,
				string rtreeFileName)
{
  rtreeSearch = new Rtree;
  rtreeSearch->RTreeLoad((char *)rtreeFileName.c_str());
  this->searchStrings = &searchStrings;
}

int StringMap::Search(const string queryString,
		      int edThreshold,
		      float newThreshold,
		      bool countingDuplicates)
{
  if (rtreeSearch == NULL)
    {
      cout << "You need to call ConstructSearchRtree() first to build "
	   << "the RTree before you can do a search." << endl;
      return 0;
    }

  Coordinates coordinates = mapper->mappedCoordinates(queryString);
  vector<int> answers;

  struct Point queryPoint;
  for (register int i = 0; i < NUMDIMS; i++) 
    queryPoint.position[i]= coordinates[i];
  rtreeSearch->RTreeSearchSphere(&queryPoint, newThreshold, answers);

  /*// use the old rectanngle search
  Rect rect;
  for (register int i = 0; i < NUMDIMS; i++) 
    {
      rect.boundary[i] = coordinates[i] - newThreshold;
      rect.boundary[i + NUMDIMS] = coordinates[i] + newThreshold;
    }
    rtreeSearch->RTreeSearch(&rect, answers);*/

  int count = 0;
  for (register int i = 0; i < answers.size(); i ++)
    {
      int id = answers[i];
      // we have to convert a dummy maximal number back to 0 since we have
      // replaced id 0 with this dummy number when constructing the R-tree
      if( id == DUMMYMAXNUM ) id = 0;

      /*int eddist = ed(queryString, (*searchStrings)[id]);
      if (eddist <= edThreshold
      && (countingDuplicates || eddist > 0)) // check if need to ignore those identical-string pairs*/
      if (SimilarString(queryString, (*searchStrings)[id], edThreshold)
	  && (countingDuplicates || queryString != (*searchStrings)[id])) // check if need to ignore those identical-string pairs
	{
	  //cout << "StringMap found:\t [" << (*searchStrings)[id] << "]" << endl;
	  count ++;
	}
    }

  return count;
}


// compute the real number of similar pairs
int StringMap::SimilarStrings(const string queryString,
			      const vector<string> &stringVector,
			      const int edThreshold,
			      bool countingDuplicates)
{
  int count = 0;

  for (int i = 0; i < stringVector.size(); i ++)
    {
      int eddist = ed(queryString, stringVector[i]);
      if (eddist <= edThreshold
	  && (countingDuplicates || eddist > 0)) // check if need to ignore those identical-string pairs
	{
	  //cout << "Similar String:\t [" << stringVector[i] << "]" << endl;
	  count ++;
	}
      }

  return count;
}

