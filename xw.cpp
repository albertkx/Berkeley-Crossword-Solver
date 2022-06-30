#include "xw.h"
#include <unistd.h>
#include <csignal>
#include <sys/sysctl.h>

bool debug = false;

void print_usage()
{
  printf("solver options:\n"
         "\n"
         " -a seconds\tAbsolute cutoff\n"
         " -A \tTournament mode\n"
         " -b show abbreviations\n"
         " -B string\tadd string to rebus list\n"
         " -c \tCheat\n"
         " -C \tEvaluate heuristic on clues\n"
         " -d \tDebug\n"
         " -D \tDump binary database\n"
         " -e n-[ad] pat1 ...\tExamine scores for given patterns and word\n"
         " -E n-[ad] pat1 ...\tSame as -e but after theme discovered\n"
         " -f (n-[ad] pat)+\tSimilar, but theme words and fill identified\n"
         " -F \tPush back into Firefox\n"
         " -g \tUse GUI\n"
         " -G \tDon't consider Geometry\n"
         " -h \tUse historical database\n"
         " -H \texplain tHeme\n"
         " -i \tInteractive mode\n"
         " -j \tTheme analysis only (get candidates)\n"
         " -J \tTheme analysis only (do analysis)\n"
         " -l \tDefault search limits\n"
         " -L \tPush back into Across Lite\n"
         " -m \tShow mistakes\n"
         " -M \tMinimum analysis time\n"
         " -N \tUse normal database source\n"
         " -O file\tGet parameters from given file\n"
         " -p \tUse ACPT-pruned database\n"
         " -P \tTurn multiPass search OFF\n"
         " -q \tQuiet\n"
         " -Q \tQuick\n"
         " -r nodes time [bad_words]\tRelative cutoff\n"
         " -R \tDo not read binary database\n"
         " -S x y\tadd x/y split to rebus list\n"
         " -t \tTune (and use historical data)\n"
         " -T n\tSpecify number of threads\n"
         " -u \tEvaluate default tuners (and use historical data)\n"
         " -U \tAutotune\n"
         " -v \tIncrease verbosity\n"
         " -w \tUse wall time\n"
         " -x n\tmaXimum number of answers returned by Berkeley NLP engine\n"
         " -X \tlike -x 0 so no use of Berkeley engine\n"
         " -z \tSilent\n"
         " -Z \tUse Bosowrds scoring\n"
         );
}

// Construct a progress from the next two or three args on the command
// line.  Nodes first, as for the progress constructor.

progress::progress(const vector<string> &argv, size_t &idx) : progress()
{
  if (++idx < argv.size()) nodes    = stoi(argv[idx]);
  if (++idx < argv.size()) when     = stof(argv[idx]);
  if (++idx < argv.size()) bad_word = stof(argv[idx]);
}

