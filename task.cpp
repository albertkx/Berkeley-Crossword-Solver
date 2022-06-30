#include "xw.h"

// The basic search is structured using LDS.  At each point, we either
// put the best word into the best slot or pitch it.  We keep all the
// tasks in memory, since we can't expand that many; each task is just
// the current fills and pitches, along with a number of discrepancies
// used (the tcost).

// Build the puzzle from the task: reset and then reconstruct in the
// obvious way.

void puzzle::rebuild_from_task(const task &t)
{
  for (auto &w : *this) {
    erase(w);
    w.clear_pitched();
  }
  for (const choice &c : t.state) {
    word &w = (*this)[c.word_slot];
    if (c.pitch) w.add_pitch(c.fill);
    else insert(w,c.fill);
  }
}

// Should we keep going?  No, if the stack is empty.  Yes, if we're
// cheating.  No, if we're tuning.  Otherwise, do a timeout check.

bool search_node::search_continues(unsigned pass) const
{
  if (stack.empty()) return false;
  if (sets->cheat) return true;
  if (sets->tune) return false;
  if (sets->quick) return false;
  progress p = current_time();
  progress old_time = best_solution_time;
  if (leak_time.count() > best_solution_time.count()) old_time = leak_time;
  if (!sets->terminate(old_time,p,size(),pass)) return true;
  static progress reported_time;
  if (best_solution_time != reported_time) {
    cout << "terminates: timer " << p << " and last progress " 
         << best_solution_time << endl;
    reported_time = best_solution_time;
  }
  return false;
}

// A task is better if it has lower cost (i.e., uses fewer
// discrepancies or has less depth) or, if the costs are equal, if
// it's nearer the root of the search tree.

bool task::operator<(const task &other) const
{
  if (tcost != other.tcost) return tcost < other.tcost;
  return state.size() < other.state.size();
}

// Restrict the search to words worse than lim, where to compute the
// "badness" of a word, we first define the badness of a square to be
// the maximum score of any word that uses that square, and then the
// badness of a word to be the maximum of any square in that word.

// Having scored all the words, keep those that are worse than lim.
// Then split them into components, and remove any component with only
// one word.  (Shouldn't happen, but postprocessing will take care of
// it.)  Now reset the best_solution_time, which will essentially
// restart the clock, and return the number of components you found.

void show_suspects(const set<location> &suspects)
{
  cout << "**GUI suspect " << suspects.size();
  for (location loc : suspects) cout << ' ' << loc.x << ' ' << loc.y;
  cout << endl;
}

unsigned search_node::restrict_search(unsigned lim)
{
  map<square *,float> suspects; // square -> score
  for (auto &sq : squares) {
    float bad = -BIG_CHARGE;
    for (unsigned id : sq->word_ids) {
      const word &wd = (*this)[id];
      bad = max(bad,score(wd,&wd.get_solution()));
    }
    suspects[sq] = bad;
  }
  multimap<float,unsigned> words; // score -> word
  for (auto &wd : *this) {
    wd.pitch_block = -1;
    float bad = -BIG_CHARGE;
    for (auto sq : wd) bad = max(bad,suspects[sq]);
    words.insert(make_pair(bad,wd.id));                                  
  }
  set<unsigned> retained;
  for (auto itr = words.rbegin() ; itr != words.rend() ; ++itr) {
    if (retained.size() == lim) break;
    retained.insert(itr->second);
  }
  vector<set<unsigned>> cpts = find_components(retained);
  for (int i = 0 ; i < cpts.size() ; ++i)
    if (cpts[i].size() == 1) {
      cpts[i--] = cpts.back();
      cpts.pop_back();
    }
  set<location> susp;
  for (unsigned i = 0 ; i < cpts.size() ; ++i)
    for (unsigned w : cpts[i]) {
      (*this)[w].pitch_block = i;
      if (sets->gui) for (auto sq : (*this)[w]) susp.insert(*sq);
    }
  if (sets->gui) show_suspects(susp);
  best_solution_time = current_time();
  return cpts.size();
}

