
//
// $Id: rect.cpp 5770 2010-10-19 06:38:25Z abehm $
//
// rect.cpp
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: Michael Ortega-Binderberger <miki (at) ics.uci.edu>
//          Liang Jin <liangj (at) ics.uci.edu>
//          Chen Li <chenli (at) ics.uci.edu>
//

#include <stdio.h>
#include <stdlib.h>
#include "assert.h"
#include "index.h"

#include <float.h>
#include <math.h>

#define BIG_NUM (FLT_MAX/4.0)


#define Undefined(x) ((x)->boundary[0] > (x)->boundary[NUMDIMS])
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/*------------------------------------------------------------------
| Convert point to rectangle
--------------------------------------------------------------------*/
void PointToRectangle(struct Point *P, struct Rect *R)
{
  register int i;
  for(i=0; i<NUMDIMS; i++)
    {
      R->boundary[i]=P->position[i];
      R->boundary[NUMDIMS+i]=P->position[i];
    }
}

/*------------------------------------------------------------------
| Convert rectangle to point
--------------------------------------------------------------------*/
void RectangleToPoint(struct Rect *R, struct Point *P)
{
  register int i;
  for(i=0; i<NUMDIMS; i++)
    P->position[i]=R->boundary[i];
}

/*------------------------------------------------------------------
| Center of the rectangle
--------------------------------------------------------------------*/
struct Point Center(struct Rect *R)
{
  register struct Rect *r=R;
  struct Point p;
  register int i;
  for(i=0; i<NUMDIMS; i++)
    p.position[i]=(r->boundary[i]+r->boundary[NUMDIMS+i])/2;
  return p;
}

/*-----------------------------------------------------------------------------
| Initialize a rectangle to have all 0 coordinates.
-----------------------------------------------------------------------------*/
void RTreeInitRect(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	for (i=0; i<NUMSIDES; i++)
		r->boundary[i] = (RectReal)0;
}


/*-----------------------------------------------------------------------------
| Return a rect whose first low side is higher than its opposite side -
| interpreted as an undefined rect.
-----------------------------------------------------------------------------*/
struct Rect RTreeNullRect()
{
	struct Rect r;
	register int i;

	r.boundary[0] = (RectReal)1;
	r.boundary[NUMDIMS] = (RectReal)-1;
	for (i=1; i<NUMDIMS; i++)
		r.boundary[i] = r.boundary[i+NUMDIMS] = (RectReal)0;
	return r;
}


#if 0

/*-----------------------------------------------------------------------------
| Fills in random coordinates in a rectangle.
| The low side is guaranteed to be less than the high side.
-----------------------------------------------------------------------------*/
void RTreeRandomRect(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	register RectReal width;
	for (i = 0; i < NUMDIMS; i++)
	{
		/* width from 1 to 1000 / 4, more small ones
		*/
		width = drand48() * (1000 / 4) + 1;

		/* sprinkle a given size evenly but so they stay in [0,100]
		*/
		r->boundary[i] = drand48() * (1000-width); /* low side */
		r->boundary[i + NUMDIMS] = r->boundary[i] + width; // high side
	}
}


/*-----------------------------------------------------------------------------
| Fill in the boundaries for a random search rectangle.
| Pass in a pointer to a rect that contains all the data,
| and a pointer to the rect to be filled in.
| Generated rect is centered randomly anywhere in the data area,
| and has size from 0 to the size of the data area in each dimension,
| i.e. search rect can stick out beyond data area.
-----------------------------------------------------------------------------*/
void RTreeSearchRect(struct Rect *Search, struct Rect *Data)
{
	register struct Rect *search = Search, *data = Data;
	register int i, j;
	register RectReal size, center;

	assert(search);
	assert(data);

	for (i=0; i<NUMDIMS; i++)
	{
		j = i + NUMDIMS;  // index for high side boundary
		if (data->boundary[i] > -BIG_NUM &&
		    data->boundary[j] < BIG_NUM)
		{
			size = (drand48() * (data->boundary[j] -
					 data->boundary[i] + 1)) / 2;
			center = data->boundary[i] + drand48() *
				(data->boundary[j] - data->boundary[i] + 1);
			search->boundary[i] = center - size/2;
			search->boundary[j] = center + size/2;
		}
		else  // some open boundary, search entire dimension
		{
			search->boundary[i] = -BIG_NUM;
			search->boundary[j] = BIG_NUM;
		}
	}
}

#endif

