#include "score.h"
#include "fitb.h"

// Create the database in the obvious way.  get the words from the
// offset_files; open the others.

fitb_db::fitb_db(unsigned nthreads, const vector<fitb_files> &fnames,
                 const string &dir) : 
  in_use(nthreads,false)
{
  resize(fnames.size());
  for (unsigned i = 0 ; i < fnames.size() ; ++i) {
    (*this)[i].get_word_data(dir + fnames[i].offset_file);
    for (unsigned j = 0 ; j < nthreads ; ++j) {
      FILE *ifile = fopen((dir + fnames[i].index_file).c_str(),"rb");
      (*this)[i].index_file.push_back(ifile);
      FILE *dfile = fopen((dir + fnames[i].data_file).c_str(),"r");
      (*this)[i].data_file.push_back(dfile);
    }
  }
}

// Find an unused thread.  Should never fail!

unsigned fitb_db::activate_thread()
{
  thread_mx.lock();
  for (unsigned ans = 0 ; ans < in_use.size() ; ++ans)
    if (!in_use[ans]) {
      in_use[ans] = true;
      thread_mx.unlock();
      return ans;
    }
  cerr << "ran out of FITB threads!" << endl;
  abort();
}

// Release the current thread.

void fitb_db::release_thread(unsigned thread)
{
  thread_mx.lock();
  in_use[thread] = false;
  thread_mx.unlock();
}

// Get the word data in the obvious way.  Open the file of offsets;
// each line of which is of the format
//  word pointer number

void corpus::get_word_data(const string &offset_file)
{
  ifstream inf(offset_file.c_str());
  check_stream(inf,offset_file);
  string wrd;
  unsigned o,l;
  while (inf >> wrd >> o >> l) worddata[wrd] = fitb_segment(o,l);
}

// How many appearances are there of the given word, given a mask on
// the corpora?  Just add them up in the obvious way.

unsigned corpus::appearances(const string &wrd)
{
  return lookup(wrd).length;
}

unsigned fitb_db::appearances(const string &wrd, unsigned mask)
{
  unsigned ans = 0;
  for (unsigned i = 0 ; i < size() ; ++i)
    if (mask & (1 << i)) ans += (*this)[i].appearances(wrd);
  return ans;
}

// Get fitb_segment associated with a specific word.  Just find the
// associated fitb_segment.

fitb_segment corpus::lookup(const string &wrd)
{
  auto i = worddata.find(wrd);
  return (i == worddata.end())? fitb_segment() : i->second;
}

// Get the quote at the given offset in the file.  Basically get the
// data at [offset - BRACKET , offset + BRACKET], although you have to
// be a little careful not to go before the beginning of the file.
// Then look for \n in the data you got and terminate the string there
// if you found it.

string corpus::quote(size_t offset, unsigned thread)
{
  size_t start = (offset < BRACKET)? 0 : (offset - BRACKET);
  fseek(data_file[thread],start,SEEK_SET);
  char q[2 * BRACKET + 1];
  q[fread(q,1,2 * BRACKET,data_file[thread])] = 0;
  int pos = min(offset,(size_t) BRACKET);
  for (int p = pos ; q[p] ; ++p) 
    if (q[p] == '\n') {
      q[p] = 0;
      break;
    }
  for ( ; pos >= 0 ; --pos) if (q[pos] == '\n') break; // find left end
  return (pos >= 0)? (q + 1 + pos) : q;                // convert to string
}

// Given a word and a dictionary index, what are the offsets of the
// word's appearances in the relevant corpus?  Just read it out of the
// index file.

vector<unsigned> corpus::offsets(const string &wrd, unsigned thread)
{
  fitb_segment s = lookup(wrd);
  if (s.length == 0) return vector<unsigned>();
  vector<unsigned> ans(s.length);
  fseek(index_file[thread],s.offset,SEEK_SET);
  fread(&ans[0],sizeof(unsigned),s.length,index_file[thread]);
  return ans;
}

// Are two offets a match?  You need x - y to be delta, within the
// given tolerance.  In other words |x - y - delta| <= tolerance.  But
// it's a little tricky because if not in tolerance, we want to
// indicate whether y is too high or too low.  If y is too low, we
// return -1.  If too high, return 1.  Otherwise, return 0.

int loose_match(int x, int y, int delta, int tolerance)
{
  int z = x - y - delta;
  if (abs(z) <= tolerance) return 0;
  return (z < 0)? 1 : -1;
}

