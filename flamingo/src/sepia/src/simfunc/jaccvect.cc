/*
  $Id: jaccvect.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "jaccvect.h"
#include "util/src/simfuncs.h"
#include "util/src/gram.h"

#include <algorithm>

using namespace std;

JaccVect::JaccVect(const string &s1, const string &s2): inte(0)
{
  unsigned
    n = s1.length(), 
    m = s1.length();

  if (n == 0 || m == 0) {
    if (n==0) {
      this->unio = m < Q_GRAM ? 1 : m - Q_GRAM + 1;
      this->edit = m;
    } else {
      this->unio = n < Q_GRAM ? 1 : n - Q_GRAM + 1;
      this->edit = n;
    }
    return;
  }

  set<string> s1Gram, s2Gram, sInt, sUni;
  str2grams(s1, s1Gram, Q_GRAM);
  str2grams(s2, s2Gram, Q_GRAM);

  set_intersection(s1Gram.begin(), s1Gram.end(),
                   s2Gram.begin(), s2Gram.end(), 
                   inserter(sInt, sInt.begin()));

  set_union(s1Gram.begin(), s1Gram.end(),
            s2Gram.begin(), s2Gram.end(), 
            inserter(sUni, sUni.begin()));

	this->inte = sInt.size();
	this->unio = sUni.size();
	this->edit = ed(s1, s2);
}

ostream& operator<<(ostream &out, const JaccVect &jv)
{
  return out << jv.inte << ' ' << jv.unio << ' ' << jv.edit;
}

istream& operator >> (istream &in, JaccVect &jv)
{
  return in >> jv.inte >> jv.unio >> jv.edit;
}

bool operator<(const JaccVect &left, const JaccVect &right)
{
  if (left.inte != right.inte)
    return left.inte < right.inte;
  if (left.unio != right.unio)
    return left.unio < right.unio;
  return left.edit < right.edit;
}

bool operator!=(const JaccVect &left, const JaccVect &right)
{
  return left < right || right < left;
}

ostream& JaccVect::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Jacc Distance" << endl;
  out << "Jacc Vector" << endl;
  out << "QGram\t" << Q_GRAM << endl;
  return out << endl;
}
