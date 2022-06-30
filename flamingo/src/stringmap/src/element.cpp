//
// $Id: element.cpp 5770 2010-10-19 06:38:25Z abehm $
//
// element.cpp
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: Michael Ortega-Binderberger <miki (at) ics.uci.edu>
//          Liang Jin <liangj (at) ics.uci.edu>
//          Chen Li <chenli (at) ics.uci.edu>
//


#include <float.h>
#include <assert.h>
#include <math.h>
#include "element.h"


// uses struct Rect from index.h

DistanceFunction::DistanceFunction() { // constructor, initialize weights
  MakeDefaultFunction();
}


void DistanceFunction::MakeDefaultFunction() {
  for (int i=0;i < NUMDIMS; i++) {
    dimension_weights[i] = 1.0;
  }
  query_point_queue.erase(query_point_queue.begin(), query_point_queue.end());
  total_point_wt_sum = 0.0;
  NormalizeWeights();
}


void DistanceFunction::NormalizeWeights() {
  // make internal weights add up to NUMDIMS;
  float wt_sum = 0.0;
  int i;
  for (i=0;i < NUMDIMS; i++) { wt_sum += dimension_weights[i]; }
  for (i=0;i < NUMDIMS; i++) { dimension_weights[i] = dimension_weights[i] * ((float)NUMDIMS)/ wt_sum ; }
  float total = 0.0;
  for (multimap<float,struct Rect,less<float> >:: iterator it = query_point_queue.begin();
         it != query_point_queue.end(); it++) { // add up wt for points
    total += (*it).first;
  }
  total_point_wt_sum = total;
}


bool DistanceFunction::add_query_point(struct Rect * query_point, float point_wt) { // add a query point
  query_point_queue.insert(pair<float,struct Rect>(point_wt,*query_point));
  total_point_wt_sum += point_wt; // accumulate "key"

  return true; // chenli
}

// ------------------------------------------------------------
// distance functions:

float DistanceFunction::compute_distance_aux(struct Rect * query, struct Rect * item) {
  // assumes both are points
  assert(NULL != query);    assert(NULL != item);
  float distance = 0;
  for (int i=0; i<NUMDIMS; i++) {
    distance += dimension_weights[i] *
                (query->boundary[i] - item->boundary[i]) *
                (query->boundary[i] - item->boundary[i]);
  }
  distance = sqrt(distance);
  return distance;
}


// want here an "independently configurable function" so we
// can also manipulate feedback for later.
// below func needs to work for both, multipoint and joinable
float DistanceFunction::compute_distance(struct Rect * query, struct Rect * item) {
  // if NULL == query, then use the query points queue,
  // else use query.
  float distance = 0, d=0.0;
  // since here we assume we're dealing with points, we're
  // using only the first NUMDIMS dimensions, as the others
  // are repetitions.
  if (NULL == query) { // its a fixed (multipoint) query
    assert(total_point_wt_sum > 0.0); // used to normalize ->/0
    // iterate over points
    for (multimap<float,struct Rect,less<float> >:: iterator it = query_point_queue.begin();
         it != query_point_queue.end(); it++) { // add up each point
      d = compute_distance_aux( &(*it).second, item);
      distance += d * ((*it).first/total_point_wt_sum); // wsum, normalized by total
    }
  } else { // its a simple, joinable query.
    distance = compute_distance_aux(query,item);
  }
  return distance;
}

// ------------------------------------------------------------
// rectangle mindist functions
float DistanceFunction::rectangle_mindist(struct Rect * r1, struct Rect * r2) {
  // compares 2 mbrs for mindist.
  // asume: both are a rectangle
  assert(NULL != r1);    assert(NULL != r2);
  float distance = 0, choice=0.0;
  for (int i=0; i<NUMDIMS; i++) {
    if        (r1->boundary[i] > r2->boundary[i+NUMDIMS]) {
      choice = r1->boundary[i] - r2->boundary[i+NUMDIMS];
    } else if (r2->boundary[i] > r1->boundary[i+NUMDIMS]) {
      choice = r2->boundary[i] - r1->boundary[i+NUMDIMS];
    } else {
      choice = 0.0;
    }
    distance += dimension_weights[i] * choice * choice;
  }
  distance = sqrt(distance);
  return distance;
}

