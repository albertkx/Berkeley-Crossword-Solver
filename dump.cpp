#include "score.h"

// Code to dump or read in a full database.  Fields are as follows
// (from score.h):

/*
  grammar                   [subclass]
  float                     portmanteau_cutoff;
  float                     flamingo_cutoff;
  unsigned                  mlg_tot[(LONG_WORD + 1)];
  unsigned                  mlg_size[(LONG_WORD + 1)][100];
  float                     mlg_prob[100];
  wordmap<char>             word_to_score;
  wordmap<char>             sound_to_score;
  map<parse,pdata>          pos_rules;
  wordmap<pdata>            first_pos, last_pos;
  map<clue_slot,cdata>      exact;        
  map<string,cdata>         inexact[1 + SEGMENTS];
  map<int,float>            exact_probs;  
  wordmap<float>            is_abbr;  
  wordset                   multidicts[NUM_MULTI_DICTS];
  vector<vector<string>>    moby;
  map<vstring,vector<vstring>> translations;
  vector<string>            words;
  vtransition_map           transitions;
  vtransition_map           reversals;
  wordmap<vector<string>>   pronunciations;
  wordmap<vector<string>>   pron_to_words;
  vector<string>            flamingo_words[2];
  map<string,float>         rebus_words;
  vector<vector<unsigned>>  anagrams[2];

A grammar contains:
  wordmap<vector<POS>>      parses;
  map<string,string>        roots;
  map<string,vector<string>> constructs;

*/

// This uses old C-style files, but it works, it's stable, and there
// really isn't any reason to change it.

struct dumper {
  FILE *f;

  dumper(FILE *z) : f(z) { }
  ~dumper() { fclose(f); }

  void write(const unsigned[1 + LONG_WORD]);
  void write(const unsigned[1 + LONG_WORD][100]);
  void write(const float[100]);
  template<class T> void write(const wordmap<T> &);
  void write(const wordset &);
  void write(const float &);
  void write(const unsigned[1 + LONG_WORD][END_POS]);
  template<class T> void write(const vector<T> &);
  void write(const vstring &);
  template<class X,class Y> void write(const map<X,Y> &);
  template<class X,class Y> void write(const pair<X,Y> &);
  template<class T> void write(const set<T> &);
  void write(const pdata &);
  void write(const clue_slot &);
  void write(const cdata &);
  void write(const inex_base &);
  void write(const int &);
  void write(const ultralong &);
  void write(const string &);
  void write(const POS &);
  void write(const Transition &);
  void write(const unsigned &);
  void write(const char &);
  void write(const short &);
  void write(const size_t &);
  void write(const bool &);

  void read(unsigned[1 + LONG_WORD]);
  void read(unsigned[1 + LONG_WORD][100]);
  void read(float[100]);
  template<class T> void read(wordmap<T> &);
  void read(wordset &);
  void read(float &);
  void read(unsigned[1 + LONG_WORD][END_POS]);
  template<class T> void read(vector<T> &);
  void read(vstring &);
  template<class X,class Y> void read(map<X,Y> &);
  template<class X,class Y> void read(pair<X,Y> &);
  template<class T> void read(set<T> &);
  void read(pdata &);
  void read(clue_slot &);
  void read(cdata &);
  void read(inex_base &);
  void read(int &);
  void read(ultralong &);
  void read(string &);
  void read(POS &);
  void read(Transition &);
  void read(unsigned &);
  void read(char &);
  void read(short &);
  void read(size_t &);
  void read(bool &);
};

