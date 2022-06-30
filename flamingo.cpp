#include "score.h"
#include "topk.h"

struct Flamingo {
  GramGenFixedLen grammar;
  Topk::Index     index;

  Flamingo() : grammar(4) { }
};

// Build the Flamingo database.  For index 1 (the sounds), go through
// all the Flamingo words[0] and get the pronunciation.  Stick all
// possible pronunciations onto flamingo_words[1].

void database::build_flamingo_weights()
{
  size_t w = max(flamingo_words[0].size(),flamingo_words[1].size());
  flamingo_weights = vector<float>(w,1);
}

void database::flamingo_build()
{
  set<string> all_prons;
  for (auto &f : flamingo_words[0]) {
    vector<string> p = pronunciations[f];
    all_prons.insert(p.begin(),p.end());
  }
  flamingo_words[1] = vector<string>(all_prons.begin(),all_prons.end());
  for (size_t i = 0 ; i < 2 ; ++i) {
    flamingo[i] = new Flamingo;
    flamingo[i]->index.build(&flamingo_words[i][0],
                             &flamingo_words[i][0] + flamingo_words[i].size(),
                             flamingo[i]->grammar);
  }
  build_flamingo_weights();
}

void database::flamingo_dump(const string &fname, bool sound) const
{
  flamingo[sound]->index.save(fname);
}

void database::flamingo_read(const string &fname, bool sound)
{
  flamingo[sound] = new Flamingo;
  flamingo[sound]->index.load(fname);
}

// Best to_match matches for the given fill; if check_exists is true
// and the fill is a flamingo_word, return empty list

vector<string> database::flamingo_match(const string &fill, bool sound,
                                        unsigned to_match, bool check_exists)
  const
{
  if (flamingo_cache[sound].find(fill) != flamingo_cache[sound].end()) 
    return flamingo_cache[sound][fill];

  if (check_exists && is_flamingo_word(fill,sound)) return vector<string>();

  unsigned topk[to_match];
  SimMetricEdNorm simMetric(flamingo[sound]->grammar);
  Query query(fill,simMetric,to_match);
  Topk::IndexQuery topkIndexQuery(flamingo[sound]->index,query);
  Topk::Heap::getTopk(&flamingo_words[sound][0],
                      &flamingo_weights[0],
                      flamingo[sound]->index,
                      query,
                      topkIndexQuery, 
                      topk);
  vector<string> ans;
  for (size_t i = 0 ; i < to_match ; ++i) 
     if (topk[i] < flamingo_words[sound].size())
       ans.push_back(flamingo_words[sound][topk[i]]);
  return (flamingo_cache[sound][fill] = ans);
}

// Find matches but don't include plurals, which mess up the theme
// identification stuff substantially.  Call flamingo_match and then
// look for pairs where one word adds an s (or a z if it's
// sound-based).  If that happens, remove whichever word is more of a
// mismatch (length-wise) for the original fill.

vector<string> database::flamingo_match_no_plurals(const string &fill,
                                                   bool sound,
                                                   unsigned to_match,
                                                   bool check) const
{
  vector<string> f = flamingo_match(fill,sound,to_match,check);
  for (unsigned i = 0 ; i < f.size() ; ++i)
    if (f[i].back() == 's') {
      string sing = f[i];
      sing.pop_back();
      if (find(f.begin(),f.end(),sing) == f.end() &&
          word_to_score[sing] >= flamingo_cutoff)
        f.push_back(sing);
    }
  set<unsigned> to_remove;
  for (unsigned i = 0 ; i < f.size() ; ++i)   // longer
    for (unsigned j = 0 ; j < f.size() ; ++j) // shorter
      if ((f[i].size() == f[j].size() + 1) &&
          ((f[i].back() == 's') || (sound && f[i].back() == 'z')) &&
          f[i].substr(0,f[j].size()) == f[j]) {
        if (f[i] == fill) to_remove.insert(j);
        else to_remove.insert(i);
        break;
      }
  unsigned j = 0;
  for (unsigned i = 0 ; i < f.size() ; ++i)
    if (to_remove.find(i) == to_remove.end()) f[j++] = f[i];
  f.resize(j);
  return f;
}

// Flamingo replacement for the given fill.  Just call flamingo_match
// and then filter to only keep words of the same length.

vector<string> database::flamingo_choice(const string &fill, unsigned to_match)
  const
{
  if (!to_match) return vector<string>(1,fill);
  vector<string> f = flamingo_match(fill,false,to_match,false);
  unsigned j = 0;
  for (auto &i : f) if (i.size() == fill.size()) f[j++] = i;
  f.resize(j);
  if (f.empty()) f.push_back(fill);
  return f;
}
