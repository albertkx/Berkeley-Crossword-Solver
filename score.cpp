#include "xw.h"

// This file contains various primitives used to score words.  For
// dictionary entries, this is done as three pieces: 

// 1. The first part is based only on the word itself.  This is
// intended to be a relative probability that is used to scale the
// answer from the other two parts.

// 2. Next, there is a contribution from a POS analysis.  Once again,
// this is intended to only give a relative contribution; the
// fundamental value comes from the final part.

// 3. This last part is based on the clue and the appearance of
// various words therein.

// Get a word's root.  If not in the database, return the word unchanged.

const string &database::get_root(const string &wd) const
{
  auto r = roots.find(wd);
  return (r != roots.end())? r->second : wd;
}

/* Given a word slot, we know the probability that the fill will be
one of various types -- database entry with score x, etc.  So if the
probability that a word is in the DB with score 36 is p, what is the
probability that it's a *specific* word with score 36?  The answer is
obviously the prior probability divided by the number of words of
score 36.  While the priors appear to be fairly independent of length,
the numbers do depend on length. 

This kind of analysis deals with the ratio of the probability pnew
knowing the score to the probability pold not knowing it.  pold is
just 1/db_tot[size], since each word in the db is equally likely.
pnew is p(word|score) * p(score), or db_prob[s] / db_size[size][s].

Here we compute the (non-relative) probability of a word as fill.
Part of it is easy: Given a clue that has been used before, there is a
known probability that the fill has also been used before.  So we can
compute the fills based on the exact match, and the probabilistic
residue is what's left.

For the residue, there are many disparate sources of information from
many clue fragments, each of which is mapped to a list of words and
appearance counts.  The simplistic view we'll take is that the
probability of a word is the average of all the probabilities
associated with each source of information.  We don't weight the
average because we don't know if a rarely occuring combination should
be give more or less weight!

So let's say that for a given combination there are fills w1...wn with
counts c1...cn.  Say that the total count is c and that there are s words
with single appearances.  Then the probability that the word is something
*else* is about s/c.  So the probability for wi is (ci/c) * (1 - s/c) and
the probability for any other word is s/(c * (#words - n)).  In the code
below, pnon_sing is (1 - s / c) and we scale everything by the tunable
detail(MAXIMUM_MATCH).  We return a mapping from candidate fills into
probabilities.    */

map<string,basic_datum> database::exacts(unsigned len, const string &clue) const
{
  map<string,basic_datum> ans;
  auto i = exact.find(clue_slot(clue,len));
  if (i == exact.end()) return ans;
  const cdata &cd = i->second;
  float pnon_sing = 1 - exact_probs.lower_bound(- cd.ct)->second;
  float base = detail(MAXIMUM_MATCH) * pnon_sing;
  for (auto &j : cd.fills) ans[j.first].clue += j.second * base / cd.ct;
  return ans;
}

/* In some cases, we've got a clue that is the pluralization of
another clue.  [... and others] etc.  This determines if a clue is of
that form, or [... again], etc.  Return the string to add to the
answer to the subclue, and a bool indicating if you add at the
beginning or the end.

Currently four cases.  "and others", "and family" and "Family of" all
add S at the end.  "again" adds RE at the beginning.  */

string is_other_clue(const string &clue, bool &at_end)
{
  if (clue.size() >= 12) {
    at_end = true;
    if (clue.substr(clue.size() - 11) == " and others") return "s";
    if (clue.substr(clue.size() - 11) == " and family") return "s";
    if (clue.substr(0,10) == "Family of ") return "s";
  }
  if (clue.size() >= 7) {
    at_end = false;
    if (clue.substr(clue.size() - 6) == " again") return "re";
  }
  return string();
}

/* For the inexact matches, we will have a bitmap of the words we are
trying to find, and then we'll want to increment to the next legal
bitmap.  This function does that, returning true if there *is* a next
legal bitmap.  There are the following cases:

1. The normal case has b with at least one bit on.  In that case,
clear the first bit and set the bit past the last bit, failing if
these is no such bit.

2. To initialize (because the current bitmap is empty):

2a. If we want more bits than there are, fail
2b. Otherwise, the bitmap constructor does it; succees.  */

