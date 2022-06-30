/*
 $Id: util.cc 6148 2012-02-22 23:13:40Z salsubaiee $

 Copyright (C) 2010 by The Regents of the University of California

 Redistribution of this file is permitted under
 the terms of the BSD license.

 Date: 11/01/2009
 Author: Sattam Alsubaiee <salsubai (at) ics.uci.edu>
         Shengyue Ji <shengyuj (at) ics.uci.edu>
*/

#include "util.h"
#include <algorithm>


bool Rectangle::intersects(const Rectangle &rect) const
{
    return min.x <= rect.max.x && rect.min.x <= max.x
           && min.y <= rect.max.y && rect.min.y <= max.y;
}

bool Rectangle::intersect(const Rectangle &rect)
{
    if(rect.min.x > min.x)min.x = rect.min.x;
    if(rect.max.x < max.x)max.x = rect.max.x;
    if(rect.min.y > min.y)min.y = rect.min.y;
    if(rect.max.y < max.y)max.y = rect.max.y;

    if(min.x > max.x || min.y > max.y)
        return false;
    return true;
}

double Rectangle::minDist2(Point p) const
{
    double dx = 0.0;
    double dy = 0.0;
    if(p.x < min.x)dx = (double)min.x - p.x;
    else if(p.x > max.x)dx = (double)p.x - max.x;
    if(p.y < min.y)dy = (double)min.y - p.y;
    else if(p.y > max.y)dy = (double)p.y - max.y;
    return dx * dx + dy * dy;
}


double Rectangle::area() const
{
    return ((double)max.x - min.x) * ((double)max.y - min.y);
}

double Rectangle::enlargedArea(const Rectangle &rect) const
{
    Rectangle rect2 = *this;
    rect2.enlarge(rect);
    return rect2.area() - area();
}

void Rectangle::enlarge(const Rectangle &rect)
{
    if(rect.min.x < min.x)min.x = rect.min.x;
    if(rect.max.x > max.x)max.x = rect.max.x;
    if(rect.min.y < min.y)min.y = rect.min.y;
    if(rect.max.y > max.y)max.y = rect.max.y;
}

double Rectangle::overlapedArea(const Rectangle &rect) const
{
    double x, y;
    x = std::min((double)rect.max.x, (double)max.x) -
        std::max((double)rect.min.x, (double)min.x);
    if (x <= 0.0)
        return 0;
    y = std::min((double)rect.max.y, (double)max.y) -
        std::max((double)rect.min.y, (double)min.y);
    if (y <= 0.0)
        return 0;
    return x * y;
}

double Rectangle::margin() const
{
    return 2.0 * (((double)max.x - min.x) + ((double)max.y - min.y));
}
