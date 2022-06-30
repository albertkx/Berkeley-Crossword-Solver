#include "xw.h"

// Given an index into a trie, move along by a character (new index is
// the relevant extender, right shifted by one).  Set ok to true if
// this might end a word (lo bit of the relevant extender).

int trie::advance(int idx, char c, bool &ok) const
{
  const extender &e = (*this)[idx];
  ok = e[c] & 1;
  return e[c] >> 1;
}

// Is a word in a trie?  Just march along; the last character is a
// little special because if we can't continue, we don't care.

bool trie::operator[](const string &w) const
{
  int idx = 0;
  bool result;
  for (unsigned i = 0 ; 1 + i < w.size() ; ++i)
    if ((idx = advance(idx,w[i],result)) < 0) return false;
  advance(idx,w.back(),result);
  return result;
}

// Add a string to a trie.  Figure out where it goes and mark it. If
// the relevant extender is simply missing, add it.

void trie::add(const string &w)
{
  int idx = 0;
  for (unsigned i = 0 ; 1 + i < w.size() ; ++i) {
    int &next = (*this)[idx][w[i]];
    if (next < 0) {
      next = (size() << 1) | (next & 1);
      push_back(extender());
    }
    idx = (*this)[idx][w[i]] >> 1;
  }
  (*this)[idx][w.back()] |= 1;
}

// Convert a puzzle to a cgrid (vector<vector<char>>).  Use either the
// fill or the correct answer (the latter is for testing only!!).

cgrid::cgrid(const puzzle &puz, bool cheat)
{
  resize(puz.cols());
  for (auto &v : *this) v = vector<char>(puz.rows(),0);
  for (auto &sq : puz.squares)
    (*this)[*sq] = tolower(cheat? sq->answer : sq->fill);
}

// Given a grid, is a particular location a block?  Yes, if it's off
// the edge or an actual block.

bool cgrid::is_block(location l) const
{
  return l.x < 0 || l.x >= cols() || l.y < 0 || l.y >= rows() || (*this)[l] == 0;
}

// Split a boggle_string into linear segments.  This is a bit tricky
// because we want to get the ends right.  Each segment is a starting
// square, and one past the ending square.

// Initially, it's easy.  We just get the points where the direction
// changes.  Then in theory, each segment would be a starting point
// followed by the next starting point (one past the end of the
// previous segment).  But there are a variety of adjustments:

// 1. Don't include length 1 "segments".
// 2. If the previous square is actually also on this segment, include it.
// 3. If this is a diagonal segment of length 2, and it's not the second
//    segment, make it of length 1. (Diagonal segments are always second.)

vector<segment> boggle_string::segments() const
{
  vector<unsigned> starts(1,0);
  location last_delta = squares[1] - squares[0];
  for (unsigned i = 2 ; i < size() ; ++i) {
    location delta = squares[i] - squares[i - 1];
    if (delta == last_delta) continue;
    last_delta = delta;
    if (i != starts.back() + 1) starts.push_back(i);                    // 1
  }
  starts.push_back(size());
  vector<segment> ans;
  for (size_t i = 0 ; i + 1 < starts.size() ; ++i) {
    unsigned s = starts[i], e = starts[i + 1];
    if (s + 1 == size()) --s;
    location delta = get_delta(s);
    if (s && delta == get_delta(s - 1)) --s;                            // 2
    if (i != 1 && delta.x && delta.y && e - s == 2) ++s;                // 3
    ans.push_back(segment(s,e));
  }
  return ans;
}

// Describe the geometry in a boggle_string.  Show the segments and
// then the directionality of each piece, which is Up Down Right Left
// or Angle.  If there is jump across a block, add = to the string
// there.  If there is no overlap between segments, add | there.

