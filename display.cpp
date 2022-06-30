#include "xw.h"

// Display functions are generally obvious walkthroughs of the
// associated data structures.

ostream &operator<<(ostream &os, Transition t)
{
  static const vector<string> transition_strings {
    "make_plural", "make_single",
      "make_nonroot_adj", "make_root_adj",
      "make_participle", "make_past", "make_non_participle",
      "make_single_non_participle", "make_past_non_participle",
      "make_plural_participle", "make_plural_past", "make_present",
      "make_present_single", "make_present_participle",
      "make_more", "make_most", "make_const_adj", "make_no_compare",
      "make_no_const_adj", "make_syn"
      };
  return os << transition_strings[int(t)];
}

// pdata are displayed in order of likely POS.

ostream &operator<<(ostream &os, const pdata &p)
{
  os << p.ct;
  if (!p.ct) return os;
  os << " [";
  multimap<float,unsigned> ranked;
  for (unsigned i = 0 ; i < END_POS ; ++i)
    if (p.pos[i]) ranked.insert(make_pair(- p.pos[i],i));
  for (auto &p : ranked) {
    if (&p != &*ranked.begin()) os << ' ';
    os << pos_names[p.second] << " (" << - p.first << ')';
  }
  return os << ']';
}

ostream &operator<<(ostream &os, const cdata &c)
{
  os << c.ct;
  for (auto &i : c.fills) os << ' ' << i.first << ' ' << i.second;
  return os;
}

ostream &operator<<(ostream &os, const clue_slot &c)
{
  return os << c.clue << " (" << c.len << ')';
}

ostream &operator<<(ostream &os, const basic_datum &d)
{
  os << d.clue << ':' << d.pos << ':' << d.dict << ':';
  if (d.berkeley != BIG_CHARGE) os << d.berkeley;
  else os << '*';
  os << ':' << (d.paren? 'P' : '.') << (d.fitb? 'F' : '.') << (d.moby? 'M' : '.')
     << (d.exactp? 'E' : '.');
  if (d.abbr) os << ':' << d.fabbr;
  if (d.score != BIG_CHARGE) os << " -> " << d.score;
  return os;
}

ostream &operator<<(ostream &os, const scored_word &s)
{
  return os << s.word << ' ' << (basic_datum) s;
}

ostream &operator<<(ostream &os, const vector<char> &v)
{
  for (size_t i = 0 ; i < v.size() ; ++i) os << (i? "," : "") << (int) v[i];
  return os;
}

ostream &operator<<(ostream &os, const vector<unsigned char> &v)
{
  for (size_t i = 0 ; i < v.size() ; ++i) os << (i? "," : "") << (int) v[i];
  return os;
}

ostream &operator<<(ostream &os, const map<int,float> &c)
{
  for (auto &i : c) os << i.first << " : " << i.second << endl;
  return os;
}

ostream &operator<<(ostream &os, const map<clue_slot,cdata> &c)
{
  for (auto &i : c) os << i.second << " : " << i.first.clue << endl;
  return os;
}

ostream &operator<<(ostream &os, const multimap<float,string> &m)
{
  for (auto &i : m)
    os << (&i == &*m.begin()? "" : " ") << i.second << " (" << i.first << ')';
  return os;
}

ostream &operator<<(ostream &os, const square &s)
{
  os << '(' << s.x << ' ' << s.y << ')';
  if (s.fill) os << s.fill;
  return os;
}

ostream &operator<<(ostream &os, const square *sq)
{ 
  return os << *sq;
}

ostream &operator<<(ostream &os, const segment &s)
{
  return os << '[' << s.first << ' ' << s.second << ']';
}

ostream &operator<<(ostream &os, const cgrid &grid)
{
  for (unsigned y = 0 ; y < grid.rows() ; ++y) {
    for (unsigned x = 0 ; x < grid.cols() ; ++x)
      cout << (grid[x][y]? grid[x][y] : '.');
    cout << endl;
  }
  return os;
}

ostream &operator<<(ostream &os, const boggle_string &s)
{
  return os << s.value << ' ' << string(s) << ' ' << s.squares[0] << ' '
            << s.descriptor() << " (" << geo_type(s) << ')';
}

ostream &operator<<(ostream &os, const location &l)
{
  return os << '(' << l.x << ' ' << l.y << ')';
}

istream &operator>>(istream &is, location &l)
{
  string x,y;
  is >> x >> y;
  if (y.empty()) return is;     // should be done
  l.x = stoi(x.substr(1));
  l.y = stoi(y);
  return is;
}

