/*
  $Id: statsgen.h 5788 2010-10-22 10:09:57Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license.
    
  Date: 04/04/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _statsgen_h_
#define _statsgen_h_

#include "statsutil.h"
#include "ftindexermem.h"
#include "ftindexerdiscardlists.h"
#include "ftindexercombinelists.h"
#include "ftindexerondisk.h"
#include "ftsearcherondisk.h"
#include "ftsearchermem.h"

#include "common/src/gramgen.h"
#include "common/src/simmetric.h"

#include "util/src/looptimer.h"

using namespace std;

extern void genZipfDist(unsigned distinctVals, unsigned skew, double valFreqs[]);
extern void readString(vector<string> &data, const string &filenameData, unsigned count, unsigned maxLineLen);

enum QueriesDistribution {
  QD_UNIFORM,
  QD_ZIPF
};

enum OutputFlags {
  OF_WORKLOADSTATS = 0x0001,
  OF_QUERYSTATS = 0x0002,
  OF_QUERYRESULTS = 0x0004
};

class StatsGenConfig {
public:
  StatsGenConfig();
  
  // START: Config options for all types of indexes
  // ----------------------------------------------  
  GramGen* gramGen;
  SimMetric* simMetric;
  float simThresh;
  vector<AbstractFilter*> filterTypes;
  
  string dictDataFile;
  unsigned dictSizeStart, dictSizeStop, dictSizeStep;
  unsigned fanout;
  unsigned maxStrLen;
  bool autoPartFilter;

  unsigned numberQueries;
  unsigned distinctQueries;
  QueriesDistribution queriesDistrib;
  unsigned zipfSkew;
  unsigned numberRuns;
  
  QueryType qt;
  
  PostMergeFilter pmf;
  
  unsigned lcCharNum; 
  unsigned hcBuckets;
  
  char outputFlags;
  string outputFilePrefix;
  string wlStatsFile;    
  string queryStatsFile;
  string queryResultsFile;
  
  string existingWorkloadFile;
  string existingIndexFile;
  string existingStringContainer;
  
  string saveWorkloadFile;

  double tdistQuant; // for calculating confidence intervals
  // ----------------------------------------------
  // END: Config options for all types of indexes

  
  // START: Config options for compressed in-memory indexes
  // ----------------------------------------------
  float reductionStart, reductionStop, reductionStep;
  float dictSamplingFrac, queriesSamplingFrac;    
  bool sampleRatioCost;
  // ----------------------------------------------
  // END: Config options for compressed in-memory indexes
  

  // START: Config options for disk-based indexes
  // ----------------------------------------------
  bool dropCachesBeforeEachQuery;
  bool dropCachesBeforeBuildingIndex;
  bool disableStreamBuffer;
  unsigned runBuffer;
  bool scatteredOrg;  
  
  PhysOrd pho;
  unsigned avgStrLen;  
  unsigned bufferSlots;  
  // ----------------------------------------------
  // END: Config options for compressed in-memory indexes
  
  void setSimMetric(SimMetric* simMetric, float simThresh) { this->simMetric = simMetric; this->simThresh = simThresh; }
  void setGramGen(GramGen* gramGenerator) { gramGen = gramGenerator; }
  void setFanout(unsigned fanout) { this->fanout = fanout; }  
  void setMaxStrLen(const unsigned maxStrLen) { this->maxStrLen = maxStrLen; }
  void setDictDataFile(const string& dataFile) { dictDataFile = dataFile; }
  void setDictSize(unsigned start, unsigned stop, unsigned step) { dictSizeStart = start; dictSizeStop = stop; dictSizeStep = step; }
  void setDistinctQueries(unsigned d) {distinctQueries = d; }
  void setNumberQueries(unsigned numberQueries) { this->numberQueries = numberQueries; }
  void setQueriesDistribution(QueriesDistribution qd) { queriesDistrib = qd; }
  void setZipfSkew(unsigned s) {zipfSkew = s; }
  void setNumberRuns(unsigned r) { numberRuns = r; }
  void setOutputFilePrefix(const string & s) { outputFilePrefix = s; }
  void setOutputFlags(char outputFlags) { this->outputFlags = outputFlags; }
  void addPartFilter(AbstractFilter* f) { filterTypes.push_back(f); }
  void clearFilters();  
  void setAutoPartFilter(bool b) { autoPartFilter = b; }
  void setPostMergeFilter(PostMergeFilter f) { pmf = f; }  
  void useExistingWorkload(const string& fileName) { existingWorkloadFile = fileName; }
  void useExistingIndex(const string& fileName) { existingIndexFile = fileName; }
  void useExistingStringContainer(const string& fileName) { existingStringContainer = fileName; }
  
  void setReduction(float start, float stop, float step) { reductionStart = start; reductionStop = stop; reductionStep = step; }
  void setDictSamplingFrac(float f) { dictSamplingFrac = f; }
  void setQueriesSamplingFrac(float f) { queriesSamplingFrac = f; }
  void setSampleRatioCost(bool b) {sampleRatioCost = b; }
  void setRunBuffer(unsigned runBuffer) { this->runBuffer = runBuffer; }
  void setDropCachesBeforeEachQuery(bool b) { dropCachesBeforeEachQuery = b; }
  void setDisableStreamBuffer(bool b) { disableStreamBuffer = b; }
  void setDropCachesBeforeBuildingIndex(bool b) { dropCachesBeforeBuildingIndex = b; }
  void setScatteredOrg(bool s) { scatteredOrg = s; }
  void setPhysOrd(PhysOrd p) { pho = p; }
  void setAvgStrLen(unsigned x) { avgStrLen = x; }
  void setTdistQuant(double t) { tdistQuant = t; }  
  void setLetterCountCharNum(unsigned c) { lcCharNum = c; }
  void setHashCountBuckets(unsigned c) { hcBuckets = c; }
  void setQueryType(QueryType qt) { this->qt = qt; }
  void saveWorkload(const string& outputFile) { saveWorkloadFile = outputFile; }

  ~StatsGenConfig(){ clearFilters(); }
};

// function template for creating any stringcontainer with default initialization
template<class T> 
T* createDefaultStringContainer(StatsGenConfig* config);

template<class T>
T* openStringContainer(StatsGenConfig* config);

template<class StatsGenConcrete>
struct StatsGenTraits;

template<class StatsGenConcrete>
class StatsGenAbs {
public:
  typedef StatsGenTraits<StatsGenConcrete> TraitsType;
  typedef typename TraitsType::FtIndexer FtIndexer;
  typedef typename TraitsType::FtSearcher FtSearcher;
  
  typedef FtIndexerTraits<FtIndexer> IndexerTraitsType;
  typedef typename IndexerTraitsType::StringContainer StringContainer;
  typedef typename IndexerTraitsType::InvList InvList;    
  
  typedef FtSearcherTraits<FtSearcher> SearcherTraitsType;
  typedef typename SearcherTraitsType::Merger Merger;
  
protected:
  StatsGenConfig* config;
  Merger* merger;

  void setPartFilters(FtIndexer* ftixer);

  void fillWorkload(StringContainer* strContainer, vector<string>& workload);
  void fillWorkloadNormal(StringContainer* strContainer, vector<string>& workload);
  void fillWorkloadZipf(StringContainer* strContainer, vector<string>& workload);  

  // this one must be overridden by the actual statsgen
  void setIndexerAndSearcher(StringContainer* strContainer,
			     const float reductionRatio, 
			     const unsigned ftFanout, 
			     const vector<string>* workload,
			     const unsigned runBufferSize) {
    static_cast<StatsGenConcrete*>(this)->setIndexerAndSearcher_Impl(strContainer,
								     reductionRatio, 
								     ftFanout,		   
								     workload,
								     runBufferSize);
  }
  
public:
  StatsGenAbs(){}
  StatsGenAbs(StatsGenConfig config) { this->config = config; }
  
  void generateWorkloads(unsigned distinctQueries, unsigned workloadSize);
  void genWloadSameZipf(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize);
  void genWloadDiffZipf(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize);
  void genWloadSameNormal(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize);
  void genWloadDiffNormal(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize);
  
  FtIndexer* ftIndexer;
  FtSearcher* ftSearcher;
  
  void setMerger(Merger* merger) { this->merger = merger; }
  
  // generate stats on approximate string search
  void generate();
};


template<class FtIndexer, class FtSearcher>
class StatsGen;

template<class TFtIndexer, class TFtSearcher>
struct StatsGenTraits<StatsGen<TFtIndexer, TFtSearcher> > {
  typedef TFtIndexer FtIndexer;
  typedef TFtSearcher FtSearcher;
};

template <class FtIndexer = FtIndexerMem<>, class FtSearcher = FtSearcherMem<> >
class StatsGen 
  : public StatsGenAbs<StatsGen<FtIndexer, FtSearcher> > {  

public:
  typedef FtIndexerTraits<FtIndexer> IndexerTraitsType;
  typedef typename IndexerTraitsType::StringContainer StringContainer;
  typedef typename IndexerTraitsType::InvList InvList;    
  
  StatsGen(StatsGenConfig* config) { this->config = config; }
  
  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 			
				  const vector<string>* workload,
				  const unsigned runBufferSize) {
    cout << "THIS SPECIALIZATION HAS NOT BEEN IMPLEMENTED YET!" << endl;
  }
};

// partial specialization to handle uncompressed indexer
template<class FtSearcher, class StringContainer, class InvList >
class StatsGen<FtIndexerMem<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerMem<StringContainer, InvList>, FtSearcher> > {  
  
public:
  typedef FtIndexerMem<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 	
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer, config->gramGen, config->maxStrLen, ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle compressed indexer HolesGlobalLLF
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerDiscardListsLLF<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerDiscardListsLLF<StringContainer, InvList>, FtSearcher> > {  
  
public:
  typedef FtIndexerDiscardListsLLF<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer, config->gramGen, reductionRatio, config->maxStrLen, ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);
  }
};

// partial specialization to handle compressed indexer HolesGlobalSLF
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerDiscardListsSLF<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerDiscardListsSLF<StringContainer, InvList>, FtSearcher> > {  

public:
  typedef FtIndexerDiscardListsSLF<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer, config->gramGen, reductionRatio, config->maxStrLen, ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle compressed indexer HolesGlobalRandom
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerDiscardListsRandom<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerDiscardListsRandom<StringContainer, InvList>, FtSearcher> > {  
  
 public:
  typedef FtIndexerDiscardListsRandom<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer, config->gramGen, reductionRatio, config->maxStrLen, ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle compressed indexer HolesGlobalTimeCost
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerDiscardListsTimeCost<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerDiscardListsTimeCost<StringContainer, InvList>, FtSearcher> > {  
  
 public:
  typedef FtIndexerDiscardListsTimeCost<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer, 
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer,
				  config->gramGen, 
				  reductionRatio, 
				  workload,
				  config->simMetric,
				  config->simThresh,
				  config->sampleRatioCost,
				  config->dictSamplingFrac,
				  config->queriesSamplingFrac,
				  config->maxStrLen, 
				  ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle compressed indexer HolesGlobalPanicCost
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerDiscardListsPanicCost<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerDiscardListsPanicCost<StringContainer, InvList>, FtSearcher> > {  
  
 public:
  typedef FtIndexerDiscardListsPanicCost<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer,
				  config->gramGen, 
				  reductionRatio, 
				  workload,
				  config->simMetric,
				  config->simThresh,
				  config->sampleRatioCost,
				  config->dictSamplingFrac,
				  config->queriesSamplingFrac,
				  config->maxStrLen, 
				  ftFanout);
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle compressed indexer UnionGlobalBasic
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerCombineListsBasic<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerCombineListsBasic<StringContainer, InvList>, FtSearcher> > {  

public:
  typedef FtIndexerCombineListsBasic<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }
  
  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer,
				  config->gramGen, 
				  reductionRatio, 
				  config->dictSamplingFrac, 					     
				  config->maxStrLen, 
				  ftFanout);
    
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};


// partial specialization to handle compressed indexer UnionGlobalCost
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerCombineListsCost<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerCombineListsCost<StringContainer, InvList>, FtSearcher> > {  

public:
  typedef FtIndexerCombineListsCost<StringContainer, InvList> indexer;

  StatsGen(StatsGenConfig* config) { this->config = config; }

  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  

    StatsGenConfig* config = this->config;
    this->ftIndexer = new indexer(strContainer,
				  config->gramGen, 
				  reductionRatio, 
				  workload,
				  config->simMetric,
				  config->simThresh,
				  config->dictSamplingFrac, 
				  config->queriesSamplingFrac,
				  config->maxStrLen, 
				  ftFanout);

    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);      
  }
};

// partial specialization to handle disk-based indexer ftindexerondisk
template<class FtSearcher, class StringContainer, class InvList>
class StatsGen<FtIndexerOnDisk<StringContainer, InvList>, FtSearcher> 
  : public StatsGenAbs<StatsGen<FtIndexerOnDisk<StringContainer, InvList>, FtSearcher> > {  

public:
  typedef FtIndexerOnDisk<StringContainer, InvList> indexer;
  
  StatsGen(StatsGenConfig* config) { this->config = config; }
  
  void setIndexerAndSearcher_Impl(StringContainer* strContainer,
				  const float reductionRatio, 
				  const unsigned ftFanout, 
				  const vector<string>* workload,
				  const unsigned runBufferSize) {  
        
    StatsGenConfig* config = this->config;    
    this->ftIndexer = new indexer(strContainer,
				  config->gramGen, 
				  config->disableStreamBuffer,
				  "invlists.ix",
				  runBufferSize,
				  config->maxStrLen, 
				  ftFanout,
				  config->pmf,
				  config->lcCharNum,
				  config->hcBuckets,
				  config->scatteredOrg);
    
    this->ftSearcher = new FtSearcher(this->merger, this->ftIndexer);
  }
};

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
setPartFilters(FtIndexer* ftixer) {
  if(this->config->autoPartFilter) {
    ftixer->autoAddPartFilter();
  }
  else {
    for(unsigned i = 0; i < this->config->filterTypes.size(); i++) {
      ftixer->addPartFilter(this->config->filterTypes.at(i)->clone());
    }
  }
}

template<class StatsGenConcrete>
void
StatsGenAbs<StatsGenConcrete>::
generate() {
  StatsGenConfig* config = this->config;    
  LoopTimer loopTimer;
  int sysret = 0;
  
  typename FtIndexer::IxStatsType ixStats;
  typename FtSearcher::WlStatsType wlStats;
  
  config->wlStatsFile = config->outputFilePrefix + string("_wlstats.txt");
  ofstream fpWlStats;
  if(config->outputFlags & OF_WORKLOADSTATS) {
    fpWlStats.open(config->wlStatsFile.c_str(), ios::out);
    ixStats.printHeader(fpWlStats);
    fpWlStats << ";";
    wlStats.printHeader(fpWlStats);
    fpWlStats << endl;
  }
  
  config->queryResultsFile = config->outputFilePrefix + string("_qresults.txt");
  ofstream fpQueryResults;
  if(config->outputFlags & OF_QUERYRESULTS) fpQueryResults.open(config->queryResultsFile.c_str(), ios::out);
  
  config->queryStatsFile = config->outputFilePrefix + string("_qstats.txt");
  ofstream fpQueryStats;
  bool queryStatsHeaderPrinted = false;
  if(config->outputFlags & OF_QUERYSTATS) fpQueryStats.open(config->queryStatsFile.c_str(), ios::out);
  
  // read dictionary with different sizes, perform batch data generation and write data to file
  for(unsigned dict = config->dictSizeStart; dict <= config->dictSizeStop; dict += config->dictSizeStep) {    
    StringContainer* strContainer = NULL;
    if(config->existingStringContainer.length() > 0) {
      strContainer = openStringContainer<StringContainer>(config);
    }
    else {
      strContainer = createDefaultStringContainer<StringContainer>(config);
      strContainer->initStatsCollector(config->gramGen);
      strContainer->fillContainer(config->dictDataFile.c_str(), dict, config->maxStrLen);
    }
    
    // check to see if workload has been manually overridden
    vector<string> workload;
    if(config->existingWorkloadFile.length() > 0) readString(workload, config->existingWorkloadFile, config->numberQueries, config->maxStrLen);
    else {
      fillWorkload(strContainer, workload);
      if(config->saveWorkloadFile.length() > 0) {
	ofstream osWorkload;
	osWorkload.open(config->saveWorkloadFile.c_str(), ios::out);
	for(unsigned i = 0; i < workload.size(); i++) 
	  osWorkload << workload[i] << endl;
	osWorkload.close();
	cout << "SAVED WORKLOAD TO: " << config->saveWorkloadFile << endl;
      }
    }
    
    for(float reduction = config->reductionStart; reduction <= config->reductionStop; reduction += config->reductionStep) {

      ixStats.reset();
      wlStats.reset();

      for(unsigned runs = 0; runs < config->numberRuns; runs++) {	    
	
	strContainer->flushCache();

	if(runs == 0) {
	  setIndexerAndSearcher(strContainer, reduction, config->fanout, &workload, config->runBuffer);
	  setPartFilters(ftIndexer);
	  if(config->dropCachesBeforeBuildingIndex && ftIndexer->getType() == ONDISK) {
	    sysret = system("sync");
	    sysret = system("echo 3 > /proc/sys/vm/drop_caches");
	  }
	  
	  cout << endl;
	  cout << "---- STARTING EXPERIMENT ----" << endl;
	  cout << "INDEXER:         " << ftIndexer->getName() << endl;
	  cout << "SEARCHER:        " << ftSearcher->getName() << endl;
	  cout << "MERGER:          " << merger->getName() << endl;
	  cout << "DICTIONARY SIZE: " << dict << endl;
	  cout << "FANOUT:          " << config->fanout << endl;
	  cout << "PARTFILTERS:     ";
	  for(unsigned i = 0; i < config->filterTypes.size(); i++) {
	    cout << config->filterTypes.at(i)->getName();
	    if(i+1 == config->filterTypes.size()) cout << endl;
	    else cout << ", ";
	  }
	  cout << "REDUCTION RATIO: " << reduction << endl;
	  cout << "RUNBUFFER SIZE:  " << config->runBuffer << endl;
	  cout.flush();      	  
	  
	  if(config->existingIndexFile.length() > 0) {
	    cout << "LOADING INDEX FROM: " << config->existingIndexFile << endl;
	    ftIndexer->loadIndex(config->existingIndexFile.c_str());
	  }
	  else {
	    cout << "BUILDING INDEX..." << endl;
	    ftIndexer->buildIndex();
	  }
	  ixStats.add(ftIndexer->getIndexStats());
	  ftSearcher->prepare();
	}
	
	// always start queries from cold store
	if(runs == 0 && ftIndexer->getType() == ONDISK) {
	  sysret = system("sync");
	  sysret = system("echo 3 > /proc/sys/vm/drop_caches");
	}
	  
	char message[100];
	sprintf(message, "EXECUTING QUERIES RUN %d/%d", runs+1, config->numberRuns);
	loopTimer.begin(message, workload.size());
	for(unsigned q = 0; q < workload.size(); q++) {	      
	  
	  if(config->dropCachesBeforeEachQuery && ftIndexer->getType() == ONDISK) {
	    sysret = system("sync");
	    sysret = system("echo 3 > /proc/sys/vm/drop_caches");
	  }
	  
	  vector<unsigned> results;
	  Query query(workload.at(q), *(config->simMetric), config->simThresh, config->qt);
	  ftSearcher->search(query, results);
	  QueryStats* queryStats = ftSearcher->getQueryStats();
	  queryStats->qid = q;
	  wlStats.add(queryStats);
	  
	  // only output query results once (they should not change for multiple runs)
	  if( (config->outputFlags & OF_QUERYRESULTS) && (runs == 0) ) {
	    fpQueryResults << q << " " << workload.at(q) << " " << results.size() << endl;
	    sort(results.begin(), results.end());
	    for(unsigned res = 0; res < results.size(); res++) 
	      fpQueryResults << results.at(res) << " ";
	    fpQueryResults << endl;
	  }
	  
	  loopTimer.next();
	}	 	  	  
	loopTimer.end();
	
	// flush the query stats here
	if(config->outputFlags & OF_QUERYSTATS) {
	  if(!queryStatsHeaderPrinted) {
	    wlStats.allQueryStats.at(0).printHeader(fpQueryStats);
	    fpQueryStats << endl;	    
	    queryStatsHeaderPrinted = true;
	  }
	  for(unsigned i = 0; i < wlStats.allQueryStats.size(); i++) {
	    wlStats.allQueryStats.at(i).print(fpQueryStats);
	    fpQueryStats << endl;
	  }
	}
      }
      
      cout << endl;
      
      ixStats.avg();
      wlStats.avg(config->numberQueries, config->numberRuns);
      
      if(config->outputFlags & OF_WORKLOADSTATS) {
	ixStats.print(fpWlStats);
	fpWlStats << ";";
	wlStats.print(fpWlStats);
	fpWlStats << endl;   
      }   
      
      delete ftSearcher;
      delete ftIndexer;
    }

    delete strContainer;
  }
  
  if(config->outputFlags & OF_WORKLOADSTATS) fpWlStats.close();
  if(config->outputFlags & OF_QUERYRESULTS) fpQueryResults.close();
  if(config->outputFlags & OF_QUERYSTATS) fpQueryStats.close();  
}

template<class StatsGenConcrete>
void
StatsGenAbs<StatsGenConcrete>::
fillWorkload(StringContainer* strContainer, vector<string>& workload) {   
  if(config->numberQueries > 0 && config->distinctQueries > 0) {
    switch(config->queriesDistrib) {
    case QD_UNIFORM: fillWorkloadNormal(strContainer, workload); break;
    case QD_ZIPF: fillWorkloadZipf(strContainer, workload); break;
    }
  }
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
fillWorkloadNormal(StringContainer* strContainer, vector<string>& workload) {
  srand(150);
  unsigned workloadSize = config->numberQueries;
  unsigned numberOccurences = workloadSize / config->distinctQueries;
  
  for(unsigned i = 0; i < config->distinctQueries; i++) {    
    unsigned stringIndex = (strContainer->size() < 100000) ? rand() % strContainer->size() : rand() % 100000;
    for(unsigned j = 0; j < numberOccurences; j++) {
      string tmp;
      strContainer->retrieveString(tmp, stringIndex);
      workload.push_back(tmp);    
    }
  }
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
fillWorkloadZipf(StringContainer* strContainer, vector<string>& workload) {
  srand(150);

  unsigned workloadSize = config->numberQueries;  

  vector<unsigned> stringIndexes;

  double valFreqs[config->distinctQueries];
  genZipfDist(config->distinctQueries, config->zipfSkew, valFreqs);

  for(unsigned i = 0; i < config->distinctQueries; i++) {
    unsigned numberOccurences = (unsigned) floor( (double)workloadSize * valFreqs[i] );
    unsigned stringIndex = (strContainer->size() < 100000) ? rand() % strContainer->size() : rand() % 100000;
    stringIndexes.push_back(stringIndex);    
    for(unsigned j = 0; j < numberOccurences; j++) {
      string tmp;
      strContainer->retrieveString(tmp, stringIndex);
      workload.push_back(tmp);    
    }
  }

  unsigned remaining = workloadSize - workload.size();
  unsigned next = 0;
  for(unsigned i = 0; i < remaining; i++) {
    string tmp;
    strContainer->retrieveString(tmp, stringIndexes.at(next));
    workload.push_back(tmp);
    next++;
    if(next >= stringIndexes.size()) next = 0;
  }
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
generateWorkloads(unsigned distinctQueries, unsigned workloadSize) {
  cout << "GENERATING WORKLOADS" << endl;
  vector<string> dictionary;
  readString(dictionary, config->dictDataFile, 1000000, 250);	

  cout << "SAME ZIPF" << endl;
  genWloadSameZipf(dictionary, distinctQueries, workloadSize);
  cout << "DIFF ZIPF" << endl;
  genWloadDiffZipf(dictionary, distinctQueries, workloadSize); 
  cout << "SAME NORMAL" << endl;
  genWloadSameNormal(dictionary, distinctQueries, workloadSize); 
  cout << "DIFF NORMAL" << endl;
  genWloadDiffNormal(dictionary, distinctQueries, workloadSize); 

  cout << "ALL DONE!" << endl;
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
genWloadDiffZipf(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize) {
  srand(150);

  vector<string> workload;
  vector<unsigned> stringIndexes;

  double valFreqs[config->distinctQueries];
  genZipfDist(config->distinctQueries, config->zipfSkew, valFreqs);

  for(unsigned i = 0; i < config->distinctQueries; i++) {
    unsigned numberOccurences = (unsigned) floor( (double)workloadSize * valFreqs[i] );
    unsigned stringIndex = rand() % 100000 + 100000;
    stringIndexes.push_back(stringIndex);    
    for(unsigned j = 0; j < numberOccurences; j++)
      workload.push_back(dictionary.at(stringIndex));    
  }

  unsigned remaining = workloadSize - workload.size();
  unsigned next = 0;
  for(unsigned i = 0; i < remaining; i++) {
    workload.push_back(dictionary.at(stringIndexes.at(next)));
    next++;
    if(next >= stringIndexes.size()) next = 0;
  }

  ofstream fpOut;
  fpOut.open("queries_diff_zipf.txt", ios::out);
  for(unsigned i = 0; i < workload.size(); i++)
    fpOut << workload.at(i) << endl;

  fpOut.close();
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
genWloadSameZipf(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize) {
  srand(150);
  
  vector<string> workload;
  vector<unsigned> stringIndexes;

  double valFreqs[config->distinctQueries];
  genZipfDist(config->distinctQueries, config->zipfSkew, valFreqs);

  for(unsigned i = 0; i < config->distinctQueries; i++) {
    unsigned numberOccurences = (unsigned) floor( (double)workloadSize * valFreqs[i] );
    unsigned stringIndex = rand() % 100000;
    stringIndexes.push_back(stringIndex);    
    for(unsigned j = 0; j < numberOccurences; j++)
      workload.push_back(dictionary.at(stringIndex));    
  }

  unsigned remaining = workloadSize - workload.size();
  unsigned next = 0;
  for(unsigned i = 0; i < remaining; i++) {
    workload.push_back(dictionary.at(stringIndexes.at(next)));
    next++;
    if(next >= stringIndexes.size()) next = 0;
  }

  ofstream fpOut;
  fpOut.open("queries_same_zipf.txt", ios::out);
  for(unsigned i = 0; i < workload.size(); i++)
    fpOut << workload.at(i) << endl;

  fpOut.close();
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
genWloadSameNormal(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize) {
  srand(150);

  vector<string> workload;
  unsigned numberOccurences = workloadSize / distinctQueries;
  
  for(unsigned i = 0; i < distinctQueries; i++) {    
    unsigned stringIndex = rand() % 100000;
    for(unsigned j = 0; j < numberOccurences; j++)
      workload.push_back(dictionary.at(stringIndex));    
  }

  ofstream fpOut;
  fpOut.open("queries_same_normal.txt", ios::out);
  for(unsigned i = 0; i < workload.size(); i++)
    fpOut << workload.at(i) << endl;

  fpOut.close();
}

template<class StatsGenConcrete>
void 
StatsGenAbs<StatsGenConcrete>::
genWloadDiffNormal(vector<string>& dictionary, unsigned distinctQueries, unsigned workloadSize) {
  srand(150);

  vector<string> workload;
  unsigned numberOccurences = workloadSize / distinctQueries;
  
  for(unsigned i = 0; i < distinctQueries; i++) {    
    unsigned stringIndex = rand() % 100000 + 100000; 
    for(unsigned j = 0; j < numberOccurences; j++)
      workload.push_back(dictionary.at(stringIndex));    
  }

  ofstream fpOut;
  fpOut.open("queries_diff_normal.txt", ios::out);
  for(unsigned i = 0; i < workload.size(); i++)
    fpOut << workload.at(i) << endl;

  fpOut.close();
}

#endif
