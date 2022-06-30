#include <sys/resource.h>
#include <iterator>
#include "xw.h"
#include <unistd.h>

// Some POS augmentation for the dictionary.  Basic POS stuff is from
// wordnet but here we add prepositions (which wordnet does not do!).
// Also names for the various parts of speech.

vector<string> pos_names {
  "root_noun", "plural_noun",
    "root_adverb", "nonroot_adverb",
    "root_verb", "participle", "sing_verb", "past_verb",
    "root_adj", "most", "more", "constructed_adj",
    "article", "prep", "conj", "poss", "blank", "unknown"
};

// Part of speech, reconstructed from name

POS pos_from_string(const string &s)
{
  for (unsigned ans = ROOT_NOUN ; ans != END_POS ; ++ans)
    if (pos_names[ans] == s) return POS(ans);
  return END_POS;
}

ostream &operator<<(ostream &os, const parse &p)
{
  for (auto &c : p) os << (&c == &*p.begin()? "" : " ") << pos_names[c];
  return os;
}

static const map<string,string> contractions {
  { "aren't", "are not" }, { "Aren't", "Are not" },
  { "can't", "cannot" }, { "Can't", "Cannot" },
  { "could've", "could have" }, { "Could've", "Could have" },
  { "couldn't", "could not" }, { "Couldn't", "Could not" },
  { "didn't", "did not" }, { "Didn't", "Did not" },
  { "doesn't", "does not" }, { "Doesn't", "Does not" },
  { "don't", "do not" }, { "Don't", "Do not" },
  { "hadn't", "had not" }, { "Hadn't", "Had not" },
  { "hasn't", "has not" }, { "Hasn't", "Has not" },
  { "haven't", "have not" }, { "Haven't", "Have not" },
  { "he'd", "he would" }, { "He'd", "He would" },
  { "he'll", "he will" }, { "He'll", "He will" },
  { "he's", "he is" }, { "He's", "He is" },
  { "how'd", "how did" }, { "How'd", "How did" },
  { "how'll", "how will" }, { "How'll", "How will" },
  { "how's", "how is" }, { "How's", "How is" },
  { "I'd", "I had" },
  { "I'll", "I will" },
  { "I'm", "I am" },
  { "I've", "I have" },
  { "isn't", "is not" }, { "Isn't", "Is not" },
  { "it'd", "it would" }, { "It'd", "It would" },
  { "it'll", "it will" }, { "It'll", "It will" },
  { "it's", "it is" }, { "It's", "It is" },
  { "let's", "let us" }, { "Let's", "Let us" },
  { "might've", "might have" }, { "Might've", "Might have" },
  { "must've", "must have" }, { "Must've", "Must have" },
  { "needn't", "need not" }, { "Needn't", "Need not" },
  { "shan't", "shall not" }, { "Shan't", "Shall not" },
  { "she'd", "she would" }, { "She'd", "She would" },
  { "she'll", "she will" }, { "She'll", "She will" },
  { "she's", "she is" }, { "She's", "She is" },
  { "should've", "should have" }, { "Should've", "Should have" },
  { "shouldn't", "should not" }, { "Shouldn't", "Should not" },
  { "that'd", "that would" }, { "That'd", "That would" },
  { "that's", "that is" }, { "That's", "That is" },
  { "there'd", "there would" }, { "There'd", "There would" },
  { "there've", "there have" }, { "There've", "There have" },
  { "there's", "there is" }, { "There's", "There is" },
  { "they'd", "they would" }, { "They'd", "They would" },
  { "they'll", "they will" }, { "They'll", "They will" },
  { "they're", "they are" }, { "They're", "They are" },
  { "they've", "they have" }, { "They've", "They have" },
  { "wasn't", "was not" }, { "Wasn't", "Was not" },
  { "we'd", "we would" }, { "We'd", "We would" },
  { "we'll", "we will" }, { "We'll", "We will" },
  { "we're", "we are" }, { "We're", "We are" },
  { "we've", "we have" }, { "We've", "We have" },
  { "weren't", "were not" }, { "Weren't", "Were not" },
  { "what'll", "what will" }, { "What'll", "What will" },
  { "what're", "what are" }, { "What're", "What are" },
  { "what's", "what is" }, { "What's", "What is" },
  { "when's", "when is" }, { "When's", "When is" },
  { "where'd", "where did" }, { "Where'd", "Where did" },
  { "where's", "where is" }, { "Where's", "Where is" },
  { "where've", "where have" }, { "Where've", "Where have" },
  { "who'll", "who will" }, { "Who'll", "Who will" },
  { "who's", "who is" }, { "Who's", "Who is" },
  { "why's", "why has" }, { "Why's", "Why has" },
  { "won't", "will not" }, { "Won't", "Will not" },
  { "would've", "would have" }, { "Would've", "Would have" },
  { "wouldn't", "would not" }, { "Wouldn't", "Would not" },
  { "y'all", "you all" }, { "Y'all", "You all" },
  { "you'd", "you would" }, { "You'd", "You would" },
  { "you'll", "you will" }, { "You'll", "You will" },
  { "you're", "you are" }, { "You're", "You are" },
  { "you've", "you have" }, { "You've", "You have" },
    };

