#include "xw.h"

// Initialization and reset:

// Initialization just means setting up all the fillers and fill
// indexers, using multiple threads if desired.  We use the dispatcher
// structure in score.h to do that.

// First, we need a list of words to process.  We order them by length
// (longest first) but any FITB clue gets a big bump because we want
// to do them early so that we can initialize the relevant data
// structures.

vector<unsigned> search_node::to_fill()
{
  multimap<unsigned,unsigned> sorted_slots; // len -> id
  for (auto &w : *this) {
    if (!w.to_initialize) continue;
    unsigned val = w.size();
    if (w.clue.find("___") != string::npos) val += BIG_SIZE; // do __ first
    sorted_slots.insert(make_pair(val,w.id));
  }
  vector<unsigned> ans;
  for (auto i = sorted_slots.rbegin() ; i != sorted_slots.rend() ; ++i)
    ans.push_back(i->second);
  return ans;
}

// Here is the call to the filler.  The dispatcher gives us the index
// into the ids to process next, and we process it.

void search_node::threaded_filler(dispatcher &d, const vector<unsigned> &ids)
{
  unsigned n;
  while ((n = d.next()) != unsigned(-1)) {
    word &w = (*this)[ids[n]];
    if (w.to_initialize) initialize_slot(w);
  }
}

// Special case of initialization where only one word is needed.
// Initialize it, print out the information requested, and quit.

void search_node::initialize_for_check()
{ 
  sets->verbosity = 4;
  word *wc;
  for (auto &w : *this)
    if (w.numbers[0] == sets->check_fill_id && 
        w.across() == sets->check_fill_across) {
      wc = &w;
      break;
    }
  initialize_slot(*wc);
  for (size_t i = 0 ; i < sets->to_check.size() ; ++i)
    cout << wc->id << ": " << get_fill(*wc,sets->to_check[i]) << endl;
  terminate(0);
}

// Initialize the slots.  Threaded case uses the dispatch mechanism;
// non-threaded cases is simpler.

void search_node::initialize_solver(bool initialize_all)
{
  if (initialize_all) reset_fillers();
  if (check_now()) initialize_for_check();
  if (sets->threads > 1) {                   // multithreaded
    vector<unsigned> tasks = to_fill();
    vector<thread> threads;
    dispatcher disp(tasks.size());
    for (unsigned i = 0 ; i < min(unsigned(tasks.size()),sets->threads) ; ++i)
      threads.push_back(thread([&]() { threaded_filler(disp,tasks); }));
    for (auto &th : threads) th.join();
  }
  else for (auto &w : *this) if (w.to_initialize) initialize_slot(w);
                                // single threaded
}

// Reset the solver for a word, recomputing the indexer.

void word::reset_indexer()
{
  indexer = make_shared<fill_indexer>(&wfiller);
  indexer->build_from_root();
}

// Initialize all the filler arrays.

void search_node::reinitialize_solver()
{
  for (auto &w : *this) {
    w.wfiller.reset(param());
    w.reset_indexer();
  }
}

// Clear caches for a word (computed scores and multiwords).

void word::clear_cache()
{
  computed_scores.clear();
  xtra.clear();
  theme_computed.clear();
}

// Reset a word.  Make it all wild chars, clear the pitched array and caches.

void word::reset()
{
  fill.assign(fill.size(),WILD);
  for (auto ch : *this) ch->fill = WILD;
  pitched.clear();
  clear_cache();
}

// Reset all the fill.  Reset each word, and clear the stack.

void search_node::reset_fill()
{
  for (auto &w : *this) w.reset();
  stack.clear();
}

// Reset the solution only.  BIG_SCORE is awful and set all the timers to now.

void search_node::reset_solution()
{
  best_score = BIG_SCORE;
  best_solution_time = current_time();
  for (auto &w : *this) w.best_solution.clear();
}

// A choice is an error if it doesn't match the answer.  (This
// function is only called if you're cheating or tuning, so it's
// reasonable to assume that the answer is there and accurate.)

bool search_node::error(const choice &c) const
{
  return (*this)[c.word_slot].answer != c.fill;
}

// At any given point in time, words will often have in best_fills a
// sorted list of fills and values (each a heuristic_value).  This
// pulls out the top choice that hasn't been pitched, or the empty
// string if there isn't one.

const string &word::best_unpitched() const
{
  static const string emty;
  for (auto &b : best_fills) if (is_allowed(b.fill)) return b.fill;
  return emty;
}

// In order to compute the value of filling a particular slot, we need
// to compute the value of various fills for that slot.  (The value of
// filling the slot is then basically the difference between the
// values of the top two choices.)  The value is basically the sum of
// the given fill for this word and the best remaining fills for all
// the crossing words.  To make the calculation go quickly, we also
// pass a fill_cache, a table of the score for the crossing word at
// position p if that position if filled with letter l.  There is also
// a cutoff argument; if the value of the base word exceeds that, it's
// essentially a failure.  (Note it has to be the value of the base
// word, since this failure terminates the subsequent analysis as
// well, since the base values that follow will be worse.)

// One final caveat.  In general, if a word gets worse, we don't want
// to blame the current word entirely.  But if we're postprocessing,
// we do.  The bool sc indicates whether or not to scale the blame for
// crossing words.

