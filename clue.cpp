#include "xw.h"

// Basic datum construction.  Set things from the arguments as
// supplied, but we set some variables to values that have logs just
// to make things a bit simpler later.  

basic_datum::basic_datum(float z, bool is_abbr, float fa, bool ftb,
                         const basic_datum &s)
{
  *this = s;
  score = BIG_CHARGE;
  pos = 1;
  dict = z;
  is_dict = (z != 0);         // z == 0 means not in the dictionary
  abbr = is_abbr;             // is it?
  fabbr = is_abbr? fa : 1;    // if not, set fabbr to 1
  paren = moby = false;
  fitb = ftb;
  nonword = (z == 0);         // assumes FITB false
  if (ftb) { clue = 0; nonword = false; } // fixes that if need be
}

// This code evaluates various fills for a given slot.  The following
// functionality is supported:

// 1. Score based on clue
// 2. Score based on dictionaries
// 3a. Score based on stripping (out) in [Squeeze (out)] and similar
// 3b. Score based on [Vision leader] for tele and similar, incl. follower
// 3c. Score based on fill-in-the-blank (FITB) analysis

// SCORING 1: Based on the clue

// Clue-based evaluation.  Arguments:

// len             length of word slot being filled
// splits          length of subwords in See/With clue
// clue            clue for this slot
// exact_only      restrict to exact matches?
// specials        restrict to these words
// fills           RESULT: words and scores (modified destructively)
// fitb            results from FITB
// clue_abbr       should it be an abbreviation?

// When inserting the words that are found independent of the clue, we
// *don't* want to insert a word that has already been inserted as
// responsive.  At the end, we push back a dummy word indicating that
// we've evaluated this list completely.  

// In the call _specials_ is fills that we want to examine explicitly
// (this is to help debug the scoring modules).

// The way this works is that we push stuff from either the clue or
// the dictionary into a vector of words and scores called _fills_.
// Here is the clue part; an analagous function dealing with rebus
// fill is in rebus.cpp.

float database::abbr_prob(const string &str) const {
  return (str.size() > ABBR_LIM)? 1 : is_abbr[str];
}

void database::insert_from_clue(unsigned len, const vector<unsigned> &splits,
                                const string &clue, bool exacts_only,
                                const vector<string> &specials,
                                vector<scored_word> &fills,
                                const set<string> &fitb, bool clue_abbr) const
{
  for (auto &i : clue_prob(len,clue,exacts_only,specials)) {
    basic_datum bd(get_score(i.first,splits),  // non-clue prob
                   clue_abbr,
                   abbr_prob(i.first),         // is fill an abbr?
                   fitb.find(i.first) != fitb.end(), // FITB?
                   i.second);                  // clue-based prob
    fills.push_back(scored_word(i.first,BIG_CHARGE,bd));
  }
}

// SCORING 2: Based on dictionaries

// Find Moby thesaurus matches for a one-word clue, including matches
// at the root level.  Just go through the Moby entries, finding all
// the ones that are relevant.

set<string> database::moby_matches_for_clue(const string &clue, unsigned len)
  const
{
  string mclue = tolower(clue);
  vector<string> vec;
  for (auto &m : moby)
    if (binary_search(m.begin(),m.end(),mclue))
      for (auto &s : m) if (s.size() == len) vec.push_back(s);
  set<string> ans(vec.begin(),vec.end());
  ans.erase(mclue);
  return ans;
}

// And here is the analogous dictionary part.  In the (normal)
// no-splits case, we can get the score without finding it, because we
// know where in the dictionary we are.  In the split case, it's more
// complex and we just let existing code do it.

// At the end, any FITB entry can't score worse than FITB_SCORE, even
// if they wewren't even there previously.

// NOTE: Since you are adding fills as you go and only the existing
// fills are known to be sorted, you have to get f in the marked spot
// and use that (rather than fills.end()) in the binary search.

// NOTE: The Moby thesaurus stuff is actually here, although it's
// really clue-based.  The reason is that we *don't* do a Moby thing
// for anything where there is a normal clue-based match.  And once
// we're just looking at things other than that, we might as well
// stick it after the dictionary stuff, since we're just flipping the
// bd.moby flag for various entries.

