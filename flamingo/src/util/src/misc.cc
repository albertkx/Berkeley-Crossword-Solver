/*
  $Id: misc.cc 5713 2010-09-09 03:11:22Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the
  BSD License.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "misc.h"
#include "input.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

unsigned pow(unsigned x, unsigned y) 
{
  unsigned r = 1;
  while (y > 0) {
    r *= x;
    y--;
  }
  return r;
}

vector<vector<unsigned> > subsets(unsigned n, unsigned k) 
{
  if (k == 0) {
    cerr << "subsets: k cannot be 0" << endl;
    exit(EXIT_FAILURE);
  }
  vector<vector<unsigned> > subs;
  vector<unsigned> sub(k);

  unsigned i;
  for (i = 0; i < k; i++)
    sub[i] = i;
  subs.push_back(sub);

  while (sub[0] < n - k) {
    for (i = 0; i < k; i++)
      if (sub[k - i - 1] < n - i - 1)
        break;
    i = k - i - 1;
    sub[i]++;
    i++;
    for (; i < k; i++)
      sub[i] = sub[i - 1] + 1;
    subs.push_back(sub);
  }

  return subs;
}

string utosh(unsigned i) 
{
  string str;
  if (i < 1000)
    str = utos(i);
  else if (i < 1000000)
    str = utos(i / 1000.) + "k";
  else
    str = utos(i / 1000000.) + "m";
  return str;
}

UnsignedSeq::UnsignedSeq(unsigned initialValue): value(initialValue)
{
}

unsigned UnsignedSeq::operator() ()
{
  return value++;
}

string removeExt(const string filename)
{
  const string exts[] = {"txt", "bin"};
  for (unsigned i = 0; i < sizeof(exts) / sizeof(string); i++)
    if (filename.substr(filename.size() - 4) == "." + exts[i])
      return filename.substr(0, filename.size() - 4);
  return filename;
}

void writeerror(const string filename) 
{
  cerr << "can't write to file \"" << filename << "\"" << endl;
  exit(EXIT_FAILURE);
}

void readerror(const string filename) 
{
  cerr << "can't read from file \"" << filename << "\"" << endl;
  exit(EXIT_FAILURE);
}

void genZipfDist(unsigned distinctValues, unsigned zipfSkew, double valFreqs[]) {
  double divisor = 0;
  for(unsigned i = 1; i <= distinctValues; i++)
    divisor += 1.0 / (double) pow((double)i, (double)zipfSkew);

  for(unsigned i = 1; i <= distinctValues; i++)
    valFreqs[i-1] = ( 1.0 / (double) pow((double)i, (double)zipfSkew) ) / divisor;
}

void enlargeDataset(const char* targetFile, 
		    const char* inputFile, 
		    unsigned targetSize,
		    unsigned minErrors,
		    unsigned maxErrors, 
		    unsigned trueRand) {

  if(trueRand) srand(time(NULL));
  else srand(150);

  unsigned maxStrLength = 1000;

  vector<string> dictionary;
  readString(dictionary, inputFile, targetSize, maxStrLength);	

  cout << "GENERATING DICTIONARY" << endl;
  unsigned originalSize = dictionary.size();
  while(dictionary.size() < targetSize) {
    // read original strings and introduce errore
    for(unsigned i = 0; i < originalSize; i++) {
      string newString = dictionary.at(i);

      // introduce errors
      unsigned errors = (rand() % (maxErrors-minErrors)) + maxErrors;
      for(unsigned j = 0; j < errors; j++) {
	unsigned pos = rand() % newString.size();
	unsigned op = rand() % 3;
	switch(op) {
	  // deletion 
	case 0: {
	  if(newString.size() > 1) {
	    const string del = "";
	    newString.replace(pos, 1, del);
	  }
	} break;

	  // insertion
	case 1: {
	  unsigned c = (rand() % (128-33)) + 33;
	  char newChar[2];
	  newChar[0] = c;
	  newChar[1] = '\0';
	  newString.insert(pos, newChar);	  
	} break;

	  // substitution
	case 2: {
	  if(newString.size() > 0) {
	    unsigned c = (rand() % (128-33)) + 33;
	    char newChar[2];
	    newChar[0] = c;
	    newChar[1] = '\0';
	    newString.replace(pos, 1, newChar);
	  }
	} break;

	}
      }
      
      // push to dictionary vector
      dictionary.push_back(newString);
    }    
  }

  cout << "WRITING TO " << targetFile << endl;
  unsigned perc = 0;

  ofstream fp;
  fp.open(targetFile, ios::out);

  for(unsigned i = 0; i < dictionary.size(); i++) {
    fp << dictionary.at(i) << endl;    
    perc = (unsigned)ceil( (double)(i+1) / (double)dictionary.size()*100 );
    cout << perc << "%" << "\r";
  }
  cout << endl << "ALL DONE!" << endl;

  fp.close();
}

void linearRegression(const vector<float>& xvals, const vector<float>& yvals, float& slope, float& intercept) {
  float sumX = 0.0f;
  float sumY = 0.0f;
  float sumXY = 0.0f;
  float sumXSquare = 0.0f;
  unsigned n = xvals.size();

  for(unsigned i = 0; i < n; i++) {
    sumX += xvals.at(i);
    sumY += yvals.at(i);
    sumXY += xvals.at(i) * yvals.at(i);
    sumXSquare += xvals.at(i) * xvals.at(i);
  }  

  slope = (n*sumXY - sumX*sumY) / (n*sumXSquare - sumX*sumX); 
  intercept = (sumY - slope*sumX) / (float)n;
}

// factorial function
double factorial(unsigned n) {
  double result = 1.0;
  for(unsigned i = n; i > 1; i--)
    result *= i;
  return result;  
}

float binomialDistrib(unsigned k, unsigned n, float p, bool cumulative) {
  if(cumulative) {
    float res = 0.0;
    for(unsigned i = k; i <= n; i++)
      res += factorial(n)/(factorial(n-i)*factorial(i))*pow((float)p,(float)i)*pow((float)(1.0f-p),(float)(n-i));
    return res;
  }
  else {
    return factorial(n)/(factorial(n-k)*factorial(k))*pow((float)p,(float)k)*pow((float)(1.0f-p),(float)(n-k));
  }
}

bool fileExists(const char* fileName) {
  struct stat stFileInfo;
  int intStat;
  
  intStat = stat(fileName, &stFileInfo);
  if(intStat == 0) return true;
  else return false; 
}

bool getRandomFileName(string& target, unsigned maxAttempts) {
  srand(time(NULL));
  unsigned attempts = 0;
  
  while(attempts < maxAttempts) {
    char fn[11];
    unsigned r = rand();
    sprintf(fn, "%d", r);    
    
    if(!fileExists(fn)) {
      target.assign(fn);
      return true;
    }

    attempts++;
  }
  
  return false;
}
