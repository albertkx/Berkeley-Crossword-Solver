/*
  $Id: ppdentry.cc 4143 2008-12-08 23:23:55Z abehm $

  Copyright (C) 2007 by The Regents of the University of California

  Redistribution of this file is permitted under the terms of the 
  BSD license

  Date: 01/14/2006
  Author: Rares Vernica <rares (at) ics.uci.edu>
*/

#include "ppdentry.h"

#include <iomanip>
#include <iterator>

using namespace std;

PPDEntry::PPDEntry(SimVect v1, SimVect v2, SimType d, unsigned c, float f): 
  vect1(v1), vect2(v2), distM(d), countE(c), fract(f)
{
  if (distM > DIST_THRESHOLD) 
    distM = DIST_THRESHOLD;
}

bool PPDEntry::operator==(const PPDEntry &e) const 
{
  if (this == &e)
    return true;
  if (vect1 == e.vect1 && 
      vect2 == e.vect2 && 
      distM == e.distM && 
      countE == e.countE && 
      fract == e.fract)
    return true;
  return false;
}

ostream& operator<<(ostream &out, const PPDEntry &e)
{
  return out << e.vect1 << "\t" << e.vect2 << "\t"
             << setw(5) << e.distM << "\t"
             << setw(5) << e.countE << "\t" << e.fract;
}

istream& operator>>(istream &in, PPDEntry &e)
{
  return in >> e.vect1 >> e.vect2 >> e.distM >> e.countE >> e.fract;
}

bool operator<(const PPDEntry &left, const PPDEntry &right)
{
  if (&left == &right)
    return false;
  if (left.vect1 != right.vect1)
    return left.vect1 < right.vect1;
  if (left.vect2 != right.vect2)
    return left.vect2 < right.vect2;
  return left.distM < right.distM;
}

ostream& operator<<(ostream &out, const ContPPDEntry &c)
{
  out << static_cast<unsigned>(c.size()) << endl << endl;
  copy(c.begin(), c.end(), ostream_iterator<PPDEntry>(out, "\n"));
  return out;
}

istream& operator>>(istream &in, ContPPDEntry &c)
{
	unsigned n;
	in >> n;
	for (unsigned i = 0; i < n; i++) {
		PPDEntry e;
		in >> e;
		c.insert(e);
	}
	return in;
}

#if SIM_DIST == 1 && SIM_VECT == 1

void PPDEntry::serialize(ofstream &out) const
{
//   vect1.serialize(out);
//   vect2.serialize(out);
//   out.write(reinterpret_cast<const char*>(&distM), sizeof(SimType));

  // < I, D, S > < I, D, S > D   in [0, 15]
  // unsigned -  32bit

  unsigned data = 0;
  
  data |= 0x0000000F & vect1.ins;
  data |= 0x000000F0 & (vect1.del << 4);
  data |= 0x00000F00 & (vect1.sub << 8);
  data |= 0x0000F000 & (vect2.ins << 12);
  data |= 0x000F0000 & (vect2.del << 16);
  data |= 0x00F00000 & (vect2.sub << 20);
  data |= 0x0F000000 & (distM << 24);

  out.write(reinterpret_cast<const char*>(&data), sizeof(unsigned));
  out.write(reinterpret_cast<const char*>(&countE), sizeof(unsigned));

//   unsigned fractU = static_cast<unsigned>(fract * 10000);
//   unsigned char buf[2];
//   buf[0] = 0xFF & fractU;
//   buf[1] = 0xFF & (fractU >> 8);
//   out.write(reinterpret_cast<const char*>(&buf), sizeof(unsigned char) * 2);

  out.write(reinterpret_cast<const char*>(&fract), sizeof(float));
}

void PPDEntry::deserialize(ifstream &in)
{
//   vect1.deserialize(in);
//   vect2.deserialize(in);
//   in.read(reinterpret_cast<char*>(&distM), sizeof(SimType));

  unsigned data = 0;

  in.read(reinterpret_cast<char*>(&data), sizeof(unsigned));

  vect1.ins = data & 0x0000000F;
  vect1.del = (data & 0x000000F0) >> 4;
  vect1.sub = (data & 0x00000F00) >> 8;
  vect2.ins = (data & 0x0000F000) >> 12;
  vect2.del = (data & 0x000F0000) >> 16;
  vect2.sub = (data & 0x00F00000) >> 20;
  distM    = (data & 0x0F000000) >> 24;

  in.read(reinterpret_cast<char*>(&countE), sizeof(unsigned));

//   unsigned fractU = 0;
//   unsigned char buf[2];
//   in.read(reinterpret_cast<char*>(&buf), sizeof(unsigned char) * 2);
//   fractU = (buf[1] << 8) | buf[0];
//   fract = static_cast<float>(fractU) / 10000;

  in.read(reinterpret_cast<char*>(&fract), sizeof(float));
}

#endif
