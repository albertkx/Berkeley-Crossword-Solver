#include "score.h"
#include "fitb.h"

// Various functions involved in establishing and using a database.
// Code that reads the database is first; functions built using the
// database follow.

// On reading, the various fields are initialized by the following
// functions:

// portmanteau_cutoff   read_mlg_words                                  1
// flamingo_cutoff      read_mlg_words                                  1
// mlg_tot              read_mlg_words                                  1
// mlg_size             read_mlg_words                                  1
// mlg_prob             read_mlg_fills                                  2
// word_to_score        read_mlg_words                                  1
// sound_to_score       read_mlg_words                                  1
// parses               read_pos                                        3
// pos_rules            read_pos_from_clue                              4
// first_pos            read_pos_from_pos                               5
// last_pos             read_pos_from_pos                               5
// exact                read_exact   [from read_clues]                  6a
// inexact              read_inexact [from read_clues]                  6b
// exact_probs          read_exact   [from read_clues]                  6a
// roots                read_clues                                      6
// constructs           read_clues                                      6
// is_abbr              read_exact   [from read_clues]                  6a
// multidicts[0]        read_words_for_multi                            1a
// multidicts[1 2]      read_mlg_words                                  1
// moby                 read_moby                                       7
// pronunciations       read_pronunciations                             8
// pron_to_words        read_pronunciations                             8
// flamingo_words       read_mlg_words                                  1
// flamingo_weights     [in flamingo.cpp: flamingo_build]
// flamingo             [in flamingo.cpp: flamingo_build]
// flamingo_cache       [in flamingo.cpp: flamingo_match]
// rebus_words          make_rebus_words                                9
// anagrams             make_anagrams                                   10

static size_t find_cutoff(const vector<unsigned> &scores, size_t limit)
{
  return (scores.size() > limit)? scores[scores.size() - limit] : scores[0];
}

void check_stream(const ifstream &str, const string &src)
{
  if (str) return;
  cerr << "cannot open stream for " << src << endl;
  assert(false);
}

// Build the trie for the geometry analysis.  Note that we don't write
// the trie to the binary files, so we need to rebuild it when we read
// them in.

void database::build_trie()
{
  word_trie.clear();
  for (size_t len = 1 + GEO_SHORT_LIMIT ; len <= LONG_WORD ; ++len)
    for (size_t j = 0 ; j < word_to_score.size(len) ; ++j)
      if (word_to_score.get_value(len,j) >= THEME_MULTI_CUTOFF)
        word_trie.add(word_to_score.get_key(len,j));
}

// DATABASE READ STEP 1
// Read the raw words in the dictionary.  First read the basic words,
// which go into multidict[0] and also are surely contained in
// multidict[1].  Then add some specific short words, which are
// generally scored poorly by the automated stuff.  Then read each
// line, which is a word and a score.  Keep track of the total number
// of words of a given size (mlg_tot), or of a given size and with a
// given score (mlg_size).  Compute the various cutoffs so that the
// right number of words are in each "group".

void database::read_mlg_words(const string &easyfile, const string &words)
{
  map<string,char> mlg_scores;
  vector<set<string>> md = read_words_for_multi(easyfile);
  static const string smalls[] = { 
    "a", "i", "o", "ad", "ah", "am", "an", "as", "at", "aw", "ax", "be", "by",
    "dc", "dj", "do", "dr", "ed", "ex", "gi", "go", "ha", "he", "hi", "id",
    "if", "im", "in", "is", "it", "jr", "lo", "ma", "me", "mr", "my", "no",
    "of", "oh", "ok", "on", "or", "ow", "ox", "pa", "so", "to", "tv", "up",
    "us", "we", "ye", "" 
  };
  vector<string> other_smalls;
  for (auto &sm : smalls) md[sm.size()].insert(sm);
  vector<pair<string,unsigned>> dictwords;
  vector<unsigned> all_scores;
  ifstream inf(words);
  check_stream(inf,words);
  string wrd;
  unsigned sc;
  while (inf >> wrd >> sc) {
    dictwords.push_back(make_pair(wrd,sc));
    all_scores.push_back(sc);
  }
  sort(all_scores.begin(),all_scores.end());
  portmanteau_cutoff  = find_cutoff(all_scores,PORTMANTEAU_COUNT);
  size_t multi_cutoff = find_cutoff(all_scores,MULTI_COUNT);
  flamingo_cutoff     = find_cutoff(all_scores,FLAMINGO_COUNT);
  size_t mlg_cutoff   = find_cutoff(all_scores,MLG_COUNT);
  cerr << "cutoffs: " << portmanteau_cutoff << ' ' << multi_cutoff << ' '
       << flamingo_cutoff << ' ' << mlg_cutoff << endl;
  for (auto &dw : dictwords) {
    const string &w = dw.first;
    unsigned sc = dw.second;
    if (sc < mlg_cutoff) continue;
    if (w.size() > LONG_WORD) continue;
    if (w.size() < 3) {
      if (sc >= THEME_MULTI_CUTOFF) other_smalls.push_back(w);
      continue;
    }
    mlg_scores[w] = sc;
    ++mlg_size[w.size()][sc];
    ++mlg_tot[w.size()];
    if (sc >= multi_cutoff) md[w.size()].insert(w);
    if (sc >= flamingo_cutoff) flamingo_words[0].push_back(w);
    if (sc >= THEME_MULTI_CUTOFF && w.size() > GEO_SHORT_LIMIT) word_trie.add(w);
  }
  multidicts[1] = md;
  if (NUM_MULTI_DICTS == 3) {
    for (auto &sm : other_smalls) md[sm.size()].insert(sm);
    multidicts[2] = md;
  }
  word_to_score = mlg_scores;
  cerr << "read " << mlg_scores.size() << " xw words (" << get_cpu_time()
       << " sec)" << endl;
}

// DATABASE READ STEP 1a
// Read the words used for the multiword split using basic English.
// Just go through and get them, one at a time.

