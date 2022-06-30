// For compilers that support precompilation, includes "wx/wx.h".

#include "xwg.h"
#include <wx/txtstrm.h>
#include <wx/app.h>
#include <wx/msgdlg.h>
#include <wx/cmdline.h>
#include <wx/filedlg.h>
#include <wx/arrstr.h>

class wxGrid;

class DrFill : public wxApp
{
public:
    bool OnInit();
};

BEGIN_EVENT_TABLE(solver_frame, wxFrame)
    EVT_BUTTON(SUSPEND_GUI, solver_frame::SuspendGUI)
    EVT_BUTTON(NEXT_GUI, solver_frame::NextGUI)
    EVT_MENU  (wxID_EXIT, solver_frame::OnQuit)
    EVT_IDLE  (solver_frame::OnIdle)
    EVT_TIMER (EXEC_TIMER_IDLE, solver_frame::OnIdleTimer)
    EVT_GRID_CELL_LEFT_CLICK(solver_frame::click)

    EVT_MENU  (wxID_ABOUT,solver_frame::show_about)

    EVT_MENU  (OPEN_PUZZLE,solver_frame::open_puzzle)
    EVT_MENU  (ABS_TIME_MINS,solver_frame::abs_time_mins)
    EVT_MENU  (ABS_TIME_NODES,solver_frame::abs_time_nodes)
    EVT_MENU  (REL_TIME_MINS,solver_frame::rel_time_mins)
    EVT_MENU  (REL_TIME_NODES,solver_frame::rel_time_nodes)
    EVT_MENU  (TOURNAMENT,solver_frame::tournament)
    EVT_MENU  (PAUSE,solver_frame::do_pause)
    EVT_MENU  (RESET,solver_frame::do_reset)

END_EVENT_TABLE()

IMPLEMENT_APP(DrFill)

// ----------------------------------------------------------------------------
// DrFill
// ----------------------------------------------------------------------------

bool DrFill::OnInit()
{
  if (!wxApp::OnInit()) return false;
  solver_frame *frame = new solver_frame(argc);
  frame->Show(true);
  SetTopWindow(frame);

  return true;
}

// ----------------------------------------------------------------------------
// solver_frame
// ----------------------------------------------------------------------------

// GUI window.  Custom renderer allows us to put the word numbers in
// the corners of the cells.

class MyGridCellRenderer : public wxGridCellStringRenderer
{
public:
  solver_frame *sf;
  virtual void Draw(wxGrid& grid,
                    wxGridCellAttr& attr,
                    wxDC& dc,
                    const wxRect& rect,
                    int row, int col,
                    bool isSelected);
};

// Set up all the menus for the app.

void solver_frame::make_menus()
{
  wxMenuBar *menubar = new wxMenuBar;
  wxMenu *main_menu = new wxMenu;
  // main menu: about, quit
  main_menu->Append(wxID_ABOUT, _T("&About\tAlt-A"), _T("About"));
  main_menu->AppendSeparator();
  main_menu->Append(wxID_EXIT, _T("E&xit\tAlt-X"), _T("Quit"));
  menubar->Append(main_menu," ");

  // file menu: open, pause (between puzzles), reset totals
  wxMenu *file_menu = new wxMenu;
  file_menu->Append(OPEN_PUZZLE,"&Open puzzle\tCtrl-O","Open puzzle");
  file_menu->AppendSeparator();
  file_menu->Append(PAUSE,"&Pause between puzzles\tCtrl-P",
                    "Pause between puzzles",wxITEM_CHECK);
  file_menu->Check(PAUSE,true);
  file_menu->Append(RESET,"&Reset ACPT total\tCtrl-R","Reset ACPT total");
  menubar->Append(file_menu,"&File");

  // time limit menu: absolute (with submenu), relative (with submenu)
  wxMenu *abs_time = new wxMenu;
  abs_time->Append(ABS_TIME_MINS,"Minutes","Absolute time limit (minutes)");
  abs_time->Append(ABS_TIME_NODES,"Nodes","Absolute time limit (nodes)");
  wxMenu *rel_time = new wxMenu;
  rel_time->Append(REL_TIME_MINS,"Minutes","Relative time limit (minutes)");
  rel_time->Append(REL_TIME_NODES,"Nodes","Relative time limit (nodes)");
  wxMenu *time_menu = new wxMenu;
  time_menu->Append(TOURNAMENT,"Set &Tournament mode\tCtrl-T","Tournament mode");
  time_menu->Append(ABSOLUTE_TIME,"Set absolute time",abs_time,
                    "Absolute time limit");
  time_menu->Append(RELATIVE_TIME,"Set relative time",rel_time,
                    "Relative time limit");
  menubar->Append(time_menu,"&Time");

  SetMenuBar(menubar);
}

