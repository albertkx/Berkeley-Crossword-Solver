//*********************************************************************//
//*		DISTANCE.H --- header file for computing edit distance        *//
//*                                                                   *//
//*Copyright (C) Michael Gilleland (http://www.merriampark.com/ld.htm)*//
//*                                                                   *//
//*********************************************************************//

#ifndef DISTANCE1_H
#define DISTANCE1_H

class Distance1
{
  public:
    int LD (char const *s, char const *t);
  private:
    int Minimum (int a, int b, int c);
    int *GetCellPointer (int *pOrigin, int col, int row, int nCols);
    int GetAt (int *pOrigin, int col, int row, int nCols);
    void PutAt (int *pOrigin, int col, int row, int nCols, int x);
};

#endif