vector<set<string>> database::read_words_for_multi(const string &words)
{
  ifstream inf(words);
  check_stream(inf,words);
  unsigned ct = 0;
  vector<set<string>> md(1 + LONG_WORD);
  string wrd;
  while (inf >> wrd) {
    if (wrd.size() > LONG_WORD) continue;
    md[wrd.size()].insert(wrd);
    ++ct;
  }
  multidicts[0] = md;
  cerr << "read " << ct << " words (" << get_cpu_time() 
       << " sec) for multiword base dictionary " << endl;
  return md;
}

// DATABASE READ STEP 2
// Read the fraction of time a word has a score for the various
// scores.  Just a small table, read into mlg_prob.

void database::read_mlg_fills(const string &filldata)
{
  ifstream inf(filldata);
  check_stream(inf,filldata);
  int sc;
  float p;
  while (inf >> sc >> p) mlg_prob[sc] = p;
  cerr << "read fill data (" << get_cpu_time() << " sec)" << endl;
}

// DATABASE READ STEP 3
// Read the parts of speech file.  Format is word, then a list of pos
// ids.  Just clunk through them.  parses includes the whole list.

void database::read_pos(const string &pos_file)
{
  ifstream inf(pos_file);
  check_stream(inf,pos_file);
  string line;
  unsigned word_count = 0;
  map<string,char> ignored;

  while (getline(inf,line)) {
    istringstream is(line);
    string w;
    unsigned p;
    vector<POS> v;
    is >> w;
    if (w.size() > LONG_WORD) continue;
    ++word_count;
    while (is >> p) v.push_back(POS(p));
    parts_of_speech[w] = v;
  }
  cerr << "read pos file (" << word_count << " known, " << get_cpu_time()
       << " sec)" << endl;
}

// DATABASE READ STEP 4
// Read the parse file.  Format of each line is:

// #appearances POS* -> (POS prob)*

// where the initial POS* is a list of the parts of speech appearing
// in the clue and the prob is enclosed in parentheses.  So for each
// line:
// 1. get the count
// 2. get the remaining strings in _tokens_ 
// 3. reconstruct the parse of the clue until you hit the ->
// 4. read the pdata and attach it to the clue parse

void database::read_pos_from_clue(const string &parsings)
{
  ifstream inf(parsings);
  check_stream(inf,parsings);
  string line;
  while (getline(inf,line)) {
    unsigned ct;
    string tok;
    istringstream is(line);
    is >> ct;                                                   // 1
    parse p;
    for ( ;; ) {
      is >> tok;
      if (tok == "->") break;                                   // 3
      p.push_back(pos_from_string(tok));                        // 2
    }
    pos_rules[p] = pdata(ct,is);                                // 4
  }
  cerr << "read clue parse file (" << get_cpu_time() << " sec)" << endl;
}

// DATABASE READ STEP 5
// Read the POS from position file, POS depending on first/last words
// of a clue.  Each line is of the format word p1 pos1 ... pn posn
// where pn is a probability and posn is the associated part of
// speech.  The POS's based on the first word are first, those based
// on the last word are last, and they're separated by a blank line.

void database::read_pos_from_pos(const string &fname)
{
  ifstream inf(fname);
  check_stream(inf,fname);
  string line;
  map<string,pdata> pff, pfl;
  map<string,pdata> *target = &pff; // reading to first or last?
  while (getline(inf,line)) {
    if (line.empty()) { target = &pfl; continue; }
    istringstream is(line);
    unsigned ct;
    string wrd;
    is >> ct >> wrd;
    (*target)[wrd] = pdata(ct,is);
  }
  first_pos = pff;
  last_pos  = pfl;
  cerr << "read pos from pos file (" << get_cpu_time() << " sec)" << endl;
}

// DATABASE READ STEP 6
// Read the clue stuff.  Start with the roots, then open the clue data
// file and read the stats stuff.  Format of the root file is
//  word | pos | root
// After that, we read the exact and inexact clue data.

void read_inexact(unsigned idx, ifstream &inf, vector<inex> &data);

void database::read_clues(const string &cluedata, const string &rootfile)
{
  ifstream inf(rootfile);
  check_stream(inf,rootfile);
  map<string,set<string>> c;    // root -> derivatives
  string base, root;
  while (inf >> base >> root)   // like aardvarks aardvark
    if (base != root) {
      roots[base] = root;
      c[root].insert(base);
    }
  cerr << "read " << roots.size() << " roots (" << get_cpu_time() << " sec)"
       << endl;
  for (auto &i : c)
    constructs[i.first] = vector<string>(i.second.begin(),i.second.end());
  ifstream cfile(cluedata);
  check_stream(cfile,cluedata);
  read_exact(cfile);
  read_inexact_words(cfile);
  for (size_t i = 1 ; i <= SEGMENTS ; ++i) read_inexact(i,cfile);
}

// DATABASE READ STEP 6A: EXACT CLUES
// Read the exact array, a map from clue_data to cdata.  Format of
// each line is

// #appearances (fill ct)* : clue

// where #appearances is the total number of appearances of this clue
// and the fill and count are obvious

// There is some extra work here, as we deal with the fact that some
// clues/fills are abbreviations and some aren't.  The data used:
//   cts[n]          = total # clues with n appearances
//   news[n]         = # singleton fills in clues with n appearances
//   abbrv_cts[word] = # abbrv clues, # non-abbrv clues

// What we're constructing is exact (map from clue to fill),
// exact_probs (map from int to float given p(correct) given # of clue
// appearances) and is_abbr (wordmap from clue word to probability
// that fill is abbreviation given appearance of word in clue)