string boggle_string::descriptor() const
{
  vector<segment> segs = segments();
  bool dbl = false;
  string ans = "[";
  for (unsigned i = 0 ; i < segs.size() ; ++i) {
    unsigned a = segs[i].first, b = segs[i].second;
    char last = ans.empty()? 0 : ans.back();
    if (i && a == segs[i - 1].second) {
      location delta = get_delta(segs[i - 1].second - 1);
      if (delta.x == 2 || delta.y == 2) ans.push_back('='); // jump
      else ans.push_back('|');                              // "split"
    }
    if (b == a + 1) {
      if (i == 0) dbl = true;
      ans.push_back(last);
    }
    else {
      location delta = get_delta(a);
      char add;
      if (delta == location(1,0)) add = 'R';
      else if (delta == location(0,1)) add = 'D';
      else if (delta == location(-1,0)) add = 'L';
      else if (delta == location(0,-1)) add = 'U';
      else add = 'A';
      if (dbl) { ans[1] = add; dbl = false; } // leave room for opening [
      ans.push_back(add);
    }
  }
  return ans + ']';
}

// Find all the geometric words "hidden" in a puzzle. Just call the
// search function, starting at each square in the grid.  changeable
// is a set of locations where you can choose a different letter if
// you want; if it's empty, you can do that anywhere.

vector<boggle_string> trie::search(const cgrid &grid, unsigned misses,
                                   const set<location> &changeable) const
{
  vector<boggle_string> ans;
  location l;
  for (l.x = 0 ; l.x < grid.size() ; ++l.x)
    for (l.y = 0 ; l.y < grid[l.x].size() ; ++l.y)
      if (grid[l]) search(grid,changeable,trie_status(l,false,false,misses),ans);
  return ans;
}

// Wrapper to see if a boggle string is ok.  Cast away const, push on
// the new letter, get the answer, pop the letter, and you're good.

bool boggle_string::ok_extension(const cgrid &grid, location delta) const
{
  location newsq = squares.back() + delta;
  boggle_string &bs = * const_cast<boggle_string *>(this);
  bs.push_back(0,newsq);
  bool ans = bs.ok_string(grid);
  bs.pop_back();
  return ans;
}

// To see if a boggle string is ok, it depends on the number of
// segments:

// 1. Four segments are too many.
// 2. If three segments, the first segment must be "forward".
// 3. If three segments, it must start at a block.

bool boggle_string::ok_string(const cgrid &grid) const
{
  vector<segment> segs = segments();
  location delta = squares[1] - squares[0];
  bool reverse = delta.x < 0 || delta.y < 0;
  if (segs.size() > (reverse? 2 : 3)) return false;                     // 1 2
  return segs.size() < 3 || grid.is_block(squares[0] - delta);          // 3
}

// Number of blocks on the [a,b) segment.  Take a little care in case
// the segment is one square at the end, in which case you have to get
// the delta by looking backwards instead of forwards.  Then it's all
// easy.

unsigned boggle_string::num_blocks(const cgrid &grid, segment p) const
{
  unsigned a = p.first, b = p.second;
  location delta;
  if (a + 1 == size()) delta = get_delta(--a);
  else delta = get_delta(a);
  unsigned ans = 0;
  if (grid.is_block(squares[a] - delta)) ++ans;
  if (grid.is_block(squares[b - 1] + delta)) ++ans;
  return ans;
}

// Is it ok to end a boggle_string here?  It all depends on the number
// of segments.

// 1. One segment: it has to be backward or up.

// 2. Two segments:
// 2a. At least three of the four possible blocks must be present
// 2b. Unless it's a runon, there must be a block at the beginning, there
//     must be overlap, and you can't go "backwards".
//     the "turn" must be in both segments
// 2c. If d1 and d2 are in the same direction, and it's not a runon
//     because there is lateral displacement, there has to be a block
//     at the beginning and both segments must be >= 3 squares.  It also
//     can't have an angle > 90.
// 2d. Unless it's a pure reversal, there can be no repeated squares
// 2e. If it starts backwards, the bend has to be at a block.
// 2f. There can be no segment of length < 2 (runon) or < 3 (not runon).

