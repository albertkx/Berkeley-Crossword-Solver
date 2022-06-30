#include "xw.h"

// If there is more than one tuning possibility, show the tuning
// parameters you're going to consider by walking through them.  Set
// the short flag so that they're displayed briefly.

void settings::show_tuning_parameters() const
{
  if (single_tuner()) return;
  params p = tparam[0];
  p.sh = true;
  do cout << p << endl;
  while (p.param_next(tparam[0],tparam[1],tdelta));
}

// Display the results of tuning.  Report the number of samples and
// then find the index of the one that scored the best (better scores
// are higher in general, but lower for clues).  Then walk through the
// parameters again until you get to that index, and display that.
// For any parameters with trivial range, set them to -1 so they don't
// clutter the display.

float settings::show_results(params &p, bool mark) const
{
  if (!tune) return 0;
  if (verbosity != SILENT)
    tuneout() << tuning_count << " tuning sample(s)" << endl;
  unsigned best = 0;
  for (unsigned i = 1 ; i < tuning_results.size() ; ++i)
    if (clues? (tuning_results[i] < tuning_results[best]) :
        (tuning_results[i] > tuning_results[best]))
      best = i;
  // now find the parameters that actually match that index
  unsigned idx = 0;
  p = tparam[0];
  do if (idx++ == best) break;
  while (p.param_next(tparam[0],tparam[1],tdelta));

  if (mark) for (size_t i = 0 ; i < PARAM_END ; ++i)
              if (tdelta.detail(i) == 1) p.detail(i) = -1;
  float ans = tuning_results[best] / tuning_count;
  if (verbosity != SILENT)
    tuneout() << "best tuned value is " << ans << " for params " << endl
              << p << endl;
  return ans;
}

// Do we need to recompute all the scores when changing tuning
// parameters?  Yes, if any of the crucial parameters is varying.

bool settings::param_change(const vector<int> &relevant) const
{
  return any_of(relevant.begin(),relevant.end(),
                [&](int i) {
                  return find(variable_tuners.begin(),variable_tuners.end(),i) !=
                    variable_tuners.end();
                });
}

bool settings::tune_scoring() const
{
  static vector<int> score_params =
    { CBOUND, PBOUND, FBOUND, PWEIGHT, FWEIGHT, IS_ABBRV,
      FITB_BONUS, PAREN_BONUS, CLUE_BONUS, MOBY_BONUS, BERKELEY };
  return param_change(score_params);
}

bool settings::reset_scoring() const
{
  static vector<int> score_params = { 
    INEXACT, ABBRV_LIMIT, FITB_LIMIT, FITB_SCORE, MAXIMUM_MATCH, MINIMUM_MATCH
  };
  return param_change(score_params);
}

// Tune the solver by running through possible values for the
// parameters, for a single puzzle.  Begin by setting aside enough
// space for all the tuning results if you haven't already done that.

// Now increment the number of puzzles that have been analyzed (that's
// what tuning_count records), and set up the database parameters.
// Then solve the puzzle once for each set of parameter values,
// resetting between settings and reinitializing the solver as well if
// need be.  At the end, show the results.

float search_node::tune()
{
  if (sets->tuning_results.empty()) {
    unsigned rsize = 1;
    for (size_t i = 0 ; i < PARAM_END ; ++i) rsize *= sets->tdelta.detail(i);
    sets->tuning_results.resize(rsize,0);
  }
  ++sets->tuning_count;
  unsigned idx = 0;
  d->get_param() = sets->tparam[0];
  bool reinitialize = sets->tune_scoring();
  bool total_reset = sets->reset_scoring();

  do {
    if (total_reset) initialize_solver(true);
    else if (reinitialize) reinitialize_solver(); // have to reinitialize
    // even if first time because parameters not set correctly at the beginning!
    if (sets->clues) {
      initialize_solver(false);
      sets->tuning_results[idx++] += show_clue_analysis();
    }
    else {
      node_ct = 0;
      unsigned pass = 0;
      solve(true,true,pass);
      // args are reset, reset solution, timing pass (default args
      // allows postprocess, but it doesn't matter here because we
      // stop on error because sets->tune is true)
      unsigned corr = num_correct();
      if (verb() > SILENT) cout << corr << ' ' << flush;
      sets->tuning_results[idx++] += corr;
      reset_fill();
      reset_solution();
    }
    reset_fillers();
  } while (d->get_param().param_next(sets->tparam[0],sets->tparam[1],
                                     sets->tdelta));
  if (verb() > SILENT) cout << endl;
  params p;
  return sets->show_results(p,true);
}

