/*
 $Id: rstartree.cc 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 11/01/2009
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
*/

#include "rstartree.h"
#include <stack>
#include <algorithm>
#include <cstring>
#include <limits>
#include <cmath>


using namespace std;
using namespace tr1;
RTree::RTree(Storage *storage)
{
    this->storage = storage;
    nodeSize = sizeof(Node);
    nodeNum = 0;
}

void RTree::create()
{
    Node *root = (Node *)storage->alloc(nodeSize);
    storage->setRoot(root->id);
    root->level = 0;
    root->numChildren = 0;

    root->mbr.min.x = 0.0; // testing
    root->mbr.min.y = 0.0; // testing
    root->mbr.max = root->mbr.min; // testing

    storage->write(root);
    storage->free(root);
    nodeNum = 1;
}

void RTree::rangeQuery(vector<Object> &objects, const Rectangle &range,
                       uintptr_t id)
{
    Node *node = (Node *)storage->read(id);
    unsigned i;
    for(i = 0; i < node->numChildren; i++)
        if(node->objects[i].mbr.intersects(range))
        {
            if(node->isLeaf())
                objects.push_back(node->objects[i]);
            else
                rangeQuery(objects, range, node->objects[i].id);
        }
    storage->free(node);
}

void RTree::rangeQuery(vector<Object> &objects, const Rectangle &range)
{
    objects.clear();
    uintptr_t id = storage->getRoot();
    rangeQuery(objects, range, id);
}

void RTree::kNNQuery(multimap<double, Object> &objects,
                     const Point &point, unsigned k)
{
    objects.clear();
    vector<NodeMinDist2> heap;
    NodeMinDist2 nroot;
    nroot.id = storage->getRoot();
    nroot.minDist2 = 0.0;
    heap.push_back(nroot);

    while(!heap.empty() && (objects.size() < k
                            || heap[0].minDist2 < -objects.begin()->first))
    {
        uintptr_t id = heap[0].id;
        pop_heap(heap.begin(), heap.end());
        heap.pop_back();
        Node *node = (Node *)storage->read(id);
        unsigned i;
        for(i = 0; i < node->numChildren; i++)
            if(node->isLeaf())
            {
                double minDist2 = node->objects[i].mbr.minDist2(point);
                if(objects.size() < k || minDist2 < -objects.begin()->first)
                {
                    objects.insert(pair<double, Object>
                                   (-minDist2, node->objects[i]));
                    if(objects.size() > k)
                        objects.erase(objects.begin());
                }
            }
            else
            {
                NodeMinDist2 nnode;
                nnode.id = node->objects[i].id;
                nnode.minDist2 = node->objects[i].mbr.minDist2(point);
                heap.push_back(nnode);
                push_heap(heap.begin(), heap.end());
            }
        storage->free(node);
    }
}

void RTree::insert(const Object &obj)
{
    uintptr_t id = storage->getRoot();
    Node *node = (Node *)storage->read(id);
    byte* overflowArray = 0;
    overflowArray = new byte[node->level];
    memset(overflowArray, 0, node->level);
    insert(obj, 0, overflowArray);
    storage->free(node);
    delete[] overflowArray;
}

