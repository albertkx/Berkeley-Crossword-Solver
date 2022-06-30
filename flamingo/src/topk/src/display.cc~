/*
  $Id: display.cc 5772 2010-10-19 07:15:28Z abehm $

  Copyright (C) 2007 by The Regents of the University of California
 
  Redistribution of this file is permitted under the terms of the BSD
  license

  Date: 08/19/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "display.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

void DisplayLists::init(
  const list<range<uint *> > &initialLists, 
  const Query &query) 
{  
  g = query.sim.gramGen.getGramLength();

  query.sim.gramGen.decompose(query.str, gs, '#', '$');
  gn = gs.size();

  string q(query.str);
  if (query.sim.gramGen.prePost)
    q = string(g - 1, 156) + query.str + string(g - 1, 190);
  set<uint> gramsset, gramsetFinal;
  query.sim.gramGen.decompose(query.str, gramsetFinal);
  longest = 0;
  for (uint i = 0; i < gn; ++i) {
    string gram(q.substr(i, g));
    uint h = tr1::hash<string>()(gram);      
    if (gramsset.find(h) == gramsset.end()) {
      gramsset.insert(h);
      list<range<uint*> >::const_iterator l = initialLists.begin();
      for(set<uint>::const_iterator s = gramsetFinal.begin();
          s != gramsetFinal.end(); ++s, ++l)
        if (*s == h) {
          lists.push_back(DisplayListsInfo(*l));
          uint lg = l->_end - l->_begin;
          if (lg > longest)
            longest = lg;
          break;
        }
    }
  }
}

void DisplayLists::show(const list<range<uint *> > &crt)
{
  stringstream out;
  // top separator
  for (uint i = 0; i < gn; ++i) {
    if (i)
      out << ' ';
    out << " |" << string(g, '-') << '|';
  }
  out << endl;
  // header
  for (uint i = 0; i < gn; ++i) {
    if (i)
      out << ' ';
    out << " |\033[1m" << gs[i] << "\033[0m|";
  }
  out << endl;
  // ids
  for (list<DisplayListsInfo>::iterator l = lists.begin(); 
       l != lists.end(); ++l)
    for (list<range<uint*> >::const_iterator c = crt.begin(); 
         c != crt.end(); ++c)
      if (l->r._end == c->_end) {
        l->i = l->r._begin;
        if (l->c != c->_begin) {   // updated
          l->p = l->c;
          l->c = c->_begin;
          l->u = true;
        }
        l->e = false;
        break;
      }               
  for (list<DisplayListsInfo>::iterator l = lists.begin(); 
       l != lists.end(); ++l) {
    if (l->e) {                 // no longer present
      l->i = l->r._begin;
      l->e = false;
      if (l->c != l->r._end) {   // just deleted
        l->p = l->c;
        l->c = l->r._end;
        l->u = true;
      }
    }  
    if (l != lists.begin())
      out << ' ';
    out << (l->c == l->r._begin ? "\033[0;36m/\033[0;37m" : " ") 
        << '|' << string(g, '-') << '|';
  }
  out << endl;
  for (uint i = 0; i < longest + 1; ++i) {
    for (list<DisplayListsInfo>::iterator l = lists.begin(); 
         l != lists.end(); ++l) {
      if (l != lists.begin())
        out << ' ';
      if (!l->e) {
        if (l->u)
          out << "\033[1;31m";
        else
          out << "\033[0;36m";
        out << (l->i >= l->p ? 
                (l->i <= l->c ?  
                 (l->i == l->c ? '\\' :
                  (l->i == l->p ? '/' : '|')) : ' ') : ' ');
        out << "\033[0;37m";
        out << '|';
        if (l->i != l->r._end) {
          out << (l->i > l->p && l->i < l->c ? "\033[0;34m" : "")
              << setw(g) << *l->i
              << (l->i > l->p && l->i < l->c ? "\033[0;37m" : "");
          ++l->i;
        }
        else {
          out << string(g, '-');
          l->e = true;
          l->u = false;
        }
        out << '|';
      } 
      else
        out << string(g + 3, ' ');
    }
    out << endl;
  }
  out << "Press ENTER to continue...";
  cout << out.str();
  getchar();
  cout << endl;
}