/*-----------------------------------------------------------------------------
| Print out the data for a rectangle.
-----------------------------------------------------------------------------*/
void RTreePrintRect(struct Rect *R, int depth)
{
	register struct Rect *r = R;
	register int i;
	assert(r);

	RTreeTabIn(depth);
	printf("rect:\n");
	for (i = 0; i < NUMDIMS; i++) {
		RTreeTabIn(depth+1);
		printf("%f\t%f\n", r->boundary[i], r->boundary[i + NUMDIMS]);
	}
}

/*-----------------------------------------------------------------------------
| Calculate the n-dimensional volume of a rectangle
-----------------------------------------------------------------------------*/
RectReal RTreeRectVolume(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	register RectReal volume = (RectReal)1;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;

	for(i=0; i<NUMDIMS; i++)
		volume *= r->boundary[i+NUMDIMS] - r->boundary[i];
	assert(volume >= 0.0);
	return volume;
}

/*-----------------------------------------------------------------------------
| Calculate the n-dimensional volume of the bounding sphere of a rectangle
-----------------------------------------------------------------------------*/
/*
 * The volumes of the unit spheres for each dimension.
 * Generated by sphvol.c
 */
const double UnitSphereVolumes[] = {
	0.000000,  /* dimension   0 */
	2.000000,  /* dimension   1 */
	3.141593,  /* dimension   2 */
	4.188790,  /* dimension   3 */
	4.934802,  /* dimension   4 */
	5.263789,  /* dimension   5 */
	5.167713,  /* dimension   6 */
	4.724766,  /* dimension   7 */
	4.058712,  /* dimension   8 */
	3.298509,  /* dimension   9 */
	2.550164,  /* dimension  10 */
	1.884104,  /* dimension  11 */
	1.335263,  /* dimension  12 */
	0.910629,  /* dimension  13 */
	0.599265,  /* dimension  14 */
	0.381443,  /* dimension  15 */
	0.235331,  /* dimension  16 */
	0.140981,  /* dimension  17 */
	0.082146,  /* dimension  18 */
	0.046622,  /* dimension  19 */
	0.025807,  /* dimension  20 */
	0.013949150409021,            /* dimension  21 */
	0.00737043094571435,          /* dimension  22 */
	0.003810656386852123,         /* dimension  23 */
	0.001929574309403922,         /* dimension  24 */
	0.000957722408823172,         /* dimension  25 */
	0.0004663028057676124,        /* dimension  26 */
	0.0002228721247212739,        /* dimension  27 */
	0.0001046381049248456,        /* dimension  28 */
	0.00004828782273891741,       /* dimension  29 */
	0.0000219153534478302,        /* dimension  30 */
	9.78713994673736e-6,          /* dimension  31 */
	4.303069587032944e-6,         /* dimension  32 */
	1.863467088262139e-6,         /* dimension  33 */
	7.952054001475507e-7,         /* dimension  34 */
	3.34528829410897e-7,          /* dimension  35 */
	1.387895246221376e-7,         /* dimension  36 */
	5.680828718331176e-8,         /* dimension  37 */
	2.294842899726985e-8,         /* dimension  38 */
	9.15223065015956e-9,          /* dimension  39 */
	3.604730797462499e-9,         /* dimension  40 */
	1.4025649060732e-9,           /* dimension  41 */
	5.392664662608125e-10,        /* dimension  42 */
	2.049436095396476e-10,        /* dimension  43 */
	7.700707130601349e-11,        /* dimension  44 */
	2.861552613910809e-11,        /* dimension  45 */
	1.051847171693205e-11,        /* dimension  46 */
	3.82546071052037e-12,         /* dimension  47 */
	1.37686472803774e-12,         /* dimension  48 */
	4.905322148884565e-13,        /* dimension  49 */
	1.730219245836109e-13,        /* dimension  50 */
	6.043342755461587e-14,        /* dimension  51 */
	2.090632335314767e-14,        /* dimension  52 */
	7.164423095729519e-15,        /* dimension  53 */
	2.432561179993387e-15,        /* dimension  54 */
	8.18461780536469e-16,         /* dimension  55 */
	2.729327261598193e-16,        /* dimension  56 */
	9.02201234027155e-17,         /* dimension  57 */
	2.956701542854908e-17,        /* dimension  58 */
	9.6079619284046e-18,          /* dimension  59 */
	3.09625061529686e-18,         /* dimension  60 */
	9.89649265909715e-19,         /* dimension  61 */
	3.137792963448226e-19,        /* dimension  62 */
	9.87007893146823e-20,         /* dimension  63 */
	3.08052103826709e-20,         /* dimension  64 */

};


#define UnitSphereVolume UnitSphereVolumes[NUMDIMS]

#if 0
/*
 * A fast approximation to the volume of the bounding sphere for the
 * given Rect. By Paul B.
 */
