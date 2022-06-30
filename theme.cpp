#include "xw.h"

// Given a theme and some original fill (only needed in the ADD case
// where only the length of the added string is fixed), along with
// flamingo-suggested fill, what do we think we might want to insert
// into the puzzle?

vector<theme_fill> theme::get_hints(const theme_fill &tfill, const database &d)
  const
{
  unsigned len = sound? 0 : tfill.original.size();
  vector<string> ans;
  switch (type) {
  case ADD_STRING: ans = get_hints_add(len,tfill.psource,tfill.poriginal); break;
  case REMOVE_STRING: ans = get_hints_remove(len,tfill.psource);           break;
  case CHANGE_STRING: ans = get_hints_change(len,tfill.psource);           break;
  case ANAGRAM:    ans = get_hints_anagram(tfill.poriginal,tfill.psource); break;
  case PORTMANTEAU:   return get_hints_portmanteau(tfill.poriginal,d);
  case PUN:           return vector<theme_fill>(); // can't do it
  case NOTHEME:       if (sound) ans.push_back(tfill.psource);
  }

  // Now build the actual theme fills.  In the non-sound case, it's
  // easy, because the fill and pfill are given by the modified
  // strings in ans.
  vector<theme_fill> result;
  if (!sound) {
    result.resize(ans.size(),tfill);
    for (size_t i = 0 ; i < ans.size() ; ++i) 
      result[i].fill = result[i].pfill = ans[i];
  }
  // The sound case is harder.  For each modified string (now a
  // pronunciation), get each original string that has that
  // pronunciation.  For each of *those*, if you've already got it,
  // you don't have to do it again.  If it's a new fill, create a new
  // result as appropriate -- as per the original argument, but
  // modified so that fill is the unpronunciation and pfill the
  // pronunciation string that has the theme modification
  else for (auto &str : ans)
      for (const string &j : d.unpronounce(str,tfill.original.size())) {
        bool found = false;
        for (auto &r : result) if (r.fill == j) { found = true; break; }
        if (found) continue;
        result.push_back(tfill);
        result.back().pfill = str;
        result.back().fill = j;
      }
  return result;
}

// The add case is a pain if you have to add multiple copies of a
// string.  This handles that recursively; if you don't need any more,
// you're done (but have to put on the residue of the base string);
// otherwise, put the new string everywhere and add one less copy.

vector<string> theme::add_string(const string &source, unsigned needed, 
                                 const vector<string> &start, unsigned idx) const
{
  vector<string> ans;
  if (needed == 0) {
    ans.resize(start.size());
    for (size_t i = 0 ; i < start.size() ; ++i)
      ans[i] = start[i] + source.substr(idx);
    return ans;
  }
  for (size_t i = idx ; i <= source.size() ; ++i) {
    vector<string> ans2(start.size());
    for (size_t j = 0 ; j < start.size() ; ++j) 
      ans2[j] = start[j] + source.substr(idx,i - idx) + str1;
    ans2 = add_string(source,needed - 1,ans2,i);
    ans.insert(ans.end(),ans2.begin(),ans2.end());
  }
  return ans;
}

// Overall construction in the add case.  If you're looking for
// something shorter or the delta isn't a multiple of the string
// added, give up.  Otherwise, figure out how many copies are needed
// to get to the desired length.  Now if you can put them anywhere,
// the above function does it; otherwise, just put them at the
// beginning or the end.

// If you *don't* have a string to add, make sure there's just one
// copy needed.  Put it at the beginning or end as required.

vector<string> theme::get_hints_add(unsigned len, const string &source,
                                    const string &fill) const
{
  if (len && (len <= source.size() || (len - source.size()) % str1.size()))
    return vector<string>();
  unsigned n = len? ((len - source.size()) / str1.size()) : 1;
  if (n > ADD_LIMIT) return vector<string>();
  if (!wild()) return add_string(source,n,vector<string>(1,""));
  if (n > 1) return vector<string>(); // wildcard only adds one
  if (str1[0] == '?')
    return vector<string>(1,fill.substr(0,str1.size()) + source);
  return vector<string>(1,source + fill.substr(source.size()));
}

// Remove is much simpler.  If you know the string, just change that
// to the empty string.  Otherwise, just remove chars from the
// beginning or end as appropriate.

vector<string> theme::get_hints_remove(unsigned len, const string &source) const
{
  if (len >= source.size()) return vector<string>();
  if (!wild()) return get_hints_change(len,source);
  if (source.size() - len != str1.size()) return vector<string>();
  if (str1[0] == '?') return vector<string>(1,source.substr(str1.size()));
  return vector<string>(1,source.substr(0,len));
}

// Change one string for another.  Find all of the locations of the first
// string, and expect to change out any subset.