bool inexact_bitmap(unsigned lim, unsigned ct, bitmap &b)
{
  if (b.count_bits()) {                                         // 1
    unsigned f = b.first_bit();
    if (f + ct >= lim) return false;
    b.set_bit(f,false);
    b.set_bit(f + ct,true);
  }
  else if (ct > lim) return false;                              // 2a
  else b = bitmap(lim,ct);                                      // 2b
  return true;
}

/* Matches based on partial clues are trickier.  We start with the
fill length and the clue.

1. Tokenize the clue and convert it to the word roots (the parts of
   speech, parsing, etc are handled separately).  Then get the
   associated indices into the inexact words (-1 for a word that is
   missing).

2. Figure out if we can use a "different" clue because this clue ends
   with "in others" or similar.  (is_other_clue above)

3. For each lengh of n-gram (i in the code below), get each appropriate
   piece of the clue; then adjust_inexacts does the work.

4. If there is an augmentation as well, include this at this point also.

5. Compute the total assigned probability and normalize it.  */

map<string,basic_datum> database::inexacts(unsigned len, const string &clue)
  const
{
  vector<string> tokens = clue_tokenize(clue);                          // 1
  vector<int> toks(tokens.size());
  for (unsigned i = 0 ; i < toks.size() ; ++i) {
    string tok = get_root(tokens[i]);
    auto itr = lower_bound(inexact_words.begin(),inexact_words.end(),tok);
    if (itr == inexact_words.end() || *itr != tok) toks[i] = -1;
    else toks[i] = itr - inexact_words.begin();
  }
  map<string,basic_datum> ans;
  bool at_end;
  string augmentation = is_other_clue(clue,at_end);                     // 2
  for (size_t i = 1 ; i <= SEGMENTS ; ++i) {                            // 3
    bitmap b;
    while (inexact_bitmap(tokens.size(),i,b)) {
      adjust_inexacts(i,toks,b,ans);
      if (!augmentation.empty() && len > 3) 
        adjust_inexacts(i,toks,b,ans,augmentation,at_end);              // 4
    }
  }
  float psum = 0;
  for (auto &i : ans) psum += i.second.clue;                            // 5
  if (psum) for (auto &i : ans) i.second *= 1 / psum;
  return ans;
}

// cdata from a subset of an inexact clue (as represented by a
// bitmap).  If -1 is in the segment, you can't do it.  Otherwise,
// look it up.

const cdata &database::find_inexact(unsigned ni,
                                    const vector<int> &s, const bitmap &b) const
{
  static const cdata cd;
  inex_base words;
  unsigned idx = 0;
  for (unsigned i = b.first_bit() ; i != bitmap::npos ; i = b.next_excluding(i))
    if (s[i] < 0) return cd;    // can't find this word in the clue
    else words[idx++] = s[i];
  auto i = inexact[ni].find(words);
  if (i == inexact[ni].end()) return cd;
  return i->second;
}

void database::adjust_inexacts(unsigned ni, const vector<int> &s,
                               const bitmap &b, map<string,basic_datum> &ans,
                               const string &augmentation, bool at_end) const
{
  const cdata &cd = find_inexact(ni,s,b);
  if (cd == cdata()) return;
  float max_match = detail(MAXIMUM_MATCH);
  float min_match = detail(MINIMUM_MATCH);
  float base = max_match + 
    (min_match - max_match) * cd.singletons / (float) cd.ct;
  for (auto &k : cd.fills) {
    const string &str = augmentation.empty()? 
      k.first : (at_end? (k.first + augmentation) : (augmentation + k.first));
    ans[str].clue += base * k.second / float(cd.ct);
  }
}

/* Put it all together to do the clue analysis.  Get the exact and
inexact matches; if we're checking something, print out the special
analysis.

Next, note that the inexact matches are based on word root only, so we
have to check the derivative words to see if they're the right length.
The overall plan is to merge the results from the inexact analysis
into the exact analysis.

For each inexact match, if the root itself is of the correct length,
we add it to _extras_.  We also look at all the derviatives of the
given word; any that is of the right length gets added in as well.  We
compute extra_tot as the total probability assigned to all the
inexact matches.

When we're done, we get exact_tot, which is the total probability
assigned to the exact matches, and divide the probabilites for the
inexact matches among what's left, merging them destructively into the
exact list.  The net result is that we've allocated a total of
probability 1 to all of the words suggested by the clue.  We return
that exact list at the end.  */

void increment(basic_datum &currd, float &currp, const basic_datum &delta)
{
  currd += delta;
  currp += delta.clue;
}