heuristic_value search_node::find_fill(const word &wd, int fill, float cutoff,
                                       bool sc, fill_cache &cache) const
{
  scored_word cand = get_fill(wd,wd.fill,fill);
  if (cand.score == BIG_CHARGE || cand.score >= cutoff) return heuristic_value();
  for (size_t j = 0 ; j < wd.size() ; ++j)
    if (wd.fill[j] == WILD && wd.has_crosser(j)) {
      unsigned let = cand.word[j]; // compute impact of this letter if needed
      unsigned loffset = let - 'a';
      if (!cache.known_scores[j][loffset]) {
        cache.letter_scores(j,loffset) = 0;
        for (size_t k = 0 ; k < wd[j].word_ids.size() ; ++k) { // words using
          unsigned xid = wd[j].word_ids[k];                    // this square but
          if (xid == wd.id) continue;                          // not this word
          const word &wx = (*this)[xid];
          string s = wx.fill;                                  // current fill
          s[wd[j].positions[k]] = let;                         // include letter
          float scr = score(wx,&s);
          cache.letter_scores(j,loffset) += sc? scale(wx,scr) : scr;
        }
        cache.known_scores[j][loffset] = true;
      }
      cand.score += cache.letter_scores(j,loffset); // add into running score
    }
  return cand;
}

// Scaling factor for a given word.  Used to contrast fill values for
// different words, basically.  It's an inverse factor, computed from
// the number of unfilled crosswing words and also adjusted for three
// letter words, which are generally harder to fill.

float search_node::fill_scale(const word &wd) const
{
  float sc = 1 + detail(SCALE_BY_CROSSERS) * wd.active_crossers();
  if (wd.size() == 3) sc *= detail(THREE_LETTERS);
  return sc;
}

// Get the sorted list of fills.  The only tricky part is figuring out
// when you can stop looking.  Suppose that as we analyze, the best
// two fills have scores b1 and b2.  As we continue to look, every
// score will be at least the current score for this word plus the
// current score for the crossing words that will be impacted.  There
// are two pruning possibilities.

// 1. Once we get to a point that every score is worse than b2, we can
// obviously stop.  So if score(w) >= b2, we're done, where score(w)
// is the score of this word.

// 2. More interestingly, suppose that we need the fill value to be at
// least D, presumably because some *other* word had a fill value of
// at least D.  Now if we've got a b1 and b2 that are *closer* than D,
// clearly the only way for that to happen is for us to find a new
// *best* word that is at least D better than b1.  So we need score(w)
// < b1 - D.

// In sum, we can stop when either score(w) >= b2 or, if b2 - b1 < D,
// if score(w) >= b1 - D.  We also examine a maximum of BRANCH_CHECK
// words in any event.

// So as we go, we keep track of b1 and b2 ("best" and "second best"
// below) and pass the appropriate cutoff to find_fill.  Note that
// only unpitched words affect this analysis.  Sort all the values at
// the end.  (As things are, we could make this microscopically faster
// by just saving the best two fills for w.  But maybe we'll want
// more, and the sort really isn't an issue.)

void search_node::find_fills(const word &wd, float delta_reqd, bool sc,
                             fill_cache *fc) const
{
  fill_cache scache;
  if (!fc) scache.reset(wd.size());
  wd.best_fills.clear();
  float best = BIG_CHARGE, second_best = BIG_CHARGE;
  delta_reqd *= fill_scale(wd);  // because the score is adjusted in fill_value
  for (size_t i = 0 ; i < BRANCH_CHECK ; ++i) {
    float cutoff = second_best;
    if (second_best - best <= delta_reqd) cutoff = best - delta_reqd;
    heuristic_value cand = find_fill(wd,i,cutoff,sc,fc? *fc : scache);
    if (cand.value == -1) break;
    wd.best_fills.push_back(cand);
    if (wd.is_allowed(cand.fill)) {
      if (cand.value < best) { second_best = best; best = cand.value; }
      else if (cand.value < second_best) second_best = cand.value;
    }
  }
  sort(wd.best_fills.begin(),wd.best_fills.end());
}

// Heuristic value of filling a given word slot, once the fills are
// computed and sorted.  Just walk through, finding the best and
// second best unpitched words.  Now if there is no best word (i.e.,
// no candidate at all), return 0.  Otherwise, get the second best
// word score, subtract that from the best, scale appropriately, and
// return the result.

float search_node::fill_value(const word &wd) const
{
  int best_idx = -1, second_idx = -1;
  for (size_t i = 0 ; i < wd.best_fills.size() ; ++i)
    if (wd.is_allowed(wd.best_fills[i].fill)) {
      if (best_idx < 0) best_idx = i;
      else {
        second_idx = i;
        break;
      }
    }
  if (best_idx < 0) return 0;
  float sec = (second_idx < 0)? BIG_CHARGE : wd.best_fills[second_idx].value;
  return (min(ndc(wd),sec) - wd.best_fills[best_idx].value) / fill_scale(wd);
}

// Heuristic value of filling a word slot where the fills haven't been
// computed.  Just assemble the pieces.  We *do* want to scale the
// blame assigned to crossing words, which is the final argument to
// find_fills.

float search_node::fill_value(const word &wd, float delta_reqd) const
{
  find_fills(wd,delta_reqd,true);
  return fill_value(wd);
}

// Word insertion and removal.  Removal is easy -- just insert the
// "empty" string.

void puzzle::erase(word &w)
{
  insert(w,string(w.size(),WILD));
}

// To insert a word, for each crosser (word using a square that isn't
// *this* word), set the letter as indicated.  Also set this word's
// fill as indicated.

void puzzle::insert(word &w, const string &s)
{
  for (size_t i = 0 ; i < w.size() ; ++i) 
    for (size_t j = 0 ; j < w[i].word_ids.size() ; ++j) {
      unsigned xid = w[i].word_ids[j];
      if (xid != w.id) (*this)[xid].fill[w[i].positions[j]] = s[i];
    }
  w.fill = s;
  for (size_t i = 0 ; i < w.size() ; ++i) w[i].fill = s[i];
}

// Insertion in a search_node just calls the above, but also tells the
// user about it if verbosity is high enough.

