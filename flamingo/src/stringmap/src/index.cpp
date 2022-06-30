//
// $Id: index.cpp 5770 2010-10-19 06:38:25Z abehm $
//
// index.cpp
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
#include <malloc.h>
#include <math.h>
#include <assert.h>
#include <iostream>
#include "index.h"


// global variable, root
#define DEFINED(val) (val >= 0.0 && val <= 1.0 ? TRUE: FALSE)

//struct Node *RTreeRoot;
FILE *fp;
int spheres_exist=0;

// Make a new index, empty.  Consists of a single node.


Rtree::Rtree() {
        struct Node *x;
        x = RTreeNewNode();
    TreeRoot = x;
        x->level = 0; /* leaf */

  join_other_tree         = NULL;
  join_dist_func          = NULL;
  join_disk_acc1          = NULL;
  join_disk_acc_length1   = 0;
  join_disk_acc2          = NULL;
  join_disk_acc_length2   = 0;
  join_cpu_comp           = NULL;
  join_cpu_comp_length    = 0;
  join_cpu_comp_total     = NULL;
  join_active_query       = false;
  join_pqueue             = NULL;
  join_max_distance       = NULL;
}


Rtree::~Rtree() {
}


int   Rtree::RTreeLoad(char *dumpfile) {
        int loadcount;
        fp= fopen(dumpfile, "r");
        loadcount=Load(TreeRoot);
        //cout << loadcount << " nodes loaded" << endl;
        fclose(fp);
        //CreateSpheres(TreeRoot); // this will fill in all the radii
        spheres_exist = 1;
        return loadcount;
}


int Rtree::Load(struct Node *N) {
        //struct Node *newNode;
        struct Node *n= N;
        int nodeCount = 0;
        int i;

        fread(&(n->count), sizeof(int), 1, fp);
        fread(&(n->level), sizeof(int), 1, fp);
        if (n->level > 0) /* this is an internal node in the tree */ {
                for(i=0; i<n->count; i++)
                    fread(&(n->branch[i].rect), sizeof(struct Rect), 1, fp);

                for(i=0; i< n->count; i++) {
                     n->branch[i].child = RTreeNewNode();
                     nodeCount = nodeCount + Load(n->branch[i].child) + 1;
                   }
                //cout << "Inner node level: " << n->level << endl;
                return nodeCount;
        } else /* this is a leaf node */ {
                for(i=0; i<n->count; i++)
                    fread(&(n->branch[i].rect), sizeof(struct Rect), 1, fp);
                for(i=0; i<n->count; i++)
                    fread(&(n->branch[i].child), sizeof(struct Node*), 1, fp);
                //cout << "Leaf level: " << n->level << endl;
                return 0;
        }
}

bool Rtree::GenerateMBRfromNode(struct Node * node, struct Rect * the_mbr_rect) {
  // true if all ok, false otherwise (like if the node has
  // no boxes.
  assert(NULL != node); assert(NULL != the_mbr_rect);
  bool res = false;
  float min_dim, max_dim;
  for (int dims=0; dims < NUMDIMS; dims++) { // for each dimension
    // get the max and min values
    min_dim = node->branch[0].rect.boundary[dims];
    max_dim = node->branch[0].rect.boundary[dims+NUMDIMS];
    for (int i=0; i<node->count; i++) { // go over all branches
      if (node->branch[i].rect.boundary[dims] < min_dim) {
        min_dim = node->branch[i].rect.boundary[dims];
      }
      if (node->branch[i].rect.boundary[dims+NUMDIMS] > max_dim) {
        max_dim = node->branch[i].rect.boundary[dims+NUMDIMS];
      }
      res = true;
    }
    the_mbr_rect->boundary[dims]         = min_dim;
    the_mbr_rect->boundary[dims+NUMDIMS] = max_dim;
  }
  return res;
}

void Rtree::print_rect(ostream & os, struct Rect & rect, bool add_endl) {
  os << " [";
  int i;
  for (i=0; i<NUMDIMS; i++) os << rect.boundary[i] << ", ";
  os << "]-[";
  for (i=NUMDIMS; i<NUMDIMS*2; i++) os << rect.boundary[i] << ", ";
  os << "]";
  if (add_endl) os << "\n";
}


void Rtree::CreateSpheres(struct Node *n) { // this function will create all the spheres
  int i;
  struct Point centroid;
  if (n->level > 1) { // high level nodes
    for(i=0; i<n->count; i++)
      CreateSpheres(n->branch[i].child); // invoke create spheres on all children
  } else {
    assert(n->level == 1);
    for(i=0; i<n->count; i++) {
      centroid=Center(&n->branch[i].rect);
      n->branch[i].radius=ComputeRadius(&centroid, n->branch[i].child);
    }
  }
}


