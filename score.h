#ifndef _SCORE_
#define _SCORE_

// The basic structure is that this header file contains everything
// needed for databases, heuristic scoring, etc.  The header file xw.h
// contains what's needed for puzzle filling and search.

// NOTE: IN THIS AND MUCH OTHER CODE, POS STANDS FOR "PART OF SPEECH".
// IT TYPICALLY DOES NOT STAND FOR "POSITION".

// Note also that this file is included in much more than Dr.Fill (for
// example, it's included in much of the wordlist stuff).  So just
// because a constant isn't used in Dr.Fill doesn't mean that you can
// delete it.  As an example, CLUE_START isn't used anywhere in
// Dr.Fill but is used by wordlist/cluespos.cpp

#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <numeric>
#include <cassert>
#include <algorithm>
#include <mutex>
#include <thread>
#include <string.h>             // memset
#include "bits.h"

using namespace std;

#define CLUE_START 37           // where clue starts in clue file
#define CLUE_BY_POS_CUTOFF 7    // detect a pattern if this many clues
                                // w/ same structure

// word cutoffs
#define PORTMANTEAU_COUNT 575000 // # possibilities to consider in portmanteau
#define MULTI_COUNT       625000 // # words to consider as multiwords
#define FLAMINGO_COUNT    950000 // # words to include in Flamingo
#define MLG_COUNT        8000000 // # words to consider at all

#define NUM_MULTI_DICTS 3       // number of multiword dictionaries
#define SEGMENTS 4              // max length n-gram to match in DB construction
#define SERIALIZATION_MAX 100   // if this many possible parses, give up
#define MAX_RESPONSE 400        // don't display answers worse than this
#define BIG_CHARGE 10000        // random big penalty
#define BIG_SCORE (1000 * BIG_CHARGE) // random big score for puzzle
#define LONG_WORD 23            // maximum word length
#define BRANCH_CHECK 1000       // maximum number of words considered per slot
#define MAX_BLANKS 3            // maximum number of .s in multiword search
#define MAX_BLANKS2 2           // maximum number in big dict multiword search
#define ABBR_LIM 5              // longest abbrev
#define BERKELEY_SCALE 0.356    // probability scaling factor
#define GUI_TOP 3               // show this many fills in GUI
#define REBUS_MIN 30            // need 30 secs to consider it
#define REBUS_MAX_LENGTH 5      // maximum length of single rebus fill
                                // additional rebus parameters in rebus.cpp
#define MINUTE_THRESHOLD 10     // only terminate within this many secs of minute
#define SIZE_SCALE 4            // scale factor for end of minute's dep. on size
#define INITIAL_TIME 20         // always think for this long
#define MAX_OLD_DELTA 10        // theme word not in dictionary if there's 
                                // something this much better
#define MAX_OLD_DIFF 2          // if it's at least this close
#define MUST_BE_THEMED 12       // this long guaranteed to be theme
#define MAX_THEME_LENGTH 10     // this long assumed to be theme on first pass
#define THEME_LENGTH 9          // this long assumed to be theme
#define MIN_THEME_LENGTH 8      // ... but could be this long if few longer words
#define MAX_THEME_FRACTION 10   // if #themed * this > size, it's too many
#define FLAMINGO_MATCH 5        // consider this many alternatives in fill
#define FLAMINGO_LENGTH 7       // only look for Flamingo mod if this long
#define OVERLAP_LIMIT 5         // max ovelap in a portmanteau
#define PORTMANTEAU_MINIMUM 3   // need this many matches for portmanteau theme
#define ANAGRAM_MINIMUM 2       // need this many matches for anagram theme
#define SMALL_ANAGRAM 7         // shortest allowed anagram
#define ANAGRAM_CUTOFF 50       // minimum word score
#define ADD_LIMIT 4             // max # of additions in an "add" theme
#define MIN_THEME 3             // min # of theme entries
#define THEME_SUGGESTIONS 10    // # of theme-based replacements to suggest (max)
#define THEME_STRONG 7          // decrement to multiword score matching theme
#define THEME_WEAK 3
#define THEME_REQD 3            // addl bonus if opposite entry themed
#define SOUND_ADJUST 1          // number of "random" sources in a sound theme
#define RUNON_REQD 6            // at least one word has to be this long
#define GEOM_FLAMINGO_LIMIT 2   // even flamingo can't change more than this
#define GEO_CUTOFF 15           // minimum improvement for a geometry word
#define GEO_SHORT_LIMIT 6       // at least one word in a geo must be this long
#define THEME_MULTI_CUTOFF 45   // cutoff to include a word in a theme
#define DEFAULT_PROGRESS 30     // must make progress in this many secs
#define DEFAULT_LIMIT 900       // must finish in this many secs

#ifdef __linux__
#define TICKS 1.0e9
#else
#define TICKS 1.0e6
#endif

static const int BIG_PRIME = 1000003;
static const vector<unsigned> BOGGLE_ERRORS { 0, 20 };

#include "solvergui.h"

struct dumper;
extern bool debug;                // occasionally used for debugging

template<class T>
static ostream &operator<<(ostream &os, const vector<T> &v)
{
  for (auto &i : v) os << ((&i == &v[0])? "" : " ") << i;
  return os;
}

template<class T>
static ostream &operator<<(ostream &os, const set<T> &s)
{
  for (auto &i : s) os << ((&i == &*s.begin())? "" : " ") << i;
  return os;
}

template<class T, class U>
static ostream &operator<<(ostream &os, const map<T,U> &m)
{
  for (auto &i : m)
    os << (&i == &*m.begin()? "" : " ") << i.first << " (" << i.second << ')';
  return os;
}

template<class T>
vector<T> &uniquify(vector<T> &vec, bool dosort = true)
{
  if (dosort) sort(vec.begin(),vec.end());
  vec.erase(unique(vec.begin(),vec.end()),vec.end());
  return vec;
}

template<class T>
bool has_dups(const vector<T> &vec)
{
  return set<T>(vec.begin(),vec.end()).size() != vec.size();
}

// A character is a rebus character if it's after 'z' but still legal.

static inline bool rebus_char(char c) { 
  return c >= 'a' + NUMLETS && c < WILD;
}

// Not surprisingly, a lot of the data that we work with consists of
// either sets of words or maps from words to other things.  In
// addition, we will generally construct these sets or maps when the
// database is developed or read in and then never modify it, so we
// don't need to maintain the overhead of a balanced tree once the set
// or map is set up.  We can just use a sorted vector.  We don't use a
// hash table because of the space overhead involved (because the fill
// won't be completely dense, not to mention the data structure
// itself), and because there don't appear to be time savings to be
// had here; the binary search involved in the lookup does not appear
// in profiling.

// We do some compression as well, using 5 bits per character (always
// assumed, without checking, to be a-z, a rebus character or WILD).
// So a 1-character word fits in one byte.  A 2-3 character word fits
// in 15 bits, two bytes.  A 4-6 character word fits in an unsigned
// (assumed to be 4 bytes).  A 7-12 character word fits in 60 bits, a
// size_t on the presumed 64-bit OS.  Longer words (there hopefully
// aren't that many) use an ultralong, which is just space for 128
// bits.  But we also record in the ultralong the length of the word
// being encoded (say up to 31, 5 bits), so we only have 123 bits
// available really, enough for a word of length 24.  Working with
// longer words will have a variety of hard to predict and disastrous
// consequences.

// In order to support sorting, the layout of an ultralong is that the
// characters that don't fill a word are in the low-order bits of that
// word.  So for a one-character word, that character is in the low
// order 5 bits of hi.  For a two-character word, it's the low-order
// 10 bits of hi, etc.  The next characters are stored in as many
// low-order bits of _lo_ as needed, so if there are 13 characters in
// total, the 13th is stored in bits 0-4 of lo.  If there are 14
// characters, the 13th moves to bits 5-9 of lo with the 14th in bits
// 0-4 of lo.

class ultralong {
  size_t lo, hi;

public:
  ultralong() : lo(0), hi(0) { }
  ultralong(size_t len, size_t h, size_t l) : lo(l), hi(h) { lo |= (len << 55); }
  ultralong(const string &s);

private:
  size_t gethi() const { return hi; }

public:
  bool operator==(const ultralong &other) const {
    return hi == other.hi && lo == other.lo; 
  }
  bool operator!=(const ultralong &other) const { return !operator==(other); }
  bool operator<(const ultralong &other) const {
    return hi < other.hi || (hi == other.hi && lo < other.lo); 
  }
  unsigned  length() const { return lo >> 55; }
  operator  string() const;
  char      get_char(unsigned n) const;
  void      set_char(unsigned n, char c);
  bool      match(const ultralong &other, unsigned n) const;
  friend ostream& operator<< (ostream& os, const ultralong& u) {
    return os << hex << u.hi << '+' << u.lo << dec; 
  }
  friend struct dumper;
  friend class wordset;
};