// Build the grid, which is initially a 1x1 blob, and return the size
// of the square (i.e., the whole thing).  It's actually very simple,
// just a matter of getting the display size, not using all of it
// (this is to allow projection on screens with limited resolution),
// and putting the grid together.

unsigned grid_size()
{
  int w,h;
  wxDisplaySize(&w,&h);
  return 550;                   // FIXME
  return (h < 900)? 600 : 750;
}

void solver_frame::build_grid()
{
  numbers[0].resize(1);
  int sq = grid_size();
  grid = new wxGrid(this, wxID_ANY, wxPoint(0,0), wxSize(sq,sq));

  grid->CreateGrid(1,1);
  grid->SetRowLabelSize(0);
  grid->SetColLabelSize(0);
  grid->SetDefaultCellAlignment(wxALIGN_CENTRE,wxALIGN_CENTRE);
  grid->EnableScrolling(false,false);
  grid->ClearSelection();
  MyGridCellRenderer *mgcr = new MyGridCellRenderer;
  mgcr->sf = this;
  grid->SetDefaultRenderer(mgcr);
}

// Layout is straightforward

void solver_frame::create_layout()
{
  // buttons: suspend the GUI, go to next puzzle (when ready), also
  // used to show score and total
  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  suspend_gui         = new wxButton(this, SUSPEND_GUI ,"Suspend GUI");
  next_gui            = new wxButton(this, NEXT_GUI ,"Next puzzle");
  acpt_score          = new wxStaticText(this, wxID_STATIC,"ACPT score",
                                         wxDefaultPosition,wxDefaultSize,
                                         wxALIGN_CENTER | wxST_NO_AUTORESIZE);
  acpt_total          = new wxStaticText(this, wxID_STATIC ,"ACPT total",
                                         wxDefaultPosition,wxDefaultSize,
                                         wxALIGN_CENTER | wxST_NO_AUTORESIZE);
  buttons->Add(suspend_gui,1,wxEXPAND);
  buttons->Add(next_gui,   1,wxEXPAND);
  buttons->Add(acpt_score, 3,wxEXPAND);
  buttons->Add(acpt_total, 2,wxEXPAND);

  text = new wxTextCtrl(this, wxID_ANY, " ",
                        wxDefaultPosition, wxSize(220,grid_size()), // FIXME 300
                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);

  wxBoxSizer *gridbuttons = new wxBoxSizer(wxVERTICAL);
  gridbuttons->Add(grid,1,wxEXPAND);
  gridbuttons->Add(buttons,0,wxEXPAND);

  wxBoxSizer *main = new wxBoxSizer(wxHORIZONTAL);
  main->Add(gridbuttons,1,wxEXPAND);
  main->InsertSpacer(1,10);
  main->Add(text,0,wxEXPAND);

  SetSizerAndFit(main);  
  Centre();
}

// Create the crossword-solving subprocess.

void solver_frame::spawn_process(unsigned argc)
{
  process = new wxProcess();    // execute the xw process; no args means get
  process->Redirect();          // them from the cheat file

  pid = wxExecute((argc > 1)? "./xw" : "./xw -igA",wxEXEC_ASYNC,process);
  idle_timer.Start(20);
}

// Solver frame creation

solver_frame::solver_frame(unsigned argc)
        : wxFrame((wxFrame *)nullptr, wxID_ANY, _T("Dr.Fill"),
                  wxDefaultPosition,
                  wxSize(550,550)),
                  // wxSize(600,600)), // FIXME
          process(nullptr), gui_suspended(false), 
          idle_timer(this,EXEC_TIMER_IDLE), acpt_curr(0), acpt_tot(0),
          pause(true), next_puzzle(0),
          tournament_mode(false), abs_mins(-1), abs_nodes(-1), 
          rel_mins(-1), rel_nodes(-1),
          numbers(1)
{
  make_menus();
  build_grid();
  create_layout();
  spawn_process(argc);
}