int Rtree::RTreeDump(char *dumpfile) {
  int dumpcount;
  fp= fopen(dumpfile, "w");
  dumpcount=Dump(TreeRoot);
  fclose(fp);
  return dumpcount;
}

int Rtree::Dump(struct Node *N) {
        struct Node *n= N;
        int nodeCount = 0;
        int i/*,j*/;

        if (n->level > 0) { /* this is an internal node in the tree */
                fwrite(&(n->count), sizeof(int), 1, fp);
                fwrite(&(n->level), sizeof(int), 1, fp);
                for(i=0; i< n->count; i++) {
                  fwrite(&(n->branch[i].rect), sizeof(struct Rect), 1, fp);
                }
                for(i=0; i< n->count; i++)
                  nodeCount = nodeCount + Dump(n->branch[i].child) + 1;

                RTreeFreeNode(n);
                return nodeCount;
        } else /* this is a leaf node */ {
                fwrite(&(n->count), sizeof(int), 1, fp);
                fwrite(&(n->level), sizeof(int), 1, fp);
                for(i=0; i<n->count; i++) {
                  fwrite(&(n->branch[i].rect), sizeof(struct Rect), 1, fp);
                }
                for(i=0; i<n->count; i++)
                  fwrite(&(n->branch[i].child), sizeof(struct Node*), 1, fp);
                RTreeFreeNode(n);
                return 0;
        }
}



// Inserts a new data rectangle into the index structure.
// Recursively descends tree, propagates splits back up.
// Returns 0 if node was not split.  Old node updated.
// If node was split, returns 1 and sets the pointer pointed to by
// new_node to point to the new node.  Old node updated to become one of two.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
//
static int RTreeInsertRect2(struct Rect *r, int tid, struct Node *n, struct Node **new_node, int level) {
/*
        register struct Rect *r = R;
        register int tid = Tid;
        register struct Node *n = N, **new_node = New_node;
        register int level = Level;
*/

  register int i;
  struct Branch b;
  struct Node *n2;

  assert(r && n && new_node);
  assert(level >= 0 && level <= n->level);

  // Still above level for insertion, go down tree recursively
  if (n->level > level) {
    i = RTreePickBranch(r, n);
    if (!RTreeInsertRect2(r, tid, n->branch[i].child, &n2, level)) {
      // child was not split
      n->branch[i].rect = RTreeCombineRect(r,&(n->branch[i].rect));
      return 0;
    } else {   // child was split
      n->branch[i].rect = RTreeNodeCover(n->branch[i].child);
      b.child = n2;
      b.rect = RTreeNodeCover(n2);
      return RTreeAddBranch(&b, n, new_node);
    }
  } else if (n->level == level) {
    /* Have reached level for insertion. Add rect, split if necessary */
    b.rect = *r;
    b.child = (struct Node *) tid;
    /* child field of leaves contains tid of data record */
    return RTreeAddBranch(&b, n, new_node);
  } else { /* Not supposed to happen */
    assert (FALSE);
    return 0;
  }
}

int Rtree::RTreeInsertRect(struct Rect *R, int Tid) {
       struct Node *rootNode;
       rootNode = TreeRoot;
       return (RTreeInsertRect(R, Tid, &rootNode, 0));
}


// Insert a data rectangle into an index structure.
// RTreeInsertRect provides for splitting the root;
// returns 1 if root was split, 0 if it was not.
// The level argument specifies the number of steps up from the leaf
// level to insert; e.g. a data rectangle goes in at level = 0.
// RTreeInsertRect2 does the recursion.
//
int Rtree::RTreeInsertRect(struct Rect *R, int Tid, struct Node **Root, int Level) {
        struct Rect *r = R;
        int tid = Tid;
        struct Node **root = Root;
        int level = Level;
        int i;
        struct Node *newroot;
        struct Node *newnode;
        struct Branch b;
        int result;

		if (level < 0)
			int yy = 1;
        assert(r && root);
        assert(level >= 0 && level <= (*root)->level);
        for (i=0; i<NUMDIMS; i++) {
              if DEFINED(r->boundary[i])
                assert(r->boundary[i] <= r->boundary[NUMDIMS+i]);
        }

        if (RTreeInsertRect2(r, tid, *root, &newnode, level)) { /* root split */
                newroot = RTreeNewNode();  /* grow a new root, & tree taller */
                TreeRoot = newroot;
                newroot->level = (*root)->level + 1;
                b.rect = RTreeNodeCover(*root);
                b.child = *root;
                RTreeAddBranch(&b, newroot, NULL);
                b.rect = RTreeNodeCover(newnode);
                b.child = newnode;
                RTreeAddBranch(&b, newroot, NULL);
                *root = newroot;
                result = 1;
        } else
           result = 0;
        return result;
}




