/*
 $Id: util.h 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 11/01/2009
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
         Shengyue Ji <shengyuj (at) ics.uci.edu>
*/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <vector>
#include <stdint.h>

class Point
{
public:
    float x;
    float y;

    bool operator==(const Point &p) const
    {
        return x == p.x && y == p.y;
    };
};

class Rectangle
{
public:
    // the mbr's lower value
    Point min;
    // the mbr's upper value
    Point max;

    // check whether two rectangles intersect or not
    bool intersects(const Rectangle &rect) const;
    // this = intersection of this and rect, false if no intersection
    bool intersect(const Rectangle &rect);
    // return the squared minimum distance between a point and a rectangle
    double minDist2(Point p) const;
    // return the rectangle's area
    double area() const;
    // return the enlarged area needed to include an object
    double enlargedArea(const Rectangle &rect) const;
    // enlarge an object into a rectangle
    void enlarge(const Rectangle &rect);
    // return the overlapped area between two rectangles
    double overlapedArea(const Rectangle &rect) const;
    // return the perimeter of a rectangle
    double margin() const;
};

class Object
{
public:
    // the object/node id
    uintptr_t id;

    // the rectangle that represents the object/node
    Rectangle mbr;
};

class NodeMinDist2
{
public:
    uintptr_t id;
    double minDist2;
    const std::vector<char> *flags;

    bool operator<(const NodeMinDist2 &n2) const
    {
        return minDist2 > n2.minDist2;
    };
};

class CenterDistance
{
public:
    Object object;
    double distance;

    bool operator<(const CenterDistance &d) const
    {
        return distance > d.distance;
    };
};

class EntryValue
{
public:
    Object object;
    double value;
    uintptr_t index;

    bool operator<(const EntryValue &e) const
    {
        return value < e.value;
    };
};

#endif

