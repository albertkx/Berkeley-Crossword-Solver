#include "xw.h"

/*  This file contains code used to evaluate the known words against
the clues.  We have both a _filler_ structure, which includes all the
words that fit in the given slot, and a _fill_indexer_ structure,
which is used recursively to quickly find words matching a given
pattern.  In all the pattern stuff WILD is a single character
wildcard.

Here we beging by initializing one slot.  Highlight it, compute it,
and unhighlight it.  The initialization is multithreaded, since we can
initialize different words individually.  But we have to be careful to
use a mutex on the GUI calls, lest the cout calls get all garbled
together.  */

// Tell the GUI what you know about the fill.  Tell it about the
// answer here, being careful not to "remember" anything, and then let
// the filler tell it about the best n fills.  Note that we need to
// know both the word id in the "current" form (might be crossrefs,
// geometry, etc) and in the original form.

void search_node::tell_gui(const word &curr, unsigned original)
{
  if (curr.answer.size()) { // tell the GUI about the desired answer
    scored_word sw = get_fill(curr,curr.answer);
    cout << "**GUI answer_score " << original << ' ' << sw.score << endl;
    curr.computed_scores.clear(); // no cheating possible!
    curr.xtra.clear();
    curr.theme_computed.clear();
  }
  curr.wfiller.tell_gui(original);
}

// The filler doesn't know what the *original* word info was, so we
// have to pass that in here.  Note that this is a method of the
// filler structure, so we obviously know what word list we're dealing
// with.

void filler::tell_gui(unsigned original) const
{
  // tell the GUI about the best GUI_TOP fills
  for (size_t j = 0 ; j < GUI_TOP && j < size() ; ++j) {
    unsigned n = fills_by_score[j];
    cout << "**GUI desired " << original << ' ' << j << ' ' 
         << fills_by_word.get_key(length(),n) << ' ' 
         << fills_by_word.get_value(length(),n).score << endl;
  }
}

// Slot initialization.  This is complicated because we have to cater
// to the fact that when we're done with the puzzle, we'll want to
// think about rebus entries.

// 1. Mark this word as in process in the GUI.
// 2. Get the words that are suggested by the clue.  That's all clue suggested
//    words *shorter than* the actual entry; we use them in evaluating
//    multiwords.
// 3. Then build a filler and initialize the corresponding fill indexer(s)
//    as well.
// 4. If we aren't just tuning, we're eventually going to need
//    longer candidate fills for the rebus check; that's what
//    get_rebus_suggestions does.
// 5. And then clear tell the GUI that the evaluation is complete.

void search_node::initialize_slot(word &w)
{
  w.to_initialize = false;
  w.clear_cache();
  unsigned fitb_limit = detail(FITB_LIMIT);
  vector<unsigned> originals;
  if (sets->gui) {                                                    // 1
    originals = get_simple_words(w);
    cout << guard;
    for (auto j : originals) cout << "**GUI highlight " << j << " 1" << endl;
    cout << unguard;
  }
  w.suggested = d->suggested(w.size(),w.clue);                        // 2
  w.wfiller = filler(w,w.honor_splits? w.splits : vector<unsigned>(),w.clue,
                     *d,w.id,fitb_limit,difficulty,sets->gui,rebus,w.theme_sugg,
                     check_now()? sets->to_check : vector<string>()); // 3
  w.reset_indexer();

  if (!sets->tune) get_rebus_suggestions(w);                          // 4
  if (sets->gui) {                                                    // 5
    cout << guard;
    for (auto j : originals) {
      cout << "**GUI highlight " << j << " 0" << endl;
      tell_gui(w,j);
    }
    cout << unguard;
  }
}

// Filler construction.  Following steps:

// 1. Suggestions does the bulk of the work, returning the probability
// of various POSs.
// 2. Convert the scored_word's in _fills_ to basic_datum's.
// 3. Convert the vector<basic_datum> to a map, then a wordmap.
// 4. Get exact/changed fills from clue.
// 5. Once the scores as computed, resort things and clean up

filler::filler(const vector<square *> &squares, const vector<unsigned> &splits, 
               const string &c, const database &d, unsigned id,
               unsigned fitb_limit, unsigned diff, bool gui,
               const vector<rebus_answer> &rebus,
               const vector<theme_fill> &tfills, 
               const vector<string> &specials) :
  clue(c), difficulty(diff), dat(&d), bitmaps(squares.size()), word_id(id)
{
  vector<scored_word> fills; // fills by word
  pos_prob = 
    d.suggestions(squares,splits,clue,specials,fills,true,true,fitb_limit,
                  rebus,tfills);                                        // 1
  vector<pair<string,basic_datum>> v(fills.size());
  transform(fills.begin(),fills.end(),v.begin(),                        // 2
            [&](const scored_word &w) {
              return make_pair(w.word,basic_datum(w)); 
            });
  map<string,basic_datum> m(v.begin(),v.end());                         // 3
  fills_by_word = m;
  exacts = d.find_fills(clue);                                          // 4
  for (auto &w : d.find_fills_with_construction(clue)) exacts.push_back(w);
  scores_are_computed();                                                // 5
}

