#include "xw.h"

// Maximum number of words allowed in a multiword, as function of
// string length.

unsigned raw_split[(LONG_WORD + 1)] = {
  0, 0, 0, 0, 0, 0,             // 0 - 5 chars don't split
  2, 2, 2, 2,                   // 6 - 9 chars split in two
  3, 3, 3, 3,                   // 10-13
  4, 4, 4, 4,                   // 14-17
  5, 5, 5, 5,                   // 18-21
  6, 6 };                       // 22-23

// Number of words allowed is just what's in raw_split, unless this is
// dictionary 0 and it's a FITB clue.

unsigned max_split(unsigned sz, bool fitb, unsigned dict)
{
  unsigned ans = raw_split[sz];
  if (fitb && !dict && ans < 2) ans = 2;
  return ans;  
}

// useful in debugging sometimes

string broken_string(const string &w, unsigned idx)
{
  return w.substr(0,idx) + "!" + w.substr(idx);
}

// Score a string that turns out to be a multiword using the ith
// dictionary.  It's WORD_CHARGE, plus DICT_CHARGE if not the 0th
// dictionary.

float database::score_multiword(unsigned dictnum) const
{
  float ans = detail(WORD_CHARGE);
  if (dictnum) ans += detail(DICT_CHARGE);
  return ans;
}

// Given that a multiword isn't in the database, can we replace a
// component with a root to get it there?  Build up the list of words,
// see which have roots, and then work through each subset.

bool database::roots_are_word(const string &word, const segmentation &s) const
{
  vector<string> words(s.size());
  vector<unsigned> has_root;
  vector<string> rts(s.size());
  unsigned idx = 0;
  for (size_t i = 0 ; i < s.size() ; ++i) {
    words[i] = word.substr(idx,s[i]);
    idx += s[i];
    auto itr = roots.find(words[i]);
    if (itr != roots.end()) {
      has_root.push_back(i);
      rts[i] = itr->second;
    }
  }
  for (int i = 1 ; i < 1 << has_root.size() ; ++i) {
    string candidate;
    vector<bool> use_root(words.size(),false);
    for (size_t j = 0 ; j < has_root.size() ; ++j)
      if (i & (1 << j)) use_root[has_root[j]] = true;
    for (size_t j = 0 ; j < words.size() ; ++j)
      candidate += use_root[j]? rts[j] : words[j];
    if (includes(candidate,false)) return true;
  }
  return false;
}

// That was the easy part: dealing with known multiwords that are
// fixed strings.  Now the hard part, which is to take a pattern and
// generate all instances that are word sequences, along with a POS
// analysis for each such phrase.  This isn't hard in theory, but it
// is a true bottleneck for the code, so we have to make it as fast as
// possible.

// First, we'll generally have a pdata for the clue, and we'll have
// the pdata for a multiword string that extends the given pattern.
// The probability that the string is a good fill is obtained by
// convolving these two probabilities.

float pdata::convolve(const pdata &other) const
{
  float ans = 0;
  for (size_t i = 0 ; i < END_POS ; ++i) ans += pos[i] * other.pos[i];
  return ans;
}

// Given a multiword _word_ and splitting information, how much credit
// should we assign because the substrings are "good"?  Start by
// figuring out how many of the words are suggested by the clue, and
// how many are not.

// One catch: if there is a rebus character in the word, it's just too
// confusing to count matching and nonmatching words.  So we just call
// them all nonmatching in this case.  (The matching/nonmatching thing
// is of limited value in any case, oddly enough.)

pair<unsigned,unsigned>
search_node::matching_words(const string &wrd, const word &slot,
                            const segmentation &splits) const
{
  for (auto ch : wrd) if (rebus_char(ch)) return make_pair(0,splits.size());
  unsigned match = 0, unmatch = 0;
  unsigned idx = 0;
  for (auto s : splits) {
    if (slot.suggested[wrd.substr(idx,s)]) ++match;
    else if (s > 2) ++unmatch;
    idx += s;
  }
  if (verb() > 1) {
    unsigned idx = 0;
    for (unsigned char s : splits) {
      cout << wrd.substr(idx,s) << ' ';
      idx += s;
    }
    cout << "&&&&& " << slot.clue << endl;
  }
  if (verb() > 2) cout << wrd << " splits " << splits << " matches " << match 
                       << " unmatches " << unmatch;
  return make_pair(match,unmatch);
}

