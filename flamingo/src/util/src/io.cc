/*
  $Id: io.cc 5720 2010-09-09 05:42:48Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the BSD
  license.

  Date: 06/23/2008
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "io.h"
#include "common/src/typedef.h"

using namespace std;

template<>
istream& text_io_read(istream &in, string &d) 
{
  return getline(in, d);
}

istream& read_binary_string(istream &in, string &str) 
{
  uint len;
  in.read(reinterpret_cast<char*>(&len), sizeof(uint));
  
  char *textBuf = new char[len];
  in.read(textBuf, len);
  str = string(textBuf, len);
  delete [] textBuf;
  
  return in;
}

ostream& write_binary_string(ostream &out, const string &str) 
{
  uint len = str.length();
  out.write(reinterpret_cast<const char*>(&len), sizeof(uint));
  return out.write(str.c_str(), len);
}