// Allocate space for a node in the list used in DeletRect to
// store Nodes that are too empty.
//
static struct ListNode * RTreeNewListNode() {
        return (struct ListNode *) malloc(sizeof(struct ListNode));
        //return new ListNode;
}


static void RTreeFreeListNode(struct ListNode *p) {
        //free(p);
        delete(p);
}



// Add a node to the reinsertion list.  All its branches will later
// be reinserted into the index structure.
//
static void RTreeReInsert(struct Node *n, struct ListNode **ee) {
        register struct ListNode *l;

        l = RTreeNewListNode();
        l->node = n;
        l->next = *ee;
        *ee = l;
}


// Delete a rectangle from non-root part of an index structure.
// Called by RTreeDeleteRect.  Descends tree recursively,
// merges branches on the way back up.
//
static int
RTreeDeleteRect2(struct Rect *R, int Tid, struct Node *N, struct ListNode **Ee) {
        register struct Rect *r = R;
        register int tid = Tid;
        register struct Node *n = N;
        register struct ListNode **ee = Ee;
        register int i;

        assert(r && n && ee);
        assert(tid >= 0);
        assert(n->level >= 0);

        if (n->level > 0) { // not a leaf node
            for (i = 0; i < NODECARD; i++) {
                if (n->branch[i].child && RTreeOverlap(r, &(n->branch[i].rect))) {
                        if (!RTreeDeleteRect2(r, tid, n->branch[i].child, ee)) {
                                if (n->branch[i].child->count >= MinFill)
                                        n->branch[i].rect = RTreeNodeCover(
                                                n->branch[i].child);
                                else {
                                        // not enough entries in child, eliminate child node
                                        RTreeReInsert(n->branch[i].child, ee);
                                        RTreeDisconnectBranch(n, i);
                                }
                                return 0;
                        }
                }
            }
            return 1;
        } else { // a leaf node
                for (i = 0; i < NODECARD; i++) {
                        if (n->branch[i].child &&
                            n->branch[i].child == (struct Node *) tid) {
                                RTreeDisconnectBranch(n, i);
                                return 0;
                        }
                }
                return 1;
        }
}



// Delete a data rectangle from an index structure.
// Pass in a pointer to a Rect, the tid of the record, ptr to ptr to root node.
// Returns 1 if record not found, 0 if success.
// RTreeDeleteRect provides for eliminating the root.
//
int Rtree::RTreeDeleteRect(struct Rect *R, int Tid, struct Node**Nn) {
        register struct Rect *r = R;
        register int tid = Tid;
        register struct Node **nn = Nn;
        register int i;
        register struct Node *tmp_node_ptr;
        struct ListNode *reInsertList = NULL;
        register struct ListNode *e;

        assert(r && nn);
        assert(*nn);
        assert(tid >= 0);

        if (!RTreeDeleteRect2(r, tid, *nn, &reInsertList)) {
                // found and deleted a data item

                // reinsert any branches from eliminated nodes
                while (reInsertList) {
                        tmp_node_ptr = reInsertList->node;
                        for (i = 0; i < NODECARD; i++) {
                                if (tmp_node_ptr->branch[i].child) {
                                        RTreeInsertRect(
                                                &(tmp_node_ptr->branch[i].rect),
                                                (int) tmp_node_ptr->branch[i].child,
                                                nn,
                                                tmp_node_ptr->level);
                                }
                        }
                        e = reInsertList;
                        reInsertList = reInsertList->next;
                        RTreeFreeNode(e->node);
                        RTreeFreeListNode(e);
                }

                // check for redundant root (not leaf, 1 child) and eliminate

                if ((*nn)->count == 1 && (*nn)->level > 0) {
                        for (i = 0; i < NODECARD; i++) {
                                tmp_node_ptr = (*nn)->branch[i].child;
                                if(tmp_node_ptr)
                                        break;
                        }
                        assert(tmp_node_ptr);
                        RTreeFreeNode(*nn);
                        *nn = tmp_node_ptr;
                }
                return 0;
        } else {
                return 1;
        }
}



// ------------------------------------------------------------
// ------------- Searching functions --------------------------

