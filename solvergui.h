// Needed by solver and GUI both

#define BIG_SIZE 23             // maximum edge length
#define NUMLETS  26             // letters in alphabet
#define NUMREBUS 3              // max # rebus words allowed
#define NUMCHARS (NUMLETS + NUMREBUS)

#define WILD ('a' + NUMCHARS)   // unfilled character in some strings
      // changing WILD requires a complete database rebuild
