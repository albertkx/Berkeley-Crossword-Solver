#ifndef _XW_
#define _XW_

#include "score.h"

// A progress timer consists of the number of nodes expanded, the CPU
// time expended, and a limit on the badness of a word that you're
// willing to reconsider.

class progress {
  int           nodes;
  float         when;
  float         bad_word;

public:
  progress() : nodes(-1), when(-1), bad_word(BIG_CHARGE) { }
  progress(int n, float w, float b = BIG_CHARGE) :
    nodes(n), when(w), bad_word(b) { }
  progress(const vector<string> &argv, size_t &idx);

  bool somewhere_greater(const progress &other) const;
  bool time_out(float t, float delay) const {
    return when > 0 &&  t + delay > when; 
  }

  bool operator==(const progress &other) const {
    return nodes == other.nodes && when == other.when;
  }
  bool operator!=(const progress &other) const { return !operator==(other); }

  progress &operator-= (const progress &other) {
    nodes -= other.nodes;
    when -= other.when;
    return *this;
  }

  progress operator- (const progress &other) const {
    progress p = *this;
    return p -= other;
  }

  bool  end_of_minute(unsigned sz) const;
  int   count()  const { return nodes; }
  int   mins()   const { return int(when) / 60; }
  float secs()   const { return when; }
  float cutoff() const { return bad_word; }

  friend ostream & operator<<(ostream &os, const progress &p);
};

// Command line arguments, basically

enum { SILENT = -2, QUIET };

// Used for simple theme analysis.  An id (the word in question),
// whether it's across or down, and the current fill.

struct theme_entry {
  unsigned      id;
  bool          across;
  string        fill;

  theme_entry(unsigned i, bool a, const string &f) :
    id(i), across(a), fill(f) { }
};

struct settings {
  unsigned      absolute;          // a: limit in seconds
  bool          tournament_mode;   // A: set up for ACPT event
  bool          show_abbrvs;       // b: show abbrvs at end
  vector<rebus_answer> rebus;      // B: known rebus strings (see also S)
  bool          cheat;             // c: cheat
  bool          clues;             // C: evaluate heuristic on clues
  bool          debug;             // d: debug, settable from command line
  bool          dump_data;         // D: dump binary database
  unsigned      check_fill_id;     // e: word to check in detail as 43-a
  vector<string> to_check;
  bool          check_fill_across;
  bool          check_post_theme;  // E: same but post-theme only
  vector<theme_entry> theme_query; // f: explain theme and stop
  bool          firefox;           // F: write puzzle out to firefox!
  bool          gui;               // g: use GUI
  bool          consider_geometry; // G: consider geometry (G sets to false)
  string        db_source;         // h: use historical database (see also N p)
  bool          explain_theme;     // H: explain theme analysis
  bool          interactive;       // i: get puzzle to solve interactively
  bool          theme_test;        // j: theme analysis (get candidates)
  bool          theme_reproduce;   // J: theme analysis (do it)
  vector<progress> delta;          // l: default search limits (see also r)
  bool          lite;              // L: write out across lite file when done
  bool          mistakes;          // m: show mistakes
  unsigned      initial_time;      // M: always analyze for this long
                                   // N: use normal database (see also h p)
  string        param_file;        // O: file with parameters to use
                                   // p: use ACPT database (see also h N)
  bool          multipass_search;  // P: do it? (P sets to false)
                                   // q: set verbosity to -1 (see also v z)
  bool          quick;             // Q: just one pass and stop
                                   // r: relative cutoff (see also l)
  bool          read_data;         // R: use binary database (R sets to false)
                                   // S: add split rebus (see also B)
  bool          tune;              // t: tune various solving parameters (see u)
  unsigned      threads;           // T: number of threads
  vector<char>  variable_tuners;   // u: evaluate default tuners (see also t)
  bool          autotune;          // U: tune all parameters
  int           verbosity;         // v: verbosity (see also q z)
  bool          wall;              // w: use wall clock time? (implied by A also)
  int           berkeley_max;      // x: limit on # Berkeley answers produced
                                   // X: berkeley_max = 0 (so no Berkeley stuff)
                                   // z: set verbosity to -2
  bool          boswords;          // Z: use boswords scoring

  params        tparam[2], tdelta; // range to tune; # choices/parameter
  bool          acpt_geometry;

  unsigned      tuning_count;
  vector<float> tuning_results;