// Reset the search.  Reset the solution as well if requested, but
// mostly just clear the task stack and push on an empty task for each
// component you're analyzing.  Then for each task, any word that
// doesn't have a matching pitch block is just set to its value in the
// best fill.

void search_node::reset_search(bool reset_sol, unsigned cpts)
{
  reset_fill();
  if (reset_sol) reset_solution();
  stack.clear();
  stack.resize(cpts);
  for (unsigned i = 0 ; i < cpts ; ++i) {
    stack[i].pitch_block = i;
    for (auto &wd : *this)
      if (wd.pitch_block != i)
        stack[i].state.push_back(choice(wd.get_solution(),wd.id));
  }
}

// Solve the puzzle.  Reset the solver, clear the stack and push on a
// single task.  For each cutoff in delta, expand until
// search_continues returns false.  At the end of a pass, you have to
// restrict the words you're still willing to look at.

void search_node::solve(bool reset, bool reset_sol, unsigned &pass)
{
  if (reset) {
    pass = 0;
    reset_search(reset_sol,1);
  }
  for ( ; pass < sets->delta.size() ; ++pass) {
    do expand();
    while (search_continues(pass));
    if (1 + pass < sets->delta.size()) {
      unsigned cpts = restrict_search(sets->delta[1 + pass].cutoff());
      reset_search(false,cpts);
    }
  }
  if (sets->gui) cout << "**GUI suspect 0\n"; // clears suspects
  for (auto &wd : *this) wd.pitch_block = 0; // look at everything again
}

// Expand a single node off the task stack.  Go through and find the
// best task.  Pop it off the stack, and expand it.  Note that this
// code would break if the stack is empty, so test for that first.

void search_node::expand()
{
  if (stack.empty()) return;
  unsigned best = 0;
  for (size_t i = 1 ; i < stack.size() ; ++i)
    if (stack[i] < stack[best]) best = i;
  task t = stack[best];
  stack[best] = stack.back();
  stack.pop_back();
  expand(t);
}

// Here we expand a single task.  This involves the following steps:

// 1. Rebuild the puzzle.

// 2. Use the GUI to highlight any cases where the word in the best
// fill is in fact pitched.

// 3. Fill the puzzle from this point forward.  This returns true if
// the puzzle should be analyzed further.

// 4. If so, postprocess it, report the results, and add new tasks with
// higher discrepancies.

// 5. At the end, uncolor the pitched words.

void search_node::expand(const task &t)
{
  rebuild_from_task(t);                                                // 1
  choices.clear();
  vector<unsigned> pitch_marked;
  if (sets->gui)                                                       // 2
    for (auto &w : *this)
      if (find(w.pitched.begin(),w.pitched.end(),get_solution(w)) !=
          w.pitched.end()) {
        for (unsigned j : get_simple_words(w)) {
          cout << "**GUI pitch " << j << " 1" << endl;
          pitch_marked.push_back(j);
        }
      }
  if (fill_to_completion()) {                                           // 3
    if (best_score == BIG_SCORE) find_xrefs(false); // first time
    postprocess();                                                      // 4
    add_new_tasks(t);                                                   // 4
    report();
  }
  for (unsigned p : pitch_marked) cout << "**GUI pitch " << p << " 0" << endl;//5
}

// Expand a single task, returning true if you're done because the
// puzzle is full and false if you're just tuning and an error was
// made.  Repeatedly get the branch; if there isn't one, return true
// because the puzzle is full.  If you're making a mistake, then there
// are two cases:
// 1. If you're cheating, then pitch the word!
// 2. If not, and you're tuning, quit

// If you aren't making a mistake (or you've converted a mistake to a
// pitch above), add this choice.  If it's a pitch, pitch it.
// Otherwise, add the given fill and, if this is the first iteration,
// display it.  Then repeat.

