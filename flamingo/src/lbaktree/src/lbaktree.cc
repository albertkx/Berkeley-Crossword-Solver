/*
 $Id: lbaktree.cc 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 08/19/2010
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include <limits>
#include "lbaktree.h"

LBAKTree::LBAKTree(Storage *storage, AlgorithmType type, unsigned il, float simT) : RTree(storage), kf1("temp1"), kf2("temp2")
{
    if(type != fl)
    {
        cout << "error: wrong algorithm name" << endl;
        exit(1);
    }
    init(type, simT);
    indexesLevel = il;
}

LBAKTree::LBAKTree(Storage *storage, string &file, AlgorithmType type, double sb, float simT) : RTree(storage), kf1("temp1"), kf2("temp2")
{
    if(type != vl)
    {
        cout << "error: wrong algorithm name" << endl;
        exit(1);
    }
    init(type, simT);
    queryWorkloadFile = file;
    spaceBudget = sb;
}

LBAKTree::LBAKTree(Storage *storage, string &file, AlgorithmType type, double sb, float simT, float kfT) : RTree(storage), kf1("temp1"), kf2("temp2")
{
    if(type != vlf)
    {
        cout << "error: wrong algorithm name" << endl;
        exit(1);
    }
    init(type, simT);
    queryWorkloadFile = file;
    spaceBudget = sb;
    kfThreshold = kfT;
}

LBAKTree::~LBAKTree()
{
    delete gramGen;
    unordered_map <uintptr_t, vector<unsigned> *>::iterator mit1;
    for (mit1 = recordsMap.begin(); mit1 != recordsMap.end(); ++mit1)
    {
        delete recordsMap[mit1->first];
    }
    recordsMap.clear();
    unordered_map <uintptr_t, Array<unsigned> *>::iterator mit2;
    for (mit2 = keywordsHashesMap.begin(); mit2 != keywordsHashesMap.end(); ++mit2)
    {
        delete keywordsHashesMap[mit2->first];
    }
    keywordsHashesMap.clear();
    unordered_map <uintptr_t, WrapperSimpleEdNorm *>::iterator mit3;
    for (mit3 = wrappersMap.begin(); mit3 != wrappersMap.end(); ++mit3)
    {
        delete wrappersMap[mit3->first];
    }
    wrappersMap.clear();
    unordered_map <uintptr_t, StringContainerVector *>::iterator mit4;
    for (mit4 = strContainersMap.begin(); mit4 != strContainersMap.end(); ++mit4)
    {
        delete strContainersMap[mit4->first];
    }
    strContainersMap.clear();
}

void LBAKTree::init(AlgorithmType type, float simT)
{
    create();
    algoType = type;
    simThreshold = simT;
    avgKwdsLength = 0;
    numKwds = 0;
    q = 2;
    gramGen = new GramGenFixedLen(q);
    sx = 0;
    sy = 0;
    sx2 = 0;
    sy2 = 0;
    sxy = 0;
    n = 0;
}

void LBAKTree::insert(const Object &obj, vector <string> &kwds)
{
    recordsMap[obj.id] = new vector <unsigned> ();
    for(unsigned i = 0; i < kwds.size(); ++i)
    {
        unordered_map<string, unsigned>::iterator it;
        it = keywordsMap.find(kwds.at(i));
        avgKwdsLength += (double)kwds.at(i).length();
        ++numKwds;
        if(it == keywordsMap.end())
        {
            dictionary.push_back(kwds.at(i));
            keywordsMap[kwds.at(i)] = dictionary.size() - 1;
            recordsMap[obj.id]->push_back(dictionary.size() - 1);
        }
        else
        {
            recordsMap[obj.id]->push_back(it->second);
        }
    }
    RTree::insert(obj);
}

void LBAKTree::buildIndex()
{
    if(!kf1.open(true))
    {
        cout << "fatal error: failed to open temp1 file" << endl;
        exit(1);
    }
    if(!kf2.open(true))
    {
        cout << "fatal error: failed to open temp2 file" << endl;
        exit(1);
    }
    propagateKeywords(storage->getRoot());
    selectSANodes();
    fillKeywordsHashesMap();
    fillWrappersMap();
    kf1.close();
    kf2.close();
}

void LBAKTree::readQueryWorkload(const Rectangle &range)
{
    uintptr_t id = storage->getRoot();
    ++queryWorkloadMap[id];
    readQueryWorkload(range, id);
}

void LBAKTree::readQueryWorkload(const Rectangle &range, uintptr_t objectId)
{
    Node *node = (Node *)storage->read(objectId);
    for(unsigned i = 0; i < node->numChildren; ++i)
    {
        if(node->objects[i].mbr.intersects(range))
        {
            ++queryWorkloadMap[node->objects[i].id];
            if(!node->isLeaf())
            {
                readQueryWorkload(range, node->objects[i].id);
            }
        }
    }
    storage->free(node);
}

void LBAKTree::propagateKeywords(uintptr_t objectId)
{
    Node *node = (Node *)storage->read(objectId);
    unordered_set<string> kwds;
    if(node->isLeaf())
    {
        for(unsigned i = 0; i < node->numChildren; ++i)
        {
            insertKeywords(node->objects[i].id, kwds, true);
        }
    }
    else
    {
        for(unsigned i = 0; i < node->numChildren; ++i)
        {
            propagateKeywords(node->objects[i].id);
            insertKeywords(node->objects[i].id, kwds, false);
        }
    }
    string text;
    unordered_set<string>::iterator it;
    for (it = kwds.begin(); it != kwds.end(); ++it)
    {
        text += *it;
        text += " ";
    }
    kf1.write(text, node->id, kwds.size());
    storage->free(node);
}

void LBAKTree::insertKeywords(uintptr_t objectId, unordered_set<string> &kwds, bool leaf)
{
    if(leaf)
    {
        for (unsigned i = 0; i < recordsMap[objectId]->size(); ++i)
        {
            kwds.insert(dictionary[recordsMap[objectId]->at(i)]);
        }
    }
    else
    {
        string text = kf1.read(objectId);
        parseKeywords(text, kwds);
    }
}

void LBAKTree::parseKeywords(string &text, unordered_set<string> &kwds)
{
    string::size_type lastPos = text.find_first_not_of(" ", 0);
    string::size_type pos = text.find_first_of(" ", lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        kwds.insert(text.substr(lastPos, pos - lastPos));
        lastPos = text.find_first_not_of(" ", pos);
        pos = text.find_first_of(" ", lastPos);
    }
}

void LBAKTree::computeGradientIntercept(uintptr_t objectId, vector <string> &keywords)
{
    Node *node = (Node *)storage->read(objectId);
    string text = kf1.read(node->id);
    unordered_set<string> kwds;
    parseKeywords(text, kwds);
    if(objectId == storage->getRoot())
    {
        srand((unsigned)time(0));
        for(unsigned i = 0; i < 100; ++i)
        {
            keywords.push_back(dictionary[rand() % dictionary.size()]);
        }
    }
    StringContainerVector strContainer;
    strContainer.initStatsCollector(gramGen);
    unordered_set<string>::iterator it;
    for (it = kwds.begin(); it != kwds.end(); ++it)
    {
        strContainer.insertString(*it);
    }
    WrapperSimpleEdNorm wrapper(&strContainer, gramGen, false);
    wrapper.buildIndex();
    for(unsigned i = 0; i < keywords.size(); ++i)
    {
        vector<unsigned> resultStringIDs;
        struct timeval t1, t2;
        struct timezone tz;
        startTimeMeasurement(t1, tz);
        wrapper.search(keywords.at(i), simThreshold, resultStringIDs);
        stopTimeMeasurement(t2, tz);
        double timeMeasurement = getTimeMeasurement(t1, t2);
        sx += (double) strContainer.size();
        sy += timeMeasurement;
        sx2 += ((double)strContainer.size() * (double)strContainer.size());
        sy2 += (timeMeasurement * timeMeasurement);
        sxy += ((double)strContainer.size() * timeMeasurement);
        ++n;
    }
    if(!node->isLeaf())
    {
        computeGradientIntercept(node->objects[0].id, keywords);
    }
    storage->free(node);
}

void LBAKTree::selectSANodes()
{
    avgKwdsLength /= (double)numKwds;
    if(algoType == fl)
    {
        useFL();
    }
    else
    {
        vector<string> keywords;
        computeGradientIntercept(storage->getRoot(), keywords);
        gradient = ((sx * sy) - (n * sxy)) / ((sx * sx) - (n * sx2));
        intercept = ((sx * sxy) - (sy * sx2)) / ((sx * sx) - (n * sx2));

        ifstream queries(queryWorkloadFile.c_str());
        if (!queries)
        {
            cerr << "fatal error: failed to open query workload file" << endl;
            exit(1);
        }
        string line;
        while (getline(queries, line))
        {
            Rectangle range;
            string coordinates = line.substr(0, line.find(","));
            istringstream coordinatesStream(coordinates);
            coordinatesStream >> range.min.x >> range.min.y >> range.max.x >> range.max.y;
            readQueryWorkload(range);
        }
        if(algoType == vl)
        {
            useVL();
        }
        else
        {
            fillKeywordsIntersectionsFile();
            useVLF();
        }
    }
}

void LBAKTree::useFL()
{
    Node *root = (Node *)storage->read(storage->getRoot());
    if(indexesLevel > root->level)
    {
        indexesLevel = root->level;
    }
    storage->free(root);

    unordered_map <uintptr_t, IndexNode>::iterator mit;
    for (mit = kf1.begin(); mit != kf1.end(); ++mit)
    {
        Node *node = (Node *)storage->read(mit->first);
        if(node->level >= indexesLevel)
        {
            StringContainerVector *strContainer;
            strContainer = new StringContainerVector(true);
            strContainer->initStatsCollector(gramGen);
            strContainersMap[node->id] = strContainer;
        }
        storage->free(node);
    }
}

void LBAKTree::useVL()
{
    vector<NodePriority> heap;
    Node *root = (Node *)storage->read(storage->getRoot());
    double pFuzzySpaceCost = (double)kf1.getIndexNode(root->id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
    double pFuzzyTimeCost = (double)queryWorkloadMap[root->id] * (gradient * (double)kf1.getIndexNode(root->id).numKeywords + intercept);
    double cFuzzySpaceCost = 0;
    double cFuzzyTimeCost = 0;
    for(unsigned i = 0; i < root->numChildren; ++i)
    {
        cFuzzySpaceCost += (double)kf1.getIndexNode(root->objects[i].id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
        cFuzzyTimeCost += (double)queryWorkloadMap[root->objects[i].id] * (gradient * (double)kf1.getIndexNode(root->objects[i].id).numKeywords + intercept);
    }
    NodePriority rootImportance;
    rootImportance.id = root->id;
    rootImportance.priority = (cFuzzyTimeCost - pFuzzyTimeCost) / (pFuzzySpaceCost - cFuzzySpaceCost);
    rootImportance.pFuzzySpaceCost = pFuzzySpaceCost;
    rootImportance.cFuzzySpaceCost = cFuzzySpaceCost;
    rootImportance.pFuzzyTimeCost = pFuzzyTimeCost;
    rootImportance.cFuzzyTimeCost = cFuzzyTimeCost;
    spaceBudget -= pFuzzySpaceCost;
    heap.push_back(rootImportance);
    storage->free(root);
    while(!heap.empty())
    {
        NodePriority nodeImportance;
        nodeImportance.id = heap[0].id;
        nodeImportance.priority = heap[0].priority;
        nodeImportance.pFuzzySpaceCost = heap[0].pFuzzySpaceCost;
        nodeImportance.cFuzzySpaceCost = heap[0].cFuzzySpaceCost;
        nodeImportance.pFuzzyTimeCost = heap[0].pFuzzyTimeCost;
        nodeImportance.cFuzzyTimeCost = heap[0].cFuzzyTimeCost;
        pop_heap(heap.begin(), heap.end());
        heap.pop_back();
        Node *node = (Node *)storage->read(nodeImportance.id);
        if(nodeImportance.priority > 0 && nodeImportance.cFuzzySpaceCost <= (spaceBudget + nodeImportance.pFuzzySpaceCost) && !node->isLeaf() && node->level != 1)
        {
            spaceBudget += nodeImportance.pFuzzySpaceCost;
            spaceBudget -= nodeImportance.cFuzzySpaceCost;
            StringContainerVector *strContainer;
            strContainer = new StringContainerVector(true);
            strContainer->initStatsCollector(gramGen);
            strContainersMap[node->id] = strContainer;
        }
        else
        {
            StringContainerVector *strContainer;
            strContainer = new StringContainerVector(true);
            strContainer->initStatsCollector(gramGen);
            strContainersMap[node->id] = strContainer;
            storage->free(node);
            continue;
        }
        for(unsigned i = 0; i < node->numChildren; ++i)
        {
            Node *node2 = (Node *)storage->read(node->objects[i].id);
            pFuzzySpaceCost = (double)kf1.getIndexNode(node2->id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
            pFuzzyTimeCost = (double)queryWorkloadMap[node2->id] * (gradient * (double)kf1.getIndexNode(node2->id).numKeywords + intercept);
            cFuzzySpaceCost = 0;
            cFuzzyTimeCost = 0;
            if(node2->isLeaf())
            {
                cFuzzySpaceCost = 0;
                cFuzzyTimeCost = std::numeric_limits<double>::max();
            }
            else
            {
                for(unsigned j = 0; j < node2->numChildren; ++j)
                {
                    cFuzzySpaceCost += (double)kf1.getIndexNode(node2->objects[j].id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
                    cFuzzyTimeCost += (double)queryWorkloadMap[node2->objects[j].id] *  (gradient * (double)kf1.getIndexNode(node2->objects[j].id).numKeywords + intercept);
                }
            }
            NodePriority nodeImportance2;
            nodeImportance2.id = node2->id;
            nodeImportance2.priority = (cFuzzyTimeCost - pFuzzyTimeCost) / (pFuzzySpaceCost - cFuzzySpaceCost);
            nodeImportance2.pFuzzySpaceCost = pFuzzySpaceCost;
            nodeImportance2.cFuzzySpaceCost = cFuzzySpaceCost;
            nodeImportance2.pFuzzyTimeCost = pFuzzyTimeCost;
            nodeImportance2.cFuzzyTimeCost = cFuzzyTimeCost;
            heap.push_back(nodeImportance2);
            push_heap(heap.begin(), heap.end());
            storage->free(node2);
        }
        storage->free(node);
    }
}

void LBAKTree::useVLF()
{
    vector<NodePriority> heap;
    Node *root = (Node *)storage->read(storage->getRoot());
    double pFuzzySpaceCost = (double)kf1.getIndexNode(root->id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
    double pFuzzyTimeCost = (double)queryWorkloadMap[root->id] * (gradient * (double)kf1.getIndexNode(root->id).numKeywords + intercept);
    double cFuzzySpaceCost = 0;
    double cFuzzyTimeCost = 0;
    for(unsigned j = 0; j < root->numChildren; ++j)
    {
        unordered_set <string> intersectedKeywords;
        unordered_set<string>::iterator it;
        string text = kf1.read(root->objects[j].id);
        unordered_set<string> kwds;
        parseKeywords(text, kwds);
        string intsctText = kf2.read(root->objects[j].id);
        unordered_set<string> intsctKwds;
        parseKeywords(intsctText, intsctKwds);
        for (it = intsctKwds.begin(); it != intsctKwds.end(); ++it)
        {
            if(kwds.find(*it) != kwds.end())
            {
                intersectedKeywords.insert(*it);
            }
        }
        cFuzzySpaceCost += (double)(kf1.getIndexNode(root->objects[j].id).numKeywords - intersectedKeywords.size()) * (avgKwdsLength + (double)q - 1.0) * 4.0;
        cFuzzyTimeCost += (double)queryWorkloadMap[root->objects[j].id] *  (gradient * (double)(kf1.getIndexNode(root->objects[j].id).numKeywords - intersectedKeywords.size()) + intercept);
    }
    cFuzzySpaceCost += (double)kf2.getIndexNode(root->id).numKeywords * (avgKwdsLength + (double)q - 1.0) * 4.0;
    cFuzzyTimeCost += (double)queryWorkloadMap[root->id] *  (gradient * (double)kf2.getIndexNode(root->id).numKeywords + intercept);
    NodePriority rootImportance;
    rootImportance.id = root->id;
    if(pFuzzySpaceCost == cFuzzySpaceCost)
    {
        rootImportance.priority = pFuzzyTimeCost - cFuzzyTimeCost;
    }
    else
    {
        rootImportance.priority = (cFuzzyTimeCost - pFuzzyTimeCost) / (pFuzzySpaceCost - cFuzzySpaceCost);
    }
    rootImportance.pFuzzySpaceCost = pFuzzySpaceCost;
    rootImportance.cFuzzySpaceCost = cFuzzySpaceCost;
    rootImportance.pFuzzyTimeCost = pFuzzyTimeCost;
    rootImportance.cFuzzyTimeCost = cFuzzyTimeCost;
    spaceBudget -= pFuzzySpaceCost;
    heap.push_back(rootImportance);
    storage->free(root);
    while(!heap.empty())
    {
        NodePriority nodeImportance;
        nodeImportance.id = heap[0].id;
        nodeImportance.priority = heap[0].priority;
        nodeImportance.pFuzzySpaceCost = heap[0].pFuzzySpaceCost;
        nodeImportance.cFuzzySpaceCost = heap[0].cFuzzySpaceCost;
        vector <uintptr_t> ancestorsIds = heap[0].ancestorsIds;
        nodeImportance.pFuzzyTimeCost = heap[0].pFuzzyTimeCost;
        nodeImportance.cFuzzyTimeCost = heap[0].cFuzzyTimeCost;
        pop_heap(heap.begin(), heap.end());
        heap.pop_back();
        Node *node = (Node *)storage->read(nodeImportance.id);
        if(nodeImportance.priority > 0 && nodeImportance.cFuzzySpaceCost <= (spaceBudget + nodeImportance.pFuzzySpaceCost) && !node->isLeaf())
        {
            spaceBudget += nodeImportance.pFuzzySpaceCost;
            spaceBudget -= nodeImportance.cFuzzySpaceCost;
            StringContainerVector *strContainer;
            strContainer = new StringContainerVector(true);
            strContainer->initStatsCollector(gramGen);
            strContainersMap[node->id] = strContainer;
        }
        else
        {
            StringContainerVector *strContainer;
            strContainer = new StringContainerVector(true);
            strContainer->initStatsCollector(gramGen);
            strContainersMap[node->id] = strContainer;
            storage->free(node);
            continue;
        }
        ancestorsIds.push_back(nodeImportance.id);
        for(unsigned i = 0; i < node->numChildren; ++i)
        {
            Node *node2 = (Node *)storage->read(node->objects[i].id);
            string text = kf1.read(node2->id);
            unordered_set<string> kwds;
            parseKeywords(text, kwds);
            string intsctText = kf2.read(node2->id);
            unordered_set<string> intsctKwds;
            parseKeywords(intsctText, intsctKwds);
            unordered_map<uintptr_t, unordered_set<string> > tempMap;
            for(unsigned j = 0; j < ancestorsIds.size(); ++j)
            {
                string intsctText2 = kf2.read(ancestorsIds[j]);
                parseKeywords(intsctText2, tempMap[ancestorsIds[j]]);
            }
            if(node2->isLeaf())
            {
                for(unsigned j = 0; j < ancestorsIds.size(); ++j)
                {
                    unordered_set<string>::iterator it;
                    for (it = tempMap[ancestorsIds[j]].begin(); it != tempMap[ancestorsIds[j]].end(); ++it)
                    {
                        kwds.erase(*it);
                    }
                }
                cFuzzySpaceCost = 0;
                cFuzzyTimeCost = std::numeric_limits<double>::max();
            }
            else
            {
                for(unsigned j = 0; j < ancestorsIds.size(); ++j)
                {
                    unordered_set<string>::iterator it;
                    for (it = tempMap[ancestorsIds[j]].begin(); it != tempMap[ancestorsIds[j]].end(); ++it)
                    {
                        intsctKwds.erase(*it);
                        kwds.erase(*it);
                    }
                }
                cFuzzySpaceCost = 0;
                cFuzzyTimeCost = 0;
                ancestorsIds.push_back(node2->id);
                for(unsigned j = 0; j < node2->numChildren; ++j)
                {
                    string text2 = kf1.read(node2->objects[j].id);
                    unordered_set<string> kwds2;
                    parseKeywords(text2, kwds2);
                    unordered_set <string> intersectedKeywords;
                    for(unsigned k = 0; k < ancestorsIds.size(); ++k)
                    {
                        unordered_set<string>::iterator it;
                        for (it = tempMap[ancestorsIds[k]].begin(); it != tempMap[ancestorsIds[k]].end(); ++it)
                        {
                            if(kwds2.find(*it) != kwds2.end())
                            {
                                intersectedKeywords.insert(*it);
                            }
                        }
                    }
                    cFuzzySpaceCost += (double)(kf1.getIndexNode(node2->objects[j].id).numKeywords - intersectedKeywords.size()) * (avgKwdsLength + (double)q - 1.0) * 4.0;
                    cFuzzyTimeCost += (double)queryWorkloadMap[node2->objects[j].id] *  (gradient * (double)(kf1.getIndexNode(node2->objects[j].id).numKeywords - intersectedKeywords.size()) + intercept);
                }
                cFuzzySpaceCost += (double)intsctKwds.size() * (avgKwdsLength + (double)q - 1.0) * 4.0;
                cFuzzyTimeCost += (double)queryWorkloadMap[node2->id] *  (gradient * (double)intsctKwds.size() + intercept);
                ancestorsIds.pop_back();
            }
            pFuzzySpaceCost = (double)kwds.size() * (avgKwdsLength + (double)q - 1.0) * 4.0;
            pFuzzyTimeCost = (double)queryWorkloadMap[node2->id] * (gradient * (double)kwds.size() + intercept);
            NodePriority nodeImportance2;
            nodeImportance2.id = node2->id;
            if(pFuzzySpaceCost == cFuzzySpaceCost)
            {
                nodeImportance2.priority = pFuzzyTimeCost - cFuzzyTimeCost;
            }
            else
            {
                nodeImportance2.priority = (cFuzzyTimeCost - pFuzzyTimeCost) / (pFuzzySpaceCost - cFuzzySpaceCost);
            }
            nodeImportance2.pFuzzySpaceCost = pFuzzySpaceCost;
            nodeImportance2.cFuzzySpaceCost = cFuzzySpaceCost;
            nodeImportance2.ancestorsIds = ancestorsIds;
            nodeImportance2.pFuzzyTimeCost = pFuzzyTimeCost;
            nodeImportance2.cFuzzyTimeCost = cFuzzyTimeCost;
            heap.push_back(nodeImportance2);
            push_heap(heap.begin(), heap.end());
            text = "";
            unordered_set<string>::iterator it;
            for (it = kwds.begin(); it != kwds.end(); ++it)
            {
                text += *it;
                text += " ";
            }
            kf1.write(text, node2->id, kwds.size());
            intsctText = "";
            for (it = intsctKwds.begin(); it != intsctKwds.end(); ++it)
            {
                intsctText += *it;
                intsctText += " ";
            }
            kf2.write(intsctText, node2->id, intsctKwds.size());
            storage->free(node2);
        }
        storage->free(node);
    }
}

void LBAKTree::fillKeywordsIntersectionsFile()
{
    unordered_map <uintptr_t, IndexNode>::iterator mit;
    for (mit = kf1.begin(); mit != kf1.end(); ++mit)
    {
        Node *node = (Node *)storage->read(mit->first);
        if(!node->isLeaf())
        {
            unordered_set<string> intsctKwds;
            string text = kf1.read(node->id);
            unordered_set<string> kwds;
            parseKeywords(text, kwds);
            unordered_map<uintptr_t, unordered_set<string> > tempMap;
            for(unsigned i = 0; i < node->numChildren; ++i)
            {
                string text2 = kf1.read(node->objects[i].id);
                parseKeywords(text2, tempMap[node->objects[i].id]);
            }
            unordered_set<string>::iterator it;
            for (it = kwds.begin(); it != kwds.end(); ++it)
            {
                double counter = 0;
                for(unsigned i = 0; i < node->numChildren; ++i)
                {
                    if(tempMap[node->objects[i].id].find(*it) != tempMap[node->objects[i].id].end())
                    {
                        ++counter;
                    }
                }
                if(counter >= (double)node->numChildren * kfThreshold)
                {
                    intsctKwds.insert(*it);
                }
            }
            string intsctText;
            for (it = intsctKwds.begin(); it != intsctKwds.end(); ++it)
            {
                intsctText += *it;
                intsctText += " ";
            }
            kf2.write(intsctText, node->id, intsctKwds.size());
        }
        storage->free(node);
    }
}

void LBAKTree::fillKeywordsHashesMap()
{
    unordered_map <uintptr_t, IndexNode>::iterator mit;
    for (mit = kf1.begin(); mit != kf1.end(); ++mit)
    {
        Node *node = (Node *)storage->read(mit->first);
        if(strContainersMap.find(node->id) == strContainersMap.end())
        {
            string text = kf1.read(node->id);
            unordered_set<string> kwds;
            parseKeywords(text, kwds);
            Array<unsigned> *array = new Array<unsigned>(kwds.size(), 1);
            unordered_set<string>::iterator it;
            for (it = kwds.begin(); it != kwds.end(); ++it)
            {
                array->append(keywordsMap[*it]);
            }
            sort (array->begin(), array->end());
            keywordsHashesMap[node->id] = array;
        }
        storage->free(node);
    }
}

void LBAKTree::fillWrappersMap()
{
    WrapperSimpleEdNorm *wrapper;
    unordered_map <uintptr_t, IndexNode>::iterator mit;
    for (mit = kf1.begin(); mit != kf1.end(); ++mit)
    {
        Node *node = (Node *)storage->read(mit->first);
        if(strContainersMap.find(node->id) != strContainersMap.end() && strContainersMap.find(node->objects[0].id) == strContainersMap.end())
        {
            string text = kf1.read(node->id);
            unordered_set<string> kwds;
            parseKeywords(text, kwds);
            unordered_set<string>::iterator it;
            for (it = kwds.begin(); it != kwds.end(); ++it)
            {
                strContainersMap[node->id]->insertString(*it);
            }
            wrapper = new WrapperSimpleEdNorm(strContainersMap[node->id], gramGen, false);
            wrapper->buildIndex();
            wrappersMap[node->id] = wrapper;
        }
        else if(strContainersMap.find(node->objects[0].id) != strContainersMap.end())
        {
            string intsctText = kf2.read(node->id);
            unordered_set<string> intsctKwds;
            parseKeywords(intsctText, intsctKwds);
            unordered_set<string>::iterator it;
            for (it = intsctKwds.begin(); it != intsctKwds.end(); ++it)
            {
                strContainersMap[node->id]->insertString(*it);
            }
            wrapper = new WrapperSimpleEdNorm(strContainersMap[node->id], gramGen, false);
            wrapper->buildIndex();

            wrappersMap[node->id] = wrapper;
        }
        storage->free(node);
    }
}

void LBAKTree::rangeQuery(vector<Object> &objects, const Rectangle &range,
                          const vector <string> &kwds)
{
    uintptr_t id = storage->getRoot();
    vector<string> strings[kwds.size()];
    vector<unsigned> hashes[kwds.size()];

    if(searchWrapper(id, kwds, strings, hashes, strings, hashes))
    {
        rangeQuery(objects, range, id, kwds, strings, hashes);
    }
}

void LBAKTree::rangeQuery(vector<Object> &objects, const Rectangle &range,
                          uintptr_t id, const vector <string> &kwds, vector<string> strings[], vector<unsigned> hashes[])
{
    Node *node = (Node *)storage->read(id);

    if(strContainersMap.find(node->id) != strContainersMap.end() && strContainersMap.find(node->objects[0].id) == strContainersMap.end())
    {
        for(unsigned i = 0; i < kwds.size(); ++i)
        {
            if(hashes[i].empty())
            {
                return;
            }
        }
    }
    for(unsigned i = 0; i < node->numChildren; ++i)
    {
        if(node->objects[i].mbr.intersects(range))
        {
            if(node->isLeaf())
            {
                bool keywordFounded = true;
                for (unsigned j = 0; j < kwds.size(); ++j)
                {
                    bool candidateFounded = false;
                    for (unsigned k = 0; k < strings[j].size(); ++k)
                    {
                        if(searchVector(node->objects[i].id, strings[j].at(k)))
                        {
                            candidateFounded = true;
                            break;
                        }
                    }
                    if (!candidateFounded)
                    {
                        keywordFounded = false;
                        break;
                    }
                }
                if(keywordFounded)
                {
                    objects.push_back(node->objects[i]);
                }
            }
            else
            {
                vector<string> resultStrings[kwds.size()];
                vector<unsigned> resultHashes[kwds.size()];
                if(strContainersMap.find(node->objects[i].id) != strContainersMap.end())
                {
                    if(searchWrapper(node->objects[i].id, kwds, strings, hashes, resultStrings, resultHashes))
                    {
                        rangeQuery(objects, range, node->objects[i].id, kwds, resultStrings, resultHashes);
                    }
                }
                else
                {
                    if(searchArray(node->objects[i].id, kwds.size(), strings, hashes, resultStrings, resultHashes))
                    {
                        rangeQuery(objects, range, node->objects[i].id, kwds, resultStrings, resultHashes);
                    }
                }
            }
        }
    }
    storage->free(node);
}

bool LBAKTree::searchWrapper(uintptr_t objectId, const vector <string> &kwds, vector<string> strings[], vector<unsigned> hashes[], vector<string>resultStrings[], vector<unsigned>resultHashes[])
{
    for(unsigned i = 0; i < kwds.size(); ++i)
    {
        vector<unsigned> resultStringIDs;
        wrappersMap[objectId]->search(kwds.at(i), simThreshold, resultStringIDs);
        for (unsigned j = 0; j < strings[i].size(); ++j)
        {
            resultStrings[i].push_back(strings[i].at(j));
            resultHashes[i].push_back(hashes[i].at(j));
        }
        for (unsigned j = 0; j < resultStringIDs.size(); ++j)
        {
            string temp;
            strContainersMap[objectId]->retrieveString(temp, resultStringIDs.at(j));
            resultStrings[i].push_back(temp);
            resultHashes[i].push_back(keywordsMap[temp]);
        }
    }
    return true;
}

bool LBAKTree::searchArray(uintptr_t objectId, unsigned numKeywords, vector<string> strings[], vector<unsigned> hashes[], vector<string>resultStrings[], vector<unsigned>resultHashes[])
{
    for(unsigned i = 0; i < numKeywords; ++i)
    {
        for(unsigned j = 0; j < hashes[i].size(); ++j)
        {
            if(keywordsHashesMap[objectId]->has(hashes[i].at(j)))
            {
                resultStrings[i].push_back(strings[i].at(j));
                resultHashes[i].push_back(hashes[i].at(j));
            }
        }
        if (resultStrings[i].empty())
            return false;
    }
    return true;
}

bool LBAKTree::searchVector(uintptr_t objectId, const string &keyword)
{
    for(unsigned i = 0; i < recordsMap[objectId]->size(); ++i)
    {
        if(dictionary[recordsMap[objectId]->at(i)].length() == keyword.length())
        {
            if(dictionary[recordsMap[objectId]->at(i)].compare(keyword) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

void LBAKTree::getObjectKeywords(uintptr_t objectId, vector<string> &objectKeywords)
{
    for(unsigned i = 0; i < recordsMap[objectId]->size(); ++i)
    {
        objectKeywords.push_back(dictionary[recordsMap[objectId]->at(i)]);
    }
}

void LBAKTree::startTimeMeasurement(struct timeval &t1, struct timezone &tz)
{
    gettimeofday(&t1, &tz);
}

void LBAKTree::stopTimeMeasurement(struct timeval &t2, struct timezone &tz)
{
    gettimeofday(&t2, &tz);
}

double LBAKTree::getTimeMeasurement(struct timeval &t1, struct timeval &t2)
{
    unsigned totalTime = (t2.tv_sec - t1.tv_sec) * 1000000 +
                         (t2.tv_usec - t1.tv_usec);
    double tval = 0;
    tval = static_cast<double>(totalTime) / 1000;
    return tval;
}