void search_node::insert(word &w, const string &s)
{
  if (verb() > 1) cout << s << ": " << w << endl;
  puzzle::insert(w,s);
}

// Timeout calculation.  First, is a progress timer greater than
// another in any slot where the other has a nonnegative value?

bool progress::somewhere_greater(const progress &other) const
{
  if (other.nodes >= 0 && nodes > other.nodes) return true;
  if (other.when >= 0 && when > other.when) return true;
  return false;
}

// Should we stop because a minute just ended?  Only wrinkle is that
// you want to allow a little bit extra for larger puzzles, since
// examining the clues (or whatever) will take a bit longer there.

float finalization_time(unsigned sz)
{
  return MINUTE_THRESHOLD + sz / SIZE_SCALE;
}

bool progress::end_of_minute(unsigned sz) const
{
  return int(when) % 60 > 60 - int(finalization_time(sz));
}

// Use the above functions as a termination check.  Stop if the
// current time is more than the absolute limit.  If it's not the end
// of a minute (and you care!), DON'T stop.  Otherwise, stop if the
// time since the last progress point is more than the relative limit
// in delta[idx].

bool settings::terminate(const progress &then, const progress &now,
                         unsigned sz, unsigned idx) const
{
  if (now.secs() > absolute) return true;
  if (wall && !boswords && !now.end_of_minute(sz)) return false;
  if ((now - then).somewhere_greater(delta[idx])) return true;
  return false;
}

// Show the results of analyzing the clues (nontheme analysis only).
// The theme file should contain all the intended theme entries.
// Analyze each word in the puzzle, and if the best fill isn't right,
// print out the difference for diagnostic purposes.  Theme words
// don't count.

float search_node::show_clue_analysis() const
{
  ifstream inf(d->theme_file);
  check_stream(inf,d->theme_file);
  vector<string> twords;
  string line;
  while (getline(inf,line)) {
    twords.push_back(tolower(line));
    if (!isalnum(twords.back().back())) twords.back().pop_back();
  }
  set<string> themewords(twords.begin(),twords.end());
  float error = 0;
  unsigned ect = 0;
  for (auto &w : *this) {
    string lans = tolower(w.answer);
    if (themewords.find(lans) != themewords.end()) continue;
    scored_word best = w.wfiller[0];
    string lbest = tolower(best.word);
    bool correct = (lbest == lans);
    if (correct) { best = w.wfiller[1]; lbest = tolower(best.word); }
    float ascore = get_fill(w,lans).score;
    float bscore = best.score;
    float err = ascore - bscore;
    if (ascore < BIG_CHARGE * 0.9) { // non-words can't be tuned
      error += err;
      ++ect;
    }
    if (sets->variable_tuners.empty()) 
      cout << err << " ans " << lans << ' ' << ascore
           << (correct? " second" : "") << " best " << lbest << ' ' << bscore
           << ' ' << source << ' ' << w.desc() << ' ' << w.clue << endl;
  }
  if (!sets->variable_tuners.empty()) cout << error / ect << ' ' << flush;
  return error / ect;
}

// One LDS iteration.  solve() does the work; we just need to reset
// things before we call it.  We also keep track of the "pass" we're
// on in terms of restricting the area we're searching.

void search_node::lds(bool reset, bool reset_sol, unsigned &pass)
{
  if (reset) {
    if (!sets->tune && verb() > SILENT) {
      if (rebus.empty()) cout << "LDS, no rebus" << endl;
      else cout << "LDS; rebus " << rebus << endl;
    }
    initialize_solver(reset_sol); // only initialize all fills at beginning
    if (verb() > SILENT)
      sets->tuneout() << "clues analyzed @ " << get_time() << endl;
    if (reset_sol) find_xrefs(true); // replace crossref clues with empty fill
  }

  if (sets->tune) tune();
  else solve(reset,reset_sol,pass);
} 

// Put the best solution back.

void search_node::restore_best_solution()
{
  for (auto &w : *this) {
    w.fill = get_solution(w);
    for (size_t j = 0 ; j < w.size() ; ++j) w[j].fill = w.fill[j];
  }
}

// Add flamingo-suggested words (for words that don't have a rebus
// char).  Do this starting with the longest words and working down.
// If at any point we make a change (indicated by the best solution
// time changing), we immediately start completely over.

// For each word, get the fill.  If it's a flamingo word already, no
// need to worry about it.  Otherwise, mark this word as locked
// (postprocessing shouldn't undo what we're about to do!) and get the
// flamingo suggestions.  For each that's the right length, process it
// using handle_flamingo_suggestion.

void search_node::add_flamingo_suggestions()
{
  multimap<unsigned,unsigned> words_by_length;
  for (auto &w : *this)
    if (w.size() >= FLAMINGO_LENGTH && !w.rebus_fill())
      words_by_length.insert(make_pair(w.size(),w.id));
  progress bst;
  do {
    bst = best_solution_time; // good way to tell if you've changed anything
    if (verb() > SILENT) cout << "flamingo loop @ " << bst << endl;
    for (auto i = words_by_length.rbegin() ; i != words_by_length.rend() ; ++i) {
      for (const string &alt : flamingo_match((*this)[i->second]))
        if (alt.size() == i->first) {
          handle_flamingo_suggestion((*this)[i->second],alt);
          if (bst != best_solution_time) {
            locked[i->second] = true;
            break; // and start again
          }
          else restore_best_solution();
        }      
    }
  }
  while (bst != best_solution_time);
}

// Flamingo suggestions for a given slot.  If it should be a themed
// word, it has to be long or it's just not worth it.  Then aggregate
// everything for every possible theme.  The nontheme case is easier.

