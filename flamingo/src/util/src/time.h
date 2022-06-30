/*
  $Id: time.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license.

  Date: 09/18/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _time_h_
#define _time_h_

#include <sys/time.h>

#include "looptimer.h"

// -- - time measure - --
#define TIME_START struct timeval tStart, tEnd; \
  float _tms; \
  gettimeofday(&tStart, 0)

#define TIME_RESET gettimeofday(&tStart, 0)

#define TIME_END gettimeofday(&tEnd, 0); \
  _tms = (tEnd.tv_sec  - tStart.tv_sec ) * 1000 +  \
  (tEnd.tv_usec - tStart.tv_usec) / 1000.

// -- - progress indicator - --
#define PROGRESS_START(str, cnt) LoopTimer loopTimer; loopTimer.begin(str, cnt)
#define PROGRESS_STEP loopTimer.next()
#define PROGRESS_STOP loopTimer.end()

#endif