// default parameter initialization

params::params() : sh(false) 
{                                       // * = requires reinitialization
  details[CBOUND]             = 6.9e-5; // *
  details[PBOUND]             = 5e-6;   // *
  details[FBOUND]             = 5e-7;   // *
  details[PWEIGHT]            = 0.37;   // *
  details[FWEIGHT]            = 0.33;   // *
  details[INEXACT]            = 2.25;   // ! inexact weight, original
  details[ABBRV_LIMIT]        = 1e-5;   // !
  details[IS_ABBRV]           = 0.32;   // * 
  details[SCALE_BY_CROSSERS]  = 0.24;
  details[THREE_LETTERS]      = 1.2;
  details[NON_DICT_CHARGE]    = 21;
  details[NON_DICT_TRANSITION] = 9.9;
  details[NON_MULTI_CHARGE]   = 29.8;
  details[PWEIGHT_M]          = 0.03;
  details[DICT_CHARGE]        = 4.76;
  details[WORD_CHARGE]        = 38.8;
  details[EXTRA_WORD_CHARGE]  = 1.98;
  details[MATCHBASE]          = 0.48;
  details[UNMATCH]            = 0.19;
  details[FITB_BONUS]         = 1.9;    // *
  details[FITB_LIMIT]         = 7.2;    // ! (total reset)
  details[FITB_SCORE]         = 0.3;    // !
  details[PAREN_BONUS]        = 0.67;   // *
  details[CLUE_BONUS]         = 3.44;   // *
  details[MULTI_ROOT]         = 5.92;
  details[MAXIMUM_MATCH]      = 0.79;   // !
  details[MINIMUM_MATCH]      = 0.2;    // !
  details[MOBY_BONUS]         = 1.78;   // *
  details[VERTICAL]           = 1.2;
  details[NO_PARSE]           = 1.18;
  details[BERKELEY]           = 0.89;   // *
}

// names of various parameters.  We put it here so it's easy to keep
// synced up with param definition

static const string param_names[PARAM_END] = 
  { "cbound", "pbound", "fbound", "pweight", "fweight", "inexact",
    "abbrv limit", "bad abbrv penalty",
    "scale by crossers", "three-letter word scale",
    "nonword charge (multi possible, below transition)",
    "nonword transition point (multi possible)",
    "nonword charge (multi impossible)", "pweight (multiword)",
    "dictionary 1 charge", "multiword charge", "multiword charge per extra word",
    "multiword base factor", "multiword unmatch factor", 
    "fitb increment", "fitb limit", "fitb max score",
    "paren bonus", "clue_bonus", "multiword root bonus",
    "maximum match", "minimum match",
    "moby thesaurus credit (exact)", "verticality factor", "no parse",
    "Berkeley weight"
  };

// Set the parameters from a tuning file.  Just get the parameters
// from any line that begins with a parameter description.  Note the
// hack of requiring that the value begin with a digit, to separate
// "pweight" from "pweight (multiword)".

void database::set_param(const string &param_file)
{
  if (param_file.empty()) return;
  ifstream inf(param_file);
  check_stream(inf,param_file);
  string line;
  params p;
  while (getline(inf,line))
    for (unsigned i = 0 ; i < PARAM_END ; ++i)
      if (line.substr(0,param_names[i].size()) == param_names[i] &&
          isdigit(line[param_names[i].size() + 1])) {
        p.detail(i) = stof(line.substr(1 + param_names[i].size()));
        break;
      }
  get_param() = p;
}

// Utilities for tuning.  First, something to get a range of values
// from the terminal. You want lo, hi and number of steps.  But if no
// entries are provided, just use the default.  If one entry, use
// that.  If two entries, default to ten steps.  Return true if the
// range is nontrivial.

