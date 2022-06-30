#include "xw.h"

// Clean up a clue a little bit and return the result.

// First, we just copy the clue to tmp, but as we go, we handle some
// two-character multichars for ' and `.

// Now we work with tmp.  Strings we search for:
// 1. ". . ." gets replaced with ... but we take out any preceding spaces
// 2. "..." is similar
// 3. Any sequence of _ gets turned into ___
// 4. Two consecutive single quotes get turned into a double quote
// 5. Sequences of spaces get converted to single spaces

// Now we look at the result.  If it's mostly nonprinting characters,
// we give up because something is very wrong.  Otherwise, we prune
// the trailing spaces and we're done!

#include "wide.h"

string simplify(const string &clue)
{
  string ans;
  string tmp;
  for (size_t i = 0 ; i < clue.size() ; ) {
    bool found = false;
    for (auto &p : multi_trans) {
      if (i + p.first.size() < clue.size() &&
          !memcmp(&clue[i],&p.first[0],p.first.size())) {
        tmp += p.second;
        i += p.first.size();
        found = true;
        break;
      }
    }
    if (found) continue;
    if (translate.find(clue[i]) != translate.end()) {
      tmp += translate.at(clue[i]);
      ++i;
      continue;
    }
    if (!isascii(clue[i])) {
      cerr << "WARNING: Clue " << clue
           << " contains non-ascii characters in sequence";
      for (auto c : clue) cerr << ' ' << int(c);
      cerr << endl;
    }
    tmp.push_back(clue[i++]);
  }
  for (size_t i = 0 ; i < tmp.size() ; ++i)
    if (tmp.substr(i,5) == ". . .") {
      if (!ans.empty() && ans.back() == ' ') ans.pop_back();
      ans += "...";
      i += 4;			// 5th character picked up in for loop
    }
    else if (tmp.substr(i,3) == "...") {
      if (!ans.empty() && ans.back() == ' ') ans.pop_back();
      ans += "...";
      i += 2;
    }
    else if (tmp[i] == '_') {   // FITB is always ___
      ans += "___";
      while (tmp[i] == '_') ++i;
      --i;
    }
    else if (tmp[i] == 39 && i + 1 < tmp.size() && tmp[i + 1] == 39) { // '' is "
      ans += '"';
      ++i;
    }
    else if (tmp[i] == ' ') {   // remove double or leading spaces
      if (!ans.empty() && ans.back() != ' ') ans.push_back(tmp[i]);
    }
    else ans.push_back(tmp[i]);
  unsigned p = 0, np = 0;
  for (auto ch : ans)
    if (isprint(ch)) ++p;
    else ++np;
  if (p < np) ans.clear();
  while (!ans.empty() && isspace(ans.back())) ans.pop_back(); // trailing spaces
  return ans;
}

// Find a square in the puzzle.  If it's not there, make a new one and
// add it.

square *puzzle::find_square(unsigned x, unsigned y)
{
  square s(x,y);
  for (auto &sq : squares) if (*sq == s) return sq;
  squares.push_back(new square(s));
  return squares.back();
}

const square *puzzle::find_square(unsigned x, unsigned y) const
{
  square s(x,y);
  for (auto &sq : squares) if (*sq == s) return sq;
  return nullptr;
}

const square *puzzle::find_square(location loc) const
{
  for (auto &sq : squares) if (*sq == loc) return sq;
  return nullptr;
}

square *puzzle::find_square(location loc)
{
  for (auto &sq : squares) if (*sq == loc) return sq;
  return nullptr;
}

// Build a puzzle (a list of words, at this point) from a .puz file.
// This is sort of a mishmash because it was written before I found
// the reverse-engineered description of the .puz format at
// http://code.google.com/p/puz/wiki/FileFormat

// Assuming that the file exists, we read it all into a buffer.
// Apparently .puz files always contain the sequence 010, so we look
// for that.  (Unscrambled files contain 01000 but that doesn't appear
// to be true of scrambled files.)  The appearance of that marker
// allows us to find the size of the puzzle, which we save as x and y.
// Then we read the grid from the input buffer, such as it is.