void RTree::insert(const Object &obj, unsigned desiredLevel,
                   byte *overflowArray)
{
    stack<Node *> path;
    stack<unsigned> childPosition;
    uintptr_t id = storage->getRoot();
    Node *node = (Node *)storage->read(id);
    path.push(node);
    childPosition.push(0);

    unsigned minChild = 0;
    double minArea, minEnlarge;
    while(node->level > desiredLevel)
    {
        minChild = 0;
        minArea = std::numeric_limits<double>::max();
        minEnlarge = std::numeric_limits<double>::max();

        // the children pointers in the node point to leaves
        if(node->level == 1)
        {
            // find least overlap enlargment, use minimum enlarged
            // area to break tie, if tie still exists use minimum area
            // to break it
            vector <EntryValue> entries;
            unsigned k;
            for(unsigned i = 0; i < node->numChildren; ++i)
            {
                EntryValue entry;
                entry.object = node->objects[i];
                entry.value = node->objects[i].mbr.enlargedArea(obj.mbr);
                entries.push_back(entry);

                if(entry.value < minEnlarge)
                {
                    minEnlarge = entry.value;
                    minChild = i;
                }
            }

            if (minEnlarge != 0.0)
            {
                minEnlarge = std::numeric_limits<double>::max();
                if(node->numChildren > NEAR_MINIMUM_OVERLAP_FACTOR)
                {
                    // sort the entries based on their area enlargment needed
                    // to include the object
                    sort(entries.begin(), entries.end());
                    k = NEAR_MINIMUM_OVERLAP_FACTOR;
                }
                else
                {
                    k = node->numChildren;
                }

                double minOverlap = std::numeric_limits<double>::max();
                unsigned id = 0;
                for(unsigned i = 0; i < k; ++i)
                {
                    Rectangle mbr1 = entries.at(i).object.mbr;
                    Rectangle mbr2 = entries.at(i).object.mbr;
                    mbr2.enlarge(obj.mbr);
                    double difference = 0.0;

                    for(unsigned j = 0; j < node->numChildren; ++j)
                    {
                        if(node->objects[j].id != entries.at(i).object.id)
                        {
                            double intersection =
                                mbr2.overlapedArea(node->objects[j].mbr);
                            if(intersection != 0.0)
                            {
                                difference += intersection -
                                              mbr1.overlapedArea(node->objects[j].mbr);
                            }
                        }
                        else
                        {
                            id = j;
                        }
                    }
                    double enlarge =
                        entries.at(i).object.mbr.enlargedArea(obj.mbr);
                    double area = entries.at(i).object.mbr.area();

                    if(difference < minOverlap ||
                            (difference == minOverlap && enlarge < minEnlarge) ||
                            (difference == minOverlap && enlarge == minEnlarge &&
                             area < minArea))
                    {
                        minOverlap = difference;
                        minEnlarge = enlarge;
                        minArea = area;
                        minChild = id;
                    }
                }
            }
        }
        else
        {
            // find minimum enlarged area, use minimum area to break tie
            for(unsigned i = 0; i < node->numChildren; ++i)
            {
                double enlarge = node->objects[i].mbr.enlargedArea(obj.mbr);
                double area = node->objects[i].mbr.area();
                if(enlarge < minEnlarge ||
                        (enlarge == minEnlarge && area < minArea))
                {
                    minEnlarge = enlarge;
                    minArea = area;
                    minChild = i;
                }
            }
        }
        // enlarge the node and write if needed
        if(minEnlarge > 0.0)
        {
            node->objects[minChild].mbr.enlarge(obj.mbr);
            node->mbr.enlarge(obj.mbr); // testing
            storage->write(node);
        }

        // move on to the next level
        id = node->objects[minChild].id;
        node = (Node *)storage->read(id);
        path.push(node);
        childPosition.push(minChild);
    }

    // add object to leaf and write
    node->objects[node->numChildren++] = obj;
    node->mbr.enlarge(obj.mbr); // testing
    storage->write(node);

    // overflow treatment
    while(!path.empty())
    {
        node = path.top();
        minChild = childPosition.top();
        id = node->id;
        path.pop();
        childPosition.pop();
        if(node->numChildren == BRANCH_FACTOR)
        {
            // if the level is not the root level and this is the
            // first overflow treatment in the given level during the
            // insertion of one data rectangle
            if(!path.empty() && overflowArray[node->level] == 0)
            {
                overflowArray[node->level] = 1;
                reinsert(node, path.top(), desiredLevel, minChild,
                         overflowArray);
            }
            else
            {
                Object a, b;
                split(node, a, b);
                if(path.empty())
                {
                    // create new root
                    Node *root = (Node *)storage->alloc(nodeSize);
                    storage->setRoot(root->id);
                    root->level = node->level + 1;
                    root->numChildren = 2;
                    root->objects[0] = a;
                    root->objects[1] = b;

                    root->mbr = a.mbr; // testing
                    root->mbr.enlarge(b.mbr); // testing

                    storage->write(root);
                    storage->free(root);
                    ++nodeNum;
                }
                else
                {
                    // insert into parent level
                    Node *node2 = (Node *)path.top();
                    unsigned i;
                    for(i = 0; i < node2->numChildren; ++i)
                        if(node2->objects[i].id == id)
                        {
                            node2->objects[i] = a;
                            break;
                        }
                    node2->objects[node2->numChildren++] = b;
                    storage->write(node2);
                }
            }
        }
        storage->free(node);
    }
}