void database::read_exact(ifstream &inf)
{
  vector<unsigned> cts, news;
  map<string,pair<unsigned,unsigned>> abbrv_cts;
  string line;
  for ( ;; ) {
    getline(inf,line);
    size_t p = line.find(':');
    if (p == string::npos) break; // end of exact data
    vector<string> toks = raw_tokenize(line.substr(0,p - 1)); 
                                // don't include : so last token is meaningful
    cdata cd(toks);             // build cdata
    // note that all the possible fills are now inside the cdata cd;
    // in the following code, we look at toks[1] because all we really
    // care about is the clue length
    string clue = line.substr(p + 2);
    unsigned len = toks[1].size();
    exact[clue_slot(clue,len)] = cd; // clue slot from clue and fill
    if (cd.ct >= cts.size()) {  // cts[n] = #clues w/ n appearances
                                // news[n] = #single answers
      cts.resize(1 + cd.ct,0);
      news.resize(1 + cd.ct,0);
    }
    ++cts[cd.ct];               // should be += cd.ct but correct for that below
    news[cd.ct] += cd.singletons;
    if (len > ABBR_LIM) continue;            // process abbreviation info if the
    bool abbrv_clue = shows_abbrv(clue);     // fill is short enough
    for (auto &i : cd.fills)
      if (abbrv_clue) abbrv_cts[i.first].first += i.second;
      else abbrv_cts[i.first].second += i.second;
  }

  for (size_t i = 0 ; i < cts.size() ; ++i)
    if (cts[i]) exact_probs[-i] = (float) news[i] / (i * cts[i]);
  // exact_probs[-n] = prob a solution to (a clue used n times) is unique
  // (use -n instead of n so that lower_bound does the right thing in score.cpp)
  // division by i corrects for incorrect sum as indicated above
  cerr << "read exact (" << exact.size() << " clues, " << get_cpu_time()
       << " secs)" << endl;
  map<string,float> abb[1 + LONG_WORD];
  for (auto &i : abbrv_cts)
    abb[i.first.size()][i.first] = 
      (i.second.first + 2) / (i.second.first + i.second.second + 2.0);
  is_abbr = abb;                // probability that the given word is an abbrv
}

// Here we figure out if a clue means that the answer is an
// abbreviation.  This helper function is called if a . is in the
// clue, but if the . is from one of the strings mentioned in _isnt_,
// it doesn't mean anything.

bool okabbrv(const string &str, unsigned idx)
{
  static const vector<string> isnt = 
    { "e.g.", "Mr.", "Mrs.", "Ms.", "No.", "no.", "vs.", "var.", "etc." };
  for (const string &i : isnt)
    if (i.size() <= 1 + idx && str.substr(idx + 1 - i.size(),i.size()) == i)
      return false;
  return true;
}

// And here is the code itself.  Analysis proceeds as follows:

// 1. Null strings do not correspond to abbreviations.
// 2. If there is a nonleading . that doesn't follow another . and is
//    either the end of the string or precedes a non-alpha character,
//    and this piece of the string is ok as an abbreviation marker,
//    return true.
// 3. If a known abbreviation string appears in the clue, return true.
// 4. If an abbreviation "marker" appears in the clue, either preceded by 
//    a comma or ending the clue, return true.
// 5. If an uppercase-only word appears in the string (other than "I" or "A"),
//    return true (ignore trailing 's' because it might be something like GIs).
// 6. Otherwise, return false.

bool shows_abbrv(const string &str)
{
  static const vector<string> abbrv_strings1 {
    "abbrv", "Abbrv", "Abbr.", "abbr." 
      };
  static const vector<string> abbrv_strings2 { 
    "for short", "shortly", "briefly", "in brief", "Abbr", "abbr" 
      };
  if (str.empty()) return false;
  for (size_t i = 1 ; i < str.size() ; ++i)
    if (str[i] == '.' && str[i - 1] != '.' && 
        (i == str.size() - 1 || (str[i + 1] != '.' && !isalpha(str[i + 1]))) &&
        okabbrv(str,i))
      return true;
  for (auto &a : abbrv_strings1) if (str.find(a) != string::npos) return true;
  for (auto &a : abbrv_strings2)
    if (str.find(", " + a) != string::npos || 
        (str.size() > a.size() && str.substr(str.size() - a.size()) == a)) 
      return true;

  istringstream iss(str);
  string tok;
  while (iss >> tok) {
    string alph;
    for (auto ch : tok) if (isalpha(ch)) alph.push_back(ch);
    if (alph.empty()) continue;
    if (alph == "I" || alph == "A" || alph == "Ms") continue;
    if (alph.size() > 1 && alph.size() < tok.size() && alph.back() == 's')
      alph.pop_back();
    if (none_of(alph.begin(),alph.end(),[](char x) { return islower(x); }))
      return true;
  }
  return false;
}

// No longer used, assuming statistics are stable

#if 0

// See how good the abbreviation stuff is.  Just go through the words
// and clues, seeing how accurate the abbreviation classification is.
// That means you find clues that look like they should/shouldn't mark
// abbreviations but are actually cluing words that *other* clues make
// clear are something else.  Then print out the list of
// mis-identified words, sorted by how sure you are that they're
// wrong.  This functionality is used to develop the abbreviation test
// and is probably not useful any more.  (It's been translated to
// C++-11 but not tested.)

struct abbrv_info {
  unsigned       yes, no;
  vector<string> yesstring, nostring;

  abbrv_info() : yes(0), no(0) { }
};

void analyze_abbrv(unsigned limit, const string &fname)
{
  map<string,abbrv_info> abbrv_cts;
  ifstream inf(fname);
  check_stream(inf,fname);
  string line;
  for ( ;; ) {
    getline(inf,line);
    size_t p = line.find(':');
    if (p == string::npos) break; // end of exact data
    vector<string> toks = raw_tokenize(line.substr(0,p - 1)); // ignore :
    cdata cd(toks);             // build cdata
    if (toks[1].size() > ABBR_LIM) continue; // process abbreviation info if the
    string clue = line.substr(p + 2);
    bool abbrv_clue = shows_abbrv(clue);     // fill is short enough
    for (auto &c : cd.fills) {
      abbrv_info &ai = abbrv_cts[c.first];
      if (abbrv_clue) {
        ai.yes += c.second;
        ai.yesstring.push_back(clue);
      }
      else {
        ai.no += c.second;
        ai.nostring.push_back(clue);
      }
    }
  }

  multimap<float,string> abb;
  for (auto &a : abbrv_cts)
    if (a.second.no && a.second.yes) {
      float f = (a.second.yes + 2) / (a.second.yes + a.second.no + 2.0);    
      abb.insert(make_pair(f,a.first));
    }
  auto i = abb.begin();
  for (unsigned j = 0 ; j < limit ; ++j) {
    if (++i == abb.end()) break;
    cout << i->second << " appears not to be an abbreviation (" << i->first
         << ") but has clues" << endl;
    for (size_t k = 0 ; k < abbrv_cts[i->second].yesstring.size() ; ++k)
      cout << ' ' << abbrv_cts[i->second].yesstring[k] << endl;
  }
  auto ii = abb.rbegin();
  for (unsigned j = 0 ; j < limit ; ++j) {
    if (++ii == abb.rend()) break;
    cout << ii->second << " appears to be an abbreviation (" << ii->first
         << ") but has clues" << endl;
    for (size_t k = 0 ; k < abbrv_cts[ii->second].nostring.size() ; ++k)
      cout << ' ' << abbrv_cts[ii->second].nostring[k] << endl;
  }
}

