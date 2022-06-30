/*
 $Id: example.cc 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 08/19/2010
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include <string>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include "lbaktree.h"

bool sortCondition(const string &a, const string &b)
{
    return a.size() > b.size();
}

bool parseLine(string &line, Rectangle &mbr, vector<string> &kwds)
{
    string coordinates = line.substr(0, line.find(","));
    string keywords = line.substr(line.find(",") + 1, line.length());
    istringstream coordinatesStream(coordinates);
    istringstream keywordsStream(keywords);
    string keyword;
    while(keywordsStream >> keyword)
    {
        transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
        kwds.push_back(keyword);
    }
    float coord;
    vector<float> coords;
    while(coordinatesStream >> coord)
    {
        coords.push_back(coord);
    }
    if(coords.size() == 2)
    {
        mbr.min.x = coords[0];
        mbr.min.y = coords[1];
        mbr.max.x = mbr.min.x;
        mbr.max.y = mbr.min.y;
    }
    else
    {
        mbr.min.x = coords[0];
        mbr.min.y = coords[1];
        mbr.max.x = coords[2];
        mbr.max.y = coords[3];
    }
    if(kwds.empty())
    {
        return false;
    }
    return true;
}

void printResults(LBAKTree &lbaktree, vector<Object> &objects)
{
    for(unsigned i = 0; i < objects.size(); ++i)
    {
        cout << objects[i].id;
        vector<string> objectKeywords;
        lbaktree.getObjectKeywords(objects[i].id, objectKeywords);
        for(unsigned j = 0; j < objectKeywords.size(); ++j)
        {
            cout << ", " << objectKeywords[j];
        }
        cout << endl;
    }
}

int main(int argc, char **argv)
{
    string datasetFile = "../data/data.txt";
    string queryWorkloadFile = "../data/queries.txt";

    // this is an enum, can be "fl", "vl", or "vlf"
    AlgorithmType algoType = vlf;

    // space budget in bytes for the approximate indexes, this parameter is only for "vl" and "vlf" algorithms
    double spaceBudget = 500000000.0;

    // can be between 0 and 1
    float similarityThreshold = 0.8f;

    // can be between 0 and 1
    double keywordFrequencyThreshold = 1.0;

    // the approximate indexes level on the tree, this parameter is only for the "fl" algorithm
    //unsigned indexesLevel = 1;

    Storage st;

    // for the FL (fixed level) algorithm
    //LBAKTree lbaktree(&st, algoType, indexesLevel, similarityThreshold);

    // for the VL (variable level) algorithm
    //LBAKTree lbaktree(&st, queryWorkloadFile, algoType, spaceBudget, similarityThreshold);

    // for the VLF (variable level exploiting keyword-frequencies) algorithm
    LBAKTree lbaktree(&st, queryWorkloadFile, algoType, spaceBudget, similarityThreshold, keywordFrequencyThreshold);

    ifstream data(datasetFile.c_str());
    if (!data)
    {
        cerr << "cannot open dataset file" << endl;
        return -1;
    }
    ifstream queries(queryWorkloadFile.c_str());
    if (!queries)
    {
        cerr << "cannot open query workload file" << endl;
        return -1;
    }

    string line;
    uintptr_t count = 0;
    struct timeval t1, t2;
    struct timezone tz;

    // inserting the objects in the LBAK-tree
    while (getline(data, line))
    {
        Rectangle mbr;
        Object obj;
        vector<string> kwds;
        if(parseLine(line, mbr, kwds))
        {
            obj.mbr = mbr;
            obj.id = count;
            lbaktree.insert(obj, kwds);
        }
        if ((count % 1000) == 0)
            cerr << count << endl;
        ++count;
    }
    cout << "building the index structures" << endl;
    lbaktree.buildIndex();
    cout << "starting the queries" << endl;

    unsigned numOfQueries = 0, answer = 0;
    double totalTime = 0.0;

    while (getline(queries, line))
    {
        vector<Object> objects;
        Rectangle range;
        vector<string> kwds;
        parseLine(line, range, kwds);

        sort(kwds.begin(), kwds.end(), sortCondition);
        lbaktree.startTimeMeasurement(t1, tz);
        lbaktree.rangeQuery(objects, range, kwds);
        lbaktree.stopTimeMeasurement(t2, tz);
        double timeMeasurement = lbaktree.getTimeMeasurement(t1, t2);
        totalTime += timeMeasurement;

        printResults(lbaktree, objects);
        answer += objects.size();
        ++numOfQueries;

        if ((count % 1000) == 0)
            cerr << count << endl;

        ++count;
    }
    cout << "AVG query time (ms): " << totalTime / numOfQueries << endl;
    cout << "Total queries answer: " << answer << endl;

    return 0;
}