// Loose intersection of two offset lists.  Just march along.  At any
// point, if a match, push the element from v1 onto the list.
// Otherwise, increment one index or the other.  Note that if we push
// the element from v1 onto the list we *don't* increment the v2 index
// because one v2 index might match a bunch of v1 indices.

vector<unsigned> loose_intersect(const vector<unsigned> &v1, 
                                 const vector<unsigned> &v2,
                                 int delta, int tolerance)
{
  vector<unsigned> ans;
  if (v2.empty()) return ans;
  size_t idx2 = 0;
  for (size_t idx1 = 0 ; idx1 < v1.size() ; ) {
    int m = loose_match(v1[idx1],v2[idx2],delta,tolerance);
    if (m == 0) ans.push_back(v1[idx1++]);
    else if (m > 0) ++idx1;
    else if (++idx2 == v2.size()) break;
  }
  return ans;
}

// Given a quote from the corpus and a clue, what fills the blank?
// _common_ is the word that's the base for the offset.  If len is
// zero, return the whole quote; this way, the empty string can always
// represent failure.

// cp is the position in the clue of the common word; qp is the
// position in the quote (and remember, it may appear in the quote
// multiple times).  For each qp, if it's too early or late, the quote
// won't start or finish in time. If len is zero, then we return the
// clue if it's a match for the quote, and the empty string otherwise.
// (With no blanks, you don't have to look further.)

// Otherwise, if p > cp so that the blank is after the common word, we
// check the fitb_segment leading up to the common word, and the
// fitb_segment leading up to the blank.  Then we fill the blank,
// ignoring spaces in the quote.  Then if we're at a space in the
// quote and the rest matches as well, we're good.  The case where p <
// cp is similar.

string fill(const string &quote, const string &clue, const string &common,
            unsigned len)
{
  string ans;
  int x;
  size_t p = clue.find("___");
  if (len && p == string::npos) return ans;
  size_t cp = clue.find(common);
  if (cp == string::npos) return ans;
  for (size_t qp = quote.find(common) ; qp != string::npos ; 
       qp = quote.find(common,1 + qp)) {
    if (qp < cp) continue;      // quote doesn't start early enough
    if (quote.size() - qp < clue.size() - cp) continue; // or end
    unsigned filled = 0;
    if (len == 0) return (quote.substr(qp - cp,clue.size()) == clue)? clue : ans;
    else if (p > cp) {                 // blank after common word
      if (quote.substr(qp - cp,cp) != clue.substr(0,cp)) continue;
      if (quote.substr(qp,p - cp) != clue.substr(cp,p - cp)) continue;
      ans.resize(len,' ');
      for (x = qp + p - cp ; x < (int) quote.size() && filled < len ; ++x)
        if (quote[x] != ' ') ans[filled++] = quote[x];
      if (filled != len) continue;
      unsigned next = x + clue.size() - (p + 3);
      if (next > quote.size()) continue;
      if ((next == quote.size() || quote[next] == ' ') && 
          quote.substr(x,next - x) == clue.substr(p + 3))
        return ans;
    }
    else {
      if (quote.substr(qp,clue.size() - cp) != clue.substr(cp)) continue;
      unsigned l = cp - (p + 4);
      if (quote.substr(qp - l,l) != clue.substr(cp - l,l)) continue;
      ans.resize(len);
      filled = len;
      for (x = qp - l - 1 ; x >= 0 && filled ; --x)
        if (quote[x] != ' ') ans[--filled] = quote[x];
      if (filled || x + 1 < (int) p) continue;
      if ((x + 1 == (int) p || quote[x - p] == ' ') &&
          quote.substr(x - p + 1,p) == clue.substr(0,p))
        return ans;
    }
  }
  return string();
}

// Given a sequence of words, which has the fewest appearances?  Also
// compute best_offset, which is the offset in the string of this
// word, and blank, which is the index (not offset!) of the blank.

unsigned corpus::find_smallest(const vector<string> &words, unsigned len,
                               unsigned &best_offset, unsigned &blank)
{
  unsigned best = 0, bestn = -1, offset = 0;
  blank = -1;
  best_offset = 0;
  for (size_t i = 0 ; i < words.size() ; ++i)
    if (words[i] == "___") {
      blank = i;
      offset += len + 1;
    }
    else {
      unsigned n = appearances(words[i]);
      if (n == 0) return -1;
      if (n < bestn) { 
        best = i; 
        bestn = n; 
        best_offset = offset;
      }
      offset += words[i].size() + 1;
    }
  return best;
}