#endif

// Note: following code should really be removed, but there are
// switches to display this stuff and we're leaving those switches in
// just in case we want to repeat the analysis some day.

void database::show_abbrvs() const
{
  for (size_t i = 0 ; i <= LONG_WORD ; ++i)
    for (size_t j = 0 ; j < is_abbr.size(i) ; ++j)
      if (is_abbr.get_value(i,j))
        cout << is_abbr.get_key(i,j) << ' ' << is_abbr.get_value(i,j) << endl;
}

// DATABASE READ STEP 6B: INEXACT CLUES

// Read the inexact words and then an inexact array, a map from
// segment_data to cdata.  Format of each line is

// #appearances (root ct)* : root_index*

// where the part after the : is the clue segment.

// The inexact words are just a line at a time.

void database::read_inexact_words(ifstream &inf)
{
  string line;
  for ( ;; ) {
    getline(inf,line);
    if (line.empty()) break;
    inexact_words.push_back(line);
  }
  cerr << "read " << inexact_words.size() << " inexact words, " << get_cpu_time()
       << " secs)\n";
}

void database::read_inexact(unsigned idx, ifstream &inf)
{
  string line;
  inexact.resize(1 + SEGMENTS);
  for ( ;; ) {
    getline(inf,line);
    size_t p = line.find(':');
    if (p == string::npos) break; // end of section
    vector<string> toks = raw_tokenize(line.substr(0,p - 1));
    inex_base indices;
    unsigned idx = 0;
    for (auto &index : raw_tokenize(line.substr(p + 2)))
      indices[idx++] = stoi(index);
    inexact[idx][indices] = cdata(toks);
  }
  cerr << "read inexact[" << idx << "] (" << inexact[idx].size()
       << " clues, " << get_cpu_time() << " secs)\n";
}

// DATABASE READ STEP 7
// Read the moby file.  Each line is a list of comma-separated
// synonyms, and that's how you store it.

void database::read_moby(const string &name)
{
  ifstream inf(name);
  check_stream(inf,name);
  string line;
  while (getline(inf,line)) {
    vector<string> syns;
    size_t start = 0;
    for (size_t c = line.find(',') ;; c = line.find(',',start) ) {
      syns.push_back(remove_spaces(line.substr(start,c - start)));
      if (syns.back().empty()) syns.pop_back();
      if (c == string::npos) break;
      start = 1 + c;
    }
    sort(syns.begin(),syns.end());
    moby.push_back(syns);
  }
  cerr << "read moby" << " (" << moby.size() << " syn lists, " 
       << get_cpu_time() << " secs)" << endl;
}

// DATABASE READ STEP 8
// Build data needed to modify clues and produce transformed fills.

// Parse a clue line, pulling out the fill and the clue.

void parse_line(const string &line, string &fill, string &clue)
{
  fill = line.substr(0,line.find(' '));
  for (auto &ch : fill) ch = tolower(ch);
  clue = line.substr(CLUE_START);
  for (auto &ch : clue) ch = tolower(ch);
}

// Given a database, get the ID of a word, or -1 if it's not in the
// word list.

unsigned database::find_word(const string &str) const
{
  auto itr = ::lower_bound(words.begin(),words.end(),str);
  return (*itr == str)? (itr - words.begin()) : -1;
}

// Here is code designed to convert a clue to a "related" clue where
// one or more words in the clue has been replaced by something
// related to it via a transition (make singular, etc).  For a start,
// we need to reconstruct the modified clue as a string.

// Convert an ID back to a string.

string database::to_string(const vstring &v) const
{
  string ans;
  for (unsigned u : v) {
    ans += words[u];
    ans.push_back(' ');
  }
  ans.pop_back();
  return ans;
}

// Construct a vtransition_map from a transition_map simply by
// converting each string to its associated ID.  If there is no
// associated ID, just drop it.

// NOTE: A transition map maps strings to vectors of
// <string,Transition> pairs.  A vtransition map maps unsigneds to
// vectors of <unsigned,Transition> pairs.

vtransition_map::vtransition_map(const transition_map &m, const database &db)
{
  for (auto &p : m) {
    unsigned w = db.find_word(p.first);
    if (w == unsigned(-1)) continue;
    for (auto &p2 : p.second) { // p2 in vector<pair<string,Transition>>
      unsigned w2 = db.find_word(p2.first);
      if (w2 != unsigned(-1)) (*this)[w].push_back(make_pair(w2,p2.second));
    }
  }
}

// Invert a vtransition_map, so we can reverse a transition acting on
// a clue to get the original fill.

vtransition_map vtransition_map::reverse() const
{
  vtransition_map ans;
  for (auto &p : *this)
    for (auto &p2 : p.second)
      ans[p2.first].push_back(make_pair(p.first,p2.second));
  return ans;
}

// Get the transitions from the grammar.  It's pretty simple: we get
// all the transitions [get_transitions()] by looking at each word.
// We get the transitions for a single word [get_trasitions(from)] by
// getting all the derivatives of the root and examining each.  We get
// the transitions for a pair [get_transitions(from,to)] by looking at
// all the transitions mapping a POS of from to a POS of to.

vector<Transition> grammar::get_transitions(const string &from, const string &to)
  const
{
  vector<Transition> ans;
  for (POS inpos : parts_of_speech.at(from))
    for (POS outpos : parts_of_speech.at(to))
      if (transitions.find(make_pair(inpos,outpos)) != transitions.end()) {
        ans.push_back(transitions.at(make_pair(inpos,outpos)));
      }
  return ans;
}

