/*
  $Id: lbaktree.h 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 08/19/2010
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include <sys/time.h>
#include "kwdsstorage.h"
#include "rstartree/rstartree.h"
#include "rstartree/util.h"
#include "filtertree/src/wrappers/wrappers.h"


enum AlgorithmType {fl, vl, vlf};

class NodePriority
{
public:
    uintptr_t id;
    double priority;
    double pFuzzyTimeCost, cFuzzyTimeCost, pFuzzySpaceCost, cFuzzySpaceCost;
    vector <uintptr_t> ancestorsIds;

    bool operator<(const NodePriority &n) const
    {
        return priority < n.priority;
    };
};

class LBAKTree: public RTree
{
private:
    GramGenFixedLen *gramGen;
    KeywordsFile kf1, kf2;
    double spaceBudget, avgKwdsLength, sx, sy, sx2, sy2, sxy, n, gradient, intercept;
    float simThreshold, kfThreshold;
    unsigned numKwds, q, indexesLevel;
    string queryWorkloadFile;
    AlgorithmType algoType;
    vector<string> dictionary;
    unordered_map <string, unsigned> keywordsMap;
    unordered_map <uintptr_t, unsigned> queryWorkloadMap;
    unordered_map <uintptr_t, vector<unsigned> *> recordsMap;
    unordered_map <uintptr_t, Array<unsigned> *> keywordsHashesMap;
    unordered_map <uintptr_t, StringContainerVector *> strContainersMap;
    unordered_map <uintptr_t, WrapperSimpleEdNorm *> wrappersMap;
    void init(AlgorithmType type, float simT);
    void readQueryWorkload(const Rectangle &range);
    void readQueryWorkload(const Rectangle &range, uintptr_t objectId);
    void propagateKeywords(uintptr_t objectId);
    void insertKeywords(uintptr_t objectId, unordered_set<string> &kwds, bool leaf);
    void parseKeywords(string &text, unordered_set<string> &kwds);
    void selectSANodes();
    void computeGradientIntercept(uintptr_t objectId, vector <string> &keywords);
    void useFL();
    void useVL();
    void useVLF();
    void fillKeywordsIntersectionsFile();
    void fillKeywordsHashesMap();
    void fillWrappersMap();
    void rangeQuery(vector<Object> &objects, const Rectangle &range, uintptr_t id, const vector <string> &kwds, vector<string> strings[], vector<unsigned> hashes[]);
    bool searchWrapper(uintptr_t objectId, const vector <string> &kwds, vector<string>strings[], vector<unsigned>hashes[], vector<string>resultStrings[], vector<unsigned>resultHashes[]);
    bool searchArray(uintptr_t objectId, unsigned numKeywords, vector<string> strings[], vector<unsigned> hashes[], vector<string> resultStrings[], vector<unsigned> resultHashes[]);
    bool searchVector(uintptr_t objectId, const string &keyword);
public:
    LBAKTree(Storage *storage, AlgorithmType type, unsigned il, float simT);
    LBAKTree(Storage *storage, string &file, AlgorithmType type, double sb, float simT);
    LBAKTree(Storage *storage, string &file, AlgorithmType type, double sb, float simT, float kfT);
    ~LBAKTree();
    void insert(const Object &obj, vector <string> &kwds);
    void buildIndex();
    void rangeQuery(vector<Object> &objects, const Rectangle &range, const vector <string> &kwds);
    void getObjectKeywords(uintptr_t objectId, vector<string> &objectKeywords);
    void startTimeMeasurement(struct timeval &t1, struct timezone &tz);
    void stopTimeMeasurement(struct timeval &t2, struct timezone &tz);
    double getTimeMeasurement(struct timeval &t1, struct timeval &t2);
};