vector<string> search_node::flamingo_match(const word &wrd) const
{
  const string &f = get_solution(wrd);
  vector<string> ans;
  if (should_be_themed[wrd.id]) {
    if (f.size() <= FLAMINGO_LENGTH) return ans;
    for (auto &th : puzzle_theme) th.flamingo(ans,f,*d);
    return uniquify(ans);
  }
  return d->flamingo_match(f,false,FLAMINGO_MATCH,true);
}

// We handle a single flamingo suggestion by building an appropriate
// task (remember that a task is basically a vector of choices).  In
// that task, we continue to pitch anything that was pitched in the
// current best solution, and continue to use as fill everything that
// doesn't cross the word being changed or share a crosser with that
// word.  (Just starting from scratch might not produce the same good
// answer without modifying the pitches; the search is just too
// unstable for that.)

bool skeleton::has_as_crosser(unsigned x) const
{
  for (auto sq : *this)
    if (find(sq->word_ids.begin(),sq->word_ids.end(),x) != sq->word_ids.end())
      return true;
  return false;
}

task search_node::flamingo_task(const word &slot, const string &fill) const
{
  task ans;
  for (auto &w : *this)
    for (auto &p : w.best_pitches) ans.state.push_back(choice(p,w.id,true));
  ans.state.push_back(choice(fill,slot.id));
  set<unsigned> xings;
  for (auto sq : slot) xings.insert(sq->word_ids.begin(),sq->word_ids.end());
  xings.erase(slot.id);
  for (auto &w : *this) {
    if (slot.has_as_crosser(w.id)) continue;
    bool found_as_crosser = false;
    for (auto x : xings) if (w.has_as_crosser(x)) {
        found_as_crosser = true;
        break;
      }
    if (!found_as_crosser) ans.state.push_back(choice(get_solution(w),w.id));
  }
  return ans;
}

void search_node::handle_flamingo_suggestion(const word &w, const string &sugg)
{
  unsigned match = 0;
  for (size_t i = 0 ; i < sugg.size() ; ++i) if (w.fill[i] == sugg[i]) ++match;
  if (match * 2 < sugg.size()) return;
                                // don't do it if you change half the chars!
  if (verb() > SILENT) 
    cout << "flamingo: inserting " << sugg << " for " << w << endl;
  expand(flamingo_task(w,sugg));
}

// Given a bunch of geometry words before, and a bunch you've found
// now, are there more now?  Just credit one for each word in _curr_
// that's not in _prior_, and debit one for the other way around.

int geo_delta(const vector<boggle_string> &prior,
              const vector<boggle_string> &curr)
{
  int ans = 0;
  for (auto &bs : curr)
    if (find_if(prior.begin(),prior.end(),
                [&](const boggle_string &other) {
                  return other.squares == bs.squares;
                }) == prior.end()) ++ans;
  for (auto &bs : prior)
    if (find_if(curr.begin(),curr.end(),
                [&](const boggle_string &other) {
                  return other.squares == bs.squares;
                }) == prior.end()) --ans;
  return ans;
}

// Complete search, including themes and rebus.  To begin, we fix the
// state of the random number generator; there is a small random
// element to how we do the LDS probes.  Then:

// 1. Do an LDS probe.
// 2. If "quick", that's enough.  Otherwise postprocess:
// 2a. Put the best solution back
// 2b. If you can find a new theme, reset everything because you
//     will loop and resolve.
// 2c. Otherwise, if you've leaked past the end of minute, try again.
// 3. At the end of the loop, if quick is false, try all the Flamingo stuff.

void search_node::themed_search()
{
  srandom(1);                   // ensure repeatability across puzzles
  if (!sets->theme_query.empty()) check_theme();
  bool reset = true;
  bool reset_sol = true;
  bool has_theme = false;
  bool check_geometry = false;
  bool changed_geometry = false;
  int showed_leakage_mins = -1;
  bool quick = (sets->tune || sets->quick);
  unsigned pass = 0;
  unsigned geo_passes = 0;
  static const unsigned GEO_RECURSION_LIMIT = 3;

  for ( ;; ) {
    lds(reset,reset_sol,pass);                                          // 1
    if (quick) break;                                                   // 2
    restore_best_solution();                                            // 2a

    if (notheme) {
      if (verb() > SILENT) cout << "puzzle known to be themeless" << endl;
      break;
    }
    if (check_geometry && ++geo_passes <= GEO_RECURSION_LIMIT) { // geo recursion
      check_geometry = false;
      vector<boggle_string> prior = geo.words;
      geometry g = find_geometry();
      if (try_geometry(g,false) && geo_delta(prior,g.words) > 0) {
        changed_geometry = true; // new words!  treat as a new theme
        try_geometry(g,true);
      }
    }

    if (changed_geometry || theme_check_with_reset(has_theme)) {        // 2b
      reset = true;
      reset_sol = false;
      best_score = BIG_SCORE;   // to reset GUI
      changed_geometry = false;
      if ((check_geometry = !geo.words.empty())) geo_passes = 1;
    }
    else if (!sets->boswords && !current_time().end_of_minute(size())) {
      if (current_time().mins() != showed_leakage_mins) {
        cout << "leaked past end of minute at time " << get_time() << endl;
        showed_leakage_mins = current_time().mins();
        if (sets->gui) cout << "**GUI theme spend an extra minute!\n";
      }
      reset = reset_sol = false;
      leak_time = current_time();
    }
    else break;
  }
  if (!quick) {     // note that you are committed to finishing now;
                    // the postprocess does not generate new work
    add_flamingo_suggestions();                                         // 3
    if ((*this)[0].best_solution.empty())
      cout << "puzzle not completed!" << endl;
    else restore_best_solution();
  }
  if (!sets->tune && verb() > SILENT)
    cout << "complete at " << get_time() << " and " << node_ct
         << " nodes" << endl << "TOTAL NODES: " << node_ct << endl;
}

