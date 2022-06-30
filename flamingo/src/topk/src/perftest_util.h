/*
  $Id: perftest_util.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 06/25/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _perftest_util_h_
#define _perftest_util_h_

#include "topkindex.h"
#include "topkindex.v1.h"

enum PerfDataName 
{
  PerfDataIMDB, 
  PerfDataWebCorpus, 
  PerfDataDBLP, 
  PerfDataENRON
};

std::string getDataName(PerfDataName dataName);

class PerfData 
{
public:
  static const std::string path;
  std::string fn, fnStr, fnWht, fnNGr;

  uint no;
  std::string *data;
  float *weights;
  uint *nograms;
 
  struct Record 
  {
    std::string text;
    float weight;
    
    bool operator<(const Record &r) const { return weight < r.weight; }
    bool operator>(const Record &r) const { return weight > r.weight; }
    
    friend std::ostream& operator<<(std::ostream& out, const Record &r);
  };

  PerfData(const GramGen &gramGen, PerfDataName name, uint no = 0);
  ~PerfData() { delete [] data; delete [] weights; delete [] nograms; }

  void setFn(std::string fn);
  bool exist()
    const; 
  void load();
  void save() 
    const;
  
private:
  PerfData(const PerfData&);
  PerfData& operator=(const PerfData&);
};

class PerfQue 
{
public:
  std::vector<std::string> que;
  uint no;
  uint noKwd;
  
  PerfQue(
    const PerfData &data, 
    uint no, 
    uint noData = 0,
    uint noKwd = 1,
    uint noEd = 1);

private:
  PerfQue(const PerfQue&);
  PerfQue& operator=(const PerfQue&);
};

class PerfAns 
{
public:
  std::vector<std::vector<uint> > ans;
  uint no;
  
  struct Answer 
  {
    uint wID;
    float score;
  
    Answer(uint wID, float score): 
      wID(wID), 
      score(score) 
      {}
    
    bool operator<(const Answer &a) const { return score < a.score; }
  };

  PerfAns(
    const PerfData &data, 
    const PerfQue &que, 
    const SimMetric &simMetric,      
    uint no, 
    bool isWeight = true);

  void load(const std::string &filename);
  void save(const std::string &filename) 
    const;
  
  static bool isEqual(
    const PerfData &data, 
    const SimMetric &simMetric,
    const std::vector<uint> &topk, 
    const std::vector<uint> &ans, 
    const Query &q);
  
private:
  PerfAns(const PerfAns&);
  PerfAns& operator=(const PerfAns&);
};

class PerfIndex 
{
public:
  Topk::Index index;
  Topk::Index_v1 index_v1;

  PerfIndex(const PerfData &data, const GramGen &gramGen, uint ver);
  
private:
  PerfIndex(const PerfIndex&);
  PerfIndex& operator=(const PerfIndex&);
};

#endif
