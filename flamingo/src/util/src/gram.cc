/*
  $Id: gram.cc 6132 2012-02-22 21:53:14Z abehm $

  Copyright (C) 2010 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license.

  Date: 01/30/2007
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "gram.h"

#include <sys/stat.h>

#include "input.h"
#include "misc.h"

hash<string> hashString;

void str2grams(const string &s, vector<string> &res, unsigned q,
               unsigned char st, unsigned char en)
{
  const string sPad = string(q - 1, st) + s + string(q - 1, en);
  
  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.push_back(sPad.substr(i, q));  
}

void str2grams(const string &s, vector<unsigned> &res, 
               unsigned q, unsigned char st, unsigned char en) 
{
  string sPad = string(q - 1, st) + s + string(q - 1, en);
  
  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.push_back(hashString(sPad.substr(i, q)));
}

void str2grams(const string &s, multiset<string> &res, unsigned q, 
               unsigned char st, unsigned char en)
{
  const string sPad = string(q - 1, st) + s + string(q - 1, en);

  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.insert(sPad.substr(i, q));  
}

void str2grams(const string &s, multiset<unsigned> &res, 
               unsigned q, unsigned char st, unsigned char en) 
{
  string sPad = string(q - 1, st) + s + string(q - 1, en);
  
  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.insert(hashString(sPad.substr(i, q)));
}

void str2grams(const string &s, set<string> &res, unsigned q, 
               unsigned char st, unsigned char en)
{
  const string sPad = string(q - 1, st) + s + string(q - 1, en);

  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.insert(sPad.substr(i, q));  
}

void str2grams(const string &s, set<unsigned> &res, 
               unsigned q, unsigned char st, unsigned char en) 
{
  string sPad = string(q - 1, st) + s + string(q - 1, en);
  
  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res.insert(hashString(sPad.substr(i, q)));
}

void str2grams(const string &s, map<unsigned, unsigned> &res, unsigned q, 
               unsigned char st, unsigned char en)
{
  const string sPad = string(q - 1, st) + s + string(q - 1, en);

  for (unsigned i = 0; i < s.length() + q - 1; i++)
    res[hashString(sPad.substr(i, q))]++;  
}

void str2gramsNoPrePost(const string &s, vector<unsigned> &res,  unsigned q) 
{
  for (unsigned i = 0; i < s.length() - q + 1; i++) {
    //ignore gram with space 
    string substring = s.substr(i, q);
    string::size_type loc = substring.find(' ', 0);
    if (loc == string::npos) 
      res.push_back(hashString(substring));
  }//end for
}

void str2gramsNoPrePost(const string &s, set<string> &res, unsigned q)
{
  if (s.length() < q) {
    cerr << "string length (" << s.length()
         << ") less than q (" << q << ")" << endl;
    exit(1);
  }
  
  for (unsigned i = 0; i < s.length() - q + 1; i++)
    res.insert(s.substr(i, q));  
}

void str2gramsNoPrePost(const string &s, set<unsigned> &res,  unsigned q) 
{
  for (unsigned i = 0; i < s.length() - q + 1; i++) {
    //ignore gram with space 
    string substring = s.substr(i, q);
    string::size_type loc = substring.find(' ', 0);
    if (loc == string::npos) 
      res.insert(hashString(substring));
  }//end for
}

//Get special grams which contains in "ch" set
//this function is used in synonym work

void getSpecialGrams(const string &s, const unsigned q, const vector<char> ch,
                     set<unsigned> &res) 
{
  for (unsigned i = 0; i < s.length() - q + 1; i++) {
    string substring = s.substr(i, q);
    for(unsigned i=0;i<ch.size();i++) {
      string::size_type loc = substring.find(ch.at(i), 0 );
      if( loc != string::npos ) 
        res.insert(hashString(substring));
    }//end for
  }//end for
}// getSpecialGrams

//convert strings to inverted lists with id and position information
//Please remember to release memory space in map in your own code!
// If create grams without prefix and suffic, please set addStEn = false
void createIdPosInvertedLists(const vector<string> data, bool addStEn,
                              GramListMap &idLists, GramListMap &posLists,
                              unsigned q,
                              unsigned char st, unsigned char en)
{
  for(unsigned i=0;i<data.size();i++) {
    vector<unsigned> gramCodes;
    if (addStEn)
      str2grams(data.at(i),gramCodes,q,st,en);
    else
      str2gramsNoPrePost(data.at(i),gramCodes,q);
      
    for(unsigned j=0;j<gramCodes.size();j++) {
      unsigned gram = gramCodes.at(j);
	  
      if (idLists.find(gram) == idLists.end()) {
	      // a new gram
	      Array<unsigned> *arrayGram = new Array<unsigned>();
	      arrayGram->append(i);
	      idLists[gram] = arrayGram;
	      
	      Array<unsigned> *arrayPos = new Array<unsigned>();
	      arrayPos->append(j);
	      posLists[gram] = arrayPos;
	    }//end if
      else {
	      Array<unsigned> *arrayGram = idLists[gram];
	      arrayGram->append(i);
	      
	      Array<unsigned> *arrayPos = posLists[gram];
	      arrayPos->append(j);
	    }//end else
    }//end for  
  }//end for
}//end  createIdPosInvertedLists

void grams2str(const vector<string> &v, string &res, const unsigned q)
{
  res = "";
  
  for (unsigned i = 0; i < v.size() - q + 1; i++)
    res += v[i].substr(q - 1, 1);
}

unsigned gram2id(const string &gram) 
{
  unsigned
    id = 0, 
    gramLen = gram.length();
  string::size_type pos;
  for (unsigned i = 0; i < gramLen; i++) {
    pos = GramId::charsetEn.find(gram[i]);
    if (pos == string::npos) {
      cerr << "can't find character '" << gram[i] << "'(" 
           << static_cast<unsigned>(gram[i]) << ") of gram \"" << gram
           << "\" in charset" << endl;
      exit(EXIT_FAILURE);
    }
    id += pow(GramId::charsetEn.size(), gramLen - i - 1) * pos;
  }
  return id;
}

void id2gram(unsigned id, string &res, const unsigned q)
{
  res = "";
  while (id > 0) {
    res = string(1, GramId::charsetEn[id % GramId::charsetEn.size()]) + res;
    id = id / GramId::charsetEn.size();
  }
  while (res.length() < q)
    res = string(1, GramId::charsetEn[0]) + res;
}

// GramId
const unsigned GramId::charsetLenMax = 500;
const string GramId::charsetEn = 
  " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~0123456789abcdefghijklmnopqrstuvwxyz";
const string GramId::gramidSuffix = ".gid.bin";

GramId::GramId(unsigned q, char st, char en, 
               const string &charset, bool withPerm):
  q(q),
  st(st),
  en(en),
  charset(string(1, st) + charset + string(1, en)), 
  charsetLen(this->charset.size()), 
  n(pow(charsetLen, q)), 
  perm(vector<unsigned>(n))
{
  for (unsigned i = 0; i < n; i++)
    perm[i] = i;
  if (withPerm)
    random_shuffle(perm.begin(), perm.end());
  
  
}

GramId::GramId(const string &filenamePrefix) 
{
  loadData(filenamePrefix);
}

unsigned GramId::getId(const string &gram) const
{
  unsigned
    id = 0, 
    gramLen = gram.length();
  string::size_type pos;
  for (unsigned i = 0; i < gramLen; i++) {
    pos = charset.find(gram[i]);
    if (pos == string::npos) {
      cerr << "can't find character '" << gram[i] << "'(" 
           << static_cast<unsigned>(gram[i]) << ") of gram \"" << gram
           << "\" in charset" << endl;
      exit(EXIT_FAILURE);
    }
    id += pow(charsetLen, gramLen - i - 1) * pos;
  }
  return id;
}

string GramId::getGram(unsigned id) const
{
  string gram = "";
  while (id > 0) {
    gram = string(1, charset[id % charsetLen]) + gram;
    id = id / charsetLen;
  }
  while (gram.length() < q)
    gram = string(1, charset[0]) + gram;
  return gram;
}

void GramId::getIds(const string &s, vector<unsigned> &ids) const
{
  vector<string> gs;
  str2grams(s, gs, q);
  for (vector<string>::const_iterator it = gs.begin(); it != gs.end(); it++)
    ids.push_back(getId(*it));
}

void GramId::getGrams(const vector<unsigned> &ids, vector<string> &grams) const
{
  for (vector<unsigned>::const_iterator it = ids.begin(); it != ids.end(); it++)
    grams.push_back(getGram(*it));
}

void GramId::saveData(const string &filenamePrefix) const
{
  string filename = filenamePrefix + gramidSuffix;

  cerr << "write to \"" << filename << "\"...";
  cerr.flush();

  ofstream file(filename.c_str(), ios::out | ios::binary);
  if (!file) {
    cerr << "can't open output file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file.write(reinterpret_cast<const char*>(&q), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  file.write(reinterpret_cast<const char*>(&st), sizeof(char));
  if (file.fail()) writeerror(filename);

  file.write(reinterpret_cast<const char*>(&en), sizeof(char));
  if (file.fail()) writeerror(filename);

  file.write(reinterpret_cast<const char*>(&charsetLen), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  for (unsigned i = 0; i < charsetLen; i++) {
    file.write(reinterpret_cast<const char*>(&charset[i]), sizeof(char));
    if (file.fail()) writeerror(filename);
  }
  
  file.write(reinterpret_cast<const char*>(&n), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  for (vector<unsigned>::const_iterator it = perm.begin(); it != perm.end(); ++it)
    file.write(reinterpret_cast<const char*>(&*it), sizeof(unsigned));

  file.close();

  cerr << "OK" << endl;
}

void GramId::loadData(const string &filenamePerfix)
{
  string filename = filenamePerfix + gramidSuffix;
 
  cerr << "read from \"" << filename << "\"...";
  cerr.flush();

  ifstream file(filename.c_str(), ios::in | ios::binary);
  if (!file) {
    cerr << "can't open input file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  file.read(reinterpret_cast<char*>(&q), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  file.read(reinterpret_cast<char*>(&st), sizeof(char));
  if (file.fail()) writeerror(filename);

  file.read(reinterpret_cast<char*>(&en), sizeof(char));
  if (file.fail()) writeerror(filename);

  file.read(reinterpret_cast<char*>(&charsetLen), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  charset = "";
  for (unsigned i = 0; i < charsetLen; i++) {
    char ch;
    file.read(reinterpret_cast<char*>(&ch), sizeof(char));
    if (file.fail()) writeerror(filename);
    charset += string(1, ch);
  }
  
  file.read(reinterpret_cast<char*>(&n), sizeof(unsigned));
  if (file.fail()) writeerror(filename);

  for (unsigned i = 0; i < n; i++) {
    unsigned e;
    file.read(reinterpret_cast<char*>(&e), sizeof(unsigned));
    perm.push_back(e);
  }

  file.close();

  cerr << "OK" << endl;
}

unsigned GramId::invPerm(unsigned id) const
{
  for (unsigned i = 0; i < perm.size(); i++)
    if (perm[i] == id)
      return i;
  cerr << "ID " << id << " out of range in permutation" << endl;
  exit(EXIT_FAILURE);
}

bool GramId::consistData(const string &filenamePrefix, 
                         const string &filenameExt) const
{
  string filename = filenamePrefix + gramidSuffix;

  struct stat attrib, attribExt;
  if (stat(filename.c_str(), &attrib)) {
    cerr << "can't stat file \"" << filename << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  if (stat(filenameExt.c_str(), &attribExt)) {
    cerr << "can't stat file \"" << filenameExt << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  if (!(attribExt.st_mtime <= attrib.st_mtime))
    return false;

  return true;  
}

bool GramId::operator==(const GramId& g) const 
{
  if (this == &g)
    return true;
  if (q == g.q && 
      st == g.st && 
      en == g.en && 
      charset == g.charset && 
      charsetLen == g.charsetLen && 
      n == g.n && 
      perm == g.perm)
    return true;
  return false;
}

// str2words
void str2words(const string &s, vector<string> &res, const string &delims) 
{
  string::size_type begIdx, endIdx;
  
  begIdx = s.find_first_not_of(delims);
  
  while (begIdx != string::npos) {
    endIdx = s.find_first_of(delims, begIdx);
    if (endIdx == string::npos)
      endIdx = s.length();
//     cout << begIdx << " " << endIdx << " " 
//          << s.substr(begIdx, endIdx - begIdx) << endl;
    res.push_back(s.substr(begIdx, endIdx - begIdx));
    begIdx = s.find_first_not_of(delims, endIdx);
  }
}

// WordIndex
void WordIndex::build(const vector<string> &data, WordHash &wordHash)
{
  cout << "WordIndex::build..."; cout.flush();
  
  vector<string> words;
  for (unsigned i = 0; i < data.size(); i++) {

    words.clear();
    str2words(data[i], words);
    for (vector<string>::const_iterator word = words.begin();
         word != words.end(); ++word)
      wordHash[*word].insert(i);
  } 

  cout << "OK" << endl;
}

void WordIndex::build(const string &filenameDataset, WordHash &wordHash)
{
  cout << "WordIndex::build..."; cout.flush();

  ifstream fileDataset(filenameDataset.c_str());
  if (!fileDataset) {
    cerr << "can't open input file \"" << filenameDataset << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  
  vector<string> words;
  unsigned i = 0;
  const unsigned maxLineLen = 256;
  char line[maxLineLen];

  while (true) {
    fileDataset.getline(line, maxLineLen);
    if (fileDataset.eof())
      break;
    if (fileDataset.fail()) {
      cerr << "open reading input file \""
           << filenameDataset << "\"" << endl
           << "line length might exceed " << maxLineLen << " characters" << endl;
      exit(EXIT_FAILURE);
    }

    words.clear();
    str2words(string(line), words);
    for (vector<string>::const_iterator word = words.begin();
         word != words.end(); ++word)
      wordHash[*word].insert(i);

    i++;
  } 

  cout << "OK" << endl;
}

void WordIndex::build(const vector<string> &data,
                      WordIds &wordIds, WordKey &wordKey)
{
  cout << "WordIndex::build..."; cout.flush();
  
  vector<string> words;
  for (unsigned i = 0; i < data.size(); i++) {

    words.clear();
    str2words(data[i], words);
    for (vector<string>::const_iterator word = words.begin();
         word != words.end(); ++word) {

      pair<WordKey::iterator, bool> wordIns = 
        wordKey.insert(make_pair(*word, 0));

      unsigned wordPos;
      if (wordIns.second) {
        // word not in WordLevel
        wordPos = wordIds.size();
        wordIns.first->second = wordPos;
        Ids ids;
        ids.insert(i);
        wordIds.push_back(make_pair(*word, ids));
      }
      else {
        // word in WordLevel
        wordPos = wordIns.first->second;
        wordIds[wordPos].second.insert(i);
      }            
    }
  } 

  cout << "OK" << endl;
}

void WordIndex::build(const string &filenameDataset,
                      WordIds &wordIds, WordKey &wordKey)
{
  cout << "WordIndex::build..."; cout.flush();

  ifstream fileDataset(filenameDataset.c_str());
  if (!fileDataset) {
    cerr << "can't open input file \"" << filenameDataset << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  
  vector<string> words;
  unsigned i = 0;
  const unsigned maxLineLen = 256;
  char line[maxLineLen];

  while (true) {
    fileDataset.getline(line, maxLineLen);
    if (fileDataset.eof())
      break;
    if (fileDataset.fail()) {
      cerr << "open reading input file \""
           << filenameDataset << "\"" << endl
           << "line length might exceed " << maxLineLen << " characters" << endl;
      exit(EXIT_FAILURE);
    }

    words.clear();
    str2words(string(line), words);
    for (vector<string>::const_iterator word = words.begin();
         word != words.end(); ++word) {

      pair<WordKey::iterator, bool> wordIns = 
        wordKey.insert(make_pair(*word, 0));

      unsigned wordPos;
      if (wordIns.second) {
        // word not in WordLevel
        wordPos = wordIds.size();
        wordIns.first->second = wordPos;
        Ids ids;
        ids.insert(i);
        wordIds.push_back(make_pair(*word, ids));
      }
      else {
        // word in WordLevel
        wordPos = wordIns.first->second;
        wordIds[wordPos].second.insert(i);
      }            
    }

    i++;
  } 

  cout << "OK" << endl;
}

void WordIndex::build(const vector<string> &data,
                      vector<string> &wordVect, vector<Ids> &idsVect, 
                      WordKey &wordPosMap) 
{
  cout << "WordIndex::build..."; cout.flush();

  vector<string> words;
  unsigned pos;
  
  for (unsigned i = 0; i < data.size(); i++) {

    words.clear();
    str2words(data[i], words);
    for (vector<string>::const_iterator word = words.begin();
         word != words.end(); ++word) {

      pair<WordKey::iterator, bool> ins = 
        wordPosMap.insert(make_pair(*word, 0));

      if (ins.second) {
        // word not in WordHash
        pos = wordVect.size();
        wordVect.push_back(*word);
        Ids ids;
        ids.insert(i);
        idsVect.push_back(ids);
        ins.first->second = pos;
      }
      else {
        // word in WordHash
        pos = ins.first->second;
        idsVect[pos].insert(i);
      } 
    }
  }

  cout << "OK" << endl;  
}

void WordIndex::save(const string &filenameWords, 
                     const string &filenameIds,
                     const WordHash &wordHash) 
{
  cerr << "write to \"" << filenameWords << "\" and \""
       << filenameIds << "\"...";

  ofstream fileWords(filenameWords.c_str(), ios::out);  
  if (!fileWords) {
    cerr << "can't open output file \"" << filenameWords << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  ofstream fileIds(filenameIds.c_str(), ios::out | ios::binary);  
  if (!fileIds) {
    cerr << "can't open output file \"" << filenameIds << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  unsigned size;
  for(WordHash::const_iterator it = wordHash.begin();
      it != wordHash.end(); ++it) {
    fileWords << it->first << endl;
    
    size = it->second.size();
    fileIds.write(reinterpret_cast<const char*>(&size), sizeof(unsigned));
    for (Ids::const_iterator jt = it->second.begin();
         jt != it->second.end(); ++jt)
      fileIds.write(reinterpret_cast<const char*>(&*jt), sizeof(unsigned));
  }
    
  fileWords.close();
  fileIds.close();

  cerr << "OK" << endl;
}

void WordIndex::load(const string &filenameWords, 
                     const string &filenameIds, 
                     WordHash &wordHash)
{
  cerr << "read from \"" << filenameWords << "\" and \""
       << filenameIds << "\"...";

  ifstream fileWords(filenameWords.c_str(), ios::in);  
  if (!fileWords) {
    cerr << "can't open input file \"" << filenameWords << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  ifstream fileIds(filenameIds.c_str(), ios::in | ios::binary);  
  if (!fileIds) {
    cerr << "can't open input file \"" << filenameIds << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  unsigned size, id;
  string word;
  Ids ids;

  while (true) {
    // hash
    fileWords >> word;
    if (fileWords.eof())
      break;

    ids.clear();
    fileIds.read(reinterpret_cast<char*>(&size), sizeof(unsigned));
    for (unsigned i = 0; i < size; i++) {
      fileIds.read(reinterpret_cast<char*>(&id), sizeof(unsigned));
      ids.insert(id);
    }
    
    wordHash[word] = ids;
  }
    
  fileWords.close();
  fileIds.close();

  cerr << "OK" << endl;
}

void WordIndex::save(const string &filenameWids, 
                     const string &filenameWkey,
                     const WordIds &wordIds, const WordKey &wordKey)
{
  cerr << "write to \"" << filenameWids << "\" and \""
       << filenameWkey << "\"...";

  ofstream fileWids(filenameWids.c_str(), ios::out | ios::binary);  
  if (!fileWids) {
    cerr << "can't open output file \"" << filenameWids << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  unsigned size;
  for(WordIds::const_iterator it = wordIds.begin();
      it != wordIds.end(); ++it) {
    size = it->second.size();
    fileWids.write(reinterpret_cast<const char*>(&size), sizeof(unsigned));
    for (Ids::const_iterator jt = it->second.begin();
         jt != it->second.end(); ++jt)
      fileWids.write(reinterpret_cast<const char*>(&*jt), sizeof(unsigned));
  }
    
  fileWids.close();

  ofstream fileWkey(filenameWkey.c_str(), ios::out);  
  if (!fileWkey) {
    cerr << "can't open output file \"" << filenameWkey << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  for (WordIds::const_iterator it = wordIds.begin();
       it != wordIds.end(); ++it)
    fileWkey << it->first<< " " << wordKey.find(it->first)->second << endl;

  fileWkey.close();

  cerr << "OK" << endl;
}

void WordIndex::load(const string &filenameWids, 
                     const string &filenameWkey, 
                     WordIds &wordIds, WordKey &wordKey)
{
  cerr << "read from \"" << filenameWids << "\" and \""
       << filenameWkey << "\"...";

  ifstream fileWids(filenameWids.c_str(), ios::in | ios::binary);  
  if (!fileWids) {
    cerr << "can't open input file \"" << filenameWids << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  ifstream fileWkey(filenameWkey.c_str(), ios::in);  
  if (!fileWkey) {
    cerr << "can't open input file \"" << filenameWkey << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  string word;
  unsigned size, id;
  Ids ids;

  while (true) {
    fileWids.read(reinterpret_cast<char*>(&size), sizeof(unsigned));
    if (fileWids.eof())
      break;
    ids.clear();
    for (unsigned i = 0; i < size; i++) {
      fileWids.read(reinterpret_cast<char*>(&id), sizeof(unsigned));
      ids.insert(id);
    }

    fileWkey >> word >> id;
    if (fileWkey.eof()) {
      cerr << "inconsistency in input file \"" << filenameWkey << "\"" << endl;
      exit(EXIT_FAILURE);
    }
    
    wordIds.push_back(make_pair(word, ids));
    wordKey[word] = id;
  }

  fileWids.close();  
  fileWkey.close();

  cerr << "OK" << endl;
}

bool WordIndex::exist(const string &filename1, const string &filename2) 
{
  ifstream file1(filename1.c_str(), ios::in | ios::binary);  
  if (!file1) 
    return false;
  file1.close();

  ifstream file2(filename2.c_str(), ios::in);  
  if (!file2)
    return false;
  file2.close();
  
  return true;
}

const string
filenameExtWordVect = ".wi.wv.txt", 
                  filenameExtIdsVect = ".wi.idv.bin";


void WordIndex::save(const string &filename, const vector<string> &wordVect,
                     const vector<Ids> &idsVect, const WordKey &wordPosMap) 
{
  const string
    filenameWordVect = filename + filenameExtWordVect, 
    filenameIdsVect = filename + filenameExtIdsVect;
  
  cerr << "write to \"" << filenameWordVect << "\" and" << endl
       << "         \"" << filenameIdsVect  << "\"...";
  cerr.flush();

  ofstream fileWordVect(filenameWordVect.c_str(), ios::out);  
  if (!fileWordVect) {
    cerr << "can't open output file \"" << filenameWordVect << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  for (vector<string>::const_iterator word = wordVect.begin();
       word != wordVect.end(); ++word)
    fileWordVect << *word << endl;

  fileWordVect.close();


  ofstream fileIdsVect(filenameIdsVect.c_str(), ios::out | ios::binary);  
  if (!fileIdsVect) {
    cerr << "can't open output file \"" << filenameIdsVect << "\"" << endl;
    exit(EXIT_FAILURE);
  }

  for (vector<Ids>::const_iterator ids = idsVect.begin();
       ids != idsVect.end(); ++ids) {
    unsigned size = ids->size();
    fileIdsVect.write(reinterpret_cast<const char*>(&size), sizeof(unsigned));
    for (Ids::const_iterator id = ids->begin();
         id != ids->end(); ++id)
      fileIdsVect.write(reinterpret_cast<const char*>(&*id), sizeof(unsigned));
  }
  
  fileIdsVect.close();

  cerr << "OK" << endl;
}

void WordIndex::load(const string &filename, vector<string> &wordVect,
                     vector<Ids> &idsVect, WordKey &wordPosMap) 
{
  const string
    filenameWordVect = filename + filenameExtWordVect, 
    filenameIdsVect = filename + filenameExtIdsVect;
  
  cerr << "read from \"" << filenameWordVect << "\" and" << endl
       << "          \"" << filenameIdsVect  << "\"...";
  cerr.flush();

  readString(wordVect, filenameWordVect);

  ifstream fileIdsVect(filenameIdsVect.c_str(), ios::in | ios::binary);  
  if (!fileIdsVect) {
    cerr << "can't open input file \"" << filenameIdsVect << "\"" << endl;
    exit(EXIT_FAILURE);
  }
  
  for (unsigned i = 0; i < wordVect.size(); i++) {    
    unsigned size;
    fileIdsVect.read(reinterpret_cast<char*>(&size), sizeof(unsigned));

    Ids ids;
    unsigned el;
    for (unsigned j = 0; j < size; j++) {
      fileIdsVect.read(reinterpret_cast<char*>(&el), sizeof(unsigned));
      ids.insert(el);
    }
    idsVect.push_back(ids);

    wordPosMap[wordVect[i]] = i;
  }
  
  fileIdsVect.close();

  cerr << "OK" << endl;
}
