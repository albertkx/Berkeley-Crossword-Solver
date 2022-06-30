#include "score.h"

// All the string compression stuff is here.  First, the ultralong
// machinery.  Primitive converts a substring of length _len_ starting
// at _start_ to a size_t by packing it in 5 bits at a time.

static inline size_t convert_substring(const string &s, unsigned start, 
                                       unsigned len)
{
  size_t ans = 0;
  for (unsigned i = 0 ; i < len ; ++i) {
    ans <<= 5;
    // character should be in range, but if it isn't, weird things will
    // happen.  Enable this to find those.
    // assert(s[start + i] >= 'a' && s[start + i] <= WILD);
    ans |= s[start + i] - 'a';
  }
  return ans;
}

// Ultralong from string.  If > length 12, convert the first 12 chars
// to hi and then the remainder to lo (which will drop them in the
// low-order part of lo, as desired).  If <= length 12, just convert
// the characters and drop them into hi.

ultralong::ultralong(const string &s)
{
  if (s.size() > 12) {
    hi = convert_substring(s,0,12); 
    lo = convert_substring(s,12,s.size() - 12);
  }
  else {
    hi = convert_substring(s,0,s.size());
    lo = 0;
  }
  lo |= (s.size() << 55);
}

// Do the first n chars of two strings of length l (bit-encoded in
// size_t's) match?  Well, if l <= 12, there are 5l bits and we care
// about the high-order 5n.  So we *don't* care about the low order
// 5*(l-n).  If l is more than 12, we just take l = 12.  So the number
// of fields to ignore is min(l,12) - n.  We ignore those fields
// by masking those bits on, and then comparing the results.

static bool match(size_t s1, size_t s2, int len, unsigned n)
{
  static const size_t mask[13] = { 0, 0x1F, 0x3FF, 0x7FFF, 0xFFFFF, 0x1FFFFFF, 
                                   0x3FFFFFFF, 0x7FFFFFFFF, 0xFFFFFFFFFF, 
                                   0x1FFFFFFFFFFF, 0x3FFFFFFFFFFFF,
                                   0x7FFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFF };
  unsigned ignore = min(len,12) - n;
  return (s1 | mask[ignore]) == (s2 | mask[ignore]);
}

// Same but ultralongs.  If matching <= 12 characters, just use the
// above function.  If > 12 characters, we match the hi's and then the
// residue on the lo's.

bool ultralong::match(const ultralong &other, unsigned n) const
{
  if (n <= 12) return ::match(hi,other.hi,length(),n);
  return hi == other.hi && ::match(lo,other.lo,length() - 12,n - 12);
}

// Here is code that recovers a string from an ultralong.  First,
// recover a single character in the obvious way.

static inline char restore_field(size_t s, unsigned bits)
{
  return ((s >> 5 * bits) & 0x1F) + 'a';
}

// Now recover a string of length l from a size_t.  Just be careful to
// restore the early characters from the higher order bits!

static inline string restore_string(size_t s, unsigned len)
{
  string ans(len,WILD);         // need some char to initialize with
  for (size_t i = 0 ; i < len ; ++i) ans[i] = restore_field(s,len - 1 - i);
  return ans;
}

// Turn an ultralong into a string by composing the two parts if
// needed.

ultralong::operator string() const
{
  if (length() <= 12) return ::restore_string(hi,length());
  return ::restore_string(hi,12) + ::restore_string(lo,length() - 12);
}

// Get a single character from an ultralong.  Just use restore_field;
// again, be careful to get the direction right.

char ultralong::get_char(unsigned n) const
{
  if (length() < 12) return restore_field(hi,length() - 1 - n);
  if (n < 12) return restore_field(hi,11 - n);
  return restore_field(lo,length() - 1 - n);
}

// Set a character: first in a size_t, then in an ultralong.
// Ultralong parallels get_char, of course.  For size_t's, just clear
// the bits and then drop c in.

void set_char(size_t &s, unsigned n, unsigned len, size_t c)
{
  s &= ~((size_t) 0x1F << (5 * (len - 1 - n)));
  s |= (c << (5 * (len - 1 - n)));
}

void ultralong::set_char(unsigned n, char c)
{
  if (length() < 12) ::set_char(hi,n,length(),c);
  else if (n < 12) ::set_char(hi,n,12,c);
  else ::set_char(lo,n - 12,length() - 12,c);
}

// Now on to the wordset stuff.  Here we construct a wordset from a
// vector of pointers to strings, already divided by the lengths of
// the strings in question.

void wordset::build(const vector<const string *> words[1 + LONG_WORD])
{
  w1.resize(words[1].size());
  for (size_t i = 0 ; i < w1.size() ; ++i)
    w1[i] = convert_substring(*words[1][i],0,1);
  for (size_t i = 2 ; i <= 3 ; ++i) {
    w2[i - 2].resize(words[i].size());
    for (size_t j = 0 ; j < words[i].size() ; ++j)
      w2[i - 2][j] = convert_substring(*words[i][j],0,i);
  }
  for (size_t i = 4 ; i <= 6 ; ++i) {
    w4[i - 4].resize(words[i].size());
    for (size_t j = 0 ; j < words[i].size() ; ++j)
      w4[i - 4][j] = convert_substring(*words[i][j],0,i);
  }
  for (size_t i = 7 ; i <= 12 ; ++i) {
    w8[i - 7].resize(words[i].size());
    for (size_t j = 0 ; j < words[i].size() ; ++j)
      w8[i - 7][j] = convert_substring(*words[i][j],0,i);
  }
  for (size_t i = 13 ; i <= LONG_WORD ; ++i) {
    w16[i - 13].resize(words[i].size());
    for (size_t j = 0 ; j < words[i].size() ; ++j)
      w16[i - 13][j] = ultralong(*words[i][j]);
  }
}