  settings() : absolute(DEFAULT_LIMIT), tournament_mode(false),
               show_abbrvs(false), cheat(false), clues(false), debug(false),
               dump_data(false), check_post_theme(false), firefox(false),
               gui(false), consider_geometry(true), db_source("curr"),
               explain_theme(false), interactive(false), theme_test(false),
               theme_reproduce(false), delta{{-1,DEFAULT_PROGRESS}}, lite(false),
               mistakes(false), initial_time(INITIAL_TIME), 
               multipass_search(true), quick(false), read_data(true),
               tune(false), threads(0), autotune(false),
               verbosity(0), wall(false),
#ifdef __linux__
               berkeley_max(5000),
#else
               berkeley_max(0),
#endif
               boswords(false), acpt_geometry(true), tuning_count(0)
  { }

  void     get_tuners();
  void     get_autotuners(const params &base, const tuning_fit &ranges);
  void     get_default_tuners();
  void     parse_options(const vector<string> &argv, vector<string> &files);
  void     parse_options(const string &str, vector<string> &files);
  void     set_tournament_time(const string &fname, int &difficulty,
                               bool &notheme);
  void     set_tournament_mode(unsigned absolute_mins, unsigned relaative_secs);
  void     process_cheat_file(int &argc, const char **argv,
                              vector<string> &files);
  bool     process_simulation_file();

  void     show_tuning_parameters() const;
  float    show_results(params &p, bool mark) const;
  bool     param_change(const vector<int> &relevant) const;
  bool     single_tuner() const { return variable_tuners.empty(); }
  bool     tune_scoring() const;
  bool     reset_scoring() const;
  bool     report_themeless() const;

  bool     terminate(const progress &then, const progress &now, unsigned sz,
                     unsigned idx) const;
  ostream &tuneout() const { return (single_tuner() && clues)? cerr : cout; }
};

// Structure to allow caching of heuristic values.  Just the value and
// the fill that produced it.

struct heuristic_value {
  string        fill;
  float         value;

  heuristic_value() : value(-1) { }
  heuristic_value(const string &str, float v = 0) : fill(str), value(v) { }
  heuristic_value(const scored_word &ws) : fill(ws.word), value(ws.score) { }
  bool operator<(const heuristic_value &other) const {
    if (value != other.value) return value < other.value;
    return fill < other.fill;
  }
  bool operator==(const heuristic_value &other) const {
    return value == other.value && fill == other.fill;
  }
};

// Stuff needed for fast multiword computation; documented in multiword.cpp

struct extra : vector<scored_word_sorter> {
  unsigned      offset;
};

// A simple_word is basically a word as originally appeared in the
// puzzle, before we messed around with xrefs, geometry, etc.

class simple_word {
  location      start;          // starting square
  unsigned      number;
  bool          across;
  unsigned      length;

  simple_word(unsigned n, bool a, unsigned l, location loc) : 
    start(loc), number(n), across(a), length(l) { }

public:
  bool includes(const square &sq) const;
  friend class puzzle;
};

// It appears pretty impractical to make this a class; too many things
// need the various fields.  There are three word types:
// 1. A SKELETON is just a vector of squares, along with an id (the word
//    they come from) and the current fill.
// 2. A BASIC_WORD includes all the other word-associated stuff (clue,
//    splits, etc) but NOT the real search-based stuff
// 3. A WORD includes the search-based stuff (pitched lists, etc)

struct skeleton : vector<square *> {
  string        fill;             // current fill
  unsigned      id;               // number words consecutively
  mutable vector<bool> analyzed;  // lengths you've considered

  skeleton(size_t n = 0) : vector<square *>(n), fill(n,WILD),
    analyzed(1 + LONG_WORD,false) { }
  skeleton(const vector<square *> &sq) : vector<square *>(sq), 
    fill(sq.size(),WILD), analyzed(1 + LONG_WORD,false) { }
  skeleton(vector<square *>::const_iterator b, 
           vector<square *>::const_iterator e) :
    vector<square *>(b,e), analyzed(1 + LONG_WORD,false) {
    fill.resize(size(),WILD);
  }

  const square & operator[](unsigned idx) const {
    return *vector<square *>::operator[](idx); 
  }
  square & operator[](unsigned idx) {
    return *vector<square *>::operator[](idx); 
  }

  bool across  () const { return size() > 1 && (*this)[0].y == (*this)[1].y; }
  bool down    () const { return size() > 1 && (*this)[0].x == (*this)[1].x; }
  bool vertical() const { return !across() || size() < 7; }
  
