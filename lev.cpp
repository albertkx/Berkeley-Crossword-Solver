#include "score.h"
#include <algorithm>

static const unsigned LEV_MAX       = 5; // max # changes to consider
static const unsigned WILD_LIMIT    = 2; // max # of wildcard changes
static const unsigned WILD_FACTOR   = 2; // how much to "downgrade" wildcards
static const unsigned CHANGE_LIMIT  = 4; // max length string to change

struct transformation;
struct modify_str;

// Sources for theme fill.  A list of source words (found by Flamingo,
// presumably) and a bool indicating whether or not the original fill
// was in the dictionary.  (Some theme fills have slipped into the
// dictionary somehow.)

struct sources {
  vector<string> words;
  bool           in_dict;

  sources() : in_dict(false) { }

  // Push a new source.  Update the in_dict flag as well.
  void source(const string &orig, const string &src) {
    words.push_back(src);
    in_dict |= (orig == src);
  }
};

ostream &operator<<(ostream &os, const sources &s)
{
  if (s.in_dict) os << "[D] ";
  return os << s.words;
}

// A replacements is a map from strings to sources.

struct replacements : map<string,sources> {
  set<modify_str> construct_possible_themes() const;
};

// A valuator is basically a data structure containing the data from
// evaluating a single theme possibility on the word data.  Fields are
// as follows:
//  non_dict[n]  number of non-dictionary words that are reduced to 
//               distance n from higher
//  dict[n]      number of dictionary words that are reduced to 
//               distance n from higher
//  wildcard     are there wildcards in the theme?

// The array values should be high; wildcard should be false.  We
// compare two valuators by comparing the sequences, which seems to
// work as well as anything else

struct valuator {
  unsigned non_dict[1 + LEV_MAX], dict[1 + LEV_MAX];
  bool     wildcard;

  valuator() { memset(this,0,sizeof(valuator)); }
  valuator(const string &from, const string &to, unsigned base, bool dct);
  valuator(const modify_str &mod, const string &from,
           const string &to, unsigned base, bool dct, unsigned idx = 0);

  bool operator<(const valuator &other) const {
    unsigned factor = 1, other_factor = 1;
    if (wildcard && !other.wildcard) other_factor = WILD_FACTOR; // only this
    else if (wildcard != other.wildcard) factor = WILD_FACTOR;   // only other
    for (size_t i = 0 ; i <= LEV_MAX ; ++i) {
      if (non_dict[i] * factor != other.non_dict[i] * other_factor)
        return non_dict[i] * factor < other.non_dict[i] * other_factor;
      if (dict[i] * factor != other.dict[i] * other_factor)
        return dict[i] * factor < other.dict[i] * other_factor;
    }
    return wildcard > other.wildcard;
  }

  // take the better
  valuator &operator*=(const valuator &other) {
    if (*this < other) *this = other;
    return *this;
  }
  valuator operator*(const valuator &other) const {
    valuator ans = *this;
    return ans *= other;
  }

  // add values, presumably from multiple words.  Note that this function
  // DOES NOT set the wildcard field.  You have to do that separately.
  valuator &operator+=(const valuator &other) {
    for (unsigned i = 0 ; i <= LEV_MAX ; ++i) {
      non_dict[i] += other.non_dict[i];
      dict[i]     += other.dict[i];
    }
    return *this;
  }
  valuator operator+(const valuator &other) const {
    valuator ans = *this;
    return ans += other;
  }

  // total number of words improved
  unsigned sum() const {
    unsigned ans = 0;
    for (unsigned i = 0 ; i <= LEV_MAX ; ++i) ans += non_dict[i] + dict[i];
    return ans;
  }
  bool viable() const;
  float num_sources() const;
};

ostream &operator<<(ostream &os, const valuator &v)
{
  for (unsigned i = 0 ; i <= LEV_MAX ; ++i)
    os << v.non_dict[i] << ':' << v.dict[i] << ' ';
  os << "= " << v.sum();
  if (v.wildcard) os << " (*)";
  return os;
}

// Given an evaluation, is a theme worth considering?  (This is where
// the "crossword knowledge" comes in.)  We say yes if:
//  1. At least one non-dictionary entry must be improved by the theme.
//  2. At least two entries must be reduced to <= 1 error.
//  3. *Either* something must be "solved" by the theme or there must
//     be at least three appearances of the theme.  (Note that "solved by
//     the theme" means a non-dictionary entry.)
//  4. If wildcard, *both* something must be solved by the theme and there
//     must be at least three things brought down to one error or less.
//  5. The theme must fix one entry (maybe dictionary, maybe not).
//  6. The theme must bring at least one nondictionary entry down to <= 1
//     error.
//  7. If the theme doesn't solve anything, it must help at least two
//     non-dictionary entries.

