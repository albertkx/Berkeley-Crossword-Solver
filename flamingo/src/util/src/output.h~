/*
  $Id: output.h 5720 2010-09-09 05:42:48Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _output_h_
#define _output_h_

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "common/src/tr1_local.h"

using namespace std;
using namespace tr1;

template <typename T> 
void writeBin(const vector<T> &data, const string &filenameData)
{
  ofstream fileData(filenameData.c_str(), ios::out | ios::binary);
  if (!fileData) {
    cerr << "can't open output file \"" << filenameData << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  cerr << "writing \"" << filenameData << "\"...";
  cerr.flush();

  for (typename vector<T>::const_iterator dat = data.begin(); dat != data.end();
       ++dat)
    fileData.write(reinterpret_cast<const char*>(&*dat), sizeof(T));
  
  fileData.close();

  cerr << "OK" << endl;
}

void writeString(const vector<string> &data, const string &filenameData);

template <class T>
ostream& operator<<(ostream& out, const vector<T> &v) 
{
  out << '[';
  for(typename vector<T>::const_iterator it = v.begin(); it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ']';

  return out;
}

template <class T>
ostream& operator<<(ostream& out, const multiset<T> &v) 
{
  out << '(';
  for(typename multiset<T>::const_iterator it = v.begin();
      it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}

template <class T, class V>
ostream& operator<<(ostream& out, const set<T, V> &v) 
{
  out << '(';
  for(typename set<T, V>::const_iterator it = v.begin(); it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}

template <class T>
ostream& operator<<(ostream& out, const unordered_set<T> &v) 
{
  out << '(';
  for(typename unordered_set<T>::const_iterator it = v.begin();
      it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}

template <class T, class V, class W>
ostream& operator<<(ostream& out, const map<T, V, W> &v) 
{
  out << '(';
  for(typename map<T, V, W>::const_iterator it = v.begin(); it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}

template <class T, class V>
ostream& operator<<(ostream& out, const multimap<T, V> &v) 
{
  out << '(';
  for(typename multimap<T, V>::const_iterator it = v.begin(); it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}

template <class T, class V>
ostream& operator<<(ostream& out, const unordered_map<T, V> &v) 
{
  out << '(';
  for(typename unordered_map<T, V>::const_iterator it = v.begin();
      it != v.end(); it++) {
    if (it != v.begin()) {
      out << ", ";
    }
    out << *it;
  }
  out << ')';

  return out;
}


template <class T, class V>
ostream& operator<<(ostream& out, const pair<T, V> &p) 
{
  return out << '(' << p.first << ", " << p.second << ')';
}

template <class T>
void output(ostream& out, const T *const vect, unsigned size) 
{
  out << '[';
  for(unsigned i = 0; i < size; i++) {
    out << vect[i];
    if (i < size - 1)
      out << ", ";
  }
  out << ']' << endl;
}

#endif