// Check specific theme words to figure out what the theme is

void search_node::check_theme()
{
  vector<unsigned> themewords;
  for (auto &t : sets->theme_query) {
    for (auto &w : *this)
      if (w.numbers[0] == t.id && w.across() == t.across) {
        set_fill(w,t.fill);
        themewords.push_back(w.id);
        break;
      }
  }
  find_possible_themes(themewords);
  terminate(0);
}

// Check for a theme.  If no theme, check for rebus.  (This order
// appears to work better than the reverse.)  Note that we check first
// to see if there is a known theme, and set has_theme if we find a
// theme.

bool search_node::theme_check_with_reset(bool &has_theme)
{
  if (has_theme) return false;  // already checked!
  if (sets->tune || sets->quick) return false;
  if (leak_time.count() > best_solution_time.count()) return false;
                                // don't check again!
  if (!puzzle_theme.empty() || !rebus.empty()) return false;
  if (nonrebus_check_with_reset()) return has_theme = !theme_complete();
  return has_theme = rebus_check_with_reset();
}

// Does the fill already satisfy the theme?  Basically, every long
// word has to strongly match the theme, and every vaguely long word
// has to strongly match the theme if its opposite does.  We don't
// even give the last across word a pass (it might be a revealer)
// because we really want to be very conservative here.

bool search_node::theme_complete() const
{
  if (puzzle_theme.empty()) return false; // no theme!  (maybe geometry)
  for (unsigned i = 0 ; i < size() ; ++i) {
    const word &wd = (*this)[i];
    if (wd.size() >= MUST_BE_THEMED) {
      if (!is_theme_fill(wd,true)) return false;
      else continue;
    }
    if (wd.size() < MIN_THEME_LENGTH) continue;
    unsigned opp = opposite(wd);
    if (opp == -1 || opp >= wd.id) continue; // no opposite, or already checked
    int t1 = is_theme_fill(wd,true);
    int t2 = is_theme_fill((*this)[opp],true);
    if (t1 + t2 == 1) return false; // exactly one themed
  }
  cout << "theme already satisfied!\n";
  return true;
}

// Find the theme words.  As long as THEME_LENGTH, or as long as the
// longest horizontal word, or marked with an asterisk.

vector<unsigned> puzzle::find_theme_words(unsigned length) const
{
  size_t longest_horizontal = 0;
  vector<unsigned> ans;
  for (const word &w : *this)
    if (w.across() && w.splits.size() == 1)
      longest_horizontal = max(longest_horizontal,w.size());
  if (longest_horizontal < length) return ans;
  for (auto &w : *this) 
    if (w.size() >= length ||
        w.size() >= longest_horizontal ||
        w.clue[0] == '*')
      ans.push_back(w.id);
  return ans;
}

// Find theme words of an appropriate length.  There have to be at
// least 3 theme entries if possible, and never more than 10% of the
// puzzle.

vector<unsigned> puzzle::find_theme_words() const
{
  unsigned len = THEME_LENGTH;
  vector<unsigned> ans = find_theme_words(len);
  if (ans.size() < 3) {
    while (ans.size() < 3 && len > MIN_THEME_LENGTH)
      ans = find_theme_words(--len);
  }
  else while (ans.size() * MAX_THEME_FRACTION > size())
         ans = find_theme_words(++len);
  if (ans.size() < 3 || ans.size() * MAX_THEME_FRACTION > size())
    return vector<unsigned>();
  return ans;
}

// Tell the user and the GUI about the theme, assuming that there is one.

void search_node::show_theme() const
{
  if (verb() > SILENT) cout << "possible themes: " << puzzle_theme << endl;
  if (sets->gui)
    cout << "**GUI theme possible theme(s): " << puzzle_theme << endl;
}

// Mark a word -- and its opposite -- as themed.

void search_node::set_themed(unsigned i)
{
  should_be_themed[i] = true;
  unsigned opp = opposite((*this)[i]);
  if (opp != (unsigned) -1) should_be_themed[opp] = true;
}

// Is a puzzle pun themed?  You need more than two more than the best
// other theme, and the pun "score" adds 1 for each miss by 1 and 0.5
// for each miss by 2.  You also *subtract* one for a word in the
// database, or a homonym.  If it's a win (there are other conditions;
// see below), replace the existing theme with the pun.

void search_node::check_pun_theme(const vector<unsigned> &themewords)
{
  if (!puzzle_theme.empty() && puzzle_theme[0].sources >= 4) return;
                                // good non-pun theme
  float puns = 0;
  for (unsigned tw : themewords) {
    unsigned dist = d->pun_distance((*this)[tw],(*this)[tw].get_solution());
    switch (dist) {
    case 0: --puns; break;
    case 1: ++puns; break;
    case 2: puns += 2; break;
    }
  }
  if (puns <= 4) return;        // you seem to need at least five puns
                                // to have a chance at improvement
  if (!puzzle_theme.empty() && puns <= 2 + puzzle_theme[0].sources) return;
                                // not punny enough
  puzzle_theme.clear();
  puzzle_theme.push_back(sourced_theme(theme(true,PUN),puns));
}

// Find possible themes by calling the various theme primitives.
// Don't allow empty themes!