class database;

// The multiword stuff is nasty, in part because we need it to be
// fast.  When we analyze a multiword, we return a vector of unsigned
// chars.  This is the length of each word in the multiword.  That's
// called a _segmentation_.

struct segmentation : vector<unsigned char> {
  segmentation(unsigned n = 0, unsigned u = 0) : vector<unsigned char>(n,u) { }
  segmentation operator+=(const segmentation &other) { // append
    insert(end(),other.begin(),other.end());
    return *this;
  }
  segmentation operator+(const segmentation &other) const {
    segmentation ans = *this;
    return ans += other;
  }
};

// Now imagine that we have a partial fill; we want to construct all
// the multiwords that complete it, along with an alaysis of each.
// That's a _splitting_.

struct splitting : multimap<string,segmentation> {
  splitting() { }
  splitting operator+=(const splitting &other) {
    insert(other.begin(),other.end());
    return *this;
  }
  splitting operator+(const splitting &other) const {
    splitting ans = *this;
    return ans += other;
  }
};

enum POS {                          // parts of speech
  ROOT_NOUN, PLURAL_NOUN,
  ROOT_ADVERB, NONROOT_ADVERB,
  ROOT_VERB, PARTICIPLE_VERB, SINGULAR_VERB, PAST_VERB,
  ROOT_ADJECTIVE, MOST_ADJECTIVE, MORE_ADJECTIVE, CONSTRUCTED_ADJECTIVE,
  ARTICLE, PREPOSITION, CONJ, POSSESSOR, BLANK, UNKNOWN, END_POS
};

enum PNoun { FAIL, ACCEPT, NOUNIFY }; // what to do with an apparent proper noun

// Ways that a word can change into another word.

// root_noun -> plural_noun     make_plural
// plural_noun -> root_noun     make_single
// root_adv -> nonroot_adv      make_nonroot_adj
// nonroot_adv -> root_adv      make_root_adj
// root_verb -> participle_verb make_participle
// root_verb -> singular_verb   make_single
// root_verb -> past_verb       make_past
// part_verb -> root_verb       make_non_participle
// part_verb -> singular_verb   make_single_non_participle
// part_verb -> past_verb       make_past_non_participle
// singular_verb -> root_verb   make_plural
// singular_verb -> part_verb   make_plural_participle
// singular_verb -> past_verb   make_plural_past
// past_verb -> root_verb       make_present
// past_verb -> singular_verb   make_present_single
// past_verb -> part_verb       make_present_participle
// root_adj -> more_adj         make_more
// root_adj -> most_adj         make_most
// root_adj -> constructed_adj  make_const_adj
// more_adj -> root_adj         make_no_compare
// more_adj -> most_adj         make_most
// more_adj -> constructed_adj  make_const_adj
// most_adj -> root_adj         make_no_compare
// most_adj -> more_adj         make_most
// most_adj -> constructed_adj  make_const_adj
// constructed_adj -> root_adj  make_no_const_adj
// constructed_adj -> more_adj  make_more
// constructed_adj -> most_adj  make_most

enum Transition {
  PLURAL, SINGLE,
  NONROOT_ADJ, ROOT_ADJ,
  PARTICIPLE, PAST, NON_PARTICIPLE, SINGLE_NON_PARTICIPLE, PAST_NON_PARTICIPLE,
    PLURAL_PARTICIPLE, PLURAL_PAST, PRESENT, PRESENT_SINGLE, PRESENT_PARTICIPLE,
  MORE, MOST, CONST_ADJ, NO_COMPARE, NO_CONST_ADJ, SYNONYM
};

// A map from a string to derivative words, each a new string and
// Transition type.

struct transition_map : map<string,vector<pair<string,Transition>>> {
};

// A transition_map, but with word IDs instead of strings.

struct vtransition_map : map<unsigned,vector<pair<unsigned,Transition>>> {
  vtransition_map() { }
  vtransition_map(const transition_map &m, const database &db);

  vtransition_map reverse() const;
};

// A parse is just a vector<POS>, each entry being the POS of a word

struct parse : vector<POS> {
  parse() { }
  parse(const vector<POS> &v) : vector<POS>(v) { }
  parse(const string &clue, PNoun nouner, const map<string,POS> &parsed);
};

// When we analyze a multiword, we get both a segmentation (as above)
// and, eventually, a vector<vector<char>> which gives the possible
// parses for the subwords in the multiword.  This is a _multiparse_

struct multiparse : map<segmentation,vector<parse>> {
};

// Data structure where we have some construct that has appeared
// multiple times, each appearance with a specific pos.  This gives
// the total count and the relative frequency of each pos.

struct pdata {
  unsigned      ct;             // number of instances
  float         pos[END_POS];   // probability of each possible POS for answer

  pdata(unsigned c = 0) : ct(c) { memset(pos,0,END_POS * sizeof(float)); }
  pdata(const map<char,unsigned> &counts);
  pdata(unsigned c, istringstream &ist);
  pdata(const map<char,float> &m);

  float         convolve(const pdata &other) const;
  void          merge(map<char,float> &res) const;
  void          normalize();
  float         mass() const;
};

// And a _multidata_ is a map from a segmentation to a pdata (the parse).

struct multidata : map<segmentation,pdata> {
  bool separates(unsigned sz, bool beginning) const;
};

// set of words, assumed to be sorted.  The [] operator returns either
// the position in the list or (unsigned) -1.

// NOTE: This code needs to be as fast as possible, so there is no
// error checking done at any point.  The result is that if you try to
// save a word that includes an illegal character (e.g., a digit),
// horrible and unpredictable things will happen.  The allowed
// characters are a-z, WILD and the rebus characters.

// NOTE: The entries of any given length must be sorted when a
// constructor is called.

class wordset {
  vector<char>      w1;                  // one-letter strings (1 byte each)
  vector<short>     w2[2];               // 2-3 letter strings (2 bytes each)
  vector<unsigned>  w4[3];               // 4-6 letter strings (4 bytes each)
  vector<size_t>    w8[6];               // 7-12 letter strings (8 bytes each)
  vector<ultralong> w16[LONG_WORD - 12]; // 13-n letter strings (16 bytes each)

  void build(const vector<const string *> words[1 + LONG_WORD]);
  // used by constructors

public:
  wordset() { }
  wordset(vector<string> &v);
  wordset(const set<string> &s);
  wordset(const set<string> v[1 + LONG_WORD]);    // split by length
  wordset(const vector<string> v[1 + LONG_WORD]); // split by length
  wordset(const vector<set<string>> &v);
  ultralong getultra(unsigned idx, unsigned len) const;
  string getword(unsigned idx, unsigned len) const { return getultra(idx,len); }
  unsigned size(unsigned len) const;              // number of words of given len
  size_t size() const;                            // total number of words

  unsigned operator[](const ultralong &s) const;  // return index or -1
  unsigned lower_bound(const ultralong &s) const; // similar
  unsigned upper_bound(const ultralong &s) const;
  bool     includes(const ultralong &s) const {   // is the ultralong in the set?
    return operator[](s) != (unsigned) -1; 
  }

  unsigned operator[](const string &s) const {    // all similar, strings as args
    if (s.size() > LONG_WORD) return (unsigned) -1;
    return operator[](ultralong(s)); 
  }
  unsigned lower_bound(const string &s) const {
    return lower_bound(ultralong(s)); 
  }
  unsigned upper_bound(const string &s) const {
    return upper_bound(ultralong(s)); 
  }
  bool     includes(const string &s) const {
    return operator[](s) != (unsigned) -1; 
  }

  // These two functions find words in the multiset that match the
  // input, with WILD in the input replaced with an actual letter.
  // Used for dealing with multiword fill.
  set<string>    ground_word(const string &ww) const;
  set<ultralong> ground_word(ultralong &l, ultralong &h, unsigned idx) const;
  friend struct dumper;
};

// A wordmap is essentially a map on words.  The simplest way to do it
// is to have two lists, the first the keys and the second all of the
// values (by length). 

// The [] operator can always be called but not used for setting.  If
// the key is missing a default value is returned.  You can modify the
// result returned, but if you modify the default value, bad things
// will happen.

// NOTE: Because the constructors all take maps, the requirement that
// the keys are sorted (for the wordset constructors) will be
// satisfied by the walk through those maps.

