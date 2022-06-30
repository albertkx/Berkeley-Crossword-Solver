/*
  $Id: sample.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "sample.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

Sample::Sample(): count(0)
{
}

Sample::Sample(const unsigned max): count(0)
{
  initIndex(max);
  initRand();
}

Sample::Sample(const unsigned no, const unsigned max): count(0)
{  
  if (no < max) 
    {
      initIndex(max);
      initRand();

      sample.reserve(no);
      for (; count < no; count++) 
        {
          unsigned idx = rand() % (max - count);
          unsigned val = index[idx];
          index[idx] = index[max - count - 1];
          sample.push_back(val);
        }
    }
  else
    {
      sample.reserve(no);
      for (; count < no; count++) sample.push_back(count);
    }
}

Sample::Sample(const string &fileName): count(0)
{
  read(fileName);
}

void Sample::initIndex(const unsigned max)
{
  index.reserve(max);
  for (unsigned i = 0; i < max; i++) index.push_back(i);
}

void Sample::initRand() const
{
  time_t ltime;
  time(&ltime);
  srand(static_cast<unsigned int>(ltime));
}

unsigned Sample::generate() 
{
  if (index.size() == count)
    {
      cerr << "no more samples in Sample::generate" << endl;
      exit(1);
    } 
  unsigned idx = rand() % (index.size() - count);
  unsigned val = index[idx];
  index[idx] = index[index.size() - count - 1];
  count++;
  return val;
}

void Sample::push_back(const unsigned val)
{
  sample.push_back(val);
}

void Sample::read(const string &fileName)
{
  cerr << "sample " << fileName << endl; 
  ifstream fin(fileName.c_str());
  if (!fin) {
    cerr << "can't open input file \"" << fileName << "\"" << endl; 
    exit(EXIT_FAILURE); 
  } 
  fin >> *this;
  cerr << "sample read (" << sample.size() << ")" << endl;
}

void Sample::write(const string &fileName)
{
  cerr << "sample " << fileName << endl; 
  ofstream fout(fileName.c_str());
  if (!fout) { 
    cerr << "can't open output file \"" << fileName << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  fout << *this;
  cerr << "sample (" << sample.size() << ") wrote" << endl;
}
      
ostream& operator<<(ostream& out, const Sample& s)
{
  out << s.sample.size() << endl << endl;
  for (vector<unsigned>::const_iterator it = s.sample.begin(); 
       it != s.sample.end(); ++it)
    out << *it << endl;
  return out;
}

istream& operator>>(istream& in, Sample& s) 
{
  unsigned no;
  in >> no;
  s.sample.reserve(no);
  for (; s.count < no; s.count++) 
    {
      unsigned val;
      in >> val;
      s.sample.push_back(val);
    }
  return in;
}