void database::insert_from_dict(unsigned len, const vector<unsigned> &splits,
                                const string &clue,
                                const vector<string> &specials,
                                vector<scored_word> &fills,
                                const set<string> &fitb, bool clue_abbr) const
{
  sort(fills.begin(),fills.end());
  unsigned f = fills.size();    // can't avoid this!
  set<string> moby_matches;
  if (clue.find(' ') == string::npos)
    moby_matches = moby_matches_for_clue(clue,len);

  for (size_t i = 0 ; i < word_to_score.size(len) ; ++i) {
    const string &str = word_to_score.get_key(len,i);
    if (binary_search(fills.begin(),fills.begin() + f,scored_word(str,0)))
      continue;
    float sc;
    if (splits.size() < 2) sc = get_score(len,word_to_score.get_value(len,i));
    else sc = get_score(str,splits);
    basic_datum bd(sc,clue_abbr,abbr_prob(str),
                   fitb.find(str) != fitb.end());
    if (moby_matches.find(str) != moby_matches.end()) {
      bd.moby = true;
      bd.clue = 1.0 / moby_matches.size();
    }
    fills.push_back(scored_word(str,BIG_CHARGE,bd));
  }
  float fsc = detail(FITB_SCORE);
  for (string f : fitb) {
    bool badword = false;
    for (auto &c : f)
      if (!isalpha(c)) { badword = true; break; }
      else c = tolower(c);
    if (badword) continue;
    auto i = find(fills.begin(),fills.end(),scored_word(f,0));
    if (i == fills.end()) fills.push_back(scored_word(f,fsc));
    else i->score = min(i->score,fsc);
  }
}

// SCORING 3a: Based on parentheses

// If there is a parenthetical remark at the beginning or end of a
// clue, we should also try dropping those, and the fill gets extra
// credit if they can be appended as well.  So, for example [Expect
// (of)] clues ASK because [Expect] does, and the fact that ASKOF is
// in the dictionary counts extra.  Here, we strip off the
// parenthetical stuff.

string unparen(const string &orig, string &left, string &right)
{
  size_t l = 0, r = orig.size();
  if (orig[0] == '(') {
    l = orig.find(')');
    if (l == string::npos) l = 0;
    else {
      left = orig.substr(1,l - 1);
      ++l;
      while (isspace(orig[l])) ++l;
    }
  }
  if (orig.back() == ')') {
    r = orig.rfind('(');
    if (r == string::npos) r = orig.size();
    else {
      right = orig.substr(r + 1);
      right.resize(right.size() - 1);
      while (r > 0 && isspace(orig[r - 1])) --r;
    }
  }
  if (r <= l) return string();
  return orig.substr(l,r - l);
}

// Actually crediting ASK as above is easy; we just look it up in
// word_to_score.

void database::credit_parentheticals(const string &left, const string &right,
                                     vector<scored_word> &fills) const
{
  for (auto &f : fills)
    if (includes(left + f.word + right,false)) f.paren = true;
}

// Process possible parentheticals.  clue is the original clue; clue1
// has the parenthetical piece stripped out.  lparen is before the
// paren; rparen is after it.  fills is possible fill.
// credit_parentheticals builds the compound words and sets the
// appropriate flag if they're in the dictionary.

void database::clue_processing(unsigned len, const string &clue,
                               const string &clue1,
                               string &lparen, string &rparen,
                               vector<scored_word> &fills) const
{
  if (clue != clue1 && strip_spaces(lparen) && strip_spaces(rparen) && 
      lparen.size() + len + rparen.size() <= LONG_WORD)
    credit_parentheticals(lparen,rparen,fills);
}

// SCORING 3b: Leader/follower

// Is this a leader or follower clue?  Return a vector of the
// non-leader/follower part(s) and set leader to true or false
// depending on what's requested.  The way it works is that these
// clues are identified by the presence of a _lead_follow_phrase_, and
// each such phrase can indicate that the fill is a leader (precedes
// the balance of the clue) or doesn't.  fixed_word_follows indicates
// whether the "balance of the clue" follows the fixed part, so we can
// handle, for example, [Commercial prefix for Clean] and have OXI
// score well.  allows_split means that there might be multiple FITBs,
// like [Word after x or y].  At the end, we try to handle clues like
// [Completely, after "in"] cluing TOTO.  We can't do much, but [In
// ___] is better than what we've got.