struct tuning_range {
  float lo, hi;
  unsigned num;
  tuning_range() : lo(1), hi(1), num(1) { }
  tuning_range(float l, float h, unsigned n) : lo(l), hi(h), num(n) { }
};

ostream &operator<<(ostream &os, const tuning_range &r)
{
  return os << "lo " << r.lo << " hi " << r.hi << " n " << r.num;
}

// A tuning_info is a tuning_range, but also include which parameter
// is being tuned so we can print things back out.

struct tuning_info : tuning_range {
  unsigned which;
  tuning_info(unsigned w, float l, float h, unsigned n) :
    tuning_range(l,h,n), which(w) { }
  tuning_info(unsigned w, float h, unsigned n) :
    tuning_range(1 / h,h,n), which(w) { }
  tuning_info(unsigned w, unsigned n) : tuning_range(0.5,2,n), which(w) { }
};

// A tuning_fit is just a map from parameters to tuning_range's.
// (This is basically just another way to store the ID.)  We also
// include a description.

struct tuning_fit : map<unsigned,tuning_range> {
  string desc;

  tuning_fit(const string &d, const vector<tuning_info> &v) : desc(d) {
    for (auto &ti : v) (*this)[ti.which] = ti;
  }
};

bool get_range(const string &str, float def, float &lo, float &hi, float &d,
               bool &use_default)
{
  if (use_default) {
    hi = lo = def;
    d = 1;
    return false;
  }
  string resp;
  cerr << str << " (" << def << ")? ";
  getline(cin,resp);
  if (resp == ".") use_default = true;
  if (use_default || resp.empty()) {
    hi = lo = def;
    d = 1;
    return false;
  }
  istringstream is(resp);
  is >> lo;
  if (is.eof()) { hi = lo; d = 1; return true; }
  is >> hi;
  if (is.eof()) d = 10;
  else is >> d;
  return true;
}

// Get a range for autotuning.  Now lo and hi are ratios to the
// current default value and the number should be there.

void get_range(float def, float &lo, float &hi, float &d, tuning_range &r)
{
  if (r.lo >= 0) {
    lo = r.lo * def;
    hi = r.hi * def;
  }
  else {
    lo = -r.lo;
    hi = -r.hi;
  }
  d  = r.num;
}

// Get tuning parameters by getting each detail.

void settings::get_tuners()
{
  bool use_default = false;
  params p;
  for (size_t i = 0 ; i < PARAM_END ; ++i)
    if (get_range(param_names[i],p.detail(i),tparam[0].detail(i),
                  tparam[1].detail(i),tdelta.detail(i),use_default))
      variable_tuners.push_back(i);
}

// Get tuners from autotuning information.

void settings::get_autotuners(const params &base, const tuning_fit &ranges)
{
  for (size_t i = 0 ; i < PARAM_END ; ++i) {
    tuning_range t;
    if (ranges.find(i) != ranges.end()) {
      variable_tuners.push_back(i);
      t = ranges.at(i);
    }
    get_range(base.detail(i),tparam[0].detail(i),tparam[1].detail(i),
              tdelta.detail(i),t);
  }
}

// Parameter output is here because it uses the parameter names above.

ostream &operator<<(ostream &os, const params &p)
{
  for (size_t i = 0 ; i < PARAM_END ; ++i)
    if (p.sh) os << p.detail(i) << ' ';
    else if (p.detail(i) >= 0) 
      os << param_names[i] << ' ' << p.detail(i) << endl;
  return os;
}

// Default tuners have no iteration and just evaluate the default settings.

void settings::get_default_tuners()
{
  params p;
  for (size_t i = 0 ; i < PARAM_END ; ++i) {
    tparam[0].detail(i) = tparam[1].detail(i) = p.detail(i);
    tdelta.detail(i) = 1;
  }
}

// As you tune, you want to be able to switch from one set of
// parameters to the "next".  Just advance each detail; if you run off
// the edge, reset it and its predecessors and advance the next one.