// remove the contractions in a string

string uncontract(const string &str)
{
  istringstream ist(str);
  string tok, ans;
  while (ist >> tok) {
    if (!ans.empty()) ans += ' ';
    bool up = isupper(tok[0]);
    if (up) tok[0] = tolower(tok[0]);
    size_t n = ans.size();
    ans += (contractions.find(tok) == contractions.end())? tok :
      contractions.at(tok);
    if (up) ans[n] = toupper(ans[n]);
  }
  return ans;
}

// timing utilities.  Note you can work with either wall time or CPU time.

double get_cpu_time(void)
{
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  return ( ru.ru_utime.tv_sec +
           ru.ru_utime.tv_usec/1000000.0 +
           ru.ru_stime.tv_sec +
           ru.ru_stime.tv_usec/1000000.0 );
}

double get_time(bool wall, bool reset)
{
  static chrono::time_point<chrono::system_clock> wall_start_time;
  static double cpu_start_time;
  if (!reset)
    if (wall)
      return (chrono::system_clock::now() - wall_start_time).count() / TICKS;
    else return get_cpu_time() - cpu_start_time;
  else if (wall) wall_start_time = chrono::system_clock::now();
  else cpu_start_time = get_cpu_time();
  return 0;                     // reset case
}

// open a file, making sure that it's actually there

FILE *checked_fopen(const string &str, const string &mode)
{
  FILE *ans = fopen(str.c_str(),mode.c_str());
  if (!ans) {
    cerr << "cannot open " << str << " with mode " << mode << endl;
    terminate(1);
  }
  return ans;
}

// Many times we want to take a string (a clue, fill, or what have
// you) and rework it to be lower case and take out junk.  Here are
// functions that do that, depending on whether or not you want to
// include digits in what's kept.

string fill_no_digits(const string &s)
{
  string ans;
  ans.resize(s.size());
  unsigned i = 0;
  for (auto ch : s) if (isalpha(ch)) ans[i++] = tolower(ch);
  ans.resize(i);
  return ans;
}

string fill_digits(const string &s)
{
  string ans;
  ans.resize(s.size());
  unsigned i = 0;
  for (auto ch : s) if (isalnum(ch)) ans[i++] = tolower(ch);
  ans.resize(i);
  return ans;
}

string strip_braces(const string &s)
{
  string ans;
  ans.resize(s.size());
  unsigned i = 0;
  for (auto ch : s) if (ch != '{' && ch != '}') ans[i++] = ch;
  ans.resize(i);
  return ans;
}

// When we do the lookup for ASKOF, we have to be careful not to crash
// the compression stuff.  So this takes a string (like lparen above)
// and removes spaces; if there is something that isn't a letter or a
// space, we return a failure marker.

bool strip_spaces(string &str)
{
  int i = 0;
  for (auto ch : str)
    if (isalpha(ch)) str[i++] = tolower(ch);
    else if (!isspace(ch)) return false;
  str.resize(i);
  return true;
}

// Destructively strip the quotes from a string.  If the string is
// *entirely* inside of quotes, make it the empty string.  Otherwise,
// if you can find \ "..." \ or \ '...' \ (matching quotes with
// leading and trailing spaces), replace that substring with "aa"
// (which will be parsed as a singular noun).  Tricky bit: replace 's
// with "possmarker", which means str might get longer (space needs to
// have been reserved!), and we have to do it all with copying.
// Similarly, we replace ___ with "blankinclue".

// We could presumably do all of this with regex_replace but it
// doesn't appear that the code would be really either simpler or
// faster.

