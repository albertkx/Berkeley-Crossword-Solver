//
// $Id: element.h 5770 2010-10-19 06:38:25Z abehm $
//
// element.h
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: Michael Ortega-Binderberger (miki (at) ics.uci.edu)
//          Liang Jin (liangj (at) ics.uci.edu)
//          Chen Li (chenli (at) ics.uci.edu)
//

#ifndef  DIST_FUNC_H
#define  DIST_FUNC_H


#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <deque>
#include <map>
#include <queue>
#include <iostream>
using namespace std;

//#include "RankedL.h"
#include "rtreeparams.h" // get the definition of the dimensionality
// general defines for the whole system
#define NUMSIDES (2 * NUMDIMS)

typedef float RectReal;
typedef float PointReal;

struct Rect {
        RectReal boundary[NUMSIDES]; /* xmin,ymin,...,xmax,ymax,... */
};

struct Point {
        PointReal position[NUMDIMS]; /* xmin,ymin,...,xmax,ymax,... */
};


// ------------------------------------------------------------
// added by miki, needed datatypes
struct appDataItem { // what the application uses, a recangle plus id.
  int    the_id;
  struct Rect rect;
};
typedef map<int,appDataItem,less<int> > AnswerList; // for NN queries

struct join_pair_result { // for join queries: two appitems + their distance
  float       distance; // the distance between the two items
  appDataItem first, second; // the two items
};
typedef multimap<float,join_pair_result,less<float> > join_pair_result_list;

// ------------------------------------------------------------



class DistanceFunction {
public:
  // for multipoint query:
  multimap<float,struct Rect,less<float> > query_point_queue;
  //  in this map, the key (.first) is a float which is a
  //  WEIGHT for the point.
  float total_point_wt_sum; // adds the key from above for easy normalization
  float dimension_weights[NUMDIMS]; // a weight for each matching dimension. Must add up to 1.

  DistanceFunction(); // constructor, initialize weights
  void  MakeDefaultFunction();
  void  NormalizeWeights(); // make them add up to NUMDIMS;
  bool  add_query_point(struct Rect * query_point, float point_wt = 1.0); // add a query point
  float rectangle_mindist(struct Rect * r1, struct Rect * r2);
  float compute_distance_aux(struct Rect * query, struct Rect * item);
  float compute_distance(struct Rect * query, struct Rect * item);
  float compute_mindist_aux(struct Rect * query, struct Rect * item);
  float compute_mindist(struct Rect * query, struct Rect * item);

  // feedback specific info collection
  // .first (float) is the relevance value, .second (join_pair_result) is
  // the two matching rectangles (resp. points).
  join_pair_result_list feedback_join_data; // .first = relevance, .second = data
  multimap<float,struct Rect,less<float> > feedback_nn_data; // .first = relevance, .second = feedback
  bool feedback_start();
  int  feedback_add_tuple(float relevance, struct Rect * query, struct Rect * item);
  bool feedback_do_it();
}; // end class distance function

#endif