// Render a cell.  Put out the fill, which is easy, and then the
// number in the upper left if it's not zero.

void MyGridCellRenderer::Draw(wxGrid& grid,
                              wxGridCellAttr& attr,
                              wxDC& dc,
                              const wxRect& rect,
                              int row, int col,
                              bool isSelected)
{
  wxGridCellStringRenderer::Draw(grid,attr,dc,rect,row,col,isSelected);
  if (sf->number(col,row)) {
    int sq = grid_size() * 160 / 550; // FIXME 600
    wxFont font(sq / sf->extent(),wxFONTFAMILY_SWISS,wxFONTSTYLE_NORMAL,
                wxFONTWEIGHT_NORMAL);
    dc.SetFont(font);
    wxString n;
    n.Printf("%d",sf->number(col,row));
    dc.DrawText(n,rect.x + 1,rect.y + 1);
  }
}

void solver_frame::OnQuit(wxCommandEvent& WXUNUSED(ev))
{
  if (process && process->Exists(pid)) process->Kill(pid);
  Close(true);
}

// Hitting the suspend button actually toggles the suspend state.

void solver_frame::SuspendGUI(wxCommandEvent & WXUNUSED(ev))
{
  gui_suspended = !gui_suspended;
  suspend_gui->SetLabel(gui_suspended? "Resume GUI" : "Suspend GUI");
}

void solver_frame::NextGUI(wxCommandEvent & WXUNUSED(ev))
{
  send_puzzle();
}

// possible display messages
//  highlight n 0/1 (1 = do it)
//  pink x y
//  render first-time?
//  fill slot string score
//  display dots finished
//  size x y #words
//  word id x y len # across
//  answer id string
//  crossers n c1 ...
//  blacks n x1 y1 x2 y2 ...
//  source file
//  answer_score n val
//  clue id clue
//  desired w n fill-n-for-word-w score-n-for-word-w
//  clear_pink 
//  acpt score minutes words letters
//  complete
//  spinner f # f is float % of progress in reading dictionary
//  rebus n w1 ... wn [rebus chars]
//  pitch n 0/1 (1 = highlight in red to show pitch)
//  theme msg
//  suspects n x1 y1 ... xn yn [0 clears them]

enum { HIGHLIGHT, PINK, RENDER, FILL, DISPLAY, SIZE, WORD, 
       ANSWER_FILL, CROSSERS, BLACKS, SOURCE, ANSWER_SCORE, CLUE, DESIRED,
       CLEAR_PINK, ACPT, COMPLETE, SPINNER, REBUS, PITCH, THEME, SUSPECTS,
       HESITATE };

static const vector<string> commands { 
  "highlight", "pink", "render", "fill", "display", "size", "word", "answer", 
  "crossers", "blacks", "source", "answer_score", "clue", "desired",
  "clear_pink", "acpt", "complete", "spinner", "rebus", "pitch",
  "theme", "suspect", "hesitate" };

// All of these functions are easy; we just pull the relevant data
// from the input stringstream.

void solver_frame::highlight(istringstream &is)
{
  unsigned n;
  bool h;
  is >> n >> h;
  highlight(n,h); 
}

void solver_frame::pink(istringstream &is)
{
  unsigned x,y;
  is >> x >> y;
  set_pink(x,y);
}

// render is in gui.cpp

void solver_frame::fill(istringstream &is)
{
  unsigned w;
  is >> w;                      // split line to initialize w
  is >> words[w].fills[3] >> words[w].scores[3];
}

void solver_frame::display(istringstream &is)
{
  bool d,f;
  is >> d >> f;
  display(d,f);
}

void solver_frame::size(istringstream &is)
{
  unsigned w;
  is >> x >> y >> w;
  words.clear();
  words.resize(w);
  numbers.clear();
  numbers.resize(x);
  for (size_t i = 0 ; i < x ; ++i) numbers[i].resize(y,0);
  black_squares.clear(); 
  pink_squares.clear();
}