vector<string> theme::get_hints_change(unsigned len, const string &source) const
{
  vector<size_t> indices;
  for (size_t curr_idx = source.find(str1) ; curr_idx != string::npos ; 
       curr_idx = source.find(str1,curr_idx + str1.size()))
    indices.push_back(curr_idx);
  vector<string> ans;
  for (int i = 1 ; i < (1 << indices.size()) ; ++i) {
    string s;
    for (unsigned j = 0 ; j < indices.size() ; ++j) {
      unsigned begin = j? (str1.size() + indices[j - 1]) : 0;
      s += source.substr(begin,indices[j] - begin);
      s += (i & (1 << j))? str2 : str1;
    }
    s += source.substr(indices.back() + str1.size());
    if (len == 0 || s.size() == len) ans.push_back(s);
  }
  return ans;
}

// Anagram of the source.  For locations where the two strings differ,
// consider all possible anagrams.  next_permutation is a C++ builtin.

vector<string> theme::get_hints_anagram(const string &fill, const string &source)
  const
{
  static const unsigned ANAGRAM_LIMIT = 5;
  if (fill.size() != source.size()) return vector<string>();
  vector<unsigned> diffs;
  vector<unsigned> perm;
  for (size_t i = 0 ; i < source.size() ; ++i) 
    if (fill[i] != source[i]) {
      diffs.push_back(i);
      perm.push_back(i);
    }
  if (diffs.size() > ANAGRAM_LIMIT) return vector<string>();
  vector<string> ans;
  while (next_permutation(perm.begin(),perm.end())) {
    string h = source;
    for (size_t i = 0 ; i < diffs.size() ; ++i) h[diffs[i]] = source[perm[i]];
    ans.push_back(h);
  }
  return ans;
}

// Portmanteau possibilities.  If testing is true, we've got a
// candidate portmanteau and want to confirm that this really is the
// theme.  If false, we're trying to generate candidates.

// First, get the intended overlap (3 for testing, 2 otherwise).  Now
// if there isn't room for at least 5 unique characters (total),
// you're done (and increasing overlap obviously won't help).
// Otherwise, delta is the difference between the lengths of the two
// halves, absolute value at most 2.  Find the size of the two
// overlapping words, then get the words, then get possible
// replacements for each.  Now for each pair of possible replacements,
// we check that the overlap actually works.  If so, we're good -- but
// mark the portmanteau as "coming from" whichever side was weaker.
// (But in the testing case, we also want to ensure that the
// non-overlap parts aren't words, lest we treat a normal compound
// word as part of a portmanteau.)

vector<theme_fill> database::get_hints_portmanteau(const string &fill, 
                                                   bool testing, bool explain)
  const
{
  static const unsigned PORTMANTEAU_DELTA = 15;
  vector<theme_fill> ans;
  for (unsigned overlap = 2 + testing ; overlap <= OVERLAP_LIMIT ; ++overlap) {
    if (overlap + 5 > fill.size()) break; // 4 = 2 squares each side, no good
    for (int delta = -2 ; delta < 3 ; ++delta) {
      unsigned left_size = (fill.size() - overlap) / 2 + delta + overlap;
      if (left_size < 2 + overlap || fill.size() < 2 + left_size) continue;
      string lword = fill.substr(0,left_size);
      string rword = fill.substr(left_size - overlap);
      vector<string> left, right;
      if (testing) {
        left.push_back(lword);
        right.push_back(rword);
      }
      else {
        left = flamingo_match(lword,false,FLAMINGO_MATCH,false);
        right = flamingo_match(rword,false,FLAMINGO_MATCH,false);
      }
      for (auto &l : left)
        for (auto &r : right) {
          if (l.size() != lword.size() || r.size() != rword.size()) continue;
          if (l.substr(l.size() - overlap) != r.substr(0,overlap)) continue;
          if (!in_portmanteau(l) || !in_portmanteau(r)) continue;
          if (testing) {
            string ort = l.substr(0,l.size() - overlap);
            if (in_portmanteau(ort) &&
                get_mlg_score(ort) + PORTMANTEAU_DELTA > get_mlg_score(l))
              continue;
            ort = r.substr(overlap);
            if (in_portmanteau(ort) &&
                get_mlg_score(ort) + PORTMANTEAU_DELTA > get_mlg_score(r))
              continue;
          }
          float lsc = get_score(l);
          float rsc = get_score(r);
          ans.push_back(theme_fill());
          ans.back().original = ans.back().poriginal = fill;
          ans.back().pfill = ans.back().fill = l + r.substr(overlap);
          ans.back().source = (lsc < rsc)? r : l;
          if (testing) {
            if (explain)
              cout << "portmanteau: string " << fill << " left " << l
                   << " right " << r << " ans " << ans[0] << endl;
            return ans;
          }
        }
    }
  }
  return ans;
}

vector<theme_fill> theme::get_hints_portmanteau(const string &fill, 
                                                const database &d) const
{
  return d.get_hints_portmanteau(fill,false,false);
}