void database::dump(const string &file_name, const string &flamingo_name,
                    const string &pflamingo_name) const
{
  cerr << "dumping database: " << file_name << ' ' << flamingo_name << ' '
       << pflamingo_name << endl;
  dumper d = checked_fopen(file_name.c_str(),"wb");
  d.write(parts_of_speech);
  d.write(roots);
  d.write(constructs);
  d.write(portmanteau_cutoff);
  d.write(flamingo_cutoff);
  d.write(mlg_tot);
  d.write(mlg_size);
  d.write(mlg_prob);
  d.write(word_to_score);
  d.write(sound_to_score);
  d.write(pos_rules);
  d.write(first_pos);
  d.write(last_pos);
  d.write(exact);
  d.write(inexact_words);
  d.write(inexact);
  d.write(exact_probs);
  d.write(is_abbr);
  for (unsigned i = 0 ; i < NUM_MULTI_DICTS ; ++i) d.write(multidicts[i]);
  d.write(moby);
  d.write(translations);
  d.write(words);
  d.write(transitions);
  d.write(reversals);
  d.write(pronunciations);
  d.write(pron_to_words);
  d.write(flamingo_words[0]);
  d.write(flamingo_words[1]);
  d.write(rebus_words);
  d.write(anagrams[0]);
  d.write(anagrams[1]);
  cerr << "main ..." << flush;
  flamingo_dump(flamingo_name,false);
  cerr << " flamingo ..." << flush;
  flamingo_dump(pflamingo_name,true);
  cerr << " pflamingo\n";
}

// Print out a status bar message indicating the percent complete.
// Times below were from watching the database load on 1/4/17.

void spinner(int i)
{
  static vector<float> observed {
    0.219615, 0.292408, 0.35664, 0.356661, 1.2668, 1.39069, 1.39114, 1.39196,
      1.39282, 8.47136, 8.51442, 19.2833, 19.2836, 19.2896, 19.2902, 19.339,
      19.3854, 19.4867, 21.6815, 21.9623, 21.9898, 22.0162, 23.2878, 24.1329,
      24.8044, 25.9402, 25.9436, 26.211, 27.0343, 51.1683, 52.0285 
      };
  float total = observed.back();
  if (i < 0 || i >= observed.size()) i = observed.size() - 1;
  cout << "**GUI spinner " << observed[i] / total << endl;
}

void time_and_spin(const string &desc, bool verbose, bool gui)
{
  static unsigned spin = 0;
  if (verbose) cout << "read " << desc << " @ " << get_cpu_time() << endl;
  if (verbose || gui) spinner(spin++);
}

// Read the database.  If gui is true, print out supporting progress
// messages.

void database::read(const string &file_name, const string &flamingo_name,
                    const string &pflamingo_name, bool verbose, bool gui)
{
  dumper d = checked_fopen(file_name.c_str(),"rb");
  d.read(parts_of_speech); time_and_spin("POS info",verbose,gui);
  d.read(roots);         time_and_spin("word roots",verbose,gui);
  d.read(constructs);    time_and_spin("root derivatives",verbose,gui);
  d.read(portmanteau_cutoff);
  d.read(flamingo_cutoff);
  d.read(mlg_tot);
  d.read(mlg_size);
  d.read(mlg_prob);      time_and_spin("basic word list",verbose,gui);
  d.read(word_to_score); time_and_spin("word scores",verbose,gui);
  d.read(sound_to_score); time_and_spin("sound scores",verbose,gui);
  d.read(pos_rules);     time_and_spin("POS from clue",verbose,gui);
  d.read(first_pos);     time_and_spin("POS from first word",verbose,gui);
  d.read(last_pos);      time_and_spin("POS from last word",verbose,gui);
  d.read(exact);         time_and_spin("exact clues",verbose,gui);
  d.read(inexact_words); time_and_spin("inexact words",verbose,gui);
  d.read(inexact);       time_and_spin("inexact cluies",verbose,gui);
  d.read(exact_probs);   time_and_spin("clue probabilities",verbose,gui);
  d.read(is_abbr);       time_and_spin("abbreviation indicators",verbose,gui);
  for (unsigned i = 0 ; i < NUM_MULTI_DICTS ; ++i) {
    d.read(multidicts[i]);
    ostringstream d;
    d << "multiword (" << i << ") dictionary";
                         time_and_spin(d.str(),verbose,gui);
  }
  d.read(moby);          time_and_spin("moby",verbose,gui);
  d.read(translations);  time_and_spin("translations",verbose,gui);
  d.read(words);         time_and_spin("clue words",verbose,gui);
  d.read(transitions);   time_and_spin("transitions",verbose,gui);
  d.read(reversals);     time_and_spin("reverse transitions",verbose,gui);
  d.read(pronunciations); time_and_spin("pronunciations",verbose,gui);
  d.read(pron_to_words); time_and_spin("pronunciation from word",verbose,gui);
  d.read(flamingo_words[0]);
                         time_and_spin("flamingo database 1",verbose,gui);
  d.read(flamingo_words[1]);
                         time_and_spin("flamingo database 2",verbose,gui);
  d.read(rebus_words);   time_and_spin("rebus words",verbose,gui);
  d.read(anagrams[0]);
  d.read(anagrams[1]);   time_and_spin("anagrams",verbose,gui);
  build_flamingo_weights();
  flamingo_read(flamingo_name,false);
  flamingo_read(pflamingo_name,true);
                         time_and_spin("flamingo internals",verbose,gui);
}