void search_node::find_possible_themes(const vector<unsigned> &themewords)
{
  possible_theme p, q;
  if (verb() > SILENT) cout << "theme search @ " << get_time() << endl;
  unsigned distance;
  for (auto &th : themewords) {
    word &wd = (*this)[th];
    p.push_back(d->get_themewords(wd.get_solution()));
    if (p.back().empty()) p.pop_back();
    q.push_back(d->get_soundwords(wd,distance));
    if (q.back().empty()) q.pop_back();
  }

  puzzle_theme.clear();
  p.analyze(puzzle_theme,*d,false,sets->explain_theme);
  q.analyze(puzzle_theme,*d,true,sets->explain_theme);
  check_pun_theme(themewords);
  if (sets->explain_theme) {
    cout << "possible themes analyzed: p is\n" << p << " and q is\n" << q
         << " ... produce theme\n";
    for (auto &st : puzzle_theme) cout << st << endl;
  }
}

// In order to "confirm" a geometry theme, there have to be at least
// two words that "work".  But when you count all such words, you have
// to subtract cases where two different words each want a specific
// square to have competing values.

int geo_conflicts(const vector<boggle_string> &words)
{
  map<location,set<char>> choices;
  for (auto &bs : words)
    for (unsigned i = 0 ; i < bs.size() ; ++i)
      choices[bs.squares[i]].insert(bs[i]);
  int ans = 0;
  for (auto &p : choices) ans += p.second.size() - 1;
  return ans;
}

// Look for a geometry theme (if the above theme search didn't work).
// Save the geometry found (for recursion, basically), and doit
// indicates whether or not you want to rework the puzzle to include
// any words found.

bool search_node::try_geometry(geometry &g, bool doit)
{
  if (g.words.empty()) return false;
  cout << "GEOMETRY " << g << ' ' << g.score << endl;
  for (auto &bs : g.words) cout << ' ' << bs.squares << ' ' << bs << endl;
  if (doit) puzzle_theme.clear();
  vector<unsigned> to_remove;
  vector<geo_replacement> replacements;
  unsigned j = 0;
  for(size_t i = 0 ; i < g.words.size() ; ++i)
    if (geo_word(g.words[i].squares,g.jump,to_remove,replacements))
      g.words[j++] = g.words[i];
  g.words.resize(j);
  int conflicts = geo_conflicts(g.words);
  cout << g.words.size() << " candidates remain; " << conflicts
       << " conflicts\n";
  if (g.words.size() < 2 + conflicts) {
    cout << "geometry theme abandoned!\n";
    return false;
  }
  if (!doit) return true;
  if (sets->gui) {
    cout << "**GUI theme GEOMETRY words";
    for (auto &bs : g.words) cout << ' ' << string(bs);
    cout << endl;
  }
  for (auto &r : replacements) geo_replace(r);
  remove_words(to_remove);
  for (auto &w : *this) {
    w.pitched.clear();
    w.best_solution.clear();
    w.best_pitches.clear();
  }
  geo = g;
  return true;
}

bool settings::report_themeless() const
{
  if (verbosity > SILENT) cout << "no theme found @ " << get_time(wall) << endl;
  if (gui) cout << "**GUI theme no theme I understand" << endl;
  return false;
}

void search_node::mark_themed_words(const vector<unsigned> &themed)
{
  size_t shortest_themed = LONG_WORD;
  for (unsigned th : themed) {
    if (!should_be_themed[th]) set_themed(th);
    shortest_themed = min(shortest_themed,(*this)[th].size());
  }
  for (auto &w : *this) {
    if (should_be_themed[w.id]) continue; // already done
    if (w.size() + 2 < shortest_themed && opposite(w) != unsigned(-1)) continue;
    // too short and not center entry
    if (is_theme_fill(w,true)) set_themed(w.id);
  }
}

// Find the words marked as themed.

vector<unsigned> search_node::get_identified_theme_words() const
{
  vector<unsigned> ans;
  for (unsigned i = 0 ; i < size() ; ++i) 
    if (should_be_themed[i]) ans.push_back(i);
  return ans;
}

// Get suggested theme fill for the themed words.

possible_theme search_node::find_theme_suggestions(vector<unsigned> &themed)
{
  possible_theme th(themed.size());
  unsigned dummy;
  for (size_t i = 0 ; i < themed.size() ; ++i)
    if (puzzle_theme[0].get_sound())
      th[i] = d->get_soundwords((*this)[themed[i]],dummy);
    else th[i] = d->get_themewords(get_solution((*this)[themed[i]]));
  return th.analyze_theme(puzzle_theme,*d);
}

void search_node::find_extra_themed_words(const vector<unsigned> &themed,
                                          const possible_theme &sugg)
{
  for (unsigned i = 0 ; i < sugg.size() ; ++i) 
    for (auto &t : sugg[i])
      if (t.original == t.fill) {
        set_themed(themed[i]);
        break;                  // next i
      }
}

// Check for a nonrebus theme.  First compute the apparent theme in
// the obvious way.  Now if the theme is weak (or doesn't exist at
// all), and there appears to be a geometry theme, use that instead.

// Now if there is *still* no theme, you're done.  Otherwise,
// incorporate the theme after recomputing the theme wordss;
// basically, a word isn't thought of as theme unless it's at least as
// long as a word that is known to be themed.  Return true if you've
// changed the fill.

// In the normal case, mark the themed words.  Get the current themed
// words and then get all the reasonable suggestions for each into
// sugg[i].  If folding those suggestions into the puzzle does
// nothing, you're done.  Otherwise, also set as themed any word that
// seems to *want* to have theme fill.