template<class T>
class wordmap {
  wordset       keys;
  vector<T>     values[1 + LONG_WORD];
public:
  wordmap() { }
  wordmap(const map<string,T> &m) {
    vector<string> k[1 + LONG_WORD];
    for (auto &i : m) {
      if (i.first.size() > LONG_WORD) continue;
      k[i.first.size()].push_back(i.first);
      values[i.first.size()].push_back(i.second);
    }
    keys = k;
  }
  wordmap(const map<string,T> m[1 + LONG_WORD]) {
    vector<string> k[1 + LONG_WORD];
    for (size_t j = 0 ; j < 1 + LONG_WORD ; ++j)
      for (auto &i : m[j]) {
        k[j].push_back(i.first);
        values[j].push_back(i.second);
      }
    keys = k;
  }
  bool includes(const string &s) const { return keys.includes(s); }
  const T &operator[](const string &s) const {
    static T t;                 // can't return T() because you return a 
    unsigned idx = keys[s];     // reference
    if (idx == (unsigned) -1) return t;
    return values[s.length()][idx];
  }
  T &operator[](const string &s) {
    static T t;
    unsigned idx = keys[s];
    if (idx == (unsigned) -1) return t;
    return values[s.length()][idx];
  }
  const wordset &get_keys() const { return keys; }
  string get_key(unsigned len, unsigned idx) const {
    return keys.getword(idx,len); 
  }
  const T &get_value(unsigned len, unsigned idx) const {
    return values[len][idx]; 
  }
  T &get_value(unsigned len, unsigned idx) { return values[len][idx]; }
  size_t size(unsigned len) const { return values[len].size(); }
  size_t size() const { return keys.size(); }

  friend struct dumper;
};

extern vector<string> pos_names;

// We often map something about the clue to something about the
// answer.  The domain of this map can be a clue_slot (the clue
// itself, together with the length of the fill) or a segment of the
// clue (just a collection of words, separated by WILD)

struct clue_slot {
  string        clue;
  unsigned      len;            // length of desired fill

  clue_slot() : len(0) { }
  clue_slot(const string &c, const string &w) : clue(c), len(w.size()) { }
  // needed by filldata
  clue_slot(const string &c, unsigned l) : clue(c), len(l) { }

  bool operator< (const clue_slot &other) const {
    if (len != other.len) return len < other.len;
    return clue < other.clue;
  }
  bool operator==(const clue_slot &other) const {
    return clue == other.clue && len == other.len;
  }
  bool operator!=(const clue_slot &other) const { return !operator==(other); }
};

// And here is what we typically map *to*.  There is a number of times
// this feature matched and then a map of fills, giving the match and
// the number of times it was used.  We also record how many of those
// matches are singletons.

// In general, we build cdata's up from strings of the form

//   s str1 ct1 ... strn ctn

// where s is the number of singleton fills (in general, we don't care
// about the singleton strings because we have no idea about their
// predictive value).

struct cdata {
  unsigned      ct;             // # matches
  int           singletons;     // # with only one match
  map<string,unsigned> fills;   // # matches for each match

  cdata() : ct(0) { }
  cdata(vector<string> toks) {
    ct = stoi(toks[0]);
    unsigned sum = 0;
    for (size_t i = 1 ; i < toks.size() ; i += 2) {
      unsigned n = stoi(toks[i + 1]);
      fills[toks[i]] = n;
      if (n > 1) sum += n;
    }
    singletons = ct - sum;      // if reduced, missing entries assumed singletons
  }

  cdata(unsigned c, int s) : ct(c), singletons(s) { }

  bool operator==(const cdata &other) const { return fills == other.fills; }
  bool operator!=(const cdata &other) const { return !operator==(other); }

  const cdata reduce() const;   // needed by filldata

  // new match found; stick it in the appropriate place (used in makestats only)
  void insert(const string &f) {
    ++ct;
    ++fills[f];
  }
};

struct basic_datum;

// various parameters in the evaluation

// CBOUND               clue-based charge cannot exceed this
// PBOUND               pos-based charge cannot exceed this
// FBOUND               word-based charge cannot exceed this
// PWEIGHT              weight for pos charge relative to clue-based
// FWEIGHT              weight for mlg word charge relative to clue-based
// INEXACT              relative weights for inexact features
// ABBRV_LIMIT          limit on certainty that a word is/is not an abbreviation
// IS_ABBRV             penalty for being an abbrv when it's wrong
// SCALE_BY_CROSSERS    divide total charge by # words affected times this
// THREE_LETTERS        divide value for three letter word by this
// NON_DICT_CHARGE      charge for fill not in dictionary
// NON_DICT_TRANSITION  scaling for non-dictionary fill starts here
// NON_MULTI_CHARGE     charge for fill not even a multiword
// PWEIGHT_M            scale for pos analysis of multiword
// DICT_CHARGE          charge for using dictionary 1
// WORD_CHARGE          charge for multiword
// EXTRA_WORD_CHARGE    extra charge per word in multiword
// MATCHBASE            base factor for word in multiword matching clue
// UNMATCH              incremental factor for not matching clue
// FITB_BONUS           score delta for FITB analysis
// FITB_LIMIT           maximum number of returns for an FITB clue
// FITB_SCORE           score for a wiki-based FITB
// PAREN_BONUS          in paren clue, bonus for a fill that matches paren
// CLUE_BONUS           bonus for match from clue or modified clue
// MULTI_ROOT           credit for a multiword that seems to be derived
// MAXIMUM_MATCH        max probability ascribed to an exact
// MINIMUM_MATCH        min probability ascribed to an exact
// MOBY_BONUS           bonus if suggested by Moby thesaurus
// VERTICAL             multiplicative factor for vertical (or short) words
// NO_PARSE             penalty for no multiword parse

// If you make a change, you have to do it in five places:
// 1. Here
// 2. In params::params in tune.cpp, where it needs a value
// 3. In param_names in tune.cpp, where it needs a description
// 4. In tune_scoring and reset_scoring in tune.cpp, which might need
//    addition to lists of reinitialization factors
// 5. In autotune_adjustments in tune.cpp, where you'll need to adjust the
//    autotune iterations.

enum { CBOUND, PBOUND, FBOUND, PWEIGHT, FWEIGHT, INEXACT, ABBRV_LIMIT, IS_ABBRV, 
       SCALE_BY_CROSSERS, THREE_LETTERS, NON_DICT_CHARGE, NON_DICT_TRANSITION,
       NON_MULTI_CHARGE, PWEIGHT_M, DICT_CHARGE, WORD_CHARGE, EXTRA_WORD_CHARGE,
       MATCHBASE, UNMATCH, FITB_BONUS, FITB_LIMIT, FITB_SCORE,
       PAREN_BONUS, CLUE_BONUS, MULTI_ROOT, MAXIMUM_MATCH, MINIMUM_MATCH,
       MOBY_BONUS, VERTICAL, NO_PARSE, BERKELEY, PARAM_END };

struct tuning_fit;

class params {
  float         details[PARAM_END];

public:
  bool          sh;             // compress display (public access)
  params();

  float compute_score(const basic_datum &bd) const;
  bool param_next(const params &begin, const params &end, 
                  const params &interval); // tuning
  vector<tuning_fit> autotune_adjustments() const;

  float   detail(unsigned which) const { return details[which]; }
  float & detail(unsigned which)       { return details[which]; }
};

// The fills generally are based on sorted lists, where each list
// entry includes both a fill and the score thereof.  Sometimes, we
// want to walk the list by score.  Other times, we want to search for
// a specific fill.  The way we support this is by first, having one
// map from strings to scores (it's a wordmap, of course), which we
// can use to walk through alphabetically.  Then we have another
// vector which gives the indices in the first vector of the same
// list, sorted by score.  This vector allows us to walk the list by
// score if we want.

// We will also often want to record the reason for the score and the
// required information is a basic_datum.

// A basic_datum contains information about how likely a specific clue
// is to lead to a specific fill.  The fields are an overall
// probability and then a bunch of individual contributing values.

struct basic_datum {
  float         score;        // overall score
  float         pos;          // contribution from POS
  float         dict;         // contribution from dictionary
  float         fabbr;        // contribution from abbrv consideration
  float         clue;         // contribution from clue
  float         berkeley;     // Berkeley-computed value
  unsigned char paren, fitb, abbr, moby, is_dict, nonword, exactp;

  basic_datum() : score(BIG_CHARGE), pos(1), dict(0), fabbr(1), clue(0),
                  berkeley(BIG_CHARGE), paren(false), fitb(false), abbr(false),
                  moby(false), is_dict(false), nonword(true), exactp(false) { }
  
  basic_datum(float z, bool is_abbr = false, float fa = 1, bool ftb = false,
              const basic_datum &s = basic_datum());

  basic_datum operator*=(float x) { clue *= x; return *this; }
  basic_datum operator+=(const basic_datum &other) {
    clue += other.clue;
    return *this;
  }
};

// Structure to use when sorting by score.  Includes a word, the
// score, and the index of the word in the word list (so we know where
// things are in the overall data structure).

struct scored_word_sorter {
  float         score;
  string        word;
  unsigned      index;

  scored_word_sorter() : score(0), index(0) { }
  scored_word_sorter(const string &w, float s) : score(s), word(w), index(0) { }
  bool operator<(const scored_word_sorter &other) const {
    if (score != other.score) return score < other.score;
    return word < other.word;
  }
  bool operator==(const scored_word_sorter &other) const {
    return score == other.score && word == other.word;
  }
};