// Each of the individual dumpers (write or read) just goes through
// the various elements of the object in question, writing or reading
// each.

template<class T> void dumper::write(const wordmap<T> &x)
{
  write(x.keys);
  for (unsigned i = 0 ; i <= LONG_WORD ; ++i) write(x.values[i]);
}

template<class T> void dumper::write(const vector<T> &x)
{
  size_t n = x.size();
  fwrite(&n,sizeof(size_t),1,f);
  for (size_t i = 0 ; i < n ; ++i) write(x[i]);
}

template<class X,class Y> void dumper::write(const map<X,Y> &x)
{
  size_t n = x.size();
  fwrite(&n,sizeof(size_t),1,f);
  for (typename map<X,Y>::const_iterator i = x.begin() ; i != x.end() ; ++i) {
    write(i->first);
    write(i->second);
  }
}

template<class X,class Y> void dumper::write(const pair<X,Y> &x)
{
  write(x.first);
  write(x.second);
}

template<class T> void dumper::write(const set<T> &x)
{
  size_t n = x.size();
  fwrite(&n,sizeof(size_t),1,f);
  for (typename set<T>::const_iterator i = x.begin() ; i != x.end() ; ++i) 
    write(*i);
}

void dumper::write(const vstring &v)  { write(vector<unsigned>(v)); }
void dumper::write(const char &x)     { fwrite(&x,sizeof(char),1,f); }
void dumper::write(const short &x)    { fwrite(&x,sizeof(short),1,f); }
void dumper::write(const POS &x)      { fwrite(&x,sizeof(POS),1,f); }
void dumper::write(const Transition &x) { fwrite(&x,sizeof(Transition),1,f); }
void dumper::write(const unsigned &x) { fwrite(&x,sizeof(unsigned),1,f); }
void dumper::write(const int &x)      { fwrite(&x,sizeof(int),1,f); }
void dumper::write(const float &x)    { fwrite(&x,sizeof(float),1,f); }
void dumper::write(const size_t &x)   { fwrite(&x,sizeof(size_t),1,f); }
void dumper::write(const bool &x)     { fwrite(&x,sizeof(bool),1,f); }

void dumper::write(const string &x) // include terminating 0
{
  fwrite(x.c_str(),sizeof(char),x.size() + 1,f); 
}

void dumper::write(const unsigned x[1 + LONG_WORD])
{
  fwrite(x,sizeof(unsigned),1 + LONG_WORD,f);
}

void dumper::write(const unsigned x[1 + LONG_WORD][100])
{
  fwrite(x,sizeof(unsigned),100 * (1 + LONG_WORD),f);
}

void dumper::write(const float x[100])
{
  fwrite(x,sizeof(float),100,f);
}

void dumper::write(const unsigned x[1 + LONG_WORD][END_POS])
{
  fwrite(x,sizeof(unsigned),(1 + LONG_WORD) * END_POS,f);
}

void dumper::write(const pdata &x)
{
  write(x.ct);
  fwrite(x.pos,sizeof(float),END_POS,f);
}

void dumper::write(const cdata &x)
{
  write(x.ct);
  write(x.singletons);
  write(x.fills);
}

void dumper::write(const inex_base &i)
{
  fwrite(&i.words,sizeof(unsigned),4,f);
}

void dumper::write(const wordset &x)
{
  write(x.w1);
  for (unsigned i = 0 ; i < 2 ; ++i) write(x.w2[i]);
  for (unsigned i = 0 ; i < 3 ; ++i) write(x.w4[i]);
  for (unsigned i = 0 ; i < 6 ; ++i) write(x.w8[i]);
  for (unsigned i = 0 ; i < LONG_WORD - 12 ; ++i) write(x.w16[i]);
}