// Vector of indices where a string might occur.  Start with the
// offsets of the least frequent word. Then for each word, intersect
// as appropriate.  The only really tricky part is that when you're on
// the opposite side of the rare word as the blank is, there is a
// tolerance in the position of the word you're checking because you
// don't know how many spaces are in the word/phrase that fills the
// blank.

vector<unsigned> corpus::all_indices(const vector<string> &words, unsigned len,
                                     unsigned best, unsigned blank,
                                     unsigned best_offset, unsigned thread)
{
  vector<unsigned> indices = offsets(words[best],thread);
  unsigned new_offset = 0;
  for (size_t i = 0 ; i < words.size() ; ++i) 
    if (i == blank) new_offset += len + 1;
    else {
      if (i != best) {
        unsigned tolerance = 0;
        if (len && (best < blank) != (i < blank)) tolerance = len / 2;
        indices = loose_intersect(indices,offsets(words[i],thread),
                                  best_offset - new_offset,tolerance);
      }
      new_offset += words[i].size() + 1;
    }
  return indices;
}

// Get the quotes that match a clue, returning the number thereof.  To
// start, find the word with the fewest appearances, setting best up
// as the index in _words_ and bestoff as the associated offset.  The
// call to find_smallest also sets _blank_ as the index of the blank.
// If best is -1, it means that there is a word in _words_ that
// doesn't appear in the corpus.

// Now rebuild the original clue string and get all the appearances of
// the words in the corpus: for each appearance, get the quote, get
// the fill at the appropriate point, and add it to the overall
// answer.  If len==0, just return the number, with the actual vector
// being empty.

vector<string> corpus::quotes(const vector<string> &words, unsigned len,
                              unsigned thread, unsigned &num)
{
  unsigned bestoff, blank;
  unsigned best = find_smallest(words,len,bestoff,blank);
  if (best == (unsigned) -1) {
    num = 0;
    return vector<string>();
  }
  vector<unsigned> indices = all_indices(words,len,best,blank,bestoff,thread);

  vector<string> ans;
  if (len) {
    string matcher;
    for (size_t i = 0 ; i < words.size() ; ++i)
      matcher += (i? " " : "") + words[i];
    for (unsigned ind : indices) {
      string q = quote(ind,thread);
      string a = fill(q,matcher,words[best],len);
      if (a.size()) {
        ans.push_back(a);
        if (len && ans.size() == FITB_STOP) break;
      }
    }
    num = ans.size();
  }
  else num = indices.size();

  return ans;
}

// Strings and counts that fill a blank.  Collect the words in the
// clue and use quotes to do the work.  Make sure that you're the only
// thread trying to do this (we're going to be adjusting the file
// position in the offset and text files).

map<string,unsigned> fitb_db::fillers(const string &clue, unsigned len, 
                                      unsigned mask, unsigned &num)
{
  map<string,unsigned> ans;
  vector<string> words;
  istringstream is(clue);
  string wrd;
  while (is >> wrd) words.push_back(wrd);
  if (words.empty()) return ans;
  num = 0;
  unsigned thread = activate_thread();
  for (unsigned i = 0 ; i < size() ; ++i) 
    if (mask & (1 << i)) {
      unsigned n;
      vector<string> qs = (*this)[i].quotes(words,len,thread,n);
      num += n;
      for (auto &q :qs) ++ans[q];
    }
  release_thread(thread);

  return ans;
}

// Similar, but a vector of masks instead of just one.  As soon as you
// find a match, return the answer.

map<string,unsigned> fitb_db::fillers(const string &clue, unsigned len, 
                                      const vector<unsigned> &mask)
{
  for (unsigned m : mask) {
    unsigned n;
    map<string,unsigned> ans = fillers(clue,len,m,n);
    if (len == 0 && n) ans[clue] = n;
    if (!ans.empty()) return ans;
  }
  return map<string,unsigned>();
}

// Given a FITB clue, make sure it's in the right format for this
// code.

// 1. Make sure that it's got a blank!

// 2. If there is a trailing parenthetical expression, remove it.
//    It's probably a source or some such, not part of the actual
//    quote.  Put the result in np ("no parentheses").  And if you
//    just removed the blank, give up!

// 3. Remove anything after a : *after the blank* for a similar reason.

// 4. Now look at each possible end for a quote (pq), after the blank.
//    If that character is a quote of some kind, and it's either the
//    last character in the clue or it's followed by a non-alpha
//    (i.e., a space), then it appears to be the *end* of a quote.
//    Look for matching punctuation earlier; if you find it, it's only
//    the stuff in quotes that's the clue.  Otherwise, the clue is
//    whatever's in np.