void settings::parse_options(const vector<string> &argv, vector<string> &files)
{
  files.clear();
  for (size_t i = 0 ; i < argv.size() ; ++i)
    if (argv[i][0] == '-') {
      size_t j = i;
      for (size_t k = 1 ; argv[j][k] ; ++k) {
        switch (argv[j][k]) {
        case 'a': absolute = stoi(argv[++i]); break;
        case 'Z': boswords = true; // fall through
        case 'A':
          tournament_mode = wall = true;
          delta[0] = progress(-1,initial_time);
          break;
        case 'b': show_abbrvs = true; break;
        case 'B': rebus.push_back(rebus_answer(argv[++i])); break;
        case 'c': cheat = true; break;
        case 'C': clues = true; // now match 'u' below
          if (db_source == "curr") db_source = "hist"; 
          tune = true; 
          get_default_tuners(); 
          break;
        case 'd': debug = ::debug = true; break;
        case 'D': dump_data = true; read_data = false; break;
        case 'E': check_post_theme = true; // and fall through
        case 'e': {
          check_fill_id = stoi(argv[++i]);
          size_t p = argv[i].find('-');
          if (p == string::npos || p == argv[i].size() - 1) {
            cerr << "specify word to check as 42-d or similar" << endl;
            abort();
          }
          check_fill_across = (tolower(argv[i][1 + p]) == 'a');
          for (++i ; i < argv.size() ; ++i) to_check.push_back(argv[i]);
          break;
        }
        case 'f': explain_theme = true;
          while (i + 1 < argv.size()) {
            unsigned theme_id = stoi(argv[++i]);
            size_t p = argv[i].find('-');
            if (p == string::npos || p == argv[i].size() - 1) {
              cerr << "specify word to check as 42-d or similar" << endl;
              abort();
            }
            bool acr = (tolower(argv[i][1 + p]) == 'a');
            if (i + 1 == argv.size()) {
              cerr << "missing theme entry for " << argv[i] << endl;
              abort();
            }
            theme_query.push_back(theme_entry(theme_id,acr,argv[++i]));
          }
          break;
        case 'F': firefox = true; break;
        case 'g': gui = true; break;
        case 'G': consider_geometry = false; break;
        case 'h': db_source = "hist"; break;
        case 'H': explain_theme = true; break;
        case 'i': interactive = true; break;
        case 'j': theme_test = true; break;
        case 'J': theme_reproduce = true; break;
        case 'l': delta[0] = progress(-1,initial_time); break; // relative limit
        case 'L': lite = true; break;
        case 'm': mistakes = true; break;
        case 'M': initial_time = stoi(argv[++i]); break;
        case 'N': db_source = "curr"; break;
        case 'O': param_file = argv[++i]; break;
        case 'p': db_source = "acpt"; break;
        case 'P': multipass_search = false; break;
        case 'q': verbosity = QUIET; break;
        case 'Q': quick = true; break;
        case 'r': {
          unsigned d = stoi(argv[++i]);
          if (delta.size() < 1 + d) delta.resize(1 + d);
          delta[d] = progress(argv,i); break;
        }
        case 'R': read_data = false; break;
        case 'S': rebus.push_back(rebus_answer(argv[1 + i],argv[2 + i]));
          i += 2; 
          break;
        case 't': if (db_source == "curr") db_source = "hist"; 
          tune = true;
          get_tuners(); 
          break;
        case 'T': threads = stoi(argv[++i]); break;
        case 'u': if (db_source == "curr") db_source = "hist"; 
          tune = true; 
          get_default_tuners(); 
          break;
        case 'U': if (db_source == "curr") db_source = "hist";
          autotune = true;
          verbosity = SILENT;
          break;
        case 'v': ++verbosity; break;
        case 'w': wall = true; break;
        case 'x': berkeley_max = stoi(argv[++i]); break;
        case 'X': berkeley_max = 0; break;
        case 'z': verbosity = SILENT; break;
        default: print_usage(); ::terminate(1);
        }
      }
    }
    else files.push_back(argv[i]);
  // now if he didn't specify the number of threads, figure it out
  if (!threads) threads = thread::hardware_concurrency();
}

// Parse the options in a string, essentially pretending it's the
// command line.

void settings::parse_options(const string &str, vector<string> &files)
{
  istringstream is(str);
  vector<string> tokens;
  vector<string> args;
  string x;
  while (is >> x) args.push_back(x);
  parse_options(args,files);
}

// Convert a file name to a puzzle name.  Strip out the directory, if
// there is one, and the extension.  Convert to upper case.

string puzname(string fname)
{
  size_t dir = fname.rfind('/');
  if (dir != string::npos) fname = fname.substr(1 + dir);
  size_t dot = fname.rfind('.');
  if (dot != string::npos) fname = fname.substr(0,dot);
  for (auto &ch : fname) ch = toupper(ch);
  return fname;
}

// Is this the name of an ACPT puzzle?  Has to be of form [pP]uzzle[1-8][abc]?

int acpt_number(const string &fname)
{
  size_t pos = fname.find('.');
  if (pos != 7 && pos != 8) return 0;
  if (tolower(fname[0]) != 'p') return 0;
  if (fname.substr(1,5) != "uzzle") return 0;
  if (!isdigit(fname[6]) || fname[6] == '0' || fname[6] == '9') return 0;
  int base = fname[6] - '0';
  if (fname[7] == '.') return base;
  if (fname.size() <= 8 || fname[8] != '.' || fname[7] < 'a' || fname[7] > 'c')
    return 0;
  return base + tolower(fname[7]) - 'a';
}