// 3. Three segments:
// 3a. At most one segment can be less than three squares.
// 3b. The overlap must either exist at both turns or not exist at both.
// 3c. Begin by getting all three directions, but if segment 0 or 2 is of
//     length 1, give it the direction of the middle segment.
// 3d. If the middle segment is DIAGONAL, then the first and last segments
//     must be enclosed in black squares.
// 3e. If the middle segment is NOT DIAGONAL, then the middle segment must
//     be enclosed in black squares, the whole "word" must begin and end
//     with a black square, and it can't end orthogonal to its beginning.
//     Finally, you have to return to the row from which you started.
//     FIXME: Why is that last condition there?
// 3f. If all three directions are the same, you're done.
// 3g. If there are duplicate squares, fail.
// 3h. If there is no diagonal, succeed.
// 3i. Given a diagonal, the last segment must be of size at least 3.

// At the end, unless there are three segments going the same
// direction, the last segment must be of size at least three.

bool boggle_string::ok_word(const cgrid &grid, const trie_status &ts) const
{
  if (size() < 3) return false;
  vector<segment> segs = segments();
  switch (segs.size()) {
  case 1: {
    location delta = get_delta(0);
    return num_blocks(grid,segs[0]) == 2 && (delta.x < 0 || delta.y < 0);
  }
  case 2: {
    unsigned n1 = num_blocks(grid,segs[0]);
    unsigned n2 = num_blocks(grid,segs[1]);
    if (n1 + n2 < 3) return false;                                      // 2a
    location d1 = get_delta(0);
    location d2 = get_delta(segs[1].first);
    if (d1 != d2) {                                                     // 2b
      if (!grid.is_block(squares[0] - d1)) return false;
      if (segs[0].second == segs[1].first) return false;
    }
    else if (d1.x? (squares[0].y != squares[segs[1].first].y) :
             (squares[0].x != squares[segs[1].first].x)) { // not a runon
      if (!grid.is_block(squares[0] - d1)) return false;                // 2c
      if (segs[0].size() < 3 || segs[1].size() < 3) return false;
      if (d1.dot(get_delta(segs[0].second - 1)) < 0) return false; // backwards
    }
    if (has_dups(squares) && (d1 != -d2 || ts.misses_used)) return false;// 2d
    if ((d1.x < 0 || d1.y < 0) && !grid.is_block(squares[segs[0].second] + d1))
      return false;                                                     // 2e
    unsigned small = 3 - (d1 == d2);
    return segs[0].second - segs[0].first >= small &&
      segs.back().second - segs.back().first >= small;
  }
  case 3: {
    unsigned small = 0;
    for (auto seg : segs) if (seg.size() < 3) ++small;
    if (small > 1) return false;                                        // 3a
    bool overlap1 = (segs[1].first == segs[0].second);
    bool overlap2 = (segs[2].first == segs[1].second);
    if (overlap1 != overlap2) return false;                             // 3b
    location d[3];
    d[1] = get_delta(segs[1].first);
    if (segs[0].second == segs[0].first + 1) d[0] = d[1];
    else d[0] = get_delta(0);
    if (segs[2].second == segs[2].first + 1) d[2] = d[1];
    else d[2] = get_delta(segs[2].first);                               // 3c
    bool diag = (d[1].x && d[1].y);
    if (diag) {                                                         // 3d
      if (num_blocks(grid,segs[0]) != 2 || num_blocks(grid,segs[2]) != 2)
        return false;
    }
    else {
      if (num_blocks(grid,segs[1]) < 2) return false;                   // 3e
      if (!grid.is_block(squares[segs[0].first] - d[0])) return false;
      if (!grid.is_block(squares[segs[2].second - 1] + d[2])) return false;
      if (d[0].dot(d[2]) == 0) return false;
      // if (d[0].x? (squares.back().y != squares[0].y) :
      //     (squares.back().x != squares[0].x)) return false;
    }
    if (d[0] == d[1] && d[1] == d[2]) return true;                      // 3e
    if (has_dups(squares)) return false;                                // 3f
    if (!diag) return true;                                             // 3g
    return segs.back().second - segs.back().first >= 3;                 // 3h
  }
  default: return false;        // too many segments
  }
}

