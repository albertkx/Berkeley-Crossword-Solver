#include <chrono>
#include "xw.h"

void remove(const string &str) { remove(str.c_str()); }

void berkeley_solver::wait_for_semaphore() const
{
  remove(answer_ready);
  ofstream outf(data_ready);
  outf.close();
  for ( ;; ) {
    ifstream inf(answer_ready);
    if (inf) break;
    this_thread::sleep_for(chrono::milliseconds(100));
  }
}

// Solve the queries.  Write the queries, and set the semaphore.  Wait
// for an answer to be ready.  Then read the lines in the answer file,
// noting that an index of -1 means move along to the next query and
// only keeping answers of good lengths.  An answer that begins with a
// digit is an index into the dictionary; an answer that begins with a
// letter is fill.

vector<vector<vector<berkeley_answer>>>
berkeley_solver::solve(const vector<string> &queries) const
{
  ofstream outf(data_file);
  outf << answer_max << endl;
  for (auto &q : queries) outf << q << endl;
  outf.close();
  wait_for_semaphore();
  ifstream inf(answer_file);
  int idx;
  float sc;
  unsigned ctr = 0;
  vector<vector<vector<berkeley_answer>>> ans(queries.size());
  string line;
  while (getline(inf,line))
    if (isalpha(line[0])) {
      size_t pos = line.rfind(' '); // might be multiple words, then score
      string fill;
      for (unsigned idx = 0 ; idx < pos ; ++idx)
        if (isalpha(line[idx])) fill.push_back(tolower(line[idx]));
      unsigned sz = fill.size();
      if (ans[ctr].size() <= sz) ans[ctr].resize(1 + sz);
      ans[ctr][sz].push_back({fill,stof(line.substr(pos))});
    }
    else {
      istringstream iss(line);
      iss >> idx >> sc;
      if (idx < 0) ++ctr;       // next clue
      else {                    // index into dictionary
        unsigned sz = fills[idx].size();
        if (ans[ctr].size() <= sz) ans[ctr].resize(1 + sz);
        ans[ctr][sz].push_back({fills[idx],sc});
      }
    }
  inf.close();
  return ans;
}

// Clear the semaphore, telling Python to terminate.  Empty the data
// file and then reset the semaphore.

berkeley_solver::berkeley_solver(const string &pyfile, const string &pydir,
                                 bool keep, int amax) :
  answer_max(amax) {
  if (pyfile.empty()) return;
  string semdir = mlg_tmpdir();
  if (semdir.back() != '/') semdir.push_back('/');
  data_file    = semdir + "clues";
  answer_file  = semdir + "indices";
  data_ready   = semdir + "sem_in";
  answer_ready = semdir + "sem_out";
  ostringstream oss;
  oss << "python3 " << pydir << "semaphore.py --semaphore_directory=" << semdir;
  if (keep) oss << " --retain_semaphore=True";
  oss << " &";
  cout << "invoking " << oss.str() << endl;
  system(oss.str().c_str());
  ifstream inf(pydir + "checkpoints/wordlist.txt");
  assert(inf);
  string line;
  while (getline(inf,line)) {
    string raw = line.substr(9);
    string f;
    for (char ch : raw) if (isalpha(ch)) f.push_back(tolower(ch));
    fills.push_back(f);
  }
}

// Clear the semaphore, which should stop the Python process.

void berkeley_solver::clear()
{
  if (data_file.empty()) return; // not initialized
  cerr << "C++: clear semaphore" << endl;
  remove(data_file);
  ofstream touch(data_ready);
  touch.close();
}

// Observed probability that the correct fill is in the Berkeley set,
// as a function of length.

float total_prob(size_t len)
{
  static const vector<float> data = {
    0, 0, 0, 0.949484, 0.929801, 0.907865, 0.854938, 0.80106, 0.717669,
    0.577526, 0.439896, 0.349055, 0.266307, 0.276989, 0.220558, 0.162233
  };
  return data[min(len,data.size() - 1)];
}

// compute probabilities to scores using softmax.

float max_score(const vector<berkeley_answer> &data)
{
  float hi = 0;
  for (auto &b : data) hi = max(hi,b.value);
  return hi;
}

vector<berkeley_answer> do_softmax(unsigned len,
                                   const vector<berkeley_answer> &raw)
{
  float hi = max_score(raw);
  double sum = 0;
  for (const berkeley_answer &i : raw) sum += exp(i.value - hi);
  double mult = total_prob(len) / sum; // sum should be in total_prob
  vector<berkeley_answer> result = raw;
  for (berkeley_answer &i : result) i.value = mult * exp(i.value - hi);
  return result;
}

// Compute probabilities (NOT SCORES!) using the Berkeley model.  What
// we've computed just gets dumped into the berkeley_solver itself,
// which retains
//  results[clue-string][length] = vector<berkeley_answer>

// berkeley_solver::solve does most of the work, but all of the
// answers are scores instead of probabilities.  So we go through each
// vector<berkeley_answer> and convert the values to probabilities
// using softmax.  It's slightly tricky because the scores can be
// pretty big and the exponentiation introduces errors.  So we find
// the smallest score in each group and reduce by that before doing
// the softmax.

void berkeley_solver::get_probs(const vector<string> &clues) const
{
  vector<vector<vector<berkeley_answer>>> res = solve(clues);
  for (unsigned i = 0 ; i < res.size() ; ++i) {
    raw[clues[i]] = res[i];
    results[clues[i]].resize(res[i].size());
    for (unsigned len = 0 ; len < res[i].size() ; ++len) {
      if (!res[i][len].empty())
        results[clues[i]][len] = do_softmax(len,res[i][len]);
    }
  }
}

// The money function.  Go through all the words in the puzzle,
// getting each clue.  Don't reprocess anything we've done before.

void berkeley_solver::get_berkeley_results(const puzzle &puz) const
{
  if (data_file.empty()) return; // not initialized
  vector<string> clues;
  for (auto &w : puz) {
    const string &c = w.clue;
    if (results.find(c) == results.end()) clues.push_back(strip_braces(c));
  }
  if (!clues.empty()) get_probs(clues);
}