void RTree::reinsert(Node *node, Node *parentNode, unsigned desiredLevel,
                     unsigned position, byte *overflowArray)
{
    Point p, center;
    center.x = 0.5 * (parentNode->objects[position].mbr.min.x +
                      parentNode->objects[position].mbr.max.x);
    center.y = 0.5 * (parentNode->objects[position].mbr.min.y +
                      parentNode->objects[position].mbr.max.y);

    vector <CenterDistance> dist; // MBR centers distances

    // compute the distance between the center of the node's MBR and
    // all its children MBRs centers
    for(unsigned i = 0; i < node->numChildren; ++i)
    {
        p.x = 0.5 * (node->objects[i].mbr.max.x + node->objects[i].mbr.min.x) -
              center.x;
        p.y = 0.5 * (node->objects[i].mbr.max.y + node->objects[i].mbr.max.y) -
              center.y;

        CenterDistance child;
        child.object.id = node->objects[i].id;
        child.object.mbr = node->objects[i].mbr;
        child.distance = (p.x * p.x) + (p.y * p.y);
        dist.push_back(child);
    }

    // sort the entries in decreasing order of their distances
    sort(dist.begin(), dist.end());

    // delete top (BRANCH_FACTOR * REINSERT_FACTOR) entries from the
    // node
    unsigned i;
    unsigned k = std::floor((double)BRANCH_FACTOR * REINSERT_FACTOR);
    for(i = 0; i < k; ++i)
    {
        deleteObject(node, dist[i].object.id);
    }

    // reset the node's MBR
    parentNode->objects[position].mbr = dist[i].object.mbr;

    node->mbr = dist[i].object.mbr; // testing

    ++i;
    // rebuild the node's MBR
    for(; i < BRANCH_FACTOR; ++i)
    {
        parentNode->objects[position].mbr.enlarge(dist[i].object.mbr);

        node->mbr.enlarge(dist[i].object.mbr); // testing
    }
    storage->write(node);
    storage->write(parentNode);

    // reinsert the top (REINSERT_FACTOR * BRANCH_FACTOR) entries
    for(unsigned j = 0; j < k; ++j)
    {
        insert(dist[j].object, node->level, overflowArray);
    }
}

void RTree::deleteObject(Node *node, uintptr_t id)
{
    for(unsigned i = 0; i < node->numChildren; ++i)
    {
        // to delete an object, place the last object in the deleted
        // slot and reduce numChildren by 1
        if(node->objects[i].id == id)
        {
            node->objects[i] = node->objects[node->numChildren - 1];
            break;
        }
    }
    node->numChildren--;
}

