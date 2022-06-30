/*
  $Id: perftest_util.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/25/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "perftest_util.h"

#include <cmath>
#include <iomanip>

#define DEBUG_TIMER

#include "misc.h"
#include "util/src/input.h"
#include "util/src/io.h"
#include "util/src/misc.h"
#include "util/src/debug.h"

using namespace std;
using namespace Topk;

const string PerfData::path = "/data/topk/";

string getDataName(PerfDataName dataName)
{
  switch (dataName) {
  case PerfDataIMDB:
    return "IMDB";
  case PerfDataWebCorpus:
    return "WebCorpus";
  case PerfDataDBLP:
    return "DBLP";
  default:
    return "Unknown";
  }
}

PerfData::PerfData(const GramGen &gramGen, PerfDataName name, uint no): 
  no(no)
{
  vector<string> dataWithWeights;
  vector<Record> records;
        
  switch (name) {
  case PerfDataIMDB:
    setFn(path + "imdb/actors.txt");

    if (exist()){
      load();
      return;
    }

    readString(dataWithWeights, fn, no);
    no = dataWithWeights.size();

    for (uint i = 0; i < no; i++) {

      string dat = dataWithWeights[i];
      uint pos = dat.find('\t');

      Record r;
      r.text = dat.substr(0, pos);
      // r.weight = 1 - 1. / atoi(dat.substr(pos + 1).c_str());
      // r.weight = atoi(dat.substr(pos + 1).c_str()) / 5047.;
      r.weight = log(atoi(dat.substr(pos + 1).c_str())) / log(5319);

      records.push_back(r);
    }

    break;
  case PerfDataWebCorpus:
    setFn(path + "webcorpus/webcorpus.shuf.2.4m.txt");

    if (exist()){
      load();
      return;
    }

    readString(dataWithWeights, fn, no);
    no = dataWithWeights.size();

    for (uint i = 0; i < no; i++) {
      string dat = dataWithWeights[i];
      uint pos = dat.find('\t');

      Record r;
      r.text = dat.substr(0, pos);
      // r.weight = 1 - 1. / atoi(dat.substr(pos + 1).c_str());
      // r.weight = atoi(dat.substr(pos + 1).c_str()) / 15087432.;
      r.weight = log(atoi(dat.substr(pos + 1).c_str())) / log(15087432);

      records.push_back(r);
    }
    break;
  case PerfDataDBLP:
    setFn(path + "dblp/articles.rank.csv");

    if (exist()){
      load();
      return;
    }

    readString(dataWithWeights, fn, no, 700);
    no = dataWithWeights.size();

    for (uint i = 0; i < no; i++) {

      string dat = dataWithWeights[i];
      uint pos = dat.find('\t');

      Record r;
      r.text = dat.substr(0, pos);
      r.weight = 1. / atoi(dat.substr(pos + 1).c_str());

      records.push_back(r);
    }
    break;
  case PerfDataENRON:
    setFn(path + "enron/words.txt");

    if (exist()){
      load();
      return;
    }

    readString(dataWithWeights, fn, no);
    no = dataWithWeights.size();

    for (uint i = 0; i < no; i++) {
      string dat = dataWithWeights[i];
      uint pos = dat.find('\t');

      Record r;
      r.text = dat.substr(0, pos);
      r.weight = atof(dat.substr(pos + 1).c_str());

      records.push_back(r);
    }
    break;    
  }
  
  // sort data elements by weight
  stable_sort(records.begin(), records.end(), greater<Record>());

  data = new string[no];
  weights = new float[no];
  nograms = new uint[no];

  for (uint i = 0; i < no; i++) {
    data[i] = records[i].text;
    weights[i] = records[i].weight;
    nograms[i] = gramGen.getNumGrams(data[i]);
  }

  save();
}

void PerfData::setFn(string fn) 
{
  this->fn = fn;
  fnStr = fn.substr(0, fn.rfind('.')) + "/data/data." + utosh(no) + ".str.txt";
  fnWht = fn.substr(0, fn.rfind('.')) + "/data/data." + utosh(no) + ".wht.bin";
  fnNGr = fn.substr(0, fn.rfind('.')) + "/data/data." + utosh(no) + ".ngr.bin";
}

bool PerfData::exist()
  const
{ 
  return existFile(fnStr) && existFile(fnWht) && existFile(fnNGr);
}

void PerfData::load()
{
  data = new string[no];
  weights = new float[no];
  nograms = new uint[no];
  readTextFile<string>(fnStr, data);
  readBinaryFile<float>(fnWht, weights);
  readBinaryFile<uint>(fnNGr, nograms);
}  

void PerfData::save() 
  const
{
  writeTextFile<string>(fnStr, data, data + no);
  writeBinaryFile<float>(fnWht, weights, weights + no);
  writeBinaryFile<uint>(fnNGr, nograms, nograms + no);
}

string randomEdit(string s, uint noEd)
{
  for (uint i = 0; i < noEd; ++i) {
    uint pos = rand() % s.length();
    s[pos] = s[pos] + 1;
  }

  return s;
}

PerfQue::PerfQue(
  const PerfData &data, 
  uint no, 
  uint noData, 
  uint noKwd, 
  uint noEd): 
  no(no), noKwd(noKwd) 
{
  if (noData == 0 || noData > data.no)
    noData = data.no;

  string fn = data.fn.substr(0, data.fn.rfind('.')) + 
    "/que/data." + utosh(noData) + ".que." + utosh(no) + 
    (noKwd == 1? "" : "." + utosh(noKwd) + "." + utosh(noEd)) + ".txt";
  
  if (existFile(fn)) 
    readString(que, fn);
  else {
    selectRandom(data.data, data.data + noData, back_inserter(que), no);

    srand(time(0));
    for (uint i = 0; i < no; i += noKwd)
      for (uint j = 1; j < noKwd && i + j < no; ++j)
        que[i + j] = randomEdit(que[i], noEd);

    writeTextFile<string>(fn, que.begin(), que.end());
  }
}
  
PerfAns::PerfAns(
  const PerfData &data, 
  const PerfQue &que, 
  const SimMetric &simMetric,      
  uint no, 
  bool isWeight): 
  no(no) 
{
  string fn = data.fn.substr(0, data.fn.rfind('.')) + 
    "/ans/data." + utosh(data.no) + 
    ".que." + utosh(que.no) + 
    ".ans." + utosh(no) + 
    "." + (isWeight ? "wh" : "nw") + 
    ".q." + utos(simMetric.gramGen.getGramLength()) + 
    "." + simMetric.name + ".bin";

  if (existFile(fn))
    load(fn);
  else 
    {
      cerr << "building ans..." << endl;
      TIMER_START("ans", que.no);
      for (vector<string>::const_iterator qu = que.que.begin(); 
           qu != que.que.end(); ++qu) {
        multiset<Answer, less<Answer> > ansTopk;
        set<uint> gramQue;
        simMetric.gramGen.decompose(*qu, gramQue);
        for (uint i = 0; i < data.no; i++) {
          set<uint> gramData;
          simMetric.gramGen.decompose(data.data[i], gramData);
          set<uint> gramInter;
          set_intersection(
            gramQue.begin(), 
            gramQue.end(), 
            gramData.begin(), 
            gramData.end(), 
            inserter(gramInter, gramInter.begin()));
          if (gramInter.size()) {
            float sim = simMetric(*qu, data.data[i]);
            ansTopk.insert(Answer(i, score(sim, data.weights[i])));
            if (ansTopk.size() > no)
              ansTopk.erase(ansTopk.begin());
          }
        
        }
        vector<unsigned> answID;
        for (multiset<Answer, less<Answer> >::const_reverse_iterator a = 
               ansTopk.rbegin(); a != ansTopk.rend(); ++a) {
          answID.push_back(a->wID);
        }
        ans.push_back(answID);
        TIMER_STEP();
      }
      TIMER_STOP();
      cerr << "OK" << endl;
    
      save(fn);
    }
}

void PerfAns::load(const string &filename)
{
  vector<uint> *data = new vector<uint>;
  readBin(*data, filename);
  for (vector<uint>::const_iterator dat = data->begin(); dat != data->end(); ) {
    uint count = *dat; 
    ++dat;
    vector<uint> a;
    for (uint i = 0; i < count; i++, ++dat)
      a.push_back(*dat);
    ans.push_back(a);
  }
  delete data;
}

void PerfAns::save(const string &filename) 
  const
{
  vector<uint> *data = new vector<uint>;
  for (vector<vector<uint> >::const_iterator an = ans.begin();
       an != ans.end(); ++an) {
    data->push_back(an->size());
    for (vector<uint>::const_iterator a = an->begin(); a != an->end(); ++a)
      data->push_back(*a);
  }
  writeBinaryFile<uint>(filename, data->begin(), data->end());
  delete data;
}
  
bool PerfAns::isEqual(
  const PerfData &data, 
  const SimMetric &simMetric,
  const vector<uint> &topk, 
  const vector<uint> &ans, 
  const Query &q)
{
  const float eps = .0001;  // .000001
  if (topk != ans) {
    for (uint i = 0; i < topk.size(); i++) {
      float 
        sc_t = score(
          simMetric(q.str, data.data[topk[i]]), data.weights[topk[i]]), 
        sc_a = score(
          simMetric(q.str, data.data[ans[i]]), data.weights[ans[i]]);

      if (topk[i] != ans[i] && 
          !(fabs(sc_t - sc_a) < numeric_limits<float>::epsilon() + eps)) {
        SimMetricGramCount s(q.sim.gramGen);
        cerr << q.str << endl
             << "t: " << setw(7) << topk[i] << ' '
             << setw(20) << data.data[topk[i]] << ", "
             << setw(2) << static_cast<uint>(s(q.str, data.data[topk[i]])) 
             << ", "
             << simMetric(q.str, data.data[topk[i]]) << ' '
             << data.weights[topk[i]] << ' '
             << sc_t
             << endl
             << "a: " << setw(7) << ans[i] << ' '
             << setw(20) << data.data[ans[i]] << ", "
             << setw(2) << static_cast<uint>(s(q.str, data.data[ans[i]])) 
             << ", "
             << simMetric(q.str, data.data[ans[i]]) << ' '
             << data.weights[ans[i]] << ' '
             << sc_a
             << endl
             << fabs(sc_t - sc_a)
             << endl;
        return false;
      }
    }
    if (topk.size() != ans.size()) {
      cerr << "topk.size: " << topk.size() 
           << " ans.size " << ans.size() << endl;
      return false;
    }
  }
  return true;
}

PerfIndex::PerfIndex(const PerfData &data, const GramGen &gramGen, uint ver)
{
  string fn = data.fn.substr(0, data.fn.rfind('.')) + 
    "/gram/data." + utosh(data.no) + ".q." + utos(gramGen.getGramLength()) + 
    (ver ? ".v" + utos(ver) : "") + ".bin";

  if (existFile(fn))
    if (ver)
      index_v1.load(fn);
    else
      index.load(fn);
  else {
    if (ver) {
      index_v1.build(data.data, data.data + data.no, gramGen);
      index_v1.save(fn);
    }
    else {
      index.build(data.data, data.data + data.no, gramGen);
      index.save(fn);
    }
  }
}