vector<pair<string,Transition>> grammar::get_transitions(const string &from)
               const
{
  vector<pair<string,Transition>> ans;
  string root = from;
  if (roots.find(from) != roots.end()) root = roots.at(from);
  vector<string> to_vec(1,root);
  if (constructs.find(root) != constructs.end()) {
    const vector<string> &tmp = constructs.at(root);
    to_vec.insert(to_vec.end(),tmp.begin(),tmp.end());
  }
  for (const string &to : to_vec)
    if (from != to && parts_of_speech.find(to) != parts_of_speech.end())
      for (Transition t : get_transitions(from,to))
        ans.push_back(make_pair(to,t));
  return ans;
}

transition_map grammar::get_transitions() const
{
  transition_map ans;
  for (auto &p : parts_of_speech) {
    vector<pair<string,Transition>> t = get_transitions(p.first);
    ans[p.first].insert(ans[p.first].end(),t.begin(),t.end());
  }
  return ans;
}

// And here we build the translation database.  It involves five steps:

// 1. Read all the clues and associated fills.  Build a map _clues_
//    from fill to a vector of clues, where each clue is a vector of
//    clue words.
// 2. Find all the words in the fills or the clues.
// 3. Convert the clue->fill map from string->string to
//    vstring->unsigned based on ID.  Build clues_by_id, just like
//    _clues_ but with unsigneds instead of strings.
// 4. Build the translations from the vstring->unsigned map.
// 5. Build and reverse the transitions for easy access later.

void database::process_clues(const string &cluefile)
{
  string line;                                                          // 1
  ifstream inf(cluefile);
  map<string,vector<vector<string>>> clues;
  string fill, clue;
  while (getline(inf,line)) {
    parse_line(line,fill,clue);
    clues[fill].push_back(raw_tokenize(clue));
  }
  cerr << "clues read (" << get_cpu_time() << " sec)" << endl;
  set<string> swords;                                                   // 2
  for (auto &p : clues) {
    swords.insert(p.first);
    for (auto &v : p.second) for (auto &w : v) swords.insert(w);
  }
  words = vector<string>(swords.begin(),swords.end());
  cerr << "words found (" << get_cpu_time() << " sec)" << endl;
  map<unsigned,vector<vstring>> clues_by_id;                            // 3
  for (auto &p : clues)
    for (auto &c : p.second) {
      vstring v;
      for (auto &w : c) v.push_back(find_word(w));
      clues_by_id[find_word(p.first)].push_back(v);
    }
  for (auto &p : clues_by_id) { // unique clue for each fill
    sort(p.second.begin(),p.second.end());
    auto itr = unique(p.second.begin(),p.second.end());
    p.second.erase(itr,p.second.end());
  }
  cerr << "clues converted to ints (" << get_cpu_time() << " sec)" << endl;
  for (auto &p : clues_by_id) {                                         // 4
    vstring f;
    f.push_back(p.first);       // translations are clue to clue; only
                                // tricky bit is that one-word clues
                                // are treated symmetrically
    for (auto &c : p.second)
      switch (c.size()) {
      case 0: break;
      case 1: translations[f].push_back(c); // fall through
      default: translations[c].push_back(f);
      }
  }
}

void database::make_transitions(const string &cluefile)
{
  process_clues(cluefile);
  transitions = vtransition_map(get_transitions(),*this);               // 5
  reversals = transitions.reverse();                     
  cerr << "transitions found/reversed (" << get_cpu_time() << " sec)" << endl;
}

// DATABASE READ STEP 9
// Fill the pronunciation database, given a file of possible words.
// Remove spaces in the words being processed.

// Here we read already-known pronunciations from the CMU database.

void read_pronunciations(const string &pronfile,
                         map<string,vector<string>> &from_word,
                         map<string,vector<string>> &to_word)
{
  ifstream inf(pronfile);
  check_stream(inf,pronfile);
  string line;
  bool go = false;
  map<string,set<string>> set_from_word, set_to_word;
  while (getline(inf,line)) {
    if (!go) go = (line.find("Main Dictionary") != string::npos);
    if (!go) continue;
    size_t pos = line.find("  ");
    if (pos == string::npos) continue;
    string wd = fill_no_digits(line.substr(0,pos));
    if (wd.empty()) continue;
    string pron = pstring_to_string(line.substr(pos + 2));
    set_from_word[wd].insert(pron);
    set_to_word[pron].insert(wd);
  }
  for (auto &p : set_from_word)
    from_word[p.first] = vector<string>(p.second.begin(),p.second.end());
  for (auto &p : set_to_word)
    to_word[p.first] = vector<string>(p.second.begin(),p.second.end());
  cerr << "CMU read complete (" << get_cpu_time() << " sec)" << endl;
}

// In order to keep the number of pronunciations modest, we want to
// use any known segmentation for phrases that have been collapsed to
// single words.  This uses the original set of Google queries as a
// base, and records the segmentation(s) that appear there.  One
// caveat -- the query set includes nonsense like "H E A V Y M E T A
// L" so if the average word length is < 2 characters, we ignore it.

map<string,vector<segmentation>> find_spaces(const string &spacefile)
{
  cout << "finding spaces in multiwords\n";
  ifstream inf(spacefile);
  check_stream(inf,spacefile);
  string line, tok;
  map<string,set<segmentation>> spaces;
  while (getline(inf,line)) {
    istringstream iss(line);
    segmentation segs;
    string nospaces;
    bool failed = false;
    while (iss >> tok) {
      if (!strip_spaces(tok)) { failed = true; break; }
      nospaces += tok;
      segs.push_back(tok.size());
    }
    if (failed || segs.size() == 1) continue;
    if (2 * segs.size() > nospaces.size()) continue; // avg word length < 2
    spaces[nospaces].insert(segs);
  }
  map<string,vector<segmentation>> ans;
  for (auto &p : spaces)
    ans[p.first] = vector<segmentation>(p.second.begin(),p.second.end());
  cout << "found spaces in multiwords\n";  
  return ans;
}

// Get all the pronunciations.  from_word is a map from words to
// vectors of possible pronunciations; to_word is a map from
// pronunciations to vectors of possible words.

// Begin by reading the CMU file.  Then store that in pronunications,
// since get_pronunciations needs it.  Then go through all the words
// in the dictionary file.  For each one that's worth processing, we
// get the word itself and see if we already have it.  If so, we do
// nothing more.