// When we include both the word and the full score detail, we can
// assume that any two entries with the same word are equal, since
// there won't ever be two such entries in one list.

struct scored_word : basic_datum {
  string        word;

  scored_word() { }
  scored_word(const string &w, float s, const basic_datum &bd = basic_datum()) : 
    basic_datum(bd), word(w) { score = s; }
  scored_word(const scored_word_sorter &s) : word(s.word) { score = s.score; }
  bool operator< (const scored_word &other) const { return word < other.word; }
  bool operator==(const scored_word &other) const { return word == other.word; }
  bool operator!=(const scored_word &other) const { return !operator==(other); }
};

static const scored_word hopeless("",BIG_CHARGE);

// As we compute multiword splits, we need to keep track of what
// worked and what didn't.  A split_cache does that, keeping track
// both of splittings for subsequences and whether or not any
// particular subsequence has yet been analyzed.

struct split_cache {
  set<string> groundings[LONG_WORD + 1][LONG_WORD + 1];
  char        found     [LONG_WORD + 1][LONG_WORD + 1];

  split_cache() {
    memset(found,0,(LONG_WORD + 1) * (LONG_WORD + 1));
  }
};

// When we suggest new fill based on a theme, here's how it works.  We
// start with the ORIGINAL fill that appears in the puzzle.  Then we
// find SOURCE fills that are close (using Flamingo).  Then we modify
// the SOURCE fills to get the actual suggested FILL.

// In the sound-based case, it's more complex.  Now we start with the
// ORIGINAL and get its pronunciation, PORIGINAL.  We find
// pronunciations close to PORIGINAL; those are PSOURCE.  The actual
// SOURCE is still there (because we need its score to evaluate the
// theme entry).  Then we modify the PSOURCE to get PFILL, the
// pronunciation of the suggested fill, and the actual FILL is a
// string that's pronounced like PFILL.

// In general, then we have these fields:
//  ORIGINAL
//  PORIGINAL   set to ORIGINAL for non-sound theme
//  PSOURCE     found from PORIGINAL using Flamingo
//  SOURCE      set to PSOURCE for non-sound theme; otherwise pronounced PSOURCE
//  PFILL       result of applying theme to PSOURCE
//  FILL        string that is pronounced like PFILL (or = to it for non-sound)

struct theme_fill {
  string        original;
  string        poriginal;
  string        psource;
  string        source;
  string        pfill;
  string        fill;

  theme_fill() { }

  bool operator==(const theme_fill &other) const {
    return fill == other.fill; 
  }
};

// A location in just an (x,y).  Some arithmetic operations on
// vectors, used for geometry stuff.

struct location {
  int x,y;
  location() : x(0), y(0) { }
  location(int a, int b) : x(a), y(b) { }

  bool operator==(const location &other) const {
    return x == other.x && y == other.y;
  }
  bool operator!= (const location &other) const { return !operator==(other); }
  bool operator< (const location &other) const {
    return (x == other.x)? (y < other.y) : (x < other.x);
  }

  location operator-() const { return location(-x,-y); }

  location &operator+=(const location &other) {
    x += other.x;
    y += other.y;
    return *this;
  }
  location operator+(const location &other) const {
    location ans = *this;
    return ans += other;
  }
  location &operator-=(const location &other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
  location operator-(const location &other) const {
    location ans = *this;
    return ans -= other;
  }

  void max(const location &other) {
    x = ::max(x,other.x);
    y = ::max(y,other.y);
  }

  int dot(const location &other) const { return x * other.x + y * other.y; }
};

// A square in the puzzle.  Location, current fill, and correct fill
// if known.  Also lists of words using this square (geometry themes
// may have three words using a square), and positions in each such
// word.

struct square : location {
  char             fill, answer;
  vector<unsigned> word_ids;    // ids of words using this square
  vector<unsigned> positions;   // position of this square in each such word
  square() : fill(WILD), answer(0) { }
  square(unsigned a, unsigned b, char f = WILD) :
    location(a,b), fill(f), answer(0) { }
};

// A square where a rebus is used.  Location, and direction of the
// word that uses it.

struct rebus_square : location {
  bool      down;
  rebus_square(unsigned a, unsigned b, bool d) : location(a,b), down(d) { }
  bool      operator<(const rebus_square &other) const {
    return location::operator==(other)? (down < other.down) :
      location::operator<(other);
  }
  bool      operator==(const rebus_square &other) const {
    return location::operator==(other) && down == other.down;
  }
};

// Places where a rebus is used.

struct rebus_uses : vector<rebus_square>
{
  size_t num_doubles() const;
  bool   is_doubles() const;
  bool   split_rebus(rebus_uses other) const;
};

// Value of a rebus possibility.  Actual value, along with places
// where the rebus is useful.

struct rebus_value {
  float      value;
  rebus_uses squares;
  rebus_value() : value(0) { }
  bool operator<(const rebus_value &other) const { return value > other.value; }
};

// Rebus "hint".  String to use, along with an alternate for a
// bidirectional rebus and a place to put it if there are many rebuses
// in the puzzle.

struct rebus_answer : string {
  string        alternate;
  rebus_square  sq;

  rebus_answer(const string &s = string(), const string &a = string()) : 
    string(s), alternate(a), sq(-1,-1,false) { }
};

class  word;
class  puzzle;

// chars in a filled grid, with 0 being a block

struct cgrid : vector<vector<char>> {
  cgrid() { }
  cgrid(const puzzle &puz, bool cheat = false);

  vector<char> &operator[](unsigned i) {
    return vector<vector<char>>::operator[](i);
  }
  const vector<char> &operator[](unsigned i) const {
    return vector<vector<char>>::operator[](i);
  }
  char &operator[](location i) { return (*this)[i.x][i.y]; }
  const char &operator[](location i) const { return (*this)[i.x][i.y]; }

  int cols() const { return size(); } // stored column-major
  int rows() const { return (*this)[0].size(); }
  bool contains(location l) const {
    return l.x >= 0 && l.x < cols() && l.y >= 0 && l.y < rows();
  }

  bool is_block(location l) const;
  string find_fill(const word &wd) const;
};

// Information about where you are as you do a boggle search.  Current
// position in the grid, flags regarding what you've done so far,
// point in the trie, and most recent step.

// In the constructor from a previous one, note that if dx or dy is 2
// (a jump), we set dx or dy to just 1, since that's the *next* step
// that isn't a change.

struct trie_status {
  location loc;                 // position in puzzle
  bool     changes;             // has there been a direction change?
  bool     jump;                // has there been a jump?
  unsigned misses_used;         // how many can remain?
  unsigned misses_allowed;
  int      idx;                 // current position in trie
  location delta;               // change from prior position

  trie_status(location l, bool c, bool j, unsigned m) :
    loc(l), changes(c), jump(j), misses_used(0), misses_allowed(m), idx(0),
    delta(-1,-1) { }
  trie_status(const trie_status &orig, int i, location d, bool dc = false,
              bool dj = false) :
    loc(orig.loc + d), changes(orig.changes | dc), jump(orig.jump | dj),
    misses_used(orig.misses_used), misses_allowed(orig.misses_allowed), idx(i),
    delta(d) {
    if (delta.x == 2) delta.x = 1;
    if (delta.y == 2) delta.y = 1;
  }
};

struct segment : pair<unsigned,unsigned> {
  segment(int a, int b) : pair<unsigned,unsigned>(a,b) { }

  unsigned size() const { return second - first; }
};

// A string on a "boggle board", used to analyze geometric themes.
// Basically a string, together with a vector of the locations used.

struct boggle_string : string {
  vector<location> squares;
  float            value;

  void push_back(char c, location s) {
    string::operator+=(c);
    squares.push_back(s);
  }
  void pop_back() { string::pop_back(); squares.pop_back(); }

  bool operator<(const boggle_string &other) const { return value < other.value;}
  bool operator==(const string &other) const { return other == *this; }
  // order reversal breaks loop

  string descriptor() const;

  vector<segment> segments() const;
  unsigned num_blocks(const cgrid &grid, segment p) const;
  location get_delta(unsigned a) const { return squares[a + 1] - squares[a]; }

  bool ok_extension(const cgrid &grid, location d) const;
  bool ok_string(const cgrid &grid) const;
  bool ok_word(const cgrid &grid, const trie_status &ts) const;
};

// Trie structure.  For each letter, where do you go next?

struct extender {
  int results[26];
  // lo bit is 0 if it's not a word, 1 if it's a word.  Higher bits
  // are -1 if dead, or index into array

  int  operator[](char c)  const { return results[c - 'a']; }
  int &operator[](char c)        { return results[c - 'a']; }

