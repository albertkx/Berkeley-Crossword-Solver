/*
  $Id: statsutil.cc 5718 2010-09-09 05:39:08Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD License.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#include <cstring>
#include "statsutil.h"
#include <iostream> // ALEX remove

using namespace std; // ALEX remove

void
IndexStats::
add(const IndexStats* ixStats) {
  dictSize += ixStats->dictSize;
  gramLen += ixStats->gramLen;
  maxStrLen += ixStats->maxStrLen;
  ftFanout += ixStats->ftFanout;
  partFilters += ixStats->partFilters;
  buildTime += ixStats->buildTime;
  adds++;
}

void
IndexStats::
avg() {
  dictSize /= adds;
  gramLen /= adds;
  maxStrLen /= adds;
  ftFanout /= adds;
  partFilters /= adds;
  buildTime /= (double)adds;
}

void
IndexStats::
reset() {
  dictSize = 0;
  gramLen = 0;
  maxStrLen = 0;
  ftFanout = 0;
  partFilters = 0;
  buildTime = 0.0;
  adds = 0;
}


void
HolesGlobalIndexStats::
add(const IndexStats* ixStats) {
  this->IndexStats::add(ixStats);
  reductionRatio += dynamic_cast<const HolesGlobalIndexStats*>(ixStats)->reductionRatio;
  numberHoles += dynamic_cast<const HolesGlobalIndexStats*>(ixStats)->numberHoles;
}

void
HolesGlobalIndexStats::
avg() {
  this->IndexStats::avg();
  reductionRatio /= (float)adds;
  numberHoles /= adds;

}

void
HolesGlobalIndexStats::
reset() {
  this->IndexStats::reset();
  reductionRatio = 0.0f;
  numberHoles = 0;
}


void
UnionGlobalIndexStats::
add(const IndexStats* ixStats) {
  this->IndexStats::add(ixStats);
  reductionRatio += dynamic_cast<const UnionGlobalIndexStats*>(ixStats)->reductionRatio;
}

void
UnionGlobalIndexStats::
avg() {
  this->IndexStats::avg();
  reductionRatio /= (float)adds;
}

void
UnionGlobalIndexStats::
reset() {
  this->IndexStats::reset();
  reductionRatio = 0.0f;
}


void
OnDiskIndexStats::
add(const IndexStats* ixStats) {
  this->IndexStats::add(ixStats);
  runBufferSize += dynamic_cast<const OnDiskIndexStats*>(ixStats)->runBufferSize;
  createRunsTime += dynamic_cast<const OnDiskIndexStats*>(ixStats)->createRunsTime;
  mergeRunsTime += dynamic_cast<const OnDiskIndexStats*>(ixStats)->mergeRunsTime;
  reorgTime += dynamic_cast<const OnDiskIndexStats*>(ixStats)->reorgTime;
  indexSize += dynamic_cast<const OnDiskIndexStats*>(ixStats)->indexSize;
}

void
OnDiskIndexStats::
avg() {
  this->IndexStats::avg();
  runBufferSize /= adds;
  createRunsTime /= (double)adds;
  mergeRunsTime /= (double)adds;
  reorgTime /= (double)adds;
  indexSize /= adds;
}

void
OnDiskIndexStats::
reset() {
  this->IndexStats::reset();
  runBufferSize = 0;
  createRunsTime = 0.0;
  mergeRunsTime = 0.0;
  reorgTime = 0.0;
  indexSize = 0;
}

void
IndexStats::
print(std::ostream& stream) {
  stream << dictSize << ";"
	 << gramLen << ";"
	 << maxStrLen << ";"
	 << ftFanout << ";"
	 << partFilters << ";"
	 << buildTime;
}

void
HolesGlobalIndexStats::
print(std::ostream& stream) {
  stream << dictSize << ";"
	 << gramLen << ";"
	 << maxStrLen << ";"
	 << ftFanout << ";"
	 << partFilters << ";"
	 << buildTime << ";"
	 << reductionRatio << ";"
	 << numberHoles;
}

void
UnionGlobalIndexStats::
print(std::ostream& stream) {
  stream << dictSize << ";"
	 << gramLen << ";"
	 << maxStrLen << ";"
	 << ftFanout << ";"
	 << partFilters << ";"
	 << buildTime << ";"
	 << reductionRatio;
}

void
OnDiskIndexStats::
print(std::ostream& stream) {
  stream << dictSize << ";"
	 << gramLen << ";"
	 << maxStrLen << ";"
	 << ftFanout << ";"
	 << partFilters << ";"
	 << buildTime << ";"
	 << runBufferSize << ";"
	 << createRunsTime << ";"
	 << mergeRunsTime << ";"
	 << reorgTime << ";"
	 << indexSize;
}

void
IndexStats::
printHeader(std::ostream& stream) {
  stream << "DictSize" << ";"
	 << "GramLen" << ";"
	 << "MaxStrLen" << ";"
	 << "FtFanout" << ";"
	 << "PartFilters" << ";"
	 << "IndexBuildTime";
}

void
HolesGlobalIndexStats::
printHeader(std::ostream& stream) {
  stream << "DictSize" << ";"
	 << "GramLen" << ";"
	 << "MaxStrLen" << ";"
	 << "FtFanout" << ";"
	 << "PartFilters" << ";"
	 << "IndexBuildTime" << ";"
	 << "ReductionRatio" << ";"
	 << "Holes";
}

void
UnionGlobalIndexStats::
printHeader(std::ostream& stream) {
  stream << "DictSize" << ";"
	 << "GramLen" << ";"
	 << "MaxStrLen" << ";"
	 << "FtFanout" << ";"
	 << "PartFilters" << ";"
	 << "IndexBuildTime" << ";"
	 << "ReductionRatio";
}

void
OnDiskIndexStats::
printHeader(std::ostream& stream) {
  stream << "DictSize" << ";"
	 << "GramLen" << ";"
	 << "MaxStrLen" << ";"
	 << "FtFanout" << ";"
	 << "PartFilters" << ";"
	 << "IndexBuildTime" << ";"
	 << "BufferSize" << ";"
	 << "CreateRunsTime" << ";"
	 << "CergeRunsTime" << ";"
	 << "ReorgTime" << ";"
	 << "IndexSize";
}

void
QueryStats::
reset() {
  qid = 0;
  strLen = 0;
  mergeThresh = 0;
  simThresh = 0.0;
  threshTime = 0.0;
  preprocTime = 0.0;
  mergeTime = 0.0;
  postprocTime = 0.0;
  panicTime = 0.0;
  totalTime = 0.0;
  candidates = 0.0;
  panics = 0.0;
  results = 0.0;
}

void
OnDiskQueryStats::
reset() {
  qid = 0;
  strLen = 0;
  mergeThresh = 0;
  simThresh = 0.0;
  threshTime = 0.0;
  preprocTime = 0.0;
  mergeTime = 0.0;
  postprocTime = 0.0;
  panicTime = 0.0;
  totalTime = 0.0;
  candidates = 0.0;
  panics = 0.0;
  results = 0.0;
  invListSeeks = 0.0;
  invListData = 0.0;
  postprocPages = 0.0;
  initialCandidates = 0.0;
}

void
QueryStats::
print(std::ostream& stream) {
  stream << qid << ","
	 << strLen << ","
	 << mergeThresh << ","
	 << simThresh << ","
	 << threshTime << ","
	 << preprocTime << ","
	 << mergeTime << ","
	 << postprocTime << ","
	 << panicTime << ","
	 << totalTime << ","
	 << candidates << ","
	 << panics << ","
	 << results;
}

void
OnDiskQueryStats::
print(std::ostream& stream) {
  stream << qid << ","
	 << strLen << ","
	 << mergeThresh << ","
	 << simThresh << ","
	 << threshTime << ","
	 << preprocTime << ","
	 << mergeTime << ","
	 << postprocTime << ","
	 << panicTime << ","
	 << totalTime << ","
	 << candidates << ","
	 << panics << ","
	 << results << ","
	 << invListSeeks << ","
	 << invListData << ","
	 << postprocPages;
}

void
QueryStats::
printHeader(std::ostream& stream) {
  stream << "QID" << ","
	 << "StrLen" << ","
	 << "MergeThresh" << ","
	 << "SimThresh" << ","
	 << "ThreshTime" << ","
	 << "PreprocTime" << ","
	 << "MergeTime" << ","
	 << "PostprocTime" << ","
	 << "PanicTime" << ","
	 << "TotalTime" << ","
	 << "Candidates" << ","
	 << "Panics" << ","
	 << "Results";
}

void
OnDiskQueryStats::
printHeader(std::ostream& stream) {
  stream << "QID" << ","
	 << "StrLen" << ","
	 << "MergeThresh" << ","
	 << "SimThresh" << ","
	 << "ThreshTime" << ","
	 << "PreprocTime" << ","
	 << "MergeTime" << ","
	 << "PostprocTime" << ","
	 << "PanicTime" << ","
	 << "TotalTime" << ","
	 << "InitialCandidates" << ","
	 << "Candidates" << ","
	 << "Panics" << ","
	 << "Results" << ","
	 << "InvListSeeks" << ","
	 << "InvListData" << ","
	 << "PostprocPages";
}

void
WorkloadStats::
print(std::ostream& stream) {
  stream << threshTime << ";"
	 << preprocTime << ";"
	 << mergeTime << ";"
	 << postprocTime << ";"
	 << panicTime << ";"
	 << totalTime << ";"
	 << candidates << ";"
	 << panics;
}

void
OnDiskWorkloadStats::
print(std::ostream& stream) {
  stream << threshTime << ";"
	 << preprocTime << ";"
	 << mergeTime << ";"
	 << postprocTime << ";"
	 << panicTime << ";"
	 << totalTime << ";"
	 << initialCandidates << ";"
	 << candidates << ";"
	 << panics << ";" 
	 << invListSeeks << ";"
	 << invListData << ";"
	 << postprocPages;
}

void
WorkloadStats::
printHeader(std::ostream& stream) {
  stream << "ThreshTime" << ";"
	 << "PreprocTime" << ";"
	 << "MergeTime" << ";"
	 << "PostprocTime" << ";"
	 << "PanicTime" << ";"
	 << "TotalTime" << ";"
	 << "Candidates" << ";"
	 << "Panics";
}

void
OnDiskWorkloadStats::
printHeader(std::ostream& stream) {
  stream << "ThreshTime" << ";"
	 << "PreprocTime" << ";"
	 << "MergeTime" << ";"
	 << "PostprocTime" << ";"
	 << "PanicTime" << ";"
	 << "TotalTime" << ";"
	 << "InitialCandidates" << ";"
	 << "Candidates" << ";"
	 << "Panics" << ";"
	 << "InvListSeeks" << ";"
	 << "InvListData" << ";"
	 << "PostprocPages";
}

void
WorkloadStats::
reset() {
  threshTime = 0.0;
  preprocTime = 0.0;
  mergeTime = 0.0;
  postprocTime = 0.0;
  panicTime = 0.0;
  totalTime = 0.0;
  candidates = 0.0;
  panics = 0.0;
}

void
OnDiskWorkloadStats::
reset() {
  threshTime = 0.0;
  preprocTime = 0.0;
  mergeTime = 0.0;
  postprocTime = 0.0;
  panicTime = 0.0;
  totalTime = 0.0;
  candidates = 0.0;
  panics = 0.0;  
  invListSeeks = 0.0;
  invListData = 0.0;
  postprocPages = 0.0;
  initialCandidates = 0.0;
}

void
WorkloadStats::
add(const QueryStats* queryStats, bool saveQueryStats) {
  if(queryStats) {
    if(saveQueryStats) allQueryStats.push_back(*queryStats);
    threshTime += queryStats->threshTime;
    preprocTime += queryStats->preprocTime;
    mergeTime += queryStats->mergeTime;
    postprocTime += queryStats->postprocTime;
    panicTime += queryStats->panicTime;
    totalTime += queryStats->totalTime;
    candidates += queryStats->candidates;
    panics += queryStats->panics;  
  }
}

void
OnDiskWorkloadStats::
add(const QueryStats* queryStats, bool saveQueryStats) {
  if(queryStats) {
    if(saveQueryStats) allQueryStats.push_back(*queryStats);
    threshTime += queryStats->threshTime;
    preprocTime += queryStats->preprocTime;
    mergeTime += queryStats->mergeTime;
    postprocTime += queryStats->postprocTime;
    panicTime += queryStats->panicTime;
    totalTime += queryStats->totalTime;
    candidates += queryStats->candidates;
    panics += queryStats->panics;  
    invListSeeks += dynamic_cast<const OnDiskQueryStats*>(queryStats)->invListSeeks;
    invListData += dynamic_cast<const OnDiskQueryStats*>(queryStats)->invListData;
    postprocPages += dynamic_cast<const OnDiskQueryStats*>(queryStats)->postprocPages;
    initialCandidates += dynamic_cast<const OnDiskQueryStats*>(queryStats)->initialCandidates;
  }
}

void
WorkloadStats::
avg(unsigned queries, unsigned runs) {
  threshTime /= queries * runs;
  preprocTime /= queries * runs;
  mergeTime /= queries * runs;
  postprocTime /= queries * runs;
  panicTime /= queries * runs;
  totalTime /= queries * runs;
  candidates /= queries * runs;
  panics /= runs;
}

void
OnDiskWorkloadStats::
avg(unsigned queries, unsigned runs) {
  threshTime /= queries * runs;
  preprocTime /= queries * runs;
  mergeTime /= queries * runs;
  postprocTime /= queries * runs;
  panicTime /= queries * runs;
  totalTime /= queries * runs;
  candidates /= queries * runs;
  panics /= runs;
  invListSeeks /= queries * runs;
  invListData /= queries * runs;
  postprocPages /= queries * runs;
  initialCandidates /= queries * runs;
}

double
StatsUtil::
getTimeMeasurement(TimeFormat format) {
  unsigned totalTime = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
  double tval = 0;

  switch(format) {
  case TFSEC: tval = static_cast<double>(totalTime) / 1000000; break;
  case TFMSEC: tval = static_cast<double>(totalTime) / 1000; break;
  case TFUSEC: tval = static_cast<double>(totalTime); break;
  }

  return tval;
}
