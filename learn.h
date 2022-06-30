#ifndef _LEARN_
#define _LEARN_

#include <map>
#include <vector>
#include <string>

using namespace std;

struct learning_datum;

// C++ part of the interface to the Python learner.  We can't include
// Python.h in the main files because it redefines tolower.  That
// means we can't include any Python stuff, hence the void *s.

// An lclue has the clue as a string and the desired length of the fill.

struct lclue {
  string clue;
  unsigned len;
  lclue(const string &c, unsigned l) : clue(c), len(l) { }
  bool operator<(const lclue &other) const {
    return (len == other.len)? (clue < other.clue) : (len < other.len);
  }
};

// Results of learning: a map from lclues and fills to scores.

struct learning : map<lclue,map<string,double>> {
};

void *new_cluer(const string &pickle); // initialize Python
void finalize(void *cluer);            // clean up
vector<double> analyze_clues(const void *pyc,
                             const vector<learning_datum> &clues);
#endif