  // append two words.  But if they share a letter, keep only one copy.
  skeleton operator+(const skeleton &other) const {
    skeleton ans(*this);
    skeleton::const_iterator i = other.begin();
    if (!empty() && !other.empty() && back() == &other[0]) ++i;
    ans.insert(ans.end(),other.begin(),other.end());
    ans.fill.resize(size(),WILD);
    return ans;
  }

  // unary - is vector reversal
  skeleton operator-() const {
    skeleton ans = *this;
    reverse(ans.begin(),ans.end());
    reverse(ans.fill.begin(),ans.fill.end());
    return ans;
  }

  skeleton operator-(const skeleton &other) const {
    return (*this) + (- other);
  }

  void          clear_pointers();
  void          establish_pointers();
  bool          included_in(const vector<location> &locs) const;
  bool          rebus_fill() const;              // does fill use a rebus?
  unsigned      position(const square &s) const;
  bool          contains(const square &s) const { // square in word?
    return position(s) != (unsigned) -1;
  }
  bool          filled() const { return fill.find(WILD) == string::npos; }
  unsigned      unfilled() const;
  string        compute_fill(bool upper, char wild) const;
  bool          rebus_candidate(const rebus_square &sq) const;
  bool          has_crosser(unsigned i) const;
  unsigned      cross_pos(const skeleton &crosser) const;
  unsigned      active_crossers() const;
  bool          has_as_crosser(unsigned x) const;
};

struct basic_word : skeleton {
  vector<vector<unsigned>> repeated_squares;
                                // indices in word of puzzle squares that
                                // appear multiple times in the word
  string        clue;           // as in puzzle
  string        original_clue;  // before xref substitution
  mutable bool  fitb;           // is it?
  vector<unsigned> splits;      // point where xref clue splits
  bool          honor_splits;   // does the word have to split there?  (yes for
                                // xref, no for geometry)
  vector<unsigned> numbers;     // number(s) in the grid (for output)
  string        answer;         // correct fill if known

  basic_word(const vector<square *> &sq) : skeleton(sq), fitb(false),
                                           splits(1,0), honor_splits(false) { }
  basic_word() { }

  string        desc() const;
  void          establish_pointers(bool check_repeated);
  bool          clueless() const;
  void          find_fill(char f[BIG_SIZE][BIG_SIZE]) const;
  void          gui_info() const;
  bool          multi_done(unsigned dict) const;
  float         correctp(const string &str, const database *d) const;
};

// And here is what's more relevant, a basic_word that includes what
// you need to conduct the search.

class word : public basic_word {
  vector<string>     pitched;       // words that have been pitched by LDS
  int                pitch_block;   // used to restrict search to trouble areas
  vector<string>     best_solution; // stack of fills that were part of the
                                    // best solution at some time
  vector<string>     best_pitches;  // fills that were pitched in best solution
  wordmap<float>     suggestions;   // suggestions for this word
  bool               to_initialize; // do I need to (re)initialize this word?
  vector<theme_fill> theme_sugg;    // suggested fill based on theme
  string             pun_sound;     // target phrase if you're punning

  mutable map<string,float> computed_scores;  // cache
  mutable vector<heuristic_value> best_fills; // value of best fills
  mutable map<string,float> theme_computed;   // theme value
  mutable map<string,extra> xtra;             // multiword fills
  mutable filler wfiller;                     // filler for this word
  mutable shared_ptr<fill_indexer> indexer;   // fill indexer (has to be a
                                // shared_ptr because a puzzle is a vector of
                                // words, so you have to be able to copy
  mutable wordset suggested;                  // suggested fills

  bool           is_allowed(const string &s) const;
  void           clear_cache();
  void           reset();
  void           reset_indexer();
  const string & best_unpitched() const;

public:
  word(const vector<square *> &sq) : basic_word(sq), pitch_block(0),
                                     to_initialize(true), indexer(nullptr) { }
  word() { id = -1; }

  void set_initialize()           { to_initialize = true; }
  void clear_pitched()            { pitched.clear(); }
  void add_pitch(const string &p) { pitched.push_back(p); }

  const string &get_solution() const {
    return best_solution.empty()? fill : best_solution.back(); 
  }

  const string &psound() const      { return pun_sound; }
  void set_psound(const string &f)  { pun_sound = f; }

  friend class puzzle;
  friend class search_node;
  friend void *threaded_filler(void *ptr);
  friend ostream & operator<<(ostream &os, const word &w);
};

