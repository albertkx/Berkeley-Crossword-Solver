/*
  $Id: statsutil.h 5718 2010-09-09 05:39:08Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 07/08/2007
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _statsutil_h_
#define _statsutil_h_

#include <iostream>
#include <vector>
#include <sys/time.h>

// enumerations for time formats
enum TimeFormat {
  TFSEC,
  TFMSEC,
  TFUSEC      
};

class StatsUtil {
private:
  struct timeval t1, t2;
  struct timezone tz;
  
public:  
  void startTimeMeasurement() { gettimeofday(&t1, &tz); }
  void stopTimeMeasurement() { gettimeofday(&t2, &tz); }
  double getTimeMeasurement(TimeFormat t = TFMSEC);
};


class IndexStats {
public:
  unsigned dictSize;
  unsigned gramLen;
  unsigned maxStrLen;
  unsigned ftFanout;
  unsigned partFilters;
  double buildTime;
  unsigned adds;
  
  IndexStats() : dictSize(0), gramLen(0), maxStrLen(0), ftFanout(0), partFilters(0), buildTime(0), adds(0) {}
  virtual void add(const IndexStats* ixStats);
  virtual void avg();
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
  
  virtual ~IndexStats() {}
};

class HolesGlobalIndexStats : public IndexStats {
public:
  float reductionRatio;
  unsigned numberHoles;
  
  HolesGlobalIndexStats() : IndexStats(), reductionRatio(0.0f), numberHoles(0) {}
  virtual void add(const IndexStats* ixStats);
  virtual void avg();
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
};

class UnionGlobalIndexStats : public IndexStats {
public:
  float reductionRatio;
  
  UnionGlobalIndexStats() : IndexStats(), reductionRatio(0.0f) {}
  virtual void add(const IndexStats* ixStats);
  virtual void avg();
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
};

class OnDiskIndexStats : public IndexStats {
public:
  unsigned runBufferSize;
  double createRunsTime;
  double mergeRunsTime;
  double reorgTime;
  double indexSize;
  
  OnDiskIndexStats()
    : IndexStats(), runBufferSize(0), createRunsTime(0.0), mergeRunsTime(0.0), reorgTime(0), indexSize(0) {}
  
  virtual void add(const IndexStats* ixStats);
  virtual void avg();
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
};


class QueryStats {
public:
  unsigned qid;
  unsigned strLen;
  unsigned mergeThresh;
  double simThresh;
  double threshTime;
  double preprocTime;
  double mergeTime;
  double postprocTime;
  double panicTime;
  double totalTime;
  double candidates;
  double panics;
  double results;
  
  QueryStats() : qid(0xFFFFFFFF), strLen(0), mergeThresh(0), simThresh(0.0), threshTime(0.0), 
		 preprocTime(0.0), mergeTime(0.0), postprocTime(0.0), panicTime(0.0),
		 totalTime(0.0), candidates(0.0), panics(0.0), results(0.0) {}

  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);    
};

class OnDiskQueryStats : public QueryStats {
public:
  double invListSeeks;
  double invListData;
  double postprocPages;
  double initialCandidates;

  OnDiskQueryStats() : QueryStats(), invListSeeks(0.0), invListData(0.0), postprocPages(0.0), initialCandidates(0.0) {}

  virtual void print(std::ostream& stream);
  virtual void printHeader(std::ostream& stream);
  virtual void reset();
};



class WorkloadStats {
private:
  // conf interval and mean for totalSearchTime
  double confMean;
  double confLbound;
  double confUbound;
  
public:  
  double threshTime;
  double preprocTime;
  double mergeTime;
  double postprocTime;
  double panicTime;
  double totalTime;
  double candidates;  
  double panics;  
  std::vector<QueryStats> allQueryStats;

  WorkloadStats() : confMean(0.0), confLbound(0.0), confUbound(0.0), threshTime(0.0), preprocTime(0.0),
		    postprocTime(0.0), panicTime(0.0), totalTime(0.0), candidates(0.0), panics(0.0) {}
    
  virtual void add(const QueryStats* queryStats, bool saveQueryStats = true);
  virtual void avg(unsigned queries, unsigned runs = 1);
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
  
  virtual ~WorkloadStats() {}
};

class OnDiskWorkloadStats : public WorkloadStats {
public:
  double invListSeeks; // number of seeks for getting inverted lists
  double invListData;  // bytes transferred for getting all inverted lists
  double postprocPages; // pages in postprocessing
  double initialCandidates;  

  OnDiskWorkloadStats() : WorkloadStats(), invListSeeks(0.0), invListData(0.0), postprocPages(0.0), initialCandidates(0.0) {}
  
  virtual void add(const QueryStats* queryStats, bool saveQueryStats = true);
  virtual void avg(unsigned queries, unsigned runs = 1);
  virtual void reset();
  virtual void printHeader(std::ostream& stream);
  virtual void print(std::ostream& stream);
};

#endif