// The basic trie search function.  Inputs:
//  grid        the filled grid
//  changeable  locations where you can make a change, or empty if you
//              can change anything
//  ts          status of the search (jump, changes, misses, etc)
//  results     all the words you've found
//  cpartial    current string being constructed

// Once you've decided what letter to add, do the following:

// 1. Add the new square to the boggle_string.

// 2. If we can end here, and something interesting has happened (a
// direction change or a jump), and we've completed a word, add it to
// the results.

// 3. If we're "off the trie", we're done.

// 4. If we're just starting the word (one letter in it), try going
// across or down (in either direction).

// 5. Otherwise, try continuing in a straight line (note that we don't
// do this if a letter is being repeated).  That means:

// 5a. Increment by delta
// 5b. If delta is not diagonal, and is not a jump, try jumping in
//     that direction.

// 6. The remaining case is where we change direction.  First off
// (6a), if we've jumped or started off backwards, we don't change
// direction. Now look at each possible new delta, and:
// 6b. If it's the same as the old delta, don't do it.
// 6c. If it takes you off the grid or into a block, don't do it.
// 6d. If you're staying on the same square and you've already had a change,
//     don't do it.
// 6e. If you're going backwards and you've already had a change, don't do it.
// 6f. Finally, if the extension is ok, do it!

void trie::search(const cgrid &grid, const set<location> &changeable,
                  const trie_status &ts, vector<boggle_string> &results, 
                  const boggle_string &cpartial) const
{
  search(grid,changeable,grid[ts.loc],ts,results,cpartial);
  if (ts.misses_used < ts.misses_allowed && // allow a character to be wrong
      (changeable.empty() || changeable.find(ts.loc) != changeable.end())) {
    trie_status tsm = ts;
    ++tsm.misses_used;
    for (char c = 'a' ; c <= 'z' ; ++c)
      if (c != grid[ts.loc]) search(grid,changeable,c,tsm,results,cpartial);
  }
}

void trie::search(const cgrid &grid, const set<location> &changeable,
                  char c, const trie_status &ts, vector<boggle_string> &results, 
                  const boggle_string &cpartial) const
{
  boggle_string &partial = * const_cast<boggle_string *>(&cpartial);
  bool wrd;
  if (c < 'a' || c > 'z') return; // unfilled square, basically
  partial.push_back(c,ts.loc);                                          // 1
  int idx = advance(ts.idx,partial.back(),wrd);
  if (wrd && cpartial.ok_word(grid,ts)) results.push_back(cpartial);    // 2
  if (idx < 0) { partial.pop_back(); return; }                          // 3
  if (partial.size() == 1) {                                            // 4
    if (!grid.is_block(ts.loc + location(1,0)))
      search(grid,changeable,trie_status(ts,idx,{1,0}),results,partial);
    if (!grid.is_block(ts.loc - location(1,0)))
      search(grid,changeable,trie_status(ts,idx,{-1,0}),results,partial);
    if (!grid.is_block(ts.loc + location(0,1)))
      search(grid,changeable,trie_status(ts,idx,{0,1}),results,partial);
    if (!grid.is_block(ts.loc - location(0,1)))
      search(grid,changeable,trie_status(ts,idx,{0,-1}),results,partial);
    partial.pop_back();
    return;
  }
  if (ts.loc != location(0,0) && grid.contains(ts.loc + ts.delta)) {    // 5
    if (grid[ts.loc + ts.delta])                                        // 5a
      search(grid,changeable,trie_status(ts,idx,ts.delta),results,partial);
    else if (!ts.changes && (ts.delta.x == 0 || ts.delta.y == 0) &&
             (ts.delta.x == 1 || ts.delta.y == 1)) {                    // 5b
      if (ts.delta.x && ts.loc.x + 2 < grid.cols() &&
          grid[ts.loc + location(2,0)])
        search(grid,changeable,trie_status(ts,idx,{2,0},false,true),results,
               partial);
      else if (ts.delta.y && ts.loc.y + 2 < grid.rows() &&
               grid[ts.loc + location(0,2)])
        search(grid,changeable,trie_status(ts,idx,{0,2},false,true),results,
               partial);
    }
  }
  if (ts.jump) { partial.pop_back(); return; }                          // 6a
  location delta = partial.get_delta(0);
  if ((delta.x < 0 || delta.y < 0) && ts.changes) { partial.pop_back(); return; }
  location dd;
  for (dd.x = -1 ; dd.x <= 1 ; ++dd.x)
    for (dd.y = -1 ; dd.y <= 1 ; ++dd.y) {
      if (dd == ts.delta) continue;                                     // 6b
      if (grid.is_block(ts.loc + dd)) continue;                         // 6c
      if (ts.changes && dd == location(0,0)) continue;                  // 6d
      if (ts.changes && dd == -ts.delta) continue;                      // 6e
      if (cpartial.ok_extension(grid,dd))                               // 6f
        search(grid,changeable,trie_status(ts,idx,dd,true),results,partial);
    }
  partial.pop_back();
}