bool valuator::viable() const
{
  static unsigned zeroes[1 + LEV_MAX] = { };
  if (!memcmp(zeroes,non_dict,sizeof(zeroes))) return false;              // 1
  if (non_dict[0] + non_dict[1] + dict[0] + dict[1] < 2) return false;    // 2
  if (non_dict[0] == 0 && sum() <= 2) return false;                       // 3
  if (wildcard && (non_dict[0] == 0 || non_dict[0] + non_dict[1] < 3)) 
    return false;                                                         // 4
  if (non_dict[0]) return true;                          // 5-7 (non-dict case)
  if (dict[0] == 0) return false;                                         // 5
  if (non_dict[1] == 0) return false;                  // 6 (non_dict[0]=0 above)
  unsigned non_dict_sum = 0;
  for (unsigned i = 0 ; i <= LEV_MAX ; ++i) non_dict_sum += non_dict[i];
  if (non_dict_sum < 2) return false;                                     // 7
  return true;
}

// As we do the Levenshtein analysis, there are four possible
// modifications we consider.  These are adding a character, removing
// a character, changing a character (these three are standard), and
// moving a character (which is not standard).

enum modification { ADD, REMOVE, CHANGE, NOTHING };

// Single Lev step.  Type of modification, and ch1 is character added,
// removed or changed (to ch2).

struct modify_char 
{
  modification   mod;
  char           ch1, ch2;      // ch2 relevant for CHANGE only

  modify_char(modification st, char c1, char c2 = 0) : 
    mod(st), ch1(c1), ch2(c2) { }
  modify_char(const string &str);

  bool operator==(const modify_char &other) const {
    if (mod != other.mod) return false;
    switch (mod) {
    case ADD: case REMOVE: return ch1 == other.ch1;
    case CHANGE: return ch1 == other.ch1 && ch2 == other.ch2;
    case NOTHING: return true;
    }
  }

  bool operator!=(const modify_char &other) const { return !operator==(other); }

  bool operator<(const modify_char &other) const {
    if (mod != other.mod) return mod < other.mod;
    switch (mod) {
    case ADD: case REMOVE: return ch1 < other.ch1;
    case CHANGE:
      return (ch1 != other.ch1)? (ch1 < other.ch1) : (ch2 < other.ch2);
    case NOTHING: return false;
    }
  }
};

// Construct from string representation.  Possible values are +h (add
// char h), -h (remove char h), x/y (change x to y) or . (nothing).

modify_char::modify_char(const string &str)
{
  switch (str[0]) {
  case '+': mod = ADD; ch1 = str[1];    break;
  case '-': mod = REMOVE; ch1 = str[1]; break;
  case '.': mod = NOTHING;              break;
  default: mod = CHANGE;
    ch1 = str[0];
    ch2 = str[2];
  }
}

// Output inverts the above.

ostream &operator<<(ostream &os, const modify_char &m)
{
  switch (m.mod) {
  case ADD:     os << '+' << m.ch1;          break;
  case REMOVE:  os << '-' << m.ch1;          break;
  case CHANGE:  os << m.ch1 << '/' << m.ch2; break;
  case NOTHING: os << '.';                   break;
  }
  return os;
}

// A transformation is a vector of character modifications

struct transformation : vector<modify_char> {
  bool operator<(const transformation &other) const {
    if (size() != other.size()) return size() < other.size();
    for (unsigned i = 0 ; i < size() ; ++i)
      if ((*this)[i] != other[i]) return (*this)[i] < other[i];
    return false;
  }

  modification mod(unsigned i) const { return (*this)[i].mod; }
  char         ch1(unsigned i) const { return (*this)[i].ch1; }
  char         ch2(unsigned i) const { return (*this)[i].ch2; }

  unsigned     length() const {
    return count_if(begin(),end(),
                    [](const modify_char &m) { return m.mod != NOTHING; });
  }

  void aggregate(vector<modify_str> &possibles) const;
};

// Levenshtein disantace calculation, but return the actual paths
// instead of just the length of the shortest one.  Algorithm is
// basically unchanged from the usual dynamic programming; d[i][j] is
// the length of the path from the intial subsequence of the source of
// length i to the initial subsequence of the target of length j and
// dd[i][j] is the collection of paths of that length.  If dist is
// non-NULL, you just want the distance only (no paths), so a lot of
// the computation can be skipped.