struct inexact_combiner : map<string,basic_datum> {
  unsigned len;
  float total;

  inexact_combiner(unsigned l) : len(l), total(0) { }

  inexact_combiner operator+=(const pair<string,basic_datum> &delta) {
    increment((*this)[delta.first],total,delta.second);
    return *this;
  }
};

// By the time you include all the extra stuff, a score isn't really
// -log(probability) any more.  In fact, it's -log(probability /
// SCALE) and we want to treat the Berkeley numbers the same way.

float database::prob_to_score(float prob) const
{
  return -log(prob / BERKELEY_SCALE);
}

void database::insert_berkeley(map<string,basic_datum> &results,
                               const berkeley_answer &answer, bool exacts_only)
  const
{
  if (!exacts_only || results.find(answer.fill) != results.end())
    results[answer.fill].berkeley = prob_to_score(answer.value);
}
                               
map<string,basic_datum> database::clue_prob(unsigned len, const string &clue,
                                            bool exacts_only, 
                                            const vector<string> &specials) const
{
  if (len < 3) return map<string,basic_datum>(); // nothing to do
  map<string,basic_datum> e = exacts(len,clue);
  map<string,basic_datum> i;
  if (!exacts_only) i = inexacts(len,clue);
  for (auto &j : specials) {
    if (e.find(j) == e.end()) cout << "no exact match for " << j << endl;
    else cout << "exact match for " << j << ": " << e[j].clue << endl;
    string r = get_root(j);
    if (i.find(r) != i.end())
      cout << "inexact match for " << r << ": " << i[r].clue << endl;
    else cout << "no inexact match for " << r << endl;
  }
  inexact_combiner extras(len);
  for (auto &j : i) {
    if (j.first.size() == len) extras += j;
    auto k = constructs.find(j.first);
    if (k == constructs.end()) continue;
    for (auto &l : k->second)
      if (l.size() == len) extras += make_pair(l,j.second);
  }
  float exact_tot = 0;
  for (auto &j : e) exact_tot += j.second.clue;
  for (auto &j : extras) {
    float d = detail(INEXACT);
    j.second *= d * (1 - exact_tot) / extras.total;
    e[j.first] += j.second;
  }

  // Add Berkeley results
  if (results.find(clue) != results.end()) {
    const vector<vector<berkeley_answer>> by_clue = results.at(clue);
    if (by_clue.size() > len)   // are there any of the required length?
      for (const berkeley_answer &a : by_clue[len])
        insert_berkeley(e,a,exacts_only);
  }
  for (auto &j : specials)
    if (e.find(j) == e.end()) cout << "no result for " << j << endl;
    else cout << "result for " << j << ": " << e[j] << endl;
  return e;
}

// Words suggested by the clue, for consideration as subwords in a
// multiword.  Just call clue_prob for all the various sublengths, and
// combine everything into a wordset.

wordset database::suggested(unsigned len, const string &clue) const
{
  static const vector<string> sp;
  vector<string> all[1 + LONG_WORD];
  for (size_t i = 3 ; i < len ; ++i)
    for (auto &p : clue_prob(i,clue,false,sp)) all[i].push_back(p.first);
  return all;
}

// Convert a basic_datum into an actual score using the search
// parameters.  Definition of basic_datum from score.h; the number
// before the comment is the step below where the field is considered.

// struct basic_datum {
//   float         pos;          // 5  contribution from POS
//   float         dict;         // 1  contribution from dictionary
//   float         fabbr;        // 4  contribution from abbrv consideration
//   float         prob;         // 2b contribution from clue
//   unsigned char paren, fitb, abbr, moby; // 6 2a 4 3

// This code does not handle the correct and total values (which will
// have been incorporated into the other values by now); the procedure
// is as follows.

// 1.  If there is a dictionary value, we get an appropriate credit for that.
// 2a. If there is FITB information, we credit that.
// 2b. If not, but it's in the dictionary, credit the clue contribution for that.
// 2c. If it's not in the dictionary, charge for that.
// 3.  Include any considerations from the Moby thesaurus.  So many of the
//     Moby words are stupid that this number might be *more* than the
//     number for a "normal" clue match.
// 4.  Next, include any charge for being or not being an abbreviation.
//     In this part, cabbr true means that the clue indicates that the
//     fill should be an abbreviation.  bd.fabbr is the probability
//     that the fill actually *is* an abbreviation.
// 5.  If there is POS information, fold that in.
// 6.  If there is parenthetical information (e.g. [Squeeze (out)]
//     deal with that.
// Note that any credit for an exact or changed match for the clue is added
// later so that we don't check the word's value a zillion time.