// A choice is a forward step in the search.  It consists of the word
// slot being impacted, a bool indicating if this choice is a word
// pitch, and either the fill to use or the word to pitch.

struct choice {
  unsigned      word_slot;
  bool          pitch;
  string        fill;

  choice(const string &f, unsigned slot, bool p = false) : 
    word_slot(slot), pitch(p), fill(f) { }

  bool operator==(const choice &other) const {
    return word_slot == other.word_slot && pitch == other.pitch && 
      fill == other.fill; 
  }
  bool operator!=(const choice &other) const { return !operator==(other); }
};

// The cost of a search node (how we decide what to expand next)
// involves the number of words pitched; beyond that, we use a random
// second value if there are >= 2 pitches, and depth if there is only
// one.  (This appears to work better than other approaches.)

struct cost {
  unsigned      pitches;
  unsigned      extra;

  cost() : pitches(0), extra(0) { }
  
  bool operator==(const cost &other) const {
    return pitches == other.pitches && extra == other.extra; 
  }
  bool operator!=(const cost &other) const { return !operator==(other); }

  bool operator<(const cost &other) const {
    if (pitches != other.pitches) return pitches < other.pitches;
    return extra > other.extra;
  }
};

// A task (a node in the search stack, basically) consists of the
// choices made to get here and the (lazily computed) cost.

struct task {
  vector<choice>    state;
  int               pitch_block;
  mutable cost      tcost;

  task() : pitch_block(0) { }

  void find_cost() const;
  bool operator<(const task &other) const;
};

// A replacement for geometry reasons.  The ID of the word being
// replaced, and new squares and numbers for the new word.  If numbers
// is empty, it means you "found" this word elsewhere and are just
// substituting different squares.

struct geo_replacement {
  unsigned         id;
  vector<location> squares;
  vector<unsigned> numbers;

  geo_replacement(unsigned i, const vector<location> &s,
                  const vector<unsigned> n = vector<unsigned>()) :
    id(i), squares(s), numbers(n) { }
};

// A puzzle is just a collection of words.  Total grid size is in
// xymax, and there is a list of explicit black squares useful for
// output.  We also record the time allowed to complete the puzzle in
// the ACPT, which is needed for ACPT-like scoring.

class search_node;

class puzzle : public vector<word> {
  vector<square *>     squares;
  vector<simple_word>  simple_words;
  string               source;
  string               title;
  string               author;
  int                  difficulty;
  bool                 notheme;
  location             xymax;
  vector<square>       black_squares;
  unsigned             acpt_time;
  vector<rebus_answer> rebus;         // rebus fill candidates
  vector<bool>         should_be_themed;

public:
  puzzle() { }
  puzzle(const string &fname);

  unsigned rows() const { return xymax.y; }
  unsigned cols() const { return xymax.x; }

  unsigned limit() const { return acpt_time; }
  unsigned &limit()      { return acpt_time; }

  int      diff() const { return difficulty; }
  int     &diff()       { return difficulty; }

  bool     noth() const { return notheme; }
  bool    &noth()       { return notheme; }

  const square *find_square      (location loc) const;
  square       *find_square      (location loc);
  const square *find_square      (unsigned x, unsigned y) const;
  square       *find_square      (unsigned x, unsigned y);

  bool illegal() const;
  void set_fill(word &wd, const string &f);
  unsigned opposite(const word &w) const;
  void geo_replace(const geo_replacement &rep);
  bool geo_word(const vector<location> &s, bool jump,
                vector<unsigned> &to_remove,
                vector<geo_replacement> &replacements) const;

private:
  char *insert_word(unsigned xpos, unsigned ypos, unsigned xlim,
                    unsigned &number, char *clue, bool unscrambled,
                    char grid[50][50], bool *hasacross, bool newnumber);
  const word &xref(const string &c) const;
  const word &xref(const word &w, bool recur) const;
  void compute_fill(char fill[BIG_SIZE][BIG_SIZE]) const;
  void erase(word &w);
  void insert(word &w, const string &s);
  void clear_fill() { for (auto &w : *this) erase(w); }
  void restore_best(const search_node &node);

  void reset_squares(word &wd, const vector<location> &locs);
  void remove_word(unsigned x);
  void remove_words(vector<unsigned> &rems);

  friend class search_node;
  void   show_fill(char f[BIG_SIZE][BIG_SIZE], ostream &os) const;

public:
  void delete_squares() {
    for (size_t i = 0 ; i < squares.size() ; ++i) delete squares[i];
  }