bool params::param_next(const params &begin, const params &end, 
                        const params &delta)
{
  for (size_t i = 0 ; i < PARAM_END ; ++i) {
    if (delta.detail(i) > 1) {
      if (delta.detail(i) == 2) {
        if (detail(i) == begin.detail(i)) {
          detail(i) = end.detail(i);
          return true;
        }
      }
      else {
        float n = detail(i) * pow(end.detail(i) / begin.detail(i),
                                  0.99 / (delta.detail(i) - 1));
        if (n <= end.detail(i)) {
          detail(i) = n;
          return true;
        }
      }
      detail(i) = begin.detail(i);
    }
  }
  return false;
}

// How we do autotuning.  Some parameters are tuned in parallel; most
// are not.

vector<tuning_fit> params::autotune_adjustments() const
{
  return vector<tuning_fit> {
    { "cbound",            {{ CBOUND, 6 }} },
    { "pweight and bound", {{ PBOUND, 4 }, { PWEIGHT, 1.5, 4 }} },
    { "fweight and bound", {{ FBOUND, 4 }, { FWEIGHT, 1.5, 4 }} },
    { "inexact",           {{ INEXACT, 6 }} },
    { "abbrv limit/bool",  {{ ABBRV_LIMIT, 0.1, 5, 4 }, { IS_ABBRV, 4 }} },
    { "crossing scales",   {{ SCALE_BY_CROSSERS, 6 }, { THREE_LETTERS, 6 }} },
    { "nondict charge/trans", {{ NON_DICT_CHARGE, 0.8, 1.2, 5 },
                               { NON_DICT_TRANSITION, 0.8, 1.2, 5 }} },
    { "nonword charges",   {{ NON_MULTI_CHARGE, 0.8, 1.2, 5 },
                            { WORD_CHARGE, 0.8, 1.2, 5 }} },
    { "multi weight",      {{ PWEIGHT_M, 6 }} },
    { "dict/word charges", {{ DICT_CHARGE, 1.5, 4 },
                            { EXTRA_WORD_CHARGE, 1.5, 4 }} },
    { "match/unmatch",     {{ MATCHBASE, 1.5, 4 }, { UNMATCH, 1.5, 4 }} },
    { "fitb bonus",        {{ FITB_BONUS, 1.5, 4 }} },
    { "fitb limit",        {{ FITB_LIMIT, 1.5, 4 }} },
    { "fitb score",        {{ FITB_SCORE, 1.5, 4 }} },
    { "paren bonus",       {{ PAREN_BONUS, 1.5, 4 }} },
    { "exact bonus",       {{ CLUE_BONUS, 1.5, 4 }} },
    { "multi root",        {{ MULTI_ROOT, 6 }} },
    { "min/max match",     {{ MAXIMUM_MATCH, 1.5, 4 },
                            { MINIMUM_MATCH, 1.5, 4 }} },
    { "Moby bonus",        {{ MOBY_BONUS, 1.5, 4 }} },
    { "vertical",          {{ VERTICAL, 0.8, 1.4, 4 }} },
    { "multiparse",        {{ NO_PARSE, 1.7, 6 }} },
    { "Berkeley",          {{ BERKELEY, -0.8, -1, 4 }} } // negative means
                                // negated values instead of ratios
  };
}

// And here is the actual autotuning.  First get the current value,
// then for each autotune adjustment, reset things and tune.  Now if
// it's an improvement, adjust the parameters.  Either way, continue.

void database::autotune(settings &sets, const vector<string> &files) const
{
  sets.get_default_tuners();
  sets.tune = true;
  process_files(sets,files);
  params pbest;
  float best = sets.show_results(pbest,false);
  cout << "base value " << best << endl;
  for (auto &m : pbest.autotune_adjustments()) {
    sets.get_autotuners(pbest,m);
    sets.show_tuning_parameters();
    sets.tuning_results.clear();
    sets.tuning_count = 0;
    process_files(sets,files);
    params p;
    float result = sets.show_results(p,false);
    cout << "new result for " << m.desc << " = " << result;
    if (result <= best) cout << " no";
    else {
      pbest = p;
      best = result;
    }
    cout << " improvement\n";
  }
  cout << "Adjustments complete.  Final parameters " << pbest << endl
       << " and score " << best << endl;
}
