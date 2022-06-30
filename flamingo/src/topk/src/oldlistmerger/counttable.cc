/*  
 $Id: counttable.cc 5716 2010-09-09 04:27:56Z abehm $ 

 Copyright (C) 2010 by The Regents of the University of California
 	
 Redistribution of this file is permitted under the terms of 
 the BSD license.

 Author: Jiaheng Lu and Yiming Lu
 Date: 05/21/2007
*/

#include <fstream>
#include <iostream>

#include "counttable.h"
 	
using namespace std;

CountTable::~CountTable(){};

void CountTable::addCount(unsigned queryId) 
{
  if ( this->queryId == queryId )
    {
      ++stringCount;
    }
  //Otherwise reset the count to 0
  else 
    {
      this->queryId = queryId;
      stringCount =1;
    }
  
}//end addCount


void CountTable::markCount()
{
    
      stringCount = MAXCOUNT;

}//end markCount

unsigned CountTable::getCount()
{
    
      return stringCount;

}//end getCount

bool CountTable::isMarked()
{
  if (stringCount < MAXCOUNT)
    return false;
  else
    return true;

}//end isMarked