// Mark exact/changed matches.  Note that a word that appears on both
// lists is marked as exact only.  Also, if a word doesn't appear
// anywhere, we wind up changing the default basic_word but that
// doesn't appear to matter.

void filler::fold_exact()
{
  for (auto &w : set<string>(exacts.begin(),exacts.end())) {
    basic_datum &bd = fills_by_word[w];
    bd.exactp = true;
    bd.score -= dat->detail(CLUE_BONUS);
  }
}

// Once the words are computed, we can finalize things. 

// 1. fill_by_score is computed by computing a
// vector<scored_word_sorter>, where a scored_word_sorter includes a
// score (which is what's used for sorting), a word and an index.  We
// set that up, sort it, and convert to a vector<unsigned>, so we
// basically have an vector of indices into fills_by_word that orders
// them by score.

// 2. Construct the bitmaps for each position.  This means that for
// each square s in the word, bitmaps[s] will incude NUMCHARS bits,
// where bit b is on if there is a word that has character b in square
// s.

// One additional thing is that we make sure that we set the number of
// exact clues for every word, even though that word may never have
// been an answer to this exact clue.

void filler::scores_are_computed()
{
  fold_exact();
  unsigned sz = fills_by_word.size(length());
  // now get indices sorted by score
  vector<scored_word_sorter> fbs(sz);
  for (size_t i = 0 ; i < sz ; ++i) {
    const basic_datum &bd = fills_by_word.get_value(length(),i);
    fbs[i] = scored_word_sorter(fills_by_word.get_key(length(),i),bd.score);
    fbs[i].index = i;
  }
  sort(fbs.begin(),fbs.end());
  fills_by_score.resize(fbs.size());
  for (size_t i = 0 ; i < fbs.size() ; ++i) fills_by_score[i] = fbs[i].index;
  fills_by_score.push_back((unsigned) -1); // end marker

  // now construct bitmaps, also sorted by score
  for (size_t j = 0 ; j < length() ; ++j) {
    bitmaps[j].clear();
    bitmaps[j].resize(NUMCHARS);
    for (size_t i = 0 ; i < sz ; ++i)
      bitmaps[j][fbs[i].word[j] - 'a'].set_bit(i,true);
  }
}

// Reset the fillers when the scoring parameters change.  In other
// words, recompute all the scores and then call scores_are_computed
// to reindex.

void filler::reset(const params &p)
{
  for (size_t i = 0 ; i < size() ; ++i) 
    fills_by_word.get_value(length(),i).score = 
      p.compute_score(fills_by_word.get_value(length(),i));
  scores_are_computed();
}

/*

Fields of a fill_indexer, repeated for convenience:

fills           fills of root filler that match the pattern
root            pointer to root filler
parent          pointer to parent of this node (nullptr for root)
pattern         string to match
history         branches that produced this indexer from the root
kids            children of this indexer (NUMCHARS * len of them, each a pointer)
parent_index    how far we've gotten in collecting the parent's answers at
                this level.  The fill at this index has NOT been considered.
bitmaps         bitmaps for children of *this* indexer (NUMCHARS * len of them)

Basically, a fill_indexer is a big tree indicating whether or not a
particular letter pattern appears in the fills associated with the
root filler.  (The filler is set up when the clue is analyzed, before
any of the search happens and before you need this partial lookup
ability.)  A fill indexer for a word of length n has n * NUMCHARS
children, indicating whether or not the pattern can be extended by
including the given character in the given slot.  All of this is
constructed lazily, so that we initially just have the root node and
its kids.

Each fill_indexer has a _fills_ which is the indices of the fills at
the root filler that match the given pattern.  This list is also
computed lazily and we don't extend the fill_indexer to its kids
unless there are enough fills to warrant it.

The first bit of code rebuilds the fill_indexer from the root.  We use
it when we want to completely reset the fill_indexer because the
scores have changed, so the indices have changed as well. */

void fill_indexer::insert_fill(unsigned root_pos)
{
  const string &word = root->nth_best_fill(root_pos); // this is a lookup
  for (size_t i = 0 ; i < word.size() ; ++i)
    set_bit(i,word[i],fills.size(),true);
  fills.push_back(root_pos);
}

// Add all the root's words to this indexer (used at the root only).

void fill_indexer::build_from_root()
{
  for (unsigned idx = 0 ; idx < root->size() ; ++idx) insert_fill(idx);
}

