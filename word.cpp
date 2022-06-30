#include "xw.h"

bool simple_word::includes(const square &sq) const
{
  if (sq.x == start.x)
    return sq.y == start.y ||
      (!across && sq.y >= start.y && sq.y - start.y < length);
  return sq.y == start.y && across && sq.x >= start.x && sq.x - start.x < length;
}

// Position of a square in a skeleton.  Just find it.  NOTE THAT THIS
// FUNCTION WORKS WITH THE ADDRESSES OF THE SQUARES, NOT THEIR
// CONTENTS.

unsigned skeleton::position(const square &s) const
{
  for (unsigned ans = 0 ; ans < size() ; ++ans)
    if (&(*this)[ans] == &s) return ans;
  return -1;
}

// Is there a crossing word at position i?  Fastest way to check is to
// see if more than one word uses the square in question.

bool skeleton::has_crosser(unsigned i) const
{
  return (*this)[i].word_ids.size() > 1;
}

// position in this word at which a word crosses the given word.  Easy
// to compute from ids of words using each square.

unsigned skeleton::cross_pos(const skeleton &crosser) const
{
  for (unsigned ans = 0 ; ans < size() ; ++ans) {
    const vector<unsigned> &p = (*this)[ans].word_ids;
    if (find(p.begin(),p.end(),crosser.id) != p.end()) return ans;
  }
  return -1;
}

// Number of active crossers of a word.  Fill has to be . and there
// has to actually be a crossing word (might be unchecked letters).

unsigned skeleton::active_crossers() const
{
  unsigned ans = 0;
  for (size_t i = 0 ; i < size() ; ++i) 
    if (fill[i] == WILD && has_crosser(i)) ++ans;
  return ans;
}

// Here are various utilities to sort out the pointers in a puzzle,
// where the squares have to have pointers to the words that contain
// them.  Here we clear the pointers to a given skeleton (along with
// the associated position markers).

// If this is word x, then every square in x should list x as a word
// using that square.  If this doesn't happen, assert.

void skeleton::clear_pointers()
{
  for (auto sq : *this) {
    for (size_t j = 0 ;; ++j)   // has to be there
      if (sq->word_ids[j] == id) {
        sq->word_ids[j] = sq->word_ids.back();
        sq->word_ids.pop_back();
        sq->positions[j] = sq->positions.back();
        sq->positions.pop_back();
        break;
      }
      else assert(j < sq->word_ids.size());
  }
}

// And here, we add the pointers for a given skeleton.

void skeleton::establish_pointers()
{
  for (size_t i = 0 ; i < size() ; ++i) {
    square &sq = (*this)[i];
    sq.word_ids.push_back(id);
    sq.positions.push_back(i);
  }
}

// Sort out the pointers for a basic_word.  The skeleton version does
// much of it, but we also check for repeated squares in the word, if
// requested.

void basic_word::establish_pointers(bool check_repeated)
{
  skeleton::establish_pointers();
  repeated_squares.clear();
  if (!check_repeated) return;
  for (size_t j = 0 ; j < size() ; ++j)
    for (size_t k = 1 + j ; k < size() ; ++k)
      if ((*this)[j] == (*this)[k]) {
        repeated_squares.push_back(vector<unsigned>(1,j));
        repeated_squares.back().push_back(k);
      }
}

// Is a word included in a vector of locations?

bool skeleton::included_in(const vector<location> &locs) const
{
  for (auto sq : *this)
    if (find(locs.begin(),locs.end(),*sq) == locs.end()) return false;
  return true;
}

// Remove a word from the puzzle.  (This is called if a cross-ref is
// causing two words to collapse to one.)  It's basically a matter of
// dropping the final word into this slot.  NOTE THAT THIS FUNCTION
// WILL MESS UP THE SQUARE POINTERS BECAUSE THE IDs WILL CHANGE.

void puzzle::remove_word(unsigned x)
{
  word &replaceme = (*this)[x];
  replaceme = back();
  replaceme.id = x;
  replaceme.set_initialize();
  pop_back();
}

// Remove a bunch of words and then fix the pointers.  Also remove the
// best pitches and best solution, which presumably no longer make
// sense.

void puzzle::remove_words(vector<unsigned> &rems)
{
  sort(rems.begin(),rems.end());
  for (int i = rems.size() - 1 ; i >= 0 ; --i) remove_word(rems[i]);
  for (auto &sq : squares) {
    sq->word_ids.clear();
    sq->positions.clear();
  }
  for (auto &w : *this) {
    w.establish_pointers(true);
    w.best_solution.clear();
    w.best_pitches.clear();
  }
}