// 5. So now the clue is set up in c.  Rebuild c in place, copying
//    alphanumeric and blanks, but (a) removing any opening space
//    before a blank, (b) removing multiple blanks, and (c) stopping
//    when you hit a colon.

// 6. The last step inserts spaces around the blank, if they aren't there.

string normalize(const string &clue)
{
  size_t pos = clue.find("___");
  if (pos == string::npos) return string();                             // 1
  string np;
  if (clue.back() == ')') np = clue.substr(0,clue.rfind('(') - 1);      // 2
  else np = clue;
  if (pos >= np.size()) return string();
  np = np.substr(0,np.find(':',pos));                                   // 3
  size_t pq;
  string c;
  for (pq = pos ; pq < np.size() ; ++pq)                                // 4
    if ((np[pq] == '\'' || np[pq] == '"') && 
        (pq == np.size() - 1 || !isalpha(np[pq + 1])))
      break;
  if (pq != np.size()) {
    int pq1;
    for (pq1 = pos ; pq1 >= 0 ; --pq1)
      if (np[pq1] == np[pq] && (pq1 == 0 || isspace(np[pq1 - 1]))) break;
    if (pq1 >= 0) c = np.substr(1 + pq1,pq - pq1 - 1);
  }
  if (c.empty()) c = np;                                                // end 4
  char ans[1 + c.size()];
  unsigned idx = 0;
  for (auto ch : c)
    if (isalnum(ch)) ans[idx++] = tolower(ch);
    else if (ch == '_') {
      if (idx == 1 && ans[0] == ' ') ans[0] = '_';                      // 5a
      else ans[idx++] = '_';
    }
    else if (isspace(ch)) {
      if (idx == 0 || ans[idx - 1] != ' ') ans[idx++] = ch;             // 5b
    }
    else if (ch == ':' && c.size() != np.size()) break;                 // 5c
  ans[idx] = 0;
  string result = ans;
  pos = result.find("___");                                             // 6
  if (pos == string::npos) return string(); // some weird quote thing happened
  if (pos && result[pos - 1] != ' ') {
    result = result.substr(0,pos) + " ___" + result.substr(pos + 3);
    ++pos;
  }
  if (pos + 3 < result.size() && result[pos + 3] != ' ')
    result = result.substr(0,pos) + "___ " + result.substr(pos + 3);
  return result;
}

// FITB main function.  First, make sure that there's a blank!  (Also
// just one blank; repeated blanks wreak havoc.)  Then initialize the
// fitb database if need be.  Normalize the clue, get the results, and
// push them all together.

// If a normal search doesn't work, try splitting the clue at , or :
// (taking the part before the , or after the :) and trying again.

// When you're done, sort the answers by frequency of occurrence, in
// case you don't want them all.

vector<string> database::fitb_from_corpora(unsigned len, const string &orig_clue,
                                           const vector<unsigned> &mask) const
{
  size_t p1 = orig_clue.find("___");
  if (p1 == string::npos || orig_clue.find("___",1 + p1) != string::npos)
    return vector<string>();

  string clue = len? normalize(orig_clue) : orig_clue;
  map<string,unsigned> results = blank_filler->fillers(clue,len,mask);
  if (results.empty()) {
    size_t p2 = orig_clue.find(',',p1);
    if (p2 != string::npos)
      results = blank_filler->fillers(normalize(orig_clue.substr(0,p2)),len,
                                      mask);
    else {
      p2 = orig_clue.find(':');
      if (p2 < p1)
        results = blank_filler->fillers(normalize(orig_clue.substr(p2 + 2)),
                                       len,mask);
    }
  }
  multimap<unsigned,string> sorted;
  for (auto &r : results) sorted.insert(make_pair(r.second,r.first));
  vector<string> ans;
  for (auto i = sorted.rbegin() ; i != sorted.rend() ; ++i) 
    ans.push_back(i->second);
  return ans;
}

// Top level FITB analysis.

vector<string> database::fitb(unsigned len, const string &clue, unsigned limit)
  const
{
  if (clue.find("___") == string::npos) return vector<string>();
  vector<unsigned> mask { 24, 2, 4, 1, 0 };// Wiki titles and dictionary together
  vector<string> ans = fitb_from_corpora(len,clue,mask);
  if (limit && ans.size() > limit) ans.resize(limit);
  return ans;
}
