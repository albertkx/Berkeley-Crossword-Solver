#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#if wxUSE_FILE
    #include <wx/file.h>
#endif

#include <wx/colordlg.h>
#include <wx/fontdlg.h>
#include <wx/numdlg.h>

#include <wx/grid.h>
#include <wx/generic/gridctrl.h>
#include <wx/process.h>
#include <wx/textctrl.h>
#include <wx/progdlg.h>

#include <vector>
#include <set>
#include <iostream>
#include <sstream>

using namespace std;

#include "solvergui.h"

enum { CURRENT = 3, ANSWER };

enum {
  EXEC_TIMER_IDLE = 10, 

  SUSPEND_GUI = 100,
  NEXT_GUI    = 101,

  OPEN_PUZZLE,
  ABS_TIME_MINS,
  ABS_TIME_NODES,
  ABS_TIME_LDS,
  REL_TIME_MINS,
  REL_TIME_NODES,
  REL_TIME_LDS,
  TOURNAMENT,
  ABSOLUTE_TIME,
  RELATIVE_TIME,
  PAUSE,
  RESET
};

// A word from the GUI's perspective.  Includes everything needed for display.

struct gui_word {
  string        clue;
  unsigned      id;
  unsigned      x, y;
  unsigned      len;
  bool          across;
  vector<int>   crossers;
  string        fills[5];       /* 0-2 are "best", 3 is current, 4 is sol'n */
  float         scores[5];      /* same but score of those words */

  const string & fill()   const { return fills[3]; }
  const float    score()  const { return scores[3]; }
  const string & answer() const { return fills[4]; }
  bool           contains(unsigned, unsigned) const;
};

class solver_frame : public wxFrame
{
  wxGrid         *grid;
  wxButton       *suspend_gui;   // suspend the GUI
  wxButton       *next_gui;      // go to next puzzle
  wxStaticText   *acpt_score;    // current score
  wxStaticText   *acpt_total;    // current total
  wxTextCtrl     *text;          // where to put diagnostic info
  wxProcess      *process;       // process that is solving the puzzle
  int             pid;           // associated PID
  bool            gui_suspended; // is the GUI suspended?
  wxTimer         idle_timer;    // polling timer
  unsigned        acpt_curr;     // ACPT score on current puzzle
  unsigned        acpt_tot;      // total ACPT score
  vector<string>  rebus;
  vector<unsigned> rebusx, rebusy;
  const wxColour *saved_color[BIG_SIZE][BIG_SIZE];

  bool            pause;         // pause between puzzles? (default yes)
  unsigned        next_puzzle;   // next to pass to solver
  bool            tournament_mode;
  int             abs_mins, abs_nodes;
  int             rel_mins, rel_nodes;
  wxProgressDialog *progress;
  vector<string>  puzzle_stack;

  unsigned        x,y;           // size
  vector<gui_word> words;
  vector<pair<unsigned,unsigned>> black_squares;
  set<pair<unsigned,unsigned>>    pink_squares;
  set<pair<unsigned,unsigned>>    suspect_squares;
  string          source;
  vector<vector<unsigned>> numbers; // little numbers in the corners

  void set_cell_color(unsigned x, unsigned y, const wxColour *c, 
                      bool setbg = true, bool resetbg = false);

  void show_about(wxCommandEvent &event);
  void open_puzzle(wxCommandEvent &event);
  void abs_time_mins(wxCommandEvent &event);
  void abs_time_nodes(wxCommandEvent &event);
  void rel_time_mins(wxCommandEvent &event);
  void rel_time_nodes(wxCommandEvent &event);
  void tournament(wxCommandEvent &event);
  void do_pause(wxCommandEvent &event) { pause = event.IsChecked(); }
  void do_reset(wxCommandEvent &event);

  void highlight(istringstream &is);
  void pink(istringstream &is);
  void fill(istringstream &is);
  void display(istringstream &is);
  void size(istringstream &is);
  void word(istringstream &is);
  void answer(istringstream &is);
  void crossers(istringstream &is);
  void blacks(istringstream &is);
  void answer_score(istringstream &is);
  void desired(istringstream &is);
  void clue(const string &str);
  void show_pitch(istringstream &is);
  void suspects(istringstream &is);
  void hesitate(istringstream &is);
  void clear_pink();
  void acpt(istringstream &is);
  void complete();
  void spin(const string &str);
  void read_rebus(istringstream &is);
  void theme(const string &str);
  void message(const string &str);

  void SuspendGUI(wxCommandEvent &);
  void NextGUI(wxCommandEvent &);
  void send_puzzle();
  void click(wxGridEvent &);
  void show_fill(unsigned slot, const string &str, int which, 
                 const string &fill,float value);
  void show_word(unsigned w);
  void show_pitch(unsigned w, bool p);
  void set_suspects(const vector<unsigned> &x, const vector<unsigned> &y);
  void clear_suspects();

  void make_menus();
  void build_grid();
  void create_layout();
  void spawn_process(unsigned argc);

 public:
  void process_line(const string &str);

  void render(bool first);
  void highlight(unsigned x, unsigned y, unsigned len, bool acrossp,
                 const wxColour *c, bool setbg, bool resetbg);
  void highlight(unsigned w, bool hlt);
  void display(bool show_dots, bool finished);
  void set_pink(unsigned x, unsigned y);

  solver_frame(unsigned argc);
  ~solver_frame() { delete grid; delete text; delete progress; }

  void OnQuit( wxCommandEvent& );
  void About( wxCommandEvent& );
  void OnVTable( wxCommandEvent& );
  void OnSmallGrid( wxCommandEvent& );
  void OnIdle( wxIdleEvent& );
  void OnIdleTimer(wxTimerEvent&);

  unsigned number(unsigned x, unsigned y) const { return numbers[x][y]; }
  unsigned extent() const { return max(x,y); }

  DECLARE_EVENT_TABLE()
    };

unsigned grid_size();