// # of unfilled characters of a word.

unsigned skeleton::unfilled() const
{
  return count(fill.begin(),fill.end(),WILD);
}

// Is a string allowed for the given word?  If it's been pitched, then
// obviously not.  Otherwise, if two letters in the word have to be
// the same (because they're the same letter in the puzzle!), enforce
// that.

bool word::is_allowed(const string &s) const
{
  if (find(pitched.begin(),pitched.end(),s) != pitched.end()) return false;
  for (auto &r : repeated_squares)
    for (size_t j = 1 ; j < r.size() ; ++j) if (s[r[0]] != s[r[j]]) return false;
  return true;
}

// Is the clue for a word empty?  Yes, if it's just spaces, - and _.

bool basic_word::clueless() const
{
  return clue.find_first_not_of(" -_") == string::npos;
}

// A puzzle is illegal if it's got an entry longer than the allowed
// maximum.

bool puzzle::illegal() const
{
  if (xymax.x > BIG_SIZE || xymax.y > BIG_SIZE) return true;
  return find_if(begin(),end(),
                 [](const word &w) { return w.size() > LONG_WORD; }) != end();
}

// Set the fill for a word.  Not just this word -- you also have to
// set individual letters in the crossing words, and in the squares
// themselves.

void puzzle::set_fill(word &wd, const string &f)
{
  wd.fill = f;
  for (size_t i = 0 ; i < wd.size() ; ++i) {
    wd[i].fill = f[i];
    for (size_t j = 0 ; j < wd[i].word_ids.size() ; ++j) {
      unsigned xid = wd[i].word_ids[j];
      if (xid != wd.id) (*this)[xid].fill[wd[i].positions[j]] = f[i];
    }
  }
}

// Get the fill for display.  Splice the words in, replacing WILD with
// '?' in the fills.  Mistakes are in upper case.

string skeleton::compute_fill(bool upper, char wild) const
{
  string ans = fill;
  for (auto &ch : ans)
    if (ch == WILD) ch = wild;
    else if (upper) ch = toupper(ch);
    else ch = tolower(ch);
  return ans;
}

// Reconstruct the fill as a 2-D array (used for writing Across Lite
// files back out).  Here, we deal with a single word, making any
// incorrect char upper case.

void basic_word::find_fill(char f[BIG_SIZE][BIG_SIZE]) const
{
  for (size_t j = 0 ; j < size() ; ++j) {
    char c = '?';
    if (fill[j] != WILD) {
      c = fill[j];
      if (answer.size() && answer[j] != fill[j]) c = toupper(c);
    }
    f[(*this)[j].x][(*this)[j].y] = c;
  }
}

// And here we do the whole frame, one word at a time.  Everything is
// initially set to WILD, just in case some squares aren't part of any
// word.

void puzzle::compute_fill(char fill[BIG_SIZE][BIG_SIZE]) const
{
  memset(fill,WILD,BIG_SIZE * BIG_SIZE);
  for (auto &w : *this) w.find_fill(fill);
}

// To output a puzzle, output the fill, which is computed by
// compute_fill.

void puzzle::show_fill(char f[BIG_SIZE][BIG_SIZE], ostream &os) const
{
  for (size_t i = 0 ; i < xymax.y ; ++i) {
    for (size_t j = 0 ; j < xymax.x ; ++j) 
      os << ((f[j][i] == WILD)? '.' : f[j][i]);
    os << endl;
  }
}

// Print out a puzzle.

ostream &operator<< (ostream &os, const puzzle &p)
{
  char fill[BIG_SIZE][BIG_SIZE];
  p.compute_fill(fill);
  p.show_fill(fill,os);
  return os;
}

// Is this rebus_square a factor in the given skeleton?

bool skeleton::rebus_candidate(const rebus_square &sq) const
{
  return find_if(begin(),end(),[&](const square *s) { return *s == sq; }) !=
    end();
}

// Simple (i.e., original) words associated with a given word (which
// may be a mutiple word, as in "with 33-across" etc, or in the
// geometry case.  This function is used for the GUI only, so can be
// quite slow.  Just break into pieces and find each.  If you can't
// find it (diagonal, what have you), just skip it.