vector<string> lead_follow(const string &orig, bool &leader)
{
  static vector<string> lead_follow_phrases = {
    "leader", "leadin", "opener", "opening", "front",
    "follower", "introduction", "word before",
    "word after", "prefix with", "prefix before", "suffix after",
    "suffix with"
  };
  static bool lead_follow_leader[] = { true, true, true, true, true,
                                       false, true, true,
                                       false, true, true, false,
                                       false };
  static bool fixed_word_follows[] = { false, false, false, false, false,
                                       false, false, true,
                                       true, true, true, true,
                                       true };
  static bool allows_split[]       = { false, false, false, false, false,
                                       false, false, true,
                                       true, true, true, true, 
                                       true };
  vector<string> ans;
  size_t idx = orig.find(' ');
  if (idx == string::npos) return ans;
  string str = tolower(orig);
  for (unsigned i = 0 ; i < lead_follow_phrases.size() ; ++i)
    if ((idx = str.find(lead_follow_phrases[i])) != string::npos) {
      leader = lead_follow_leader[i];
      string str;               // clue without lead_follow_phrase
      if (fixed_word_follows[i]) 
        str = orig.substr(idx + lead_follow_phrases[i].size() + 1);
      else str = orig.substr(0,idx - 1);
      ans.push_back(str);
      if (!allows_split[i]) break;
      idx = ans[0].find(" or ");
      if (idx == string::npos) break;
      // [x or y follower] is already [x or y]; now we make it [x] and [y]
      ans.push_back(ans[0].substr(0,idx));
      ans.push_back(ans[0].substr(idx + 4));
      break;
    }
  if (!ans.empty()) return ans;
  idx = orig.find(", after ");
  if (idx == string::npos) idx = orig.find(", before ");
  if (idx == string::npos) return ans;
  return lead_follow("Word" + orig.substr(idx + 1),leader);
}

// Having converted the lead/follow clue to FITB form, we just call
// fitb to do the analysis.

set<string> database::do_lead_follow(unsigned len, const string &clue,
                                     bool leader, unsigned limit) const
{
  if (clue.empty()) return set<string>();
  string fitb_clue = leader? ("___ " + clue) : (clue + " ___");
  return fitb(len,fitb_clue,limit,true);
}

// SCORING 3c: FITB analysis

// FITB lookup.  First, make sure that there's a blank in the clue!
// Now call ::fitb to look in the various corpora we've got.  Then in
// many cases, we actually want to return now because we don't want
// (for example) [ad ___] for a 3-letter fill REM to match every ad???
// in the dictionary.  But if we don't have anything, or we want to
// consider the dictionary (many "follower" clues don't break at a
// word boundary), we build a clue that has only letters and blanks,
// ground it using ground_word, and construct the answers from that.

set<string> database::fitb(unsigned len, const string &clue, unsigned limit,
                           bool use_dict) const
{
  if (clue.find("___") == string::npos) return set<string>();
  vector<string> f = fitb(len,clue,limit);
  set<string> ans(f.begin(),f.end());
  if (use_dict || ans.empty()) fitb_from_dict(len,clue,0,ans);
  return ans;
}

// Either the FITB stuff didn't work, or we want to try all the
// possible words.  So we actually can use the filler stuff to do
// this.  We build up clue2, which is just the basic clue but with
// spaces etc removed and ___ turned into len WILD characters.  Now
// ground_word actually does the work!  Then we just collect the
// results.

void database::fitb_from_dict(unsigned len, const string &clue, int cutoff,
                              set<string> &ans) const
{
  string clue2;
  bool filler = false;
  unsigned fpos = 0;
  for (auto ch : clue)
    if (isalpha(ch)) clue2.push_back(tolower(ch));
    else if (!filler && ch == '_') {
      fpos = clue2.size();
      clue2 += string(len,WILD);
      filler = true;
    }
  if (clue2.size() && clue2.size() <= LONG_WORD) {
    set<string> spl = word_to_score.get_keys().ground_word(clue2);
    for (auto &s : spl)
      if (cutoff == 0 || word_to_score[s] >= cutoff)
        ans.insert(s.substr(fpos,len));
  }
}