bool isquote(char c) { return c == '"' || c == '\''; }

string remove_quotes(const string &str, bool possessives_only,
                     bool include_possessives)
{
  if (!possessives_only && isquote(str[0]) && str.back() == str[0])
    return string();
  string result;
  for (unsigned i = 0 ; i < str.size() ; ++i) 
    if (!possessives_only && i && str[i - 1] == ' ' && isquote(str[i])) {
      // i = 0 and a partial quote seems to make performance worse!
      // (not picking up some instances, perhaps?)
      unsigned j;
      for (j = i + 1 ; j < str.size() ; ++j)
        if (str[j] == str[i] && ((j + 1 == str.size()) || isspace(str[j + 1]))) {
          result += "aa";       // stick in "aa", a noun
          i = j;
          break;
        }
      if (j == str.size()) result.push_back(str[i]); // no match
    }
    else if (str[i] == '\'' && i + 1 < str.size() && str[i + 1] == 's' && 
             (i + 2 == str.size() || isspace(str[i + 2]))) {
      if (include_possessives) result += " possmarker";
      ++i;                      // skip the s
    }
    else if (str[i] == '\'' && (i + 1 == str.size() || isspace(str[i + 1]))) {
      if (include_possessives) result += " possmarker";
    }
    else if (str[i] == '_' && str[i + 1] == '_' && str[i + 2] == '_') {
      if (include_possessives) result += " blankinclue";
      i += 2;
    }
    else if (str[i] == '-') result.push_back(' ');
    else result.push_back(str[i]);
  return result;
}

// Two strings are anagrams if they match after sorting.  Note call is
// not by reference.

bool is_anagram(string s1, string s2)
{
  if (s1.size() != s2.size()) return false; // faster
  sort(s1.begin(),s1.end());
  sort(s2.begin(),s2.end());
  return s1 == s2;
}

// Tokenization.  There are basically three functions that need to do
// it.

// 1. For clues, digits may or may not be ok, but we want to leave the
// quotes in.

// 2. For POS analysis, we are tokenizing a clue string to convert it
// into a bunch of parts of speech, so we need the words only, and we
// want to remove the quotes.

// 3. For statistics, we treat them the same as the clues but remove
// the quotes.

vector<string> raw_tokenize(const string &str)
{
  size_t pos = str.find('-');
  if (pos == string::npos) {    // don't need to make a copy
    istringstream is(str);
    return vector<string>(istream_iterator<string>(is),
                          istream_iterator<string>());
  }

  string tmp = str;
  for (size_t i = pos ; i < tmp.size() ; ++i) if (tmp[i] == '-') tmp[i] = ' ';
  istringstream is(tmp);
  return vector<string>(istream_iterator<string>(is),
                        istream_iterator<string>());
}

vector<string> clue_tokenize(const string &str, bool allow_digits)
{
  vector<string> tokens = raw_tokenize(str);
  for (size_t i = 0 ; i < tokens.size() ; ++i)
    if (allow_digits) tokens[i] = fill_digits(tokens[i]);
    else tokens[i] = fill_no_digits(tokens[i]);
  return tokens;
}

vector<string> pos_tokenize(const string &str)
{
  return clue_tokenize(remove_quotes(uncontract(str)),false);
}

vector<string> stats_tokenize(const string &str)
{
  return clue_tokenize(remove_quotes(str,true,false),true);
}

// Number of words in a string

unsigned countwords(const string &str)
{
  return 1 + count(str.begin(),str.end(),' ');
}

// Convert the phoneme list giving the pronunciation to something
// better suited to use by flamingo.  That mostly means that the
// spaces have to go, but in that case we have ambiguity from the
// phonemes ?G and ?H, which we replace with ?1 and ?2 respectively.

// So to convert *from* a list of phonemes, we drop all the stress
// markers and spaces, and convert G/H not following a space.

string pstring_to_string(const string &str)
{
  string ans;
  ans.resize(str.size());
  unsigned j = 0;
  for (size_t i = 0 ; i < str.size() ; ++i)
    if (isalpha(str[i])) {
      if (i && !strchr(" AEIOU",str[i - 1]))
        switch (str[i]) {
        case 'g': case 'G': ans[j++] = 'z' + 1; break;
        case 'h': case 'H': ans[j++] = 'z' + 2; break;
        default: ans[j++] = tolower(str[i]);
        }
      else ans[j++] = tolower(str[i]);
    }
  ans.resize(j);
  return ans;
}