  extender() { memset(results,-2,sizeof(results)); }
};

// And the trie itself

struct trie : vector<extender> {
  void add(const string &w);
  int advance(int idx, char c, bool &ok) const;
  extender &operator[](int x) { return vector<extender>::operator[](x); }
  const extender &operator[](int x) const {
    return vector<extender>::operator[](x);
  }
  bool operator[](const string &w) const;

  trie() : vector<extender>(1) { }

  void clear() { vector<extender>::clear(); push_back(extender()); }
  vector<boggle_string> search(const cgrid &g, unsigned misses,
                               const set<location> &changeable) const;
  void search(const cgrid &p, const set<location> &changeable,
              const trie_status &ts, vector<boggle_string> &results,
              const boggle_string &partial = boggle_string()) const;
  void search(const cgrid &p, const set<location> &changeable, char c,
              const trie_status &ts, vector<boggle_string> &results,
              const boggle_string &partial) const;
};

// Geometry property. Does it include a diagonal segment? A bounce
// backwards? A jump?

struct geo_type {
  unsigned num_segs;
  bool     diagonal;            // has a diagonal
  bool     jump;                // has a jump
  bool     overlap;             // segments overlap
  bool     reversed;            // has up or backwards segment

  geo_type() : num_segs(0), diagonal(false), jump(false), overlap(false),
               reversed(false) { }
  geo_type(const boggle_string &bs);

  bool operator<(const geo_type &other) const {
    if (num_segs != other.num_segs) return num_segs < other.num_segs;
    if (diagonal != other.diagonal) return diagonal < other.diagonal;
    if (jump     != other.jump)     return jump < other.jump;
    if (overlap  != other.overlap)  return overlap < other.overlap;
    return reversed < other.reversed;
  }
};

struct geometry : geo_type {
  vector<boggle_string> words;
  float                 score;

  geometry(const geo_type &g = geo_type()) : geo_type(g), score(0) { }
};

struct Flamingo;
class  theme;
struct sourced_theme;
struct settings;
class  search_node;
struct fitb_db;
struct skeleton;

// Inexact information: a map from vector<unsigned> (indices into the
// inexact word list) to cdata.

struct inex_base {
  unsigned      words[SEGMENTS];

  inex_base() { memset(words,0,(SEGMENTS) * sizeof(unsigned)); }

  unsigned &operator[](unsigned idx)       { return words[idx]; }
  unsigned  operator[](unsigned idx) const { return words[idx]; }

  bool operator==(const inex_base &other) const {
    return !memcmp(words,other.words,(SEGMENTS) * sizeof(unsigned));
  }

  bool operator<(const inex_base &other) const {
    return memcmp(words,other.words,(SEGMENTS) * sizeof(unsigned)) < 0;
  }
};

struct inex : map<inex_base,cdata> {
};

// A vstring is a vector of unsigneds that corresponds to a vector of
// words.  It's a clue, basically.

struct vstring : vector<unsigned> {
  bool operator<(const vstring &other) const {
    for (unsigned i = 0 ; i < size() && i < other.size() ; ++i)
      if ((*this)[i] != other[i]) return (*this)[i] < other[i];
    return size() < other.size();
  }

  vstring(const vector<unsigned> &vec = vector<unsigned>()) :
    vector<unsigned>(vec) { }
};

// An expansion tree is what happens when you look at modifications of
// a given clue.  At each depth, there is a set of vstrings that are
// translations of the clue at depth 0.

struct expansion_tree : vector<set<vstring>> {
  expansion_tree(const vstring &v) {
    push_back(set<vstring>());
    back().insert(v);
  }
};

// The Berkeley solver just returns fills and probabilities

struct berkeley_answer {
  string        fill;
  float         value;
};

// Structure required to invoke the Berkeley solver

struct berkeley_solver {
  string                data_file, answer_file, data_ready, answer_ready;
  vector<string>        fills;
  int                   answer_max;
  int                   query_lim;
  mutable map<string,vector<vector<berkeley_answer>>> raw, results;
  // first is raw score; second converts to probabilities

  berkeley_solver(const string &pyfile = string(),
                  const string &pydir = string(), bool keep = false,
                  int amax = 1000);
  ~berkeley_solver() { clear(); }

  vector<vector<vector<berkeley_answer>>> solve(const vector<string> &queries)
    const;
  void get_probs(const vector<string> &clues) const;
  void clear();
  void clear_berkeley_cache() const { results.clear(); }
  void wait_for_semaphore() const;
  void get_berkeley_results(const puzzle &puz) const;
};

// A grammar is a piece of a database.  It contains linguistic stuff:
// parts of speech, etc.  The transitions are ways for one POS to
// become another.

class grammar {
  map<string,vector<POS>>       parts_of_speech; // word to possible POS's
  map<string,string>            roots;      // word -> root
  map<string,vector<string>>    constructs; // root -> list of derivatives
  map<pair<POS,POS>,Transition> transitions;

  grammar();
  vector<Transition> get_transitions(const string &from, const string &to) const;
  vector<pair<string,Transition>> get_transitions(const string &from) const;
  transition_map get_transitions() const;
  friend class database;
};

// data structure to hold all statistics etc.  Functions are used to
// extract information in various ways and are documented with the
// source.  roots and constructs are often indexed with clue words
// (which might not be lower case), so we don't make them wordmaps

class database : grammar, berkeley_solver {
  float          portmanteau_cutoff;       // consider in portmanteau or 
  float          flamingo_cutoff;          // flamingo if this score or higher
  unsigned       mlg_tot[(LONG_WORD + 1)]; // # words of given length
  unsigned       mlg_size[(LONG_WORD + 1)][100]; // # of given length and score
  float          mlg_prob[100];            // probability of given score
  wordmap<char>  word_to_score;
  wordmap<char>  sound_to_score;
  map<parse,pdata>          pos_rules;     // clue parse to possible POS's
  wordmap<pdata>            first_pos, last_pos; // first/last word to POS's
  map<clue_slot,cdata>      exact;         // fill from exact clue
  vector<string>            inexact_words;
  vector<inex>              inexact;       // inexact[# ngrams]
  map<int,float>            exact_probs;   // p(match) given clue match and
                                           // # of appearances
  wordmap<float>            is_abbr;       // probability that it is
  wordset                   multidicts[NUM_MULTI_DICTS];
  vector<vector<string>>    moby;

  map<vstring,vector<vstring>> translations; // information needed to modify
  vector<string>               words;        // a clue and find matches
  vtransition_map              transitions;
  vtransition_map              reversals;

  wordmap<vector<string>>   pronunciations;
  wordmap<vector<string>>   pron_to_words;
  vector<string>            flamingo_words[2];
  vector<float>             flamingo_weights; // both use same array, all 1's
  Flamingo                * flamingo[2];
  mutable map<string,vector<string>> flamingo_cache[2];
  map<string,float>         rebus_words;
  vector<vector<unsigned>>  anagrams[2];
  mutable map<string,vector<string>> pronunciation_cache[2]; // unique or not
  string                    theme_file;
  fitb_db                 * blank_filler;
  trie                      word_trie;

  mutable params param;
  friend int main(int argc, const char **argv);
  friend class puzzle;
  friend class search_node;

public:
  database(const string &flist, bool readit, bool dumpit, bool verbose,
           bool gui);
  ~database();

  void       set_berkeley(const berkeley_solver &bs) {
    berkeley_solver::operator=(bs);
  }
  params   & get_param() const { return param; }
  void       set_param(const string &fname);
  float      detail(unsigned which) const { return get_param().detail(which); }

  float      get_score(const string &w, 
                       const vector<unsigned> &splits = vector<unsigned>()) 
    const;
  unsigned get_mlg_score(const string &w) const { return word_to_score[w]; }
  float word_score(const string &wrd, const pdata &pos, basic_datum &bd) const;
  float score_multiword(unsigned dictnum) const;
  bool  parseable(const string &pat, unsigned dict) const;

  bool  is_simpleword(const string &pat) const;
  bool  is_multiword(const string &pat, bool fitb, unsigned dict) const;
  bool  is_flamingo_word(const string &fill, bool sound) const {
    return binary_search(flamingo_words[sound].begin(),
                         flamingo_words[sound].end(),fill); 
  }
  bool  in_flamingo(const string &fill) const;
  bool  roots_are_word(const string &word, const segmentation &s) const;

  void   clear_flamingo_cache() const {
    flamingo_cache[0].clear(); flamingo_cache[1].clear(); 
  }
  void   clear_pronunciation_cache() const {
    pronunciation_cache[0].clear();
    pronunciation_cache[1].clear();
  }

  vector<string> flamingo_match(const string &fill, bool sound,
                                unsigned to_match, bool check_exists) const;