// Basic multiword scoring primitive.  The multidata is a map from
// segmentation to pdata, and therefore contains information about a
// variety of ways to split the word up into segments.  We want to use
// the one that scores the best in terms of POS and whether the words
// themselves somehow seem relevant.  The basic formula is

// ans = base * mbase * (1 + unmatch * # bad-words)

// (a similar factor for matching words doesn't help), also adjusted
// favorably if you can replace words with roots and get something in
// the database.

float search_node::score_multiword(const string &wrd, unsigned dictnum, 
                                   const word &slot, const multidata &md) const
{
  float base = d->score_multiword(dictnum);
  bool parse = d->parseable(wrd,1);
  const pdata &pos = slot.wfiller.get_pos_prob();
  float bestans = -1;
  for (auto &m : md) {
    float ans = base;
    if (m.second.ct && pos.ct) {
      float p = m.second.convolve(pos);
      ans -= detail(PWEIGHT_M) * log(max(p,detail(PBOUND)));
    }
    pair<unsigned,unsigned> match = matching_words(wrd,slot,m.first);
    ans *= detail(MATCHBASE);
    ans *= (1 + detail(UNMATCH) * match.second);
    if (d->roots_are_word(wrd,m.first)) ans -= detail(MULTI_ROOT);
    unsigned ms = max_split(wrd.size(),slot.fitb,dictnum);
    ans -= (ms - m.first.size()) * detail(EXTRA_WORD_CHARGE);
    if (verb() > 2)
      cout << ": " << ans << ((bestans < 0 || ans < bestans)? " *" : "") << endl;
    if (bestans < 0 || ans < bestans) bestans = ans;
  }
  if (!parse) bestans += detail(NO_PARSE);
  if (verb() > 2)
    cout << "score " << wrd << " with dict " << dictnum << " produces base "
         << base << " parse " << parse << " and answer " << bestans << endl;
  return bestans;
}

// Here we have a successful split of _word_ into subwords (a
// segmentation is a vector of lengths for the subwords).  We want to
// compute a vector of possible parses for the given string.

// First, get possible parses for each word into p.  Then serialize p
// to get the answer.

vector<parse> database::do_parse(const string &word, const segmentation &segs)
  const
{
  unsigned idx = 0;
  vector<vector<POS>> p;
  for (auto s : segs) {
    p.push_back(pos(word.substr(idx,s)));
    idx += s;
  }
  vector<vector<POS>> ans = serialize(p);
  vector<parse> result;
  for (auto &p : ans) result.push_back(p);
  return result;
}

// Given a lo word (wilds replaced with 'a') and a pattern h, find all
// instances that appear in the dictionary.

// 1. Walk along, looking for a WILD in h.  If there isn't one, the
//    word is already ground and it's either in the dictionary or it
//    isn't.

// 2. Assuming that there is a WILD, we have to find the first
//    dictionary entry that might be relevant.  If the first
//    dictionary entry of l or lower (one less than the upper_bound of
//    l) matches l itself for the first idx characters, than that
//    entry is the place to start.  Otherwise, the place to start is
//    the next entry, and if that doesn't match either, nothing will
//    and we're done.

// 3. The "high" word is just the upper bound, so it's easier.

// 4. If there are < WALKIT words to consider, we just examine each of
//    them because all of this weird iteration is expensive.

// 5. Otherwise, we look for every character in this slot between the
//    low and high entries, and call ground_word recursively.

set<ultralong> wordset::ground_word(ultralong &l, ultralong &h, unsigned idx) 
  const
{
  static unsigned WALKIT = 50;
  unsigned len = h.length();
  set<ultralong> ans;
  for ( ; idx < len ; ++idx) if (h.get_char(idx) == WILD) break;        // 1
  if (idx == len) {
    if (includes(h)) ans.insert(h);
    return ans;
  }
  unsigned lo = upper_bound(l);                                         // 2
  ultralong lostring;
  bool match = false;
  if (lo) {
    lostring = getultra(--lo,len);
    match = lostring.match(l,idx);
    if (!match) ++lo;
  }
  if (!match) {
    if (lo == size(len)) return ans;
    lostring = getultra(lo,len);
    if (!lostring.match(l,idx)) return ans;
  }
  unsigned hi = upper_bound(h);                                         // 3
  if (hi - lo < WALKIT) {       // note hi == 0 will get handled here
    for (unsigned i = lo ; i < hi ; ++i) {                              // 4
      ultralong poss = getultra(i,len);
      bool match = true;
      for (unsigned j = idx ; j < len ; ++j)
        if (poss.get_char(j) != h.get_char(j) && h.get_char(j) != WILD) {
          match = false;
          break;
        }
      if (match) ans.insert(poss);
    }
    return ans;
  }
  const ultralong &histring = getultra(hi - 1,len);                     // 5
  for (char c = lostring.get_char(idx) ; c <= histring.get_char(idx) ; ++c) {
    l.set_char(idx,c - 'a');
    h.set_char(idx,c - 'a');
    set<ultralong> other = ground_word(l,h,idx + 1);
    ans.insert(other.begin(),other.end());
  }
  l.set_char(idx,0);
  h.set_char(idx,NUMCHARS);
  return ans;
}

