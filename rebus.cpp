#include "xw.h"

// A word has rebus fill if it has a rebus character, which is any
// non-alphabetic non-wild character..

bool skeleton::rebus_fill() const
{
  for (char ch : fill) if (ch >= 'a' + NUMLETS && ch != WILD) return true;
  return false;
}

// Get possible rebus fill by generating all the words that are long
// enough to have a single rebus entry in them as the rebus
// candidates.  suggestions does the work, and we ignore all the
// current rebus entries when we call it because that would just be
// too confusing.  d->suggestions adds in all the rebus possibilities
// into one big vector of scored words v, which we then turn into a
// wordmap and leave in suggestions[word_number] for the rebus test to
// use later.

void search_node::get_rebus_suggestions(word &w)
{
  vector<square *> squares(w.size(),nullptr); // values don't matter
  vector<scored_word> v;
  for (unsigned r = 1 ; r < REBUS_MAX_LENGTH && squares.size() < LONG_WORD ;
       ++r) {
    squares.push_back(nullptr);
    d->suggestions(squares,vector<unsigned>(),
                   w.clue,
                   vector<string>(),v,true,false,
                   detail(FITB_LIMIT),
                   vector<rebus_answer>(),vector<theme_fill>());
                                // puts answer in v
  }
  map<string,float> m;
  for (auto &j : v) m[j.word] = j.score;
  w.suggestions = m;
}

// Here is a nasty little piece of code to see if a given new length
// (used by database::suggestions) might be appropriate, given some
// rebus hints.  _singles_ is the lengths of rebuses that can be used
// once (all decremented by one, so it's a vector of length
// *increments*) and _multiples_ is increments for multi-use rebuses.
// _length_ is the target length increment and _limit_ is the number
// of rebus insertions you're still allowed.

#define REBUS_APPEARANCE_LIMIT 5 // max # rebus words in single fill entry

struct length_query {
  vector<unsigned> singles;
  vector<unsigned> multiples;
  int              length;
  unsigned         limit;

  length_query() : length(0), limit(REBUS_APPEARANCE_LIMIT) { }

  bool operator<(const length_query &other) const {
    if (singles != other.singles) return singles < other.singles;
    if (multiples != other.multiples) return multiples < other.multiples;
    if (length != other.length) return length < other.length;
    return limit < other.limit;
  } 

  operator bool() const;
};

static map<length_query,bool> length_cache;

// Convert a query to a bool, yes or no.  If no length increment is
// needed, yes!  If negative or you're out of words, no.  If you know
// the answer, use that.  Otherwise, try using each single or not, and
// try using each multiple (and keeping it to use again) or not.  Note
// that all the intermediate queries will get cached automatically.

// Note that the cache isn't reset between puzzles, because it's all
// about length deltas and that is the same from puzzle to puzzle.

length_query::operator bool() const
{
  if (length == 0) return true;
  if (length < 0 || limit == 0) return false;
  if (length_cache.find(*this) != length_cache.end())
    return length_cache.at(*this);
  bool ans = false;
  if (!singles.empty()) {
    length_query q = *this;
    q.singles.pop_back();
    if (q) ans = true;
    else {
      q.length -= singles.back();
      --q.limit;
      ans = q;                  // evaluate recursively
    }
  }
  else if (!multiples.empty()) {
    length_query q = *this;       // try last element of multiples; put it back
    q.length -= multiples.back(); // if no good
    --q.limit;
    if (q) ans = true;
    else {
      q.length += multiples.back();
      ++q.limit;
      q.multiples.pop_back();
      ans = q;
    }
  }
  return length_cache[*this] = ans;
}

// Is _len_ a possible length after rebus replacement?  Make a
// length_query to figure it out.