// In the interesting case, get_pronunciations does the work and we
// process each result returned.  Note that we also map each sound to
// the maximum score of an associated word.

// In reality, it's much more complicated than this.  The reason is
// that if we don't know how to split a multiword, we wind up
// generating a ton of possible pronunciations and flamingo really
// slows down both because the database of pronunciations is big.  So
// for something for which we don't have a known pronunciation, there
// are two possibilities:

// 1. If we can find it split in the "spaces" file, we use that
// splitting, or

// 2. If there is a unique splitting, we use that, or

// 3. If we can't, we pass the whole thing to flite and just take what
// comes back.

void database::read_pronunciations(const string &dict, const string &pronfile,
                                   const string &spacefile)
{
  cerr << "building pronunciations (" << get_cpu_time() << " sec)" << endl;
  map<string,vector<string>> from_word, to_word;
  ::read_pronunciations(pronfile,from_word,to_word);
  pronunciations = from_word;
  ifstream inf(dict);
  check_stream(inf,dict);
  string line;
  unsigned ct = 0;
  map<string,vector<segmentation>> segs = find_spaces(spacefile);
  map<string,char> sound_scores;
  while (getline(inf,line)) {
    size_t pos = line.find(';');
    if (++ct % 100000 == 0) cout << line.substr(0,pos) << endl;
    if (stoi(line.substr(1 + pos)) < flamingo_cutoff) continue;
    unsigned adjlen = 0;
    for (unsigned j = 0 ; j < pos ; ++j)
      if (isalpha(line[j]) || line[j] == ' ') line[adjlen++] = line[j];
    string str = line.substr(0,adjlen);
    if (isspace(str.back())) str.pop_back();
    string orig = str;
    strip_spaces(str); // mostly convert to lower case and stop at nonalpha
    if (str.empty() || str.size() > LONG_WORD) continue;
    if (from_word.find(str) != from_word.end()) continue; // repeated word
    const string &tosay = multidicts[0].includes(str)? str : orig;
    vector<string> prons;
    if (segs.find(tosay) != segs.end())
      prons = get_pronunciations(tosay,false,true,segs.at(tosay));
    else prons = get_pronunciations(tosay,true,true);
    from_word[str] = prons;
    for (auto &p : prons) if (p.size() <= LONG_WORD) {
        to_word[p].push_back(str);
        sound_scores[p] = max(sound_scores[p],char(stoi(line.substr(1 + pos))));
      }
  }
  pronunciations = from_word;
  pron_to_words = to_word;
  sound_to_score = sound_scores;
  cerr << " ... complete! (" << from_word.size() << " pronunciations, (" 
       << get_cpu_time() << " sec)" << endl;
}

// DATABASE READ STEP 10
// Allowed rebus possibilities.  Currently, all two-letter combos,
// 3-letter words in the big dictionary, and small dictionary entries
// up to the maximum allowed rebus length.  Also compute the number of
// Flamingo words in which the rebus words appear, which is valuable
// in figuring out if a particular rebus combo is showing up because
// it's real or accidentally.

void database::make_rebus_words()
{
  vector<string> ans;
  string w2 = "  ";
  for (w2[0] = 'a' ; w2[0] <= 'z' ; ++w2[0])
    for (w2[1] = 'a' ; w2[1] <= 'z' ; ++w2[1])
      ans.push_back(w2);
  for (unsigned j = 0 ; j < multidicts[1].size(3) ; ++j)
    ans.push_back(multidicts[1].getword(j,3));
  for (unsigned i = 4 ; i <= REBUS_MAX_LENGTH ; ++i)
    for (unsigned j = 0 ; j < multidicts[0].size(i) ; ++j)
      ans.push_back(multidicts[0].getword(j,i));

  set<string> s(ans.begin(),ans.end());
  unsigned ict = 0;
  for (auto &wrd : s) {
    unsigned ct = 0;
    for (size_t j = 0 ; j < flamingo_words[0].size() ; ++j) {
      if (flamingo_words[0][j].find(wrd) != string::npos) ++ct;
    }
    rebus_words[wrd] = (float) ct / flamingo_words[0].size();
    if (++ict % 100 == 0) cout << wrd << " rebus count " << ct << endl;
  }
}

// DATABASE READ STEP 11

// Here we build a database that allows us to decide quickly if a fill
// is an anagram.  For any string, we have a hash function that will
// return identical values for all strings that are anagrams of one
// another.  Then for a given string, we basically build a hash table
// where for a given hash, the entry is a vector of 64 * pos + len,
// where len is the len of a particular entry and pos is the position
// of the entry in mlg_scored_words.  Of course, we only need include
// one such entry for each anagram (we appear to only want to ever
// know if fill *is* an anagram).  So before we push a new entry into
// the hash table, we check to see if we're working on a known anagram
// for something already there.

// Hash function.  Each letter is mapped to the nth prime, where a is
// 0, etc.  The hash function is the product of the primes
// corresponding to all the letters, modded out by a prime that's
// around a million.

int anagram_hash(const string &str)
{
  int hash = 1;
  static int primes[28] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
                            31,37,41,43,47, 53, 59, 61, 67, 71,
                            73,79,83,89,97,101,103,107 };
  for (char c : str) {
    assert(c >= 'a' && c <= 2 + 'z'); // sound-based have two more characters
    hash *= primes[c - 'a'];
    if (hash > BIG_PRIME) hash %= BIG_PRIME;
  }
  return hash;
}

// Check to see if fill is an anagram.  This actually returns 0 if
// it's an anagram, and the hash if it isn't.  Saves having to hash it
// twice.

unsigned database::is_not_anagram(const string &fill, bool sound) const
{
  int h = anagram_hash(fill);
  const vector<unsigned> &possibles = anagrams[sound][h];
  vector<char> sfill;
  for (unsigned i : possibles)
    if ((i & 63) == fill.size()) { // check lengths match
      if (sfill.empty()) {         // build sorted version of _fill_ greedily
        sfill = vector<char>(fill.begin(),fill.end());
        sort(sfill.begin(),sfill.end());
      }
      const string &poss =
        (sound? sound_to_score : word_to_score).get_key(i & 63,i >> 6);
      vector<char> sposs(poss.begin(),poss.end());
      sort(sposs.begin(),sposs.end());
      if (sposs == sfill) return 0; // it's a match!
    }
  return h;                     // return hash value if no match
}

