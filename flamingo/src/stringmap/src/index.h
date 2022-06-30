//
// $Id: index.h 5770 2010-10-19 06:38:25Z abehm $
//
// index.h
//
//  Copyright (C) 2003 - 2007 by The Regents of the University of
//  California
//
// Redistribution of this file is permitted under the terms of the 
// BSD license
//
// Date: March 2002
//
// Authors: Michael Ortega-Binderberger (miki (at) ics.uci.edu)
//          Liang Jin (liangj (at) ics.uci.edu)
//          Chen Li (chenli (at) ics.uci.edu)
//

#ifndef _INDEX_
#define _INDEX_

#include <map>

#include "rtreeparams.h" // get the number of dimensions
#include "element.h"

using namespace std;

#define NDEBUG

#define RANGEFACTOR 0.5
#define DIST_METRIC LMAX

/* distance functions */
#define L1 1
#define MANHATTAN 1
#define L2 2
#define EUCLIDEAN 2
#define L3 3
#define LMAX 100

/*-----------------------------------------------------------------------------
| Global definitions.
-----------------------------------------------------------------------------*/

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAXINSERTS 100000
#define MAXSEARCH 10000



struct Node;

struct Branch {
        struct Rect rect;
        struct Node *child;
        float radius;
};

#define PGSIZE (8096)

/* branching factor of a node */
#define NODECARD (int)((PGSIZE-(2*sizeof(int))) / sizeof(struct Branch))

/* balance criterion for node splitting */
/* NOTE: can be changed if needed. */
#define MinFill (NODECARD / 2)


struct Node {
        int count;
        int level; /* 0 is leaf, others positive */
        struct Branch branch[NODECARD];
};

struct ListNode {
        struct ListNode *next;
        struct Node *node;
};


struct NNElement {
  struct Node * the_node;
  struct Rect   the_mbr;
  int           the_id; // the actual data point id.
  bool          is_data; // true == its a true (point,leaf) data item, false== its an mbr
  float         dist; // its distance from query. -1== mbr, >=0 == its a point(leaf)
  bool          target_node_explored; // true if Ialso looked at the_node and put it into queue
  NNElement() : the_node(NULL), dist(0.0), is_data(false),
                the_id(-1), target_node_explored(false)  {};
  int operator()(const struct NNElement & first,
		 const struct NNElement & second)
  {
    //return first.dist > second.dist;
    return false; // everybody has the same priority
  }
};
typedef priority_queue<NNElement, deque<NNElement>, NNElement> NNPQueue;


struct JoinElement {
  struct JoinTerm {
    struct Node * the_node;
    struct Rect   the_mbr;
    int           the_id; // the actual data point id.
    bool          is_data; // true == its a true (point,leaf) data item, false== its an mbr
    bool          target_node_explored; // true if Ialso looked at the_node and put it into queue
    JoinTerm() : the_node(NULL), is_data(false), the_id(-1), target_node_explored(false)  {};
  } first, second; // first is for "me", second is for the other index
  float           dist; // its distance from query. -1== mbr, >=0 == its a point(leaf)
  JoinElement() : dist(0.0) {};
  int operator()(const struct JoinElement & first,
		 const struct JoinElement & second)
  {
    // later also account for ties and level of node, volume and so on.
    //return first.dist > second.dist;
    return false; // everybody has the same priority
  }
};
typedef priority_queue<JoinElement, deque<JoinElement>, JoinElement> JoinPQueue;






enum { NN_MODEL_RECONSTRUCT_NAIVE,   NN_MODEL_RECONSTRUCT_FULL} ;
enum { JOIN_MODEL_RECONSTRUCT_NAIVE, JOIN_MODEL_RECONSTRUCT_FULL} ;

class Rtree {
private:
        struct Node* TreeRoot;

        int Load(struct Node*);
        int Dump(struct Node*);
        int RTreeSearch(struct Node*, struct Rect*, long*, vector<int> &answers);
        //int RTreeSearch(struct Node*, struct Rect*, long*, AnswerList * answers);
        int RTreeInsertRect(struct Rect*, int, struct Node**, int depth);
        void CreateSpheres(struct Node *);
        bool GenerateMBRfromNode(struct Node * node, struct Rect * rect);