// The above code took a known theme and fill, and tried to create
// possible theme entries based on both the fill and a source of some
// sort, a dictionary entry that was a near match for the fill.  The
// next block of code actually figures out the theme in the first
// place.

// Find the themes suggested by a fill/src pair.  For anagram, just
// check.  For portmanteau, portmanteau(string) calls
// get_hints_portmanteau with testing set to true.

vector<theme> database::find_special_themes(const string &fill, 
                                            bool try_portmanteau,
                                            bool explain) const
{
  vector<theme> ans;
  if (is_anagram(fill,false)) ans.push_back(theme(false,ANAGRAM));
  else if (try_portmanteau && portmanteau(fill,explain))
    ans.push_back(theme(false,PORTMANTEAU));
  return ans;
}

// A word is a sound anagram if one of its pronunciations is (but if
// any pronunciation is actually a word, we can give up).

bool database::is_sound_anagram(const string &fill) const
{
  for (auto &pron : get_pronunciations(fill,false,false))
                                // multiple pronunciations but don't call flite
    if (pron_to_words.includes(pron)) return false;
    else if (is_anagram(pron,true))   return true;
  return false;
}

// When we're trying to figure out if a puzzle is themed, we have to
// see if all of the "theme" entries are just plain old dictionary
// entries.  This checks that.

// First, if the source is *never* equal to the original, then
// obviously it's a real possible theme and in_dictionary should
// return false.

// But even if the source is sometimes equal to the original, the
// "dictionary" entry might be a themed entry that has simply been
// used before!  So in this case, we evaluate it based on the Flamingo
// matches found.  We assume that the fill is not really "in" the
// dictionary if there is a Flamingo match with a score separated from
// the possible "dictionary" entry by more than MAX_OLD_DELTA and the
// Levenshtein distance is at most MAX_OLD_DIFF.  If that happens,
// then this is once again a real possible theme.

bool database::in_dictionary(const vector<theme_fill> &fills) const
{
  if (find_if(fills.begin(),fills.end(),
              [](const theme_fill &tf) {
                return tf.source == tf.original;
              } ) == fills.end())
    return false;        // all words changed in fill (source != original)
  return find_if(fills.begin(),fills.end(),
                 [&](const theme_fill &tf) {
                   return
                     get_mlg_score(tf.source) >
                       get_mlg_score(tf.original) + MAX_OLD_DELTA &&
                     changes(tf.source,tf.original) <= MAX_OLD_DIFF;
                 } ) == fills.end();
}

// A puzzle is unthemed if we can't figure out a theme.  That might be
// because there are less than MIN_THEME theme entries, because there
// is a known rebus theme (i.e., a nonalpha character in some fill),
// or because we can't identify a candidate theme frequently enough on
// the longest words.  "Frequently enough" means we need candidate
// theme types for two or more theme entries if there are only
// MIN_THEME of them, or three or more if there are more than
// MIN_THEME.  We do the counting starting at the longest words, and
// stopping when we have at least MIN_THEME candidates.

bool possible_theme::unthemed(bool sound, const database &d) const
{
  if (size() < MIN_THEME) return true;
  if (sound) return false;      // sound-based is weird enough that we
                                // should always give it a shot, which
                                // is what happens if unthemed fails
  vector<unsigned> theme_word(LONG_WORD + 1,0), nontheme_word(LONG_WORD + 1,0);
  for (const vector<theme_fill> &v : *this) {
    for (auto ch : v[0].original) if (!isalpha(ch)) return true; // rebus!
    bool dict = d.in_dictionary(v);
    ++(dict? nontheme_word : theme_word)[v[0].original.size()];
  }

  unsigned themed_tot = 0, unthemed_tot = 0;
  for (unsigned i = LONG_WORD ; i > 0 ; --i) {
    themed_tot += theme_word[i];
    unthemed_tot += nontheme_word[i];
    if (i <= MAX_THEME_LENGTH && themed_tot + unthemed_tot >= MIN_THEME) break;
  }
  return (themed_tot <= 1 + (themed_tot + unthemed_tot != MIN_THEME));    
}

// Get the theme words in a puzzle.  That means that for each slot, we
// need a list of possibilities.  For each, we need to produce the
// current fill, then the proposed fill.  This routine assumes sound = false.

vector<theme_fill> database::get_themewords(const string &f) const
{
  vector<string> fm = flamingo_match_no_plurals(f,false,FLAMINGO_MATCH,false);
  theme_fill tf;
  tf.original = tf.poriginal = f;
  vector<theme_fill> ans;
  for (auto &f : fm) {
    ans.push_back(tf);
    ans.back().source = ans.back().psource = f;
  }
  return ans;
}

