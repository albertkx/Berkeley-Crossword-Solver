/*
  $Id: gramgen.h Tue Apr 05 10:20:24 PDT 2008 abehm$

  Copyright (C) 2010 by The Regents of the University of California
	
  Redistribution of this file is permitted under
  the terms of the BSD license
    
  Date: 04/05/2008
  Author: Alexander Behm <abehm (at) ics.uci.edu>
          Rares Vernica <rares (at) ics.uci.edu> 
*/

#ifndef _gramgen_h_
#define _gramgen_h_

#include <map>
#include <set>
#include <vector>
#include <fstream>

#include "tr1_local.h"
#include "typedef.h"

typedef enum
{
  GGT_FIXED,
  GGT_WORDS,
  GGT_VGRAM,
  GGT_GAPPED
} GramGenType;

#ifdef _WIN32
class hash_win32 
{   
public:   
  std::size_t operator()(const std::string& s) const
  { 
    const char* first = s.data();
    std::size_t length = s.length();
    std::size_t result = 0;
    for (; length > 0; --length)
      result = (result * 131) + *first++;
    return result;
  }  
};
#endif

// gram generator interface
class GramGen
{
protected:
#ifdef _WIN32
  static const hash_win32 hashString;
#else 
  static const std::tr1::hash<std::string> hashString;
#endif
  static const uchar PREFIXCHAR = 156; // pound
  static const uchar SUFFIXCHAR = 190; // yen

  GramGenType gramGenType;
  
public:
  // pre-and postfix string when generating grams?
  // needs to be accessible by simmetric for calculating filter bounds (e.g. by jacc and cos)
  bool prePost;

  GramGen(bool usePrePost = true):
    prePost(usePrePost)
    {}

  virtual ~GramGen(){}

  // convert a string to a BAG of grams
  virtual void decompose(
    const std::string &s, 
    std::vector<std::string> &res,	 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a BAG of hashed grams
  virtual void decompose(
    const std::string &s, 
    std::vector<uint> &res,	 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a BAG (multiset) of grams
  virtual void decompose(
    const std::string &s, 
    std::multiset<std::string> &res,	 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a BAG (multiset) of hashed grams
  virtual void decompose(
    const std::string &s, 
    std::multiset<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a SET of grams
  virtual void decompose(
    const std::string &s, 
    std::set<std::string> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a SET of hashed grams
  virtual void decompose(
    const std::string &s, 
    std::set<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;
  
  // convert a string to a SET of hashed grams with count
  virtual void decompose(
    const std::string &s, 
    std::map<uint, uint> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const = 0;  
  
  virtual uint getNumGrams(const std::string& s) const = 0;

  virtual uint getGramLength() const = 0;
  
  virtual void getPrePostString(const std::string& src, 
				std::string& dest, 
				uchar st = PREFIXCHAR, 
				uchar en = SUFFIXCHAR) const = 0;
  
  virtual void saveGramGenInstance(std::ofstream& fpOut) = 0;

  GramGenType getType() const { return gramGenType; }  

  virtual unsigned getPrePostStrLen(unsigned origStrLen) const = 0;
  
  virtual unsigned getLength(const std::string& s) const { return s.length(); }
  
  static GramGen* loadGramGenInstance(std::ifstream& fpIn);
};


class GramGenFixedLen: public GramGen
{
 private:
  uint q;
  bool noSpace;

  bool containsSpace(std::string& s) const;

 public:
  GramGenFixedLen(
    uint gramLength = 3, 
    bool usePrePost = true, 
    bool ignoreSpaces = false): 
    GramGen(usePrePost), 
    q(gramLength), 
    noSpace(ignoreSpaces)
    { gramGenType = GGT_FIXED; }
    
  GramGenFixedLen(std::ifstream& fpIn);
    
  void decompose(
    const std::string &s, 
    std::vector<std::string> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::vector<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::multiset<std::string> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::multiset<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::set<std::string> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::set<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::map<uint, uint> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
    
  uint getNumGrams(const std::string& s) const;

  uint getGramLength() const { return q; }

  void getPrePostString(const std::string& src, 
			std::string& dest, 
			uchar st = PREFIXCHAR, 
			uchar en = SUFFIXCHAR) const;

  unsigned getPrePostStrLen(unsigned origStrLen) const;
  
  void saveGramGenInstance(std::ofstream& fpOut);
};

class WordGen: public GramGen {
private:
  std::string delimiter;
  bool containsSpace(std::string& s) const;

public:
  WordGen(const std::string& delimiter = " "): 
    GramGen(false),
    delimiter(delimiter)
  { gramGenType = GGT_WORDS; }
  
  WordGen(std::ifstream& fpIn);
  
  void decompose(
    const std::string &s, 
    std::vector<std::string> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::vector<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::multiset<std::string> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::multiset<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::set<std::string> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::set<uint> &res, 
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
  
  void decompose(
    const std::string &s, 
    std::map<uint, uint> &res,
    uchar st = PREFIXCHAR, 
    uchar en = SUFFIXCHAR) 
    const;
    
  uint getNumGrams(const std::string& s) const;

  uint getGramLength() const { return 0; }

  void getPrePostString(const std::string& src, 
			std::string& dest, 
			uchar st = PREFIXCHAR, 
			uchar en = SUFFIXCHAR) const;

  unsigned getPrePostStrLen(unsigned origStrLen) const;
  
  unsigned getLength(const std::string& s) const { return getNumGrams(s); }
  
  void saveGramGenInstance(std::ofstream& fpOut);
};


// to be implemented by Ajey
class GramGenGapped: public GramGen
{
};

// to be implemented
class GramGenVGram: public GramGen
{
};

#endif

