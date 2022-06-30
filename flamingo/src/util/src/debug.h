/*
  $Id: debug.h 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the 
  BSD License.

  Date: 02/28/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _debug_h_
#define _debug_h_

#ifdef NDEBUG
#undef DEBUG
#undef DEBUG_L2
#undef DEBUG_TIMER
#undef DEBUG_TIMER_FANCY
#undef DEBUG_STAT
#endif

#ifdef DEBUG
#define OUTPUT(name, val) std::cerr << (name) << ": " << (val) << std::endl
#define OUTPUT_VEC(name, vect, size) \
  std::cerr << (name) << ": "; output(std::cerr, (vect), (size))
#else
#define OUTPUT(name, val)
#define OUTPUT_VEC(name, vect, size)
#endif

#ifdef DEBUG_L2
#define OUTPUT_L2(name, val) std::cerr << (name) << ": " << (val) << std::endl
#define OUTPUT_VEC_L2(name, vect, size) \
  std::cerr << (name) << ": "; output(std::cerr, (vect), (size))
#else
#define OUTPUT_L2(name, val)
#define OUTPUT_VEC_L2(name, vect, size)
#endif

#ifdef DEBUG_TIMER
#include <boost/progress.hpp>
#define TIMER_START(str, cnt) boost::progress_display loopTimer(cnt)
#define TIMER_STEP() ++loopTimer
#define TIMER_STOP()
#else
#ifdef DEBUG_TIMER_FANCY
#include "looptimer.h"
#define TIMER_START(str, cnt) LoopTimer loopTimer; loopTimer.begin(str, cnt)
#define TIMER_STEP() loopTimer.next()
#define TIMER_STOP() loopTimer.end()
#else
#define TIMER_START(str, cnt)
#define TIMER_STEP()
#define TIMER_STOP()
#endif
#endif

#ifdef DEBUG_STAT
#define STAT_RESET(var) var = 0
#define STAT_INC(var) var++
#define STAT_ADD(var, val) var += val
#define STAT_OUTPUT(val1, val2) cout << val1 << ' ' << val2 << std::endl
#else
#define STAT_RESET(var)
#define STAT_INC(var)
#define STAT_ADD(var, val)
#define STAT_OUTPUT(val1, val2)
#endif

#ifdef DEBUG_IO
#define READING() std::cerr << "reading...";    \
  std::cerr.flush()
#define READING_FILE(fname) std::cerr << "reading \"" << fname << "\"..."; \
  std::cerr.flush()
#define READING_DONE() std::cerr << "OK" << std::endl
#define WRITING() std::cerr << "writing...";    \
  std::cerr.flush()
#define WRITING_FILE(fname) std::cerr << "writing \"" << fname << "\"..."; \
  std::cerr.flush()
#define WRITING_DONE() std::cerr << "OK" << std::endl
#else
#define READING()
#define READING_FILE(fname)
#define READING_DONE()
#define WRITING()
#define WRITING_FILE(fname)
#define WRITING_DONE()
#endif

#endif