// In tournament mode, we set the times from the puzzle file name,
// which should be "puzzlen.puz" or "puzzle8[abc].puz.  So we get the
// number, and get the time limit from that.  We decide whether we
// want to consider geometric themes (hardest puzzle only, typically),
// and print out a catty remark about how hard the puzzle is.  We set
// delta[1] appropriately.  Then set_tournament_mode does that actual
// work.  We also potentially set up a second pass for the search at
// this point, if we think the puzzle is really hard.

// Return the difficulty of the puzzle (0 = Monday, 5 = Saturday) as
// explained by Will Shortz.

struct hardness {
  unsigned limit;               // time in minutes
  unsigned do_geo;              // true or false
  unsigned pass1;               // time (or 0) for delta[1]
  int      difficult;           // basic difficulty
  bool     themeless;
  hardness(unsigned l, unsigned g, unsigned p1, int d, bool t = false) :
    limit(l), do_geo(g), pass1(p1), difficult(d), themeless(t) { }
};

const hardness default_hardness { 15, 0, 0, 2 };
const hardness acpt_hardness[10] = {
  { 15, 0, 0, 0 },              // puzzle 1
  { 25, 1, 8, 3 },              // puzzle 2
  { 30, 0, 0, 2 },              // puzzle 3
  { 20, 0, 0, 1 },              // puzzle 4
  { 30, 1, 8, 5 },              // puzzle 5
  { 30, 0, 8, 1 },              // puzzle 6
  { 60, 0, 8, 2 },              // puzzle 7
  { 15, 0, 15, 5, true },       // puzzle 8a
  { 15, 0, 8, 2, true },        // puzzle 8b
  { 15, 0, 0, 1, true }         // puzzle 8c
};

const hardness bos_hardness[7] = {
  { 15, 0, 0, 0 },              // puzzle 1
  { 20, 0, 0, 1 },              // puzzle 2
  { 20, 0, 0, 2 },              // puzzle 3
  { 35, 1, 8, 4 },              // puzzle 4
  { 15, 0, 8, 1 },              // puzzle 5
  { 20, 0, 15, 5, true },       // puzzle 6a
  { 20, 0, 0, 2, true }         // puzzle 6b
};

void settings::set_tournament_time(const string &fname, int &difficulty,
                                   bool &notheme)
{
  static const string difficulties[6] = { "EASY", "SOMEWHAT EASY", "MODERATE",
                                          "DIFFICULT",
                                          "PROBABLY TOO HARD FOR ME",
                                          "PROBABLY TOO HARD FOR ME" };
  static const unsigned PASS1_LIMIT = 30;
  unsigned puzzle_number = 0;
  const hardness *h = &default_hardness;
  for (unsigned i = 0 ; i + 8 < fname.size() ; ++i)
    if ((puzzle_number = acpt_number(fname.substr(i)))) break;
  if (!puzzle_number)
    cerr << "cannot find puzzle number in file " << fname << endl;
  else h = &(boswords? bos_hardness : acpt_hardness)[puzzle_number - 1];
  
  unsigned time_limit = h->limit;
  acpt_geometry = h->do_geo;
  if (puzzle_number) {
    if (gui) cout << "**GUI theme ";
    cout << "Puzzle " << puzzle_number << " is " << difficulties[h->difficult]
         << endl;
  }
  set_tournament_mode(time_limit,INITIAL_TIME);
  if (multipass_search) {
    delta.resize(1);
    if (h->pass1) delta.push_back(progress(-1,h->pass1,PASS1_LIMIT));
  }
  difficulty = h->difficult;
  notheme = h->themeless;
}

// Actually set tournament mode.  Args are absolute time limit
// (minutes) and initial time/time to make progress (seconds).  In
// practice, it appears to be good to make these the same integer in
// the call above!

// Also set wall to true, meaning that's the clock that matters.

void settings::set_tournament_mode(unsigned absolute_mins,
                                   unsigned relative_secs)
{
  absolute = 60 * absolute_mins;
  delta[0] = progress(-1,relative_secs);
  initial_time = relative_secs;
  wall = true;
}

// Solve a single puzzle.  Read it in, display the name, solve it, and
// then display the result if you're supposed to.

