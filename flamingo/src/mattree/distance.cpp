//*********************************************************************//
//*		DISTANCE.CPP --- compute edit distance between two strings    *//
//*                                                                   *//
//*Copyright (C) Michael Gilleland (http://www.merriampark.com/ld.htm)*//
//*                                                                   *//
//*********************************************************************//

#include <stdafx.h>
#include "distance.h"
#include <string.h>
#include <malloc.h>
#include <iostream>
using namespace std;

//****************************
// Get minimum of three values
//****************************

int Distance1::Minimum (int a, int b, int c)
{
int mi;

  mi = a;
  if (b < mi) {
    mi = b;
  }
  if (c < mi) {
    mi = c;
  }
  return mi;

}

//**************************************************
// Get a pointer to the specified cell of the matrix
//**************************************************

int *Distance1::GetCellPointer (int *pOrigin, int col, int row, int nCols)
{
  return pOrigin + col + (row * (nCols + 1));
}

//*****************************************************
// Get the contents of the specified cell in the matrix
//*****************************************************

int Distance1::GetAt (int *pOrigin, int col, int row, int nCols)
{
int *pCell;

  pCell = GetCellPointer (pOrigin, col, row, nCols);
  return *pCell;

}

//*******************************************************
// Fill the specified cell in the matrix with the value x
//*******************************************************

void Distance1::PutAt (int *pOrigin, int col, int row, int nCols, int x)
{
int *pCell;

  pCell = GetCellPointer (pOrigin, col, row, nCols);
  *pCell = x;

}

//*****************************
// Compute Levenshtein distance
//*****************************

int Distance1::LD (char const *s, char const *t)
{
int *d; // pointer to matrix
int n; // length of s
int m; // length of t
int i; // iterates through s
int j; // iterates through t
char s_i; // ith character of s
char t_j; // jth character of t
int cost; // cost
int result; // result
int cell; // contents of target cell
int above; // contents of cell immediately above
int left; // contents of cell immediately to left
int diag; // contents of cell immediately above and to left
int sz; // number of cells in matrix

  // Step 1

  n = strlen (s);
  m = strlen (t);
  if (n == 0) {
    return m;
  }
  if (m == 0) {
    return n;
  }
  sz = (n+1) * (m+1) * sizeof (int);
  d = (int *) malloc (sz);

  // Step 2

  for (i = 0; i <= n; i++) {
    PutAt (d, i, 0, n, i);
  }

  for (j = 0; j <= m; j++) {
    PutAt (d, 0, j, n, j);
  }

  // Step 3

  for (i = 1; i <= n; i++) {

    s_i = s[i-1];

    // Step 4

    for (j = 1; j <= m; j++) {

      t_j = t[j-1];

      // Step 5

      if (s_i == t_j) {
        cost = 0;
      }
      else {
        cost = 1;
      }

      // Step 6

      above = GetAt (d,i-1,j, n);
      left = GetAt (d,i, j-1, n);
      diag = GetAt (d, i-1,j-1, n);
      cell = Minimum (above + 1, left + 1, diag + cost);
      PutAt (d, i, j, n, cell);
    }
  }

  // Step 7

  result = GetAt (d, n, m, n);
  free (d);
  return result;

}

