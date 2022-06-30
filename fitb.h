#ifndef _FITB_
#define _FITB_

const unsigned BRACKET   = 100; // maximum length of non-FITB clue part
const unsigned FITB_STOP = 250; // maximum number of "answers" returned

// The FITB stuff does not try to keep the entire corpora in memory;
// instead, we leave the corpora on disk and have pointers indicating
// where each clue word appears in the various corpora.  Then to
// analyze a specific FITB clue, we basically intersect all the
// relevant pointers to get a very small number of required disk
// lookups.

// Of course, we can't actually keep a pointer for every word into all
// of its positions in the corpus, that would be as big as the corpus
// itself!  So we *also* put that data on disk; what we actually store
// in memory is, for each word, a pointer to this secondary file
// indicating where the word appears, together with a count indicating
// how many appearances there are.  That's an _fitb_segment_.

struct fitb_segment {
  unsigned offset;
  unsigned length;

  fitb_segment(unsigned o = 0, unsigned l = 0) : offset(o), length(l) { }
};

// Here is the full FITB database.  A corpus consists of:
//  worddata     maps words into their fitb_segments as above
//  index_file   is the FILE* for the positional data
//  data_file    is the FILE* for the actual word data
// (we use FILE* because C can read ints from a binary file, and C++ can't)

// Actually, index_file and data_file are *vectors* of FILE*s to allow
// multithreading; each corpus can be used up to nthreads times
// simultaneously.  The upshot is that the clue analysis never has to
// wait for an FITB thing to free up.

struct corpus {
  map<string,fitb_segment> worddata;
  vector<FILE *>      index_file, data_file;

  ~corpus() { 
    for (auto &f : index_file) fclose(f);
    for (auto &f : data_file)  fclose(f);
  }

  void          get_word_data(const string &offset_file);
  fitb_segment  lookup(const string &wrd);
  string        quote(size_t offset, unsigned thread);
  vector<unsigned> offsets(const string &wrd, unsigned thread);
  unsigned      appearances(const string &wrd);
  unsigned      find_smallest(const vector<string> &words, unsigned len,
                              unsigned &best_offset, unsigned &blank);
  vector<unsigned> all_indices(const vector<string> &words, unsigned len,
                               unsigned best, unsigned blank, 
                               unsigned best_offset, unsigned thread);
  vector<string> quotes(const vector<string> &words, unsigned len, 
                        unsigned thread, unsigned &num);
};

//  offset_file  is the file with tse fitb_segments
//  index_file   is the file with the positional data
//  data_file    is the file with the actual word data

struct fitb_files {
  string offset_file, index_file, data_file;

  fitb_files(const string &o, const string &i, const string &d) :
    offset_file(o), index_file(i), data_file(d) { }
};

// There may be many "books" in the FITB database.  We have a
// vector<bool> to indicate which of them is in use at any particular
// moment, and an overall mutex to lock the in_use vector.

struct fitb_db : vector<corpus> {
  fitb_db() { }
  fitb_db(unsigned nthreads, const vector<fitb_files> &fnames,
          const string &dir);

  ~fitb_db() {
    cout << "deleting fitb_db, which means deleting " << size()
         << " individual corpuses @ " << get_time(true) << endl;
    clear();
    cout  << "corpuses deleted @ " << get_time(true) << endl;
  }

  mutex               thread_mx;
  vector<bool>        in_use;

  unsigned activate_thread();   // find an unused thread to use
  void release_thread(unsigned th);

  unsigned appearances(const string &wrd, unsigned mask = -1);
  map<string,unsigned> fillers(const string &clue, unsigned len, 
                               unsigned mask, unsigned &num);
  map<string,unsigned> fillers(const string &clue, unsigned len, 
                               const vector<unsigned> &mask);
};

#endif