// Current fill.  If the word has an answer, use that.  But if the
// word's answer has wildcards (or any non-letter), get it from the
// grid.

string cgrid::find_fill(const word &wd) const
{
  bool good_string = true;
  for (auto c : wd.fill) if (c < 'a' || c > 'z') { good_string = false; break; }
  if (good_string) return wd.fill;
  string ans;
  for (auto sq : wd) ans.push_back((*this)[*sq]);
  return ans;
}

// Number of squares in common between a word and a vector of
// locations

unsigned commonality(const word &w, const vector<location> &squares)
{
  unsigned ans = 0;
  for (auto sq : w)
    if (find(squares.begin(),squares.end(),*sq) != squares.end()) ++ans;
  return ans;
}

// When you insert a boggle string into a puzzle, you have to figure
// out what words it corresponds to.  There are the following cases
// (and note that we only deal with each word once):

// 1. A word uses a single square of the boggle string.  Treat
// normally.

// 2. More than one square from a word is in the boggle string, but
// not all such squares are.  Then:

// 2a. If the first square of the word is not in the boggle string,
// ignore it.  The clue is somewhere else.

// 2b. If the first square of the word is in the boggle string, then
// the boggle string is supposedly fill for this clue.

// 3. All of the squares in the word are in the boggle string.  Now
// there are also two cases:

// 3a. If this word is the same length as the number of missed squares
// in some other word, then the missed squares become the fill for
// this word.

// 3b. In all other cases, treat the boggle string as replacement fill
// for the given string, provided that either there was no clue for
// the slot in question or this is the first clue you have.

struct geo_evaluator : string {
  unsigned      id;

  geo_evaluator(int i, const string &f) : string(f), id(i) { }
  geo_evaluator(unsigned i, const string &f) : string(f), id(i) { }
};

float search_node::update(const geo_evaluator &old_fill,
                          const geo_evaluator &new_fill) const
{
  float old_score = BIG_CHARGE, new_score = BIG_CHARGE;
  const word &wold = (*this)[old_fill.id];
  const word &wnew = (*this)[new_fill.id];
  if (wold.analyzed[old_fill.size()] &&
      wnew.analyzed[new_fill.size()]) {
    old_score = get_fill(wold,old_fill).score;
    new_score = get_fill(wnew,new_fill).score;
  }
  if (old_score == BIG_CHARGE) old_score = d->evaluate(old_fill,wold.clue);
  if (new_score == BIG_CHARGE) new_score = d->evaluate(new_fill,wnew.clue);
  return new_score - old_score;
}

