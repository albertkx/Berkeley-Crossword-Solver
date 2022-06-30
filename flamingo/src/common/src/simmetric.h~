/*
  $Id: simmetric.h 6084 2011-05-07 18:18:30Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD license.
  
  Some of the code in this file is part of compression techniques.
  Such parts are appropriately marked.
  Redistribution of the compression techniques is permitted under 
  the terms of the Academic BSD License.

  Date: 04/15/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>, 
  Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef ED_MATRIX_DIM
#define ED_MATRIX_DIM 500
#endif

#ifndef _simmetric_h_
#define _simmetric_h_

#include "typedef.h"
#include "gramgen.h"
#include "compressionargs.h"
#include "filtertypes.h"
#include <cmath>
#include <algorithm>

// these enums are used in searcher, e.g. ftsearcherabs
enum SimMetricType
{
  SMT_EDIT_DISTANCE,
  SMT_EDIT_DISTANCE_NORM,
  SMT_EDIT_DISTANCE_SWAP,
  SMT_JACCARD,
  SMT_COSINE,
  SMT_DICE,
  SMT_JACCARD_BAG,
  SMT_COSINE_BAG,
  SMT_DICE_BAG,
  SMT_GRAMCOUNT
};

class SimMetric                 // abstract class
{
public:
  const GramGen& gramGen;
  const std::string name;
  const SimMetricType smt;
    
  SimMetric(const GramGen &gramGen, const std::string name, SimMetricType smt): 
    gramGen(gramGen), name(name), smt(smt)
  {}
    
  virtual ~SimMetric() 
  {};
  
  virtual bool simBatch(const std::string& candi, const std::string& query, float simThreshold);
  
  virtual float simBatch(const std::string& candi, const std::string& query) { return 0.0f; }

  virtual bool lcFilter(const std::string& query, const std::string& candi, float simThreshold) { return false; }
  
  virtual float operator()(const std::string &s1, const std::string &s2)
    const = 0;

  virtual bool operator()(
    const std::string &s1, 
    const std::string &s2, 
    float threshold)
    const;

  // compute merging threshold
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const = 0;

  // functions used within holes compression (sampling techniques)
  // used for quickly getting threshold if number of holes is known
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const = 0;
  
  // get filter bounds  
  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const = 0;  

  // bounds on similarity
  virtual float getSimMin(
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const = 0;
 
  virtual float getSimMax(
    uint lenQuery, 
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const = 0;
  
  // bound on common grams
  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const = 0;

  // bound on common grams for multiple keyword search 
  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const = 0;

  // for answering topk queries by multiple range searches
  virtual float topkInitialRange() const = 0;
  virtual float topkNextRange(float lastRange, const std::string& queryStr) const = 0;

  // for set-based simmetrics
  // set the minimum string length in indexed dataset for computing lower bound T
  // set queryGrams for better cache behavior in simBatch
  virtual void prepare(uint minStrLen, vector<uint>& queryGrams) {}
};

class SimMetricEd: public SimMetric 
{
protected:
  // preallocated dp matrix to avoid repeated allocation and get more cache hits
  // the method of allocation MATTERS in practice
  // I've found this one to be the faster than other common 2d array allocation methods
  uint __restrict__ (*matrix)[ED_MATRIX_DIM+1];

  // for letter counting optimization
  uint __restrict__ tLetterCounter[256];
  uint __restrict__ qLetterCounter[256];
  uint lcStart;
  uint lcEnd;
  
  SimMetricEd(const GramGen &gramGen, const std::string name, SimMetricType smt): 
    SimMetric(gramGen, name, smt), matrix(new uint[ED_MATRIX_DIM+1][ED_MATRIX_DIM+1])
  {
    lcStart = (uint)(unsigned char)'a';
    lcEnd = (uint)(unsigned char)'a' + 26;    
  }
  
public:
  SimMetricEd(const GramGen &gramGen, const std::string name = "ed"): 
    SimMetric(gramGen, name, SMT_EDIT_DISTANCE), matrix(new uint[ED_MATRIX_DIM+1][ED_MATRIX_DIM+1])
    {
      lcStart = (uint)(unsigned char)'a';
      lcEnd = (uint)(unsigned char)'a' + 26;
    } 
  
  virtual bool lcFilter(const std::string& query, const std::string& candi, float simThreshold);
  
  virtual bool simBatch(const std::string&__restrict__ candi, const std::string&__restrict__ query, float simThreshold);
  
  virtual float simBatch(const std::string& candi, const std::string& query);

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual bool operator()(
    const std::string &s1, 
    const std::string &s2, 
    float threshold)
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;  
  
  virtual float getSimMin(
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const;

  virtual float getSimMax(
    uint lenQuery, 
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  // for answering topk queries by multiple range searches
  virtual float topkInitialRange() const { return 0; }
  virtual float topkNextRange(float lastRange, const std::string& queryStr) const { 
    float lenBoost = (float)queryStr.length() / 10.0f;
    return lastRange + max( (unsigned)floor(sqrt(lastRange+lenBoost)), (unsigned)1);
  }
  
  ~SimMetricEd() {
    if(matrix) delete[] matrix;
  }
};

class SimMetricEdNorm: public SimMetricEd
{
public:
  SimMetricEdNorm(const GramGen &gramGen, const std::string name = "ednorm"): 
    SimMetricEd(gramGen, name, SMT_EDIT_DISTANCE_NORM)
  {}

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual bool operator()(
    const std::string &s1, 
    const std::string &s2, 
    float threshold)
    const;

  virtual bool simBatch(const std:: string& candi, const std::string& query, float simThreshold);

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;  
  
  virtual float getSimMin(
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const;

  virtual float getSimMax(
    uint lenQuery, 
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
  
  // for answering topk queries by multiple range searches
  virtual float topkInitialRange() const { return 1.0f; }
  virtual float topkNextRange(float lastRange, const std::string& queryStr) const { return lastRange - 0.1f; }
};

class SimMetricEdSwap: public SimMetricEd
{
public:
  SimMetricEdSwap(const GramGen &gramGen, const std::string name = "edswap"): 
    SimMetricEd(gramGen, name, SMT_EDIT_DISTANCE_SWAP)
  {}

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;
 
  virtual bool simBatch(const std::string&__restrict__ candi, const std::string&__restrict__ query, float simThreshold);

  // compression not implemented
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;  
};

class SimMetricGram: public SimMetric // abstract class
{
protected:
  uint minStrLen;
  
  // reuse memory for better cache behavior when doing many similarity computations
  vector<uint> queryGrams;  
  vector<uint> candiGrams;
  
public:
  SimMetricGram(const GramGen &gramGen, const std::string name, SimMetricType smt): 
    SimMetric(gramGen, name, smt) 
  {}

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const = 0;

  virtual bool operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon, 
    float threshold)
    const 
  { return operator()(noGramsData, noGramsQuery, noGramsCommon) >= threshold; }
  
  virtual float getSimMin(
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const 
  { return operator()(noGramsData, noGramsQuery, noGramsCommon); }

  virtual float getSimMax(
    uint lenQuery, 
    uint noGramsQuery, 
    uint noGramsData, 
    uint noGramsCommon) 
    const
  { return operator()(noGramsData, noGramsQuery, noGramsCommon); }
  
  virtual void prepare(uint minStrLen, vector<uint>& queryGrams) { 
    this->minStrLen = minStrLen;
    this->queryGrams.clear();
    this->queryGrams = queryGrams; // copy the query grams into local container
    sort(this->queryGrams.begin(), this->queryGrams.end()); // sort for set operations (union, intersect)
  }
  
  // for answering topk queries by multiple range searches
  virtual float topkInitialRange() const { return 1.0f; }
  virtual float topkNextRange(float lastRange, const std::string& queryStr) const { return lastRange - 0.1f; }
};

class SimMetricJacc: public SimMetricGram // set semantics !
{
public:
  SimMetricJacc(const GramGen &gramGen, const std::string name = "jc"): 
    SimMetricGram(gramGen, name, SMT_JACCARD)
  {}
  
  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;
  
  // compute merging threshold
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const; 

  // functions used within holes compression (sampling techniques)
  // used for quickly getting threshold if number of holes is known
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  // get filter bounds  
  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};

class SimMetricCos: public SimMetricGram // set semantics !
{
public:
  SimMetricCos(const GramGen &gramGen, const std::string name = "cos"): 
    SimMetricGram(gramGen, name, SMT_COSINE)
  {}

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;

  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const;
  
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};

class SimMetricDice: public SimMetricGram // set semantics !
{
public:
  SimMetricDice(const GramGen &gramGen, const std::string name = "dice"): 
    SimMetricGram(gramGen, name, SMT_DICE)
  {}

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;
  
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL)
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};

class SimMetricGramCount: public SimMetricGram 
{
public:
  SimMetricGramCount(const GramGen &gramGen, const std::string name = "cnt"): 
    SimMetricGram(gramGen, name, SMT_GRAMCOUNT)
  {}
 
  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;

  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL)
    const;

  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};


// only works when gramgen does NOT use pre- and postfixing
class SimMetricJaccBag: public SimMetricGram // bag semantics !
{
private:
  vector<uint> setUnion; // actually bag union, name chosen to be consistent with stl set_union
  
public:
  SimMetricJaccBag(const GramGen &gramGen, const std::string name = "jcb"): 
    SimMetricGram(gramGen, name, SMT_JACCARD_BAG)
  {}

  virtual bool simBatch(const std::string& candi, const std::string& query, float simThreshold);

  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;
  
  // compute merging threshold
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const; 

  // functions used within holes compression (sampling techniques)
  // used for quickly getting threshold if number of holes is known
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  // get filter bounds  
  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};

// only works when gramgen does NOT use pre- and postfixing
class SimMetricCosBag: public SimMetricGram // bag semantics !
{
private:
  // used within simBatch for better cache behavior when doing many similarity computations
  vector<uint> intersection;
  
public:
  SimMetricCosBag(const GramGen &gramGen, const std::string name = "cosb"): 
    SimMetricGram(gramGen, name, SMT_COSINE_BAG)
  {}

  virtual bool simBatch(const std::string& candi, const std::string& query, float simThreshold);
  
  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;
  
  // compute merging threshold
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const; 

  // functions used within holes compression (sampling techniques)
  // used for quickly getting threshold if number of holes is known
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  // get filter bounds  
  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;
  
  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};


// only works when gramgen does NOT use pre- and postfixing
class SimMetricDiceBag: public SimMetricGram // bag semantics !
{
private:
  vector<uint> intersection;

public:
  SimMetricDiceBag(const GramGen &gramGen, const std::string name = "diceb"): 
    SimMetricGram(gramGen, name, SMT_DICE_BAG)
  {}
  
  virtual bool simBatch(const std::string& candi, const std::string& query, float simThreshold);
  
  virtual float operator()(const std::string &s1, const std::string &s2)
    const;

  virtual float operator()(
    uint noGramsData, 
    uint noGramsQuery, 
    uint noGramsCommon) 
    const;
  
  // compute merging threshold
  virtual uint getMergeThreshold(
    const std::string& query, 
    const std::vector<uint>& queryGramCodes,
    const float simThreshold,
    const CompressionArgs* compressArgs = NULL) 
    const; 

  // functions used within holes compression (sampling techniques)
  // used for quickly getting threshold if number of holes is known
  // ATTENTION: part of compression techniques
  virtual uint getMergeThreshold(
    const std::string& query,
    const float simThreshold,
    const uint numberHoles)
    const;

  // get filter bounds  
  virtual void getFilterBounds(
    const std::string& query,
    const float simThreshold,
    const AbstractFilter* filter,
    uint& lbound,
    uint& ubound)
    const;

  virtual uint getNoGramsMin(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;

  virtual uint getNoGramsMax(
    uint lenQuery, 
    uint noGramsMin, 
    uint noGramsQuery, 
    float sim)
    const;
};

#endif