  const string get_solution(const word &w) const { return w.get_solution(); }
  void collapse_xrefs();
  vector<unsigned> get_simple_words(const word &w) const;
  void solve(const database &db, settings &s);
  vector<unsigned> find_theme_words(unsigned length) const;
  vector<unsigned> find_theme_words() const;
  void request_render() const;
  void across_lite(const string &puzfile) const;
  void firefox() const;
  void rebuild_from_task(const task &t);
  unsigned num_correct() const;

  friend ostream &operator<< (ostream &os, const puzzle &p);
  friend cgrid::cgrid(const puzzle &puz, bool answer);
};

// As we decide which word to fill, we want to keep track of what the
// impact is on crossing words when we put characters in specific
// places.  This lets us cache that.  For each position and character,
// we have the actual impact, and then we also have a vector<bool>
// indicating whether or not the cached value is accurate.

struct fill_cache {
  float               *letscores;
  vector<vector<bool>> known_scores;

  fill_cache() : letscores(nullptr) { }
  ~fill_cache() { free(letscores); }
  void reset(unsigned sz) {
    free(letscores);
    letscores = (float *) malloc(sz * NUMCHARS * sizeof(float));
    known_scores.resize(sz);
    for (size_t i = 0 ; i < sz ; ++i) 
      known_scores[i] = vector<bool>(NUMCHARS,false);
  }
  
  float letter_scores(unsigned let, unsigned ch) const {
    return letscores[let * NUMCHARS + ch];
  }
  float &letter_scores(unsigned let, unsigned ch) {
    return letscores[let * NUMCHARS + ch];
  }
};

// And here is the search node itself.

enum error_type { CORRECT, SEARCH, HEURISTIC };

struct geo_evaluator;

class search_node : public puzzle {
  const database *d;                    // word database to use
  settings      *sets;                  // settings established by user

  unsigned      node_ct;                // # nodes expanded so far
  float         best_score;             // score of best solution found
  progress      best_solution_time;     // when it was found
  progress      leak_time;              // soln time when you leaked

  vector<task>   stack;
  vector<char>   locked;                // keep word n stable in postprocess?
  vector<sourced_theme> puzzle_theme;
  geometry       geo;

  vector<choice> choices;               // choices after task instantiation

public:
  search_node(puzzle &p, const database *db, settings &s) :
    puzzle(p), d(db), sets(&s), node_ct(0), best_score(BIG_SCORE),
    locked(p.size(),false) { 
      rebus = s.rebus;
      get_time(true);
  }

private:
  const params &param ()               const { return d->get_param(); }
  float         detail(unsigned which) const { return param().detail(which); }

  float ndc(const word &w) const;
  float score() const;          // score of current fill
  float score_with_crossers(const word &w) const;
  bool  error(const choice &c) const; // is the choice a mistake?
  float score(const word &wd, const string *f = nullptr) const;
  float scale(const word &w, float orig) const;
  float fill_scale(const word &w) const;
  heuristic_value find_fill(const word &wd, int fill, float cutoff, bool sc,
                            fill_cache &cache) const;
  void  find_fills(const word &wd, float delta_reqd, bool sc, 
                   fill_cache *cache = nullptr) const;
  float fill_value(const word &wd) const;
  float fill_value(const word &wd, float delta_reqd) const;

  pair<unsigned,unsigned> matching_words(const string &wrd, const word &slot,
                                         const segmentation &splits) 
    const;
  float score_multiword(const string &wrd, unsigned dictnum, const word &slot,
                        const multidata &md) const;

  float theme_bonus(const word &slot, const string &fill) const;
  bool  is_theme_fill(const word &slot, bool strong) const;
  float theme_conforming(const word &w, const string &fill, bool strong,
                         bool quick) const;
  void get_scored_multiwords(const string &pat, unsigned dict, const word &slot,
                             vector<scored_word_sorter> &ans) const;

  scored_word get_fill(const word &w, const string &pat, unsigned n = 0) const;
  void tell_gui(const word &curr, unsigned original);

  vector<unsigned> to_fill();
  void threaded_filler(dispatcher &d, const vector<unsigned> &ids);

  void insert(word &w, const string &s);

  float tune();
  float show_clue_analysis() const;