// Wordset from a set of strings.  Split and use the above function.
// Other constructors are similar.  Note that build expected an array
// of vectors of *pointers*, so we really do have to build them all up
// like this.

wordset::wordset(vector<string> &v)
{
  sort(v.begin(),v.end());
  vector<const string*> words[1 + LONG_WORD];
  for (auto &w : v)
    if (w.size() && w.size() <= LONG_WORD) words[w.size()].push_back(&w);
  build(words);
}

wordset::wordset(const set<string> &s)
{
  vector<const string*> words[1 + LONG_WORD];
  for (auto &w : s)
    if (w.size() && w.size() <= LONG_WORD) words[w.size()].push_back(&w);
  build(words);
}

wordset::wordset(const vector<set<string>> &s)
{
  vector<const string*> words[1 + LONG_WORD];
  for (size_t j = 0 ; j < s.size() ; ++j)
    for (auto &w : s[j]) words[j].push_back(&w);
  build(words);
}

wordset::wordset(const set<string> s[1 + LONG_WORD])
{
  vector<const string*> words[1 + LONG_WORD];
  for (size_t j = 0 ; j <= LONG_WORD ; ++j)
    for (auto &w : s[j]) words[j].push_back(&w);
  build(words);
}

wordset::wordset(const vector<string> s[1 + LONG_WORD])
{
  vector<const string*> words[1 + LONG_WORD];
  for (size_t j = 0 ; j <= LONG_WORD ; ++j)
    for (auto &w : s[j]) words[j].push_back(&w);
  build(words);
}

// There are basically three ways to access a wordset.  They all take
// either string or ultralong arguments.  [] returns the index of the
// entry in the wordset, or -1 (as an unsigned) if it's not there.
// lower_bound returns an index like lower_bound, and upper_bound is
// similar.

// The implementation is always the same: an implementation of the
// underlying primitive as a template, and then a case split for the
// actual wordset implementation.

template<class T>
static inline unsigned adj_idx(const vector<T> &vec, const T &key)
{
  auto lb = lower_bound(vec.begin(),vec.end(),key);
  if (lb == vec.end() || *lb != key) return -1;
  return lb - vec.begin();
}

unsigned wordset::operator[](const ultralong &s) const
{
  switch(s.length()) {
  case 1: return adj_idx(w1,(char) s.gethi());
  case 2: case 3: return adj_idx(w2[s.length() - 2],(short) s.gethi());
  case 4: case 5: case 6:
    return adj_idx(w4[s.length() - 4],(unsigned) s.gethi());
  case 7: case 8: case 9: case 10: case 11: case 12:
    return adj_idx(w8[s.length() - 7],(size_t) s.gethi());
  default: return adj_idx(w16[s.length() - 13],s);
  }
}

template<class T>
static inline unsigned adj_lb(const vector<T> &vec, const T &key)
{
  auto lb = lower_bound(vec.begin(),vec.end(),key);
  return (lb == vec.end())? -1 : (lb - vec.begin());
}

unsigned wordset::lower_bound(const ultralong &s) const
{
  switch(s.length()) {
  case 1: return adj_lb(w1,(char) s.gethi());
  case 2: case 3: return adj_lb(w2[s.length() - 2],(short) s.gethi());
  case 4: case 5: case 6: return adj_lb(w4[s.length() - 4],(unsigned) s.gethi());
  case 7: case 8: case 9: case 10: case 11: case 12:
    return adj_lb(w8[s.length() - 7],(size_t) s.gethi());
  default: return adj_lb(w16[s.length() - 13],s);
  }
}

template<class T>
static inline unsigned adj_ub(const vector<T> &vec, const T &key)
{
  return upper_bound(vec.begin(),vec.end(),key) - vec.begin();
}

unsigned wordset::upper_bound(const ultralong &s) const
{
  switch(s.length()) {
  case 1: return adj_ub(w1,(char) s.gethi());
  case 2: case 3: return adj_ub(w2[s.length() - 2],(short) s.gethi());
  case 4: case 5: case 6: return adj_ub(w4[s.length() - 4],(unsigned) s.gethi());
  case 7: case 8: case 9: case 10: case 11: case 12:
    return adj_ub(w8[s.length() - 7],(size_t) s.gethi());
  default: return adj_ub(w16[s.length() - 13],s);
  }
}

// And a few extra helper functions.  Given a wordset, get the idx
// element of a given length.

ultralong wordset::getultra(unsigned idx, unsigned len) const
{
  switch(len) {
  case 1: return ultralong(1,w1[idx],0);
  case 2: case 3: return ultralong(len,w2[len - 2][idx],0);
  case 4: case 5: case 6: return ultralong(len,w4[len - 4][idx],0);
  case 7: case 8: case 9: case 10: case 11: case 12: return 
      ultralong(len,w8[len - 7][idx],0);
  default: return w16[len - 13][idx];
  }
}

// Number of words of a given length in a wordset

unsigned wordset::size(unsigned len) const
{
  switch(len) {
  case 1: return w1.size();
  case 2: case 3: return w2[len - 2].size();
  case 4: case 5: case 6: return w4[len - 4].size();
  case 7: case 8: case 9: case 10: case 11: case 12: return w8[len - 7].size();
  default: return w16[len - 13].size();
  }
}

size_t wordset::size() const
{
  size_t ans = 0;
  for (unsigned i = 1 ; i <= LONG_WORD ; ++i) ans += size(i);
  return ans;
}
