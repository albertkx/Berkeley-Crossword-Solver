/*
  $Id: perftest_exps.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 07/21/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _perftest_exps_h_
#define _perftest_exps_h_

#include "topk.h"
#include "perftest_util.h"

void exp_xDataSize_yTime_Group(
  const SimMetric &simMetric, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep,
  uint noGroupKwd, 
  uint noGroupEd);

void exp_xQue_yThr(
  const SimMetric &simMetric, 
  Topk::Alg alg,
  PerfDataName dataName,   
  uint noData);

void exp_xQue_yTime_zThr(
  const SimMetric &simMetric, 
  Topk::Alg alg,
  PerfDataName dataName, 
  uint noData);

void exp_PerfThr_Time(
  const SimMetric &simMetric, 
  Topk::Alg alg, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName = "sum");

void exp_PerfThr_Iter(
  const SimMetric &simMetric, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName = "sum");

void exp_xDataSize_yTime_zAlg_Perf(
  const SimMetric &simMetric, 
  Topk::Alg alg,
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName = "sum");

void exp_xDataSize_yTime_zAlg(
  const SimMetric &simMetric, 
  Topk::Alg alg,
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep);

#endif
