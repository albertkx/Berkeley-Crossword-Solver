/*
  $Id: perftest.cc 5776 2010-10-20 01:34:22Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 05/31/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <cassert>
#include <iomanip>
#include <iostream>

#include "topk.h"
#include "perftest_exps.h"
#include "perftest_util.h"
#include "util/src/io.h"
#include "util/src/output.h"
#include "util/src/misc.h"
#include "util/src/time.h"

using namespace std;
using namespace Topk;

uint thresholdInit = 0;             // threshold for Iterative and TwoPhase
bool thresholdPerf = false;
uint factorK = 1;                   // for Iterative
float time1 = 0;

void smoke(Alg alg)
{
  cerr << "smoke " << getAlgName(alg) << endl;

  const uint noData = 1500000;
  const uint noQue  =    1000;
  const uint noAns  =    1000;
  const bool isWeight = true;

  GramGenFixedLen gramGen(3);
  SimMetricEdNorm simMetric(gramGen);

  PerfData data(gramGen, PerfDataIMDB, noData);
  PerfQue que(data, noQue);
  PerfAns ans(data, que, simMetric, noAns, isWeight);
  PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

  TIMER_START("smoke", noQue);
  vector<uint> topk;
  for (uint i = 0; i < que.no; i++) 
    // uint i = 5;
    { 
      OUTPUT("i", i);
        
      Query query(que.que[i], simMetric, noAns);

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
          back_inserter(topk));
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

        thresholdInit = query.noGrams;

        runAlg_v1(
          alg, 
          data.data, 
          data.weights, 
          data.nograms, 
          index.index_v1, 
          query, 
          idxQue_v1, 
          back_inserter(topk));
      }

      OUTPUT_L2("top", topk);
      OUTPUT_L2("ans", ans.ans[i]);

      assert(PerfAns::isEqual(data, simMetric, topk, ans.ans[i], query));
      topk.clear();
      
      TIMER_STEP();
    }
  TIMER_STOP();

  cerr << "OK" << endl;
}

void smokeMulti()
{
  cerr << "smokeMulti" << endl;

  const uint noData = 1500000;
  const uint noQue  =     100;
  const uint noAns  =      10;
  const bool isWeight = true;

  GramGenFixedLen gramGen(3);
  SimMetricEdNorm simMetric(gramGen);

  PerfData data(gramGen, PerfDataIMDB, noData);
  PerfQue que(data, noQue);
  PerfAns ans(data, que, simMetric, noAns, isWeight);
  PerfIndex index(data, simMetric.gramGen, 0);

  uint noKwd = 2;

  range<Query**> ques;
  ques._begin = new Query*[noKwd];
  ques._end = ques._begin + noKwd;

  vector<vector<uint> > topks;
  for (uint i = 0; i < noKwd; ++i)
    topks.push_back(vector<uint>(noAns));

  TIMER_START("smoke", noQue);
  vector<uint> topk;
  for (uint i = 0; i < que.no - noKwd + 1; i++) 
    // uint i = 5;
    {
      QueryGroup queGrp(que.que.begin() + i, noKwd, simMetric, noAns);

      OUTPUT("i", i);

      IndexQuery indexQuery(index.index, queGrp);
      Topk::Heap::getTopks(
        data.data, 
        data.weights, 
        index.index, 
        queGrp, 
        indexQuery, 
        topks);

      for (uint j = 0; j < noKwd; ++j) {
        OUTPUT_L2("que", queGrp.strs[j]);
        OUTPUT_L2("top", topks[j]);
        OUTPUT_L2("ans", ans.ans[i + j]);
      }
      
      for (uint j = 0; j < noKwd; ++j)
        assert(
          PerfAns::isEqual(
            data,
            simMetric,
            topks[j],
            ans.ans[i + j], 
            Query(queGrp.strs[j], queGrp.sim, queGrp.k)));
      
      TIMER_STEP();
    }
  TIMER_STOP();

  cerr << "OK" << endl;
}

void perf(
  SimMetric &simMetric, Alg alg, PerfDataName perfDataName, uint noData)
{
  cerr << "perf " << getAlgName(alg) << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;

  PerfData data(simMetric.gramGen, perfDataName, noData);
  PerfQue que(data, noQue);
  PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));
  
  uint jMax = 5;               // 5
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
  cout << setw(8) << s << " ms/que" << endl;
  cerr << "OK" << endl;
}

void perfMulti(
  SimMetric &simMetric, PerfDataName perfDataName, uint noData)
{
  cerr << "perfMulti" << endl;

  const uint noQue  =     100;
  const uint noAns  =      10;
  const uint noKwd  =       2;

  PerfData data(simMetric.gramGen, perfDataName, noData);
  PerfQue que(data, noQue, noData, noKwd, 2);
  PerfIndex index(data, simMetric.gramGen, 0);

  const uint jMax = 3;               // 5
  float t = 0;

  uint topk[noAns];

  for (uint j = 0; j < jMax; j++) {
    TIME_START;
    for (uint i = 0; i < que.no; i++) {
      Query query(que.que[i], simMetric, noAns);
      thresholdInit = query.noGrams;
        
      IndexQuery indexQuery(index.index, query);
      Topk::Heap::getTopk(
        data.data, 
        data.weights, 
        index.index, 
        query, 
        indexQuery, 
        topk);
    }
    TIME_END;
    t += _tms / que.no;
  }

  float s = t / jMax;
  cout << setw(8) << s << " ms/que, total: " 
       << (s * noKwd) <<" ms/que "<< endl;


  uint topks[noKwd][noAns];
  t = 0;

  TIMER_START("exps", jMax);
  for (uint j = 0; j < jMax; j++) {

    TIME_START;
    for (uint i = 0; i < que.no - que.no % noKwd; i += noKwd) {
      QueryGroup queGrp(que.que.begin() + i, noKwd, simMetric, noAns);
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
    t += _tms / ((que.no - que.no % noKwd) / noKwd);

    TIMER_STEP();
  }
  TIMER_STOP();

  s = t / jMax;
  cout << setw(8) << s << " ms/que" << endl;
  cerr << "OK" << endl;
}

#ifdef DEBUG
void debug()
{
  cerr << "debug " << endl;

  const uint noData =  400000;
  const uint noQue  =     100;
  const uint noAns  =      10;
  const uint noKwd   =      2;

  uint iBeg = 0;

  GramGenFixedLen gramGen(2);
  SimMetricEdNorm simMetric(gramGen);

  PerfData data(gramGen, PerfDataENRON, noData);
  PerfQue que(data, noQue, noData / 1000);

  Query q(que.que[iBeg], simMetric, noAns);
  TIME_START;
  // for (uint i = 0; i < data.no; i++)
  //   q.sim(q.str, data.data[i]);
  TIME_END;
  float simSpeed = _tms / data.no;
  cerr << "sim speed " << simSpeed << "ms/sim" << endl;

  uint topk[noAns];

  PerfIndex index(data, simMetric.gramGen, 0);

  fill(topk, topk + noAns, 0);

  simComp = simTime = 0;
  thrFreq.clear();

  cout << "--------------------------------------" << endl;

  TIME_RESET;
  for (uint i = iBeg; i < noKwd + iBeg; i++) {
    Query query(que.que[i], simMetric, noAns);
    thresholdInit = query.noGrams;
      
    IndexQuery indexQuery(index.index, query);
    Topk::Heap::getTopk(
      data.data, 
      data.weights, 
      index.index, 
      query, 
      indexQuery, 
      topk);

    cout << "que " << que.que[i] << endl;
    // cout << "thr freq" << endl;
    // for (map<uint, uint>::const_iterator t = thrFreq.begin();
    //      t != thrFreq.end(); ++t)
    //   cout << t->first << '\t' << t->second << endl;
    // cout << endl;
    // thrFreq.clear();
    cout << "thr freq sim" << endl;
    for (map<uint, uint>::const_iterator t = thrFreqSim.begin();
         t != thrFreqSim.end(); ++t)
      cout << t->first << '\t' << t->second << endl;
    cout << endl;
    thrFreqSim.clear();

    for (uint j = 0; j < noAns; ++j)
      cout << topk[j] << ":" << data.data[topk[j]] << "; ";
    cout << endl;
  }
  TIME_END;
  cerr << "topk      " << setw(20) << right << _tms << "ms" << endl;
  cout << "sim comp " << simComp << endl;
  cout << "sim time comp " << simComp * simSpeed << endl;
  cout << "sim time " << simTime << endl;
  cout << "--------------------------------------" << endl;

  uint topks[noKwd][noAns];

  simComp = simTime = 0;
  thrFreq.clear();
  
  QueryGroup queGrp(que.que.begin() + iBeg, noKwd, simMetric, noAns);
  
  TIME_RESET;
  IndexQuery indexQuery(index.index, queGrp);
  Topk::Heap::getTopks(
    data.data, 
    data.weights, 
    index.index, 
    queGrp, 
    indexQuery, 
    topks);
  TIME_END;

  cerr << "topk-multi" << setw(20) << right << _tms << "ms" << endl;
  cout << "sim comp " << simComp << endl;
  cout << "sim time comp " << simComp * simSpeed << endl;
  cout << "sim time " << simTime << endl;
  cout << "thr freq" << endl;
  for (map<uint, uint>::const_iterator t = thrFreq.begin();
       t != thrFreq.end(); ++t)
    cout << t->first << '\t' << t->second << endl;
  cout << endl;
  // cout << "thrs freq" << endl;
  // for (map<uint, map<uint, uint> >::const_iterator ts = thrsFreq.begin();
  //      ts != thrsFreq.end(); ++ts) {
  //   cout << ts->first;
  //   for (map<uint, uint>::const_iterator t = ts->second.begin();
  //        t != ts->second.end(); ++t)
  //     cout << '\t' << t->first << '\t' << t->second << endl;
  //   cout << endl;
  // }
  // cout << endl;
  cout << "thrs freq sim" << endl;
  for (map<uint, map<uint, uint> >::const_iterator ts = thrsFreqSim.begin();
       ts != thrsFreqSim.end(); ++ts) {
    cout << ts->first;
    for (map<uint, uint>::const_iterator t = ts->second.begin();
         t != ts->second.end(); ++t)
      cout << '\t' << t->first << '\t' << t->second << endl;
    cout << endl;
  }
  cout << endl;

  for (uint i = 0; i < noKwd; ++i) {
    for (uint j = 0; j < noAns; ++j)
      cout << topks[i][j] << ":" << data.data[topks[i][j]] << "; ";
    cout << endl;
  }

  cerr << "OK" << endl;
}

void exp(
  SimMetric &simMetric, 
  Alg alg,
  PerfDataName dataName, 
  uint noDataStart, 
  uint noDataStop, 
  uint noDataStep)
{
  cerr << "export " 
       << getDataName(dataName)
       << '(' 
       << utosh(noDataStart) << ", "
       << utosh(noDataStop) << ", "
       << utosh(noDataStep) << ")"
       << endl;

  const uint noQue  =     100;

  for (uint noData = noDataStart; noData <= noDataStop; noData += noDataStep) {
    cerr << "noData " << utosh(noData) << endl;

    PerfData data(simMetric.gramGen, dataName, noData);
    PerfQue que(data, noQue);
    PerfIndex index(data, simMetric.gramGen, getAlgVer(alg));

    string path("/data/search_dbms/imdb/");
    string filename(path + "que." + utosh(noData) + ".txt");
    
    ofstream file(filename.c_str(), ios::out);

    if (!file) {
      cerr << "can't open output file \"" << filename << "\"" << endl;
      exit(EXIT_FAILURE);
    }

    cerr << "writing \"" << filename << "\"...";
    cerr.flush();

    // for (uint i = 0; i < data.no; ++i)
    //   file << i << '\t' << data.data[i] << '\t' << data.weights[i] << endl;

    // for (unordered_map<uint, range<uint*> >::const_iterator 
    //        i = index.index.lists.begin();
    //      i != index.index.lists.end(); ++i)
    //   for (uint* j = i->second._begin; j != i->second._end; ++j)
    //     file << i->first<< '\t' << *j << endl;
 
    for (vector<string>::const_iterator q = que.que.begin(); 
         q != que.que.end(); ++q)
      file << *q << endl;
   
    cerr << "OK" << endl;

    break;
  }
  cerr << "OK" << endl;  
}
#endif

int main(int argc, char* argv[])
{
  cout << fixed << setprecision(2);
  cerr << fixed << setprecision(8);

  GramGenFixedLen gramGen(3);   // 3
  SimMetricJacc simMetric(gramGen);
  string scoreName = "sum";

  cerr << scoreName << ", " << simMetric.name << endl;

  Topk::Heap::nLongParam = .01;               // IMDB .01; WebCoprus .017
  
  smoke(AlgHeap);
  // smokeMulti();
  // perf(simMetric, AlgHeap, PerfDataIMDB, 1500000);
  // perf(simMetric, AlgHeap, PerfDataENRON, 400000);
  // perfMulti(simMetric, PerfDataIMDB, 1500000);
  // perfMulti(simMetric, PerfDataENRON, 400000);
  // debug();

  // exp_xDataSize_yTime_zAlg(
  //   simMetric, AlgHeap, PerfDataIMDB, 200000, 200000, 200000);
  // exp_xDataSize_yTime_Group(
  //   simMetric, PerfDataIMDB, 200000, 200000, 200000, 2, 2);

  // exp_xDataSize_yTime_zAlg(
  //   simMetric, (Alg)atoi(argv[1]), PerfDataIMDB, 200000, 1200000, 200000);
  // exp_xDataSize_yTime_zAlg(
  //   simMetric, (Alg)atoi(argv[1]), PerfDataWebCorpus, 400000, 2400000, 400000);

  // exp_PerfThr_Iter(
  //   simMetric, PerfDataIMDB, 200000, 1200000, 200000, scoreName);
  // exp_PerfThr_Iter(
  //   simMetric, PerfDataWebCorpus, 400000, 2400000, 400000, scoreName);

  // exp_PerfThr_Time(
  //   simMetric, 
  //   (Alg)atoi(argv[1]), 
  //   PerfDataIMDB, 
  //   200000, 
  //   1200000, 
  //   200000, 
  //   scoreName);
  // exp_PerfThr_Time(
  //   simMetric, 
  //   (Alg)atoi(argv[1]), 
  //   PerfDataWebCorpus, 
  //   400000, 
  //   2400000, 
  //   400000, 
  //   scoreName);

  // exp_xDataSize_yTime_zAlg_Perf(
  //   simMetric,
  //   (Alg)atoi(argv[1]),
  //   PerfDataIMDB,
  //   200000,
  //   1200000,
  //   200000,
  //   scoreName);
  // exp_xDataSize_yTime_zAlg_Perf(
  //   simMetric, 
  //   (Alg)atoi(argv[1]),
  //   PerfDataWebCorpus,
  //   400000,
  //   2400000,
  //   400000,
  //   scoreName);

  // exp_xQue_yTime_zThr(
  //   simMetric, (Alg)atoi(argv[1]), PerfDataIMDB, 1200000);
  // exp_xQue_yTime_zThr(
  //   simMetric, (Alg)atoi(argv[1]), PerfDataWebCorpus, 2400000);

  // exp_xQue_yThr(simMetric, AlgTwoPhase, PerfDataIMDB, atoi(argv[1]));
  // exp_xQue_yThr(simMetric, AlgTwoPhase, PerfDataWebCorpus, atoi(argv[1]));

  // exp(simMetric, AlgHeap, PerfDataIMDB, 200000, 1200000, 200000);

  return 0;
}