  wordset suggested(unsigned len, const string &clue) const;
  string special_suggestions(unsigned len, unsigned fitb_limit,
                             const string &clue, set<string> &fills,
                             string &lparen, string &rparen) const;
  void fill_normally(const string &clue, const string &clue1, unsigned len,
                     const vector<unsigned> &splits,
                     const vector<string> &specials,
                     vector<scored_word> &fills, bool do_dict,
                     const set<string> &f, bool clue_abbr, const pdata &pp,
                     string &lparen, string &rparen) const;
  vector<scored_word> fill_rebus(const vector<rebus_answer> &rebus,
                                 const string &clue, const string &clue1,
                                 const skeleton &squares,
                                 vector<scored_word> &fills, bool do_dict,
                                 const pdata &pp) const;
  vector<theme_fill> analyze_theme(const vector<theme_fill> &fills,
                                   const vector<sourced_theme> &themes) const;
  bool in_dictionary(const vector<theme_fill> &fills) const;
  void incorporate_fills(vector<scored_word> &fills,
                         vector<scored_word> &newfills) const;
  void show_specials(const string &clue, const vector<scored_word> &fills,
                     const vector<string> &specials) const;
  pdata suggestions(const skeleton &squares,
                    const vector<unsigned> &splits, const string &clue,
                    const vector<string> &specials, vector<scored_word> &fills,
                    bool maybe_abbr, bool do_dict, unsigned fitb_limit,
                    const vector<rebus_answer> &rebus,
                    const vector<theme_fill> &tsugg) const;
  float evaluate(const string &ans, const string &clue) const;

  bool  includes(const string &fill, bool sound) const;
  string resplit(const string &clue) const;

  map<string,multidata> get_multiwords(const string &pat, bool fitb,
                                       unsigned dict) const;
  vector<theme_fill> get_hints_portmanteau(const string &fill, bool testing,
                                           bool explain) const;
  const map<string,float> &get_rebus_words() const { return rebus_words; }
  vector<rebus_answer> rebus_from_data(map<string,rebus_value> &svalues, 
                                       map<string,rebus_value> &dvalues) const;
  multimap<string,multidata>
  get_multiwords_with_rebus(const string &pat, bool fitb, unsigned dict,
                            const vector<rebus_answer> &rebus) const;

  bool  homonym(const string &fill) const;
  unsigned pun_distance(const word &w, const string &fill) const;
  vector<theme_fill> get_themewords(const string &f) const;
  vector<theme_fill> get_soundwords(word &wd, unsigned &distance) const;
  unsigned is_not_anagram(const string &fill, bool sound) const;
                                // returns hash if true
  bool     is_anagram(const string &fill, bool sound) const;
  vector<theme> find_special_themes(const string &fill, bool try_portmanteau,
                                    bool explain) const;
  bool is_sound_anagram(const string &fill) const;
  set<string> unpronounce(const string &phonemes, unsigned len, bool any = false)
    const;
  vector<string> get_pronunciations(const string &fill, bool unique,
                                    bool call_flite,
                                    const vector<segmentation> &segs =
                                    vector<segmentation>()) const;
  void handle_interactive(settings &sets, vector<string> &files) const;
  vector<string> fitb(unsigned len, const string &clue, unsigned limit) const;

  unsigned find_word(const string &str) const;
  string to_string(unsigned n) const { return words[n]; }
  vector<string>   find_fills(const string &clue, unsigned lim = 2) const;
  vector<string>   find_fills_with_construction(const string &clue, 
                                                unsigned lim = 2) const;

private:
  void solver(string fname, settings &sets) const;
  void process_files(settings &sets, const vector<string> &files) const;
  void autotune(settings &sets, const vector<string> &files) const;

  float get_score(unsigned sz, unsigned sc) const;
  const vector<POS> &pos(const string &wrd) const;

  void dump(const string &file_name, const string &flamingo_name,
            const string &pflamingo_name) const;
  void show_abbrvs() const;

  vector<parse> all_parses(const string &clue, PNoun nouner, string &fword,
                           string &lword) const;
  pdata cpos_prob(const string &c) const;
  void credit_parentheticals(const string &left, const string &right,
                             vector<scored_word> &fills) const;
  void clue_processing(unsigned len, const string &clue, const string &clue1,
                       string &lparen, string &rparen, 
                       vector<scored_word> &fills) const;
  void score_fills(const pdata &pp, vector<scored_word> &fills) const;

  bool  in_portmanteau(const string &fill) const;
  bool  portmanteau(const string &str, bool explain) const {
    return !get_hints_portmanteau(str,true,explain).empty();
  }

  vector<string> flamingo_match_no_plurals(const string &fill, bool sound,
                                           unsigned to_match, bool check) const;
  vector<string> flamingo_choice(const string &fill, unsigned to_match) const;

  void test_theme(const string &fname) const;

  void read(const string &file_name, const string &flamingo_name, 
            const string &pflamingo_name, bool verbose, bool gui);

  void build(const string &mlg_words, const string &filldata,
             const string &pos_file, const string &pos_pos,
             const string &parsings, const string &cluedata,
             const string &rootfile, const string &easyfile,
             const string &mobyfile, const string &dictfile,
             const string &pronfile, const string &spacefile,
             const string &cluefile);

  void build_trie();
  void read_mlg_words(const string &easyfile, const string &words);
  void read_mlg_fills(const string &filldata);
  void build_flamingo_weights();
  void flamingo_build();
  void flamingo_dump(const string &fname, bool sound) const;
  void flamingo_read(const string &fname, bool sound);
  void make_rebus_words();
  void make_anagrams(bool sound);
  void read_pos(const string &parts_of_speech);
  void read_pos_from_pos(const string &pos_from_first);
  void read_pos_from_clue(const string &parsings);
  void read_exact(ifstream &inf);
  void read_inexact_words(ifstream &inf);
  void read_inexact(unsigned idx, ifstream &inf);
  void read_clues(const string &cluedata, const string &rootfile);
  vector<set<string>> read_words_for_multi(const string &easyfile);
  void read_moby(const string &mobyfile);
  string to_string(const vstring &v) const;
  void process_clues(const string &fname);
  void make_transitions(const string &cluefile);
  void read_pronunciations(const string &wordfile, const string &pronfile,
                           const string &spacefile);

  float prob_to_score(float prob) const;
  void insert_berkeley(map<string,basic_datum> &results,
                       const berkeley_answer &answer, bool exacts_only) const;
  map<string,basic_datum> clue_prob(unsigned len, const string &clue,
                                    bool exacts_only, 
                                    const vector<string> &specials) const;

  float pos_prob(const string &w, const pdata &p) const;
  float pos_prob(const pdata &p, const vector<POS> &v) const;
  void  fold_parses(const vector<parse> &parses, pdata &p) const;
  map<string,basic_datum> exacts(unsigned len, const string &clue) const;
  map<string,basic_datum> inexacts(unsigned len, const string &clue) const;
  const cdata &find_inexact(unsigned ni, const vector<int> &s, const bitmap &b)
    const;
  void adjust_inexacts(unsigned ni, const vector<int> &s, const bitmap &b,
                       map<string,basic_datum> &ans, 
                       const string &augmentation = string(),
                       bool at_end = false) const;

  const string &get_root(const string &wrd) const;
  set<string> moby_matches_for_clue(const string &clue, unsigned len) const;
  void insert_from_clue(unsigned len, const vector<unsigned> &splits,
                        const string &clue, bool exacts_only, 
                        const vector<string> &specials,
                        vector<scored_word> &fills,
                        const set<string> &fitb, bool clue_abbr) const;
  void insert_from_clue_rebus(const skeleton &squares,
                              const string &clue, 
                              bool exacts_only, const pdata &pp,
                              vector<scored_word> &newfills,
                              const vector<rebus_answer> &rebus) const;
  void insert_from_dict(unsigned len, const vector<unsigned> &splits,
                        const string &clue, const vector<string> &specials,
                        vector<scored_word> &fills,
                        const set<string> &fitb, bool clue_abbr) const;
  void insert_from_dict_rebus(const skeleton &squares, const pdata &pp,
                              const vector<scored_word> &oldfills,
                              vector<scored_word> &newfills,
                              const vector<rebus_answer> &rebus) const;

  float abbr_prob(const string &str) const;

  void expand(expansion_tree &t, const vstring &src, unsigned pos, unsigned len)
    const;
  void expand(expansion_tree &tree, const vstring &src) const;
  void expand(expansion_tree &tree) const;
  vector<unsigned> find_fills(const vstring &clue, unsigned lim) const;
  vector<unsigned> find_fills_with_construction(const vstring &clue,
                                                unsigned lim) const;

  set<string> fitb(unsigned len, const string &clue, unsigned limit, 
                   bool use_dict) const;
  void fitb_from_dict(unsigned len, const string &clue, int cutoff,
                      set<string> &ans) const;
  set<string> do_lead_follow(unsigned len, const string &clue, bool leader,
                             unsigned limit) const;

