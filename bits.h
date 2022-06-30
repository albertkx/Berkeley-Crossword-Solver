#ifndef _BITS_
#define _BITS_

#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

#define usize (8 * sizeof(unsigned long)) // # bits you can process at once

// We also have (potentially quite big) vector<bool>s that allow us to
// compute which words in a given class have particular letters in
// particular places.  A _bitmap_ is basically a vector<bool> that 
// allows for conjunction (and any other logical operations that we
// might need; conjunction just seems to be it at the moment).

class bitmap {
  vector<unsigned long> bits;

public:
  bitmap(unsigned n = 0, unsigned lo = 0) : bits((n + usize - 1) / usize,0) {
    if (n && lo < usize) bits[0] = ~(((unsigned long) -1) << lo);
    // beware n=0, lo=0!
    else for (unsigned i = 0 ; i < lo ; ++i) set_bit(i,true);
  }

  bitmap(unsigned n, unsigned lo, unsigned hi) :
    bits((n + usize - 1) / usize,0) {
      if (hi < usize) bits[0] = (~(((unsigned long) -1) << (hi - lo))) << lo;
      else for (unsigned i = lo ; i < hi ; ++i) set_bit(i,true);
  }

  bool operator[] (unsigned idx) const {
    return bits[idx / usize] & ((unsigned long) 1 << (idx % usize));
  }

private:
  void _set_bit(unsigned idx) {
    if (idx / usize >= bits.size()) bits.resize(1 + idx / usize,0);
    bits[idx / usize] |= ((unsigned long) 1 << (idx % usize));
  }

  void _clear_bit(unsigned idx) {
    bits[idx / usize] &= ~((unsigned long) 1 << (idx % usize));
  }

public:
  void set_bit(unsigned idx, bool val) {
    if (val) _set_bit(idx);
    else _clear_bit(idx);
  }

  bitmap operator~() const;

  void initialize();            // initialize various data structures

  static const unsigned npos = -1;
  unsigned size() const { return usize * bits.size(); }
  unsigned count_bits() const;
  unsigned first_bit(unsigned lidx = 0) const; // npos if nothing left
  unsigned next_including(int idx) const;      // npos if nothing left
  unsigned next_excluding(int idx) const;      // npos if nothing left
  unsigned nth(unsigned idx) const;

  // for conjunction, note that we resize *down* to the smaller size,
  // so if this object is bigger, we shrink it.  (We resize down
  // because anything past the end is assumed to be zero, and it's
  // conjunction.)
  void operator &=(const bitmap &other) {
    if (bits.size() > other.bits.size()) bits.resize(other.bits.size());
    for (size_t i = 0 ; i < bits.size() ; ++i) bits[i] &= other.bits[i];
  }
};

ostream &operator<<(ostream &os, const bitmap &b);

#endif