bool search_node::fill_to_completion()
{
  for (choice b = branch() ; b.word_slot != size() ; b = branch()) {
    ++node_ct;
    if (error(b)) {
      if (sets->cheat) {
        b.pitch = true;
        cout << (*this)[b.word_slot].desc() << ' ' << b << endl;
      }
      else if (sets->tune) return false;
    }
    choices.push_back(b);
    if (b.pitch) (*this)[b.word_slot].pitched.push_back(b.fill); // cheating only
    else {
      insert((*this)[b.word_slot],b.fill);
      if (best_score == BIG_SCORE && sets->gui) request_display(false,false);
    }
  }
  return true;
}

// Branch choice.  For each unfilled word, get its fill value.  If
// it's got something useful to do, add it to the list of values.
// (Each is a float giving priority and the index of the word in
// question.)  Now at the end, if there is anything to do, it's at the
// end of the given list, since high values are better.

choice search_node::branch() const
{
  multimap<float,unsigned> values;
  for (auto &w : *this) {
    if (w.filled()) continue;
    float bv = values.empty()? 0 : values.rbegin()->first;
    float v = fill_value(w,bv);
    if (v > 0 && w.best_unpitched().size()) values.insert(make_pair(v,w.id));
  }
  if (values.empty()) return choice("",size());
  unsigned best_slot = values.rbegin()->second;
  return choice((*this)[best_slot].best_unpitched(),best_slot);
}

// Add new tasks.  This is actually trickier than it looks.  For each
// insertion (not pitch) that involves a word in the current "pitch
// block" (i.e., something you're modifying the search for), make a
// new task where the "top" of the search tree is the top of the given
// task, plus all the choices through that insertion.  Then flip the
// insertion itself into a pitch, and evaluate the cost of the node.

void search_node::add_new_tasks(const task &t)
{
  for (size_t i = 0 ; i < choices.size() ; ++i) {
    if (choices[i].pitch) continue;
    const word &wd = (*this)[choices[i].word_slot];
    if (wd.pitch_block != t.pitch_block) continue;
    stack.push_back(t);
    for (size_t j = 0 ; j <= i ; ++j) stack.back().state.push_back(choices[j]);
    stack.back().state.back().pitch = true;
    stack.back().find_cost();
  }
}

// "Cost" associated with a task.  Count pitches and pick at random if LDS
// >= 2.

void task::find_cost() const
{
  tcost.pitches = 0;
  for (size_t i = 0 ; i < state.size() ; ++i)
    if (state[i].pitch) ++tcost.pitches;
  tcost.extra = state.size();
  if (tcost.pitches > 1) tcost.extra = random() % 5000;
}

// Postprocess.  This actually involves three separate types of action:
// 1. Change a word, and then try to change all of the crossers.
// 2. Complete any words with blanks in them.
// 3. Consider the possibility that fixing one section may mess up another.

// POSTPROCESS TYPE 1

// For each word, replace most of it with spaces and then find the
// best fill.  We make wild any one or two characters, or the 3 "best"
// ones, where we want to consider changing a character if the
// crossing word could benefit.

// Here, we adjust the fill for w given a current _old_score_ and a
// list of indices of characters that we want to consider replacing.
// So just make all those characters wild, put the word in the puzzle,
// and get the best fill.  Now if something went wrong, there isn't
// anything to do -- put the old fill back.  Otherwise, put the best
// fill in.  If the new score is worse (it might be, due to
// multiwords), put the old fill back.  In all cases, return true if
// we improved the word and false if we didn't.  If there is a change,
// we mark the word as postprocessed and update _old_score_ as well
// (it's called by reference).

bool search_node::adjust_fill(word &w, float &old_score, vector<unsigned> wilds,
                              fill_cache &fc)
{
  string old = w.fill;
  string f = old;
  for (auto w : wilds) f[w] = WILD;
  insert(w,f);
  find_fills(w,0,false,&fc); // don't scale blame for crossing words
  if (w.best_fills.empty() || w.best_fills[0].fill.size() == 0) {
    insert(w,old);              // nothing to do
    return false;
  }
  insert(w,w.best_fills[0].fill);
  if (w.best_fills[0].fill == old) return false; // no change
  float new_score = score_with_crossers(w);
  if (new_score + 0.001 >= old_score) { 
    // old fill was better (or rounding error)
    insert(w,old);
    return false;
  }
  old_score = new_score;
  return true;
}

