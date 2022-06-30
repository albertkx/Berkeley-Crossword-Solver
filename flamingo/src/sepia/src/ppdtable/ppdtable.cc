/*
  $Id: ppdtable.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ppdtable.h"

PPDTable::PPDTable(const PPDTable &t) 
{
  table = new ContPPDEntry;
  *table = *t.table;
}

PPDTable& PPDTable::operator=(const PPDTable &t) 
{
  if (this == &t)
    return *this;
  *table = *t.table;
  return *this;
}

bool PPDTable::operator==(const PPDTable &t) const 
{
  if (this == &t)
    return true;
  if (*table == *t.table)
    return true;
  return false;
}

ostream& operator<<(ostream &out, const PPDTable &t) 
{
  return out << *t.table;
}

istream& operator>>(istream &in, PPDTable &t) 
{
  return in >> *t.table;
}

void PPDTable::insert(const PPDEntry &e) 
{
  PPDEntry eMin = PPDEntry(e.vect1, e.vect2, SIM_TYPE_MIN, 0, 0);
  PPDEntry eMax = PPDEntry(e.vect1, e.vect2, SIM_TYPE_MAX, 0, 0);

  PPDEntry eNew = e; eNew.countE = 0;
  // the element that will be added

  ContPPDEntryIt itMin = table->lower_bound(eMin);
  ContPPDEntryIt itMax = table->upper_bound(eMax);

  // between itMin and itMax are all the elements with the same vect1 and vect2
  unsigned a = 0;
  // "a" holds the total number of occurences of pair (v1, v2)
  if (itMin != itMax) { // there is at lease one pair with (v1, v2)
    ContPPDEntryIt itLast = itMax; --itLast;
    a = itLast->countE;

    ContPPDEntryIt itPrev = table->lower_bound(e);
    //--itPrev;

    // !!! PPDTable assumes that ContPPDEntry is an ordered container !!!
    if (itPrev->vect1 == eNew.vect1 && itPrev->vect2 == eNew.vect2)
      // there is an entry (v1, v2, d) with d >= eNew.d
      eNew.countE = itPrev->countE;
  }

  // add new element (if necessary)
  if (table->find(eNew) == table->end()) {
    bool doInsert = false;
    if (eNew.distM > DIST_THRESHOLD)
      // if eNew.distM > itLast->distM
      //   insert eNew
      if (itMin != itMax) {
        ContPPDEntryIt itLast = itMax; --itLast;
        if (e.distM > itLast->distM) {
          table->erase(itLast);
          doInsert = true;
        }
      } else
        doInsert = true;
    else
      doInsert = true;
    if (doInsert) {
      table->insert(eNew);
      // if the element is added, the iterators need to be updated
      itMin = table->lower_bound(eMin);
      itMax = table->upper_bound(eMax);
    }
  }

  // update countE and fract of all the elements with (vect1, vect2)
  a++;
  ContPPDEntry tableNew;
  for (ContPPDEntryIt it = itMin; it != itMax; ++it) {
    PPDEntry eTmp = *it;
    if (eTmp.distM >= e.distM)
      eTmp.countE+=e.countE;
    eTmp.fract = static_cast<float>(eTmp.countE)/a;
    tableNew.insert(eTmp);
  }
  table->erase(itMin, itMax);
  table->insert(tableNew.begin(), tableNew.end());
}

void PPDTable::erase(const PPDEntry &e) 
{
  ContPPDEntryIt it = table->find(e);
  if (it == table->end()) {
    cerr << "PPDTable::erase entry not found\t" << e << endl;
    return;
  }

  if (it->countE == 0) {
    cerr << "PPDTable::erase entry has cont 0\t" << e << endl;
    return;
  }

  PPDEntry eMin = PPDEntry(e.vect1, e.vect2, SIM_TYPE_MIN, 0, 0);
  PPDEntry eMax = PPDEntry(e.vect1, e.vect2, SIM_TYPE_MAX, 0, 0);

  ContPPDEntryIt itMin = table->lower_bound(eMin);
  ContPPDEntryIt itMax = table->upper_bound(eMax);

  // between itMin and itMax are all the elements with the same vect1 and vect2
  unsigned a = 0;
  // "a" holds the total number of occurences of pair (v1, v2)
  if (itMin == itMax) {
    cerr << "PPDTable::erase entry range not found\t" << e << endl;
    return;
  }  
  // there is at lease one pair with (v1, v2)
  ContPPDEntryIt itLast = itMax; --itLast;
  a = itLast->countE;

  // update countE and fract of all the elements with (vect1, vect2)
  a--;
  ContPPDEntry tableNew;
  for (ContPPDEntryIt it = itMin; it != itMax; ++it) {
    PPDEntry eTmp = *it;
    if (eTmp.distM >= e.distM)
      eTmp.countE -= e.countE;
    eTmp.fract = static_cast<float>(eTmp.countE) / a;
    if (eTmp.countE != 0)
      tableNew.insert(eTmp);
  }
  table->erase(itMin, itMax);
  table->insert(tableNew.begin(), tableNew.end());
}

void PPDTable::pruneMax() 
{
  for (ContPPDEntryIt it = table->begin(); it != table->end();)
    if (it->distM > DIST_THRESHOLD) {
      ContPPDEntryIt er = it;
      it++;
      // !!! PPDTable assumes that ContPPDEntry is a container where
      // remove does not invalidate iterators !!!
      table->erase(er);
    } else
      it++;
}

#if SIM_DIST == 1 && SIM_VECT == 1

void PPDTable::serialize(ofstream &out)
{
  unsigned sz = static_cast<unsigned>(table->size());
  out.write(reinterpret_cast<char *>(&sz), sizeof(unsigned));
  for(ContPPDEntryIt it = table->begin(); it != table->end(); it++)
    it->serialize(out);
}

void PPDTable::deserialize(ifstream &in)
{
  unsigned sz;
  in.read(reinterpret_cast<char *>(&sz), sizeof(unsigned));
  for (unsigned i = 0; i < sz; i++) {
    PPDEntry e;
    e.deserialize(in);
    table->insert(e);
  }
}

#endif