bool search_node::nonrebus_check_with_reset()
{
  vector<unsigned> themed = find_theme_words();
  if (!themed.empty()) find_possible_themes(themed);
  if (puzzle_theme.empty() || puzzle_theme[0].sources == 1) 
    if (sets->consider_geometry && sets->acpt_geometry) {
      geometry g = find_geometry();
      if (try_geometry(g,true)) return true;
    }
  if (puzzle_theme.empty()) return sets->report_themeless();

  size_t smallest_themed = MUST_BE_THEMED;
  for (unsigned id : themed)
    if (is_theme_fill((*this)[id],true))
      smallest_themed = min(smallest_themed,(*this)[id].size());
  themed = find_theme_words(smallest_themed);

  mark_themed_words(themed);
  themed = get_identified_theme_words();
  possible_theme sugg = find_theme_suggestions(themed);

  if (!incorporate_theme_suggestions(themed,sugg)) return false;

  show_theme();
  find_extra_themed_words(themed,sugg);
  return true;
}

// Incorporate theme suggestions from themed (a vector of the themed
// words in the puzzle) and sugg (suggested fills for each theme
// word).

// For each themed word, get the associated element of possible_theme.
// Now for each suggested replacement, see how far the suggested
// replacement is from the best known answer.  If it's too far, give
// up.  If it's on the list of theme suggestions for this word, it's a
// new best candidate that you've already analyzed.  Otherwise *it* is
// the new best mismatch and we want to add it to the list of theme
// suggestions for this word.  We've also changed something, and this
// word needs to be reinitialized.

bool search_node::incorporate_theme_suggestions(const vector<unsigned> &themed,
                                                const possible_theme &sugg)
{
  if (puzzle_theme[0].get_type() == PUN) return true;
  // can't really search for these so don't die trying
  bool changed = false;
  for (size_t i = 0 ; i < sugg.size() ; ++i) {
    unsigned best_mismatch = 100;
    word &w = (*this)[themed[i]];
    for (const theme_fill &tf : sugg[i]) {
      unsigned m = mismatch(get_solution(w),tf.fill);
      if (m > best_mismatch) break;
      if (find(w.theme_sugg.begin(),w.theme_sugg.end(),tf) !=
          w.theme_sugg.end()) 
        best_mismatch = m;
      else {
        cout << "suggest " << tf.fill << " from " << tf.source << " for "
             << w << endl;
        best_mismatch = m;
        w.theme_sugg.push_back(tf);
        w.to_initialize = true;
        changed = true;
      }
    }
  }
  return changed;
}

// Here is the main solving function.  If the puzzle is empty, do
// nothing.  Otherwise, render it out, and use themed_search to solve
// it.  If we're tuning or testing the DB, we're done.  Otherwise,
// restore the best solution, and call _check_, which compares to the
// answer and does whatever wrapping up is to be done.

void puzzle::solve(const database &db, settings &s)
{
  if (empty()) return;
  db.clear_flamingo_cache();
  db.clear_pronunciation_cache();
  db.clear_berkeley_cache();
  if (s.gui) request_render();
  collapse_xrefs();          // turn cross-ref clues into single words
  db.get_berkeley_results(*this);
  search_node node(*this,&db,s);

  node.themed_search();
  if (s.tune) return;
  restore_best(node);
  node.check(true);
  if (s.gui) node.request_display(true,true);
  rebus = node.rebus;
}

// Restore the best solution from a search_node into the current puzzle.

void puzzle::restore_best(const search_node &node)
{
  vector<word>::operator=(node);
  squares = node.squares;
  for (auto &w : *this) insert(w,get_solution(w));
}

// Once a puzzle is solved, we want to report whether the mistakes are
// search or heuristic.  To do that, we first have to split the
// mistakes into connected components.

vector<set<unsigned>> search_node::find_components(const set<unsigned> &words) 
  const
{
  vector<set<unsigned>> components;
  set<unsigned> w = words;
  while (!w.empty()) {
    vector<unsigned> cpt(1,*w.begin()); // new component
    w.erase(*w.begin());
    for (size_t i = 0 ; i < cpt.size() ; ++i) {
      const word &wd = (*this)[cpt[i]];
      for (auto sq : wd)
        for (unsigned xid : sq->word_ids) {
          if (xid == wd.id) continue;
          if (w.find(xid) == w.end()) continue;
          // single word may appear multiple times if lots of mistakes
          cpt.push_back(xid);
          w.erase(xid);
        }
    }
    components.push_back(set<unsigned>(cpt.begin(),cpt.end()));
  }
  return components;
}

// Compute the type of error (or correctness!) for each word.  First
// use compute_error (below) to figure out if the words are right or
// not, and then split errors into components.  Then evaluate each
// component to see if the problem is search or the heuristics.

// args to compute_error: focus word, display it?, extra display
// string, score of wrong fill, score of right fill, set of error
// words, set of error letters

vector<error_type> search_node::find_errors() const
{
  vector<error_type> ans(size(),CORRECT);
  float wrong, right;
  set<unsigned> w;              // wrong words
  set<square> l;                // wrong letters
  for (auto &wd : *this) compute_error(wd,false,"",wrong,right,w,l);
  vector<set<unsigned>> components = find_components(w);
  set<unsigned> junk;
  for (auto &cpt : components) {
    wrong = right = 0;
    for (unsigned j : cpt) 
      compute_error((*this)[j],false,"",wrong,right,junk,l);
    error_type e = (wrong < right)? HEURISTIC : SEARCH;
    for (unsigned j : cpt) ans[j] = e;
  }
  return ans;
}

// Display the mistakes in _words_.  Split into components, figure out
// which type of error each is and let compute_error do the work.