  splitting sequence(const string &w, const wordset &words, unsigned num_left,
                     const splitting &prior, split_cache &sc, unsigned idx = 0, 
                     bool one_ok = true) const;

  vector<parse> do_parse(const string &word, const segmentation &segs) const;

  multimap<rebus_value,string> rebus_quality(map<string,rebus_value> &svalues, 
                                             map<string,rebus_value> &dvalues) 
    const;
  vector<rebus_answer> split_rebus(const multimap<rebus_value,string> &scored) 
    const;

  map<string,multiparse> get_multiparses(const string &pat, bool fitb,
                                         unsigned dict, bool nonwords_only)
    const;
  vector<string> multiword_pronunciation(const string &str,
                                         const segmentation &s) const;
  unsigned multiword_score(const string &str, const segmentation &s) const;
  void initialize_fitb(const string &dir);
  vector<string> fitb_from_corpora(unsigned len, const string &orig_clue,
                                   const vector<unsigned> &mask) const;
};

// Compute the fills for a slot in the puzzle, which basically
// consists of a clue and a length, using a lazy data structure that
// allows us to generate successively worse candidate fills with the
// proviso that we never do work until we really need to.  But at the
// root level (no restrictions on the words at all), we generate
// everything up front.  The tricky part is when various letters get
// filled in and we need partial matches quickly.

class filler {
  string            clue;
  vector<string>    exacts;        // exact/changed matches in clue DB
  unsigned          difficulty;
  const database  * dat;
  vector<unsigned>  fills_by_score; // indices into fills_by_word, built lazily
  wordmap<basic_datum> fills_by_word;
  pdata             pos_prob;      // cached
  vector<vector<bitmap>> bitmaps;  // by letter and position
  unsigned          word_id;       // useful for debugging
public:
  filler() { }
  filler(const vector<square *> &squares, const vector<unsigned> &splits,
         const string &c, const database &d, unsigned id, unsigned fitb_limit,
         unsigned diff, bool gui, const vector<rebus_answer> &rebus, 
         const vector<theme_fill> &tsugg, const vector<string> &specials);

  void reset(const params &p);
  const pdata & get_pos_prob() const { return pos_prob; }
  void tell_gui(unsigned original) const;

  // lookup by word.  Putting in a non-const version breaks things!?
  // FIXME
  float  operator[](const string &str) const { return fills_by_word[str].score; }

  const scored_word operator[](unsigned idx) { // lookup by position
    if (idx >= size()) return scored_word("",BIG_CHARGE);
    const string w = nth_best_fill(idx);
    float f = nth_best_score(idx);
    return scored_word(w,f);                          
  }

  size_t length() const { return bitmaps.size(); }
  size_t size()  const { return fills_by_score.size() - 1; } // extra -1 at end

  string nth_best_fill(unsigned n) const {
    return fills_by_word.get_key(length(),fills_by_score[n]);
  }
  const basic_datum &nth_best_value(unsigned n) const {
    return fills_by_word.get_value(length(),fills_by_score[n]);
  }
  basic_datum &nth_best_value(unsigned n) {
    return fills_by_word.get_value(length(),fills_by_score[n]);
  }
  float nth_best_score(unsigned n) const { return nth_best_value(n).score; }

  int position(const string &str) const {
    unsigned k = fills_by_word.get_keys()[str];
    if (k == (unsigned) -1) return -1;
    for (size_t i = 0 ; i < fills_by_score.size() ; ++i)
      if (fills_by_score[i] == k) return i;
    return -1;                  // shouldn't happen
  }

  void show_data(unsigned right_index, const string &clue, const database &d,
                 bool inexact, settings *sets) const;
  void fold_exact();
  void scores_are_computed();
  void rescore(const vector<string> &fills, vector<double> &vals,
               unsigned lo, unsigned hi);

  friend class fill_indexer;
  friend class search_node;
};

// When we gradually restrict based on a pattern, a _branch_ is a
// single restriction.  It consists of a slot being bound, and the
// letter selected.

struct branch {
  unsigned      slot;
  char          letter;
  branch(unsigned s, char l) : slot(s), letter(l) { }
  operator unsigned() const { return NUMCHARS * slot + letter - 'a'; }
  bool operator== (const branch &other) const {
    return slot == other.slot && letter == other.letter; 
  }
};

// Here is the interesting case, where we're also trying to match a
// pattern when we find the fill.  The basic way we do this is to have
// a big tree with the empty pattern at the root and each single
// letter refinement as a child.  We construct the refinements lazily,
// and we don't want to create a refinement if we don't "have to" in
// that we have so many fills that we need to actively split them.  So
// we only split when a "fill indexer" has more than SPLIT entries.
// (We can always match a pattern just via the bitmaps in the fill
// indexer; it just might take a while if there are a lot of matches
// for the current indexer).  The parent_index is the index of the
// first fill for the parent that has *not* been tested to see if it
// matches the desired pattern.

class fill_indexer {
  vector<int>             fills;        // indices into fill of root
  filler                * root;
  fill_indexer          * parent;
  string                  pattern;
  vector<branch>          history;
  vector<fill_indexer *>  kids;         // nullptr where not yet needed
  unsigned                parent_index; // 0 means not started
  vector<bitmap>          bitmaps;      // for computing children

public:
  fill_indexer() { }
  fill_indexer(filler *r) : 
    root(r), parent(nullptr), pattern(r->length(),WILD), 
    kids(r->length() * NUMCHARS,nullptr), parent_index(0), 
    bitmaps(r->length() * NUMCHARS,0)
  { }

  ~fill_indexer() { for (auto k : kids) delete k; }

  void build_from_root();

  const scored_word operator[](unsigned idx);
  const scored_word find_match(const string &pat, unsigned idx);

private:
  unsigned size() const { return fills.size(); }
  bool complete() const { return !fills.empty() && fills.back() < 0; }

  bitmap &bits(const branch &b)           { return bitmaps[b]; }
  bitmap &bits(unsigned idx, char letter) { return bits(branch(idx,letter)); }
  void set_bit(const branch &b, unsigned bit, bool val)
  { bits(b).set_bit(bit,val); }
  void set_bit(unsigned idx, char letter, unsigned bit, bool val) {
    set_bit(branch(idx,letter),bit,val);
  };
  void insert_fill(unsigned root_pos);
  bool extend(const vector<branch> &h);

  fill_indexer *spawn(const branch &b);
};

// Theme identification works by finding a bunch of theme types; each
// type is basically a partial order where bigger is more specific.
// So given a bunch of candidate instances, there is a glb that is
// what the puzzle is apparently "telling" us.

enum theme_type { ADD_STRING, REMOVE_STRING, CHANGE_STRING, ANAGRAM,
                  PORTMANTEAU, PUN, NOTHEME };

// possible themes:
//  ADD string
//  ADD n @ beginning (string is ???)
//  ADD n @ end (string is !!!)
//  REMOVE string
//  REMOVE n @ beginning (string is ???)
//  REMOVE n @ end (string is !!!)
//  CHANGE str1 str2
//  ANAGRAM
//  PORTMANTEAU
// homonyms are treated specially (sound is true but there is "no" theme)

// Does an add/remove refer to an unknown string?

static inline bool iswild(const string &str)
{
  return str[0] == '?' || str[0] == '!';
}

class theme {
  bool          sound;
  theme_type    type;
  string        str1, str2;

public:
  theme() : sound(false), type(NOTHEME) { }
  theme(bool s, theme_type t, const string &s1 = string(), 
        const string &s2 = string()) :
    sound(s), type(t), str1(s1), str2(s2) { }

  theme_type get_type() const { return type; }
  bool no_theme()       const { return !sound && type == NOTHEME; }
  bool is_homonym()     const { return sound && type == NOTHEME; }
  bool is_portmanteau() const { return type == PORTMANTEAU; }
  bool is_anagram()     const { return type == ANAGRAM; }
  bool is_wild_add()    const { return type == ADD_STRING && iswild(str1); }

  bool operator==(const theme &other) const {
    return sound == other.sound && type == other.type && str1 == other.str1 &&
      str2 == other.str2;
  }

  bool   get_sound() const { return sound; }
  bool & get_sound()       { return sound; }
  bool   wild()      const { return iswild(str1); }

  float conforms(const word &w, const string &fill, bool strong,
                 const database &d, bool verbose) const;
  void flamingo(vector<string> &ans, const string &fill, const database &d) 
    const;
  vector<theme_fill> get_hints(const theme_fill &tfill, const database &d) const;

private:
  bool operator<(const theme &other) const {
    if (type != other.type) return type < other.type;
    if (str1 != other.str1) return str1 < other.str1;
    return str2 < other.str2;
  }