// Start suggestions for a sound-based theme.  Get a single
// pronunciation, and then look for things in the Flamingo database
// that sound similar (it takes too long to process all the
// pronunciations).  Make a theme suggestion out of each of them,
// ordering by Levenshtein distance to the original pronunciation.
// Only keep FLAMINGO_MATCH matches for the original string, even if
// it has many pronunciations.

vector<theme_fill> database::get_soundwords(word &wd, unsigned &distance) const
{
  const string &f = wd.get_solution();
  distance = LONG_WORD;
  multimap<unsigned,theme_fill> ans;
  vector<string> prons = get_pronunciations(f,true,false);
                                // single pronunciation; don't call flits
  theme_fill tf;
  tf.original = f;
  for (auto &pron : prons)
    for (const string &f : flamingo_match_no_plurals(pron,true,FLAMINGO_MATCH,
                                                     false)) {
      tf.poriginal = pron;
      tf.psource = f;
      const vector<string> &wds = pron_to_words[f];
      if (!wds.empty()) tf.source = wds[0];
      unsigned dist = levenshtein(pron,f);
      ans.insert(make_pair(dist,tf));
      if (dist < distance) {
        wd.set_psound(f);       // in case it's a pun, record best match
        distance = dist;
      }
    }
  vector<theme_fill> result;
  for (auto &i : ans) {
    if (i.second.poriginal != ans.begin()->second.poriginal) continue;
    // use best pronunciation only
    result.push_back(i.second);
    if (result.size() == FLAMINGO_MATCH && i.first != ans.begin()->first) break;
  }
  return result;
}

// Number of words that appear to be homonyms.

unsigned possible_theme::homonym_count(const database &d) const
{
  unsigned ct = 0;
  for (const vector<theme_fill> &tf : *this)
    if (!tf.empty() && d.homonym(tf[0].original)) ++ct;
  return ct;
}

// Analyze a puzzle, modifying a list of candidate themes.  Each is
// actually a sourced_theme, which is a candidate theme and the number
// of theme entries that support it.

// 1. For each theme type t, get base_themes[t] = number of elements
// of *this supported by the given theme type.  For sound-based,
// consider homonyms only (that's the NOTHEME element of base_themes
// in this case).

// Now in the non-sound case, look at each vector<theme_fill> in the
// possible_theme.  These are ordered by distance from source to
// original; set first_match to indicate that the distance is
// potentially zero.  There are now the following cases:

// 1a. first_match, only one entry.  Nothing to do.

// 1b. first_match, 2+ entries.  If the first match is better
//     (higher-scoring source), abandon it.

// 1c. Otherwise, look at everything that's not a match but block
//     portmanteaus on the very first entry.  For each theme t found,
//     set bt[t] because this vector<theme_fill> supports it.  Then
//     add all the bt's into the overall base_themes counts.

// 2. Add to this all of the Levenshtein-distance based themes, which
// are found by lev_analyze.  Note that sound-based themes are handled
// here, and there is no collapsing done; we just have a long vector
// of themes and counts.

// 3. Convert that vector into a set, which means that for a
// particular theme type, we'll only keep the sourced theme with
// higher count (as opposed to adding the two counts).  Now we have to
// see if it's worth adding into the overall answer _ans_.

// 3a. If it's Portmanteau, it needs to have enough support.
//     (Portmanteaus are easy to create by accident, so need more
//     support.)  Ditto for anagrams.
// 3b. Sound-based themes are easier to produce, so they get dinged
//     by SOUND_ADJUST.
// 3c. Keep all the best-support sourced themes.  So if ans is
//     currently empty, keep whatever you just produced.  If the new
//     answer has more sources than what you've got, throw out
//     everything you've got and start with the new sourced_theme.
//     And finally, if it's got the same number of sources, keep it.
// 3d. Mark the last sourced_theme as having sound if that's what
//     you're doing.