// Build all the anagrams.  As you walk through them, only consider
// words of length at least SMALL_ANAGRAM and only words with score at
// least ANAGRAM_CUTOFF.

void database::make_anagrams(bool sound)
{
  const wordmap<char> &scores = sound? sound_to_score : word_to_score;
  anagrams[sound].clear();
  anagrams[sound].resize(BIG_PRIME);
  for (unsigned len = SMALL_ANAGRAM ; len <= LONG_WORD ; ++len) {
    unsigned lim = scores.size(len);
    for (unsigned i = 0 ; i < lim ; ++i)
      if (scores.get_value(len,i) >= ANAGRAM_CUTOFF) {
        const string &key = scores.get_key(len,i);
        unsigned hash = is_not_anagram(key,sound);
        if (hash) anagrams[sound][hash].push_back(i * 64 + len);
      }
  }
}

// Database construction.

void database::build(const string &mlg_words, const string &filldata,
                     const string &pos_file, const string &parsings,
                     const string &pos_pos, const string &cluedata,
                     const string &rootfile, const string &easyfile,
                     const string &mobyfile, const string &dictfile,
                     const string &pronfile, const string &spacefile,
                     const string &cluefile)
{
  memset(mlg_tot,0,(LONG_WORD + 1) * sizeof(unsigned));
  memset(mlg_prob,0,100 * sizeof(float));
  memset(mlg_size,0,100 * (1 + LONG_WORD) * sizeof(unsigned));
  read_mlg_words(easyfile,mlg_words);
  read_mlg_fills(filldata);
  read_pos(pos_file);
  read_pos_from_clue(parsings);
  read_pos_from_pos(pos_pos);
  read_clues(cluedata,rootfile);
  read_moby(mobyfile);
  make_transitions(cluefile);
  read_pronunciations(dictfile,pronfile,spacefile);
  flamingo_build();
  make_rebus_words();
  make_anagrams(false);
  make_anagrams(true);
  cerr << "hashing complete\n";
  cerr << "data read complete (" << get_cpu_time() << " secs)" << endl;
}

// Initialize the FITB stuff.

void database::initialize_fitb(const string &dir)
{
  static const vector<fitb_files> blank_files {
    { "aoff",  "aind",  "allworks.txt"    },
    { "boff",  "bind",  "bartlett.txt"    },
    { "soff",  "sind",  "shakespeare.txt" },
    { "wtoff", "wtind", "wikititles.txt"  },
    { "doff",  "dind",  "dict.txt"        }
  };

  blank_filler = new fitb_db(thread::hardware_concurrency(),blank_files,dir);
}

database::~database()
{
  delete blank_filler;
}

// When you initialize a grammar, set up all the transition types.

grammar::grammar()
{
  transitions[make_pair(ROOT_NOUN,PLURAL_NOUN)] = PLURAL;
  transitions[make_pair(PLURAL_NOUN,ROOT_NOUN)] = SINGLE;
  transitions[make_pair(ROOT_ADVERB,NONROOT_ADVERB)] = NONROOT_ADJ;
  transitions[make_pair(NONROOT_ADVERB,ROOT_ADVERB)] = ROOT_ADJ;
  transitions[make_pair(ROOT_VERB,PARTICIPLE_VERB)] = PARTICIPLE;
  transitions[make_pair(ROOT_VERB,SINGULAR_VERB)] = SINGLE;
  transitions[make_pair(ROOT_VERB,PAST_VERB)] = PAST;
  transitions[make_pair(PARTICIPLE_VERB,ROOT_VERB)] = NON_PARTICIPLE;
  transitions[make_pair(PARTICIPLE_VERB,SINGULAR_VERB)] = SINGLE_NON_PARTICIPLE;
  transitions[make_pair(PARTICIPLE_VERB,PAST_VERB)] = PAST_NON_PARTICIPLE;
  transitions[make_pair(SINGULAR_VERB,ROOT_VERB)] = PLURAL;
  transitions[make_pair(SINGULAR_VERB,PARTICIPLE_VERB)] = PLURAL_PARTICIPLE;
  transitions[make_pair(SINGULAR_VERB,PAST_VERB)] = PLURAL_PAST;
  transitions[make_pair(PAST_VERB,ROOT_VERB)] = PRESENT;
  transitions[make_pair(PAST_VERB,SINGULAR_VERB)] = PRESENT_SINGLE;
  transitions[make_pair(PAST_VERB,PARTICIPLE_VERB)] = PRESENT_PARTICIPLE;
  transitions[make_pair(ROOT_ADJECTIVE,MORE_ADJECTIVE)] = MORE;
  transitions[make_pair(ROOT_ADJECTIVE,MOST_ADJECTIVE)] = MOST;
  transitions[make_pair(ROOT_ADJECTIVE,CONSTRUCTED_ADJECTIVE)] = CONST_ADJ;
  transitions[make_pair(MORE_ADJECTIVE,ROOT_ADJECTIVE)] = NO_COMPARE;
  transitions[make_pair(MORE_ADJECTIVE,MOST_ADJECTIVE)] = MOST;
  transitions[make_pair(MORE_ADJECTIVE,CONSTRUCTED_ADJECTIVE)] = CONST_ADJ;
  transitions[make_pair(MOST_ADJECTIVE,ROOT_ADJECTIVE)] = NO_COMPARE;
  transitions[make_pair(MOST_ADJECTIVE,MORE_ADJECTIVE)] = MOST;
  transitions[make_pair(MOST_ADJECTIVE,CONSTRUCTED_ADJECTIVE)] = CONST_ADJ;
  transitions[make_pair(CONSTRUCTED_ADJECTIVE,ROOT_ADJECTIVE)] = NO_CONST_ADJ;
  transitions[make_pair(CONSTRUCTED_ADJECTIVE,MORE_ADJECTIVE)] = MORE;
  transitions[make_pair(CONSTRUCTED_ADJECTIVE,MOST_ADJECTIVE)] = MOST;
  for (unsigned i = 0 ; i < unsigned(END_POS) ; ++i)
    transitions[make_pair(POS(i),POS(i))] = SYNONYM;
}