  vector<string> get_hints_add(unsigned len, const string &source,
                               const string &fill) const;
  vector<string> get_hints_remove(unsigned len, const string &source) const;
  vector<string> get_hints_change(unsigned len, const string &source) const;
  vector<string> get_hints_anagram(const string &fill, const string &source) 
    const;
  vector<string> get_hints_homonym(unsigned len, const string &psource,
                                   const database &d) const;
  vector<theme_fill> get_hints_portmanteau(const string &fill, const database &d)
    const;
  vector<string> add_string(const string &source, unsigned needed,
                            const vector<string> &ans, unsigned idx = 0) const;

  friend ostream &operator<<(ostream &os, const theme &t);
  friend struct sourced_theme;
  friend class database;

  bool conforms_add(const string &fill, bool strong, const database &d) const;
  bool conforms_remove(const string &fill, bool strong, const database &d) const;
  bool conforms_change(const string &fill, bool strong, const database &d) const;
  bool conforms_homonym(const string &fill, bool strong, const database &d) 
    const;
  bool conforms2(const word &w, string fill, bool strong, const database &d)
    const;
  float bonus(bool strong) const;

  void flamingo_add(vector<string> &ans, const string &fill, const database &d)
    const;
  void flamingo_remove(vector<string> &ans, const string &fill, 
                       const database &d) const;
  void flamingo_change(vector<string> &ans, const string &fill, 
                       const database &d) const;
};

// A sourced_theme is a theme for a whole puzzle, together with the number
// of source words that support it.

struct sourced_theme : theme {
  float sources;

  sourced_theme() : sources(0) { }
  sourced_theme(const theme &t, unsigned n) : theme(t), sources(n) { }

  bool operator<(const sourced_theme &other) const {
    if (sources != other.sources) return sources > other.sources;
    // order reversed so high-support sourced_themes are first in a sort
    return theme::operator<(other);
  }
};

// A possible theme is a vector of vector of theme fills as above.
// (Basically, a theme_fill is a way to convert a phrase into theme
// fill; there is a vector of possibilities for various theme slots).

struct possible_theme : vector<vector<theme_fill>> {
  possible_theme(unsigned n = 0) : vector<vector<theme_fill>>(n) { }
  bool     unthemed(bool sound, const database &d) const;
  unsigned homonym_count(const database &d) const;
  void     analyze(vector<sourced_theme> &ans, const database &d, bool sound,
                   bool explain);
  sourced_theme  lev_analyze(bool sound) const; // whole puzzle
  possible_theme analyze_theme(const vector<sourced_theme> &v,
                               const database &d) const; // find suggested fill
  friend ostream &operator<<(ostream &os, const possible_theme &p);
};

// Simple structure to support a mutex-protected write.

struct protector {
  bool locking;
  protector(bool l) : locking(l) { }
};

extern protector guard, unguard;

POS            pos_from_string(const string &s);
double         get_cpu_time(void);
double         get_time(bool wall, bool reset = false);
FILE          *checked_fopen(const string &str, const string &mode);
void           check_stream(const ifstream &str, const string &src);
string         mlg_tmpfile(const string ext = "", const string dir = "/tmp");
string         mlg_tmpdir();
void           send_to_firefox(const string &str);
string         fill_no_digits(const string &s);
string         fill_digits(const string &s);
string         strip_braces(const string &str);
bool           strip_spaces(string &str);
vector<string> lead_follow(const string &orig, bool &leader);
vector<string> convert_fitb(const string &orig);
string         remove_quotes(const string &str, bool possessives_only = false,
                             bool include_possessives = true);
bool           is_anagram(string s1, string s2);
vector<string> raw_tokenize(const string &str);
vector<string> clue_tokenize(const string &str, bool allow_digits = true);
string         uncontract(const string &str);
vector<string> pos_tokenize(const string &str);
vector<string> stats_tokenize(const string &str);
unsigned       countwords(const string &str);
string         pstring_to_string(const string &str);
string         string_to_pstring(const string &str);
unsigned       changes(const string &s1, const string &s2);
unsigned       levenshtein(const string &s1, const string &s2);
string         pronounce(const string &s);
string         drop_comma(const string &s);
bool           is_lead_follow(const string &clue);
string         clue_for_pos(const string &clue);
vector<string> fitb(unsigned len, const string &clue, unsigned limit,
                    const string &dir);
void           time_and_spin(const string &desc, bool verbose, bool gui);
void           spinner(int i);
void           reproduce_theme(const vector<string> &files);

ostream &operator<<(ostream &os, const Transition t);
ostream &operator<<(ostream &os, const parse &p);
ostream &operator<<(ostream &os, const pdata &p);
ostream &operator<<(ostream &os, const cdata &c);
ostream &operator<<(ostream &os, const clue_slot &c);
ostream &operator<<(ostream &os, const basic_datum &c);
ostream &operator<<(ostream &os, const scored_word &c);
ostream &operator<<(ostream &os, const vector<POS> &v);
ostream &operator<<(ostream &os, const vector<char> &v);
ostream &operator<<(ostream &os, const vector<unsigned char> &v);
ostream &operator<<(ostream &os, const set<string> &s);
ostream &operator<<(ostream &os, const map<int,float> &c);
ostream &operator<<(ostream &os, const map<clue_slot,cdata> &c);
ostream &operator<<(ostream &os, const map<string,float> &c);
ostream &operator<<(ostream &os, const multimap<float,string> &m);
ostream &operator<<(ostream &os, const branch &b);
ostream &operator<<(ostream &os, const params &p);
ostream &operator<<(ostream &os, const splitting &s);
ostream &operator<<(ostream &os, const theme &t);
ostream &operator<<(ostream &os, const sourced_theme &r);
ostream &operator<<(ostream &os, const theme_fill &tf);
ostream &operator<<(ostream &os, const map<string,unsigned> &c);
ostream &operator<<(ostream &os, const square &s);
ostream &operator<<(ostream &os, const square *sq);
ostream &operator<<(ostream &os, const rebus_square &rs);
ostream &operator<<(ostream &os, const rebus_value &v);
ostream &operator<<(ostream &os, const rebus_answer &rb);
ostream &operator<<(ostream &os, const segment &s);
ostream &operator<<(ostream &os, const cgrid &grid);
ostream &operator<<(ostream &os, const boggle_string &s);
ostream &operator<<(ostream &os, const location &l);
ostream &operator<<(ostream &os, const geo_type &l);
ostream &operator<<(ostream &os, const inex_base &i);
ostream &operator<<(ostream &os, const protector &p);

istream &operator>>(istream &is, location &l);
istream &operator>>(istream &is, theme_fill &tf);

// A dispatcher is used to dispatch jobs from a queue to a bunch of
// threads.  It's basically a vector<bool> where this[n] is true if
// task n is complete.  next() gets the next job to do (returning -1
// if there isn't one to avoid possible race conditions), and
// finished() tells you if they're all done.

struct dispatcher : vector<bool> {
  mutable mutex mex;
  unsigned ct;
  dispatcher(unsigned n) : vector<bool>(n,false), ct(0) { }

  unsigned next() {             // next or -1
    mex.lock();
    unsigned n = find(begin(),end(),false) - begin();
    if (n < size()) { (*this)[n] = true; ++ct; }
    mex.unlock();
    return (n == size())? -1 : n;
  }

  unsigned count() const {      // number completed or running
    mex.lock();
    unsigned ans = ct; 
    mex.unlock();
    return ans; 
  }
};

unsigned mismatch(const string &s1, const string &s2);
bool     possible_multi(const string &pat);
bool     shows_abbrv(const string &str);
extern unsigned raw_split[LONG_WORD + 1]; // max # words in a multiword
unsigned max_split(unsigned sz, bool fitb, unsigned dict);

// Return a lower case copy of a string

static inline string tolower(const string &str) 
{
  string ans = str;
  for (auto &ch : ans) ch = tolower(ch); 
  return ans;
}

// Remove the spaces and convert to lower case.  If anything is a
// non-letter, return the empty string to indicate failure.

static inline string remove_spaces(const string &str)
{
  string ans;
  for (auto ch : str) 
    if (isalpha(ch)) ans.push_back(tolower(ch));
    else if (isspace(ch)) continue;
    else return string();
  return ans;
}

// Given a vector of vectors vi, construct a new vector of vectors wi,
// where each wi selects one element of each vi.

template<class T>
static vector<vector<T>> serialize(const vector<vector<T>> &tokens)
{
  vector<vector<T>> ans;
  ans.resize(1);
  for (auto &t : tokens) {
    if (t.empty()) return vector<vector<T>>();
    for (auto &a : ans) a.push_back(t[0]);
    unsigned s = ans.size();
    for (size_t i = 1 ; i < t.size() ; ++i)
      for (size_t j = 0 ; j < s ; ++j) {
        ans.push_back(ans[j]);
        ans.back().back() = t[i];
      }
    if (ans.size() > SERIALIZATION_MAX) {
      ans.clear(); // getting too big!
      break;
    }
  }
  return ans;
}

#endif