RectReal RTreeRectSphericalVolume(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	RectReal maxsize=(RectReal)0, c_size;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;
	for (i=0; i<NUMDIMS; i++) {
		c_size = r->boundary[i+NUMDIMS] - r->boundary[i];
		if (c_size > maxsize)
			maxsize = c_size;
	}
	return (RectReal)(pow(maxsize/2, NUMDIMS) * UnitSphereVolume);
}
#endif

/*
 * The exact volume of the bounding sphere for the given Rect.
 */
RectReal RTreeRectSphericalVolume(struct Rect *R)
{
	register struct Rect *r = R;
	register int i;
	register double sum_of_squares=0, radius;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;
	for (i=0; i<NUMDIMS; i++) {
		double half_extent =
			(r->boundary[i+NUMDIMS] - r->boundary[i]) / 2;
		sum_of_squares += half_extent * half_extent;
	}
	radius = sqrt(sum_of_squares);
	return (RectReal)(pow(radius, NUMDIMS) * UnitSphereVolume);
}


/*-----------------------------------------------------------------------------
| Calculate the n-dimensional surface area of a rectangle
-----------------------------------------------------------------------------*/
RectReal RTreeRectSurfaceArea(struct Rect *R)
{
	register struct Rect *r = R;
	register int i, j;
	register RectReal sum = (RectReal)0;

	assert(r);
	if (Undefined(r))
		return (RectReal)0;

	for (i=0; i<NUMDIMS; i++) {
		RectReal face_area = (RectReal)1;
		for (j=0; j<NUMDIMS; j++)
			/* exclude i extent from product in this dimension */
			if(i != j) {
				RectReal j_extent =
					r->boundary[j+NUMDIMS] - r->boundary[j];
				face_area *= j_extent;
			}
		sum += face_area;
	}
	return 2 * sum;
}



/*-----------------------------------------------------------------------------
| Combine two rectangles, make one that includes both.
-----------------------------------------------------------------------------*/
struct Rect RTreeCombineRect(struct Rect *R, struct Rect *Rr)
{
	register struct Rect *r = R, *rr = Rr;
	register int i, j;
	struct Rect new_rect;
	assert(r && rr);

	if (Undefined(r))
		return *rr;

	if (Undefined(rr))
		return *r;

	for (i = 0; i < NUMDIMS; i++)
	{
		new_rect.boundary[i] = MIN(r->boundary[i], rr->boundary[i]);
		j = i + NUMDIMS;
		new_rect.boundary[j] = MAX(r->boundary[j], rr->boundary[j]);
	}
	return new_rect;
}


/*-----------------------------------------------------------------------------
| Decide whether two rectangles overlap.
-----------------------------------------------------------------------------*/
int RTreeOverlap(struct Rect *R, struct Rect *S)
{
	register struct Rect *r = R, *s = S;
	register int i, j;
	assert(r && s);

	for (i=0; i<NUMDIMS; i++)
	{
		j = i + NUMDIMS;  /* index for high sides */
		if (r->boundary[i] > s->boundary[j] ||
		    s->boundary[i] > r->boundary[j])
		{
			return FALSE;
		}
	}
	return TRUE;
}

/*-----------------------------------------------------------------------------
| Decide a rectange and a sphere overlap. Written by Chen Li
-----------------------------------------------------------------------------*/
bool RTreeOverlapSphere(struct Rect *R, struct Point *point, float radius)
{
  register struct Rect *r = R;
  register int i, j;
  assert(r && point);
  
  // compute the mindist
  float squareSum = 0;
  for (register int i = 0; i < NUMDIMS; i ++)
    {
      int low = i, high = i + NUMDIMS;
      float diff = 0;

      if (point->position[i] < r->boundary[low])
	diff = r->boundary[low] - point->position[low];
      else if (point->position[i] > r->boundary[high])
	diff =  r->boundary[high] - point->position[i];
      else
	diff = 0;
      
      squareSum += diff * diff;
    }
  
  float mindist = sqrt(squareSum);
  return (mindist <= radius);
}

static float ComputeDistance(float *dist_array, int dist_func)
{
  float distance;
  float running=0.0;
  register int i;

  switch(dist_func)
    {
    case(L1):
      for(i=0; i<NUMDIMS; i++)
	running+=dist_array[i];
      return running;
      break;

    case (L2):
      for(i=0; i<NUMDIMS; i++)
	running+=(dist_array[i]*dist_array[i]);
      distance=sqrt(running);
      return distance;
      break;

    case (LMAX):
      for(i=0; i<NUMDIMS; i++)
	if (running < dist_array[i])
	  running=dist_array[i];
      return running;
    }
}