void search_node::show_mistakes(set<unsigned> &words) const
{
  set<unsigned> w = words;
  vector<set<unsigned>> components = find_components(w);
  vector<error_type> errors = find_errors();
  static const string error_strings[3] = { "", "search", "heuristic" };
  set<unsigned> junk1;
  set<square> junk2;
  for (auto &cpt : components) {
    float wrong = 0, right = 0;
    for (unsigned j : cpt) 
      compute_error((*this)[j],false,"",wrong,right,junk1,junk2);
    cout << "** " << source << ' ' 
         << error_strings[errors[*cpt.begin()]]
         << " error " << (fabs(wrong - right) / cpt.size())
         << " (" << wrong << " vs " << right
         << " on " << cpt.size() << " words)" << endl;
    for (unsigned j : cpt)
      compute_error((*this)[j],true,"... ",wrong,right,junk1,junk2);
  }
}

// Find mistakes.  A little tricky because a rebus character isn't
// wrong if the first character matches what's in the grid.

// For a start, here we take a fill character in the grid and return a
// string of things that Across Lite might think it is.  If this is a
// non-split rebus, we just find the square and return the first char
// of the known rebus.  If it's a split rebus (the x component of its
// square is -1), we return both the first letter of the rebus and of
// the alternate.  (We return a vector<char> basically, but think of
// it as a string.)

string search_node::adjust_to_check_rebus(char fill, const square &sq) const
{
  string ch(1,fill);            // first letter of possible rebus fill
  if (fill >= 'a' + NUMLETS && 
      (fill - ('a' + NUMLETS)) < (int) rebus.size()) {
    if (rebus[0].sq.x == (unsigned) -1) {
      const rebus_answer &ra = rebus[fill - ('a' + NUMLETS)];
      ch[0] = ra[0];
      if (ra.alternate.size()) ch.push_back(ra.alternate[0]);
    }
    else for (unsigned i = 1 ; i < rebus.size() ; ++i)
           if (sq.x == rebus[i].sq.x && sq.y == rebus[i].sq.y) {
             ch[0] = rebus[i][0];
             break;
           }
  }
  return ch;
}

// Find mistakes, bearing in mind the above functionality.  Update all
// sorts of things for scoring -- set of incorrect letters, number of
// original words that are wrong.  _words_ is the set of wrong words.

vector<unsigned> search_node::compute_error(const word &w, bool display, 
                                            const string &str, float &wrong,
                                            float &right, set<unsigned> &words,
                                            set<square> &letters) const
{
  string lfill = tolower(w.fill);                   // will be lower case
  string lans = tolower(w.answer), uans = w.answer; // lower and upper case
  for (auto &ch : uans) ch = toupper(ch);
  bool error = false;
  for (size_t j = 0 ; j < w.size() ; ++j) { // find wrong words and squares
    string ch = adjust_to_check_rebus(lfill[j],w[j]);
    if (ch.find(lans[j]) != string::npos) continue;
    error = true;
    letters.insert(w[j]);
    words.insert(w.id);
  }
  vector<unsigned> ans;
  if (!error) return ans;       // correct!
  float fillscore = score(w,&lfill);
  float ansscore = score(w,&lans);
  if (display) {
    cout << str << "w" << w.id << ' ' << w.desc()
         << "  [" << w.clue << "] is " << w.compute_fill(true,'?')
         << " (" << fillscore
         << ") but should be " << uans << " (" << ansscore << ")"
         << ((w.is_allowed(lans))? "" : " -- pitched!") << endl;
  }
  wrong += fillscore;           // fold wrong/right scores into sums
  right += ansscore;
  for (size_t i = 0 ; i < simple_words.size() ; ++i) 
    if (any_of(letters.begin(),letters.end(),
               [&](const square &l) { return simple_words[i].includes(l); }))
      ans.push_back(i);
  return ans;                   // return all original words that are wrong
}

// Final bookkeeping, etc.  Compute number of incorrect letters,
// number of incorrect words, and score of incorrect words and correct
// words for those slots.  Print out the summary, and show the ACPT
// score as well.  Return true if puzzle solved correctly.

bool search_node::check(bool details) const
{
  float elapsed = get_time();
  set<unsigned> words;
  set<square> letters;
  float wrong = 0, right = 0;
  set<unsigned> wrong_words;
  for (auto &w : *this) {
    vector<unsigned> errs = 
      compute_error(w,details && !sets->mistakes,"",wrong,right,words,letters);
    wrong_words.insert(errs.begin(),errs.end());
  }

  if (!words.empty()) {
    if (details && sets->mistakes) show_mistakes(words);
    cout << letters.size() << " letter(s) and " << wrong_words.size() 
         << " word(s) wrong; wrong score " << wrong 
         << " and right score " << right << endl;
  }
  else cout << "correct!" << endl;
  cout << endl;

  assert(simple_words.size() >= wrong_words.size());
  int remain_minutes = (int) (60 * limit() - elapsed) / 60;
  int remain_seconds = (int) (60 * limit() - elapsed) % 60;
  string tourney;
  int bonus;
  int correct_bonus = sets->boswords? 200 : 150;
  unsigned acpt = 10 * (simple_words.size() - wrong_words.size());
  // same, Boswords and ACPT
  if (sets->boswords) {
    tourney = "Boswords";
    bonus = remain_minutes * 60 + remain_seconds;
  }
  else {
    tourney = "ACPT";
    bonus = remain_minutes * 25 - 25 * letters.size();
  }
  if (words.empty()) acpt += correct_bonus;
  if (bonus > 0) acpt += bonus;
  cout << tourney << (details? " final" : "") << " score: " << acpt << " (" 
       << remain_minutes << ':' << setfill('0') << setw(2) << remain_seconds
       << " minutes, " << wrong_words.size() << " words, " 
       << letters.size() << " letters)" << endl;
  if (sets->gui) cout << "**GUI acpt " << acpt << ' ' << remain_minutes
                      << ':' << setfill('0') << setw(2) << remain_seconds << ' ' 
                      << wrong_words.size() << ' ' << letters.size() << endl;

  return words.empty();
}