void dumper::write(const ultralong &x)
{
  write(x.hi);
  write(x.lo);
}

void dumper::write(const clue_slot &x)
{
  write(x.clue);
  write(x.len);
}

template<class T> void dumper::read(wordmap<T> &x)
{
  read(x.keys);
  for (unsigned i = 0 ; i <= LONG_WORD ; ++i) read(x.values[i]);
}

template<class T> void dumper::read(vector<T> &x)
{
  size_t n;
  fread(&n,sizeof(size_t),1,f);
  x.resize(n);
  for (size_t i = 0 ; i < n ; ++i) read(x[i]);
}

void dumper::read(vstring &v)
{
  vector<unsigned> *u = &v;
  read(*u);
}

template<class X,class Y> void dumper::read(map<X,Y> &m)
{
  m.clear();
  size_t n;
  fread(&n,sizeof(size_t),1,f);
  X x;
  Y y;
  for (size_t i = 0 ; i < n ; ++i) {
    read(x);
    read(y);
    m[x] = y;
  }
}

template<class X,class Y> void dumper::read(pair<X,Y> &p)
{
  read(p.first);
  read(p.second);
}

// constructing a vector and then converting to a set does not appear
// to be faster

template<class T> void dumper::read(set<T> &x)
{
  x.clear();
  size_t n;
  fread(&n,sizeof(size_t),1,f);
  T t;
  for (size_t i = 0 ; i < n ; ++i) {
    read(t);
    x.insert(t);
  }
}

void dumper::read(float &x)    { fread(&x,sizeof(float),1,f); }
void dumper::read(char &x)     { fread(&x,sizeof(char),1,f); }
void dumper::read(short &x)    { fread(&x,sizeof(short),1,f); }
void dumper::read(size_t &x)   { fread(&x,sizeof(size_t),1,f); }
void dumper::read(bool &x)     { fread(&x,sizeof(bool),1,f); }
void dumper::read(POS &x)      { fread(&x,sizeof(POS),1,f); }
void dumper::read(Transition &x) { fread(&x,sizeof(Transition),1,f); }
void dumper::read(unsigned &x) { fread(&x,sizeof(unsigned),1,f); }
void dumper::read(int &x)      { fread(&x,sizeof(int),1,f); }

void dumper::read(string &x) 
{
  x.clear();
  char ch;
  for ( ;; ) {
    fread(&ch,sizeof(char),1,f);
    if (ch == 0) break;
    x.push_back(ch);
  }
}

void dumper::read(unsigned x[1 + LONG_WORD])
{
  fread(x,sizeof(unsigned),1 + LONG_WORD,f);
}

void dumper::read(unsigned x[1 + LONG_WORD][100])
{
  fread(x,sizeof(unsigned),100 * (1 + LONG_WORD),f);
}

void dumper::read(float x[100])
{
  fread(x,sizeof(float),100,f);
}

void dumper::read(unsigned x[1 + LONG_WORD][END_POS])
{
  fread(x,sizeof(unsigned),(1 + LONG_WORD) * END_POS,f);
}

void dumper::read(pdata &x)
{
  read(x.ct);
  fread(x.pos,sizeof(float),END_POS,f);
}

void dumper::read(inex_base &i)
{
  fread(&i.words,sizeof(unsigned),4,f);
}

void dumper::read(cdata &x)
{
  read(x.ct);
  read(x.singletons);
  read(x.fills);
}

void dumper::read(wordset &x)
{
  read(x.w1);
  for (unsigned i = 0 ; i < 2 ; ++i) read(x.w2[i]);
  for (unsigned i = 0 ; i < 3 ; ++i) read(x.w4[i]);
  for (unsigned i = 0 ; i < 6 ; ++i) read(x.w8[i]);
  for (unsigned i = 0 ; i < LONG_WORD - 12 ; ++i) read(x.w16[i]);
}

void dumper::read(ultralong &x)
{
  read(x.hi);
  read(x.lo);
}

void dumper::read(clue_slot &x)
{
  read(x.clue);
  read(x.len);
}