// generic search, counts accesses only
//int Rtree::RTreeSearch(struct Rect *R, long *diskacc, vector<int> &answers) {
int Rtree::RTreeSearch(struct Rect *R, vector<int> &answers)
{
  long  disk_acc[10];

  struct Node *rootNode;
  rootNode = TreeRoot;

  return (RTreeSearch(rootNode, R, disk_acc, answers));
}


// Search in an index tree or subtree for all data retangles that
// overlap the argument rectangle.
// Returns the number of qualifying data rects.
int Rtree::RTreeSearch(struct Node *N, struct Rect *R,
		       long *diskacc, vector<int> &answers)
{
  struct Node *n = N;
  struct Rect *r = R;
  int hitCount = 0, i;
  struct Point querypoint/*, centroid*/;
  float maxradius=0.0;

  assert(n);
  assert(n->level >= 0);
  assert(r);

  maxradius=(r->boundary[NUMDIMS]-r->boundary[0])*RANGEFACTOR;
  for(i=0; i<NUMDIMS; i++)
    querypoint.position[i]=(r->boundary[i]+ r->boundary[i+NUMDIMS])/2;

  if (n->level > 0) { /* this is an internal node in the tree */
    for (i=0; i<NODECARD; i++) {
      if (spheres_exist && n->level == 1) { // use both conditions
        if (n->branch[i].child && RTreeOverlap(&querypoint,&n->branch[i].rect, maxradius, DIST_METRIC)) {
          /* centroid=Center(&n->branch[i].rect);
             if (Distance(&querypoint, &centroid, DIST_METRIC) <= (n->branch[i].radius + maxradius)) */
                 //printf("Accessing a node at level %d \n", n->level);
          diskacc[n->level]++;
          hitCount += RTreeSearch(n->branch[i].child, R, diskacc, answers);
        }
      } else {
        if (n->branch[i].child && RTreeOverlap(&querypoint,&n->branch[i].rect, maxradius, DIST_METRIC)) {
          //printf("Accessing a node at level %d \n", n->level);
          diskacc[n->level]++;
          hitCount += RTreeSearch(n->branch[i].child, R, diskacc, answers);
        }
      }
    }
  } else { /* this is a leaf node */
    for (i=0; i<NODECARD; i++) {
    /* if (n->branch[i].child && RTreeOverlap(r,&n->branch[i].rect)) */
      if (n->branch[i].child && RTreeOverlap(&querypoint,&n->branch[i].rect, maxradius, DIST_METRIC)) {
        // printf("Accessing a node at level %d \n", n->level);
        diskacc[n->level]++;
        hitCount++;

	answers.push_back((int)n->branch[i].child);  // Added by Chen Li
      }
    }
  }
  return hitCount;
}

//--------------------------------------------------------------------------------------
//--------------------------------- join search ----------------------------------------

int Rtree::SearchJoinOpen(Rtree * rt, DistanceFunction * df,
                          float * max_join_dist,
                          long *diskacc1, long disk_acc_length1, // disk for tree 1 (me)
                          long *diskacc2, long disk_acc_length2, // disk for tree 2 (other)
                          long *cpu_computations, long cpu_computations_length, // per level
                          long *cpu_comps_total) { // total distance computations
  join_other_tree = rt; // I'm me (1), this is the parner tree for join(2)
  join_dist_func  = df; // retain distance function
  join_max_distance = max_join_dist;


  // init variables.
  join_disk_acc_length1 = disk_acc_length1; // preserve for later copy
  join_disk_acc1        = diskacc1;
  join_disk_acc_length2 = disk_acc_length2; // preserve for later copy
  join_disk_acc2        = diskacc2;
  join_cpu_comp        = cpu_computations; // preserve for later copy
  join_cpu_comp_length = cpu_computations_length;
  join_cpu_comp_total  = cpu_comps_total;
  int i;
  for (i=0; i<join_disk_acc_length1; i++) { join_disk_acc1[i] = 0; }
  for (i=0; i<join_disk_acc_length2; i++) { join_disk_acc2[i] = 0; }
  for (i=0; i<join_cpu_comp_length ; i++) { join_cpu_comp [i] = 0; }
  *join_cpu_comp_total = 0;

  struct Rect r1, r2; // root mbr's
  GenerateMBRfromNode(this           ->TreeRoot, &r1); // bounding box for myself
  GenerateMBRfromNode(join_other_tree->TreeRoot, &r2); // bounding box for other index
  //print_rect(cout, r1); print_rect(cout, r2); // gonna print out the bboxes.

  // bootstrap priority queue with first pair of roots.
  JoinElement j_el;
  j_el.first.the_node              = TreeRoot;
  j_el.first.the_mbr               = r1;
  j_el.first.the_id                = -1;
  j_el.first.is_data               = false; // is not a data point
  j_el.first.target_node_explored  = false; // have not looked at children

  j_el.second.the_node             = join_other_tree->TreeRoot;
  j_el.second.the_mbr              = r2;
  j_el.second.the_id               = -1;
  j_el.second.is_data              = false; // is not a data point
  j_el.second.target_node_explored = false; // have not looked at children

  j_el.dist = 0.0; // two roots forced to have 0 distance at first?

  // get the queues.
  if (NULL != join_pqueue)             delete join_pqueue;   // if old, nuke it.
  //if (NULL != join_q_already_returned) delete join_q_already_returned;   // if old, nuke it.
  join_pqueue             = new JoinPQueue; // get a new queue;
  //join_q_already_returned = new JoinPQueue; // get a new queue;
  // count up.
  join_disk_acc1[j_el.first .the_node->level] ++; // I/O to
  join_disk_acc2[j_el.second.the_node->level] ++; // the roots
  (*join_cpu_comp_total) ++;   // 'computed the distance between roots'

  join_pqueue->push(j_el);     // bootstrap;

  join_active_query = true;    // ok, we're all set, so mark it

  return 0;
}