static bool possible_length(const vector<square *> &squares,
                            const vector<rebus_answer> &rebus,
                            unsigned len)
{
  static mutex pl_lock;
  length_query q;
  for (auto &r : rebus)
    if (r.sq.x != (unsigned) -1) q.singles.push_back(r.size() - 1);
    else {
      q.multiples.push_back(r.size() - 1);
      if (r.alternate.size()) q.multiples.push_back(r.alternate.size() - 1);
    }
  sort(q.singles.begin(),q.singles.end());
  sort(q.multiples.begin(),q.multiples.end());
  q.length = len - squares.size();
  unique_lock<mutex> lck(pl_lock);
  return q;
}

// Utility to see if m appears at the given position in s.  (This
// avoids a substr call, which turns out to be very expensive.)

static inline bool string_match(const string &s, unsigned posn, const string &m)
{
  if (posn + m.size() > s.size()) return false;
  for (unsigned i = 0 ; i < m.size() ; ++i) 
    if (s[posn + i] != m[i]) return false;
  return true;
}

// Insert rebus characters into a string.  Return the empty string if
// the result is no good, either shorter than the number of squares or
// longer (unless longer_allowed is true -- which happens if you're
// looking for a geometry theme).  Just bop along, inserting rebus
// characters as you get to them.

static string rebus_string(const string &wrd, const vector<square *> &squares,
                           const vector<rebus_answer> &rebus,
                           bool longer_allowed)
{
  if (rebus.empty()) return wrd;
  string tmp = wrd;
  if (rebus[0].sq.x != (unsigned) -1) {
    for (unsigned i = 0 ; i < tmp.size() && i < squares.size() ; ++i) 
      for (auto &r : rebus) {
        if (r.sq.x == squares[i]->x && r.sq.y == squares[i]->y &&
            string_match(tmp,i,r)) {
          tmp.replace(i,r.size(),1,'a' + NUMLETS);
          break;
        }
      }
  }
  else for (unsigned i = 0 ; i < tmp.size() ; ++i)
         for (unsigned j = 0 ; j < rebus.size() ; ++j)
           if (string_match(tmp,i,rebus[j])) {
             tmp.replace(i,rebus[j].size(),1,'a' + NUMLETS + j);
             break;
           }
           else if (rebus[j].alternate.size() &&
                    string_match(tmp,i,rebus[j].alternate)) {
             tmp.replace(i,rebus[j].alternate.size(),1,'a' + NUMLETS + j);
             break;
           }
  if (longer_allowed? (tmp.size() >= squares.size()) : 
      tmp.size() == squares.size())
    return tmp;
  return string();
}

// Insert rebus characters.  Called "unrebus" because you're replacing
// the rebus string with individual non-letter characters.

string search_node::unrebus(const word &w, const string &str) const
{
  return rebus_string(str,w,rebus,true);
}

// In looking for possible fill with rebus (replacing it with the
// rebus character, of course), we give up on FITB and abbr stuff.  In
// the rebus case, the adjusted length (which we'll use on the lookup
// side) has to be possible given the rebus options.  compare with
// insert_from_clue in fill.cpp

void database::insert_from_clue_rebus(const skeleton &squares,
                                      const string &clue, 
                                      bool exacts_only, const pdata &pp,
                                      vector<scored_word> &fills,
                                      const vector<rebus_answer> &rebus) const
{
  map<string,basic_datum> rfills;
  for (unsigned len = squares.size() + 1 ; len <= LONG_WORD ; ++len)
    if (possible_length(squares,rebus,len)) {
      const map<string,basic_datum> &f1 = clue_prob(len,clue,exacts_only,
                                                    vector<string>());
      rfills.insert(f1.begin(),f1.end());      
    }
  for (auto &i : rfills) {
    string str = rebus_string(i.first,squares,rebus,false);
    if (str.empty()) continue;
    basic_datum bd(get_score(i.first), // non-clue prob
                   false,-1,false,
                    i.second);          // clue-based prob;
    float sc = word_score(i.first,pp,bd);
    fills.push_back(scored_word(str,sc,bd));
  }
}

// Similar but just dictionary-based.