// ground_word on a string.  Construct l as above, and you're good to go.

set<string> wordset::ground_word(const string &w) const
{
  string lo = w;
  for (auto &ch : lo) if (ch == WILD) ch = 'a';
  ultralong l(lo);
  ultralong h(w);
  set<ultralong> u = ground_word(l,h,0);
  vector<string> ans;
  for (auto &i : u) ans.push_back(i);
  return set<string>(ans.begin(),ans.end());
}

// Here we finally can check to see that a string is a multiword.
// It's assumed that the [0,idx) substring *is* a multiword, and that
// the splittings for that multiword are in _prior_.  There are
// num_left additional words allowed (or fewer), and we have a rule
// that you can't have two one letter words in a row, so one_ok
// indicates whether or not the next chunk can be a single letter.

// 1.  Boundary cases
// 1a.  If the word is too long, give up because split_cache can't handle it.
// 1b.  if the word is completely processed, you're done.
// 1c.  If no words are left, it's hopeless.  

// In the interesting case, we save time by recording the legal
// groundings for every subsequence, since many of these appear over
// and over again.  The split_cache is an array of these, and a
// similar array to indicate whether or not they've been computed on
// this call to sequence.

// 2.  Set lo to the smallest "nibble" we could take at this point.
//     It's one character (or two, depending on one_ok), but if this
//     is the last word the it's got to be the whole string (i.e.,
//     what's left after idx characters are already processed).  Then
//     for nibbles of this size or greater we perform the main loop.

// 3.  If we haven't yet analyzed this substring, we do so and store
//     the result.

// 4.  If there are no groundings, we're done.

// 5.  Otherwise, we convolve the groundings for this substring with
//     the groundings in _prior_, call the function recursively and
//     merge the result into the overall answer.

splitting database::sequence(const string &w, const wordset &words,
                             unsigned num_left, const splitting &prior,
                             split_cache &sc, unsigned idx, bool one_ok) const
{
  if (idx == 0 && w.size() > LONG_WORD) return prior;           // 1a
  if (idx == w.size()) return prior;                            // 1b
  splitting ans;
  if (num_left == 0) return ans;                                // 1c
  unsigned lo = one_ok? 1 : 2;
  if (num_left == 1) lo = max(lo,(unsigned) w.size() - idx);
  for (unsigned i = lo ; idx + i <= w.size() ; ++i) {
    if (i > LONG_WORD) continue;
    if (!sc.found[idx][i]) {
      sc.found[idx][i] = true;
      sc.groundings[idx][i] = words.ground_word(w.substr(idx,i));
    }
    const set<string> &curr = sc.groundings[idx][i];
    if (curr.empty()) continue;
    splitting np;               // splitting so far
    for (auto &j : curr)
      for (auto &k : prior)
        np.insert(make_pair(k.first + j,                           // string part
                            k.second + segmentation(1,j.size()))); // seg part
    splitting na = sequence(w,words,num_left - 1,np,sc,idx + i,i != 1);
    // splitting from here on; needs to be pushed into answer
    ans.insert(na.begin(),na.end());
  }
  return ans;
}

// Have the multiwords been computed for this word?  Yes if either
// it's too short to *have* multiwords, or if the total number of
// unfilled cells is MAX_BLANKS or less.

bool basic_word::multi_done(unsigned dict) const
{
  return size() > LONG_WORD || max_split(size(),fitb,dict) == 0 || 
    unfilled() <= MAX_BLANKS;
}

// Given a pattern, we're going to construct the multiwords by getting
// a mapping of strings to collections of parses, which then get
// convolved against the known pdata for the clue in question.  Here
// is the first part of that.

