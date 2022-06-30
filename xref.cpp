#include "xw.h"

// Here we figure out if a clue is a cross reference.  We can return
// SEE (meaning that it's the "see x-across" part, NOX (not a cross
// reference) or the word in "with x-across, ...".

static word SEE;
static word NOX;

// This piece takes the part of the code that looks like "32-Across"
// and returns the id of the associated word.  The clue has to be of
// the form
//   nnn[- ]((a|A)cross|(d|D)own)
// and, even if all that happens, there has to be a corresponding word
// in the puzzle!

const word &puzzle::xref(const string &c) const
{
  int n = atoi(c.c_str());      // stoi fails if no number at front of c
  if (n <= 0) return NOX;
  size_t idx = 0;
  while (isdigit(c[idx])) ++idx;
  if (c[idx] != ' ' && c[idx] != '-') return NOX;
  bool across = false;
  string c2 = tolower(c);
  if (c2.substr(++idx,6) == "across") across = true;
  else if (c2.substr(idx,4) != "down") return NOX;
  for (auto &w : *this)
    if (w.across() == across && w.numbers[0] == (unsigned) n) return w;
  return NOX;
}

// Get the cross reference for the clue for word i.  If it's of the
// form "See 32-a", return SEE.  If "With 32-a", return the cross
// reference as computed above.  Otherwise, return NOX.

// In actuality, this is a nightmare because we really have to get it
// right or we get all sorts of bizarre race conditions in the
// multithreaded initialization because multiple threads will be
// trying to initialize a single word.  So we have to find the
// appropriate pattern, and we also have to check that it works the
// other way around.  Of course, that secondary check doesn't have to
// do a tertiary check, etc.  There are two cases:

// 1. If this word appears to be "See 4-Across", then we have to check
// that 4-Across actually points back to this word.

// 2. If this word appears to be "With 4-Across ..." then we have to
// check that 4-Across is a SEE and furthermore, it points back to
// this word.

const word &puzzle::xref(const word &w, bool recur) const
{
  string c = tolower(w.clue);
  if (c.substr(0,3) == "see" && c.size() > 3) {
    const word &w2 = xref(c.substr(4));
    if (&w2 != &NOX && (!recur || &xref(w2,false) == &w)) // check #1 above
      return SEE;  
    else return NOX;
  }
  size_t idx = c.find("with");
  if (idx == string::npos || idx + 5 > c.size()) return NOX;
  const word &w2 = xref(c.substr(idx + 5));
  if (&w2 == &NOX) return NOX;
  if (w.size() + w2.size() > LONG_WORD) return NOX;
  if (!recur) return w2;
  if (&xref(w2,false) != &SEE) return NOX;
  if (&xref(w2.clue.substr(4)) != &w) return NOX; // check #2
  return w2;
}

// And here is where we actually collapse two words in a crossref.  If
// this is a crossref, w is the first word and see is the second.  First,
// we make sure the combo isn't too long!  Then in the normal case, we:

// 1. Adjust splits and numbers to "remember" the original second
//    word.  splits is a vector of the squares marking the beginnings
//    of the various words, so we just push on the length of the first
//    word.
// 2. Push the squares in _see_ onto the original word.
// 3. Adjust the fill of the combined word to be longer.
// 4. Get the answer for the combined word from the two subwords
// 5. Remove the "with xx-across" (or whatever) from the clue for the
//    combined word.

// At the end, we remove all the words marked for removal.  (It's hard
// to do it sooner, because the word we're removing might be before
// the word we're working on.)

void puzzle::collapse_xrefs()
{
  vector<bool> to_remove(size(),false);
  vector<unsigned> removals;
  for (size_t i = 0 ; i < size() ; ++i) {
    if (to_remove[i]) continue;                      // already a crossref
    word &w = (*this)[i];
    const word &see = xref(w,true);
    if (see.id == unsigned(-1)) continue;            // not a crossref
    if (w.size() + see.size() > LONG_WORD) continue; // can't handle it!
    w.splits.push_back(w.size());                    // 1
    w.honor_splits = true;
    w.numbers.push_back(see.numbers[0]);
    w.insert(w.end(),see.begin(),see.end());         // 2
    w.fill.resize(w.size(),WILD);                    // 3
    w.answer = w.answer + see.answer;                // 4
    string c = tolower(w.clue);                      // 5
    size_t widx = c.find("with");
    size_t idx = 5 + widx; // following space
    while (isdigit(c[idx])) ++idx;
    ++idx;                      // space or hyphen
    idx = 1 + c.find(' ',idx);  // include space in what's cut out
    w.clue = w.clue.substr(0,widx) + w.clue.substr(idx);
    to_remove[see.id] = true;
    removals.push_back(see.id);
  }
  remove_words(removals);
}