float search_node::evaluate(const boggle_string &bs, const cgrid &grid) const
{
  vector<geo_evaluator> olds, news; // word-clue pairs
  int boggle_clue = -1;
  vector<location> missed;
  const vector<location> &squares = bs.squares;
  set<unsigned> ids;
  for (unsigned i = 0 ; i < bs.size() ; ++i) {
    location l = squares[i];
    bool changed = (grid[l] != bs[i]);
    const square *sq = find_square(l);
    if (!sq) continue;          // shouldn't happen, but don't crash
    for (unsigned j = 0 ; j < sq->word_ids.size() ; ++j) {
      const word &wd = (*this)[sq->word_ids[j]];
      if (ids.find(wd.id) != ids.end()) continue;
      ids.insert(wd.id);
      unsigned n = commonality(wd,squares);
      if (n == 1) {                                                        // 1
        if (changed) {
          string fill = grid.find_fill(wd);
          olds.push_back({wd.id,fill});
          fill[sq->positions[j]] = bs[i];
          news.push_back({wd.id,fill});
        }
      }
      else if (n < wd.size()) {                                            // 2
        if (find(squares.begin(),squares.end(),wd[0]) != squares.end()) {  // !2a
          if (boggle_clue < 0) boggle_clue = wd.id;
          olds.push_back({wd.id,grid.find_fill(wd)});
          news.push_back({boggle_clue,bs});
          for (unsigned k = 1 ; k < wd.size() ; ++k)
            if (find(squares.begin(),squares.end(),wd[k]) == squares.end())
              missed.push_back(wd[k]);
        }
      }
      else if (wd.size() == missed.size()) {                               // 3a
        olds.push_back({wd.id,grid.find_fill(wd)});
        string newfill;
        for (auto &l : missed) newfill.push_back(grid[l]);
        news.push_back({wd.id,newfill});
      }
      else if (wd.clueless() || boggle_clue < 0) {                         // 3b
        int newclue = boggle_clue;
        if (boggle_clue < 0) newclue = boggle_clue = wd.id;
        olds.push_back({wd.id,grid.find_fill(wd)});
        news.push_back({newclue,bs});
      }
    }
  }
  float ans = 0;
  for (unsigned i = 0 ; i < olds.size() ; ++i) ans += update(olds[i],news[i]);
  return ans;
}

// Geo_type construction is much like descriptor() above; at the end,
// in the R|R|R case, reduce # segs to two because that seems
// possible.

geo_type::geo_type(const boggle_string &bs) :
  diagonal(false), jump(false), overlap(false), reversed(false)
{
  vector<segment> segs = bs.segments();
  num_segs = segs.size();
  if (num_segs == 1) return;
  overlap = (segs[0].second > segs[1].first);
  for (unsigned i = 0 ; i < segs.size() ; ++i) {
    unsigned a = segs[i].first, b = segs[i].second;
    if (i && !overlap) {
      location delta = bs.get_delta(segs[i - 1].second - 1);
      if (delta.x == 2 || delta.y == 2) jump = true;
    }
    if (b != a + 1) {
      location delta = bs.get_delta(a);
      if (delta.x && delta.y) diagonal = true;
      else if (delta.x < 0 || delta.y < 0) reversed = true;
    }
  }
  if (!diagonal && !jump && num_segs == 3) {
    if (!overlap) num_segs = 2; // reduce 3 to 2 in this case
    else {
      location d1 = bs.get_delta(0);
      location d2 = bs.get_delta(bs.size() - 2);
      reversed = (d1 != d2);    // like a "U"
    }
  }
}

// When you evaluate a geometry, you get a total score, the number of
// times it seems to have appeared, and whether you can stop thinking
// about it because subsequent appearances aren't so good.

struct geo_eval {
  float    total_score;
  unsigned num;
  bool     closed;
  set<location> locs;           // can't use same loc twice
  vector<boggle_string> words;

