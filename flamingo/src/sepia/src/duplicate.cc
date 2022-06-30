/*
  $Id: duplicate.cc 5767 2010-10-19 05:52:31Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "duplicate.h"

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "util/src/simfuncs.h"

using namespace std;

string trim(const string &s)
{
  if (s.length()==0)
    return s;
  size_t b=s.find_first_not_of(" \t");
  size_t e=s.find_last_not_of(" \t");

  if (b==string::npos) // No non-spaces
    return "";
  return string(s, b, e-b+1);
}

void EditDistModif(string &sDup, unsigned noChanges)
{
  for (unsigned j = 0; j < noChanges && trim(sDup).length() > 2; j++) {
    // make one change to sDup
    const unsigned change = rand() % 3;
    /* change:
       0 -> inseriton
       1 -> deletion
       2 -> substitution
    */
    string::size_type pos;
    switch (change) {
    case 0: // insertion
      pos=rand()%sDup.length();
      sDup.insert(pos, 1, 'a' + static_cast<char>(rand() % ('z'-'a'+1)));
      break;
    case 1: // deletion
      pos=rand()%sDup.length();
      sDup.erase(pos, 1);
      break;
    case 2: // substitutiion
      pos=rand() % sDup.length();
      string::size_type pos2 = rand() % sDup.length();
      // the positons might be equal!
      const char posChar = sDup[pos];
      const char pos2Char = sDup[pos2];
      sDup.replace(pos, 1, 1, pos2Char);
      sDup.replace(pos2, 1, 1, posChar);
      break;
    }
  }
}

void insertDuplicates()
{
  const unsigned dataSetSize = 100000;
  const unsigned startSetSize =  5000;
  int percentDup = 100;
  // this is not const and not int in order not to 
  // generate a warining in if bellow
  const unsigned maxDup = 40; // # of duplicates is from 1 to maxDup
  const unsigned maxChanges = 4; // # nr of changes if form 1 to maxChanges

  string
    finName  = "in.txt",
    foutName = "out.txt";

  ifstream fin(finName.c_str()); 
  if (!fin) { 
    cerr << "can't open input file \"" << finName << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  
  ofstream fout(foutName.c_str());
  if (!fout) { 
    cerr << "can't open output file \"" << foutName << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  srand(static_cast<unsigned>(time(NULL)));

  cout << "reading records form file \"" << finName << "\"..." << endl;
  cout << "writing records to   file \"" << foutName << "\"..." << endl;
  string s;
  while (getline(fin, s)) {
    if (rand() % 1000 < 
        static_cast<int>(static_cast<float>(startSetSize) / 
                         dataSetSize * 1000)) {
      // the current string is selected
      fout << s << endl;
      // should it be duplicated
      if (percentDup == 100 ||
          rand() % 100 < percentDup) {
        // now generate duplicates
        const unsigned noDup = 1 + rand() % maxDup;
        for (unsigned i = 0; i < noDup; i++) {
          string sDup = s;
          EditDistModif(sDup, 1 + rand() % maxChanges);
          fout << trim(sDup) << endl;
        }
      }
    }
  }
}