// Continuing to process forward from the marker, we should find the
// author's name, followed by the copyright.  After that should be the
// clues, stored by square: across and then down, with the across clue
// first for corner squares.  For each word slot, we read the clue and
// create and store the word in question.  That's it!

// One subtlety involves the numbering.  newnumber indicates that a
// new word number is needed for this word.  In order to compute that,
// we need to know if the word is horizontal (newnumber is true) or,
// if it's down, if there was a horizontal word.  We can't just say
// "yes" if we're *looking* for a word because we might have an
// unchecked horizontal letter (which doesn't count).  So hasacross
// non-NULL indicates that we *think* this is an across word, but we
// only set hasacross to true after we *find* such a word.

// At the end, we do a little cleanup.  We compute all the crossings
// (in the dumbest possible way -- for each square in the word, we
// just look for a different word that contains it).

char *puzzle::insert_word(unsigned xpos, unsigned ypos, unsigned lim, 
                          unsigned &number, char *clue, bool unscrambled,
                          char grid[50][50], bool *hasacross, bool newnumber)
{
  int ch = 0;
  if (hasacross) {
    unsigned xx = xpos;
    while (xx < lim && grid[xx][ypos] != '.') { ++ch; ++xx; }
  }
  else {
    unsigned yy = ypos;
    while (yy < lim && grid[xpos][yy] != '.') { ++ch; ++yy; }
  }
  if (ch < 2) return clue;    // unchecked letter
  if (!clue) {
    cerr << source << ": not enough clues" << endl;
    clear();
    return nullptr;
  }
  vector<square *> sqs;
  for (int s = 0 ; s < ch ; ++s) 
    sqs.push_back(find_square(hasacross? xpos + s : xpos,
                              hasacross? ypos : ypos + s));
  push_back(word(sqs));

  if (newnumber) ++number;
  back().numbers.push_back(number);
  if (hasacross) *hasacross = true;
  if (unscrambled)
    for (auto sq : sqs) {
      sq->answer = tolower(grid[sq->x][sq->y]);
      back().answer.push_back(sq->answer);
    }
  back().id = size() - 1;
  back().establish_pointers(false);
  for (char *ch = ++clue ; *ch ; ++ch) if (*ch == 13) {
      cerr << source << ": confusing new line" << endl;
      clear();
      return nullptr;
    }
  if (!strlen(clue)) {
    cerr << source << ": missing clue" << endl;
    clear();
    return nullptr;
  }
  back().clue = simplify(clue);
  back().fitb = (back().clue.find("___") != string::npos);
  return strchr(clue,0);        // next clue
}

// Day of the week, needed for difficulty

static const int DEFAULT_DIFFICULTY = 2;