// exploration model can be SYMMETRIC (explore both sides as n^2
// A_SYMMETRIC



void Rtree::join_aux_point_point(JoinElement & j_el, join_pair_result & item,
                          int exploration_model) {
  // handle the final point to point
  // regardless of exploration model, its only 1 to 1
  assert(j_el.first.is_data && j_el.second.is_data); // they are points
  assert(NULL==j_el.first.the_node && NULL==j_el.second.the_node); // ensure correctness
  assert(j_el.first .target_node_explored);
  assert(j_el.second.target_node_explored);
  item.distance      = j_el.dist;
  item.first.the_id  = j_el.first.the_id;
  item.first.rect    = j_el.first.the_mbr;
  item.second.the_id = j_el.second.the_id;
  item.second.rect   = j_el.second.the_mbr;
}


void Rtree::join_aux_node_node(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second,
                               JoinElement & j_el, bool swap, int exploration_model) {
  // this compares internal nodes in the tree
  assert( ! first->is_data); assert( ! second->is_data); // none is a point
  assert(first ->the_node != NULL && second->the_node != NULL); // valid
  assert(first ->the_node->level > 0); // first is an internal node
  assert(second->the_node->level > 0); // second is also a node
  JoinElement j_el_aux;
  for (int i=0; i<first->the_node->count; i++) {
    for (int j=0; j<second->the_node->count; j++) {
      j_el_aux.first.the_node              = first->the_node->branch[i].child;
      j_el_aux.first.the_mbr               = first->the_node->branch[i].rect;
      j_el_aux.first.the_id                = -1;
      j_el_aux.first.is_data               = false; // its another node
      j_el_aux.first.target_node_explored  = false; //

      j_el_aux.second.the_node             = second->the_node->branch[j].child;
      j_el_aux.second.the_mbr              = second->the_node->branch[j].rect;
      j_el_aux.second.the_id               = -1;
      j_el_aux.second.is_data              = false; // a node
      j_el_aux.second.target_node_explored = false; //

      // note the order, the second one is assumed to be the mbr and first is query point
      j_el_aux.dist = join_dist_func->rectangle_mindist(& j_el_aux.first.the_mbr, & j_el_aux.second.the_mbr);
      if ((NULL != join_max_distance && j_el_aux.dist < *join_max_distance) ||
          (NULL == join_max_distance)) join_pqueue->push(j_el_aux);

      (*join_cpu_comp_total) ++; // accounting |n1| * |n2| computations
    }
  }

  // misc accounting crap (only 1 IO per node!)
  join_disk_acc1[first ->the_node->level]++;
  join_disk_acc2[second->the_node->level]++;

  // remember what we got out and mark it as explored
  j_el.first.target_node_explored = j_el.second.target_node_explored = true;
}