// ------------------------------------------------------------
// mindist functions

float DistanceFunction::compute_mindist_aux(struct Rect * query, struct Rect * item) {
  // asume: query is a point, item is a rectangle
  assert(NULL != query);    assert(NULL != item);
  float distance = 0, choice=0.0;
  for (int i=0; i<NUMDIMS; i++) {
    if        (query->boundary[i] < item->boundary[i]) {
      choice = item->boundary[i]; // small_i if point_i<small_i
    } else if (query->boundary[i] > item->boundary[i+NUMDIMS]) {
      choice = item->boundary[i+NUMDIMS]; // big_i if point_i > big_i
    } else {
      choice = query->boundary[i]; // else point_i (results in a dist of 0)
    }
    distance += dimension_weights[i] *
                (query->boundary[i] - choice) *
                (query->boundary[i] - choice);
  }
  distance = sqrt(distance);
  return distance;
}


float DistanceFunction::compute_mindist(struct Rect * query, struct Rect * item) {
  // see also function compute_distance
  // compute the mindist
  // watch out in the case of multipoint queries.
  // item should always be treated as a rectangle.
  float distance = 0, d=0.0;
  if (NULL == query) { // its a fixed (multipoint) query, use list.
    assert(total_point_wt_sum > 0.0); // used to normalize ->/0
    // iterate over points
    for (multimap<float,struct Rect,less<float> >:: iterator it = query_point_queue.begin();
         it != query_point_queue.end(); it++) { // add up each point
      d = compute_mindist_aux( &(*it).second, item);
      // I'm sort of assuming there is only 1 point.
      distance += d * ((*it).first/total_point_wt_sum); // wsum, normalized by total
    }
  } else { // its a simple, joinable query.
    // assume query is a single point and item is a
    // rectangle.
    distance = compute_mindist_aux(query,item);
  }
  return distance;
}


// ------------------------------------------------------------



bool DistanceFunction::feedback_start() {
  // reset all single-iteration feedback data
  feedback_join_data.erase(feedback_join_data.begin(), feedback_join_data.end());
  feedback_nn_data.erase  (feedback_nn_data.begin(),   feedback_nn_data.end());
  return true;
}

int DistanceFunction::feedback_add_tuple(float relevance, struct Rect * query, struct Rect * item) {
  assert(NULL != item); // item has to exist
  if (NULL == query) { // dealing with NN, use points in query_point_queue as base points
    feedback_nn_data.insert(pair<float,struct Rect>(relevance,*item));
  } else {
    // dealing with join, accumulate them and then
    // in one go, issue an update to dimension_weights
    join_pair_result d_item;
    appDataItem di1, di2;
    di1.the_id      = di2.the_id = 0;
    di1.rect        = *query;
    di2.rect        = *item;
    d_item.first    = di1;
    d_item.second   = di2;
    d_item.distance = relevance;
    feedback_join_data.insert(pair<float,join_pair_result>(relevance, d_item));
  }
  return 1;
}

// alpha and beta to restrain speed of convergence.
#define F_ALFA 0.5
#define F_BETA 0.5