// Adjust the fill, trying all combinations of _makewild_ blanks
// chosen from the best _limit_ slots.

// 1. Get the blanks and nonblanks; wild characters have to stay wild.

// 2. Make sure you have a chance, in that (a) there aren't too many
//    blanks, and (b) the blanks and nonblanks together are
//    sufficient.  Now you have to try each.

// 3. Rank characters using the difference between the best fill
//    matching the crossing word with a wild, and the current score of
//    the crossing word.  Then multiply the current score by a tad bit
//    more than one so that ties resolve in favor of counting a lousy
//    crosser as worth changing.

// 4. Now you can get the set of nonblanks you want to consider, by
//    adding the squares in order until you've got enough.

// 5. Set up the loop by changing makewild to the number of "extra"
//    wilds you need, and then using newblank as the iterator, where
//    newblank is a collection of indices into nonblank.

// 6. Start with 0..n for the correct n, and each time increment that
//    last one, cycling back as need by when you hit the end.  It's a
//    little tricky because the last allowable value for slot n-1 is
//    in fact n-1 (not n), because slot n needs value n, etc.  With
//    each call, record whether or not there has been an improvement,
//    and note that adjust_fill updates old_score to stay current.

bool search_node::adjust_fill_detail(word &w, unsigned makewild, 
                                     unsigned limit, fill_cache &fc) 
{
  makewild = min(makewild,(unsigned) w.size());
  vector<unsigned> blank, nonblank;
  unsigned numcross = 0;
  multimap<float,unsigned> xscores; // score delta -> crossing position
  for (size_t j = 0 ; j < w.size() ; ++j)                               // 1
    if (w.fill[j] == WILD) blank.push_back(j);
    else if (w.has_crosser(j)) ++numcross;
  if (blank.size() > makewild) return false;                            // 2a
  if (makewild > blank.size() + numcross) return false;                 // 2b
  for (size_t j = 0 ; j < w.size() ; ++j)                               // 3
    if (w.fill[j] != WILD && w.has_crosser(j)) {
      float curr_score = 0, new_score = 0;
      for (size_t k = 0 ; k < w[j].word_ids.size() ; ++k) {
        unsigned xid = w[j].word_ids[k];
        if (xid == w.id) continue;
        word &wx = (*this)[xid];
        curr_score += score(wx);
        string p = wx.fill;
        p[w[j].positions[k]] = WILD;
        new_score += wx.indexer->find_match(p,0).score;
      }
      xscores.insert(make_pair(curr_score * 1.0001 - new_score,j));
    }

  bool changed = false;
  for (auto j = xscores.rbegin() ; j != xscores.rend() ; ++j) {         // 4
    nonblank.push_back(j->second);
    if (nonblank.size() == limit) break;
  }
  
  unsigned oldblank = blank.size();
  blank.resize(makewild);
  makewild -= oldblank;
  vector<unsigned> newblank(makewild);
  for (size_t j = 0 ; j < makewild ; ++j) newblank[j] = j;              // 5
  float old_score = score_with_crossers(w);
  for ( ;; ) {                                                          // 6
    for (size_t j = 0 ; j < makewild ; ++j)
      blank[oldblank + j] = nonblank[newblank[j]];
    changed |= adjust_fill(w,old_score,blank,fc);
    unsigned idx = newblank.size();
    for ( ; idx > 0 ; --idx)
      if (newblank[idx - 1] != nonblank.size() - 1 + (idx - newblank.size()))
        break;
    if (idx == 0) break;
    ++newblank[idx - 1];
    for ( ; idx < newblank.size() ; ++idx) newblank[idx] = newblank[idx - 1] + 1;
  }
  return changed;
}

