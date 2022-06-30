/*
  $Id: editvect.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "editvect.h"

EditVect::EditVect(const string &a, const string &b): ins(0), del(0), sub(0)
{
  // uses O(m) space instead of O(nm) as the classic algorithm
  unsigned n = static_cast<unsigned>(a.length());
  unsigned m = static_cast<unsigned>(b.length());
  if (n == 0 || m == 0) {
    if (n == 0)
      this->ins = m;
    else
      this->del = n;
    return;
  }
  unsigned i, j, d[n + 1][m + 1], mini;
  for (i = 0; i <= n; i++) d[i][0] = i;
  for (j = 0; j <= m; j++) d[0][j] = j;
  for (i = 1; i <= n; i++)
    for (j = 1; j <= m; j++)
      d[i][j] = (min(min(d[i-1][j  ]+1,
                         d[i  ][j-1]+1),
                     d[i-1][j-1]+
                     (a[i-1] == b[j-1]?0:1)));
  i = n;
  j = m;
  while (!(i == 0 && j == 0))
    if (i == 0 || j == 0) // on the edge - there is only one way from here
      if (i == 0) {
        this->ins++;
        j--;
      } else {
        this->del++;
        i--;
      }
    else {
      mini = min(min(d[i  ][j - 1],
                     d[i - 1][j  ]),
                 d[i - 1][j - 1]);
      if (mini == d[i][j - 1] &&
          d[i][j] == d[i][j - 1] + 1) { // Insertion
        ins++;
        j--;
      } else if (   mini == d[i - 1][j] &&
                    d[i][j] == d[i - 1][j] + 1) {	// Deletion
        del++;
        i--;
      } else { // Substitution
        sub += d[i][j] - d[i-1][j-1];
        i--;
        j--;
      } 
    }
}

void EditVect::serialize(ofstream &out) const
{
  out.write(reinterpret_cast<const char*>(&ins), sizeof(unsigned));
  out.write(reinterpret_cast<const char*>(&del), sizeof(unsigned));
  out.write(reinterpret_cast<const char*>(&sub), sizeof(unsigned));
}

void EditVect::deserialize(ifstream &in)
{
  in.read(reinterpret_cast<char*>(&ins), sizeof(unsigned));
  in.read(reinterpret_cast<char*>(&del), sizeof(unsigned));
  in.read(reinterpret_cast<char*>(&sub), sizeof(unsigned));
}

ostream& EditVect::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Edit Distance" << endl;
  out << "Edit Vector\t< I, D, S >" << endl;
  return out << endl;
}

ostream& operator<<(ostream &out, const EditVect &ev)
{
  return out << ev.ins << ' ' << ev.del << ' ' << ev.sub;
}

istream& operator>>(istream &in, EditVect &ev)
{
  return in >> ev.ins >> ev.del >> ev.sub;
}

bool operator<(const EditVect &left, const EditVect &right)
{
  if (&left == &right)
    return false;
  if (left.ins != right.ins)
    return left.ins < right.ins;
  if (left.del != right.del)
    return left.del < right.del;
  return left.sub < right.sub;
}

bool operator!=(const EditVect &left, const EditVect &right)
{
  return left < right || right < left;
}

EditVectID::EditVectID(const string &a, const string &b)
{
  EditVect ev = EditVect(a, b);
  insDel = ev.ins + ev.del;
  sub = ev.sub;
}

ostream& operator<<(ostream &out, const EditVectID &ev)
{
  return out << ev.insDel << ' ' << ev.sub;
}

istream& operator>>(istream &in, EditVectID &ev)
{
  return in >> ev.insDel >> ev.sub;
}

bool operator<(const EditVectID &left, const EditVectID &right)
{
  if (&left == &right)
    return false;
  if (left.insDel != right.insDel)
    return left.insDel < right.insDel;
  return left.sub < right.sub;
}

bool operator!=(const EditVectID &left, const EditVectID &right)
{
  return left < right || right < left;
}

ostream& EditVectID::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Edit Distance" << endl;
  out << "Edit Vector\t< I + D, S >" << endl;
  return out << endl;
}

EditVectIS::EditVectIS(const string &a, const string &b)
{
  EditVect ev = EditVect(a, b);
  insSub = ev.ins + ev.sub;
  del = ev.del;
}

ostream& operator<<(ostream &out, const EditVectIS &ev)
{
  return out << ev.insSub << ' ' << ev.del;
}

istream& operator>>(istream &in, EditVectIS &ev)
{
  return in >> ev.insSub >> ev.del;
}

bool operator<(const EditVectIS &left, const EditVectIS &right)
{
  if (&left == &right)
    return false;
  if (left.insSub != right.insSub)
    return left.insSub < right.insSub;
  return left.del < right.del;
}

bool operator!=(const EditVectIS &left, const EditVectIS &right)
{
  return left < right || right < left;
}

ostream& EditVectIS::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Edit Distance" << endl;
  out << "Edit Vector\t< I + S, D >" << endl;
  return out << endl;
}

EditVectDS::EditVectDS(const string &a, const string &b) 
{
  EditVect ev = EditVect(a, b);
  delSub = ev.del + ev.sub;
  ins = ev.ins;
}

ostream& operator<<(ostream &out, const EditVectDS &ev)
{
  return out << ev.delSub << ' ' << ev.ins;
}

istream& operator>>(istream &in, EditVectDS &ev)
{
  return in >> ev.delSub >> ev.ins;
}

bool operator<(const EditVectDS &left, const EditVectDS &right)
{
  if (&left == &right)
    return false;
  if (left.delSub != right.delSub)
    return left.delSub < right.delSub;
  return left.ins < right.ins;
}

bool operator!=(const EditVectDS &left, const EditVectDS &right) 
{
  return left < right || right < left;
}

ostream& EditVectDS::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Edit Distance" << endl;
  out << "Edit Vector\t< D + S, I >" << endl;
  return out << endl;
}

ostream& operator<<(ostream &out, const EditVectIDS &ev)
{
  return out << ev.insDelSub;
}

istream& operator>>(istream &in, EditVectIDS &ev)
{
  return in >> ev.insDelSub;
}

bool operator<(const EditVectIDS &left, const EditVectIDS &right) 
{
  if (&left == &right)
    return false;
  return left.insDelSub < right.insDelSub;
}

bool operator!=(const EditVectIDS &left, const EditVectIDS &right) 
{
  return left < right || right < left;
}

ostream& EditVectIDS::info(ostream &out)
{
  out << "Metric" << endl << "---" << endl;
  out << "Edit Distnace" << endl;
  out << "Edit Vector\t< I + D + S >" << endl;
  return out << endl;
}
