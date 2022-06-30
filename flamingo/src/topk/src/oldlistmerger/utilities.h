/*  
    $Id: utilities.h 5716 2010-09-09 04:27:56Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of 
    the BSD license.

    Author: Jiaheng Lu 
    Date: 05/11/2007

*/


#ifndef _utilities_h_
#define _utilities_h_

#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <functional>
#include <iterator>
#include <list>
#include <algorithm>

#include "util/src/array.h"

using namespace std;

void dataProcessing();

void tradeOffResearchInHybrid(const vector<Array<unsigned>*> &arrays,
			      const unsigned threshold,
			      vector<unsigned> & results,
			      unsigned & totalMergeTime,
			      unsigned & totalSearchTime);

unsigned max(unsigned s1, unsigned s2);

void  researchMergSkip(const vector<Array<unsigned>*> &arrays,
		       const unsigned threshold,
		       vector<unsigned> &results,
		       unsigned &elementsScanned);

void  researchMergOpt(const vector<Array<unsigned>*> &arrays,
		      const unsigned threshold,
		      vector<unsigned> &results,
		      unsigned &elementsScanned);

void  researchHybridMerger(const vector<Array<unsigned>*> &arrays,
			   const unsigned threshold,
			   vector<unsigned> &results,
			   unsigned &elementsScanned);

void  researchScanCount(const vector<Array<unsigned>*> &arrays,
			const unsigned threshold,
			vector<unsigned> &results,
			unsigned &elementsScanned);


void  CountSkipNodes (const vector<Array<unsigned>* > &lists,
		      unsigned vectorIndexContainer[],
		      unsigned containerSize, 
		      unsigned pivotData,
		      unsigned pointersIndexList[],
		      unsigned &elementScanned);


void analyzeResults(vector<unsigned> &result1,
		    vector<unsigned> &result2,
		    const vector<Array<unsigned>*> &lists);

unsigned  shorestStringSize(const vector<string> &strings);

void searchOptimalParameterInHybrid(const vector<Array<unsigned>*> &arrays,
				    const unsigned threshold,
				    vector<unsigned> &results);

void  binaryOptimalSearch(const vector<Array<unsigned>*> &arrays,
			     const unsigned threshold);

bool binarySearch(vector<unsigned> *v,
		  unsigned value,
		  unsigned start,
		  unsigned end);

bool testConsistent(const vector<unsigned> &result1,
		    const vector<unsigned> &result2);

void analyzeScanCount(const vector<Array<unsigned>*> &arrays,
		      const unsigned threshold,
		      vector<unsigned> &results,
		      unsigned &time);


void separateTwoSets(vector<Array<unsigned>*>
				*longLists,
		     vector<Array<unsigned>*>
				*shortLists,
		     unsigned threshold,
		     const vector<Array<unsigned>*>
		     &originalLists,
		     unsigned sortedIndex[]
		     );

unsigned  getTotalSize(const vector<Array<unsigned>*> &lists);

void sortBySizeOfLists(const vector<Array<unsigned>*> &allLists,
		       unsigned sortedIndex [] );

// ADDED BY ALEX - needed for duplicate detection
void sortByArrayAddress(const vector<Array<unsigned>*> &allLists,
			unsigned sortedIndex [] );

void  binarySearchSet(unsigned &count,
		      vector<Array<unsigned>* >
		      *lists,
		      unsigned data);

void  insertToHeaps(unsigned dataHeap[],
		    unsigned indexHeap[],
		    unsigned &heapSize,
		    const vector< Array<unsigned>* > &lists,
		    unsigned  pointersIndexList[],			  
		    unsigned  vectorIndexContainer[],
		    unsigned  containerSize);


void  skipNodes (const vector<Array<unsigned>* > &lists,
		 unsigned vectorIndexContainer[],
		 unsigned containerSize, 
		 unsigned pivotData,
		 unsigned pointersIndexList[]);




void  mergeSkipShortLists(const vector<Array<unsigned>*> &arrays,
			  const unsigned threshold,
			  vector<unsigned> &results,
			  vector<unsigned> &counters);

void mergeSkipShortListsWithDuplicate(const vector<Array<unsigned>*> &arrays,
				      const vector<unsigned> &weights,
				      const unsigned threshold,
				      vector<unsigned> &results,
				      vector<unsigned> &counters);
 
unsigned hamming(const string &s1, 
		 const string &s2,
		 unsigned T);

void  splitTwoSets(vector<Array<unsigned>*>
		   *longLists,
		   vector<Array<unsigned>*>
		   *shortLists,
		   const unsigned threshold,
		   const vector<Array<unsigned>*>
		   &originalLists,
		   unsigned sortedIndex[],
		   unsigned longListsSize);


void  splitTwoSizeSets(vector<unsigned> *longLists,
		       vector<unsigned> *shortLists,
		       const unsigned threshold,
		       const vector<unsigned> &originalLists,
		       unsigned sortedIndex[],
		       unsigned longListsSize);

void sortBySize(const vector<unsigned> &allLists,
		unsigned sortedIndex []);

void  splitTwoSetsWithDuplicates(vector<Array<unsigned>*> &longLists,
				 vector<Array<unsigned>*> &shortLists,
				 vector<unsigned> &longListsWeights,
				 vector<unsigned> &shortListsWeights,
				 const vector<Array<unsigned>*> &originalLists,
				 const vector<unsigned> &originalWeights,
				 const unsigned shortListsSize);


void  getStatistics(const vector<Array<unsigned>*> &arrays,
		    unsigned threshold,
		    unsigned longListsSize,
		    const vector<unsigned> &partialResults,
		    const vector<unsigned> &results);

void detectDuplicateLists(const vector<Array<unsigned>*> &arrays,
			  vector<Array<unsigned>*> &newArrays,
			  vector<unsigned> &newWeights);

#endif