void Rtree::join_aux_node_leaf(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second,
                               JoinElement & j_el, bool swap, int exploration_model) {
  // this compares internal node in the tree to a leaf
  assert( ! first->is_data); assert( ! second->is_data); // none is a point
  assert(first ->the_node != NULL && second->the_node != NULL); // valid
  assert(first ->the_node->level  > 0); // first is  an internal node
  assert(second->the_node->level == 0); // second one is a leaf
  JoinElement j_el_aux;
  for (int i=0; i<first->the_node->count; i++) {
    for (int j=0; j<second->the_node->count; j++) {
      j_el_aux.first.the_node              = first->the_node->branch[i].child;
      j_el_aux.first.the_mbr               = first->the_node->branch[i].rect;
      j_el_aux.first.the_id                = -1;
      j_el_aux.first.is_data               = false;
      j_el_aux.first.target_node_explored  = false;

      j_el_aux.second.the_node             = NULL;
      j_el_aux.second.the_mbr              = second->the_node->branch[j].rect;
      j_el_aux.second.the_id               = (int)second->the_node->branch[j].child;
      j_el_aux.second.is_data              = true;  // its a data item (P)
      j_el_aux.second.target_node_explored = true;  // no more to explore

      // note the order, the second one is assumed to be the mbr and first is a point
      j_el_aux.dist = join_dist_func->compute_mindist(& j_el_aux.second.the_mbr, & j_el_aux.first.the_mbr);
      if ((NULL != join_max_distance && j_el_aux.dist < *join_max_distance) ||
          (NULL == join_max_distance)) join_pqueue->push(j_el_aux);

      (*join_cpu_comp_total) ++; // accounting |n1| * |n2| computations
    }
  }
  // misc accounting crap (only 1 IO per node!)
  join_disk_acc1[first ->the_node->level]++;
  join_disk_acc2[second->the_node->level]++;

  // remember what we got out and mark it as explored
  j_el.first.target_node_explored = j_el.second.target_node_explored = true;
}


void Rtree::join_aux_leaf_leaf(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second,
                               JoinElement & j_el, bool swap, int exploration_model) {
  // compares two leaves
  assert( ! first->is_data); assert( ! second->is_data); // none is a point
  assert(first ->the_node != NULL && second->the_node != NULL); // valid
  assert(first ->the_node->level == 0); // first  one is a leaf
  assert(second->the_node->level == 0); // second one is a leaf
  JoinElement j_el_aux;
  for (int i=0; i<first->the_node->count; i++) {
    for (int j=0; j<second->the_node->count; j++) {
      j_el_aux.first.the_node              = NULL;
      j_el_aux.first.the_mbr               = first->the_node->branch[i].rect;
      j_el_aux.first.the_id                = (int)first->the_node->branch[i].child;
      j_el_aux.first.is_data               = true;  // its a data item (P)
      j_el_aux.first.target_node_explored  = true;  // no more to explore

      j_el_aux.second.the_node             = NULL;
      j_el_aux.second.the_mbr              = second->the_node->branch[j].rect;
      j_el_aux.second.the_id               = (int)second->the_node->branch[j].child;
      j_el_aux.second.is_data              = true;  // its a data item (P)
      j_el_aux.second.target_node_explored = true;  // no more to explore

      // both are considered to be points
      j_el_aux.dist = join_dist_func->compute_distance(& j_el_aux.second.the_mbr, & j_el_aux.first.the_mbr);
      if ((NULL != join_max_distance && j_el_aux.dist < *join_max_distance) ||
          (NULL == join_max_distance)) join_pqueue->push(j_el_aux);

      (*join_cpu_comp_total) ++; // accounting |n1| * |n2| computations
    }
  }
  // misc accounting crap (only 1 IO per node!)
  join_disk_acc1[first ->the_node->level]++;
  join_disk_acc2[second->the_node->level]++;

  // remember what we got out and mark it as explored
  j_el.first.target_node_explored = j_el.second.target_node_explored = true;
}


void Rtree::join_aux_leaf_point(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second,
                               JoinElement & j_el, bool swap, int exploration_model) {
  // compares a leaf to a point (first is leaf, second is point)
  assert( ! first->is_data); assert( second->is_data); // only second is a point
  assert(first ->the_node != NULL); // valid
  assert(first ->the_node->level == 0); // first is a leaf
  assert(second->the_node == NULL); // second one is a point
  JoinElement j_el_aux;
  for (int i=0; i<first->the_node->count; i++) {
    j_el_aux.first.the_node              = NULL;
    j_el_aux.first.the_mbr               = first->the_node->branch[i].rect;
    j_el_aux.first.the_id                = (int)first->the_node->branch[i].child;
    j_el_aux.first.is_data               = true;  // its a data item (P)
    j_el_aux.first.target_node_explored  = true;  // no more to explore

    j_el_aux.second.the_node             = second->the_node;
    j_el_aux.second.the_mbr              = second->the_mbr;
    j_el_aux.second.the_id               = second->the_id;
    j_el_aux.second.is_data              = second->is_data;
    j_el_aux.second.target_node_explored = second->target_node_explored;

    // both are considered to be points
    j_el_aux.dist = join_dist_func->compute_distance(& j_el_aux.second.the_mbr, & j_el_aux.first.the_mbr);
    if ((NULL != join_max_distance && j_el_aux.dist < *join_max_distance) ||
        (NULL == join_max_distance)) join_pqueue->push(j_el_aux);

    (*join_cpu_comp_total) ++; // accounting |n1| * |n2| computations
  }
  // misc accounting crap (only 1 IO per node!)
  join_disk_acc1[first ->the_node->level]++;
  //join_disk_acc2[second->the_node->level]++; // just copied second

  // remember what we got out and mark it as explored
  j_el.first.target_node_explored = j_el.second.target_node_explored = true;
}