ostream &operator<<(ostream &os, const geo_type &g)
{
  if (!g.num_segs) return os << "---";
  os << g.num_segs;
  if (g.num_segs == 1) return os;
  if (g.reversed) os << '*';
  if (g.diagonal) os << 'D';
  if (g.jump) os << '=';
  else if (!g.overlap) os << '|';
  return os;
}

// Remove more than MAX_RESPONSE candidate fills based on a partial
// match.  It's a bit of a pain because they're stored by string; we
// have to store them all, sort by count, then keep the top n.  But we
// also don't prune in the middle of a particular count, lest our fill
// be biased alphabetically.  (This function is used to output a map
// from segment_slot to cdata, which is used to dump the statistics in
// the preprocessing phase, which is why the bias would matter.)

const cdata cdata::reduce() const
{
  if (fills.size() < MAX_RESPONSE) return *this;
  multimap<unsigned,string> xf;
  unsigned s = 0;
  for (auto &f : fills) {
    if (f.second == 1) ++s;
    xf.insert(make_pair(f.second,f.first));
  }
  cdata ans(ct,s);
  vector<pair<string,unsigned>> tmp;
  for (auto i = xf.rbegin() ; i != xf.rend() ; ++i) {
    tmp.push_back(make_pair(i->second,i->first));
    if (tmp.size() == MAX_RESPONSE) {
      unsigned next = (++i)->first;
      while (!tmp.empty() && tmp.back().second == next) tmp.pop_back();
      break;
    }
  }
  ans.fills = map<string,unsigned>(tmp.begin(),tmp.end());
  return ans;
}

ostream &operator<<(ostream &os, const branch &b)
{
  return os << b.letter << '@' << b.slot;
}

ostream & operator<<(ostream &os, const splitting &s)
{
  for (auto &i : s) os << i.first << ": " << i.second << endl;
  return os;
}

// Following are declared in xw.h

ostream & operator<<(ostream &os, const progress &p)
{
  return os << p.when << '/' << p.nodes << '/' << p.bad_word;
}

// Output a single word as follows:

// 1. Asterisk if fill is wrong (and it's filled)
// 2. Word and id
// 3. Either length or answer
// 4. Number in grid (e.g. 1-A)
// 5. Fill in upper case
// 6. Clue
// 7. List of pitched words, if any

ostream & operator<<(ostream &os, const word &w)
{
  string s = w.fill;
  for (auto &ch : s) ch = (ch == WILD)? '.' : toupper(ch);
  if (!w.answer.empty() && w.fill != w.answer && w.filled()) os << '*';
  os << "w" << w.id << " (";
  if (!w.answer.empty()) os << w.answer;
  else os << w.size();
  os << "):  " << w.desc() << ' ' << s << " [" << w.clue << "] ";
  if (!w.pitched.empty()) 
    os << "pitched " << w.pitched.size() << ": [" << w.pitched << "] ";
  return os;
}

// word description.  If it's a combination of multiple words from the
// puzzle, print them all out.

string basic_word::desc() const
{
  ostringstream str;
  for (unsigned i = 0 ; i < numbers.size() ; ++i) {
    if (i) str << '+';
    if (numbers[i] && splits.size() > i) {
      bool acr = ((*this)[splits[i]].y == (*this)[1 + splits[i]].y);
      str << numbers[i] << '-' << (acr? 'A' : 'D');
    }
    else str << "**";
  }
  return str.str();
}

ostream & operator<<(ostream &os, const choice &c)
{
  return os << "choice: slot " << c.word_slot << " and " 
            << ((c.pitch)? "pitch" : "fill") << ' ' << c.fill;
}

ostream & operator<<(ostream &os, const heuristic_value &hv)
{
  return os << hv.fill << " (" << hv.value << ')';
}

// Various theme-related display functions.

ostream &operator<<(ostream &os, const theme &t)
{
  if (t.sound) os << '*';
  switch (t.type) {
  case ADD_STRING: case REMOVE_STRING:
    os << ((t.type == ADD_STRING)? "ADD" : "REMOVE") << ' ';
    switch (t.str1[0]) {
    case '?': os << "string of length " << t.str1.size() << " @ BEGIN"; break;
    case '!': os << "string of length " << t.str1.size() << " @ END";   break;
    default:  os << t.str1;
    }
    break;
  case CHANGE_STRING: os << "CHANGE " << t.str1 << " to " << t.str2; break;
  case ANAGRAM:       os << "anagram"; break;
  case PORTMANTEAU:   os << "portmanteau"; break;
  case PUN:           os << "pun"; break;
  case NOTHEME:       os << (t.sound? "homonym" : "unthemed"); break;
  }
  return os;
}