void solver_frame::word(istringstream &is)
{
  unsigned w;
  is >> w;
  is >> words[w].x >> words[w].y >> words[w].len >> words[w].id 
     >> words[w].across;
  numbers[words[w].x][words[w].y] = words[w].id;
}

void solver_frame::answer(istringstream &is)
{
  unsigned w;
  is >> w;
  is >> words[w].fills[4];
}

void solver_frame::crossers(istringstream &is)
{
  unsigned w;
  is >> w;
  words[w].crossers.resize(words[w].len);
  for (size_t i = 0 ; i < words[w].len ; ++i) is >> words[w].crossers[i];
}

void solver_frame::blacks(istringstream &is)
{
  unsigned n, bx, by;
  is >> n;
  for (size_t i = 0 ; i < n ; ++i) {
    is >> bx >> by;
    black_squares.push_back(make_pair(bx,by));
  }
}

// source handled inline in case split

void solver_frame::answer_score(istringstream &is)
{
  unsigned w;
  is >> w;
  is >> words[w].scores[4];
}

// Pull out the number, and then the clue

void solver_frame::clue(const string &line)
{
  size_t pos = line.find(' ');
  unsigned n = stoi(line.substr(1 + pos));
  pos = line.find(' ',1 + pos);
  words[n].clue = line.substr(1 + pos);
}

void solver_frame::desired(istringstream &is)
{
  unsigned w,n;
  is >> w >> n;
  is >> words[w].fills[n] >> words[w].scores[n];
}

// clear_pink is in gui.cpp

// Update ACPT scoring labels.  Both current and total score get
// adjusted; acpt_curr retains total so far on completed puzzles.

void solver_frame::acpt(istringstream &is)
{
  unsigned minutes, words, letters;
  is >> acpt_curr >> minutes >> words >> letters;
  ostringstream os;
  os << "Score " << acpt_curr << " (" << minutes << " minutes, " << words
     << " words, " << letters << " letters)";
  acpt_score->SetLabel(os.str().c_str());
  ostringstream os2;
  os2 << "Total " << acpt_tot + acpt_curr;
  acpt_total->SetLabel(os2.str().c_str());
}

void solver_frame::complete()
{
  acpt_tot += acpt_curr;
}

// Update the spinner.  Create the progress dialog if need be and
// update it as appropriate.

void solver_frame::spin(const string &line)
{
  if (!progress) progress = new wxProgressDialog("","Reading database ...");
  progress->Update(100 * stof(line.substr(7)) + 0.5); // bypass "spinner"
}

// Get the rebus words, and print them out in purple in the text window.

void solver_frame::read_rebus(istringstream &is)
{
  rebus.clear();
  rebusx.clear();
  rebusy.clear();
  unsigned n;
  string w, ystr;
  is >> n;
  for (unsigned i = 0 ; i < n ; ++i) {
    is >> w >> ystr;
    int x = stoi(w.substr(w.find('@') + 2)); // @(
    rebus.push_back(w.substr(0,w.find('@')));
    if (x == BIG_SIZE) { rebusx.push_back(-1), rebusy.push_back(-1); }
    else { rebusx.push_back(x), rebusy.push_back(atoi(ystr.c_str())); }
  }

  (*text) << "\n";
  long beginp = text->GetInsertionPoint();
  (*text) << "REBUS:";
  for (unsigned i = 0 ; i < rebus.size() ; ++i) {
    (*text) << ' ' << wxString(rebus[i].c_str());
    if (rebusx[i] != (unsigned) -1) 
      (*text) << " @(" << (int) rebusx[i] << ' ' << (int) rebusy[i] << ')';
  }
  (*text) << "\n";
  text->SetStyle(beginp,text->GetInsertionPoint(),
                 wxTheColourDatabase->Find("PURPLE"));
}

void solver_frame::show_pitch(istringstream &is)
{
  unsigned w,n;
  is >> w >> n;
  show_pitch(w,n);
}

// Theme is like message (below), but purple

void solver_frame::theme(const string &str)
{
  long beginp = text->GetInsertionPoint();
  (*text) << str << "\n";
  text->SetStyle(beginp,text->GetInsertionPoint(),
                 wxTheColourDatabase->Find("PURPLE"));
}