// Read files with standard names out of a given directory.  Bools
// indicate read or write the binary files, also can pass in the
// desired ngram limit, which is passed into the parameter set.
// verbose is passed to the binary read version.

enum { NO_SPACES, FILLSTATS, POS_FROM_WORD, PARSINGS, POS_FROM_POS, STATS, ROOTS,
       BASIC, DICTIONARY, PRON_FILE, SPACE_FILE, CLUE_FILE,
       DB, FLAMINGO, PFLAMINGO, MOBYF, FITB_DIR, NFILES };

database::database(const string &flist, bool readit, bool dumpit, bool verbose,
                   bool gui)
{
  vector<string> fnames(NFILES);
  ifstream f(flist);
  check_stream(f,flist);
  string line;
  for (auto &fn : fnames) {
    getline(f,line);
    istringstream(line) >> fn;
  }

  if (readit) read(fnames[DB],fnames[FLAMINGO],fnames[PFLAMINGO],verbose,gui);
  else {
    build(fnames[NO_SPACES],fnames[FILLSTATS],fnames[POS_FROM_WORD],
          fnames[PARSINGS],fnames[POS_FROM_POS],fnames[STATS],fnames[ROOTS],
          fnames[BASIC],fnames[MOBYF],fnames[DICTIONARY],fnames[PRON_FILE],
          fnames[SPACE_FILE],fnames[CLUE_FILE]);
    if (dumpit) dump(fnames[DB],fnames[FLAMINGO],fnames[PFLAMINGO]);
  }
  initialize_fitb(fnames[FITB_DIR]);
  time_and_spin("FITB database",verbose,gui);
  if (gui) spinner(-1);
  build_trie();
}

// *** DATABASE UTILIZATION *** //

// A word is an anagram if IT IS NOT IN THE DICTIONARY ALREADY and the
// hash is nonzero. (If you allow real words in, you'll get a ton of
// anagrams that really aren't.)

bool database::is_anagram(const string &fill, bool sound) const
{
  const wordmap<char> &score = sound? sound_to_score : word_to_score;
  return score[fill] < ANAGRAM_CUTOFF && !is_not_anagram(fill,sound);
}

// POS of a string.  Look it up or return UNKNOWN

const vector<POS> &database::pos(const string &wrd) const
{
  static const vector<POS> unk(1,UNKNOWN);
  if (parts_of_speech.find(wrd) == parts_of_speech.end()) return unk;
  return parts_of_speech.at(wrd);
}

// Does the given fill appear in the DB?

bool database::includes(const string &fill, bool sound) const
{
  return sound? pron_to_words.includes(fill) : word_to_score.includes(fill);
}

// Check potential fill against the Flamingo or portmanteau cutoffs

bool database::in_flamingo(const string &fill) const
{
  return get_mlg_score(fill) >= flamingo_cutoff;
}

bool database::in_portmanteau(const string &fill) const
{
  return get_mlg_score(fill) >= portmanteau_cutoff;
}

// If a word has a specific score, what's the score to use in the
// actual puzzle?  Computed in the obvious probabilistic way: if the
// probability that a word is in the DB with score 36 is p, what is
// the probability that it's a *specific* word with score 36?  The
// answer is obviously the prior probability divided by the number of
// words of score 36.  While the priors appear to be fairly
// independent of length, the numbers do depend on length.

// This kind of analysis deals with the ratio of the probability pnew
// knowing the score to the probability pold not knowing it.  pold is
// just 1/mlg_tot[size], since each word in the db is equally likely.
// pnew is p(word|score) * p(score), or mlg_prob[s] / mlg_size[size][s].

float database::get_score(unsigned sz, unsigned sc) const
{
  return mlg_prob[sc] * mlg_tot[sz] / max((unsigned) 1,mlg_size[sz][sc]);
}

// Puzzle score of a particular word sequence (perhaps just one word),
// assuming you've seen this (multi)word before.  The subword lengths
// are given by the _splits_ argument.  Call the above function, or
// return 0.  But it's slightly tricky because of split words.  In
// that case, the idea is that the score of the word (in the CCW
// sense) can't be higher than the score of the best split.  (We don't
// use the worst split because if this is a phrase, some subphrase
// might be a multiword.)

float database::get_score(const string &w, const vector<unsigned> &splits) const
{
  unsigned sc = get_mlg_score(w);
  if (!sc) return 0;
  if (splits.size() > 1) {
    unsigned split_score = 0;
    for (unsigned i = 1 ; i < splits.size() ; ++i) {
      string subword = w.substr(splits[i - 1],splits[i] - splits[i - 1]);
      split_score = max(split_score,get_mlg_score(subword));
    }
    if (!split_score) return 0;
    sc = min(sc,split_score);
  }
  return get_score(w.size(),sc);
}

// Pronunciation of a multiword, given a segmentation.  Split into
// segments and get pronunciations of each from the cache.  Serialize
// then makes all possible pronunciations of the original string (you
// do have to concatenate the pronunciations of the individual words).
// If any subword is not in the pronunciation DB, return the empty
// vector.

vector<string> database::multiword_pronunciation(const string &str, 
                                                 const segmentation &s) const
{
  vector<vector<string>> elts;
  unsigned idx = 0;
  for (auto seg : s) {
    vector<string> p = pronunciations[str.substr(idx,seg)];
    if (p.empty()) return vector<string>();
    elts.push_back(p);
    idx += seg;
  }
  vector<string> ans;
  for (auto &v : serialize(elts)) {
    string s;
    for (auto &ss : v) s += ss;
    ans.push_back(s);
  }
  return ans;
}

// Sometimes, we don't want to pronounce a multiword but just get the
// lowest scoring word it contains.

unsigned database::multiword_score(const string &str, const segmentation &s)
  const
{
  char ans = 100;
  unsigned idx = 0;
  for (auto seg : s) {
    string substr = str.substr(idx,seg);
    if (seg > 2) ans = min(ans,word_to_score[str.substr(idx,seg)]);
    if (ans == 0) return 0;
    idx += seg;
  }
  return ans;
}