ostream &operator<<(ostream &os, const sourced_theme &r)
{
  return os << r.sources << ": " << (theme) r;
}

ostream &operator<<(ostream &os, const possible_theme &p)
{
  for (auto &i : p) {
    for (auto &j : i) os << j << endl;
    os << endl;
  }
  return os;
}

// Display a theme_fill.  General format:

// original * -> psource ** -> [pfill ***]
// original * -> psource ** -> [no suggestion]

// where * is either empty or -> poriginal
// and **  is either empty or (source)
// and *** is either empty or -> fill

// so that the end of a theme_fill is always a ] and then you can
// parse it from there.

ostream &operator<<(ostream &os, const theme_fill &ts)
{
  os << ts.original;
  if (ts.original != ts.poriginal) os << " -> " << ts.poriginal;
  os << " -> " << ts.psource;
  if (ts.psource != ts.source) os << " (" << ts.source << ')';
  if (ts.pfill.size()) {
    os << " -> [";
    if (ts.pfill != ts.fill) os << ts.pfill << " -> ";
    os << ts.fill << ']';
  }
  else os << " [no suggestion]";
  return os;
}

istream &operator>>(istream &is, theme_fill &tf)
{
  vector<string> toks;
  string tok;
  while (is >> tok) {
    toks.push_back(tok);
    if (toks.back().back() == ']') break;
  }
  if (toks.size() < 5) return is;  // is this best?
  if (toks[toks.size() - 2] != "[no") {
    unsigned pfill = 0;
    while (toks[pfill][0] != '[') ++pfill;
    tf.pfill = toks[pfill].substr(1);
    if (tf.pfill.back() == ']') {
      tf.pfill.pop_back();
      tf.fill = tf.pfill;
    }
    else {
      tf.fill = toks.back();
      tf.fill.pop_back();
    }
    toks.resize(pfill);
  }
  else {
    tf.fill = tf.pfill = "";
    toks.resize(toks.size() - 2);
  }
  tf.original = toks[0];
  // now there are four possibilities.  In terms of number of tokens:
  // 3: original -> psource with poriginal = original and source = psource
  // 5: original -> poriginal -> psource with source = psource
  // 4: original -> psource (source) with poriginal = original
  // 6: original -> poriginal -> psource (source)
  switch(toks.size()) {
  case 3: tf.poriginal = tf.original;
    tf.source = tf.psource = toks[2];
    break;
 case 4: tf.poriginal = tf.original;
   tf.psource = toks[2];
   tf.source = toks[3].substr(1);
   tf.source.pop_back();
   break;
 case 5: tf.poriginal = toks[2];
   tf.source = tf.psource = toks[4];
   break;
 case 6: tf.poriginal = toks[2];
   tf.psource = toks[4];
   tf.source = toks[5].substr(1);
   tf.source.pop_back();
   break;
  }
  return is;
}

// Display a skeleton.  Show the squares it contains, and the fill if
// it's interesting (and legal).

ostream &operator<<(ostream &os, const skeleton &s)
{
  vector<square> squares(s.size());
  for (unsigned i = 0 ; i < s.size() ; ++i) squares[i] = s[i];
  bool show_fill = false;
  if (s.size() == s.fill.size()) 
    for (auto ch : s.fill) if (ch != WILD) { show_fill = true; break; }
  os << squares;
  if (show_fill) os << ": " << s.fill;
  return os;
}

// Display various elements of the rebus calculation

ostream &operator<<(ostream &os, const rebus_square &rs)
{
  return os << '[' << rs.x << ' ' << rs.y << ']' << (rs.down? 'd' : 'a');
}

ostream &operator<<(ostream &os, const rebus_value &v)
{
  return os << v.value << '(' << v.squares << ')';
}

ostream &operator<<(ostream &os, const rebus_answer &rb)
{
  os << (string) rb;
  if (rb.alternate.size()) os << '/' << rb.alternate;
  if (rb.sq.x != (unsigned) -1) os << "@(" << rb.sq.x << ' ' << rb.sq.y << ')';
  return os;
}

ostream &operator<<(ostream &os, const inex_base &i)
{
  return os << i[0] << ' ' << i[1] << ' ' << i[2] << ' ' << i[3];
}

// Simplest way I could think of for a mutex-protected write.  The
// protector either locks or unlocks the mutex without actually
// printing anything.

ostream &operator<<(ostream &os, const protector &p)
{
  static mutex pmex;
  if (p.locking) pmex.lock();
  else pmex.unlock();
  return os;
}

protector guard(true);
protector unguard(false);
