/*  
    $Id: looptimer.h 6077 2011-04-28 19:12:15Z abehm $ 

    Copyright (C) 2010 by The Regents of the University of California
 	
    Redistribution of this file is permitted under the terms of the BSD license.

    Author: Shengyue Ji <shengyuj (at) ics.uci.edu>
    Date: 05/11/2007

*/

#ifndef _looptimer_h_
#define _looptimer_h_


#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string>

using namespace std;

class LoopTimer
{
private:
    string desc;
    unsigned count;
    unsigned step;
    struct timespec startTime;
    struct timespec prevTime;
    struct timespec currTime;
    struct timespec sumTime;
    bool running;

    void print();
    
public:
    FILE* file;
    
    LoopTimer();
    void begin(const string &desc, unsigned count);
    void next(unsigned step = 1, unsigned count = 0);
    void pause();
    void resume();
    void end();
    unsigned totalTime();
    float avgTime();
};

#endif