// Read the suspects, and then process them

void solver_frame::suspects(istringstream &is)
{
  vector<unsigned> x,y;
  unsigned n;
  is >> n;
  x.resize(n);
  y.resize(n);
  for (unsigned i = 0 ; i < n ; ++i) is >> x[i] >> y[i];
  set_suspects(x,y);
}

// Hesitate for a given number of seconds.  (Used for debugging only;
// not part of the GUI.)

void solver_frame::hesitate(istringstream &is)
{
  unsigned x;
  is >> x;
  wxSleep(x);
}

void solver_frame::message(const string &str)
{
  long beginp = text->GetInsertionPoint();
  (*text) << str << "\n";
  text->SetStyle(beginp,text->GetInsertionPoint(),*wxRED);
}

// Here we process input from the xw process cout stream.  If it
// starts **GUI, that means it's a GUI command, so we get the command,
// figure out which it is, and dispatch on that.  We croak if there is
// no dispatch.  If it doesn't start **GUI, we just send it to cout.

void solver_frame::process_line(const string &str)
{
  if (str.size() >= 6 && str.substr(0,6) == "**GUI ") {
    istringstream is(str.substr(6));
    string cmd;
    is >> cmd;
    vector<string> args;
    bool found = false;
    const char *real_str = str.c_str() + 6;
    for (size_t i = 0 ; i < commands.size() ; ++i)
      if (cmd == commands[i]) {
        found = true;
        switch(i) {
        case HIGHLIGHT: highlight(is); break;
        case PINK: pink(is); break;
        case RENDER: render(atoi(str.c_str() + 13)); break;
        case FILL: fill(is); break;
        case DISPLAY: display(is); break;
        case SIZE: size(is); break;
        case WORD: word(is); break;
        case ANSWER_FILL: answer(is); break;
        case CROSSERS: crossers(is); break;
        case BLACKS: blacks(is); break;
        case SOURCE: getline(is,source); break;
        case ANSWER_SCORE: answer_score(is); break;
        case CLUE: clue(str.substr(6)); break;
        case DESIRED: desired(is); break;
        case CLEAR_PINK: clear_pink(); break;
        case ACPT: acpt(is); break;
        case COMPLETE: complete(); break;
        case SPINNER: spin(real_str); break;
        case REBUS: read_rebus(is); break;
        case PITCH: show_pitch(is); break;
        case THEME: theme(str.substr(12)); break; // ignore "theme " at beginning
        case SUSPECTS: suspects(is); break;
        case HESITATE: hesitate(is); break;
        }
        break;
      }
    if (!found) {
      cerr << "unknown: " << str << endl;
      abort();
    }
  }
  else cout << str << endl;
}

// Main polling loop.  Don't do anything if the GUI is suspended or
// the process doesn't exist.  Otherwise, process input sent to either
// cout or cerr.

void solver_frame::OnIdle(wxIdleEvent& event)
{
  if (gui_suspended || !process || !process->Exists(pid)) return;
  if (process->IsInputAvailable()) {
      wxTextInputStream tis(*(process->GetInputStream()));
      wxString msg;
      msg << tis.ReadLine();
      const char *cstr = msg.c_str();
      process_line(cstr);
      event.RequestMore();
  }
  else if (process->IsErrorAvailable()) {
    wxTextInputStream tis(*(process->GetErrorStream()));
    wxString msg;
    msg << tis.ReadLine();
    cerr << msg << endl;
    event.RequestMore();
  }
}

// The idle timer just wakes up the idle system.

void solver_frame::OnIdleTimer(wxTimerEvent &WXUNUSED(event))
{
  wxWakeUpIdle();
}

// Clicking on a cell displays all words containing that cell.

bool gui_word::contains(unsigned x2, unsigned y2) const
{
  return across? (y2 == y && x2 >= x && x2 < x + len) :
    (x2 == x && y2 >= y && y2 < y + len);
}

void solver_frame::click(wxGridEvent &ev)
{
  unsigned x = ev.GetRow();
  unsigned y = ev.GetCol();
  for (size_t i = 0 ; i < words.size() ; ++i)
    if (words[i].contains(y,x)) show_word(i);
  ev.Skip();
}

string version = "1.0.0";
int year = 2017;