  geo_eval() : total_score(0), num(0), closed(false) { }
  geo_eval(float sc) : total_score(sc), num(1), closed(false) { }
  geo_eval operator+=(float sc) { total_score += sc; ++num; return *this; }
  geo_eval operator+(float sc) const {
    geo_eval ans = *this;
    ans += sc;
    return ans;
  }
  operator float() const { return total_score * num; }
};

// When we find the geometry associated with a puzzle, we will have a
// map from floats to squares, indicating how bad each square is, and
// we will want to mark as changeable the worst n squares.  This
// produces a set of locations from such a map.

template<class T>
set<location> firstn(const multimap<T,square *> &m, unsigned n)
{
  set<location> ans;
  if (n == 0) return ans;
  for (auto &p : m) {
    ans.insert(location(*(p.second)));
    if (ans.size() == n) break;
  }
  return ans;
}

// Find the geometry associated with a puzzle.

// Get the worst squares, and then do the trie search as many times as
// we have BOGGLE_ERRORS, where BOGGLE_ERRORS[i] is the number of
// squares to mark as changeable if there are i+1 changes allowed.
// This produces a bunch of boggle_strings.

// Take the boggle_strings, evaluate them, and store them in a big
// multimap, sorted by value.  Then we go through, adding each one to
// an associated geo_eval.  We close a particular geo_eval when the
// associated geo_type was not improved by the last relevant
// boggle_string.  (After all, the next one will be worse.)  Then we
// find the best geo_eval and return the geo_type that generated it.

geometry search_node::find_geometry() const
{
  cgrid g(*this);
  if (sets->explain_theme) cout << "geometry search for grid\n" << g << endl;
  set<location> changeable;
  multimap<float,square *> suspects; // score -> square
  for (auto &sq : squares) {
    float bad = -BIG_CHARGE;
    for (unsigned id : sq->word_ids) {
      const word &wd = (*this)[id];
      bad = max(bad,score(wd,&wd.get_solution()));
    }
    suspects.insert(make_pair(-bad,sq)); // bad squares first
  }
  vector<boggle_string> bs_results = geo.words;
  for (unsigned i = 0 ; i < BOGGLE_ERRORS.size() ; ++i)
    for (auto &b : d->word_trie.search(g,i + 1,
                                       firstn(suspects,BOGGLE_ERRORS[i]))) {
      if (find(geo.words.begin(),geo.words.end(),b) == geo.words.end()) {
        b.value = evaluate(b,g);
        bs_results.push_back(b);
      }
    }
  sort(bs_results.begin(),bs_results.end());
  if (sets->explain_theme) {
    cout << "geometry search finds:\n";
    for (auto &bs : bs_results) cout << ' ' << bs << endl;
  }
  map<geo_type,geo_eval> by_type;
  for (auto &p : bs_results) {
    float diff = p.value;
    geo_type gt(p);
    auto itr = by_type.find(gt);
    location loc = p.squares[0];
    if (itr == by_type.end()) {
      by_type[gt] = diff;
      by_type[gt].locs.insert(loc);
      by_type[gt].words.push_back(p);
    }
    else if (!itr->second.closed) {
      if (itr->second + diff > itr->second) itr->second.closed = true;
      else if (itr->second.locs.find(loc) == itr->second.locs.end()) {
        itr->second += diff;
        itr->second.locs.insert(loc);
        itr->second.words.push_back(p);
      }
    }
  }
  geometry ans;
  ans.score = -GEO_CUTOFF;
  for (auto &p : by_type) if (float(p.second) < ans.score) {
      ans = p.first;
      ans.score = float(p.second);
      ans.words = p.second.words;
    }
  if (sets->explain_theme) cout << "geometry returning " << ans << endl;
  return ans;
}

// Reset the square *s in a word so that they match the given vector
// of locations.  Clear everything, deal with the locations one at a
// time, and then reestablish all the pointers.

