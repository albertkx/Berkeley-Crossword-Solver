#include "xwg.h"
#include <wx/colour.h>

// Set the background color in a cell.  setbg means that you should
// record this as a color to "put back" and resetbg means that you
// should in fact just put back the recorded color.

void solver_frame::set_cell_color(unsigned y, unsigned x, const wxColour *c,
                                  bool setbg, bool resetbg)
{
  if (resetbg) grid->SetCellBackgroundColour(y,x,*saved_color[x][y]);
  else {
    if (setbg) saved_color[x][y] = c;
    grid->SetCellBackgroundColour(y,x,*c);
  }
}

// The actual GUI is really pretty simple.  To render out a puzzle,
// first clear the current window and all the saved colors, then add
// rows and columns as for the puzzle and figure out the appropriate
// row size, etc.  Set the font based on the square size, and put out
// the black squares.  But some of that is only needed the first time
// we render it (we rerender when a new rebus word is found).

void solver_frame::render(bool first)
{
  for (size_t i = 0 ; i < BIG_SIZE ; ++i)
    for (size_t j = 0 ; j < BIG_SIZE ; ++j)
      saved_color[i][j] = wxWHITE;

  int ir = grid->GetNumberRows();
  if (ir) grid->DeleteRows(0,ir);
  grid->AppendRows(y);
  ir = grid->GetNumberCols();
  if (ir) grid->DeleteCols(0,ir);
  grid->AppendCols(x);

  if (first) {
    acpt_score->SetLabel("ACPT score");

    int sq = grid_size();
    grid->SetDefaultRowSize(sq / y - 1);
    grid->SetDefaultColSize(sq / x - 1);

    // Initialize font
    wxFont font(400 / extent(), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, // FIXME 500
                wxFONTWEIGHT_NORMAL);
    grid->SetDefaultCellFont(font);
    grid->SetGridLineColour("BLACK");

    SetTitle(wxString(("Dr.Fill: " + source).c_str()));
    message("New puzzle: " + source);
  }

  for (auto &bs : black_squares) 
    set_cell_color(bs.second,bs.first,wxBLACK,true,false);
                                // save as background, don't reset
  Update();
}

static const wxColour highlightc(255,119,255);
static const wxColour errorcolor(255,255,50);
static const wxColour donecolor(200,255,255);
static const wxColour modcolor(221,255,204);
static const wxColour pitchcolor(255,145,145);
static const wxColour suspectcolor(255,230,240);
static const wxColour bad_suspectcolor(255,153,194);

// Highlight a sequence of squares.  Just work through it, one cell at
// at time.  Only caveat is that pitchcolor gets replaced by red if
// the cell is wrong.

void solver_frame::highlight(unsigned x, unsigned y, unsigned len, bool acrossp,
                             const wxColour *c, bool setbg, bool resetbg)
{
  grid->BeginBatch();
  for (size_t i = 0 ; i < len ; i++) {
    const wxColour *c2 = c;
    if (c == &pitchcolor && saved_color[x][y] == &errorcolor) c2 = wxRED;
    set_cell_color(y,x,c2,setbg,resetbg);
    if (acrossp) ++x;
    else ++y;
  }
  grid->EndBatch();

  Update();
}

// Highlight a word or make it white (used to show progress during
// clue analysis).  We don't change the reversion color (in case we
// are reanalyzing the clues after a rebus check, say).

void solver_frame::highlight(unsigned w, bool hlt)
{
  highlight(words[w].x,words[w].y,words[w].len,words[w].across,
            &highlightc,false,!hlt);
}

// Show a pitch is similar.  The bool argument is the reset flag.

void solver_frame::show_pitch(unsigned w, bool p)
{
  highlight(words[w].x,words[w].y,words[w].len,words[w].across,
            &pitchcolor,false,!p);
}

// Show the puzzle.  First compute the scores for all the words; each
// letter is colored based on the least certain (highest score) word
// of which it's a part.  Now set the background to yellow for any
// mistake, and just push it all out.

const wxColour & fill_color(float s)
{
  static const wxColour purple("PURPLE");
  if (s < 0) return *wxGREEN;
  else if (s < 7) return *wxBLUE;
  else if (s < 15) return *wxBLACK;
  else if (s < 30) return purple;
  else return *wxRED;
}

