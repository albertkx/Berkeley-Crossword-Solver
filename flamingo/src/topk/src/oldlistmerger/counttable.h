/* 
$Id: counttable.h 5716 2010-09-09 04:27:56Z abehm $

Copyright (C) 2010 by The Regents of the University of California

Redistribution of this file is permitted under
the terms of the BSD license.

Date: 05/21/2007
Author: Jiaheng Lu, Yiming Lu
*/

 	
#ifndef _counttable_h_
#define _counttable_h_


using namespace std;

class CountTable
{
 public:

  CountTable()
    {
      queryId = 0;
      MAXCOUNT =  999999;
    };//end CountTable

  ~CountTable();

  void  addCount(unsigned queryId); 
  void  markCount();
  unsigned getCount();
  bool isMarked();


 private:
    unsigned  queryId ;
    unsigned  stringCount;
    unsigned  MAXCOUNT;
};



#endif  