void database::solver(string fname, settings &sets) const
{
  if (sets.theme_test) {
    test_theme(fname);
    return;
  }

  puzzle p(fname);
  if (sets.tournament_mode) sets.set_tournament_time(fname,p.diff(),p.noth());
  p.limit() = sets.absolute / 60;
  if (sets.verbosity > SILENT) sets.tuneout() << puzname(fname) << endl;
  else cerr << puzname(fname) << endl;
  if (p.illegal()) cout << "illegal!" << endl;
  else {
    p.solve(*this,sets);
    if (sets.verbosity > SILENT && !sets.tune) cout << p << endl;
    if (sets.lite) p.across_lite(fname);
    if (sets.firefox) p.firefox();
  }
  if (sets.gui) cout << "**GUI complete" << endl;
  p.delete_squares();
}

// Process the files as requested.  Just call the solver on each.

void database::process_files(settings &sets, const vector<string> &files) const
{
  for (auto &f : files) solver(f,sets);
}

// Getting the xwWidgets subprocess to run properly is quite a pain.
// What we do is start it with no command line arguments, which means
// that it should get the arguments from a special file wxcheat.zzz.
// Here we process those args, assuming that we need to.

void settings::process_cheat_file(int &argc, const char **argv,
                                  vector<string> &files)
{
  ifstream inf("wxcheat.zzz");
  if (!inf) {
    inf = ifstream("wxdebug.zzz");
    if (!inf) {
      cerr << "not enough arguments; no cheat or debug file" << endl;
      abort();
    }
  }
  vector<string> fake;
  string x;
  while (inf >> x) fake.push_back(x);
  inf.close();
  remove("wxcheat.zzz");
  parse_options(fake,files);
}

// If there is special stuff to push into the GUI, do that.  (This is
// to debug the GUI.)  

bool settings::process_simulation_file()
{
  if (!gui) return false;
  ifstream inf("simulated_results.zzz");
  if (!inf) return false;
  for (string line ; getline(inf,line) ; ) cout << line << endl;
  return true;
}

// In the interactive case, we get a line from the user.  If it starts
// with q, we quit.  Otherwise, we just treat it as a line to parse,
// and then process all the files.

void database::handle_interactive(settings &sets, vector<string> &files) const
{
  if (!sets.interactive) return;
  for ( ;; ) {
    if (!sets.gui) cout << "File or command? " << flush;
    string input;
    getline(cin,input);
    if (input == "q" || input == "quit") break;
    sets.parse_options(input,files);
    process_files(sets,files);
  }
}

// Main function.  Initialize the bit vector stuff.  Parse the command
// line options and read the database.  If there are no command line
// options, read them from a cheat file as per the wxWidgets hack
// above.

// When that's done, handle some special cases (show tuning,
// interactive, theme reproduction).  If none of those returned from
// main, build the database and now you're ready to solve.  Process
// the files he's given you, and then any new ones he enters
// interactively.  When you're done, show any tuning results before
// quitting.

berkeley_solver *_bs = nullptr; // for signal handler

void terminate(int val)
{
  if (_bs) _bs->clear();
  exit(val);
}

int main(int argc, const char **argv)
{
  bitmap().initialize();
  settings sets;
  vector<string> files;
  if (argc == 1) sets.process_cheat_file(argc,argv,files);
  else {
    vector<string> args;
    for (int i = 1 ; i < argc ; ++i) args.push_back(argv[i]);
    sets.parse_options(args,files);
  }
  sets.show_tuning_parameters();

  if (sets.process_simulation_file()) return 0;

  if (sets.theme_reproduce) {
    reproduce_theme(files);
    return 0;
  }

  string py_file, py_dir = "../crossword/";
  if (sets.berkeley_max && !sets.dump_data) py_file = "semaphore.py";

  berkeley_solver bs(py_file,py_dir,sets.debug);
  _bs = &bs;
  signal(SIGINT, [](int sig) { terminate(1); } );

  database d("files/" + sets.db_source,sets.read_data,sets.dump_data,
             max(0,sets.verbosity),sets.gui);

  bs.answer_max = sets.berkeley_max;
  d.set_berkeley(bs);
  d.set_param(sets.param_file);

  if (sets.dump_data) return 0;

  if (sets.show_abbrvs) {
    d.show_abbrvs();
    return 0;
  }

  cerr << "ready to solve (" << get_cpu_time() << " secs)" << endl;

  if (sets.autotune) d.autotune(sets,files);
  else {
    d.process_files(sets,files);
    d.handle_interactive(sets,files);
    params p;
    sets.show_results(p,true);
  }

  return 0;
}