void Rtree::join_aux_node_point(JoinElement::JoinTerm * first, JoinElement::JoinTerm * second,
                               JoinElement & j_el, bool swap, int exploration_model) {
  // compares an internal node to a point (first is node, second is point)
  assert( ! first->is_data); assert( second->is_data); // only second is a point
  assert(first ->the_node != NULL); // valid
  assert(first ->the_node->level > 0); // first is a node
  assert(second->the_node == NULL); // second one is a point
  JoinElement j_el_aux;
  for (int i=0; i<first->the_node->count; i++) {
    j_el_aux.first.the_node              = first->the_node->branch[i].child;
    j_el_aux.first.the_mbr               = first->the_node->branch[i].rect;
    j_el_aux.first.the_id                = -1;
    j_el_aux.first.is_data               = false;  // its not data
    j_el_aux.first.target_node_explored  = false;  //

    j_el_aux.second.the_node             = second->the_node;
    j_el_aux.second.the_mbr              = second->the_mbr;
    j_el_aux.second.the_id               = second->the_id;
    j_el_aux.second.is_data              = second->is_data;
    j_el_aux.second.target_node_explored = second->target_node_explored;

    // note the order, the second one is assumed to be the mbr and first is a point
    j_el_aux.dist = join_dist_func->compute_mindist(& j_el_aux.second.the_mbr, & j_el_aux.first.the_mbr);
    if ((NULL != join_max_distance && j_el_aux.dist < *join_max_distance) ||
        (NULL == join_max_distance)) join_pqueue->push(j_el_aux);

    (*join_cpu_comp_total) ++; // accounting |n1| * |n2| computations
  }
  // misc accounting crap (only 1 IO per node!)
  join_disk_acc1[first ->the_node->level]++;
  //join_disk_acc2[second->the_node->level]++;//just copied second, no io

  // remember what we got out and mark it as explored
  j_el.first.target_node_explored = j_el.second.target_node_explored = true;
}

int Rtree::SearchJoinNext(float & distance, join_pair_result & item) {
  assert(join_active_query);   assert(NULL != join_dist_func);
  assert(NULL != join_pqueue); //assert(NULL != join_q_already_returned);

  JoinElement j_el, j_el_aux;
  JoinElement::JoinTerm *  first = NULL, * second = NULL, *term = NULL;
  bool swap = false; // needed for functions that can be called with exckanged trees

  int em = 1; // explore model

  while (join_pqueue->size() > 0) { // still stuff
    j_el   = join_pqueue->top(); // get the top
    first  = &j_el.first;        // alias them
    second = &j_el.second;       // alias them
    join_pqueue->pop();          // and dispose it
    swap   = false;
    if (first->is_data && second->is_data) { // P-P
      // both are points, assemble and return them.
      join_aux_point_point(j_el, item, em);
      distance = item.distance;
      //join_q_already_returned->push(j_el);    // ok, processed it already
      return 1;
    } else if (NULL != first->the_node && NULL != second->the_node) { // both leaf or node
      assert( ! first->is_data); assert( ! second->is_data); // none is a point
      if (first->the_node->level  > 0 && second->the_node->level >0) { // N-N
        join_aux_node_node(first, second, j_el, swap, em);   // both are internal nodes
      } else if (first->the_node->level == 0 && second->the_node->level  > 0) { // L-N
        swap = true;
        join_aux_node_leaf(second, first, j_el, swap, em); // first is leaf, second is internal node
      } else if (first->the_node->level  > 0 && second->the_node->level == 0) { // N-L
        join_aux_node_leaf(first, second, j_el, swap, em); // second is leaf, first is internal node
      } else if (first->the_node->level == 0 && second->the_node->level == 0) { // L-L
        join_aux_leaf_leaf(first, second, j_el, swap, em); // second is leaf, first is internal node
      }
    } else { // one or both are NULL
      //  if both are NULL, then it must be p-p which is
      //  handled elsewhere, so its bad...

      //cout << "first is " << first->the_node << endl;
      //cout << "second is " << second->the_node << endl;


      assert( ! (first->is_data && second->is_data)); // p-p must not be allowed here!
      assert( first->is_data || second->is_data);     // exactly 1 is a p
      assert( (NULL==first->the_node && NULL!=second->the_node) ||
              (NULL!=first->the_node && NULL==second->the_node)); // only 1
      if (NULL==first->the_node && NULL != second->the_node) { // swap them
        term = first; first = second; second = term;
        swap = true;
      } // else nothing to swap!
      assert(NULL != first->the_node && NULL == second->the_node);
      assert(second->is_data); // first is N or L, second is P
      if (first->the_node->level > 0) { // N-P
        join_aux_node_point(first, second, j_el, swap, em);
      } else if (first->the_node->level == 0) { // L-P
        join_aux_leaf_point(first, second, j_el, swap, em);
      } else { // should not happen

        assert(false);
      }
    }
    //join_q_already_returned->push(j_el);    // ok, processed it already
  }

  return 0; // queue empty
}