float params::compute_score(const basic_datum &bd) const
{
  float base = 0;
  if (bd.is_dict) base -= detail(FWEIGHT) * log(bd.dict);               // 1

  if (bd.fitb) base -= log(detail(FITB_BONUS));                         // 2a
  else if (bd.is_dict) base -= log(max(bd.clue,detail(CBOUND)));        // 2b
  else base = -log(detail(FBOUND));                                     // 2c

  if (bd.moby) base -= log(detail(MOBY_BONUS));                         // 3

  if (bd.abbr) base -= detail(IS_ABBRV) * log(max(detail(ABBRV_LIMIT),bd.fabbr));
                                                                        // 4
  base -= detail(PWEIGHT) * log(max(bd.pos,detail(PBOUND)));            // 5
  if (bd.paren) base -= detail(PAREN_BONUS);                            // 6

  if (base > bd.berkeley)
    return (1 - detail(BERKELEY)) * base + detail(BERKELEY) * bd.berkeley;
  return base;
}

// Compute a score from a word, POS information, and a score detail
// pos_prob does the convolution and then compute_score handles the
// rest.

float database::word_score(const string &wrd, const pdata &pos, 
                           basic_datum &bd) const
{
  if (pos.ct) bd.pos = pos_prob(wrd,pos);
  return get_param().compute_score(bd);
}

// Penalty if there is no dictionary fill.  It depends on whether or
// not multiwords have also been eliminated; if not, we scale it down
// after NON_DICT_TRANSITION length.  (Evidence from dcheck is that
// there is a linear drop in p(word in dict) after this point.)

float search_node::ndc(const word &w) const
{
  if (w.multi_done(1)) return detail(NON_MULTI_CHARGE); // ignore FITB rule
  float ans = detail(NON_DICT_CHARGE);
  float trans = detail(NON_DICT_TRANSITION);
  if (w.size() > trans) ans -= log(w.size() - trans);
  return ans;
}

// And here is the basic scoring primitive.  If there is no cached
// value for this word, use get_fill to do the work.  Now if there is
// no answer, use the non-dictionary charge.

float search_node::score(const word &wd, const string *f) const
{
  if (!f) f = &wd.fill;
  map<string,float> &scores = wd.computed_scores;
  bool missing = (scores.find(*f) == scores.end());
  float ans = missing? 0 : scores.at(*f);
  if (missing) ans = scores[*f] = get_fill(wd,*f).score;
  if (ans == BIG_CHARGE) ans = ndc(wd);
  if (wd.vertical()) ans *= detail(VERTICAL);
  else ans /= detail(VERTICAL);
  return ans;
}

// Score a collection of words

float search_node::score(const vector<unsigned> &w) const
{
  float ans = 0;
  for (unsigned wrd : w) ans += score((*this)[wrd]);
  return ans;
}

// Score the whole puzzle.  Just add the scores for each word.

float search_node::score() const
{
  float ans = 0;
  for (auto &w : *this) ans += score(w);
  return ans;
}

// Score a word with crossing words.  Just add it up.

float search_node::score_with_crossers(const word &w) const
{
  set<int> words;
  words.insert(w.id);
  for (auto i : w) words.insert(i->word_ids.begin(),i->word_ids.end());
  float ans = 0;
  for (auto i : words) ans += score((*this)[i]);
  return ans;  
}

unsigned puzzle::num_correct() const
{
  unsigned correct = 0;
  for (auto &w : *this) if (w.fill == w.answer) ++correct;
  return correct;
}

// In some cases, we want to adjust the score of the word to include
// the fact that if something is wrong, it may have been "doomed" to
// be wrong in some other way.  We do that by scaling the score of the
// word by the number of active crossing words it has.  (The default
// value of 1 means no scaling.)

// NOTE: In theory, SCALE_BY_CROSSERS here could be different than
// elsewhere.  But that doesn't seem to help.

float search_node::scale(const word &w, float orig) const
{
  unsigned n = w.active_crossers();
  return orig / (1 + detail(SCALE_BY_CROSSERS) * n);
}