void possible_theme::analyze(vector<sourced_theme> &ans, const database &d, 
                             bool sound, bool explain)
{
  if (unthemed(sound,d)) return;
  vector<unsigned> base_themes(1 + NOTHEME,0);
  if (sound) {
    base_themes[NOTHEME] = homonym_count(d);                   // 1
    for (const vector<theme_fill> &w : *this)
      for (auto &tf : w)
        if (d.is_sound_anagram(tf.original)) {
          ++base_themes[ANAGRAM];
          break;                // each word only checked once
        }
  }
  else for (const vector<theme_fill> &w : *this) {
      size_t first_match = (w[0].source == w[0].original); // skip first if match
      if (first_match == w.size()) continue;                            // 1a
      if (first_match) {                                                // 1b
        unsigned sc1 = d.get_mlg_score(w[1].source);
        unsigned sc2 = d.get_mlg_score(w[0].source);
        if (sc1 <= sc2) continue;
      }
      vector<unsigned> bt(NOTHEME,0);
      for (size_t j = first_match ; j < w.size() ; ++j) {               // 1c
        vector<theme> th = d.find_special_themes(w[j].original,j == 0,explain);
        for (auto &t : th) bt[t.get_type()] = 1;
      }
      for (size_t j = 0 ; j < NOTHEME ; ++j) base_themes[j] += bt[j];
    }

  vector<sourced_theme> lev_based;
  sourced_theme l = lev_analyze(sound);
  if (l.sources) lev_based.push_back(l);                                // 2
  for (unsigned i = 0 ; i <= NOTHEME ; ++i) 
    if (base_themes[i])
      lev_based.push_back(sourced_theme(theme(sound,(theme_type) i),
                                        base_themes[i]));

  for (auto &i : set<sourced_theme>(lev_based.begin(),lev_based.end())) { // 3
    if (i.is_portmanteau() && i.sources < PORTMANTEAU_MINIMUM) continue;  // 3a
    if (i.is_anagram() && i.sources < ANAGRAM_MINIMUM) continue;
    sourced_theme src = i;
    if (sound && !src.is_anagram() && !src.is_homonym()) {              // 3b
      if (src.wild() || src.sources < 1 + SOUND_ADJUST) continue;
      else src.sources -= SOUND_ADJUST;
    }
    if (ans.empty()) ans.push_back(src);                                // 3c
    else if (ans[0].sources < src.sources) {
      ans.clear();
      ans.push_back(src);
    }
    else if (ans[0].sources == src.sources) ans.push_back(src);
    else break;
    if (sound) ans.back().get_sound() = true;                           // 3d
  }
}

// Pronunciations from fill.  If in the DB or we have a cached answer,
// it's easy.  Otherwise, get the unsplit pronunciation and, for each
// possible splitting, the associated pronunciation.

// One caveat.  If there is a multidata, we use that instead of
// recomputing it.  (If md is NULL, we use get_multiparses to do the
// work and then convert to a multidata to actually use it.)

// At the very end, if nothing else worked, we let pronounce() analyze
// the whole word using flite.

vector<string> database::get_pronunciations(const string &fill, bool unique,
                                            bool call_flite,
                                            const vector<segmentation> &segs)
  const
{
  vector<string> lookup = pronunciations[fill];
  if (!lookup.empty()) return lookup;
  if (pronunciation_cache[unique].find(fill) !=
      pronunciation_cache[unique].end())
    return pronunciation_cache[unique][fill];
  vector<string> &cache = pronunciation_cache[unique][fill]; // answer goes here
  set<string> prons;
  if (!segs.empty()) for (auto &seg : segs) {
      vector<string> mp = multiword_pronunciation(fill,seg);
      prons.insert(mp.begin(),mp.end());
    }
  else {
    multidata md;
    pdata p;                    // doesn't matter; we only want the string part
    map<string,multiparse> m = get_multiparses(fill,false,1,false);
    if (m.empty()) {
      if (call_flite) cache.push_back(pstring_to_string(pronounce(fill)));
      return cache;
    }
    for (auto &mp : m.begin()->second) md[mp.first] = p;

    if (!unique || md.size() == 1)
      for (auto &j : md) {
        vector<string> mp = multiword_pronunciation(fill,j.first);
        prons.insert(mp.begin(),mp.end());
      }
    else {                      // only pronounce best splitting if possible
      segmentation best_seg;
      unsigned best_val = 0;
      for (auto &j : md) {
        unsigned val = multiword_score(fill,j.first);
        if (val > best_val) {
          best_val = val;
          best_seg = j.first;
        }
      }
      vector<string> mp = multiword_pronunciation(fill,best_seg);
      prons.insert(mp.begin(),mp.end());
    }
  }
  if (call_flite && prons.empty())
    prons.insert(pstring_to_string(pronounce(fill)));
  return cache = vector<string>(prons.begin(),prons.end());
}

// Once we have the theme, we want to use it to construct suggestions,
// and then return suggestions that appear to match the fill that we
// had to start the process.  This utility function counts the number
// of characters that differ between s1 and s2.

unsigned mismatch(const string &s1, const string &s2)
{
  unsigned m = 0;
  for (size_t i = 0 ; i < s1.size() ; ++i) if (s1[i] != s2[i]) ++m;
  return m;
}

// The money function.  Given a possible_theme, what suggestions are
// there for various theme entries?

// When called, *this is the possible theme we're considering, a
// vector of vector of theme_fills, where a theme_fill is basically a
// source->fill mapping.  At this point, only the sources (and
// pronunciations thereof) are of interest.  We're going to produce a
// new possible_theme that has suggested replacements as well.  (Note
// that one theme_fill in *this may lead to many suggested
// replacements, which is why we can't do this destructively.)

// Also when called, _themes_ is just a list of possible themes, each
// of which have the same number of sources.  We only care about the
// themes themselves at this point.