set<transformation> lev(const string &source, const string &target, 
                        unsigned *dist = nullptr)
{
  vector<vector<vector<transformation>>> dd;
  if (!dist) dd.resize(source.size() + 1);
  vector<vector<unsigned>> d(source.size() + 1);

  for (unsigned i = 0 ; i < source.size() + 1 ; ++i) {
    if (!dist) dd[i] = vector<vector<transformation>>(target.size() + 1);
    d[i] = vector<unsigned>(target.size() + 1);
  }
  for (unsigned i = 0 ; i <= source.size() ; ++i) {
    // remove all the characters
    if (!dist) {
      transformation s;
      for (unsigned j = 0 ; j < i ; ++j) 
        s.push_back(modify_char(REMOVE,source[j]));
      dd[i][0].push_back(s);
    }
    d[i][0] = i;
  }
  for (unsigned i = 1 ; i <= target.size() ; ++i) {
    // add all the characters
    if (!dist) {
      transformation s;
      for (unsigned j = 0 ; j < i ; ++j) s.push_back(modify_char(ADD,target[j]));
      dd[0][i].push_back(s);
    }
    d[0][i] = i;
  }
  unsigned len_add, len_rem, len_chn;
  for (unsigned i = 1 ; i <= source.size() ; ++i)
    for (unsigned j = 1 ; j <= target.size() ; ++j) {
      len_add = d[i][j - 1] + 1;  // will need to add target[j]
      len_rem = d[i - 1][j] + 1;  // need to remove source[i]
      len_chn = d[i - 1][j - 1] + (source[i - 1] != target[j - 1]);
      d[i][j] = min(min(len_add,len_rem),len_chn);
      if (dist || d[i][j] > LEV_MAX) continue;
      if (len_add == d[i][j]) {
        for (unsigned k = 0 ; k < dd[i][j - 1].size() ; ++k) {
          dd[i][j].push_back(dd[i][j - 1][k]);
          dd[i][j].back().push_back(modify_char(ADD,target[j - 1]));
        }
      }
      if (len_rem == d[i][j])
        for (unsigned k = 0 ; k < dd[i - 1][j].size() ; ++k) {
          dd[i][j].push_back(dd[i - 1][j][k]);
          dd[i][j].back().push_back(modify_char(REMOVE,source[i - 1]));
        }
      if (len_chn == d[i][j])
        for (unsigned k = 0 ; k < dd[i - 1][j - 1].size() ; ++k) {
          dd[i][j].push_back(dd[i - 1][j - 1][k]);
          modification ta = (source[i - 1] == target[j - 1])? NOTHING : CHANGE;
          dd[i][j].back().push_back(modify_char(ta,source[i - 1],
                                                target[j - 1]));
        }
    }
  if (dist) {
    *dist = d[source.size()][target.size()];
    return set<transformation>();
  }
  const vector<transformation> &v = dd[source.size()][target.size()];
  return set<transformation>(v.begin(),v.end());
}

// Distance function that just counts number of changed characters.
// If the strings are of different lengths, treat them as all
// different.

unsigned changes(const string &s1, const string &s2)
{
  if (s1.size() != s2.size()) return max(s1.size(),s2.size());
  unsigned ans = 0;
  for (size_t i = 0 ; i < s1.size() ; ++i) if (s1[i] != s2[i]) ++ans;
  return ans;
}

// Levenshtein distance that doesn't need the change list.  Just call
// the above function, but tell it not to do the hard work.

unsigned levenshtein(const string &source, const string &target)
{
  if (source == target) return 0;
  unsigned ans;
  lev(source,target,&ans);
  return ans;
}

// A *string* modification is an actual theme candidate, but it's
// basically just the same as a char modification that manipulates
// strings instead of individual characters.

struct modify_str {
  modification   mod;
  string         str1, str2;

  modify_str(modification m, const string &s1, const string &s2 = string()) :
    mod(m), str1(s1), str2(s2) { }
  modify_str(const modify_char &ch) :
    mod(ch.mod), str1(1,ch.ch1), str2(1,ch.mod == CHANGE? ch.ch2 : ch.ch1) { }
  modify_str(const string &str);
  modify_str(const set<modify_str> &possibles, const replacements &rep,
             valuator &best_val, bool sound);