// When we put crossreference fill into a clue, we want to turn it
// back into multiple words if it's a multiword.  This does that, with
// the goal being to accept as input something like "bestfriend" and
// return the string "best friend".

// This is hopeless if:

// 1. The word is not yet grounded.
// 2. The word appears in the root database as a non-root.
// 3. We can't get the word as a list of words (we try the small
//    multi-dictionary, then the biggers one).

// When we're done with #3, if we've got a multiparse, the first one
// is as much as we need.  That's constructed in mp in step 4 below.
// We only need the first part of each multiparse, which is a
// segmentation, a vector<unsigned char> where each element is the
// length of the word segment.  The splitting we actually use is the
// one with the smallest first word, computed in step 5 below.

// Having constructed that, we just rebuild the word string, adding
// spaces before any word other than the first.

string database::resplit(const string &str) const
{
  for (auto ch : str) if (ch == WILD) return str;               // 1
  if (roots.find(str) != roots.end()) return str;               // 2
  // better to check in word_to_score??
  map<string,multiparse> split = get_multiparses(str,false,0,false);  // 3
  if (split.empty()) split = get_multiparses(str,false,1,false);
  if (split.empty()) return str;
  unsigned smallest = LONG_WORD;
  segmentation s;
  const multiparse &mp = split.begin()->second;                 // 4
  for (auto &m : mp)                                            // 5
    if (m.first.size() > 1 && m.first.size() < smallest) {
      smallest = m.first.size();
      s = m.first;
    }
  string result;
  unsigned idx = 0;
  for (size_t i = 0 ; i < s.size() ; ++i) {
    if (i) result += ' ';
    result += str.substr(idx,s[i]);
    idx += s[i];
  }
  return result;
}

// Given a clue mentioning another, find the point after the "across"
// or "down"

unsigned xref_idx(const string &clue, unsigned idx = 0)
{
  unsigned ac = min(clue.find("Across",idx),clue.find("across",idx));
  unsigned dn = min(clue.find("Down",idx),clue.find("down",idx));
  return (ac < dn)? (ac + 6) : (dn + 4);
}

// Find all the crossrefs in the puzzle.  Note that we do this both
// when we first get started (and we're adjusting the words, etc), and
// later, to handle a clue like "It's bigger than 2-Across" where we
// just take the fill from 2-Across and drop it into the revised clue.
// As we do all this, we keep track of the current (possibly adjusted)
// clue and the original clue as well.

// The test to see if this is a clue with a cross reference takes a
// tiny amount of time, and we arrange to bypass it most of the time
// by simply not filling the "original clue" slot unless this actually
// is a cross-reference clue.  So on subsequent passes (_initial_ is
// false), if a word has no original clue, we can ignore it.  We can
// also obviously ignore it if it's seen not to be a cross reference
// clue.

// If it *is* a cross reference clue, we copy the clue to the original
// clue slot if we have not yet done so.  We get w2 to be the word
// pointed to by the cross reference (the xref function expects just
// the part of the clue beginning 32-Across or whatever).  If there is
// no such word, we just give up and keep going.  Otherwise, we build
// up the new clue, replacing "26-Across" with the split version of
// w2's fill.  If this leads to a change in the clue, we reinitialize
// the slot (which will create new scores for the filler, etc).

void search_node::find_xrefs(bool initial)
{
  for (auto &w : *this) {
    if (!initial && w.original_clue.empty()) continue;
    if (&xref(w,true) == &NOX) continue;
    if (initial) w.original_clue = w.clue;
    string clue = w.original_clue;
    for (size_t j = 0 ; j < clue.size() ; ++j) 
      if (isdigit(clue[j])) {
        const word &w2 = xref(clue.substr(j));
        if (&w2 == &NOX) continue;
        unsigned l = xref_idx(clue,j);
        clue = clue.substr(0,j) + d->resplit(w2.fill) + clue.substr(l);
      }
    if (clue == w.clue) continue;
    w.clue = clue;
    initialize_slot(w);
  }
  d->get_berkeley_results(*this);
}