void database::insert_from_dict_rebus(const skeleton &squares,
                                      const pdata &pp,
                                      const vector<scored_word> &oldfills,
                                      vector<scored_word> &fills,
                                      const vector<rebus_answer> &rebus) const
{
  for (unsigned len = squares.size() + 1 ; len <= LONG_WORD ; ++len)
    if (possible_length(squares,rebus,len)) {
      squares.analyzed[len] = true;
      for (size_t i = 0 ; i < word_to_score.size(len) ; ++i) {
        const string &orig = word_to_score.get_key(len,i);
        string str = rebus_string(orig,squares,rebus,false);
        if (str.empty()) continue;
        if (binary_search(oldfills.begin(),oldfills.end(),scored_word(str,0)))
          continue;
        basic_datum bd(get_score(len,word_to_score.get_value(len,i))); // cached
        float sc = word_score(str,pp,bd);
        fills.push_back(scored_word(str,sc,bd));
      }
    }
}

// Score of using the fill _str_ (which presumably includes a rebus
// combo) for word slot w.  If the string appears on the relevant
// suggestions list, it's easy because the string was actually
// suggested by the clue.  Otherwise, we look for the word explicitly
// in the database list of scored words; if so, we compute a
// basic_datum and use that.  If not, we give up.  No multi
// consideration here; it's just too complicated.

float search_node::rebus_score(const word &w, const string &str) const
{
  if (str.size() > LONG_WORD) return detail(NON_MULTI_CHARGE);
  if (w.suggestions.includes(str)) return w.suggestions[str];
  if (!d->includes(str,false)) return detail(NON_MULTI_CHARGE);
  basic_datum bd(d->get_score(str));  // score from DB; fill not an abbr?
  return d->word_score(str,w.wfiller.get_pos_prob(),bd);
}

// Various support functions for the rebus calculation.

// num_doubles looks at the uses and sees how many are duplicated
// (presumably down and across for the same square)

// is_doubles decides whether a particular rebus is used in multiple
// places without being split.  Yes if converting to a set of squares
// makes it at least two smaller (but only one smaller if there are 3
// or fewer uses).

// split_rebus checks to see if this is a split rebus, where you've
// got uses for this rebus and uses for the possible split.  If either
// is not split by itself, then the answer is no.  Otherwise, flip all
// the directions on the other squares and see if you overlap the
// current set by at least two.

size_t rebus_uses::num_doubles() const
{
  rebus_uses tmp = *this;
  for (auto &t : tmp) t.down = false;
  return size() - set<rebus_square>(tmp.begin(),tmp.end()).size();
}

bool rebus_uses::is_doubles() const
{
  rebus_uses tmp = *this;
  for (auto &t : tmp) t.down = false;
  set<rebus_square> s(tmp.begin(),tmp.end());
  return size() >= 1 + (size() > 3) + s.size();
}

bool rebus_uses::split_rebus(rebus_uses other) const
                                // not call by value; other is modified
{
  if (is_doubles() || other.is_doubles()) return false;
  for (auto &i : other) i.down = !i.down;
  set<rebus_square> tmp(begin(),end());
  tmp.insert(other.begin(),other.end());
  return size() + other.size() >= 2 + tmp.size();
}

// Figure out a rebus theme from two maps: possible rebus to value
// considering only one direction (svalues), and a similar one for
// bidirectional (dvalues).

// First, switch it so that we map values (which include the squares
// involved) into strings, and push everything into one such map
// called _scored_.  For the bidirectional entries, we make two copies
// of the squares in question.

// Now figure out how many of these values appear to be "real".  A
// value is real if it's worth at least 10 (REBUS_CUTOFF1) *and* is
// either worth 19 (REBUS_CUTOFF2) or is a priori unlikely, based on a
// 72 word puzzle (72 = REBUS_WORDS).  Unlikely means that the
// probability is less than 0.02 (REBUS_PROBABILITY_CUTOFF).  If there
// are at least 4 (REBUS_REQD) quality words, you're good to go.