// Postprocess a single word.  All single or double blanks, and the
// best 3-blank combo.

bool search_node::adjust_fill(word &w)
{
  fill_cache fc;
  fc.reset(w.size());
  bool ans = adjust_fill_detail(w,1,-1,fc); // consider all
  ans |= adjust_fill_detail(w,2,-1,fc);     // consider all
  ans |= adjust_fill_detail(w,3,3,fc);      // consider best 3 only
  return ans;
}

// When we do the actual postprocess, we start with a big list of
// words to consider.  Then some of them change (maybe), and we can
// use the changes to find a smaller list of words to consider -- the
// changed words and all of their crossers.  Then we repeat.  In
// reality, we add crossers of crossers where there has actually been
// a change, since they might change at the next iteration.

// Here we add wd and all of its crossers into _ans_.

void search_node::add_with_crossers(set<int> &ans, const word &wd) const
{
  ans.insert(wd.id);
  for (auto sq : wd)
    for (auto xid : sq->word_ids) if (xid != wd.id) ans.insert(xid);
}

// And now we add the crossers of crossers, although only for crossers
// of wrd where the fill actually changed.

void search_node::add_with_crossers(set<int> &ans, const word &wrd, 
                                    const string &other_fill) const
{
  add_with_crossers(ans,wrd);
  for (size_t i = 0 ; i < wrd.size() ; ++i)
    if (wrd.fill[i] != other_fill[i])
      for (auto xid : wrd[i].word_ids) 
        if (xid != wrd.id) add_with_crossers(ans,(*this)[xid]);
}

// And here we finally do the postprocess.

// 1. Start with the words you're oficially considering, best to
// worst.

// 2. Replace the set of words to consider as described above.

void search_node::postprocess(vector<int> &consider)
{
  set<int> changed;
  multimap<float,unsigned> words_by_score;
  for (auto c : consider) words_by_score.insert(make_pair(score((*this)[c]),c));
  for (auto i = words_by_score.rbegin() ; i != words_by_score.rend() ; ++i) { //1
    if (locked[i->second]) continue; // not allowed to change this word!
    word &w = (*this)[i->second];
    string f = w.fill;
    if (adjust_fill(w)) add_with_crossers(changed,w,f);                 // 2a
  }
  changed.erase(-1);                // remove unchecked crossers
  consider = vector<int>(changed.begin(),changed.end());                // 2b
}

// POSTPROCESS TYPE 2

// Try changing each word, from short to long.  Return true if you
// make a change.

bool search_node::finalize()
{
  bool change = false;
  multimap<unsigned,word *> words_by_length;
  for (auto &w : *this) words_by_length.insert(make_pair(w.size(),&w));
  for (auto &i : words_by_length) {
    word &w = *i.second;
    scored_word sw = get_fill(w,w.fill);
    if (sw.word.size() && sw.word != w.fill) {
      insert(w,sw.word);
      change = true;
    }
  }
  return change;
}

// POSTPROCESS TYPE 3

// The last bit of postprocessing is here.  What sometimes happens in
// postprocessing is that it's right to revert to a previous best
// solution, at least in areas.  This block of code handles that for
// all previous best solutions.

void search_node::combine_bests()
{
  vector<string> fills(size());
  for (int i = (*this)[0].best_solution.size() - 1 ; i >= 0 ; --i) {
    for (unsigned j = 0 ; j < size() ; ++j) 
      fills[j] = (*this)[j].best_solution[i];
    combine_best(fills);
  }
}

// Given a bunch of changed words, we want to consider replacing some
// of the fill with the values in the changed words.  But what subsets
// of the changed words are worth considering?  Certainly every
// component in the changed words; that's obvious.  But sometimes
// smaller sets are also worth considering because changing one of the
// changed words may have been a mistake.

// So we consider removing each changed word, and then breaking the
// original set into components again.  Then for each component, we
// try both that component, and putting the changed word back into
// that component.  This first function just computes all possible
// sets of words that might be of interest.

