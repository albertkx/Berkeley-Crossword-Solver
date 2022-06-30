/*
  $Id: array.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD license.

  Date: 05/16/2007
  Author: Chen Li <chenli (at) ics.uci.edu>
	  Bin Wang <binwang (at) mail.neu.edu.cn>
*/

#include <iostream>
#include <string>

#include "array.h"

void arrayIntersection(Array<unsigned> *& array1, Array<unsigned> *& array2, Array<unsigned> *& res)
{
  if( array1 == NULL || array2 == NULL ){
	res = NULL;
	return;
  }
  if ( (array1->size() == 0) || (array2->size() == 0) ) {
	res = NULL;
	return;
  }

  if (res->size() >0) {
	cout << "wrong initionlized intersect array... " << endl;
  	return;
  }

  unsigned len1 = array1->size();
  unsigned len2 = array2->size();

  unsigned item1 = array1->at(0);
  unsigned item2 = array2->at(0);

  for( unsigned i=0, j=0; i<len1 && j<len2;)
  {
    if (item1 == item2){
      res->append(item1);
      if (i<len1-1) item1 = array1->at(1+i);
      if (j<len2-1) item2 = array2->at(1+j);
      i++;j++;
    }else if(item1 < item2){

      if (i<len1-1)item1 = array1->at(1+i); i++;
    }else { 

	if (j<len2-1) item2 = array2->at(1+j); j++;}
  }
}

/*
 * srcArray = arcArray - subArray
 */
void arrayDifference(Array<unsigned> * srcArray, Array<unsigned> *& subArray)
{
  if( srcArray == NULL || subArray == NULL ){
	return;
  }
  if( srcArray->size() == 0 || subArray->size() == 0 ){
	return;
  }

/*
unsigned k=0;
if ( (srcArray->at(0) == 33) && (subArray->at(0) == 5549) ){
	for (k=0; k<srcArray->size(); k++) 
		cout << " " << srcArray->at(k) ;   
	cout << endl;
	for (k=0; k<subArray->size(); k++) 
		cout << " " << subArray->at(k) ;   
	cout << endl;
}	
*/

  unsigned len1 = srcArray->size();
  unsigned len2 = subArray->size();

  unsigned item1 = srcArray->at(len1-1);
  unsigned item2 = subArray->at(len2-1);

  for( int i=len1-1, j=len2-1; i>=0 && j>=0;)
  {
    if (item1 == item2) {
        if (i>0) item1 = srcArray->at(i-1);
        if (j>0) item2 = subArray->at(j-1);
        srcArray->erase(i);
        i--;j--;
    }
    else if(item1 < item2)  {
        if (j>0) item2 = subArray->at(j-1);
        j--;
    }
    else { 
	if (i>0) item1 = srcArray->at(i-1); 
	i--;}
  }

//cout << "srcArray->size = " << srcArray->size() << endl;
}

void arrayDifference(Array<unsigned> * srcArray, Array<unsigned> * subArray, Array<unsigned> *& resultArray)
{
  if( srcArray == NULL || subArray == NULL ){
	return;
  }

  if( srcArray->size() == 0 || subArray->size() == 0 ){
	return;
  }

  int len1 = srcArray->size();
  int len2 = subArray->size();

  int i = 0;
  int j = 0;
  int item1 = srcArray->at(i);
  int item2 = subArray->at(j);

  while (true) {
    if (item1 == item2) {
      i ++;  
      j ++;
      if (i < len1) item1 = srcArray->at(i); 
      if (j < len2) item2 = subArray->at(j); 
   }
   else if(item1 < item2)  { // append the item from the first array
	resultArray->append(item1);
	i ++;
        if (i < len1) item1 = srcArray->at(i); 
    }

  else  { // ignore  the item from the second array
	j ++;
        if (j < len2) item2 = subArray->at(j); 
    }

    if ( i >= len1 )
	break; // finished first array first
      if ( j >= len2 )
	break; // finished second array first
   }

   // append the remaining items from the first array to the result
   for (; i < len1; i++)
     resultArray->append(srcArray->at(i));

/*    unsigned len1 = srcArray->size();
  unsigned len2 = subArray->size();

  unsigned item1 = srcArray->at(0);
  unsigned item2 = subArray->at(0);

  for( int i=0, j=0; i<(int)len1;)
  {
    if (item1 == item2) {
        if (i<(int)len1) item1 = srcArray->at(++i);
        if (j<(int)len2) item2 = subArray->at(++j);
    }
    else if(item1 < item2)  {
	resultArray->append(item1);
        if (i<(int)len1) item1 = srcArray->at(++i);
    }
    else { 
	if (j<(int)len2) item2 = subArray->at(++j); 
	else { resultArray->append(item1); item1 = srcArray->at(++i);}
    }
  }*/
}

/*
 * srcArray = arcArray \cap otherArray
 */
void intersect(Array<unsigned> * srcArray, Array<unsigned> *& otherArray)
{
  if( srcArray == NULL || otherArray == NULL )
	return;

  if ( (srcArray->size() == 0) || (otherArray->size() == 0) ) 
	return;

  unsigned len1 = srcArray->size();
  unsigned len2 = otherArray->size();
/*
cout << "=== srcArray " << endl;
for (unsigned i=0; i<len1; i++) {
	cout << " " << srcArray->at(i);
}
cout << endl;
cout << "=== otherArray " << endl;
for (unsigned i=0; i<len2; i++) {
	cout << " " << otherArray->at(i);
}
cout << endl;
*/
  unsigned item1 = srcArray->at(len1-1);
  unsigned item2 = otherArray->at(len2-1);

  for( int i=len1-1, j=len2-1; i>=0 && j>=0;)
  {
    if (item1 == item2) {
        if (i>0) item1 = srcArray->at(i-1);
        if (j>0) item2 = otherArray->at(j-1);
        i--;j--;
    }
    else if(item1 < item2)  {
        if (j>0) item2 = otherArray->at(j-1);
        j--;
    }
    else { 
	if (i>0) item1 = srcArray->at(i-1); 
	srcArray->erase(i); 
	i--;}
  }
/*
//cout << "srcArray->size = " << srcArray->size() << endl;
cout << "=== after intersect: srcArray " << endl;
for (unsigned i=0; i<srcArray->size(); i++) {
	cout << " " << srcArray->at(i);
}
cout << endl;
*/
}

void arrayUnion(Array<unsigned> * srcArray, Array<unsigned> * otherArray, Array<unsigned> *& resultArray)
{
  if( srcArray == NULL && otherArray == NULL )
	return;

  if ( (srcArray->size() == 0) && (otherArray->size() == 0) ) 
	return;

  unsigned len1 = srcArray->size();
  unsigned len2 = otherArray->size();

  unsigned item1 = srcArray->at(0);
  unsigned item2 = otherArray->at(0);

  for( unsigned i=0, j=0; i<len1 || j<len2;)
  {
    if (item1 == item2) {
	resultArray->append(item1);
        if (i<len1) item1 = srcArray->at(++i);
        if (j<len2) item2 = otherArray->at(++j);
    }
    else if(item1 < item2)  {
	
        if (i<len1) { 
		resultArray->append(item1);
		item1 = srcArray->at(++i);
	}else {resultArray->append(otherArray->at(j));++j;}
    }
    else { 
	if (j<len2) {
		resultArray->append(item2);		
		item2 = otherArray->at(++j); 
	}else{resultArray->append(srcArray->at(i));++i;}
    }
  }
}