// Here we want to get the next word in a fill indexer.  First, if
// there is no parent, we will have initialized it completely so we're
// done.  Alternatively, we might *know* that we're done.

// In the interesting case, we have to use the information from the
// parent.  Of course, we have a new _target_ constraint that the slot
// and letter that spawned this indexer has to match (and perhaps
// more, if there is a downstream target that we're *actually* trying
// to match).  We construct both parent_bits, which is a bitmap from
// the parent saying which elements go into the current indexer, and
// target_bits, which tell which parent elements will actually match
// the entire target.  We're careful to do a copy in constructing
// target_bits only if truly necessary.

// We begin by checking for anything that the parent has that we
// don't.  As we do so, of course, we need to update the parent_index
// (remember, it should point to the first element that *hasn't* been
// considered).  If that produces a match, we're done.  Otherwise, if
// there is nothing new in the parent, we extend the parent and try
// again.  If that doesn't work, we push -1 as an end marker.  

// There is a trap here.  Extending the parent may return false if
// nothing in that extension can match the target.  If that happens,
// we will *not* have pushed all the parent's answers down to the
// current indexer (why do so?), so we *can't* update the
// parent_index.  In the code below, the while test fails so the body
// of the do loop is not executed and parent_index is not updated.

bool fill_indexer::extend(const vector<branch> &target)
                                        // arg is intended history
{
  if (!parent || complete()) return false;
  do {
    if (parent_index < parent->size()) {
      const bitmap &parent_bits = parent->bits(history.back());
      const bitmap *target_bits_addr = &parent->bits(history.back());
      bitmap b;
      if (target.size() > history.size()) { // definitely new info
        b = *target_bits_addr;              // make a copy
        for (size_t i = history.size() ; i < target.size() ; ++i)
          b &= parent->bits(target[i]);     // update as needed
        target_bits_addr = &b;
      }
      for (unsigned next = parent_bits.next_including(parent_index) ; 
           next != bitmap::npos ; next = parent_bits.next_excluding(next)) {
        insert_fill(parent->fills[next]);
        if (next < target_bits_addr->size() && (*target_bits_addr)[next]) {
                                // found a matching word; exit loop
          parent_index = next + 1;
          return true;
        }
      }
    }
    parent_index = parent->size();
  }
  while (parent->extend(target));
  if (parent_index == parent->size() && parent->complete()) fills.push_back(-1);
  return false;
}

// Get the nth entry in a fill indexer.  If you've already got enough,
// just return the relevant one.  Otherwise, keep trying to extend it.
// Eventually, the extend fails and we just return failure.

const scored_word fill_indexer::operator[](unsigned idx)
{
  do if (idx < size()) return (*root)[fills[idx]];
  while (extend(history));
  return hopeless;
}

// Spawn a new child from a given fill indexer.  If we know where to
// put it, it's easy.

fill_indexer *fill_indexer::spawn(const branch &b)
{
  fill_indexer *ans = new fill_indexer(root);
  ans->parent = this;
  ans->pattern = pattern;
  ans->pattern[b.slot] = b.letter;
  ans->history = history;
  ans->history.push_back(b);
  kids[b] = ans;
  return ans;
}

// The money function.  Given a pattern, find the nth match.

// A couple of cases are easy.  If this is the current pattern, we're
// done.  If there is a suitable child, we descend to that case.  If
// not, it gets interesting as we gradually extend the current indexer
// and consider spawning a child.

// If we ever have more than SPLIT entries, we find some point where
// the patterns differ, spawn a child, and return that.  Otherwise,
// get the nth bit from the current indexer; if there is one, return
// it.  If it's complete, give up.  Otherwise, extend and continue.

const scored_word fill_indexer::find_match(const string &pat, unsigned idx)
{
  static unsigned SPLIT = 128;
  if (pat == pattern) return (*this)[idx];
  for (size_t i = 0 ; i < pat.size() ; ++i)
    if (pat[i] != WILD && kids[branch(i,pat[i])]) 
      return kids[branch(i,pat[i])]->find_match(pat,idx);
  vector<branch> extension;
  do {
    if (extension.empty()) {
      extension = history;
      for (unsigned i = 0 ; i < pat.size() ; ++i)
        if (pat[i] != pattern[i]) extension.push_back(branch(i,pat[i]));
    }
    if (size() > SPLIT) return spawn(extension.back())->find_match(pat,idx);
    bitmap b = bits(extension[history.size()]);
    for (size_t i = 1 + history.size() ; i < extension.size() ; ++i)
      b &= bits(extension[i]);
    unsigned which = b.nth(idx);
    if (which != bitmap::npos) return (*root)[fills[which]];
    if (complete()) return hopeless;
  } while (extend(extension));
  return hopeless;
}