  bool operator==(const modify_str &other) const {
    if (mod != other.mod) return false;
    switch (mod) {
    case ADD: case REMOVE: return str1 == other.str1;
    case CHANGE: return str1 == other.str1 && str2 == other.str2;
    case NOTHING: return true;
    }
  }

  bool operator!=(const modify_str &other) const { return !operator==(other); }
  bool operator<(const modify_str &other) const {
    if (mod != other.mod) return mod < other.mod;
    if (str1 != other.str1) return str1 < other.str1;
    return str2 < other.str2;
  }

  // # of characters "changed"
  unsigned length() const {
    return (mod == CHANGE)? levenshtein(str1,str2) : str1.size();
  }

  bool  match(const string &src, unsigned idx) const;
  bool  iswild() const { return ::iswild(str1); }
  theme convert_to_theme() const;

  bool can_wildcard() const { return mod < CHANGE && str1.size() <= WILD_LIMIT; }
  valuator evaluate(const string &from, const string &to, bool dict) const;
  valuator evaluate(const replacements &sugg) const;
};

// Representation like +hoe -?? foo/bar

modify_str::modify_str(const string &str)
{
  if (str.size() == 0) return;
  switch (str[0]) {
  case '.': break;
  case '+': case '-': {
    if (str[0] == '+') mod = ADD;
    else if (str[0] == '-') mod = REMOVE;
    str1 = str.substr(1);
    break;
  }
  default: {
    size_t slash = str.find('/');
    mod = CHANGE;
    str1 = str.substr(0,slash);
    str2 = str.substr(1 + slash);
    break;
  }
  }
}

ostream &operator<<(ostream &os, const modify_str &m)
{
  switch (m.mod) {
  case ADD:     os << '+' << m.str1;           break;
  case REMOVE:  os << '-' << m.str1;           break;
  case CHANGE:  os << m.str1 << '/' << m.str2; break;
  case NOTHING: os << '.';                     break;
  }
  return os;
}

// Does a modify_str have a match at the given index of a source
// string?  Handle the wildcard cases, or just see if the string
// matches.

bool modify_str::match(const string &src, unsigned idx) const
{
  switch (str1[0]) {
  case '?': return idx == 0;
  case '!': return idx + str1.size() == src.size();
  default: return src.substr(idx,str1.size()) == str1;
  }
}

// Collect all the possible themes from a transformation.  Basically
// collect every subsequence of the adjustment that's viable as a
// theme, aggregating into _possibles_.  For the wildcards, use ?  for
// a change at the beginning of a word and ! for a change at the end
// of a word.

// This is tricky because (for example) something that looks lie +a . w/r
// might really be ow/aor.  So we have to aggregate carefully.  As we
// do this, we look at ranges [i,j) in the transformation itself and
// try to collapse them to a single modify_str.  We fix i and then
// consider j's from i+1 to the end of the string, building up the
// modify_str as we go.  But we only consider that modify_str as "real"
// if the final (j'th) modify_char is not NOTHING.

// So for a non-nothing i, the first possibility is a modify_str that
// is just this modify_char in isolation; we call this poss and add it
// to the vector possible modify_str's.  That's the easy case.  Now as
// we go along, there are the following cases, based on the type of j:

// 1. If j is NOTHING, we basically want to push a "do nothing" onto
// the end of poss.  Note that ch1 will be the unchanged character.
// And now it depends on the type of poss:
// 1a. if poss is CHANGE, we add the char to both str1 and str2.
// 1b. if poss is ADD, it can become a CHANGE: +xxx with an unchanged
//     char c becomes CHANGE c to xxxc.
// 1c. if poss is REMOVE, it becomes a change from xxxc to c.
// Note that in all of these cases, we push the char onto str1 at the
// beginning and then sort it out.

// 2. If j is of the same type as poss, it's easy: we just push the
// new char onto poss.  If j is a change, we push the result char onto
// str2 as well.

// 3. If neither is a CHANGE, we're done, since they aren't equal and
// cannot be combined.  So note that the remaining cases have either j
// or poss as a CHANGE but not both (because they aren't equal).

// 4a. If j is ADD, we add the new character to the result of the
// change, which is in str2.
// 4b. If j is REMOVE, we add the new character to the result prior to
// the change, which is in str1.

// In the remaining cases, j is a CHANGE x/y, so poss is ADD or
// REMOVE.  Either way we change poss to CHANGE and then if the poss
// string is zzz, the new poss is x/zzzy if poss was an ADD and zzzx/y
// if poss was a REMOVE.  We put zzz in the right place below, and
// then append x and y.