  bool fill_to_completion();
  void expand(const task &t);
  void new_best_solution();
  void report();
  void add_new_tasks(const task &t);
  void expand();
  choice branch() const;
  float score(const vector<unsigned> &w) const;
  bool adjust_fill(word &w, float &old_score, vector<unsigned> wilds,
                   fill_cache &fc);
  bool adjust_fill_detail(word &w, unsigned makewild, unsigned limit,
                          fill_cache &fc);
  bool adjust_fill(word &w);
  void add_with_crossers(set<int> &ans, const word &wd) const;
  void add_with_crossers(set<int> &ans, const word &wrd, 
                         const string &other_fill) const;
  void postprocess(vector<int> &consider);
  void postprocess();
  bool finalize();
  set<set<unsigned>> combination_components(const set<unsigned> &changed);
  void combine_component(const set<unsigned> &component, 
                         const vector<string> &fill, float &best_score);
  void combine_best(const vector<string> &fill);
  void combine_bests();
  bool search_continues(unsigned pass) const;
  progress current_time() const { return progress(node_ct,get_time()); }
  unsigned restrict_search(unsigned lim);
  void reset_search(bool reset_sol, unsigned cpts);
  void solve(bool reset, bool reset_sol, unsigned &pass);
  void restore_best_solution();
  void add_flamingo_suggestions();
  vector<string> flamingo_match(const word &wrd) const;
  task flamingo_task(const word &slot, const string &word) const;
  void handle_flamingo_suggestion(const word &w, const string &sugg);
  void lds(bool reset, bool reset_sol, unsigned &pass);

  float rebus_score(const word &w, const string &str) const;
  string unrebus(const word &w, const string &str) const;
  bool check_now() const {
    return !sets->to_check.empty() &&
      !(sets->check_post_theme && puzzle_theme.empty());
  }
  bool rebus_check();
  bool rebus_check_with_reset();
  bool incorporate_theme_suggestions (const vector<unsigned> &themed,
                                      const possible_theme &sugg);
  void show_theme() const;
  void set_themed(unsigned i);
  void check_pun_theme(const vector<unsigned> &themewords);
  void find_possible_themes(const vector<unsigned> &themewords);
  float update(const geo_evaluator &old_fill, const geo_evaluator &new_fill)
    const;
  float evaluate(const boggle_string &bs, const cgrid &grid) const;
  geometry find_geometry() const;
  bool try_geometry(geometry &geo,bool doit);
  void mark_themed_words(const vector<unsigned> &themed);
  vector<unsigned> get_identified_theme_words() const;
  possible_theme find_theme_suggestions(vector<unsigned> &themed);
  void find_extra_themed_words(const vector<unsigned> &themed,
                               const possible_theme &sugg);
  bool nonrebus_check_with_reset();
  bool theme_check_with_reset(bool &has_theme);
  bool theme_complete() const;
  void check_theme();

  void initialize_for_check();
  void initialize_solver(bool initialize_all);
  void find_xrefs(bool initial);
  void get_rebus_suggestions(word &w);
  void reinitialize_solver();
  void reset_fillers() { for (auto &i : *this) i.to_initialize = true; }
  void reset_fill();
  void reset_solution();
  vector<set<unsigned>> find_components(const set<unsigned> &words) const;
  vector<error_type> find_errors() const;
  void show_mistakes(set<unsigned> &words) const;
  string adjust_to_check_rebus(char fill, const square &sq) const;
  vector<unsigned> compute_error(const word &w, bool display, const string &fill,
                                 float &wrong, float &right, 
                                 set<unsigned> &words, set<square> &letters) 
    const;

  int   verb()     const { return sets->verbosity; }
  void  show_changes(bool display = true) const;

  float best_fscore(unsigned id, const string &fill, bool combined,
                    const map<string,vector<string>> &flamingoes,
                    string *best_string = nullptr) const;
public:
  double get_time(bool reset = false) const {
    return ::get_time(sets->wall,reset);
  }
  void  initialize_slot(word &w);
  void  themed_search();
  bool  check(bool details) const;
  void  request_display(bool show_dots, bool finished) const;

  friend void *threaded_filler(void *);
};

void terminate(int val);

ostream &operator<<(ostream &os, const progress &p);
ostream &operator<<(ostream &os, const square &w);
ostream &operator<<(ostream &os, const word &w);
ostream &operator<<(ostream &os, const choice &c);
ostream &operator<<(ostream &os, const puzzle &p);
ostream &operator<<(ostream &os, const heuristic_value &hv);
ostream &operator<<(ostream &os, const skeleton &s);

#endif