set<set<unsigned>> 
search_node::combination_components(const set<unsigned> &changed)
{
  vector<set<unsigned>> components = find_components(changed);
  set<unsigned> ch = changed;
  for (auto i : changed) {
    ch.erase(i);
    for (auto &j : find_components(ch)) {
      components.push_back(j);
      j.insert(i);
      components.push_back(j);
    }
    ch.insert(i);
  }
  return set<set<unsigned>>(components.begin(),components.end());
}

// And here we do the check against a specific previous best.  Get all
// the words that have changed, and find components that might
// correspond to useful changes.  Then for each component, set the
// fill to the current (new) fill, and let combine_component figure
// out if it's an improvement.  The second arg to combine_component is
// the new fill that we want to evaluate, and combine_component is
// supposed to return the puzzle in the better of the two possible
// states.

void search_node::combine_best(const vector<string> &fill)
{
  set<unsigned> changed;
  vector<string> current_fill(size());
  for (size_t i = 0 ; i < size() ; ++i) {
    current_fill[i] = (*this)[i].fill;
    if (current_fill[i] != fill[i]) changed.insert(i);
  }
  float sc = score();
  for (auto &cpt : combination_components(changed))
    combine_component(cpt,fill,sc);
}

// Try changing the fills in a given component.  If you make progress,
// use it.  Otherwise, put the old values back.  The *new* values are
// in _fill_.

void search_node::combine_component(const set<unsigned> &component,
                                    const vector<string> &fill,
                                    float &best_score)
{
  vector<string> old_fill(component.size());
  unsigned idx = 0;
  for (auto i : component) old_fill[idx++] = (*this)[i].fill;
  for (auto i : component) set_fill((*this)[i],fill[i]);
  float sc = score();
  if (sc < best_score) best_score = sc; // better!
  else {
    idx = 0;
    for (auto i : component) insert((*this)[i],old_fill[idx++]);
  }
}

// Postprocess the whole puzzle.  Each loop involves (a)
// postprocessing to termination and then (b) filling incomplete
// words.  We stop when filling incomplete words didn't help (since
// we've already postprocessed to quiescence).  At the end, we combine
// best fills from any sources.

void search_node::postprocess()
{
  if (sets->quick) return;
  if (sets->gui && best_score == BIG_SCORE) show_changes(false);
  do {
    vector<int> consider(size());
    for (size_t i = 0 ; i < size() ; ++i) consider[i] = i;
    while (!consider.empty()) postprocess(consider);
  }
  while (finalize());
  combine_bests();
  if (sets->gui && best_score == BIG_SCORE) show_changes();
}

// Report the results of the solution attempt, and rework crossrefs as
// well.

// 1. Print out the score, time etc.

// 2. Then redo the crossrefs.

// 3. Give a more detailed report using _check_.

// 4. If the solution has actually changed the fill (it might just
//    change values because of the xref stuff), mark this as a new
//    solution.

void search_node::report()
{
  float sc = score();
  if (best_score <= sc) return;

  if (verb() > 1)                                                       // 1
    cout << "improvement; current " << score() << " and old " << best_score
         << endl;
  if (!sets->tune)
    cout << *this << endl << "score: " << sc << " at time "
         << get_time() << " and " << node_ct << " node(s)" << endl;

  find_xrefs(false);          // replace crossref clues with current fill - 2
  if (sets->gui) request_display(true,false);                           // 3
  if ((*this)[0].answer.size() && verb() > SILENT && !sets->tune) check(false);
  best_score = sc;
  bool different = false;
  for (auto &w : *this)
    if (get_solution(w) != w.fill) {
      different = true;
      break;
    }
  new_best_solution();
  if (different) {                                                      // 4
    for (auto &w : *this) w.best_pitches = w.pitched;
    best_solution_time = current_time();
    if (!sets->tune) cout << "improvement! " << best_solution_time << endl;
  }
}

// We've found a new best solution; record it.

void search_node::new_best_solution()
{
  for (auto &w : *this) w.best_solution.push_back(w.fill);
}