// Finally, we have to decide if we want to write out the new poss,
// which we do unless it's an overlength CHANGE.

void transformation::aggregate(vector<modify_str> &possibles) const
{
  if (length() > LEV_MAX) return;
  for (size_t i = 0 ; i < size() ; ++i) {
    if (mod(i) == NOTHING) continue;
    modify_str poss((*this)[i]);
    possibles.push_back(poss);
    for (size_t j = 1 + i ; j < size() ; ++j) {
      if (mod(j) == NOTHING) {                                          // 1
        poss.str1.push_back(ch1(j));
        switch (poss.mod) {
        case NOTHING: assert(false); // can't get here
        case CHANGE: poss.str2.push_back(ch1(j));        break;         // 1a
        case ADD:                                                       // 1b
          poss.mod = CHANGE;
          poss.str2 = poss.str1;
          poss.str1 = string(1,ch1(j));                  break;
        case REMOVE:                                                    // 1c
          poss.mod = CHANGE;
          poss.str2 = string(1,ch1(j));                  break;
        }
        continue;
      }
      else if (mod(j) == poss.mod) {                                    // 2
        poss.str1.push_back(ch1(j));
        if (poss.mod == CHANGE) poss.str2.push_back(ch2(j));
      }
      else if (mod(j) != CHANGE && poss.mod != CHANGE)                  // 3
        break;
      // next two cases have poss.mod == CHANGE
      else if (mod(j) == ADD) poss.str2.push_back(ch1(j));              // 4a
      else if (mod(j) == REMOVE) poss.str1.push_back(ch1(j));           // 4b
      else {                                                            // 5
        if (poss.mod == ADD) { poss.str2 = poss.str1; poss.str1.clear(); }
        poss.mod = CHANGE;
        poss.str1.push_back(ch1(j));
        poss.str2.push_back(ch2(j));
      }
      if (poss.mod == CHANGE && poss.str1.size() > CHANGE_LIMIT) break;
      possibles.push_back(poss);
    }
  }
  unsigned lim;
  if (mod(0) < CHANGE) {
    for (lim = 1 ; lim <= WILD_LIMIT ; ++lim) if (mod(lim) != mod(0)) break;
    for (unsigned j = 0 ; j < lim ; ++j)
      possibles.push_back(modify_str(mod(0),string(1 + j,'?')));
  }    
  if (back().mod < CHANGE) {
    for (lim = 1 ; lim <= WILD_LIMIT ; ++lim)
      if (mod(size() - lim - 1) != back().mod) break;
    for (unsigned j = 0 ; j < lim ; ++j)
      possibles.push_back(modify_str(back().mod,string(1 + j,'?')));
  }
}

// Get the possible themes mapping the sources to fills.  For each
// one, and each source for that fill, call lev to get the possible
// paths in modification space.  Then aggregate all the vectors of
// modify_chars into the set of possibilities.

set<modify_str> replacements::construct_possible_themes() const
{
  set<modify_str> ans;
  for (auto &f : *this) {
    vector<modify_str> possibles;
    for (const string &str : f.second.words) 
      for (auto &k : lev(f.first,str)) k.aggregate(possibles);
    ans.insert(possibles.begin(),possibles.end());
  }
  return ans;
}

// Compute the value of a candidate theme.  It's built up from two
// parts.

// First, we have a valuator based on a from string (the source,
// basically) and the target word (the result of operating on the fill
// with the theme).  Just use levenshtein to figure out how close you
// wound up; if it's an improvement, record it.

valuator::valuator(const string &from, const string &to, unsigned base, bool dct)
{
  *this = valuator();
  unsigned l = levenshtein(from,to);
  if (l < base && l <= LEV_MAX) ++(dct? dict : non_dict)[l];
}

// The second part is a loop because non-wildcard themes can
// potentially be applied multiple times.  The args here are:

// mod          modification
// from         source string
// to           target string
// base         lev distance from *original* source to target
// dct          is original source in dictionary? (needed to construct valuator)
// mod_idx      as we iterate, which mod in mods are we considering?
// string_idx   as we iterate, where in the string are we considering?

// If we're out of mods (the base case), then the above constructor
// can handle it.

// The remaining code is the actual work.  Note that we handle an ADD
// by treating it as a REMOVE from the target string.  So the string
// being adjusted is modify_me in the code that follows.

