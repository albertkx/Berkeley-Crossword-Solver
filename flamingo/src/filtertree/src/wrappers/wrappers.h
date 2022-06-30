/*
  $Id: wrappers.h 6079 2011-04-30 02:59:54Z abehm $

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD License.
    
  Date: 09/06/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
*/

#ifndef _wrappers_h_
#define _wrappers_h_

#include "wrappersimple.h"
#include "wrapperdiscardlists.h"
#include "wrappercombinelists.h"
#include "wrapperondisk.h"

// wrappers for uncompressed indexers
typedef WrapperSimple<SimMetricEd> WrapperSimpleEd;
typedef WrapperSimple<SimMetricEdNorm> WrapperSimpleEdNorm;
typedef WrapperSimple<SimMetricEdSwap> WrapperSimpleEdSwap;
typedef WrapperSimple<SimMetricJacc> WrapperSimpleJacc;
typedef WrapperSimple<SimMetricCos> WrapperSimpleCos;
typedef WrapperSimple<SimMetricDice> WrapperSimpleDice;

typedef WrapperSimple<SimMetricJaccBag> WrapperSimpleJaccBag;
typedef WrapperSimple<SimMetricCosBag> WrapperSimpleCosBag;
typedef WrapperSimple<SimMetricDiceBag> WrapperSimpleDiceBag;

typedef WrapperShortSimple<SimMetricEd> WrapperShortSimpleEd;
typedef WrapperShortSimple<SimMetricEdNorm> WrapperShortSimpleEdNorm;
typedef WrapperShortSimple<SimMetricJacc> WrapperShortSimpleJacc;
typedef WrapperShortSimple<SimMetricCos> WrapperShortSimpleCos;
typedef WrapperShortSimple<SimMetricDice> WrapperShortSimpleDice;

// wrappers for indexers compressed with global holes using long-list-first (workload independent)
typedef WrapperDiscardListsLLF<SimMetricEd> WrapperDiscardListsLLFEd;
typedef WrapperDiscardListsLLF<SimMetricEdNorm> WrapperDiscardListsLLFEdNorm;
typedef WrapperDiscardListsLLF<SimMetricJacc> WrapperDiscardListsLLFJacc;
typedef WrapperDiscardListsLLF<SimMetricCos> WrapperDiscardListsLLFCos;
typedef WrapperDiscardListsLLF<SimMetricDice> WrapperDiscardListsLLFDice;

// wrappers for indexers compressed with global holes using short-list-first (workload independent)
typedef WrapperDiscardListsSLF<SimMetricEd> WrapperDiscardListsSLFEd;
typedef WrapperDiscardListsSLF<SimMetricEdNorm> WrapperDiscardListsSLFEdNorm;
typedef WrapperDiscardListsSLF<SimMetricJacc> WrapperDiscardListsSLFJacc;
typedef WrapperDiscardListsSLF<SimMetricCos> WrapperDiscardListsSLFCos;
typedef WrapperDiscardListsSLF<SimMetricDice> WrapperDiscardListsSLFDice;

// wrappers for indexers compressed with global holes using random-list (workload independent)
typedef WrapperDiscardListsRandom<SimMetricEd> WrapperDiscardListsRandomEd;
typedef WrapperDiscardListsRandom<SimMetricEdNorm> WrapperDiscardListsRandomEdNorm;
typedef WrapperDiscardListsRandom<SimMetricJacc> WrapperDiscardListsRandomJacc;
typedef WrapperDiscardListsRandom<SimMetricCos> WrapperDiscardListsRandomCos;
typedef WrapperDiscardListsRandom<SimMetricDice> WrapperDiscardListsRandomDice;

// wrappers for indexers compressed with global holes using panic-cost-plus (workload dependent)
typedef WrapperDiscardListsPanicCost<SimMetricEd> WrapperDiscardListsPanicCostEd;
typedef WrapperDiscardListsPanicCost<SimMetricEdNorm> WrapperDiscardListsPanicCostEdNorm;
typedef WrapperDiscardListsPanicCost<SimMetricJacc> WrapperDiscardListsPanicCostJacc;
typedef WrapperDiscardListsPanicCost<SimMetricCos> WrapperDiscardListsPanicCostCos;
typedef WrapperDiscardListsPanicCost<SimMetricDice> WrapperDiscardListsPanicCostDice;

// wrappers for indexers compressed with global holes using time-cost-plus (workload dependent)
typedef WrapperDiscardListsTimeCost<SimMetricEd> WrapperDiscardListsTimeCostEd;
typedef WrapperDiscardListsTimeCost<SimMetricEdNorm> WrapperDiscardListsTimeCostEdNorm;
typedef WrapperDiscardListsTimeCost<SimMetricJacc> WrapperDiscardListsTimeCostJacc;
typedef WrapperDiscardListsTimeCost<SimMetricCos> WrapperDiscardListsTimeCostCos;
typedef WrapperDiscardListsTimeCost<SimMetricDice> WrapperDiscardListsTimeCostDice;

// wrappers for indexers compressed with global union using the basic algorithm (workload independent)
typedef WrapperCombineListsBasic<SimMetricEd> WrapperCombineListsBasicEd;
typedef WrapperCombineListsBasic<SimMetricEdNorm> WrapperCombineListsBasicEdNorm;
typedef WrapperCombineListsBasic<SimMetricJacc> WrapperCombineListsBasicJacc;
typedef WrapperCombineListsBasic<SimMetricCos> WrapperCombineListsBasicCos;
typedef WrapperCombineListsBasic<SimMetricDice> WrapperCombineListsBasicDice;

// wrappers for indexers compressed with global union using the cost-based algorithm (workload dependent)
typedef WrapperCombineListsBasic<SimMetricEd> WrapperCombineListsBasicEd;
typedef WrapperCombineListsBasic<SimMetricEdNorm> WrapperCombineListsBasicEdNorm;
typedef WrapperCombineListsBasic<SimMetricJacc> WrapperCombineListsBasicJacc;
typedef WrapperCombineListsBasic<SimMetricCos> WrapperCombineListsBasicCos;
typedef WrapperCombineListsBasic<SimMetricDice> WrapperCombineListsBasicDice;

// wrappers for disk-based indexers with simple merging algorithm
typedef WrapperOnDiskSimple<SimMetricEd> WrapperOnDiskSimpleEd;
typedef WrapperOnDiskSimple<SimMetricEdNorm> WrapperOnDiskSimpleEdNorm;
typedef WrapperOnDiskSimple<SimMetricJacc> WrapperOnDiskSimpleJacc;
typedef WrapperOnDiskSimple<SimMetricCos> WrapperOnDiskSimpleCos;
typedef WrapperOnDiskSimple<SimMetricDice> WrapperOnDiskSimpleDice;

// wrappers for disk-based indexers with adaptive merging algorithm
typedef WrapperOnDiskAdapt<SimMetricEd> WrapperOnDiskAdaptEd;
typedef WrapperOnDiskAdapt<SimMetricEdNorm> WrapperOnDiskAdaptEdNorm;
typedef WrapperOnDiskAdapt<SimMetricJacc> WrapperOnDiskAdaptJacc;
typedef WrapperOnDiskAdapt<SimMetricCos> WrapperOnDiskAdaptCos;
typedef WrapperOnDiskAdapt<SimMetricDice> WrapperOnDiskAdaptDice;

#endif