float Distance(struct Point *P1, struct Point *P2, int dist_func)
{
  register struct Point *p1 = P1;
  register struct Point *p2 = P2;
  register float diff;
  register float distance;
  register float running=0.0;
  register int i;

  switch(dist_func)
    {
    case(L1):
      for(i=0; i<NUMDIMS; i++)
        running+=fabs(p1->position[i]-p2->position[i]);
      return running;
      break;

    case (L2):
      for(i=0; i<NUMDIMS; i++)
        {
          diff=fabs(p1->position[i]-p2->position[i]);
          running+=diff*diff;
        }
      distance=sqrt(running);
      return distance;
      break;

    case (LMAX):
      for(i=0; i<NUMDIMS; i++)
        {
          diff=fabs(p1->position[i]-p2->position[i]);
          if (running < diff) running=diff;
        }
      return running;
    }
}


float Distance(struct Point *P, struct Rect *R, int dist_func)
{
  register struct Point *p = P;
  register struct Rect *r = R;
  register float distance;
  register float diff;
  register float running=0.0;
  register int i;

  switch(dist_func)
    {
    case(L1):
      for(i=0; i<NUMDIMS; i++)
        {
          if (p->position[i] < r->boundary[i])
            running+=(r->boundary[i]-p->position[i]);
          else if (p->position[i] > r->boundary[i+NUMDIMS])
            running+=(p->position[i]-r->boundary[i+NUMDIMS]);
        }
      return running;
      break;

    case (L2):
      for(i=0; i<NUMDIMS; i++)
        {
          if (p->position[i] < r->boundary[i])
            {
              diff=(r->boundary[i]-p->position[i]);
              running+=diff*diff;
            }
          else if (p->position[i] > r->boundary[i+NUMDIMS])
            {
              diff=(p->position[i]-r->boundary[i+NUMDIMS]);
              running+=diff*diff;
            }
        }
      distance=sqrt(running);
      return distance;
      break;

    case (LMAX):
      for(i=0; i<NUMDIMS; i++)
        {
          if (p->position[i] < r->boundary[i])
            {
              diff=(r->boundary[i]-p->position[i]);
              if (running < diff) running=diff;
            }
          else if (p->position[i] > r->boundary[i+NUMDIMS])
            {
              diff=(p->position[i]-r->boundary[i+NUMDIMS]);
              if (running<diff) running=diff;
            }
        }
      return running;
    }
}







int RTreeOverlap(struct Point *P, struct Rect *R, float max_distance, int dist_func)
{
	register struct Point *p = P;
	register struct Rect *r = R;
	float dist_array[NUMDIMS];
	float distance;
	register int i/*, j*/;
	assert(p && r);

	for (i=0; i<NUMDIMS; i++)
	{
	  if (p->position[i] < r->boundary[i])
	    dist_array[i]=r->boundary[i]-p->position[i];
	  else if (p->position[i] > r->boundary[i+NUMDIMS])
	    dist_array[i]=p->position[i]-r->boundary[i+NUMDIMS];
	  else /* r->boundary[i] <= p->position[i] <= r->boundary[i+NUMDIMS]*/
	    dist_array[i]=0;
	}

	distance=ComputeDistance(dist_array, dist_func);
	if (distance > max_distance) return FALSE;
	else return TRUE;
}






/*-----------------------------------------------------------------------------
| Decide whether rectangle r is contained in rectangle s.
-----------------------------------------------------------------------------*/
int RTreeContained(struct Rect *R, struct Rect *S)
{
	register struct Rect *r = R, *s = S;
	register int i, j, result;
	assert((int)r && (int)s);

 	// undefined rect is contained in any other
	//
	if (Undefined(r))
		return TRUE;

	// no rect (except an undefined one) is contained in an undef rect
	//
	if (Undefined(s))
		return FALSE;

	result = TRUE;
	for (i = 0; i < NUMDIMS; i++)
	{
		j = i + NUMDIMS;  /* index for high sides */
		result = result
			&& r->boundary[i] >= s->boundary[i]
			&& r->boundary[j] <= s->boundary[j];
	}
	return result;
}





static int isPoint(struct Rect* rect)
{
	struct Point c = Center (rect);
	for (int i = 0; i<NUMDIMS; i++)
	{
		if(rect->boundary[i]!=c.position[i]) return 0;
	}
	return 1;
}





double KNNDis(struct Point *point, struct Rect *rect,int dist_func)
{

	if(isPoint(rect))
	{
		Point p = Center(rect);
		return Distance(point,&p, dist_func);
	}
	else return Distance (point, rect, dist_func);
}