// Now for each place that there might be a match, if there *is* a
// match, note that you found one and build up the new from and to
// strings.  Then if you can repeat this modifier, start from the
// current location or from right after the post-change string if the
// mod is CHANGE.  Then recur.

valuator::valuator(const modify_str &mod, const string &from, const string &to,
                   unsigned base, bool dct, unsigned idx)
{
  *this = valuator(from,to,base,dct);
  unsigned sz = mod.str1.size();
  const string &modify_me = (mod.mod == ADD? to : from);
  for (unsigned i = idx ; i + sz <= modify_me.size() ; ++i) {
    if (mod.match(modify_me,i)) {
      string adjusted = modify_me.substr(0,i);
      if (mod.mod == CHANGE) adjusted += mod.str2;
      adjusted += modify_me.substr(i + sz);
      const string &newf = (mod.mod == ADD)? from : adjusted;
      const string &newt = (mod.mod == ADD)? adjusted : to;
      unsigned new_idx = i;
      if (mod.mod == CHANGE) new_idx += mod.str2.size();
      if (mod.iswild()) *this += valuator(newf,newt,base,dct);
      else {
        unsigned new_idx = i;
        if (mod.mod == CHANGE) new_idx += mod.str2.size();
        *this += valuator(mod,newf,newt,base,dct,new_idx);
      }
    }
  }
  wildcard = mod.iswild();
}

// General evaluation just calls the above.

valuator modify_str::evaluate(const string &from, const string &to, bool dict)
  const
{
  return valuator(*this,from,to,levenshtein(from,to),dict);
}

// Evaluation from a set of replacements takes the best valuator for
// each individual replacement and adds them all.  (Note that the
// theme is fixed here; it's *this.)

valuator modify_str::evaluate(const replacements &rep) const
{
  valuator ans;
  for (auto &p : rep) {
    valuator v;
    for (auto &s : p.second.words) v *= evaluate(p.first,s,p.second.in_dict);
    ans += v;
  }
  ans.wildcard = iswild();
  return ans;
}

// Find the best theme, and store the value in best_val.  Just walk
// through the possible modify_str's, checking each one.

modify_str::modify_str(const set<modify_str> &possibles,
                       const replacements &rep, valuator &best_val, bool sound)
{
  best_val = valuator();
  for (const modify_str &i : possibles) {
    if (!sound && i.str1.back() == 's' && i.mod < CHANGE) continue;
    // add/remove an s, not sound, just too likely to be garbage
    valuator v = i.evaluate(rep);
    if (v.viable() && best_val < v) {
      best_val = v;
      *this = i;
    }
  }
}

// Convert an adjustment to a theme.  _sound_ is set to false and
// corrected elsewhere if needed.  Also, the lev stuff converts fill
// to Flamingo match, and the theme stuff should go the other way.

theme modify_str::convert_to_theme() const
{
  switch (mod) {
  case ADD:    return theme(false,REMOVE_STRING,str1);
  case REMOVE: return theme(false,ADD_STRING,str1);
  case CHANGE: return theme(false,CHANGE_STRING,str2,str1);
  case NOTHING: assert(false);  // shouldn't happen!
  }
}

// We take the view that the number of sources of a theme scales as
// the "near misses" by the number of changes remaining after the
// theme is used.  A dictionary entry counts as an extra error.

float valuator::num_sources() const
{
  float ans = 0;
  for (unsigned i = 0 ; i <= LEV_MAX ; ++i) 
    ans += non_dict[i] / float(1 + i) + dict[i] / float(2 + i);
  return ans;
}

// Here is the interface to the theme mechanism used by the solver.  A
// possible_theme is a vector<vector<theme_fill>>, where a theme_fill
// has the "track" from original word through to theme-based fill.  We
// convert that to a replacements to use the above code.  Then
// construct_possible_themes generates possibilities, and we get the
// best one from those.  Then conversion to a sourced them is easy.

sourced_theme possible_theme::lev_analyze(bool sound) const
{
  replacements fills;           // map from poriginal to psource
  for (const vector<theme_fill> &t : *this)
    for (const theme_fill &tf : t)
      fills[tf.poriginal].source(tf.poriginal,tf.psource);
  set<modify_str> possibles = fills.construct_possible_themes();
  valuator best_val;
  modify_str best(possibles,fills,best_val,sound);
  if (!best_val.viable()) return sourced_theme();
  return sourced_theme(best.convert_to_theme(),best_val.num_sources());
}