static const float    REBUS_CUTOFF1 = 10;
static const float    REBUS_CUTOFF2 = 19;
static const float    REBUS_FINAL_CUTOFF = 1;
static const unsigned REBUS_WORDS   = 72;
static const float    REBUS_PROBABILITY_CUTOFF = 0.02;
static const unsigned REBUS_REQD    = 4;

multimap<rebus_value,string> 
database::rebus_quality(map<string,rebus_value> &svalues, 
                        map<string,rebus_value> &dvalues) const
{
  multimap<rebus_value,string> scored;
  for (auto &s : svalues) scored.insert(make_pair(s.second,s.first));
  for (auto &d : dvalues) {
    size_t s = d.second.squares.size();
    for (size_t j = 0 ; j < s ; ++j)
      d.second.squares.push_back(d.second.squares[j]);
    scored.insert(make_pair(d.second,d.first)); // these appear twice, so they
                                                // are doubled
  }
  unsigned quality = 0;
  for (auto &i : scored) {
    size_t t = i.first.squares.size();
    size_t d = i.first.squares.num_doubles();
    float p = rebus_words.find(i.second)->second;
    float p1 = powf(REBUS_WORDS * p,t) / powf(REBUS_WORDS,d);
    if (i.first.value > REBUS_CUTOFF1 && 
        (i.first.value > REBUS_CUTOFF2 || p1 < REBUS_PROBABILITY_CUTOFF)) 
      quality += t;
  }
  if (quality < REBUS_REQD) scored.clear();
  return scored;
}

// Given a multimap as constructed above, is this a split rebus?  Yes,
// if: (1) it's of size at least two, and (2) the squares so indicate
// as described in split_rebus.  If so, construct the split
// rebus_answer and return it.  Otherwise, return nothing.

vector<rebus_answer> 
database::split_rebus(const multimap<rebus_value,string> &scored) const
{
  auto i = scored.begin();
  auto j = i;
  if (scored.size() > 1 && i->first.squares.split_rebus((++j)->first.squares))
    return vector<rebus_answer>(1,rebus_answer(i->second,j->second));
  return vector<rebus_answer>();
}

// Here is the overall rebus computation.  Build up scored as above,
// and check to see that it's actually a rebus puzzle.  If so, check
// for a split rebus.  If so, you're done.

// If not, rebuild scored from the bidirectional values only.  Now if
// there is only one, or if the best one is used in at least two
// places, it's simple and you're done.  Otherwise, go through and,
// for any square that provides value of at least 1
// (REBUS_FINAL_CUTOFF), record it.  Also record the square where it
// got used.

vector<rebus_answer> database::rebus_from_data(map<string,rebus_value> &svalues, 
                                               map<string,rebus_value> &dvalues)
  const
{
  multimap<rebus_value,string> scored = rebus_quality(svalues,dvalues);
  if (scored.empty()) return vector<rebus_answer>();
  vector<rebus_answer> ans = split_rebus(scored);
  if (!ans.empty()) return ans;
  scored.clear();
  for (auto &d : dvalues) scored.insert(make_pair(d.second,d.first));
  if (!scored.empty() && 
      (scored.size() == 1 || scored.begin()->first.squares.size() > 2))
    ans.push_back(scored.begin()->second);
  else for (auto &i : scored)
         if (i.first.value < REBUS_FINAL_CUTOFF) break;
         else {
           bool found = false;  // only one rebus for any particular square!
           for (auto &a : ans) 
             if (i.first.squares[0] == a.sq) {
               found = true;
               break;
             }
           if (found) continue;
           ans.push_back(i.second);
           ans.back().sq = i.first.squares[0];
         }
  return ans;
}

// Finally, we have to use the above code.  That means making the
// single and double lists needed to get started.

// Assuming we've got enough time to do all this, rdall[string][square *]
// will give the value of using the string bidirectionally in the given
// square, with svalues constructed directly.