void RTree::split(Node *node, Object &a, Object &b)
{
    // calculations are based on the R*-tree paper
    unsigned m = std::floor((double)BRANCH_FACTOR * SPLIT_FACTOR);
    unsigned splitDistribution = (BRANCH_FACTOR - 1) - (2 * m) + 2;

    // to calculate the minimum margin in order to pick the split axis
    double minMargin = std::numeric_limits<double>::max();

    // to calculate the margin, overlap area, and the area of the
    // distributions
    Rectangle lowerMbr1, upperMbr1, lowerMbr2, upperMbr2;

    // to calculate the minimum values of the overlaped area and the
    // area of the distributions
    double lowerOverlap = 0.0, upperOverlap = 0.0;
    double lowerArea = 0.0, upperArea = 0.0;
    double minLowerOverlap[2];
    double minUpperOverlap[2];
    double minLowerArea[2];
    double minUpperArea[2];

    // to store the partition index that separate between the two
    // groups
    unsigned lowerPartitionIndex[2];
    unsigned upperPartitionIndex[2];

    // to determine which values are chosen (lower or upper values)
    unsigned sortOrder = 0;

    // to store the sorted entries
    vector <vector <EntryValue> > lowerValues;
    vector <vector <EntryValue> > upperValues;

    // choose split axis
    unsigned splitAxis = std::numeric_limits<unsigned>::max();
    for(unsigned dim = 0; dim < 2; ++dim)
    {
        // fill the vectors with the entries lower and upper values of
        // their rectangles
        vector <EntryValue> temp1, temp2;
        for(unsigned i = 0; i < node->numChildren; ++i)
        {
            EntryValue entry;
            entry.object.id = node->objects[i].id;
            entry.object.mbr = node->objects[i].mbr;

            if(dim == 0)
                entry.value = node->objects[i].mbr.min.x;
            else
                entry.value = node->objects[i].mbr.min.y;
            temp1.push_back(entry);

            if(dim == 0)
                entry.value = node->objects[i].mbr.max.x;
            else
                entry.value = node->objects[i].mbr.max.y;
            temp2.push_back(entry);
        }
        lowerValues.push_back(temp1);
        upperValues.push_back(temp2);

        // sort the values
        sort(lowerValues.at(dim).begin(), lowerValues.at(dim).end());
        sort(upperValues.at(dim).begin(), upperValues.at(dim).end());

        // initialization
        minLowerOverlap[dim] = std::numeric_limits<double>::max();
        minUpperOverlap[dim] = std::numeric_limits<double>::max();
        minLowerArea[dim] = std::numeric_limits<double>::max();
        minUpperArea[dim] = std::numeric_limits<double>::max();
        double lowerMargin = 0.0, upperMargin = 0.0;

        // generate (BRANCH_FACTOR - 1) - (2 * m) + 2 distribution for all the
        // BRANCH_FACTOR objects
        for(unsigned k = 1; k < splitDistribution; ++k)
        {
            unsigned d = m - 1 + k;

            lowerMbr1 = lowerValues.at(dim).at(0).object.mbr;
            upperMbr1 = upperValues.at(dim).at(0).object.mbr;

            for(unsigned j = 1; j < d; ++j)
            {
                lowerMbr1.enlarge(lowerValues.at(dim).at(j).object.mbr);
                upperMbr1.enlarge(upperValues.at(dim).at(j).object.mbr);
            }

            lowerMbr2 = lowerValues.at(dim).at(d).object.mbr;
            upperMbr2 = upperValues.at(dim).at(d).object.mbr;

            for(unsigned j = d + 1; j < BRANCH_FACTOR; ++j)
            {
                lowerMbr2.enlarge(lowerValues.at(dim).at(j).object.mbr);
                upperMbr2.enlarge(upperValues.at(dim).at(j).object.mbr);
            }

            // calculate the margin of the distributions
            lowerMargin += lowerMbr1.margin() + lowerMbr2.margin();
            upperMargin += upperMbr1.margin() + upperMbr2.margin();

            // calculate the overlaped area of the distributions
            lowerOverlap = lowerMbr1.overlapedArea(lowerMbr2);
            upperOverlap = upperMbr1.overlapedArea(upperMbr2);

            // calculate the area of the distributions
            lowerArea = lowerMbr1.area() + lowerMbr2.area();
            upperArea = upperMbr1.area() + upperMbr2.area();

            if(lowerOverlap < minLowerOverlap[dim] ||
                    (lowerOverlap == minLowerOverlap[dim] && lowerArea <
                     minLowerArea[dim]))
            {
                minLowerOverlap[dim] = lowerOverlap;
                minLowerArea[dim] = lowerArea;
                lowerPartitionIndex[dim] = d;
            }
            if(upperOverlap < minUpperOverlap[dim] ||
                    (upperOverlap == minUpperOverlap[dim] && upperArea <
                     minUpperArea[dim]))
            {
                minUpperOverlap[dim] = upperOverlap;
                minUpperArea[dim] = upperArea;
                upperPartitionIndex[dim] = d;
            }
        }

        double margin = ::min(lowerMargin, upperMargin);

        // store minimum margin as split axis
        if (margin < minMargin)
        {
            minMargin = margin;
            splitAxis = dim;
            sortOrder = (lowerMargin < upperMargin) ? 0 : 1;
        }
    }

    // setup bid for children a and b
    a.id = node->id;
    Node *node2 = (Node *)storage->alloc(nodeSize);
    node2->level = node->level;
    node2->numChildren = 0;
    b.id = node2->id;
    ++nodeNum;

    if(sortOrder == 0) // lower values
    {
        adjustNode(node, node2, a, b, lowerValues.at(splitAxis),
                   lowerPartitionIndex[splitAxis]);
    }
    else // upper values
    {
        adjustNode(node, node2, a, b, upperValues.at(splitAxis),
                   upperPartitionIndex[splitAxis]);
    }

    node->mbr = a.mbr; // testing
    node2->mbr = b.mbr; // testing

    // write
    storage->write(node);
    storage->write(node2);
    storage->free(node2);
}