// If no split is allowed, it's trivial.  Otherwise, we start with a
// list containing a single empty segmentation, and use sequence above
// to generate a whole splitting.  Then for each element of the
// splitting, we parse it using segmentation::parse to produce a
// vector of possible parses.  This vector is folded into the parses
// for the given string.

// nonwords_only means that if the whole string is a word, don't count
// it.

map<string,multiparse> database::get_multiparses(const string &pat,
                                                 bool fitb, unsigned dict,
                                                 bool nonwords_only) const
{
  map<string,multiparse> ans;
  if (pat.size() > LONG_WORD) return ans;
  unsigned lim = max_split(pat.size(),fitb,dict);
  if (!lim) return ans;
  splitting init;
  init.insert(make_pair("",segmentation()));
  split_cache sc;
  for (auto &i : sequence(pat,multidicts[dict],lim,init,sc))
    if (!(nonwords_only && i.second.size() == 1 && includes(i.first,false)))
      ans[i.first][i.second] = do_parse(i.first,i.second);
  return ans;
}

// Does the multiword parse like a normal clue?  All the pieces are there:
//  get_multiparses returns a map<string,multiparse>
//  each multiparse has a vector<parse> as its second element; get them all
//  see if each is in the clue parsing DB

bool database::parseable(const string &pat, unsigned dict) const
{
  vector<parse> all_parses;
  auto base = get_multiparses(pat,false,dict,true);
  if (base.empty()) return false;
  for (auto &p : base.begin()->second)
    all_parses.insert(all_parses.end(),p.second.begin(),p.second.end());
  for (auto &p : uniquify(all_parses))
    if (pos_rules.find(p) != pos_rules.end()) return true;
  return false;
}

// Is a pattern in multidicts[0] (Basic English words)?

bool database::is_simpleword(const string &pat) const
{
  return multidicts[0].includes(pat);
}

// A string is a multiword if it's got a multiparse.

bool database::is_multiword(const string &pat, bool fitb, unsigned dict) const
{
  return !get_multiparses(pat,fitb,dict,true).empty();
}

// And here we put it all together.  Start with the pattern, and get
// the multiparses as above.  Then for each one, combine all the
// parses into p.  Normalize the eventual pdata and store it with the
// overall answer.

map<string,multidata> database::get_multiwords(const string &pat, bool fitb,
                                               unsigned dict) const
{
  map<string,multidata> ans;
  for (auto &i : get_multiparses(pat,fitb,dict,true))
    for (auto &j : i.second) {  // i.first is string, j.first is segments,
                                // j.second is vector<vector<char>> parse
      pdata p;
      fold_parses(j.second,p);
      p.normalize();
      ans[i.first][j.first] = p;
    }
  return ans;
}

// Of course, using all of this is a challenge because of the rebus
// possibility.  When we actually handle a multiword, we need to
// remove the rebus characters, then process the multiword normally.
// We don't have to adjust the multidata for the rebus length, since
// the words aren't checked for match/nonmatch in the rebus case (as
// per the documentation for ground_word).  So all we really need to
// do is substitute out the rebus characters and then call
// get_multiwords above.  But even there, it's still tricky if we have
// a bidirectional rebus -- now each rebus char might be replaced with
// the rebus word or the alternate.  So we count the number of such
// possibilities and then there are 2^n ways to do it.

// This ambiguity means that we have to return a multimap, as opposed
// to a simple map.

multimap<string,multidata> 
database::get_multiwords_with_rebus(const string &pat, bool fitb, unsigned dict,
                                    const vector<rebus_answer> &rebus) const
{
  multimap<string,multidata> ans;
  if (rebus.empty()) {
    for (auto &j : get_multiwords(pat,fitb,dict)) ans.insert(j);
    return ans;                 // convert map to multimap and return
  }
  vector<unsigned> rebus_chars;
  for (size_t i = 0 ; i < pat.size() ; ++i)
    if (rebus_char(pat[i])) {   // aggregate bidirectional entries
      unsigned reb_id = pat[i] - ('a' + NUMLETS);
      if (rebus[reb_id].alternate.size()) rebus_chars.push_back(i);
    }
  for (int i = 0 ; i < (1 << rebus_chars.size()) ; ++i) {
    string pat2 = pat;
    unsigned k = rebus_chars.size();
    for (int j = pat.size() - 1 ; j >= 0 ; --j) // replace from end
      if (rebus_char(pat[j])) {
        unsigned reb_id = pat[j] - ('a' + NUMLETS);
        if (rebus[reb_id].alternate.size() && (i & (1 << --k))) // use alternate?
          pat2.replace(j,1,rebus[reb_id].alternate);
        else pat2.replace(j,1,rebus[reb_id]);
      }
    for (auto &j : get_multiwords(pat2,fitb,dict)) ans.insert(j);
  }
  return ans;
}