void puzzle::reset_squares(word &wd, const vector<location> &locs)
{
  wd.clear_pointers();
  wd.clear();
  wd.answer.clear();
  wd.fill.clear();
  for (auto l : locs) {
    wd.push_back(find_square(l));
    wd.fill.push_back(wd.back()->fill);
    wd.answer.push_back(wd.back()->answer);
  }
  wd.establish_pointers(true);
}

// Replace a word for geometry reasons.  Reset the squares and, if
// there are no new numbers, you're done.  Otherwise, use the new
// numbers, don't honor splits and reinitialize this slot.

void puzzle::geo_replace(const geo_replacement &rep)
{
  word &w = (*this)[rep.id];
  reset_squares(w,rep.squares);
  if (rep.numbers.empty()) return;
  w.numbers = rep.numbers;
  w.honor_splits = false;
  w.set_initialize();
}

// Given a vector of locations that is now supposed to be a word, and
// a flag indicating if a jump is involved, modify some existing word
// so that it matches the new location vector.  If words are to be
// removed, push their indices onto to_remove to deal with later.

// 1. (a) Find the word W that starts with the same two squares as the
//    geo word G.  (b) If there is no such word, and this is a jump,
//    try post the first jump.  (c) If there is still no such word,
//    fail.

// 2. If the squares in G are a superset of the squares in W, then (a)
//    replace the word W with the word G.  (b) For any other word X
//    whose squares are a subset of G, add X's number to the numbers
//    for X and, (c) if X has no clue, remove X from the puzzle.
//    Succeed.

// 3. Otherwise, there must be some squares in W that are not in G.
//    (a) If there are squares in W that are a displacement of those
//    squares, and that make up a whole word, then swap the squares in
//    the two words appropriately.  Succeed.  (b) If there is no such
//    displacement, fail.

bool puzzle::geo_word(const vector<location> &locs, bool jump,
                      vector<unsigned> &to_remove,
                      vector<geo_replacement> &replacements) const
{
  int new_id = -1;
  for (auto &w : *this)                                                 // 1a
    if (w[0] == locs[0] && w[1] == locs[1]) {
      new_id = w.id;
      break;
    }
  if (new_id < 0 && jump)                                               // 1b
    for (auto &w : *this)
      if (find(locs.begin(),locs.end(),w[0]) != locs.end() &&
          find(locs.begin(),locs.end(),w[1]) != locs.end()) {
        new_id = w.id;
        break;
      }
  if (new_id < 0) return false;                                         // 1c
  const word &w = (*this)[new_id];
  vector<unsigned> numbers = w.numbers;
  if (w.included_in(locs)) {
    if (w.size() == locs.size()) return true; // already have it! (geo recursion)
    for (auto &w : *this)
      if (w.id != new_id && w.included_in(locs)) {                      // 2b
        numbers.push_back(w.numbers[0]);
        if (w.clueless()) to_remove.push_back(w.id);                    // 2c
      }
  }
  else {
    vector<location> missing;
    for (auto &sq : w)
      if (find(locs.begin(),locs.end(),*sq) == locs.end())
        missing.push_back(*sq);
    int replacement_id = -1;
    for (auto &w : *this) {                                             // 3a
      if (w.size() == missing.size()) {
        location delta;
        for (unsigned i = 0 ; i < w.size() ; ++i) {
          location d = w[i] - missing[i];
          if (i) {
            if (d != delta) goto nextw;
          }
          else if (abs(d.x) + abs(d.y) > 1) goto nextw;
          else delta = d;
        }
        replacement_id = w.id;
        break;
      }
    nextw: ;
    }
    if (replacement_id < 0) return false;                               // 3b
    replacements.push_back(geo_replacement(replacement_id,missing));
    // note that no numbers means no reinitialization of the filler for this word
  }
  replacements.push_back(geo_replacement(new_id,locs,numbers));         // 2a
  return true;
}