void RTree::adjustNode(Node *node, Node *node2, Object &a, Object &b,
                       vector <EntryValue> &entries, unsigned partitionIndex)
{
    // setup mbr for objects a and b
    a.mbr = entries.at(0).object.mbr;
    node->objects[0] = entries.at(0).object;
    node->numChildren = 1;
    for(unsigned i = 1; i < partitionIndex; ++i)
    {
        a.mbr.enlarge(entries.at(i).object.mbr);
        node->objects[node->numChildren++] = entries.at(i).object;
    }

    b.mbr = entries.at(partitionIndex).object.mbr;
    node2->objects[0] = entries.at(partitionIndex).object;
    node2->numChildren = 1;
    for(unsigned i = partitionIndex + 1; i < entries.size(); ++i)
    {
        b.mbr.enlarge(entries.at(i).object.mbr);
        node2->objects[node2->numChildren++] = entries.at(i).object;
    }
}

bool RTree::retrieve(unordered_set<uintptr_t> &ids, uintptr_t oid, uintptr_t id)
{
    Node *node = (Node *)storage->read(id);
    unsigned i;
    bool ret = false;
    for(i = 0; i < node->numChildren; i++)
        if(node->isLeaf())
        {
            if(ids.count(node->objects[i].id))
                fprintf(stderr, "retrieve panic!\n");
            else
            {
                ids.insert(node->objects[i].id);
                if(oid == node->objects[i].id)
                {
                    ret = true;
                    printf("path[%u] %f %f %f %f\n", node->level,
                           node->objects[i].mbr.min.x,
                           node->objects[i].mbr.max.x,
                           node->objects[i].mbr.min.y,
                           node->objects[i].mbr.max.y);
                }
            }

        }
        else
        {
            if(retrieve(ids, oid, node->objects[i].id))
            {
                ret = true;
                printf("path[%u] %f %f %f %f\n", node->level,
                       node->objects[i].mbr.min.x,
                       node->objects[i].mbr.max.x,
                       node->objects[i].mbr.min.y,
                       node->objects[i].mbr.max.y);
            }
        }
    storage->free(node);
    return ret;
}

void RTree::retrieve(unordered_set<uintptr_t> &ids, uintptr_t oid)
{
    retrieve(ids, oid, storage->getRoot());
}