// To convert *to* a list of phonemes, we replace the 1 and 2 with G
// or H (always without a space), and push a space after any nonvowel
// or any character that is the second character in a phoneme.

string string_to_pstring(const string &str)
{
  string ans;
  for (auto ch : str)
    switch (ch) {
    case 'z' + 1: ans.push_back('G'); break;
    case 'z' + 2: ans.push_back('H'); break;
    default: 
      if (ans.size() && !strchr("AEIOU",ans.back())) ans.push_back(' ');
      else if (ans.size() > 1 && ans[ans.size() - 2] != ' ') ans.push_back(' ');
      ans.push_back(toupper(ch));
    }
  return ans;
}

// Call to t2p.  One call to flite setup, and then you're good to go.

extern "C" {
  void flite_setup();
  char *pronounce(const char *text, char *pron); 
}

string pronounce(const string &s)
{
  static unsigned BIG = 5000;
  char p[BIG];                  // not static in case need to be thread safe
  static bool init = false;
  if (!init) { flite_setup(); init = true; }
  pronounce(s.c_str(),p);
  return p;
}

// Convert a clue for POS analysis.  Remove any "make generic" phrase
// at the end, then remove the quotes and undo any contractions

static const vector<string> comma_suffix { 
  "e.g.", "perhaps", "for one", "for short", "briefly", "in a way", "maybe",
  "say", "often", "once", "slangily", "informally", "at times", "e.g.", "as a",
  "for example", "sometimes", "so to speak", "familiarly", "etc.", "in slang",
  "usually", " for one", "poetically", "e.g.: Abbr.", "in music", "notably",
  "old-style", "formerly", "essentially", "today", "et al.", "part 2",
  "in verse", "for instance", "in brief", "for two", "part 3", "in poetry",
  "to some", "of a sort", "initially", "to Caesar", "for some", "now",
  "casually", "in Paris", "in France"
};

string drop_comma(const string &str)
{
  string nocomma = str;
  size_t cm = str.rfind(',');
  if (cm == string::npos || cm + 2 >= str.size()) return str;
  string s = str.substr(cm + 2);
  if (find(comma_suffix.begin(),comma_suffix.end(),s) != comma_suffix.end())
    return str.substr(0,cm);
  return str;
}

string clue_for_pos(const string &clue)
{
  return uncontract(remove_quotes(drop_comma(clue)));
}

bool is_lead_follow(const string &clue)
{
  static vector<string> lead_follow_phrases = {
    "leader", "leadin", "opener", "opening", "front",
    "follower", "introduction", "word before",
    "word after", "prefix with", "prefix before", "suffix after",
    "suffix with"
  };
  if (clue.find(' ') == string::npos) return false;
  string str = tolower(clue);
  for (unsigned i = 0 ; i < lead_follow_phrases.size() ; ++i)
    if (str.find(lead_follow_phrases[i]) != string::npos) return true;
  return false;
}

string mlg_tmpfile(const string ext, const string dir)
{
  char f[500];
  string base = dir;
  if (base.back() != '/') base.push_back('/');
  base += "fileXXXXXX" + ext;
  strcpy(f,base.c_str());
  close(mkstemps(f,ext.size()));
  return f;
}

string mlg_tmpdir()
{
  char f[L_tmpnam];
  strcpy(f,"/tmp/dirXXXXXX");
  return mkdtemp(f);
}

// Send a string to an assumed-to-be-open Firefox.

void send_to_firefox(const string &str)
{
#ifdef __linux__
  system("xdotool search \"Mozilla Firefox\" > /tmp/windowid");
  ifstream inf("/tmp/windowid");
  size_t id;
  inf >> id;
  inf.close();
  remove("/tmp/windowid");
  ostringstream cmd;
  cmd << "xdotool key --delay 1 --window " << id;
  for (char c : str)
    if (c == '\t') cmd << " Tab";
    else cmd << ' ' << c;
  system(cmd.str().c_str());
#else
  ostringstream cmd;
  cmd << "osascript -e 'tell app \"Firefox\" to activate' "
      << "-e 'tell app \"System Events\" to keystroke \""
      << str << "\"'";
  cout << cmd.str() << endl;
  system(cmd.str().c_str());
#endif
}