// If a whole string is in quotes, remove them.  (For clues that
// actually *are* quotes, typically with blanks.)

string unquote(const string &orig)
{
  if (!orig.empty() && orig[0] == '\"' && orig.back() == '\"')
    return orig.substr(1,orig.size() - 2);
  return orig;
}

// Convert a clue as needed by the FITB stuff.  Two steps: remove any
// surrounding quotes, and convert clues of the form "a-b link" to "a
// ___ b", and similarly for "a/b link".  Also, if it is (or becomes)
// a clue like [New York's ___ Square Garden] we try keeping just the
// part after the possessive.

vector<string> convert_fitb(const string &orig)
{
  string unq = unquote(orig);
  vector<string> ans(1,unq);
  size_t sp = unq.find(' ');
  if (sp != string::npos && unq.substr(1 + sp) == "link") {
    size_t punc = unq.find('/');
    if (punc >= sp) punc = unq.find('-');
    if (punc < sp)
      ans.push_back(unq.substr(0,punc) + " ___ " + 
                    unq.substr(1 + punc,sp - (1 + punc)));
  }
  size_t blank = unq.find("___");
  if (blank == string::npos) return ans;
  size_t poss = unq.find("'s ",4); // exclude "it's" and stuff
  if (poss < blank && poss + 3 + 4 + 3 < unq.size())
    // 3 for "'s " then 4 for "___ ", then at least 3 for something useful
    ans.push_back(unq.substr(poss + 3));
  return ans;
}

// SCORING 3 (combines 3a-3c): If 3b (lead/follow) works, get all the
// fills suggested thereby and you're done.

// Otherwise, pull the parenthetical comment and set lparen/rparen to
// indicate where it was.  Convert to FITB and get the fills that show
// up.  Return the clue without the parenthetical.

// So overall, this function pushes new suggested fills into _fills_
// and returns a modified clue that doesn't have any of this stuff in
// it.

string database::special_suggestions(unsigned len, unsigned fitb_limit,
                                     const string &clue, set<string> &fills,
                                     string &lparen, string &rparen) const
{
  bool leader;
  vector<string> clue2 = lead_follow(clue,leader);
  if (!clue2.empty()) {
    map<string,unsigned> matches;
    for (auto &c : clue2)
      for (auto &ff : do_lead_follow(len,c,leader,fitb_limit)) ++matches[ff];
    unsigned best = 0;
    for (auto &p : matches) best = max(best,p.second);
    if (best)
      for (auto &p : matches) if (p.second == best) fills.insert(p.first);
    return clue;
  }
  string clue1 = unparen(clue,lparen,rparen);
  for (auto &oc : convert_fitb(clue1)) {
    set<string> other = fitb(len,oc,fitb_limit,false);
    fills.insert(other.begin(),other.end());
  }
  return clue1;
}

// Compute scores for all currently unscored fill possibilities.

void database::score_fills(const pdata &pp, vector<scored_word> &fills) const
{
  for (auto &f : fills)
    if (f.score == BIG_CHARGE) f.score = word_score(f.word,pp,f);
}

// Combine scoring types 1-3 above.

// 1. Scoring type 1: fills from the clue, both (1a) simplified and
// (1b) original clue (if simplified, exact clue matches only)
// 2. Scoring type 2: fills from the dictionary
// 3. Mark *existing* fills that are supported by type 3a (parentheses)
// 4. Score everything

void database::fill_normally(const string &clue, const string &clue1,
                             unsigned len, const vector<unsigned> &splits,
                             const vector<string> &specials,
                             vector<scored_word> &fills, bool do_dict,
                             const set<string> &f, bool clue_abbr, 
                             const pdata &pp, string &lparen, string &rparen) 
  const
{
  if (clue != clue1)
    insert_from_clue(len,splits,clue1,true,specials,fills,f,clue_abbr);     // 1a
  insert_from_clue(len,splits,clue,false,specials,fills,f,clue_abbr);       // 1b
  if (do_dict) insert_from_dict(len,splits,clue1,specials,fills,f,clue_abbr); //2

  sort(fills.begin(),fills.end());
  clue_processing(len,clue,clue1,lparen,rparen,fills);                      // 3
  score_fills(pp,fills);                                                    // 4
}