vector<unsigned> puzzle::get_simple_words(const word &w) const
{
  vector<unsigned> ans;
  for (auto &sp : w.splits) {
    bool acr = (w[sp].y == w[1 + sp].y);
    for (unsigned j = 0 ; j < simple_words.size() ; ++j) {
      const simple_word &sw = simple_words[j];
      if (sw.start == w[sp] && sw.across == acr) {
        ans.push_back(j);
        break;
      }
    }
  }
  return ans;
}

// GUI stuff.  Basic story is that if this process outputs a line like
//  **GUI xxx ...
// the GUI picks up that line and processes it in some useful way.
// Further details are in gui.cpp and related files.

// Basic info to pass a word: send the word as id, x and y, and
// length, then number and across or down.  Then send the clue with
// id.  Then the answer (if known) and the crossers.

// Note that this function is called before the analysis begins, so
// that all the words are still "simple".

void basic_word::gui_info() const
{
  cout << "**GUI word " << id << ' ' << (*this)[0].x << ' ' << (*this)[0].y
       << ' ' << size() << ' ' << numbers[0] << ' ' << across() << endl;
  cout << "**GUI clue " << id << ' ' << clue << endl;
  if (answer.size()) cout << "**GUI answer " << id << ' ' << answer << endl;
  cout << "**GUI crossers " << id;
  for (auto sq : *this) {
    unsigned xid = -1;
    for (auto i : sq->word_ids) if (i != id) { xid = i; break; }
    cout << ' ' << xid;
  }
  cout << endl;
}

// index of symmetrically placed word

unsigned puzzle::opposite(const word &w) const
{
  if (w.splits.size() > 1) return -1; // too confusing!
  location l = w[0];
  if (w.across()) l.x += w.size() - 1;
  else l.y += w.size() - 1;
  location opp = xymax - (l + location(1,1));
  for (auto &other : *this)
    if (other[0] == opp && other.splits.size() == 1 &&
        other.across() == w.across())
      return other.id; 
  return -1;                    // shouldn't get here!
}

// Request a render: pass the source, then size, then all of the words
// as above.  Then the black squares.  Then you can finally request
// the render!

void puzzle::request_render() const
{
  cout << "**GUI source " << source;
  if (!title.empty()) cout << ": " << title;
  if (!author.empty()) {
    unsigned nonsp = 0;
    while (nonsp < author.size() && author[nonsp] == ' ') ++nonsp;
    string a = author.substr(nonsp);
    while (a.back() == ' ') a.pop_back();
    cout << " (" << a << ')';
  }
  cout << endl;
  cout << "**GUI size " << xymax.x << ' ' << xymax.y << ' ' << size() << endl;
  for (auto &w : *this) w.gui_info();
  cout << "**GUI blacks " << black_squares.size();
  for (auto &bs : black_squares) cout << ' ' << bs.x << ' ' << bs.y;
  cout << endl;
  cout << "**GUI render 1" << endl;
}

// Show changes in the state of the fill.  Last value is in oldfill.
// Assuming that he wants the display, compare and, if you find a
// difference, clear the existing pink cells and set out the new ones.

void search_node::show_changes(bool display) const
{
  static char oldfill[BIG_SIZE][BIG_SIZE];
  char newfill[BIG_SIZE][BIG_SIZE];
  compute_fill(newfill);
  if (display) {
    bool change = false;
    for (size_t i = 0 ; i < xymax.x ; ++i)
      for (size_t j = 0 ; j < xymax.y ; ++j)
        if (oldfill[i][j] != newfill[i][j]) {
          if (!change) cout << "**GUI clear_pink" << endl;
          change = true;
          cout << "**GUI pink " << i << ' ' << j << endl;
        }
  }
  memcpy(&oldfill[0][0],&newfill[0][0],BIG_SIZE * BIG_SIZE);
}

// Show the state of the puzzle.  Show the changes most of the time,
// pass the current fill, and then request the display.

void search_node::request_display(bool show_dots, bool finished) const
{
  if (show_dots) show_changes();
  for (auto &w : *this) {
    const string &f = w.fill;
    vector<unsigned> sw = get_simple_words(w);
    float sc = get_fill(w,f).score;
    for (size_t j = 0 ; j < sw.size() ; ++j)
      cout << "**GUI fill " << sw[j] << ' '
           << ((j + 1 == w.splits.size())? f.substr(w.splits[j]) : 
               f.substr(w.splits[j],w.splits[1 + j] - w.splits[j]))
           << ' ' << sc << endl;
  }
  cout << "**GUI display " << show_dots << ' ' << finished << endl;
}