unsigned get_day(const string &fname)
{
  const char *date = fname.c_str();
  static string mnames[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  struct tm pub_date;
  int pub_year;
  pub_date.tm_min = pub_date.tm_sec = 0;
  pub_date.tm_hour = 12;
  pub_date.tm_year = 0;
  for (size_t i = 0 ; i < 12 ; ++i) {
    const char *ptr = strstr(date,mnames[i].c_str());
    if (!ptr) continue;
    pub_date.tm_mday = atoi(ptr + 3) / 100;
    pub_date.tm_mon = i;
    pub_date.tm_year = atoi(ptr + 5); 
    goto date_done;
  }
  while (*date && !isdigit(*date)) ++date;
  if (isdigit(date[0]) && isdigit(date[1]) && isdigit(date[2]) && 
      isdigit(date[3]) && isdigit(date[4]) && isdigit(date[5])) {
    if (isdigit(date[6]) && isdigit(date[7])) date += 2; // yyyymmdd format
    pub_date.tm_mday = atoi(date + 4);
    pub_date.tm_mon = atoi(date + 2) / 100 - 1;
    pub_date.tm_year = atoi(date) / 10000;
  }
  else return DEFAULT_DIFFICULTY; // no date field
 date_done:
  if (pub_date.tm_year < 40) pub_date.tm_year += 100;
  pub_year = pub_date.tm_year + 1900;
  if (pub_year > 10000) pub_year = 0;
  time_t t = mktime(&pub_date);
  int day = localtime(&t)->tm_wday - 1;
  if (day < 0) day = 6;         // Monday is 0, Sunday is 6
  return day;
}

// Make a puzzle from a puzfile and the number of minutes you have to
// solve it.

puzzle::puzzle(const string &puzfile) : source(puzfile)
{
  ifstream inf(puzfile.c_str(),ios::binary | ios::ate);
  assert(inf);
  char infile[50000];
  int len = inf.tellg();
  if (len == 0) {
    cerr << puzfile << ": empty file" << endl;
    return;
  }
  inf.seekg(0,ios::beg);
  inf.read(infile,len);
  int pos = 6;
  char marker[3] = { 0, 1, 0 };
  int x,y;
  bool unscrambled;
  for ( ;; ++pos) {
    for ( ; pos < len - 3 ; ++pos) if (!memcmp(infile + pos,marker,3)) break;
    if (pos == len - 3) {
      cerr << puzfile << ": cannot find 010 in file" << endl;
      return;
    }
    unscrambled = (infile[pos + 3] == 0 && infile[pos + 4] == 0);
    x = infile[pos - 3];
    y = infile[pos - 2];
    if (strstr(infile + pos + 5 + 2 * x * y,"Cryptic")) {
      cerr << puzfile << ": is cryptic" << endl;
      return;
    }
    if (x > 0 && x <= LONG_WORD && y > 0 && y <= LONG_WORD) break;
  }
  char grid[50][50];
  for (int let = 0 ; let < x * y ; ++let)
    grid[let % x][let / x] = infile[let + pos + 5];
  unsigned tchar = x * y + pos + 5;
  while (infile[tchar] == '.' || infile[tchar] == '-') ++tchar;
  title.clear();
  while (infile[tchar]) title.push_back(infile[tchar++]);
  if (!title.empty() && title[0] == '"' && title.back() == '"')
    title = title.substr(1,title.size() - 2);
  if (title.size() > 50 && title.find(' ') > 50)
    title = title.substr(1 + title.find(' '));
  char *auth = 1 + strchr(infile + pos + 5,0);
  if ((!isprint(*auth) && !isspace(*auth)) || strlen(auth) < 5) {
    cerr << puzfile << ": cannot find author" << endl;
    return;
  }
  author = auth;
  char *clue = strchr(auth,0);
  if (!clue) {
    cerr << puzfile << ": cannot find copyright" << endl;
    return;
  }
  clue = strchr(1 + clue,0);

  unsigned number = 0;
  squares.reserve(LONG_WORD * LONG_WORD);
  for (int ypos = 0 ; ypos < y ; ++ypos)
    for (int xpos = 0 ; xpos < x ; ++xpos) {
      bool hasacross = false;
      if (grid[xpos][ypos] == '.') continue;
      if (xpos == 0 || grid[xpos - 1][ypos] == '.') {
        clue = insert_word(xpos,ypos,x,number,clue,unscrambled,grid,&hasacross,
                           true);
        if (!clue) {
          cerr << puzfile << ": unchecked letter" << endl;
          return;
        }
      }
      if (ypos == 0 || grid[xpos][ypos - 1] == '.') {
        clue = insert_word(xpos,ypos,y,number,clue,unscrambled,grid,nullptr,
                           !hasacross);
        if (!clue) {
          cerr << puzfile << ": unchecked letter" << endl;
          return;
        }
      }        
    }
  should_be_themed.resize(size(),false);
  
  // compute puzzle dimensions
  xymax = location();
  for (auto &w : *this) for (auto sqp : w) xymax.max(*sqp);
  xymax += location(1,1);

  // compute black squares
  bool black[xymax.x][xymax.y];
  memset(black,true,xymax.x * xymax.y);
  for (auto &w : *this)
    for (auto sqp : w) black[sqp->x][sqp->y] = false;
  for (unsigned i = 0 ; i < xymax.x ; ++i)
    for (unsigned j = 0 ; j < xymax.y ; ++j)
      if (black[i][j]) black_squares.push_back(square(i,j));
  sort(black_squares.begin(),black_squares.end());

  // simple words
  for (auto &w : *this)
    simple_words.push_back(simple_word(w.numbers[0],w.across(),w.size(),w[0]));

  if (puzfile.find("nyt") != string::npos) {
    difficulty = get_day(puzfile);
    if (difficulty == 6) difficulty = 3; // Sunday is like Thursday
    notheme = (difficulty > 3);          // Friday / Saturday
  }
  else {
    notheme = false;
    difficulty = DEFAULT_DIFFICULTY;
  }
}

// write out the puzzle as an across lite file, and open it.  Details
// on the format are at code.google.com/p/puz/wiki/FileFormat

// This code doesn't work, and I have no idea why.  But it's not worth
// fixing, since there is a public python constructor for .puz files.
// So we use the broken code to write out the .puz file, then use a
// modified version of the public .py file to "round trip" it into
// thefill.puz.  The modification is that we've removed the checksum
// tests in the .py file, since that's what we broke.

unsigned short cksum_region(char *base, int len, unsigned short cksum)
{
  int i;
  for (i = 0 ; i < len ; ++i) {
    if (cksum & 1) cksum = (cksum >> 1) + 0x8000;
    else cksum >>= 1;
    cksum += (unsigned char) *(base + i);
  }
  return cksum;
}

void puzzle::across_lite(const string &puzfile) const
{
  unsigned sz = xymax.x * xymax.y;
  FILE *inf = checked_fopen(puzfile.c_str(),"rb");
  char infile[50000];
  unsigned short cksum = 0;
  fread(&cksum,2,1,inf);
  char magic[0xc];
  fread(magic,1,0xc,inf);      // ACROSS&DOWN
  unsigned short cib;
  unsigned lo,hi;
  fread(&cib,2,1,inf);
  fread(&lo,4,1,inf);
  fread(&hi,4,1,inf);
  char version[4];
  fread(version,1,4,inf);
  unsigned short rsv;
  fread(&rsv,2,1,inf);
  unsigned short scram;
  fread(&scram,2,1,inf);
  char rsv2[0xc];
  fread(rsv2,1,0xc,inf);
  char data[8];
  fread(data,1,8,inf);
  char soln[sz], fill[sz];
  fread(soln,1,sz,inf);
  fread(fill,1,sz,inf);
  int len = fread(infile,1,50000,inf);
  fclose(inf);

  char *title = infile;
  char *author = title + strlen(title) + 1;
  char *copyright = author + strlen(author) + 1;
  char *clue[size()];
  clue[0] = copyright + strlen(copyright) + 1;
  for (size_t i = 1 ; i < size() ; ++i)
    clue[i] = clue[i - 1] + strlen(clue[i - 1]) + 1;
  char *notes = clue[size() - 1] + strlen(clue[size() - 1]) + 1;

  char fill2[BIG_SIZE][BIG_SIZE];
  compute_fill(fill2);

  unsigned fsz = 0;
  for (unsigned y = 0 ; y < xymax.y ; ++y) 
    for (unsigned x = 0 ; x < xymax.x ; ++x) 
      switch(fill2[x][y]) {
      case WILD: fill[fsz++] = '.'; break;
      case '?': fill[fsz++] = '-'; break;
      default: fill[fsz++] = toupper(fill2[x][y]);
      }

  unsigned short c_cib = cksum_region(data,8,0);

  unsigned short c_cksum = c_cib;
  c_cksum = cksum_region(soln,sz,c_cksum);
  c_cksum = cksum_region(fill,sz,c_cksum);
  if (strlen(title)) c_cksum = cksum_region(title,strlen(title) + 1,c_cksum);
  if (strlen(author)) c_cksum = cksum_region(author,strlen(author) + 1,c_cksum);
  if (strlen(copyright))
    c_cksum = cksum_region(copyright,strlen(copyright) + 1,c_cksum);
  for (size_t i = 0 ; i < size() ; ++i)
    c_cksum = cksum_region(clue[i],strlen(clue[i]),c_cksum);
  if (strlen(notes)) c_cksum = cksum_region(notes,strlen(notes) + 1,c_cksum);

  unsigned short c_sol = cksum_region(soln,sz,0);
  unsigned short c_grid = cksum_region(fill,sz,0);
  unsigned short c_part = 0;
  if (strlen(title)) c_part = cksum_region(title,strlen(title) + 1,c_part);
  if (strlen(author)) c_part = cksum_region(author,strlen(author) + 1,c_part);
  if (strlen(copyright))
    c_part = cksum_region(copyright,strlen(copyright) + 1,c_part);
  for (size_t i = 0 ; i < size() ; ++i)
    c_part = cksum_region(clue[i],strlen(clue[i]),c_part);
  if (strlen(notes)) c_part = cksum_region(notes,strlen(notes) + 1,c_part);

  string outstr = "/tmp/df";
  size_t pos = puzfile.rfind('/');
  outstr += (pos == string::npos)? puzfile : puzfile.substr(1 + pos);
  outstr = outstr.substr(0,outstr.size() - 4); // remove .puz
  FILE *outf = checked_fopen(outstr.c_str(),"wb");

  fwrite(&c_cksum,2,1,outf);
  fwrite(magic,1,0xc,outf);
  fwrite(&c_cib,2,1,outf);

  unsigned char c[8];
  c[0] = 0x49 ^ (c_cib & 0xFF);
  c[1] = 0x43 ^ (c_sol & 0xFF);
  c[2] = 0x48 ^ (c_grid & 0xFF);
  c[3] = 0x45 ^ (c_part & 0xFF);
  c[4] = 0x41 ^ ((c_cib & 0xFF00) >> 8);
  c[5] = 0x54 ^ ((c_sol & 0xFF00) >> 8);
  c[6] = 0x45 ^ ((c_grid & 0xFF00) >> 8);
  c[7] = 0x44 ^ ((c_part & 0xFF00) >> 8);
  fwrite(c,1,8,outf);

  string outversion = "1.3";
  fwrite(outversion.c_str(),1,4,outf);
  fwrite(&rsv,2,1,outf);
  fwrite(&scram,2,1,outf);
  fwrite(rsv2,1,0xc,outf);
  fwrite(data,1,8,outf);
  fwrite(soln,1,sz,outf);
  fwrite(fill,1,sz,outf);
  fwrite(infile,1,len,outf);

  if (!rebus.empty()) {
    char rusr[5000];
    unsigned ptr = 0;
    for (unsigned i = 0 ; i < fsz ; ++i)
      if (fill[i] != '.' && fill[i] > 'z') {
        string reb;
        if (rebus[fill[i] - '{'].sq.x == (unsigned) -1) 
          reb = rebus[fill[i] - '{'];
        else for (unsigned j = 0 ; j < rebus.size() ; ++j)
               if (i == rebus[j].sq.y * xymax.x + rebus[j].sq.x) {
                 reb = rebus[j];
                 break;
               }
        strcpy(rusr + ptr,reb.c_str());
        ptr += rebus[fill[i] - '{'].size() + 1;
      }
      else rusr[ptr++] = 0;
    rusr[ptr] = 0;
    for (unsigned i = 0 ; i < ptr ; ++i) if (rusr[i]) rusr[i] = toupper(rusr[i]);
    fwrite("RUSR",1,4,outf);
    fwrite(&ptr,2,1,outf);
    unsigned short ck = cksum_region(rusr,ptr,0);
    fwrite(&ck,2,1,outf);
    fwrite(rusr,1 + ptr,1,outf);
  }

  fclose(outf);
  system(("python writefill.py " + outstr + ' ' + outstr + ".puz").c_str());
  system(("open " + outstr + ".puz").c_str());
}

// Push the data to firefox, tabs between words

void puzzle::firefox() const
{
  char fill2[BIG_SIZE][BIG_SIZE];
  compute_fill(fill2);

  string f;
  for (unsigned y = 0 ; y < xymax.y ; ++y) {
    if (!f.empty() && f.back() != '\t') f.push_back('\t');
    for (unsigned x = 0 ; x < xymax.x ; ++x) 
      switch(fill2[x][y]) {
      case WILD: if (!f.empty() && f.back() != '\t') f.push_back('\t'); break;
      case '?': break;
      default: f.push_back(fill2[x][y]);
      }
  }

  send_to_firefox(f);
}
