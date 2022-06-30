//*********************************************************************//
//*		DISTANCE.H --- header file for computing edit distance*//
//*                                                                   *//
//*Copyright (C) Michael Gilleland (http://www.merriampark.com/ld.htm)*//
//*                                                                   *//
//*********************************************************************//

#ifndef __EDITDISTANCE_H__
#define __EDITDISTANCE_H__

class EditDistance
{
 public:
  int LD (char const *s, char const *t);
  bool similar(char const *s, char const *t, const int threshold);

  private:
  int Minimum (int a, int b, int c);
  int *GetCellPointer (int *pOrigin, int col, int row, int nCols);
  int GetAt (int *pOrigin, int col, int row, int nCols);
  void PutAt (int *pOrigin, int col, int row, int nCols, int x);
};

#endif // __EDITDISTANCE_H__
