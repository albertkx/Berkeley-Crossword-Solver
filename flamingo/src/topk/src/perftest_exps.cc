/*
  $Id: perftest_exps.cc 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 07/21/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "perftest_exps.h"

#define DEBUG_TIMER

#include "util/src/io.h"
#include "util/src/misc.h"
#include "util/src/debug.h"

#include <iomanip>

using namespace std;
using namespace Topk;

void exp_xDataSize_yTime_Group(
  SimMetric &simMetric, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep,
  uint noGroupKwd, 
  uint noGroupEd)
{
  cerr << "exp_xDataSize_yTime_Group " 
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ") "
       << noGroupKwd << ' '
       << noGroupEd
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  uint **topks;
  topks = new uint*[noGroupKwd];
  for (uint i = 0; i < noGroupKwd; ++i)
    topks[i] = new uint[noAns];

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue, noData, noGroupKwd, noGroupEd);
    PerfIndex index(data, simMetric.gramGen, 0);

    uint jMax = 3;
    float t = 0;

    TIMER_START("exps", jMax);
    for (uint j = 0; j < jMax; j++) {

      TIME_START;
      for (uint i = 0; i < que.no - que.no % noGroupKwd; i += noGroupKwd) {
        QueryGroup queGrp(que.que.begin() + i, noGroupKwd, simMetric, noAns);
        IndexQuery indexQuery(index.index, queGrp);
        Topk::Heap::getTopks(
          data.data, 
          data.weights, 
          index.index, 
          queGrp, 
          indexQuery, 
          topks);
      }
      TIME_END;
      t += _tms / ((que.no - que.no % noGroupKwd) / noGroupKwd);

      TIMER_STEP();
    }
    TIMER_STOP();

    float s = t / jMax;
    cout << noData << " " << s << endl;
    cerr << s << "ms/que" << endl;
  }
  cerr << "OK" << endl;  
}

void exp_xQue_yThr(
  SimMetric &simMetric, 
  Alg alg,
  PerfDataName dataName, 
  uint noData)
{
  cerr << "exp_xQue_yThr " 
       << getAlgName(alg) << ' '
       << getDataName(dataName) << ' '
       << utosh(noData)
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  PerfData data(simMetric.gramGen, dataName, noData);
  PerfQue que(data, noQue);
  PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

  uint topk[noAns];

  uint kmax = 5;
  
  for (uint i = 0; i < que.no; ++i) 
  {
    Query query(que.que[i], simMetric, noAns);
      
    set<uint> gramsQue;
    query.sim.gramGen.decompose(query.str, gramsQue);
    IndexQuery_v1 idxQue_v1;

    float invListAvgSize = 0;
    vector<uint> invListSizes;
    uint invListMaxSize = 0, invListMinSize = ~0;

    for (set<uint>::const_iterator gram = gramsQue.begin();
         gram != gramsQue.end(); ++gram) {
      Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
      idxQue_v1.push_back(ptr);

      uint sz = ptr->size();
      invListAvgSize += sz;
      invListSizes.push_back(sz);
      if (sz > invListMaxSize)
        invListMaxSize = sz;
      if (sz < invListMinSize)
        invListMinSize = sz;
    }
    gramsQue.clear();
    sort(invListSizes.begin(), invListSizes.end());
    
    cout << invListAvgSize / query.noGrams << ' ';
    cout << invListSizes[query.noGrams / 2] << ' ';
    cout << invListMaxSize - invListMinSize << ' ';
    cout << query.noGrams << ' ';

    float timeMin = numeric_limits<float>::max();
    uint thresholdBest = ~0;

    for (thresholdInit = query.noGrams + 1; thresholdInit > 0; --thresholdInit) 
      {
      TIME_START;
      for (uint k = 0; k < kmax; ++k)
        runAlg_v1(
          AlgTwoPhase,
          data.data, 
          data.weights, 
          data.nograms, 
          index.index_v1, 
          query, 
          idxQue_v1, 
          topk);
      TIME_END;
      cerr << setw(2) << thresholdInit 
           << setw(20) << _tms / kmax << "ms"
           << endl;
      if (_tms < timeMin) {
        timeMin = _tms;
        thresholdBest = thresholdInit;
      }
      if (_tms > timeMin + 50 * kmax)
          break;
    }
    cerr << endl;
    cout << thresholdBest << endl;
  }

  cerr << "OK" << endl;
}

void exp_xQue_yTime_zThr(
  SimMetric &simMetric, 
  Alg alg,
  PerfDataName dataName, 
  uint noData)
{
  cerr << "exp_xThr_yTime_zAlg_QueSel " 
       << getAlgName(alg) << ' '
       << getDataName(dataName) << ' '
       << utosh(noData)
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  PerfData data(simMetric.gramGen, dataName, noData);
  PerfQue que(data, noQue);
  PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));
  
  uint jMax = 10, k = 0;
  uint topk[noAns];

  TIMER_START("exps", 3);
  for (uint i = 0; i < que.no; i += 34) {
    if (i != 0) {
      cout << ++k << " 0" << endl;
      cout << ++k << " 0" << endl;
    }

    Query query(que.que[i], simMetric, noAns);
    for (thresholdInit = query.noGrams + 1; thresholdInit > 0; --thresholdInit) {

      TIME_START;
      for (uint j = 0; j < jMax; j++)
        if (alg == AlgRoundRobin || alg == AlgHeap) {
          IndexQuery indexQuery(index.index, query);
          runAlg(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index, 
            query, 
            indexQuery, 
            topk);
        }
        else {
          set<uint> gramsQue;
          query.sim.gramGen.decompose(query.str, gramsQue);
          IndexQuery_v1 idxQue_v1;
          for (set<uint>::const_iterator gram = gramsQue.begin();
               gram != gramsQue.end(); ++gram) {
            Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
            idxQue_v1.push_back(ptr);
          }
          gramsQue.clear();
          runAlg_v1(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index_v1, 
            query, 
            idxQue_v1, 
            topk);
        }
      TIME_END;

      float s = _tms / jMax;
      cout << ++k << ' ' << s 
        // << ' ' << topk[0] 
           << endl;
    }

    TIMER_STEP();
  }
  TIMER_STOP();

  cerr << "OK" << endl;
}

void exp_PerfThr_Time(
  SimMetric &simMetric,  
  Alg alg, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName)
{
  cerr << "exp_PerfThr_Time " 
       << getAlgName(alg) << ' '
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ")"
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue);
    PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

    uint topk[noAns];
    uint thr[que.no];

    TIMER_START("exps", que.no);
    for (uint i = 0; i < que.no; i++) {
      Query query(que.que[i], simMetric, noAns);
      set<uint> gramsQue;
      query.sim.gramGen.decompose(query.str, gramsQue);
      IndexQuery_v1 idxQue_v1;
      for (set<uint>::const_iterator gram = gramsQue.begin();
           gram != gramsQue.end(); ++gram) {
        Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
        idxQue_v1.push_back(ptr);
      }
      gramsQue.clear();

      float tmin = numeric_limits<float>::max();
      thr[i] = query.noGrams + 1;
      for (thresholdInit = query.noGrams + (alg == AlgTwoPhase ? 1 : 0); 
           thresholdInit > 0; --thresholdInit) {
        TIME_START;
        runAlg_v1(
          alg, 
          data.data, 
          data.weights, 
          data.nograms, 
          index.index_v1, 
          query, 
          idxQue_v1, 
          topk);
        TIME_END;

        if (_tms < tmin) {
          thr[i] = thresholdInit;
          tmin = _tms;
        }
      }
      TIMER_STEP();
    }
    TIMER_STOP();

    string fn = data.fn.substr(0, data.fn.rfind('.')) + 
      "/que/data." + utosh(data.no) + 
      ".que." + utosh(que.no) + 
      "." + scoreName +
      ".q." + utos(simMetric.gramGen.getGramLength()) + 
      "." + simMetric.name + 
      ".thr." + getAlgName(alg) + ".bin";
    writeBinaryFile<uint>(fn, thr, thr + que.no);
  }
  cerr << "OK" << endl;  
}

void exp_PerfThr_Iter(
  SimMetric &simMetric, 
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName)
{
  cerr << "exp_PerfThr_Iter " 
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ")"
       << endl;

  Alg alg = AlgIterative;

  const uint noQue  =     100;
  const uint noAns  =      10;

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue);
    PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

    uint topk[noAns];
    uint thr[que.no];

    TIMER_START("exps", que.no);
    for (uint i = 0; i < que.no; i++) {
      Query query(que.que[i], simMetric, noAns);
      set<uint> gramsQue;
      query.sim.gramGen.decompose(query.str, gramsQue);
      IndexQuery_v1 idxQue_v1;
      for (set<uint>::const_iterator gram = gramsQue.begin();
           gram != gramsQue.end(); ++gram) {
        Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
        idxQue_v1.push_back(ptr);
      }
      gramsQue.clear();

      float tmin = numeric_limits<float>::max();
      for (thresholdInit = query.noGrams; thresholdInit > 0; --thresholdInit) {
        thresholdPerf = false;
        TIME_START;
        runAlg_v1(
          alg, 
          data.data, 
          data.weights, 
          data.nograms, 
          index.index_v1, 
          query, 
          idxQue_v1, 
          topk);
        TIME_END;
        
        if (thresholdPerf) {
          thr[i] = thresholdInit;
          tmin = _tms;
          break;
        }
      }
      TIMER_STEP();
    }
    TIMER_STOP();
    
    string fn = data.fn.substr(0, data.fn.rfind('.')) + 
      "/que/data." + utosh(data.no) + 
      ".que." + utosh(que.no) + 
      "." + scoreName +
      ".q." + utos(simMetric.gramGen.getGramLength()) + 
      "." + simMetric.name + 
      ".thr." + getAlgName(alg) + ".bin";
    writeBinaryFile<uint>(fn, thr, thr + que.no);
  }
  cerr << "OK" << endl;  
}

void exp_xDataSize_yTime_zAlg_Perf(
  SimMetric &simMetric, 
  Alg alg,
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep, 
  const string &scoreName)
{
  cerr << "exp_xDataSize_yTime_zAlg_Perf " 
       << getAlgName(alg) << ' '
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ")"
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue);
    PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

    uint thr[que.no];
    string fn = data.fn.substr(0, data.fn.rfind('.')) + 
      "/que/data." + utosh(data.no) + 
      ".que." + utosh(que.no) + 
      "." + scoreName +
      ".q." + utos(simMetric.gramGen.getGramLength()) + 
      "." + simMetric.name + 
      ".thr." + getAlgName(alg) + ".bin";
    readBinaryFile<uint>(fn, thr);

    uint jMax = 10;
    float t = 0;
    uint topk[noAns];

    TIMER_START("exps", jMax);
    for (uint j = 0; j < jMax; j++) {

      TIME_START;
      if (alg == AlgRoundRobin || alg == AlgHeap)
        for (uint i = 0; i < que.no; i++) {
          Query query(que.que[i], simMetric, noAns);
          thresholdInit = thr[i];
        
          IndexQuery indexQuery(index.index, query);
          runAlg(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index, 
            query, 
            indexQuery, 
            topk);
        }
      else
        for (uint i = 0; i < que.no; i++) {
          Query query(que.que[i], simMetric, noAns);
          thresholdInit = thr[i];
        
          set<uint> gramsQue;
          query.sim.gramGen.decompose(query.str, gramsQue);
          IndexQuery_v1 idxQue_v1;
          for (set<uint>::const_iterator gram = gramsQue.begin();
               gram != gramsQue.end(); ++gram) {
            Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
            idxQue_v1.push_back(ptr);
          }
          gramsQue.clear();
          runAlg_v1(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index_v1, 
            query, 
            idxQue_v1, 
            topk);
        }
      TIME_END;
      t += _tms / que.no;

      TIMER_STEP();
    }
    TIMER_STOP();

    float s = t / jMax;
    cout << noData << " " << s << endl;
    cerr << s << "ms/que" << endl;
  }
  cerr << "OK" << endl;  
}

void exp_xDataSize_yTime_zAlg(
  SimMetric &simMetric, 
  Alg alg,
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep)
{
  cerr << "exp_xDataSize_yTime_zAlg " 
       << getAlgName(alg) << ' '
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ")"
       << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue);
    PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

    uint jMax = 10;
    float t = 0;
    uint topk[noAns];

    TIMER_START("exps", jMax);
    for (uint j = 0; j < jMax; j++) {

      TIME_START;
      if (alg == AlgRoundRobin || alg == AlgHeap)
        for (uint i = 0; i < que.no; i++) {
          Query query(que.que[i], simMetric, noAns);
          thresholdInit = query.noGrams;
        
          IndexQuery indexQuery(index.index, query);
          runAlg(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index, 
            query, 
            indexQuery, 
            topk);
        }
      else
        for (uint i = 0; i < que.no; i++) {
          Query query(que.que[i], simMetric, noAns);
          thresholdInit = query.noGrams;

          set<uint> gramsQue;
          query.sim.gramGen.decompose(query.str, gramsQue);
          IndexQuery_v1 idxQue_v1;
          for (set<uint>::const_iterator gram = gramsQue.begin();
               gram != gramsQue.end(); ++gram) {
            Array<uint> *ptr = index.index_v1.lists.find(*gram)->second;
            idxQue_v1.push_back(ptr);
          }
          gramsQue.clear();

          runAlg_v1(
            alg, 
            data.data, 
            data.weights, 
            data.nograms, 
            index.index_v1, 
            query, 
            idxQue_v1, 
            topk);
        }
      TIME_END;
      t += _tms / que.no;

      TIMER_STEP();
    }
    TIMER_STOP();

    float s = t / jMax;
    cout << noData << " " << s << endl;
    cerr << s << "ms/que" << endl;
  }
  cerr << "OK" << endl;  
}
