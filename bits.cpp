#include "bits.h"

// Implement vector<bool> class that (a) allows for conjunction and
// (b) uses longs for the underlying data structure.  Also implement
// vaguely fast counting of number of bits that are set and finding
// the next bit that's set from a given one.  Algorithms all assume
// that the bit sets are relatively sparse.

static unsigned bit_count[0x10000];   // # bits set
static int      lo_bit   [0x10000];   // lowest bit set (as bit number)
static unsigned lo_mask  [0x10000];   // lowest bit set (as bit itself)

// Initialize the statistics in the obvious way.

void bitmap::initialize()
{
  for (unsigned i = 0 ; i < 0x10000 ; ++i) {
    bit_count[i] = 0;
    lo_bit[i] = -1;
    for (unsigned j = 0 ; j < 16 ; ++j)
      if (i & (1 << j)) {
        ++bit_count[i];
        if (lo_bit[i] < 0) {
          lo_bit[i] = j;
          lo_mask[i] = (1 << j);
        }
      }
  }
}

// Number of bits that are on.  Just walk the vector, counting the
// bits in each long by splitting that long into shorts.

unsigned bitmap::count_bits() const
{
  unsigned ans = 0;
  for (auto l : bits) for ( ; l ; l >>= 16) ans += bit_count[l & 0xFFFF];
  return ans;
}

// First bit that's on.  Just walk along, looking for a nonzero long.
// When you find one, find the first short with a bit that's on and
// return the answer from that.

static inline unsigned found_bit(unsigned lidx, unsigned long n)
{
  for (unsigned ans = 0 ;; ++ans)
    if (n & 0xFFFF) return lidx * usize + 16 * ans + lo_bit[n & 0xFFFF];
    else n >>= 16;
  return bitmap::npos;
}

unsigned bitmap::first_bit(unsigned lidx) const
{
  for ( ; lidx < bits.size() ; ++lidx)
    if (bits[lidx]) return found_bit(lidx,bits[lidx]);
  return npos;
}

// Next bit that's on, possibly including the current index and
// possibly excluding it.  Only tricky part is looking for bits that
// are on in the current long but equal to or after the given index.
// But that's easy; just clear the bits through the given index and
// see what's left.  (But be careful: shifting -1 by usize does
// nothing so if we're already looking at the top bit and want to
// exclude the current index, we skip this part.)

unsigned bitmap::next_including(int idx) const
{
  unsigned lidx = idx / usize;
  if (lidx >= bits.size()) return npos;
  unsigned long curr = bits[lidx] & ((unsigned long) -1) << (idx % usize);
  if (curr) return found_bit(lidx,curr);
  return first_bit(lidx + 1);
}

unsigned bitmap::next_excluding(int idx) const
{
  if (idx < 0) return first_bit();
  unsigned lidx = idx / usize;
  if (lidx >= bits.size()) return npos;
  unsigned long curr;
  if ((idx + 1) % usize) {
    curr = bits[lidx] & ((unsigned long) -1) << (1 + idx % usize);
    if (curr) return found_bit(lidx,curr);
  }
  return first_bit(lidx + 1);
}

// Find the nth bit in a bitmap.  A short at a time, if the number of
// bits in the short isn't enough, just deduct that from what you're
// looking for and keep going.  If it is enough, clear the various
// low-order bits and return the low bit of what's left.

unsigned bitmap::nth(unsigned idx) const
{
  for (unsigned i = 0 ; i < bits.size() ; ++i ) {
    unsigned offset = 0;
    for (unsigned long l = bits[i] ; l ; l >>= 16)
      if (bit_count[l & 0xFFFF] > idx)  {
        l &= 0xFFFF;
        while (idx) {
          l ^= lo_mask[l];
          --idx;
        }
        return lo_bit[l] + 16 * offset + usize * i;
      }
      else {
        idx -= bit_count[l & 0xFFFF];
        ++offset;
      }
  }
  return npos;
}

bitmap bitmap::operator~() const
{
  bitmap ans(size());
  for (unsigned i = 0 ; i < bits.size() ; ++i) ans.bits[i] = ~bits[i];
  return ans;
}

// Print a bitmap.  Print the count, first BITPRINT elements, and then
// ... if there are more.

static const unsigned BITPRINT = 70;

ostream &operator<<(ostream &os, const bitmap &b)
{
  unsigned n = b.count_bits();
  os << n << ':';
  for (size_t i = 0 ; i < min(n,BITPRINT) ; ++i) os << ' ' << b.nth(i);
  if (n > BITPRINT) os << " ...";
  return os;
}
