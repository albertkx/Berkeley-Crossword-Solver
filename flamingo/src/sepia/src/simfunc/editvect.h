/*
  $Id: editvect.h 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#ifndef _editvect_h_
#define _editvect_h_

#include <fstream>

#include "util/src/simfuncs.h"

using namespace std;

class EditVect 
{
  // <I, D, S>
public:
  unsigned ins, del, sub;

  EditVect(): ins(0), del(0), sub(0) {}
  EditVect(const string &a, const string &b);

  bool operator==(const EditVect &e) const {
    return this == &e || (ins == e.ins && del == e.del && sub == e.sub); }

  operator size_t() const {
    return static_cast<size_t>((ins % 10) * 100 + (del % 10) * 10 + sub % 10); }

  unsigned getDist() const { return ins + del + sub; }
  
  void serialize(ofstream &out) const;
  void deserialize(ifstream &in);

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const EditVect &ev);
  friend istream& operator>>(istream &in, EditVect &ev);
  friend bool operator<(const EditVect &left, const EditVect &right);
  friend bool operator!=(const EditVect &left, const EditVect &right);
};

class EditVectID 
{
  // < I + D, S >
public:
  unsigned insDel, sub;

  EditVectID(): insDel(0), sub(0) {}
  EditVectID(const string &a, const string &b);

  bool operator==(const EditVectID &e) const {
    return this == &e || (insDel == e.insDel && sub == e.sub); }

  operator size_t() const { 
    return static_cast<size_t>((insDel % 10) * 10 + sub % 10); }

  unsigned getDist() const { return insDel + sub; }

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const EditVectID &ev);
  friend istream& operator>>(istream &in, EditVectID &ev);
  friend bool operator<(const EditVectID &left, const EditVectID &right);
  friend bool operator!=(const EditVectID &left, const EditVectID &right);
};

class EditVectIS
{
  // < I + S, D >
public:
  unsigned insSub, del;

  EditVectIS(): insSub(0), del(0) {}
  EditVectIS(const string &a, const string &b);

  bool operator==(const EditVectIS &e) const {
    return this == &e || (insSub == e.insSub && del == e.del); }

  operator size_t() const { 
    return static_cast<size_t>((insSub % 10) * 10 + del % 10); }

  unsigned getDist() const { return insSub + del; }

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const EditVectIS &ev);
  friend istream& operator>>(istream &in, EditVectIS &ev);
  friend bool operator<(const EditVectIS &left, const EditVectIS &right);
  friend bool operator!=(const EditVectIS &left, const EditVectIS &right);
};

class EditVectDS
{
  // < D + S, I >
public:
  unsigned delSub, ins;

  EditVectDS(): delSub(0), ins(0) {}
  EditVectDS(const string &a, const string &b);

  bool operator==(const EditVectDS &e) const {
    return this == &e || (delSub == e.delSub && ins == e.ins); }

  operator size_t() const {
    return static_cast<size_t>((delSub % 10) * 10 + ins % 10); }

  unsigned getDist() const { return delSub + ins; }

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const EditVectDS &ev);
  friend istream& operator>>(istream &in, EditVectDS &ev);
  friend bool operator<(const EditVectDS &left, const EditVectDS &right);
  friend bool operator!=(const EditVectDS &left, const EditVectDS &right);
};

class EditVectIDS
{
  // < I + D + S >
public:
  unsigned insDelSub;

  EditVectIDS(): insDelSub(0) {}
  EditVectIDS(const string &a, const string &b): insDelSub(ed(a, b)) {}

  bool operator==(const EditVectIDS &e) const {
    return this == &e || insDelSub == e.insDelSub; }

  operator size_t() const { return static_cast<size_t>(insDelSub); }

  unsigned getDist() const { return insDelSub; }

  static ostream& info(ostream &out);

  friend ostream& operator<<(ostream &out, const EditVectIDS &ev);
  friend istream& operator>>(istream &in, EditVectIDS &ev);
  friend bool operator<(const EditVectIDS &left, const EditVectIDS &right);
  friend bool operator!=(const EditVectIDS &left, const EditVectIDS &right);
};

#endif
