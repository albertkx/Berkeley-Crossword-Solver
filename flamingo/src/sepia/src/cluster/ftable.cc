/*
  $Id: ftable.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 05/20/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ftable.h"

FTable::FTable(const FTable &t) 
{
  cont = new ContFTable();
  *cont = *t.cont;
}

FTable& FTable::operator=(const FTable &t)
{
  if (this == &t)
    return *this;
  *cont = *t.cont;
  return *this;
}

bool FTable::operator==(const FTable &t) const
{
  if (this == &t)
    return true;
  if (*cont == *t.cont)
    return true;
  return false;
}

void FTable::erase(SimVect vect)
{
  if (cont->find(vect) != cont->end()) 
    {
      if ((*cont)[vect] != 0) 
        {
          (*cont)[vect]--;
        }
      if ((*cont)[vect] == 0) 
        {
          cont->erase(vect);
        }
    }
}

ostream& operator<<(ostream &out, const FTable &t)
{
  out << t.cont->size() << endl;
  for (ContFTable::const_iterator it = t.cont->begin(); it != t.cont->end(); ++it) 
    {
      out << it->first << "\t" << it->second << endl;
    }
  return out;
}

istream& operator>>(istream &in, FTable &t)
{
  unsigned sz;
  in >> sz;
  for (unsigned i = 0; i < sz; i++) 
    {
      SimVect vect;
      unsigned cnt;
      in >> vect >> cnt;
      (*t.cont)[vect] = cnt;
    }
  return in;
}