// We split the functionality into the work (the global function
// below) and the wrapper that processes and produces a possible_theme
// instead of a vector<themed_fill>.

// For the work, there are three phases:

// 1. Get all the possible source->fill mappings as _reps_.  These are
// built up by considering each element of _themes_ and of _fills_ and
// using get_hints to do the work.  If the theme type is Portmanteau,
// though, we only consider the first theme in _themes_.

// 2. Turn _reps_ into scored suggestions, where the score is the
// difference between the original (what we filled in during the
// solve) and the proposed new fill.

// 3. Keep the best THEME_SUGGESTIONS suggestions, but only keep each
// possible replacement fill once.  (The equality operator on
// theme_fills only looks at the actual fill.)

vector<theme_fill> database::analyze_theme(const vector<theme_fill> &fills,
                                           const vector<sourced_theme> &themes)
  const
{
  vector<theme_fill> reps;                                              // 1
  for (const sourced_theme &src : themes) {
    for (const theme_fill &tf : fills) {
      vector<theme_fill> alts = src.get_hints(tf,*this);
      reps.insert(reps.end(),alts.begin(),alts.end());
      if (src.is_portmanteau()) break;
    }
  }

  multimap<unsigned,theme_fill> suggestions;                            // 2
  for (auto &r : reps)
    suggestions.insert(make_pair(mismatch(r.fill,r.original),r));

  vector<theme_fill> ans;                                               // 3
  for (auto &s : suggestions)
    if (find(ans.begin(),ans.end(),s.second) == ans.end()) {
      ans.push_back(s.second);
      if (ans.size() == THEME_SUGGESTIONS) break;
    }
  return ans;
}

possible_theme possible_theme::analyze_theme(const vector<sourced_theme> &themes,
                                             const database &d) const
{
  possible_theme ans;
  for (auto &tf : *this) ans.push_back(d.analyze_theme(tf,themes));
  return ans;
}

// The basic "forward generation" stuff is above.  But we also need
// stuff to look at candidate fill, and see if it conforms to one
// theme or another.  (This is needed by theme_conforming in
// multiword.cpp.)  That comes next.  In general, calls to this code
// include a bool _strong_ that means to want to check not only that
// this string could be the result of a theme operation, but that the
// theme operation would be acting on a word or phrase that was in the
// database to start.

// Does a multidata split off a piece at the beginning or end of the
// appropriate size?

bool multidata::separates(unsigned sz, bool beginning) const
{
  for (auto &i : *this)
    if ((beginning? i.first[0] : i.first.back()) == sz) return true;
  return false;
}

// Might a string be the result of an add operation?  Note that there
// are two cases, one where the added string is known and the other
// where only its length is known.  In the latter case, in order for
// the test to have any teeth, we have to know the size exactly and
// this has to be a "strong" test (which checks to see if the original
// string is in the database).  If we know the string to add, we just
// check to see that it's there, in the right place, and doesn't split
// off an entire word.

// Now in the non-strong case, we're done.  In the strong case, we
// check to see that the origin is in the database.  That's easy for a
// ? or ! query.  In the normal case, we just want to remove the word
// and see if what's left is good.  Only catch is that we don't check
// at the beginning or end if that would correspond to adding a whole
// word.

// In sum, then, we have the following process:

// 1. If the fill is shorter than the piece to add, return "no".
// 2. If you're adding a known piece and it's not there, return "no".
// 3. Construct the multidata if nothing useful was supplied
//    (non-sound case only).
// 4. If this is the weak case, you're now done!
// 5. In the wildcard case, the residue has to be a word
// 6. Otherwise, we have the base case.  For each possible point, if
//     it matches the string being added, and taking that piece ouf
//     leaves a word, return "yes".

// FIXME? NOTE THAT THIS DOES NOT SUPPORT MULTIPLE ADDITIONS IN THE
// STRONG CASE.

bool theme::conforms_add(const string &fill, bool strong, const database &d)
  const
{
  if (fill.size() <= str1.size()) return false;                         // 1
  if (!iswild(str1) && fill.find(str1) == string::npos) return false;   // 2
  unsigned sz = str1.size();
  if (!sound && find(fill.begin(),fill.end(),WILD) != fill.end() &&
      d.get_multiwords(fill,false,1).empty())
    return false;                                                       // 3
  if (!strong) return true;                                             // 4
  switch(str1[0]) {
  case '?': return d.includes(fill.substr(sz),sound);                   // 5
  case '!': return d.includes(fill.substr(0,fill.size() - sz),sound);
  }

  unsigned lo = 0;
  unsigned hi = fill.size() - sz + 1;
  for (unsigned i = lo ; i <= hi ; ++i)                                 // 6
    if (fill.substr(i,sz) == str1 &&
        d.includes(fill.substr(0,i) + fill.substr(i + sz),sound))
      return true;
  return false;
}

