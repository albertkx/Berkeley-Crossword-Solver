/*
  $Id: topksearch.h 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 11/21/2009
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _topksearch_h_
#define _topksearch_h_

#include "topkheap.h"

namespace Topk
{
  class KwdInfo {
  public:
    std::string kwd;
    int count;
    float weight;

    KwdInfo(std::string kwd, int count): kwd(kwd), count(count) {}

    bool operator==(const KwdInfo &k)
    {
      return count == k.count && weight == k.weight && kwd == k.kwd;
    }

    bool operator!= (const KwdInfo &k)
    {
      return !(*this == k);
    }

    static std::string* getKwdPtr(KwdInfo &k) { return &k.kwd; }
    static int* getCountPtr(KwdInfo &k) { return &k.count; }
    static float* getWeightPtr(KwdInfo &k) { return &k.weight; }

    static bool higherWeight(const KwdInfo *a, const KwdInfo *b) 
    {
      return a->weight > b->weight;
    }

    friend std::ostream& operator<<(std::ostream& output, const KwdInfo& k);
  };

  template<typename T> class KwdInfoIterator
    : public std::iterator<std::random_access_iterator_tag, T>
  {
  protected:
    std::vector<KwdInfo*>::const_iterator kwd;
    T*(*f)(KwdInfo&);

  public:
    KwdInfoIterator(
      std::vector<KwdInfo*>::const_iterator kwd, T*(*f)(KwdInfo&)):
      kwd(kwd), f(f) {}

    bool operator==(const KwdInfoIterator &i)
    {
      return kwd == i.kwd;
    }

    bool operator!=(const KwdInfoIterator &i)
    {
      return kwd != i.kwd;
    }

    bool operator<(const KwdInfoIterator &i)
    {
      return kwd < i.kwd;
    }

    KwdInfoIterator& operator=(const KwdInfoIterator& i) 
    {
      kwd = i.kwd;
      return *this;
    }

    KwdInfoIterator& operator++() 
    {
      ++kwd;
      return *this;
    }

    KwdInfoIterator operator++(int) 
    {
      KwdInfoIterator tmp(kwd);
      ++(*this);
      return tmp;
    }

    KwdInfoIterator operator+(ptrdiff_t i) 
    {
      return KwdInfoIterator(kwd + i, f);
    }

    ptrdiff_t operator-(const KwdInfoIterator &k) 
    {
      return kwd - k.kwd;
    }

    T& operator*()
    {
      return *f(**kwd);
    }

    T* operator->()
    {
      return f(**kwd);
    }

    T& operator[](ptrdiff_t i) const 
    {
      return *f(**(kwd + i));
    }
  };

  class Search
  {
  private:
    static const uint k;
    static const float thr;

    std::vector<KwdInfo*> kwds;
    std::tr1::unordered_map<std::string, KwdInfo*> kwdToKwdInfo;

    GramGenFixedLen gramGen;
    SimMetricEdNorm simMetric;
    Index topkIndex;

    std::ostream &out;

    uint *ids;

  public:

    Search(std::ostream &out = std::cerr): 
      gramGen(GramGenFixedLen(2)), 
      simMetric(SimMetricEdNorm(gramGen)), 
      out(out)
    {
      ids=new uint[k];
    }

    ~Search()
    {
      delete [] ids;
      free();
    }

    int searchKwd(
      const std::vector<std::string> &originalKwds,
      std::vector<std::string> &relaxedKwds);
    /*
      e.g.,
      originalKwd = ["A", "B"]
      relaxedKwd = ["A1", "A2", "", "B1"]
    */

    template<class InputIterator, class OutputIterator> 
    int searchKwd(
      InputIterator originalKwdBegin, 
      InputIterator originalKwdEnd, 
      OutputIterator relaxedKwdBegin);

    int updateKwd(
      const std::vector<std::string> &kwds, const std::vector<int> &counts);
    /*
      e.g.,
      kwd    = ["A", "D", "E"]
      counts = [ 2 ,  4 , -1 ]
    */

    template<class InputIteratorString, class InputIteratorInt>
    int updateKwd(
      InputIteratorString kwdsBegin, 
      InputIteratorString kwdsEnd, 
      InputIteratorInt countsBegin);

    void save(const std::string &filename) const;
    void save(std::ofstream &file) const;
    void load(const std::string &filename);
    void load(std::ifstream &file);

    KwdInfoIterator<std::string> kwdsBegin()
    {
      return KwdInfoIterator<std::string>(kwds.begin(), KwdInfo::getKwdPtr);
    }

    KwdInfoIterator<std::string> kwdsEnd()
    {
      return KwdInfoIterator<std::string>(kwds.end(), KwdInfo::getKwdPtr);
    }

    KwdInfoIterator<int> countsBegin()
    {
      return KwdInfoIterator<int>(kwds.begin(), KwdInfo::getCountPtr);
    }

    KwdInfoIterator<int> countsEnd()
    {
      return KwdInfoIterator<int>(kwds.end(), KwdInfo::getCountPtr);
    }

    KwdInfoIterator<float> weightsBegin()
    {
      return KwdInfoIterator<float>(kwds.begin(), KwdInfo::getWeightPtr);
    }

    KwdInfoIterator<float> weightsEnd()
    {
      return KwdInfoIterator<float>(kwds.end(), KwdInfo::getWeightPtr);
    }

    static float weight(int count, int countMin)
    {
      return count <= 0 ? 0 : static_cast<float>(countMin) / count / 10;
    }

    static bool lessIgnoreLessOne(int a, int b) 
    {
      if ((a < 1 && b < 1) || (a >= 1 && b >= 1)) {
          return a < b;
      }
      if (a < 1) { // b >= 1
        return false;
      }
      // a >= 1,  b < 1
      return true;
    }

  private:
    void updateWeights();
    void free();

    friend std::ostream& operator<<(std::ostream& output, const Search& s);
    friend bool operator==(const Search& s1, const Search& s2);
  };

  inline int Search::updateKwd(
    const std::vector<std::string> &kwds, const std::vector<int> &counts) {
    return updateKwd(kwds.begin(), kwds.end(), counts.begin());
  }

  template<class InputIteratorString, class InputIteratorInt>
  int Search::updateKwd(
    InputIteratorString kwdsBegin, 
    InputIteratorString kwdsEnd, 
    InputIteratorInt countsBegin)
  {
    out << "== = updateKwd = ==" << std::endl;

    for (; kwdsBegin != kwdsEnd; ++kwdsBegin, ++countsBegin) {
      if (kwdsBegin->empty()) {
        continue;
      }

      std::string kwd(*kwdsBegin);
      std::transform(
        kwdsBegin->begin(), kwdsBegin->end(), kwd.begin(), ::tolower);

      out << kwd << "\t" << *countsBegin << std::endl;
      
      std::tr1::unordered_map<std::string, KwdInfo*>::iterator 
        kwdToKwdInfoPair = kwdToKwdInfo.find(kwd);
      if (kwdToKwdInfoPair == kwdToKwdInfo.end()) {
        KwdInfo *kwdInfo = new KwdInfo(kwd, *countsBegin);
        kwds.push_back(kwdInfo);
        kwdToKwdInfo.insert(make_pair(kwd, kwdInfo));
      }
      else {
        KwdInfo *kwdInfo = kwdToKwdInfoPair->second;
        kwdInfo->count += *countsBegin;
      }
    }

    //// TODO prune elements with count 0
    // count(counts.begin(), counts.end(), bind2nd(less_equal<int>(), 0));

    updateWeights(); 

    std::sort(kwds.begin(), kwds.end(), KwdInfo::higherWeight);
    
    topkIndex.build(this->kwdsBegin(), this->kwdsEnd(), gramGen);

    out << "== == ==" << std::endl;

    return 0;
  }

  inline int Search::searchKwd(
    const std::vector<std::string> &originalKwds,
    std::vector<std::string> &relaxedKwds)
  {
    return searchKwd(
      originalKwds.begin(), 
      originalKwds.end(), 
      std::back_inserter(relaxedKwds));
  }

  template<class InputIterator, class OutputIterator> 
  int Search::searchKwd(
    InputIterator originalKwdBegin, 
    InputIterator originalKwdEnd, 
    OutputIterator relaxedKwdBegin)
  {    
    out << "== = searchKwd = ==" << std::endl;

    // std::string q = "thank";
    // for (std::vector<KwdInfo*>::const_iterator kwd = kwds.begin();
    //      kwd != kwds.end();
    //      ++kwd) {
    //   if ((**kwd).kwd.find(q) != std::string::npos) {
    //     std::cout << **kwd << ", " << simMetric(q, (**kwd).kwd) << std::endl;
    //   }
    // }      

    bool first = true;
    for (;originalKwdBegin != originalKwdEnd; ++originalKwdBegin) {
      out << *originalKwdBegin << std::endl;
      
      if (first) {
        first = false;
      } else {
        *relaxedKwdBegin++= "";
      }

      if (originalKwdBegin->empty()) {
        continue;
      }

      Query query(*originalKwdBegin, simMetric, thr, k, QueryRanking);
      IndexQuery topkIndexQuery(topkIndex, query);
      Heap::getTopk(
        kwdsBegin(), 
        weightsBegin(), 
        topkIndex, 
        query, 
        topkIndexQuery, 
        ids);

      out << "\t";
      for (
        uint* id = ids; 
        id != ids + k && *id != std::numeric_limits<uint>::max(); 
        ++id) {
        out << kwds[*id]->kwd << " ";
        *relaxedKwdBegin++= kwds[*id]->kwd;
      }
      out << std::endl;
    }

    out << "== == ==" << std::endl;

    return 0;
  }
}

#endif
