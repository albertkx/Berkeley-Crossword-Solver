#include "score.h"

// Build a pdata from a stream.  Just read through it, remembering
// that the probabilities are in parens so you have to skip the first
// character.

pdata::pdata(unsigned c, istringstream &ist) : ct(c)
{
  memset(pos,0,END_POS * sizeof(float));
  string p, prob;
  while (ist >> p >> prob) pos[pos_from_string(p)] = stof(prob.substr(1));
}

// Build a pdata from a map from POS to probabilities in the obvious
// way.

pdata::pdata(const map<char,float> &m) : ct(0)
{
  memset(pos,0,END_POS * sizeof(float));
  for (auto &p : m) if (p.second) {
      ct = 1;                   // no real meaning, just nonzero
      pos[int(p.first)] = p.second;
    }
}

// merge one pdata into a map<char,float> where the map is the maximum
// over a variety of pdata.

void pdata::merge(map<char,float> &res) const
{
  if (ct == 0) return;
  for (unsigned i = 0 ; i < END_POS ; ++i)
    if (pos[i]) res[i] = max(res[i],pos[i]);
}

/* Normalization is easy. Make the biggest entry 1 and rescale the
rest.  */

void pdata::normalize()
{
  if (ct == 0) return;
  float mx = 0;
  for (unsigned i = 0 ; i < END_POS ; ++i) mx = max(mx,pos[i]);
  for (unsigned i = 0 ; i < END_POS ; ++i) pos[i] /= mx;
}

float pdata::mass() const
{
  float ans = 0;
  for (unsigned i = 0 ; i < END_POS ; ++i) ans += pos[i];
  return ans;
}

/* For POS analysis, the basic function pos_prob(c) computes the
penalty for a fill given a pdata.  If the clue could not be parsed,
the ct on the pdata is 0 to indicate that.  Now we just take all the
possible POSs for the fill word w (if there aren't any, we return 1 to
indicate no change in probability), and take the probability of the
most likely match.  The pdata will already have been set up so that
all of the probabilities are relative to the maximum.  */

float database::pos_prob(const string &w, const pdata &p) const
{
  if (p.ct == 0) return 1;
  const vector<POS> &v = pos(w);
  if (v[0] == UNKNOWN) return 1; // trying to actually analyze these is a mistake
  return pos_prob(p,v);
}

float database::pos_prob(const pdata &p, const vector<POS> &pos_list) const
{
  float ans = 0;
  for (auto pos : pos_list) ans = max(ans,p.pos[int(pos)]);
  return ans;
}

/* Here we analyze a clue (that has been simplified for thia already)
and produce a pdata.  We first build the parse of the clue itself,
creating a vector<vector<char>> where each element is a list of
possible POS's for the associated clue word.  (If the clue word is a
proper noun, we do whatever the nouner argument asks.)  As we do this,
we keep the first word of the clue in fword and the last word in
lword.  There is also a failure marker to indicate that we couldn't
parse the clue.

Having completed the pre-parse, serialize does the rest.  */

vector<parse> database::all_parses(const string &clue, PNoun nouner,
                                   string &fword, string &lword) const
{
  istringstream isc(clue);
  bool first = true;
  string w;
  vector<vector<POS>> pos_lists;
  bool failed = false;
  while (isc >> w) {
    if (!first && isupper(w[0])) { // proper noun
      bool nouned = false;
      switch (nouner) {
      case FAIL: return vector<parse>();
      case NOUNIFY: 
        pos_lists.push_back({ ROOT_NOUN });
        nouned = true;
        break;
      case ACCEPT: break;                         // just fall through
      }
      if (nouned) continue;
    }
    w = fill_digits(w);
    if (first) fword = w;
    lword = w;
    first = false;
    if (w.empty() || parts_of_speech.find(w) == parts_of_speech.end())
      failed = true;
    else {
      const vector<POS> pos = parts_of_speech.at(w);
      for (POS p : pos) if (p >= BLANK) failed = true;
      pos_lists.push_back(pos);
    }
  }
  if (failed) return vector<parse>();
  vector<vector<POS>> result = serialize(pos_lists);
  vector<parse> ans;
  for (auto &r : result) ans.push_back(r);
  return ans;
}

// Convert a clue to a pdata.  If FITB or lead/follow, nothing to do.
// Otherwise, simplify the clue, and all_parses does most of the work.
// Merge all the results and normalize the result relative to the
// maximum.

pdata database::cpos_prob(const string &clue) const
{
  if (clue.find("___") != string::npos || is_lead_follow(clue)) return pdata();
  string adj = uncontract(remove_quotes(drop_comma(clue)));
  string fword, lword;
  vector<parse> pvec = all_parses(adj,NOUNIFY,fword,lword);
  if (pvec.empty()) return pdata();
  if (pvec[0].size() == 1) {           // one-word clue
    pdata ans(1);                      // nonzero is all that matters
    for (auto &p : pvec) ans.pos[int(p[0])] = 1;
    return ans;
  }
  else if (pvec.size() == 1 && pos_rules.find(pvec[0]) != pos_rules.end())
    return pos_rules.at(pvec[0]);
  else if (!fword.empty() && first_pos.includes(fword))
    return first_pos[fword];
  else if (!lword.empty() && last_pos.includes(lword))
    return last_pos [lword];
  return pdata();
}

/*  Here we combine all the parses into an unscaled pdata.  pos_rules
gives a pdata for each possible parse in isolation.  */

void database::fold_parses(const vector<parse> &parses, pdata &res) const
{
  for (auto &p : parses) {
    auto j = pos_rules.find(p);
    if (j == pos_rules.end()) continue; // couldn't find it
    const pdata &pd = j->second;
    res.ct += pd.ct;
    for (unsigned k = 0 ; k < END_POS ; ++k) res.pos[k] += pd.ct * pd.pos[k];
  }
}