// For REMOVE, all we can do is the strong case where we know the
// string.  In that case, we try adding it in various spots and see if
// we get something in the database.

// FIXME?  We could actually do "remove n at the end" pretty easily,
// and could even do "remove n at the beginning by keeping all of the
// reversed words as well.  But I don't think that would be a very fun
// theme.

bool theme::conforms_remove(const string &fill, bool strong, const database &d)
  const
{
  if (!strong || iswild(str1)) return false;
  for (size_t i = 0 ; i < fill.size() ; ++i)
    if (d.includes(fill.substr(0,i) + str1 + fill.substr(i),sound)) return true;
  return false;
}

// CHANGE: Yes, if the result string is there and, if need be, the
// result of undoing the change is in the database.  It's tricky
// because any *subset* of the given str2's can be a replacement.

bool theme::conforms_change(const string &fill, bool strong, const database &d)
  const
{
  size_t idx = fill.find(str2);
  if (idx == string::npos) return false;
  if (!strong) return true;
  vector<unsigned> locs(1,idx);
  for (idx = fill.find(str2,1 + idx) ; idx != string::npos ; 
       idx = fill.find(str2,1 + idx))
    locs.push_back(idx);
  unsigned sz = locs.size();
  locs.push_back(fill.size());  // change at end
  for (int mask = 1 ; mask < (1 << sz) ; ++mask) {
    string alt = fill.substr(0,locs[0]);
    for (size_t i = 0 ; i < sz ; ++i) {
      alt += (mask & (1 << i))? str1 : str2;
      alt += fill.substr(locs[i] + str2.size(),
                         locs[i + 1] - (locs[i] + str2.size()));
    }
    if (d.includes(alt,sound)) return true;
  }
  return false;
}

// HOMONYM: Strong only; if true, pronounce and unpronounce.

bool database::homonym(const string &fill) const
{
  for (const string &h : get_pronunciations(fill,false,false)) {
                                // multiple pronunciations but don't call flite
    vector<string> wds = pron_to_words[h];
    if (wds.size() > 1 || (wds.size() == 1 && wds[0] != fill)) return true;
  }
  return false;
}

bool theme::conforms_homonym(const string &fill, bool strong, const database &d)
  const
{
  return strong && d.homonym(fill);
}

// Distance of a pun.  Just minimum distance to all pronunciations,
// given pun "target".

unsigned database::pun_distance(const word &w, const string &fill) const
{
  if (w.psound().empty()) return LONG_WORD;
  unsigned dist = LONG_WORD;
  for (auto &p : get_pronunciations(fill,false,false)) {
                                // multiple pronunciations but don't call flite
    unsigned l = levenshtein(p,w.psound());
    if (l == 0) return 0;
    dist = min(dist,l);
  }
  return dist;
}

// Here is the case split.  Some we give up on and just return no.

bool theme::conforms2(const word &w, string fill, bool strong, const database &d)
  const
{
  switch (type) {
  case ADD_STRING:    return conforms_add   (fill,strong,d);
  case REMOVE_STRING: return conforms_remove(fill,strong,d);
  case CHANGE_STRING: return conforms_change(fill,strong,d);
  case ANAGRAM:
    return sound? d.is_sound_anagram(fill) : d.is_anagram(fill,false);
  case PORTMANTEAU:   return false; // just too hard
  case PUN:           {
    unsigned dist = d.pun_distance(w,fill);
    return dist && dist < 3;    // pun has to be close
  }
  case NOTHEME:       return sound? conforms_homonym(fill,strong,d) : 0;
  }
  return false;                 // can't get here
}

// We also want to dink around with the source word using Flamingo,
// and this code does that.  Basically undo the theme bit, run
// flamingo_match, and put the theme modification back in.

void theme::flamingo_add(vector<string> &ans, const string &fill,
                         const database &d) const
{
  unsigned sz = str1.size();
  if (fill.size() <= sz) return;
  string before, after;
  switch (str1[0]) {
  case '?': after = fill.substr(sz); break;
  case '!': before = fill.substr(0,fill.size() - sz); break;
  default: 
    size_t pos = fill.find(str1);
    if (pos == string::npos) return;
    before = fill.substr(0,pos);
    after = fill.substr(pos + sz);
  }
  for (const string &rep : 
         d.flamingo_match(before + after,sound,FLAMINGO_MATCH,true))
    if (rep.size() + sz == fill.size())
      ans.push_back(rep.substr(0,before.size()) + 
                    fill.substr(before.size(),sz) +
                    rep.substr(before.size()));
}

// Note that we can deal with wildcards here; Flamingo will try and
// replace them with something sensible.

