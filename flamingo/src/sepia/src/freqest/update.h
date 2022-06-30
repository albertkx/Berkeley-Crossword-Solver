/*
  $Id: update.h 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/23/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <iostream>
#include <vector>

using namespace std;

class Update 
{
public:
  unsigned id;
  string str;
  
  Update() {}
  Update(unsigned id): id(id) {}
  Update(unsigned id, string str): id(id), str(str) {}
  
  friend ostream& operator<<(ostream &out, const Update &u);
  friend istream& operator>>(istream &in, Update &u);
};

typedef vector<Update> ContUpdates;

ostream& operator<<(ostream &out, const ContUpdates &c);
istream& operator>>(istream &in, ContUpdates &c);