// For each rebus word and each word in the puzzle, and then each
// square in the word, get the score as it stands, then put the rebus
// fill in and get the rebus score.  If it's an improvement (lower
// scores are better), add it to the appropriate entry in svalues.
// Whether it's an improvement or not, add it to the appropriate entry
// in rdall.

// Once that's done, we have to convert rdall to dvalues.  That's
// easy.  Just iterate through the strings in rdall, and then the
// squares in the associated submap.  If the value is positive, record
// it in dvalues.

// Once all that's done, rebus_from_data does almost all the work.  If
// we produced nothing, we fail.  Otherwise, tell him, and push the
// answers into the overall rebus.

bool search_node::rebus_check()
{
  if (sets->absolute < get_time()) return false;
  map<string,map<square *,float>> rdall;
  const map<string,float> &rebus_words = d->get_rebus_words();
  map<string,rebus_value> svalues, dvalues;
  for (auto &rw : rebus_words)
    for (auto &w : *this) {
      for (size_t k = 0 ; k < w.size() ; ++k) {
        float orig_score = score(w);
        string reb = w.fill.substr(0,k) + rw.first + w.fill.substr(k + 1);
        float reb_score = rebus_score(w,reb);
        if (orig_score > reb_score) {
          float f = orig_score - reb_score;
          rebus_value &rv = svalues[rw.first];
          rv.value += f;
          rv.squares.push_back(rebus_square(w[k].x,w[k].y,!w.across()));
        }
        rdall[rw.first][&w[k]] += orig_score - reb_score;
      }
    }
  for (auto &rd : rdall)
    for (auto &j : rd.second)
      if (j.second > 0) {
        rebus_value &rv = dvalues[rd.first];
        rv.value += j.second;
        rv.squares.push_back(rebus_square(j.first->x,j.first->y,true));
      }

  vector<rebus_answer> rans = d->rebus_from_data(svalues,dvalues);

  if (rans.empty()) return false;
  if (sets->explain_theme) {
    cout << "rebus calculation:\n";
    for (auto &ra : rans) cout << ' ' << ra << endl;
  }
  if (verb() > SILENT) cout << "rebus result: " << rans << endl;

  rebus.insert(rebus.end(),rans.begin(),rans.end());
  // IF CALLED REPEATEDLY, YOU MIGHT HAVE TO CHECK HERE THAT THERE
  // AREN'T TOO MANY REBUS POSSIBILITIES.
  return true;
}

// Same but also reset both the solver and all the fillers (unless the
// rebuses are for specific squares, all of them need to be recomputed
// in the rebus case).

bool search_node::rebus_check_with_reset()
{
  if (verb() > SILENT) {
    if (rebus.empty()) cout << "rebus check in, no current rebus" << endl;
    else cout << "rebus check in, rebus " << rebus << endl;
  }
  bool ans = rebus_check();
  if (verb() > SILENT) {
    if (rebus.empty()) cout << "rebus check done, still no rebus" << endl;
    else cout << "rebus check done, rebus now " << rebus << endl;
  }
  if (sets->gui) {
    if (rebus.empty()) cout << "**GUI theme no apparent rebus" << endl;
    else {
      vector<rebus_answer> tmp = rebus;
      for (auto &t : tmp) if (t.sq.x == (unsigned) -1) t.sq.x = BIG_SIZE;
      cout << "**GUI rebus " << rebus.size() << ' ' << tmp << endl;
    }
    if (ans) cout << "**GUI render 0" << endl;
  }
  if (!ans) return false;

  reset_fill();
  if (rebus[0].sq.x == (unsigned) -1) reset_fillers(); // multi-use rebuses
  else for (auto &w : *this)                           // find actual words
         for (auto &r : rebus)
           if (w.rebus_candidate(r.sq)) {
             w.to_initialize = true;
             break;
           }

  return true;
}