// Rebus fills are similar but there is no postprocessing.

vector<scored_word> database::fill_rebus(const vector<rebus_answer> &rebus,
                                         const string &clue, const string &clue1,
                                         const skeleton &squares,
                                         vector<scored_word> &fills,
                                         bool do_dict, const pdata &pp) const
{
  vector<scored_word> ans;
  if (rebus.empty()) return ans;
  if (clue != clue1) insert_from_clue_rebus(squares,clue1,true,pp,ans,rebus);
  insert_from_clue_rebus(squares,clue,false,pp,ans,rebus);
  if (do_dict) insert_from_dict_rebus(squares,pp,fills,ans,rebus);
  return ans;
}

// Incorporate some extra fills (3abc) into the base set (assuming
// there are any).  Resort at the end.

void database::incorporate_fills(vector<scored_word> &fills,
                                 vector<scored_word> &newfills) const
{
  if (newfills.empty()) return;
  fills.insert(fills.end(),newfills.begin(),newfills.end());
  sort(fills.begin(),fills.end());
}

// Remove duplicate fills.  As things stand, fill is sorted by word
// but not by score.  So just go through; if you've got multiple fills
// with the same string, keep only the best one.

void remove_duplicates(vector<scored_word> &fills)
{
  unsigned idx = 0, pos = 0;
  while (pos < fills.size()) {
    unsigned best = pos;
    for ( ++pos ; pos < fills.size() && fills[pos] == fills[best] ; ++pos)
      if (fills[pos].score < fills[best].score) best = pos;
    fills[idx++] = fills[best];
  }
  fills.resize(idx);
}

// Show any special fills that he requested.  

void database::show_specials(const string &clue,
                             const vector<scored_word> &fills,
                             const vector<string> &specials) const
{
  bool shown = false;
  for (auto &s : specials)
    for (auto &f : fills) if (s == f.word) {
        if (!shown) cout << '[' << clue << ']' << endl;
        shown = true;
        cout << f << endl;
      }
}

// Put it all together in the obvious way.

pdata database::suggestions(const skeleton &squares,
                            const vector<unsigned> &splits,
                            const string &origclue,
                            const vector<string> &specials,
                            vector<scored_word> &fills, bool maybe_abbr,
                            bool do_dict, unsigned fitb_limit,
                            const vector<rebus_answer> &rebus,
                            const vector<theme_fill> &tfills) const
{
  unsigned len = squares.size();
  string clue = strip_braces(origclue);
  set<string> f;
  string lparen, rparen;
  string clue1 = special_suggestions(len,fitb_limit,clue,f,lparen,rparen);
  bool clue_abbr = maybe_abbr && (len <= ABBR_LIM) && shows_abbrv(clue);
  pdata pp = cpos_prob(clue1);

  squares.analyzed[len] = true;
  fill_normally(clue,clue1,len,splits,specials,fills,do_dict,f,clue_abbr,pp,
                lparen,rparen);
  vector<scored_word> newfills = 
    fill_rebus(rebus,clue,clue1,squares,fills,do_dict,pp);
  incorporate_fills(fills,newfills);
  remove_duplicates(fills);
  show_specials(origclue,fills,specials);
  return pp;
}

// Evaluate a single possible fill, used for geometry analysis, where
// the fill may be much longer than the apparent word length and this
// will therefore not be in the precomputed database.

// insert_from_clue does all the clue-related stuff; if that finds the
// fill, we're done (although we still have to evaluate the
// basic_datum we got).  If not, we get it from the dictionary, where
// it should be, and set the moby bit because it's a high-scoring
// word.

float database::evaluate(const string &ans, const string &clue) const
{
  vector<scored_word> fills;
  pdata p = cpos_prob(clue);
  string lparen, rparen;
  insert_from_clue(ans.size(),vector<unsigned>(),strip_braces(clue),false,
                   vector<string>(),fills,set<string>(),false);
  auto itr = find(fills.begin(),fills.end(),scored_word(ans,0));
  basic_datum bd;
  if (itr != fills.end()) bd = *itr;
  else {
    float sc = get_score(ans.size(),word_to_score[ans]);
    bd = basic_datum(sc);
  }
  return word_score(ans,p,bd);
}
