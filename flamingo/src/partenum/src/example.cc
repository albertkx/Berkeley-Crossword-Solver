/*
  $Id: example.cc 5769 2010-10-19 06:17:00Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  The implementation of the PartEnum algorithm invented by Microsoft
  researchers is limited to non commercial use, which would be covered
  under the royalty free covenant that Microsoft made public.

  Date: 01/31/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include <iostream>

#include "partenum.h"
#include "util/src/input.h"
#include "util/src/misc.h"

using namespace std;

int main() 
{
  const string 
    filenameData = "dataset.txt", 
    filenamePartEnum = "partenum";

  vector<string> data;
  readString(data, filenameData);

  const unsigned
    editdist = 2, 
    q = 1, 
    n1 = 2, 
    n2 = 6;

  PartEnum h(data, q, editdist, n1, n2);

  // 1. pre-processing (generate signatures)

  h.build();

  // 2. save

  h.saveIndex(filenamePartEnum);

  // 3. use

  vector<unsigned> results;
  h.search("delmare", editdist, results);
  
  for (vector<unsigned>::const_iterator i = results.begin(); i != results.end(); ++i)
    cout << data[*i] << endl;
}