// And finally we can get the multiwords into a vector of
// scored_word_sorter for use elsewhere.  We take care to use the best
// value for any words in ans that we've already figured out.  (We
// might call this function when the multidictionary index increases,
// and a worse dictionary might give a better value if some of the
// subwords are suggested by the clue.)  Now we get and score the new
// words, push them on, resort the answer, and we're done.  A couple
// of caveats: (1) We have to put the rebus characters back in if
// possible, lest we wind up with a total mess.  Doing so actually
// might mess up the length (perhaps we were only "planning" on
// replacing one string, but there were two available), in which case
// we don't count this as a viable fill. (2) If there is a known theme
// and this multiword could be the result of the theme "operation", we
// give the sore a little boost as well.  (The base phrase from which
// we're starting might not be in the dictionary.)

// The theme stuff is documented *after* this piece of code.

void search_node::get_scored_multiwords(const string &pat, unsigned dict,
                                        const word &slot, 
                                        vector<scored_word_sorter> &ans) const
{
  if (verb() > 2) cout << "checking multiwords[" << dict << "]; for " << pat
                       << " input  " << ans << endl;
  map<string,float> extras;
  for (auto &a : ans) extras[a.word] = a.score;
  for (auto &i : d->get_multiwords_with_rebus(pat,slot.fitb,dict,rebus)) {
                                // i is a <string,multidata> pair
    string ur = unrebus(slot,i.first);
    if (ur.empty()) continue;
    if (!rebus.empty() && ur.size() != pat.size()) continue;
    // replaced too many rebuses!  Note that if the new string is too
    // *big*, we may stick with it.  It's probably a geometry thing and so ok
    if (slot.wfiller[ur] != BIG_CHARGE) continue;
    float f = score_multiword(i.first,dict,slot,i.second);
    if (ur == i.first) f -= theme_bonus(slot,ur);
    // described below
    auto j = extras.find(ur);
    if (j == extras.end()) extras[ur] = f;
    else j->second = min(j->second,f);
  }
  ans.resize(extras.size());
  unsigned j = 0;
  for (auto &i : extras) ans[j++] = scored_word_sorter(i.first,i.second);
  sort(ans.begin(),ans.end());
  if (ans.empty() && find(pat.begin(),pat.end(),WILD) == pat.end()) {
    float f = theme_bonus(slot,pat);
    if (f) ans.push_back(scored_word_sorter(pat,detail(NON_DICT_CHARGE) - f));
  }
  if (verb() > 2) cout << "output: " << ans << endl;
}

// The next block of code has to do with the bonus for a multiword
// conforming the the apparent theme of the puzzle.

// First, figure out if the fill conforms to the theme.  This can
// happen strongly or weakly (see theme_conforming below).  If it
// conforms and the word should be themed (based on length of slot,
// etc), it gets the theme bonus.

float search_node::theme_bonus(const word &slot, const string &fill) const
{
  if (verb() > 2 && should_be_themed[slot.id])
    cout << "theme bonus for " << fill << " in " << slot << endl;
  if (!should_be_themed[slot.id] || puzzle_theme.empty()) return 0;
  if (slot.theme_computed.find(fill) != slot.theme_computed.end())
    return slot.theme_computed.at(fill);
  if (verb() > 2) cout << "theme " << puzzle_theme << " bonus for " << fill
                       << " in slot " << slot.id << ' ' << slot << endl;
  float ans = theme_conforming(slot,fill,true,true);
  if (verb() > 3) cout << "after strong: " << ans << endl;
  if (ans == 0 && !puzzle_theme[0].is_wild_add())
    ans = theme_conforming(slot,fill,false,true);
  if (verb() > 3) cout << "after weak: " << ans << endl;
  if (ans && should_be_themed[slot.id]) ans += THEME_REQD;
  if (verb() > 2) cout << "answer is " << ans << endl;
  return slot.theme_computed[fill] = ans;
}

// Is the current fill for a slot theme-conforming?  Just call
// theme_conforming; this is the one instance (this function is called
// only when searching for a theme) that you're willing for _quick_ to
// be false.

bool search_node::is_theme_fill(const word &slot, bool strong) const
{
  return theme_conforming(slot,slot.fill,strong,false);
}

