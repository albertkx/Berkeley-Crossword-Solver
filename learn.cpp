#include "xw.h"
#include <unistd.h>

static const string PYTHON_MODEL = "tfc.pkl"; // learned model, in Python

// Use learning to revalue all the scores.  At a high level, we'll
// build a bunch of learning_datum's and then have to use the python
// code to convert them all to scores.  A couple of points worth
// nothing:

// 1. We *don't* want to multithread the python part; it's already
// multithreaded in python itself.

// 2. Most words won't have clue support, but those words should be
// pretty simple and straight probabilities.  So all we have to figure
// out is how to deal with the words that are supported by the clue.
// But we *do* need to fold the new scorees into the old ones when
// we're done.  Note that it is also very difficult to *make* a model
// from just the dictionary stuff, since the sample will really be
// ridiculously biased.

void filler::learning_data(unsigned diff, const word &w,
                           vector<learning_datum> &lds, vector<string> &fills)
  const
{
  bool dn = (w.desc().back() == 'D');
  float best = nth_best_value(0).score;
  for (unsigned i = 0 ; 1 + i < fills_by_score.size() ; ++i) {
    auto sw = nth_best_value(i);
    if (sw.clue_val() == 0) break;
    fills.push_back(nth_best_fill(i));
    lds.push_back(learning_datum(sw,diff,w.size(),dn,numwords,best,i,false));
  }
}

// Rescore words for a single filler.  Basically, we have a bnuch of
// fills, which have scores.  Those scores correspond to logs of
// "probabilities" that add to some total ptot.  We want to reassign
// the scores proportionally so that the total is unchanged but the
// ratios match the given values.

void filler::rescore(const vector<string> &fills, vector<double> &vals,
                     unsigned lo, unsigned hi)
{
  for (unsigned idx = lo ; idx < hi ; ++idx)
    if (vals[idx] > dat->get_param().detail(LCUTOFF))
      fills_by_word[fills[idx]].score -= dat->get_param().detail(LDELTA);
  scores_are_computed(false);   // don't adjust again!
}

void search_node::threaded_recomputer(dispatcher &d, const vector<string> &fills,
                                      vector<double> &vals,
                                      const vector<unsigned> &indices)
{
  unsigned n;
  while ((n = d.next()) != unsigned(-1))
    (*this)[n].fp->wfiller.rescore(fills,vals,indices[n],indices[1 + n]);
}

//  As we do this, we use rescored to record which words have had
// changes, and then rescore for each filler makes the appropriate
// changes to the values.

string mlg_tmpfile()
{
  char f[L_tmpnam];
  strcpy(f,"/tmp/xwXXXXXX");
  close(mkstemp(f));
  return f;
}

vector<double> analyze_clues(const vector<learning_datum> &data)
{
  string query = mlg_tmpfile(), result = mlg_tmpfile();
  ofstream outf(query);
  extern string learning_headers();
  istringstream iss(learning_headers());
  string junk, tok;
  iss >> junk >> junk;
  outf << iss.str().substr(1 + iss.tellg()) << endl; // space
  for (auto &d : data) outf << d << endl;
  string cmd = "python3 predict.py " + PYTHON_MODEL + " " + query + " " + result;
  system(cmd.c_str());
  vector<double> ans;
  ifstream inf(result);
  while (inf >> tok) ans.push_back(stod(tok.substr(tok[0] == '[')));
  remove(query.c_str());
  remove(result.c_str());
  assert(ans.size() == data.size());
  return ans;
}

void search_node::revalue_with_learning()
{
  vector<int> pre(size()), post(size());
  for (unsigned i = 0 ; i < size() ; ++i)
    pre[i] = (*this)[i].fp->wfiller.position((*this)[i].answer);
  vector<learning_datum> inputs;
  vector<string> fills;
  vector<unsigned> indices;             // index into inputs for each word
  for (unsigned i = 0 ; i < size() ; ++i) {                             // 1
    indices.push_back(inputs.size());
    word &w = (*this)[i];
    filler &f = w.fp->wfiller;
    if (f.ready_to_learn) f.learning_data(difficulty,w,inputs,fills);
    assert(inputs.size() == fills.size());
  }
  indices.push_back(inputs.size());

  vector<double> new_scores = analyze_clues(inputs);
  if (sets->threads > 1) {
    vector<thread> threads;
    dispatcher disp(size());
    for (unsigned i = 0 ; i < min(unsigned(size()),sets->threads) ; ++i)
      threads.push_back(thread([&]() {
            threaded_recomputer(disp,fills,new_scores,indices);
          }));
    for (auto &th : threads) th.join();
  }
  else for (unsigned i = 0 ; i < size() ; ++i)
         (*this)[i].fp->wfiller.rescore(fills,new_scores,indices[i],
                                        indices[1 + i]);
}
