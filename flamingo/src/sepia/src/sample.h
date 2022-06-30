/*
  $Id: sample.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/04/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _sample_h_
#define _sample_h_

#include <string>
#include <vector>

using namespace std;

class Sample
{
private:
  vector<unsigned> sample;
  vector<unsigned> index;
  unsigned count;
  
public:
  Sample();
  Sample(const unsigned max);
  Sample(const unsigned number, const unsigned max);
  Sample(const string &fileName);

  vector<unsigned>::const_iterator begin() const;
  vector<unsigned>::const_iterator end() const;  

  unsigned operator[](const unsigned idx) const;
  void push_back(const unsigned val);
  vector<unsigned>::size_type size() const;
  unsigned getCount() const;

  unsigned generate();
  bool hasNext() const;

  void read(const string &fileName);
  void write(const string &fileName);

private:
  void initIndex(const unsigned max);
  void initRand() const;

  friend ostream& operator<<(ostream& out, const Sample& s);
  friend istream& operator>>(istream& in, Sample& s);
};

inline vector<unsigned>::const_iterator Sample::begin() const
{
  return sample.begin();
}

inline vector<unsigned>::const_iterator Sample::end() const
{
  return sample.end();
}

inline unsigned Sample::operator[](const unsigned idx) const
{
  return sample[idx];
}

inline vector<unsigned>::size_type Sample::size() const 
{
  return sample.size();
}

inline unsigned Sample::getCount() const 
{
  return count;
}

inline bool Sample::hasNext() const
{
  return index.size() != count;
}

#endif