void solver_frame::display(bool show_dots, bool finished)
{
  grid->BeginBatch();

  for (auto &w : words) {
    unsigned x = w.x;
    unsigned y = w.y;
    for (size_t j = 0 ; j < w.len ; ++j) {
      char f = w.fill()[j];
      string fs;
      if (f == WILD) fs = " ";
      else if (f >= 'a' + NUMLETS) {
        unsigned idx = f - ('a' + NUMLETS);
        if (rebusx[0] != (unsigned) -1) {
          for (size_t k = 0 ; k < rebus.size() ; ++k)
            if (rebusx[k] == x && rebusy[k] == y) {
              fs = rebus[k];
              break;
            }
        }
        else fs = rebus[idx];
      }
      if (fs.empty()) fs = string(1,f); // not set (somehow) by rebus
      if (show_dots || fs[0] != ' ') {
        bool error = false;
        if (fs[0] == ' ') error = true;
        if (!w.answer().empty()) {
          if (fs.size() == 1 || fs.find('/') == string::npos) { // split rebus
            if (w.answer()[j] != fs[0]) error = true;
          }
          else if (w.answer()[j] != fs[0] &&
                   w.answer()[j] != fs[1 + fs.find('/')])
            error = true;
        }
        if (error) set_cell_color(y,x,&errorcolor);
        else if (finished) set_cell_color(y,x,&donecolor);
        else if (grid->GetCellBackgroundColour(y,x) != modcolor)
          set_cell_color(y,x,wxWHITE);
        float s = w.score();
        if (w.crossers[j] >= 0 && w.crossers[j] < words.size()) 
          s = max(s,words[w.crossers[j]].score());
        grid->SetCellTextColour(y,x,fill_color(s));
        unsigned fsize = fs.size();
        for (size_t k = 0 ; k < fs.size() ; ++k) 
          if (fs[k] == '/') {
            fs[k] = '\n';
            fsize = max(k,fs.size() - k - 1);
          }
          else fs[k] = toupper(fs[k]);
        grid->SetCellValue(y,x,wxString(fs.c_str()));
        wxFont font((fsize > 1)? 50 / fsize : 35,wxFONTFAMILY_SWISS,
                    wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL);
        grid->SetCellFont(y,x,font);
      }
      if (w.across) ++x;
      else ++y;
    }
  }

  grid->EndBatch();
  Update();
}

// Tint a square pink.  Update the list of pink squares appropriately.

void solver_frame::set_pink(unsigned x, unsigned y)
{
  grid->BeginBatch();
  set_cell_color(y,x,&modcolor);
  grid->EndBatch();
  pink_squares.insert(make_pair(x,y));
  Update();
}

// Show suspect squares.  Different color if they are wrong; save them
// all as the suspects.

void solver_frame::set_suspects(const vector<unsigned> &x,
                                const vector<unsigned> &y)
{
  if (x.empty()) {
    clear_suspects();
    return;
  }
  grid->BeginBatch();
  for (unsigned i = 0 ; i < x.size() ; ++i) {
    bool error = (grid->GetCellBackgroundColour(y[i],x[i]) == errorcolor);
    set_cell_color(y[i],x[i],error? &bad_suspectcolor : &suspectcolor,true);
    suspect_squares.insert(make_pair(x[i],y[i]));
  }
  grid->EndBatch();
  Update();
}

// Untint all the pink squares.  Just go through and do it, although
// you don't untint a square that's marked as wrong.

void solver_frame::clear_pink()
{
  grid->BeginBatch();
  for (auto &p : pink_squares)
    if (grid->GetCellBackgroundColour(p.second,p.first) != errorcolor)
      set_cell_color(p.second,p.first,wxWHITE);
  grid->EndBatch();
  Update();
  pink_squares.clear();
}

// Clear the suspect squares.

void solver_frame::clear_suspects()
{
  grid->BeginBatch();
  for (auto &p : suspect_squares)
    set_cell_color(p.second,p.first,wxWHITE,false,true);
  grid->EndBatch();
  Update();
  suspect_squares.clear();
}

// Show fill for a word in the text window.  Used to show candidate fills,
// current fill or answer.

void solver_frame::show_fill(unsigned slot, const string &str, int which,
                             const string &fill, float value)
{
  long beginp = text->GetInsertionPoint();
  (*text) << wxString(str.c_str());
  if (which >= 0) (*text) << " " << (1 + which);
  wxString f(fill.c_str());
  f.Replace(wxString((char) WILD),".");
  if (!rebus.empty() && rebusx[0] == (unsigned) -1) 
    for (size_t i = 0 ; i < fill.size() ; ++i) {
      unsigned x = words[slot].x;
      unsigned y = words[slot].y;
      if (words[slot].across) x += i;
      else y += i;
      for (size_t j = 0 ; j < rebus.size() ; ++j)
        if (rebusx[j] == x && rebusy[j] == y)
          f.Replace(wxString((char) ('a' + NUMLETS)),
                   wxString(rebus[i].c_str()));
    }
  else for (size_t i = 0 ; i < rebus.size() ; ++i)
         f.Replace(wxString((char) ('a' + NUMLETS + i)),
                   wxString(rebus[i].c_str()));
  (*text) << ": " << f << " (" << value << ")\n";
  text->SetStyle(beginp,text->GetInsertionPoint(),fill_color(value));
}

// Display all fills for a word.  

void solver_frame::show_word(unsigned slot)
{
  (*text) << "\n";
  long beginp = text->GetInsertionPoint();
  (*text) << (int) words[slot].id << "-" << ((words[slot].across)? "A" : "D") 
          << ' ' << wxString(words[slot].clue.c_str()) 
          << " (" << (int) words[slot].len << ") " << "\n";
  text->SetStyle(beginp,text->GetInsertionPoint(),*wxRED);
  for (size_t i = 0 ; i < CURRENT ; ++i)
    show_fill(slot,"fill",i,words[slot].fills[i],words[slot].scores[i]);
  show_fill(slot,"answer",-1,words[slot].fills[ANSWER],words[slot].
            scores[ANSWER]);
  if (words[slot].fills[CURRENT].size())
    show_fill(slot,"current",-1,words[slot].fills[CURRENT],
              words[slot].scores[CURRENT]);
  else (*text) << "NO CURRENT FILL\n";
}