int Rtree::SearchJoinClose() {
  assert(join_active_query);
  if (NULL != join_pqueue) {
    delete join_pqueue;   // if old, nuke it.
    join_pqueue = NULL;
  }
  /*if (NULL != join_q_already_returned) {
    delete join_q_already_returned;   // if old, nuke it.
    join_q_already_returned = NULL;
  }*/
  join_active_query = false;
  return 1; // all ok
}

int Rtree::SearchJoinNewIteration(DistanceFunction * df_new, int & cpu_distance_comps,
                                  int & nodes_copied, int reconstruct_model) {
  assert(join_active_query); assert(NULL != join_dist_func); // ensure correct state
  return 1; // all ok
}


//--------------------------------------------------------------------------------------
//-------------- Range search using a sphere--------------------------------------------
//--------------  Written by Chen Li----------------------------------------------------
//--------------------------------------------------------------------------------------

int Rtree::RTreeSearchSphere(struct Point *queryPoint, float radius, vector<int> &answers)
{
  long  disk_acc[10];
  struct Node *rootNode = TreeRoot;
  return RTreeSearchSphere(rootNode, queryPoint, radius, disk_acc, answers);
}


// Search in an index tree or subtree for all data retangles that
// overlap the given sphere
// Returns the number of qualifying data rects.
int Rtree::RTreeSearchSphere(struct Node *N, struct Point *queryPoint, float radius,
			     long *diskacc, vector<int> &answers)
{
  struct Node *n = N;
  int hitCount = 0, i;
  //struct Rect *r = R;
  //struct Point querypoint/*, centroid*/;
  //float maxradius=0.0;

  assert(n);
  assert(n->level >= 0);
  assert(queryPoint);

  //maxradius=(r->boundary[NUMDIMS] - r->boundary[0]) * RANGEFACTOR; //??

  if (n->level > 0) { /* this is an internal node in the tree */
    for (i=0; i<NODECARD; i++) {
      if (spheres_exist && n->level == 1) { // use both conditions
        if (n->branch[i].child &&
	    RTreeOverlapSphere(&n->branch[i].rect, queryPoint, radius)) {
          /* centroid=Center(&n->branch[i].rect);
             if (Distance(&querypoint, &centroid, DIST_METRIC) <= (n->branch[i].radius + maxradius)) */
                 //printf("Accessing a node at level %d \n", n->level);
          diskacc[n->level]++;
          hitCount += RTreeSearchSphere(n->branch[i].child, queryPoint, radius, diskacc, answers);
        }
      } else {
        if (n->branch[i].child &&
	    RTreeOverlapSphere(&n->branch[i].rect, queryPoint, radius)) {
          //printf("Accessing a node at level %d \n", n->level);
          diskacc[n->level]++;
          hitCount += RTreeSearchSphere(n->branch[i].child, queryPoint, radius, diskacc, answers);
        }
      }
    }
  } else { /* this is a leaf node */
    for (i=0; i<NODECARD; i++) {
    /* if (n->branch[i].child && RTreeOverlapSphere(r,&n->branch[i].rect)) */
      if (n->branch[i].child &&
	  RTreeOverlapSphere(&n->branch[i].rect, queryPoint, radius)) {
        // printf("Accessing a node at level %d \n", n->level);
        diskacc[n->level]++;
        hitCount++;

	answers.push_back((int)n->branch[i].child);  // Added by Chen Li
      }
    }
  }
  return hitCount;
}
