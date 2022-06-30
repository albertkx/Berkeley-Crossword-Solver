#include "xw.h"

// The basic operation here is to add a new level to an expansion
// tree.  In order to do this [expand(tree)] we have to look at all
// the expansions at the previous level and expand each.

// In order to expand a tree given a string we're trying to expand
// [expand(tree,src)], we have to consider all subsequences of src,
// which we identify in terms of position (pos) and length (len).

// To expand a specific subsequence [expand(tree,src,pos,len)] we
// first pull out the subsequence replaceme that we are replacing.
// Then for each replacement, we build up the new sequence.  If it
// already appears at any level, we ignore it.  If it doesn't appear,
// we add it to the top level of the expansion_tree.

void database::expand(expansion_tree &tree, const vstring &src, unsigned pos,
                      unsigned len) const
{
  vstring replaceme({ src.begin() + pos, src.begin() + pos + len });
  if (translations.find(replaceme) == translations.end()) return;
  for (auto &rep : translations.at(replaceme)) {
    vstring newseq({ src.begin(), src.begin() + pos });
    newseq.insert(newseq.end(),rep.begin(),rep.end());
    newseq.insert(newseq.end(),src.begin() + pos + len,src.end());
    bool found = false;
    for (auto &lev : tree) if (lev.find(newseq) != lev.end()) {
        found = true;
        break;
      }
    if (!found) tree.back().insert(newseq);
  }
}

void database::expand(expansion_tree &tree, const vstring &src) const
{
  for (unsigned pos = 0 ; pos + 1 <= src.size() ; ++pos)
    for (unsigned len = 1 ; pos + len <= src.size() ; ++len)
      expand(tree,src,pos,len);
}

void database::expand(expansion_tree &tree) const
{
  tree.push_back(set<vstring>());
  for (auto &vs : tree[tree.size() - 2]) expand(tree,vs);
}

// Here we find the answers (represented as word IDs) to a given clue
// (represented as a vstring).  In the no-modification case, we just
// build the expansion tree and then return all one-word answers
// found.

vector<unsigned> database::find_fills(const vstring &clue, unsigned lim) const
{
  expansion_tree tree(clue);
  while (tree.size() <= lim) expand(tree);
  vector<unsigned> ans;
  for (auto &s : tree)
    for (auto &vs : s) if (vs.size() == 1) ans.push_back(vs[0]);
  return ans;
}

// In the yes-modification case, we get all the transitions associated
// with any clue word.  For each, we modify the clue and then find the
// fills.  For each fill, we try to invert the transition to get a
// fill for the original clue.

vector<unsigned> database::find_fills_with_construction(const vstring &clue,
                                                        unsigned lim) const
{
  vstring copy = clue;
  vector<unsigned> ans;
  for (unsigned i = 0 ; i < copy.size() ; ++i) {
    if (transitions.find(copy[i]) == transitions.end()) continue;
    for (auto &p : transitions.at(copy[i])) {
      unsigned tmp = copy[i];
      copy[i] = p.first;
      for (unsigned f : find_fills(copy,lim))
        if (reversals.find(f) != reversals.end()) 
          for (auto &r : reversals.at(f))
            if (r.second == p.second) ans.push_back(r.first);
      copy[i] = tmp;
    }
  }
  return ans;
}

// Analogous functions but dealing with strings.  Just convert the
// clue, and then convert the answer.  In the exact-match case, you
// have to be a little careful because you don't want an answer back
// that includes a non-alpha character.

vector<string> database::find_fills(const string &clue, unsigned lim) const
{
  vector<string> cluevec = raw_tokenize(clue);
  vector<unsigned> cluenums;
  for (auto &str : cluevec) {
    for (char &ch : str) ch = tolower(ch);
    unsigned wnum = find_word(str);
    if (wnum == unsigned(-1)) return vector<string>();
    cluenums.push_back(wnum);
  }
  vector<string> ans;
  for (unsigned f : find_fills(cluenums,lim)) {
    string fs = to_string(f);
    if (all_of(fs.begin(),fs.end(),[](char c) { return isalpha(c);}))
      ans.push_back(fs);
  }
  return ans;
}

vector<string> database::find_fills_with_construction(const string &clue,
                                                      unsigned lim) const
{
  vector<string> cluevec = raw_tokenize(clue);
  vector<unsigned> cluenums;
  for (auto &str : cluevec) {
    for (char &ch : str) ch = tolower(ch);
    unsigned wnum = find_word(str);
    if (wnum == unsigned(-1)) return vector<string>();
    cluenums.push_back(wnum);
  }
  vector<string> ans;
  for (unsigned f : find_fills_with_construction(cluenums,lim))
    ans.push_back(to_string(f));
  return ans;
}