// When you check to see if fill conforms to a theme, "strong" means
// that it is constructed in the theme way from known fill.  So if the
// theme is 'add e at the end", strong conforming means that there it
// ends with an e, and taking that e off produces a dictionary word.
// Weak just means that it ends with an e.

// Anagrams and portmanteaus are handled specially in the weak case.
// Anything is an anagram (you cant do anything more clever via
// multiwords, as far as I can tell).  Similarly, anything can be a
// portmanteau.  Those are both only handled if quick is false, so we
// know we're ok with approximately.

// Having done all of this, sourced_them::conforms does the real work.
// We return as soon as we find a match for one of the themes being
// considered.

float search_node::theme_conforming(const word &w, const string &fill,
                                    bool strong, bool quick) 
  const
{
  for (auto &pt : puzzle_theme) {
    if (!strong && !quick) {    // special purpose portmanteau/anagram analysis,
                                // since they return false if conforms is called
      vector<theme_fill> tfills;
      switch (pt.get_type()) {
      case PORTMANTEAU: {
        theme_fill tf;
        tf.original = tf.poriginal = tf.source = tf.psource = fill;
        tfills.push_back(tf);
        break;
      }
      case ANAGRAM: if (!pt.get_sound()) tfills = d->get_themewords(fill); break;
        // only non-sound anagrams are weakly conforming, lest the
        // call to get_themewords generates many calls to flamingo
        // (one for each possible pronunciation)
      default: break;
      }
      for (auto &tf : tfills) 
        if (!pt.get_hints(tf,*d).empty()) return THEME_WEAK;
    }
    float ans = pt.conforms(w,fill,strong,*d,verb() > 2);
    if (ans) return ans;
  }
  return 0;
}

// Here is the money function; return the nth fill for a given slot
// and a given pattern.  The basic plan is as follows.

// 1. Look in the non-multiword database.  This is split by whether or
// not there are wildcards (blanks) in the query.

// 1a. If there *are* blanks, then get the appropriate fill indexer
// and find the requested match if possible.  Return it if that
// worked.
// 1b. If there are no blanks, then if n > 0, it's hopeless.  If n == 0,
// look in the filler for this slot, returning the word if you find it.

// 2. If that didn't work, we have to check multiwords.  If we haven't
// already computed multiwords for this pattern (case 2a), then the
// index offset into the multiwords is the current index (which must
// be the first one failing).  We find the multiwords based on any
// allowed dictionaries, and cache the result.

// At this point (2b), we can either respond to the query or we can't.

scored_word search_node::get_fill(const word &w, const string &pat,
                                  unsigned n) const
{
  unsigned unf = 0;
  for (auto ch : pat) if (ch == WILD) ++unf;

  // non-multiword database
  if (pat.size() == w.size()) {
    if (unf) {                                                  // 1a
      const scored_word &nonextra = w.indexer->find_match(pat,n);
      if (nonextra.score != BIG_CHARGE) return nonextra;
    }
    else {                                                      // 1b
      if (n) return hopeless;
      float f = w.wfiller[pat];
      if (f != BIG_CHARGE) {
        if (verb() > 2) cout << "found in DB: " << pat << ' ' << f << endl;
        f -= theme_bonus(w,pat);
        return scored_word(pat,f);
      }
    }
  }

  // multiword search
  if (unf > MAX_BLANKS) return hopeless;
  unsigned cutoff = (unf <= MAX_BLANKS2); // allowed dictionary

  map<string,extra> &x = w.xtra;
  if (x.find(pat) == x.end()) {                                 // 2a
    x[pat].offset = n;
    if (pat.size() <= LONG_WORD && max_split(pat.size(),w.fitb,0)) 
      for (size_t i = 0 ; i <= cutoff ; ++i) {
        if (i && should_be_themed[w.id] && !puzzle_theme.empty() &&
            puzzle_theme[0].sources >= 3)
          i = 2;
        // this allows any 2-letter word in, which is fine if you've
        // figured out the theme.  But if you've gotten the theme
        // wrong, you definitely don't want to do it.  That's why we
        // require lots of sources
        get_scored_multiwords(pat,i,w,x[pat]);
      }
  }
  else if (verb() > 2)
    cout << "pattern " << pat << " already considered" << endl;

  unsigned nadj = n - x.at(pat).offset;                         // 2b
  return (nadj < x.at(pat).size())? x.at(pat)[nadj] : hopeless;
}
