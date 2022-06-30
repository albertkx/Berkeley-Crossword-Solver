#include <map>
#include <vector>

static map<char,string> translate = {
  { -1, "y" },
  { -4, "u" },                // u with umlaut
  { -5, "u" },                // carat
  { -6, "u" },
  { -7, "u" },                // accent
  { -8, "o" },
  { -9, "" },                 // division sign
  { -10, "o" },               // umlaut
  { -11, "o" },               // o with carat
  { -12, "o" },               // o with carat
  { -13, "o" },               // accent
  { -14, "o" },               // accent
  { -15, "n" },
  { -16, "d" },
  { -17, "i" },               // i with accept
  { -18, "i" },               // carat
  { -19, "i" },               // i with an accent
  { -20, "i" },               // accent
  { -21, "e" },               // umlaut
  { -22, "e" },               // e with carat
  { -23, "e" },               // e with accent
  { -24, "e" },               // e with accent
  { -25, "c" },               // c with an undermark
  { -26, "a" },
  { -27, "a" },               // circle above
  { -28, "a" },               // a with umlaut
  { -29, "a" },               // tilde
  { -30, "a" },               // a with carat
  { -31, "a" },               // a with accent
  { -32, "a" },               // accent
  { -33, "ss" },              // beta thing
  { -36, "U" },               // umlaut
  { -37, "o" },
  { -40, "o" },               // empty set sign
  { -41, "x" },               // times
  { -42, "O" },               // umlaut
  { -43, "'" },
  { -44, "O" },
  { -45, "\"" },
  { -46, "\"" },
  { -47, "N" },
  { -48, "D" },
  { -50, "I" },               // circle
  { -51, "I" },               
  { -52, "i" },               
  { -53, "E" },
  { -54, "E" },
  { -55, "E" },               // e with accent
  { -56, "e" },               // 
  { -57, "e" },               //
  { -58, "Ae" },
  { -59, "o" },               // 
  { -60, "A" },               // umlaut
  { -61, "A" },               // tilde
  { -62, "A" },               // carat
  { -63, "A" },               // accent
  { -64, "A" },               // accent
  { -65, "" },                // upside down question mark
  { -66, "3/4" },
  { -67, "1/2" },
  { -68, "1/4" },
  { -69, "" },                // >>
  { -70, "" },                // little circle
  { -71, "'" },
  { -73, "" },                // cdot
  { -74, "" },                // unknown punctuation
  { -76, "" },
  { -77, "" },                // little square thing
  { -78, "2" },               // ^2
  { -79, "" },                // +-
  { -80, "" },                // deg mark
  { -82, "" },                // (R)
  { -83, "" },                // hyphen?
  { -84, "" },                // negation?
  { -85, "" },                // <<
  { -87, "" },                // copyright mark?
  { -88, "_" },               // 
  { -89, "n" },               // with tilde
  { -92, "" },                // little square thing
  { -93, "u" },
  { -94, "" },                // cents
  { -95, "" },                // upside down !
  { -96, "_" },
  { -97, "u" },               // M u nster
  { -98, "l" },               // ???
  { -99, "" },                // unknown punctuation
  { -100, "e" },
  { -101, "" },               // unknown punctuation
  { -102, "c" },              // ???
  { -103, "o" },              // ???
  { -104, "" },               // unknown punctuation
  { -105, "" },               // unknown punctuation
  { -106, "" },               // unknown punctuation
  { -107, "" },               // unknown punctuation
  { -108, "'" },
  { -109, "'" },
  { -110, "'" },
  { -111, "'" },
  { -114, "e" },
  { -116, "'" },
  { -118, "s" },
  { -119, "a" },
  { -121, "a" },
  { -122, "a" },
  { -123, "..."},
  { -125, "E"},
  { -126, "e"},
  { -127, "u"},
  { -128, "" },               // some currency sign
};

static vector<pair<vector<char>,string>> multi_trans = {
  { { -61, -68 }, "u" },      // umlaut
  { { -61, -74 }, "o" },      
  { { -61, -86 }, "e" },      // tete
  { { -61, -87 }, "e" },      // aperitif
  { { -61, -88 }, "e" },      // Moliere
  { { -61, -125, -62, -81 }, "i" }, // Anais
  { { -62, -110 }, "'" },
};