	// new
	//int RTreeSearch(struct Node*, struct Rect*, long*, vector<int> &answers);

public:
        Rtree();
        int RTreeLoad(char*);
        int RTreeDump(char*);
        //int RTreeSearch(struct Rect*, long*);
        //int RTreeSearch(struct Rect*, long*, AnswerList * answers);
        int RTreeInsertRect(struct Rect*, int);
        int RTreeDeleteRect(struct Rect*, int, struct Node**);

	// new
	int RTreeSearch(struct Rect*, vector<int> &answers);
	int RTreeSearchSphere(struct Point *queryPoint, float radius, vector<int> &answers);
	int RTreeSearchSphere(struct Node *N, struct Point *queryPoint, float radius,
			      long *diskacc, vector<int> &answers);

        ~Rtree();

        // misc
        void print_rect(ostream & os, struct Rect & rect, bool add_endl=true);

        // Join Search and support data
        int  SearchJoinOpen(Rtree * rt, DistanceFunction * df,
                            float * max_dist,
                            long * disk_accesses1, long disk_accesses_length1,
                            long * disk_accesses2, long disk_accesses_length2,
                            long * cpu_computations, long cpu_computations_length,
                            long * cpu_comps_total);
        int  SearchJoinNext(float & distance, join_pair_result & data_item);
        int  SearchJoinClose();
        int  SearchJoinNewIteration(DistanceFunction * df_new, int & cpu_distance_comps,
                                    int & nodes_copied, int reconstruct_model = JOIN_MODEL_RECONSTRUCT_NAIVE);
        Rtree             * join_other_tree;
        DistanceFunction  * join_dist_func;
        long              * join_disk_acc1, join_disk_acc_length1; // array and length
        long              * join_disk_acc2, join_disk_acc_length2; // array and length
        long              * join_cpu_comp,  join_cpu_comp_length; // array and length
        long              * join_cpu_comp_total;
        bool                join_active_query; // true if between open and close
        JoinPQueue        * join_pqueue;// * join_q_already_returned;
        float             * join_max_distance;// max distance of the join element

void join_aux_point_point(JoinElement & j_el, join_pair_result & item, int exploration_model);
void join_aux_node_node(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second, JoinElement & j_el, bool swap, int exploration_model);
void join_aux_node_leaf(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second, JoinElement & j_el, bool swap, int exploration_model);
void join_aux_leaf_leaf(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second, JoinElement & j_el, bool swap, int exploration_model);
void join_aux_leaf_point(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second, JoinElement & j_el, bool swap, int exploration_model);
void join_aux_node_point(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second, JoinElement & j_el, bool swap, int exploration_model);
};



extern struct Node * RTreeNewNode();
extern void RTreeInitNode(struct Node*);
extern void RTreeFreeNode(struct Node *);
extern void RTreePrintNode(struct Node *, int);
extern void RTreeTabIn(int);
extern struct Rect RTreeNodeCover(struct Node *);


extern void PointToRectangle(struct Point *P, struct Rect *R);
extern void RectangleToPoint(struct Rect *R, struct Point *P);



float ComputeRadius(struct Point *centroid, struct Node *N);


struct Point Center(struct Rect *);
float Distance(struct Point *P1, struct Point *P2, int dist_func);
float Distance(struct Point *P, struct Rect *R, int dist_func);
double KNNDis(struct Point *point, struct Rect *rect, int dist_func);

extern void RTreeInitRect(struct Rect*);
extern struct Rect RTreeNullRect();
extern RectReal RTreeRectArea(struct Rect*);
extern RectReal RTreeRectSphericalVolume(struct Rect *R);
extern RectReal RTreeRectVolume(struct Rect *R);
extern struct Rect RTreeCombineRect(struct Rect*, struct Rect*);
extern int RTreeOverlap(struct Rect*, struct Rect*);
extern int RTreeOverlap(struct Point *, struct Rect *, float, int);
extern void RTreePrintRect(struct Rect*, int);
extern int RTreeAddBranch(struct Branch *, struct Node *, struct Node **);
extern int RTreePickBranch(struct Rect *, struct Node *);
extern void RTreeDisconnectBranch(struct Node *, int);
extern void RTreeSplitNode(struct Node*, struct Branch*, struct Node**);

extern bool RTreeOverlapSphere(struct Rect *R, struct Point *queryPoint, float radius);

#endif /* _INDEX_ */

