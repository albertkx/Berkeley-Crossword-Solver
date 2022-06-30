#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <Python.h>
#include "score.h"

// Python hooks.  Basically initialization, termination, and a lookup.

class python_cluer {
  PyObject *clue_module, *class_fn, *forest;
public:
  python_cluer(const string &pickle_file);
  ~python_cluer();
  void analyze_clues(const vector<learning_datum> &data, size_t lo,
                     size_t hi, vector<double> &ans) const;

private:
  PyObject *construct_python_args(const vector<learning_datum> &data,
                                  size_t lo, size_t hi) const;
  PyObject *call_python_filler(const vector<learning_datum> &data,
                               size_t lo, size_t hi) const;
};