void theme::flamingo_remove(vector<string> &ans, const string &fill,
                            const database &d) const
{
  unsigned sz = str1.size();
  for (unsigned i = 0 ; i <= fill.size() ; ++i)
    if (i && str1[0] == '?') continue;
    else if (i != fill.size() && str1[0] == '!') continue;
    else {
      string rep = fill.substr(0,i) + str1 + fill.substr(i);
      for (const string &r : d.flamingo_match(rep,sound,FLAMINGO_MATCH,true))
        if (r.size() == fill.size() + sz) {
          if (str1[0] == '?') ans.push_back(r.substr(sz));
          else if (str1[0] == '!') ans.push_back(r.substr(0,fill.size()));
          else {
            size_t p = r.find(str1);
            if (p != string::npos) 
              ans.push_back(r.substr(0,p) + r.substr(p + sz));
          }
        }
    }
}

void theme::flamingo_change(vector<string> &ans, const string &fill,
                            const database &d) const
{ 
  size_t p = fill.find(str2);
  if (p == string::npos) return;
  string rep = fill.substr(0,p) + str1 + fill.substr(p + str2.size());
  for (const string &r : d.flamingo_match(rep,sound,FLAMINGO_MATCH,true)) {
    if (r.size() != fill.size()) continue;
    size_t p2 = r.find(str1);
    if (p2 != string::npos) 
      ans.push_back(r.substr(0,p2) + str2 + r.substr(p2 + str1.size()));
  }
}

void theme::flamingo(vector<string> &ans, const string &fill,
                     const database &d) const
{
  if (sound) return;
  switch (type) {
  case ADD_STRING:    flamingo_add(ans,fill,d);    break;
  case REMOVE_STRING: flamingo_remove(ans,fill,d); break;
  case CHANGE_STRING: flamingo_change(ans,fill,d); break;
  default: break;
  }
}

// Bonus for a theme match.  Strong or not.

float theme::bonus(bool strong) const
{
  return strong? THEME_STRONG : THEME_WEAK;
}

// Does a string _fill_ conform to a particular theme?  The above
// functions do all the work; here, we just compute the score based on
// _strong_.

float theme::conforms(const word &w, const string &fill, bool strong,
                      const database &d, bool verbose) const
{
  return conforms2(w,fill,strong,d)? bonus(strong) : 0;
}

// All strings of a given length that are pronounced _str_.  If _any_
// is true, stop and return as soon as you've found one such entry.

// In actuality, sequence does the work, with the "word set" given by
// all of the pronunciations in the database (i.e., the keys in
// pron_to_words).  So use sequence to generate a splitting.  Then for
// each segmentation, split the phoneme sequence as given, and look it
// up in pron_to_words to get the words that might be pronounced that
// way.  Now serialize produces a list of candidate fills; collect all
// that are the right length.

set<string> database::unpronounce(const string &str, unsigned len, bool any) 
  const
{
  splitting init;
  init.insert(make_pair("",segmentation()));
  split_cache sc;
  set<string> ans;
  for (auto &i : sequence(str,pron_to_words.get_keys(),raw_split[len],init,sc)) {
    segmentation &s = i.second;
    vector<vector<string>> words(s.size());
    unsigned idx = 0;
    for (unsigned j = 0 ; j < s.size() ; ++j) {
      words[j] = pron_to_words[str.substr(idx,s[j])];
      idx += s[j];
    }
    vector<vector<string>> fills = serialize(words);
    for (auto &f : fills) {
      string a = accumulate(f.begin(),f.end(),string());
      if (len == 0 || a.size() == len) {
        ans.insert(a);
        if (any) return ans;
      }
    }
  }
  return ans;
}

// Simple theme testing functionality; theme words are in the given
// file and we just find the theme and report.

void database::test_theme(const string &fname) const
{
  vector<string> twords;
  string w;
  ifstream inf(fname);
  check_stream(inf,fname);
  while (inf >> w) twords.push_back(w);
  possible_theme p(twords.size()), q(twords.size());
  unsigned distance;
  for (size_t i = 0 ; i < twords.size() ; ++i) {
    word w;                     // need a word argument for get_soundwords
    w.fill = twords[i];
    p[i] = get_themewords(twords[i]);
    q[i] = get_soundwords(w,distance);
  }
  cout << "NO SOUNDS\n";
  for (auto &i : p) cout << i << endl;
  cout << "\nSOUNDS\n";
  for (auto &i : q) cout << i << endl;
}

void reproduce_theme(const string &fname)
{
  possible_theme thm;
  ifstream inf(fname);
  theme_fill tf;
  while (inf >> tf) {
    if (!thm.empty() && thm.back()[0].original == tf.original)
      thm.back().push_back(tf);
    else thm.push_back(vector<theme_fill>(1,tf));
  }
  cout << "theme fills: " << thm << endl;
  cout << thm.lev_analyze(true) << endl;                   // include ADD s
}

void reproduce_theme(const vector<string> &files)
{
  for (auto &f : files) reproduce_theme(f);
}