void solver_frame::show_about(wxCommandEvent &event)
{
  wxBeginBusyCursor();
  char vstring[50000];
  sprintf(vstring,
          "Dr.Fill %s Copyright (c) %d Matt Ginsberg\n"
          "Developed by Matt Ginsberg with data from public sources.\n"

          "Use should be self-explanatory.  Fill color indicates\n"
          "confidence: green, then blue, then black, then purple, then\n"
          "red.  Green generally means a clue has appeared before; blue\n"
          "means the clue seems generally familiar.  Black is a random\n"
          "word and purple is a random string of words.  Red is a wild\n"
          "guess.  Click on a square in the puzzle to see the clue and\n"
          "Dr.Fill's top three choices for an answer, along with ratings\n"
          "(lower is better).  The current fill and answer (if present\n"
          "in the Across Lite file) are also shown.  Yellow squares show\n"
          "mistakes (again, if there are answers in the puzzle file).\n"
          "Pink squares show fill that has changed recently.  Brick\n"
          "highlighting means that the word is begin considered for\n"
          "replacement.  An overall blue tint means that the puzzle is\n"
          "complete.\n\n"

          "Input files should be in .puz format.\n\n",
          version.c_str(),year);

  wxMessageDialog dialog(this,vstring,
                         _T("About Dr.Fill"),
                         wxOK | wxICON_INFORMATION);

  dialog.ShowModal();
  wxEndBusyCursor();
}

// Solve one or more puzzles.  The wxWidgets file dialog does
// basically all the work.  Pull the paths out of it, create the
// command string, and you're good to go.

void solver_frame::send_puzzle()
{
  if (puzzle_stack.size() <= next_puzzle) {
    wxMessageDialog d(this,"There are no puzzles pending!","",
                      wxOK | wxICON_ERROR);
    d.ShowModal();
    return;
  }
  ostringstream cmd;
  if (abs_mins >= 0 || abs_nodes >= 0) 
    cmd << " -a " << abs_mins << ' ' << abs_nodes;
  if (rel_mins >= 0 || rel_nodes >= 0)
    cmd << " -r " << rel_mins << ' ' << rel_nodes;
  if (tournament_mode) cmd << " -A";
  while (next_puzzle < puzzle_stack.size()) {
    cmd << ' ' << puzzle_stack[next_puzzle++];
    if (pause) break;
  }
  cmd << endl;
  wxTextOutputStream tos(*process->GetOutputStream());
  tos.WriteString(cmd.str().c_str());
}

// Open a puzzle just lets wxWidgets pick it to the above.

void solver_frame::open_puzzle(wxCommandEvent &event)
{
  wxFileDialog fd(this,"Select one or more files",".","","*.puz",
                  wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
  if (fd.ShowModal() != wxID_OK) return;
  wxArrayString paths;
  fd.GetPaths(paths);
  for (size_t i = 0 ; i < paths.GetCount() ; ++i)
    puzzle_stack.push_back(string(paths.Item(i).c_str()));
  send_puzzle();
}

// Get various time limits

void solver_frame::abs_time_mins(wxCommandEvent &event)
{
  abs_mins = wxGetNumberFromUser("Absolute time limit (minutes)",
                                 "Enter minutes:", "Absolute time",1,1,100);
}

void solver_frame::abs_time_nodes(wxCommandEvent &event)
{
  abs_nodes = wxGetNumberFromUser("Absolute time limit (nodes)",
                                  "Enter nodes:", "Absolute time",1,1,100000);
}

void solver_frame::rel_time_mins(wxCommandEvent &event)
{
  rel_mins = wxGetNumberFromUser("Relative time limit (minutes)",
                                 "Enter minutes:", "Relative time",1,1,100);
}

void solver_frame::rel_time_nodes(wxCommandEvent &event)
{
  rel_nodes = wxGetNumberFromUser("Relative time limit (nodes)",
                                  "Enter nodes:", "Relative time",1,1,100000);
}

void solver_frame::tournament(wxCommandEvent &event)
{
  tournament_mode = true;
}

void solver_frame::do_reset(wxCommandEvent &event)
{
  acpt_tot = 0;
  acpt_total->SetLabel("Total 0"); 
}