bool DistanceFunction::feedback_do_it() { // take info collected above and perform function update
  // prepare
  double sum[NUMDIMS], sumsq[NUMDIMS], mean[NUMDIMS], stddev[NUMDIMS];
  int    count = 0;
  int i;
  for (i=0; i<NUMDIMS; i++) { sum[i]=sumsq[i]=mean[i]=stddev[i]= 0.0; } // init

  if (feedback_nn_data.size() > 0) { // its a nn type, may modify query points
    // do query point movement and standard deviation along
    // each dimension.
    // throw in the relevant points
	multimap<float,struct Rect,less<float> >::iterator it;
    for (it = feedback_nn_data.begin();
         it != feedback_nn_data.end(); it++) {
      for (i=0; i<NUMDIMS; i++) {
        sum[i]   += (*it).second.boundary[i];
        sumsq[i] += ((*it).second.boundary[i]) * ((*it).second.boundary[i]);
      }
    } // have sum and sum of squares
    // then throw in the old points
    for (it = query_point_queue.begin();
         it != query_point_queue.end(); it++) {
      for (int i=0; i<NUMDIMS; i++) {
        sum[i]   += (*it).second.boundary[i];
        sumsq[i] += ((*it).second.boundary[i]) * ((*it).second.boundary[i]);
      }
    } // have sum and sum of squares
    count = feedback_nn_data.size() + query_point_queue.size(); // total
    if (count > 0) { // all ok
      for (int i=0; i<NUMDIMS; i++) { // compute mean and stddev
        mean  [i] = sum[i] / count;
        stddev[i] = (sumsq[i] - (sum[i]*mean[i])) / count;
        stddev[i] = sqrt(stddev[i]);
      }
    } else { // count is <= 0, bad
      assert(false); // don't do stddev or mean.
    }
    // got the new mean and standard deviation, update me.
    query_point_queue.erase(query_point_queue.begin(), query_point_queue.end()); // eliminate old points
    struct Rect rect;
    for (int i=0; i<NUMDIMS; i++) {
      rect.boundary[i] = mean[i];  // new "centroid"
      assert(0.0 != stddev[i]);
      dimension_weights[i] = F_ALFA * dimension_weights[i] +
                             F_BETA  * (1/(stddev[i])); // new dimensional weight
    }
    query_point_queue.insert(pair<float,struct Rect>(1.0, rect));
  } else if (feedback_join_data.size() > 0) { // its a join type
    // just concentrate in getting the standard deviation
    // along each dimension.
    // iterate over relevant pairs
    for (join_pair_result_list::iterator it = feedback_join_data.begin();
         it != feedback_join_data.end(); it++) {
      // ensure we use the difference between dimensions.
      float relevance = (*it).first;
      (*it).second.distance; // also relevance.
      for (int i=0; i<NUMDIMS; i++) { // accumulate differences
        float dim1, dim2, dimdiff; // dimension values
        dim1      = (*it).second.first.rect.boundary[i]; // the first one in the pair
        dim2      = (*it).second.second.rect.boundary[i]; // the second one in the pair
        dimdiff   = dim1 - dim2; // their difference
        sum[i]   += dimdiff;
        sumsq[i] += dimdiff * dimdiff;
      }
      count ++;
    }
    if (count > 1) { // ok so far...
      // do mean first;
      float min_stddev=FLT_MAX; // in case
      for (i=0; i<NUMDIMS; i++) {
        mean  [i] = sum[i]/count; // mean
        stddev[i] = (sumsq[i] - (sum[i]*mean[i])) / count; // variance
        stddev[i] = sqrt(stddev[i]); // standard deviation.
        if (stddev[i] > 0.0 && min_stddev > stddev[i]) { // record smallest nonzero deviation
          min_stddev = stddev[i];
        }
      }
      // got stats, and minimum stddev. Use half the
      // minimum for a zero stddev. That way, we protect
      // against divide by zero and get a nice big weight.
      for (int i=0; i<NUMDIMS; i++) { // update stddev
        if (stddev[i] <= 0.0) { // protect against useless stddev
          stddev[i] = min_stddev;
        }
      }
    } else { // count <=0, bad
      assert(false); // don't do stddev or mean.
    }
    for (int i=0; i<NUMDIMS; i++) {  // compute new dimensioanl weights
      assert(0.0 != stddev[i]);
      assert(FLT_MAX != stddev[i]); // proper update
      dimension_weights[i] = F_ALFA * dimension_weights[i] +
                             F_BETA  * (1/stddev[i]); // new dimensional weight
    }
  } else { // oops, big problem, either gave no feedback, or error
    cout << "Really? There was no feedback to process, so "
            "the distance function was not modified" << endl;
  }
  NormalizeWeights(); // does the whole enchilada
  return true;
}
