/* Copyright (C) 2007  Pascal Felber
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include "../../project/src/dstm_pthread/dstm_pthread.h"
#include "../../project/src/dstm_malloc/dstm_malloc.h"
#include "../../project/src/dstm_barrier.h"

#ifdef ENABLE_REPORTING
#include "tanger-stm-stats.h"
#endif

//#define DEBUG

#ifdef DEBUG
#define IO_FLUSH                       fflush(NULL)
/* Note: stdio is thread-safe */
#endif
#define DEFAULT_DURATION  1000
#define DEFAULT_INITIAL  1024
#define DEFAULT_NB_THREADS 120
#define DEFAULT_RANGE  0xFFFF
#define DEFAULT_SEED  0
#define DEFAULT_UPDATE  20
#define DEFAULT_NB_NODES 20

#define STORE(addr, value) (*(addr) = (value))
#define LOAD(addr) (*(addr))

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* ################################################################### *
 * GLOBALS
 * ################################################################### */

//static int verbose_flag = 0;

volatile int STOP = 0;
volatile int THREADS = 0;
barrier_t *loc_b = NULL;
gasnett_mutex_t mtx_b = GASNETT_MUTEX_INITIALIZER;

/* ################################################################### *
 * RBTREE
 * ################################################################### */

typedef struct node {
    intptr_t k;
    intptr_t v;
    struct node* p;
    struct node* l;
    struct node* r;
    intptr_t c;
} node_t;

typedef struct rbtree {
    node_t* root;
} rbtree_t;

#define LDA(a)      *(a)
#define STA(a,v)    *(a) = (v)
#define LDV(a)      (a)
#define STV(a,v)    (a) = (v)
#define LDF(o,f)    ((o)->f)
#define STF(o,f,v)  ((o)->f) = (v)
#define LDNODE(o,f) ((LDF((o),f)))

enum {
    RED   = 0,
    BLACK = 1
};


/*
 * See also:
 * - Doug Lea's j.u.TreeMap
 * - Keir Fraser's rb_stm.c and rb_lock_serialisedwriters.c in libLtx.
 *
 * Following Doug Lea's TreeMap example, we avoid the use of the magic
 * "nil" sentinel pointers.  The sentinel is simply a convenience and
 * is not fundamental to the algorithm.  We forgo the sentinel as
 * it is a source of false+ data conflicts in transactions.  Relatedly,
 * even with locks, use of a nil sentil can result in considerable
 * cache coherency traffic on traditional SMPs.
 */


/* =============================================================================
 * lookup
 * =============================================================================
 */
node_t*
lookup (rbtree_t* s, int key)
{
    node_t* p = LDNODE(s, root);

    while (p != NULL) {
        int cmp = key - LDF(p,k);

        if (cmp == 0) {
            return p;
        }
        if (cmp < 0) p = p->l;
        else p = p->r;
//        p = ((cmp < 0) ? LDNODE(p, l) : LDNODE(p, r));
    }

    return NULL;
}
#define LOOKUP(set, key)  lookup(set, key)


/*
 * Balancing operations.
 *
 * Implementations of rebalancings during insertion and deletion are
 * slightly different than the CLR version.  Rather than using dummy
 * nilnodes, we use a set of accessors that deal properly with null.  They
 * are used to avoid messiness surrounding nullness checks in the main
 * algorithms.
 *
 * From CLR
 */


/* =============================================================================
 * rotateLeft
 * =============================================================================
 */
void
rotateLeft (rbtree_t* s, node_t* x )
{
    node_t* r = LDNODE(x, r); /* AKA r, y */
    node_t* rl = LDNODE(r, l);
    STF(x, r, rl);
    if (rl != NULL) {
        STF(rl, p, x);
    }
    /* TODO: compute p = xp = x->p.  Use xp for R-Values in following */
    node_t* xp = LDNODE(x, p);
    STF(r, p, xp);
    if (xp == NULL) {
        STF(s, root, r);
    } else if (LDNODE(xp, l) == x) {
        STF(xp, l, r);
    } else {
        STF(xp, r, r);
    }
    STF(r, l, x);
    STF(x, p, r);
}
#define ROTATE_LEFT(set, node)  rotateLeft(set, node)


/* =============================================================================
 * rotateRight
 * =============================================================================
 */
void
rotateRight (rbtree_t* s, node_t* x)
{
    node_t* l = LDNODE(x, l); /* AKA l,y */
    node_t* lr = LDNODE(l, r);
    STF(x, l, lr);
    if (lr != NULL) {
        STF(lr, p, x);
    }
    node_t* xp = LDNODE(x, p);
    STF(l, p, xp);
    if (xp == NULL) {
        STF(s, root, l);
    } else if (LDNODE(xp, r) == x) {
        STF(xp, r, l);
    } else {
        STF(xp, l, l);
    }
    STF(l, r, x);
    STF(x, p, l);
}
#define ROTATE_RIGHT(set, node)  rotateRight(set, node)


/* =============================================================================
 * parentOf
 * =============================================================================
 */
node_t*
parentOf (node_t* n)
{
   return (n ? LDNODE(n,p) : NULL);
}
#define PARENT_OF(n) parentOf(n)


/* =============================================================================
 * leftOf
 * =============================================================================
 */
node_t*
leftOf (node_t* n)
{
   return (n ? LDNODE(n, l) : NULL);
}
#define LEFT_OF(n)  leftOf(n)


/* =============================================================================
 * rightOf
 * =============================================================================
 */
node_t*
rightOf (node_t* n)
{
    return (n ? LDNODE(n, r) : NULL);
}
#define RIGHT_OF(n)  rightOf(n)


/* =============================================================================
 * colorOf
 * =============================================================================
 */
int
colorOf (node_t* n)
{
    return (n ? (int)LDNODE(n, c) : BLACK);
}
#define COLOR_OF(n)  colorOf(n)


/* =============================================================================
 * setColor
 * =============================================================================
 */
void
setColor (node_t* n, int c)
{
    if (n != NULL) {
        STF(n, c, c);
    }
}
#define SET_COLOR(n, c)  setColor(n, c)


/* =============================================================================
 * fixAfterInsertion
 * =============================================================================
 */
void
fixAfterInsertion (rbtree_t* s, node_t* x)
{
    STF(x, c, RED);
    while (x != NULL && x != LDNODE(s, root)) {
        node_t* xp = LDNODE(x, p);
        if (LDF(xp, c) != RED) {
            break;
        }
        /* TODO: cache g = ppx = PARENT_OF(PARENT_OF(x)) */
        if (PARENT_OF(x) == LEFT_OF(PARENT_OF(PARENT_OF(x)))) {
            node_t*  y = RIGHT_OF(PARENT_OF(PARENT_OF(x)));
            if (COLOR_OF(y) == RED) {
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                x = PARENT_OF(PARENT_OF(x));
            } else {
                if (x == RIGHT_OF(PARENT_OF(x))) {
                    x = PARENT_OF(x);
                    ROTATE_LEFT(s, x);
                }
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                if (PARENT_OF(PARENT_OF(x)) != NULL) {
                    ROTATE_RIGHT(s, PARENT_OF(PARENT_OF(x)));
                }
            }
        } else {
            node_t* y = LEFT_OF(PARENT_OF(PARENT_OF(x)));
            if (COLOR_OF(y) == RED) {
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(y, BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                x = PARENT_OF(PARENT_OF(x));
            } else {
                if (x == LEFT_OF(PARENT_OF(x))) {
                    x = PARENT_OF(x);
                    ROTATE_RIGHT(s, x);
                }
                SET_COLOR(PARENT_OF(x),  BLACK);
                SET_COLOR(PARENT_OF(PARENT_OF(x)), RED);
                if (PARENT_OF(PARENT_OF(x)) != NULL) {
                    ROTATE_LEFT(s, PARENT_OF(PARENT_OF(x)));
                }
            }
        }
    }
    node_t* ro = LDNODE(s, root);
    if (LDF(ro, c) != BLACK) {
        STF(ro, c, BLACK);
    }
}
#define FIX_AFTER_INSERTION(s, x)  fixAfterInsertion(s, x)


/* =============================================================================
 * insert
 * =============================================================================
 */
node_t*
insert (rbtree_t* s, int k, int v, node_t* n)
{
    node_t* t  = LDNODE(s, root);
    if (t == NULL) {
        if (n == NULL) {
            return NULL;
        }
        /* Note: the following STs don't really need to be transactional */
        STF(n, l, NULL);
        STF(n, r, NULL);
        STF(n, p, NULL);
        STF(n, k, k);
        STF(n, v, v);
        STF(n, c, BLACK);
        STF(s, root, n);
        return NULL;
    }

    for (;;) {
        intptr_t cmp = k - LDF(t, k);
        if (cmp == 0) {
            return t;
        } else if (cmp < 0) {
            node_t* tl = LDNODE(t, l);
            if (tl != NULL) {
                t = tl;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, l, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        } else { /* cmp > 0 */
            node_t* tr = LDNODE(t, r);
            if (tr != NULL) {
                t = tr;
            } else {
                STF(n, l, NULL);
                STF(n, r, NULL);
                STF(n, k, k);
                STF(n, v, v);
                STF(n, p, t);
                STF(t, r, n);
                FIX_AFTER_INSERTION(s, n);
                return NULL;
            }
        }
    }
}
#define INSERT(s, k, v, n)  insert(s, k, v, n)


/*
 * Return the given node's successor node---the node which has the
 * next key in the the left to right ordering. If the node has
 * no successor, a null pointer is returned rather than a pointer to
 * the nil node
 */


/* =============================================================================
 * successor
 * =============================================================================
 */
node_t*
successor (node_t* t)
{
    if (t == NULL) {
        return NULL;
    } else if (LDNODE(t, r) != NULL) {
        node_t* p = LDNODE(t, r);
        while (LDNODE(p, l) != NULL) {
            p = LDNODE(p, l);
        }
        return p;
    } else {
        node_t* p = LDNODE(t, p);
        node_t* ch = t;
        while (p != NULL && ch == LDNODE(p, r)) {
            ch = p;
            p = LDNODE(p, p);
        }
        return p;
    }
}
#define SUCCESSOR(n)  successor(n)


/* =============================================================================
 * fixAfterDeletion
 * =============================================================================
 */
void
fixAfterDeletion (rbtree_t* s, node_t*  x)
{
    while (x != LDNODE(s,root) && COLOR_OF(x) == BLACK) {
        if (x == LEFT_OF(PARENT_OF(x))) {
            node_t* sib = RIGHT_OF(PARENT_OF(x));
            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_LEFT(s, PARENT_OF(x));
                sib = RIGHT_OF(PARENT_OF(x));
            }

            if (COLOR_OF(LEFT_OF(sib))  == BLACK &&
                COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                SET_COLOR(sib, RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(RIGHT_OF(sib)) == BLACK) {
                    SET_COLOR(LEFT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_RIGHT(s, sib);
                    sib = RIGHT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(RIGHT_OF(sib), BLACK);
                ROTATE_LEFT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s,root);
            }
        } else { /* symmetric */
            node_t* sib = LEFT_OF(PARENT_OF(x));

            if (COLOR_OF(sib) == RED) {
                SET_COLOR(sib, BLACK);
                SET_COLOR(PARENT_OF(x), RED);
                ROTATE_RIGHT(s, PARENT_OF(x));
                sib = LEFT_OF(PARENT_OF(x));
            }

            if (COLOR_OF(RIGHT_OF(sib)) == BLACK &&
                COLOR_OF(LEFT_OF(sib)) == BLACK) {
                SET_COLOR(sib,  RED);
                x = PARENT_OF(x);
            } else {
                if (COLOR_OF(LEFT_OF(sib)) == BLACK) {
                    SET_COLOR(RIGHT_OF(sib), BLACK);
                    SET_COLOR(sib, RED);
                    ROTATE_LEFT(s, sib);
                    sib = LEFT_OF(PARENT_OF(x));
                }
                SET_COLOR(sib, COLOR_OF(PARENT_OF(x)));
                SET_COLOR(PARENT_OF(x), BLACK);
                SET_COLOR(LEFT_OF(sib), BLACK);
                ROTATE_RIGHT(s, PARENT_OF(x));
                /* TODO: consider break ... */
                x = LDNODE(s, root);
            }
        }
    }

    if (x != NULL && LDF(x,c) != BLACK) {
       STF(x, c, BLACK);
    }
}
#define FIX_AFTER_DELETION(s, n)  fixAfterDeletion(s, n)


/* =============================================================================
 * delete
 * =============================================================================
 */
node_t*
delete (rbtree_t* s, node_t* p)
{
    /*
     * If strictly internal, copy successor's element to p and then make p
     * point to successor
     */
    if (LDNODE(p, l) != NULL && LDNODE(p, r) != NULL) {
        node_t* s = SUCCESSOR(p);
        STF(p, k, (intptr_t)LDNODE(s, k));
        STF(p, v, (intptr_t)LDNODE(s, v));
        p = s;
    } /* p has 2 children */

    /* Start fixup at replacement node, if it exists */
    node_t* replacement =
        ((LDNODE(p, l) != NULL) ? LDNODE(p, l) : LDNODE(p, r));

    if (replacement != NULL) {
        /* Link replacement to parent */
        /* TODO: precompute pp = p->p and substitute below ... */
        STF (replacement, p, LDNODE(p, p));
        node_t* pp = LDNODE(p, p);
        if (pp == NULL) {
            STF(s, root, replacement);
        } else if (p == LDNODE(pp, l)) {
            STF(pp, l, replacement);
        } else {
            STF(pp, r, replacement);
        }

        /* Null out links so they are OK to use by fixAfterDeletion */
        STF(p, l, NULL);
        STF(p, r, NULL);
        STF(p, p, NULL);

        /* Fix replacement */
        if (LDF(p,c) == BLACK) {
            FIX_AFTER_DELETION(s, replacement);
        }
    } else if (LDNODE(p, p) == NULL) { /* return if we are the only node */
        STF(s, root, NULL);
    } else { /* No children. Use self as phantom replacement and unlink */
        if (LDF(p, c) == BLACK) {
            FIX_AFTER_DELETION(s, p);
        }
        node_t* pp = LDNODE(p, p);
        if (pp != NULL) {
            if (p == LDNODE(pp, l)) {
                STF(pp,l, NULL);
            } else if (p == LDNODE(pp, r)) {
                STF(pp, r, NULL);
            }
            STF(p, p, NULL);
        }
    }
    return p;
}
#define DELETE(s, n)  delete(s, n)


/* =============================================================================
 * firstEntry
 * =============================================================================
 */
node_t*
firstEntry (rbtree_t* s)
{
    node_t* p = s->root;
    if (p != NULL) {
        while (p->l != NULL) {
            p = p->l;
        }
    }
    return p;
}


#if 0
/* =============================================================================
 * predecessor
 * =============================================================================
 */
static node_t*
predecessor (node_t* t)
{
    if (t == NULL)
        return NULL;
    else if (t->l != NULL) {
        node_t* p = t->l;
        while (p->r != NULL) {
            p = p->r;
        }
        return p;
    } else {
        node_t* p = t->p;
        node_t* ch = t;
        while (p != NULL && ch == p->l) {
            ch = p;
            p = p->p;
        }
        return p;
    }
}
#endif


/*
 * Compute the BH (BlackHeight) and validate the tree.
 *
 * This function recursively verifies that the given binary subtree satisfies
 * three of the red black properties. It checks that every red node has only
 * black children. It makes sure that each node is either red or black. And it
 * checks that every path has the same count of black nodes from root to leaf.
 * It returns the blackheight of the given subtree; this allows blackheights to
 * be computed recursively and compared for left and right siblings for
 * mismatches. It does not check for every nil node being black, because there
 * is only one sentinel nil node. The return value of this function is the
 * black height of the subtree rooted at the node ``root'', or zero if the
 * subtree is not red-black.
 *
 */


/* =============================================================================
 * verifyRedBlack
 * =============================================================================
 */
int
verifyRedBlack (node_t* root, int depth)
{
    int height_left;
    int height_right;

    if (root == NULL) {
        return 1;
    }

    height_left  = verifyRedBlack(root->l, depth+1);
    height_right = verifyRedBlack(root->r, depth+1);
    if (height_left == 0 || height_right == 0) {
        return 0;
    }
    if (height_left != height_right) {
        printf(" Imbalance @depth=%d : %d %d\n", depth, height_left, height_right);
    }

    if (root->l != NULL && root->l->p != root) {
       printf(" lineage\n");
    }
    if (root->r != NULL && root->r->p != root) {
       printf(" lineage\n");
    }

    /* Red-Black alternation */
    if (root->c == RED) {
        if (root->l != NULL && root->l->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        if (root->r != NULL && root->r->c != BLACK) {
          printf("VERIFY %d\n", __LINE__);
          return 0;
        }
        return height_left;
    }
    if (root->c != BLACK) {
        printf("VERIFY %d\n", __LINE__);
        return 0;
    }
    return (height_left + 1);
}


/* =============================================================================
 * rbtree_verify
 * =============================================================================
 */
int
rbtree_verify (rbtree_t* s, int verbose)
{
    node_t* root = s->root;
    if (root == NULL) {
        return 1;
    }
    if (verbose) {
       printf("Integrity check: ");
    }

    if (root->p != NULL) {
        printf("  (WARNING) root %p parent=%p\n",
               root, root->p);
        return -1;
    }
    if (root->c != BLACK) {
        printf("  (WARNING) root %p color=%lX\n",
               root, (unsigned long)root->c);
    }

    /* Weak check of binary-tree property */
    int ctr = 0;
    node_t* its = firstEntry(s);
    while (its != NULL) {
        ctr++;
        node_t* child = its->l;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        child = its->r;
        if (child != NULL && child->p != its) {
            printf("Bad parent\n");
        }
        node_t* nxt = successor(its);
        if (nxt == NULL) {
            break;
        }
        if (its->k >= nxt->k) {
            printf("Key order %p (%ld %ld) %p (%ld %ld)\n",
                   its, (long)its->k, (long)its->v,
                   nxt, (long)nxt->k, (long)nxt->v);
            return -3;
        }
        its = nxt;
    }

    int vfy = verifyRedBlack(root, 0);
    if (verbose) {
        printf(" Nodes=%d Depth=%d\n", ctr, vfy);
    }
    return vfy;
}


/* =============================================================================
 * rbtree_alloc
 * =============================================================================
 */
rbtree_t*
rbtree_alloc (void)
{
    rbtree_t* n = (rbtree_t* )dstm_malloc(sizeof(rbtree_t), UINT_MAX);
    n->root = NULL;
    return n;
}


/* =============================================================================
 * rbtree_free
 * =============================================================================
 */
void
rbtree_free (rbtree_t* r)
{
    dstm_free(r);
}


/* =============================================================================
 * getNode
 * =============================================================================
 */
node_t*
getNode (int gasnet_node)
{
    node_t* n = (node_t*)dstm_malloc(sizeof(node_t), gasnet_node);
    return n;
}


/* =============================================================================
 * releaseNode
 * =============================================================================
 */
void
releaseNode (node_t* n)
{
    dstm_free(n);
}


/* =============================================================================
 * rbtree_insert
 * =============================================================================
 */
int
rbtree_insert (rbtree_t* r, int key, int val, int gasnet_node)
{
    node_t* node = getNode(gasnet_node);
    node_t* ex = INSERT(r, key, val, node);
    if (ex != NULL) {
        releaseNode(node);
    }
    return (ex != NULL);
}


/* =============================================================================
 * rbtree_delete
 * =============================================================================
 */
int
rbtree_delete (rbtree_t* r, int key)
{
    node_t* node = NULL;
    node = LOOKUP(r, key);
    if (node != NULL) {
        node = DELETE(r, node);
    }
    if (node != NULL) {
       releaseNode(node);
    }
    return (node != NULL);
}


/* =============================================================================
 * rbtree_update
 * =============================================================================
 */
/*int
rbtree_update (rbtree_t* r, int key, int val)
{
    node_t* nn = getNode();
    node_t* ex = INSERT(r, key, val, nn);
    if (ex != NULL) {
        STF(ex, v, val);
        releaseNode(nn);
        return 0;
    }
    return 1;
}*/


/* =============================================================================
 * rbtree_get
 * =============================================================================
 */
int
rbtree_get (rbtree_t* r, int key) {
    node_t* n = LOOKUP(r, key);
    if (n != NULL) {
        int val = LDF(n, v);
        return val;
    }
    return 0;
}


/* =============================================================================
 * rbtree_contains
 * =============================================================================
 */
int
rbtree_contains (rbtree_t* r, int key)
{
    node_t* n = LOOKUP(r, key);
    return n != NULL;
}

int rbtree_getsize(rbtree_t* r)
{
	int size = 0;
	node_t* n;
	for (n = firstEntry(r); n != NULL; n = successor(n))
		size++;
	return size;
}

/* =============================================================================
 *
 * End of rbtree.c
 *
 * =============================================================================
 */

typedef rbtree_t intset_t;

intset_t *set_new()
{
  return rbtree_alloc();
}

void set_delete(intset_t *set)
{
  rbtree_free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  
  __tm_atomic{
    if (!rbtree_verify(set, 0)) {
      printf("Validation failed!\n");
      exit(1);
    }
    size = rbtree_getsize(set);
  }

  return size;
}

int set_add(intset_t *set, intptr_t val, int notx, int gasnet_node)
{
//printf("set add node %d\n", gasnet_node);
//fflush(stdout);
  int res;

  if (notx) {
    res = !rbtree_insert(set, val, val, gasnet_node);
  } else {
    __tm_atomic {
    res = !rbtree_insert(set, val, val, gasnet_node);
    }
  }
//printf("set add out\n");
//fflush(stdout);
  return res;
}

int set_remove(intset_t *set, intptr_t val, int notx)
{
//printf("set remove\n");
//fflush(stdout);

  int res;

  if (notx) {
    res = rbtree_delete(set, val);
  } else {
    __tm_atomic {
      res = rbtree_delete(set, val);
    }
  }
//printf("set remove out\n");
//fflush(stdout);

  return res;
}

int set_contains(intset_t *set, intptr_t val, int notx)
{
//printf("set contains\n");
//fflush(stdout);

  int res;

  if (notx) {
    res = rbtree_contains(set, val);
  } else {
    __tm_atomic {
      res = rbtree_contains(set, val);
    }
  }
//printf("set contains out\n");
//fflush(stdout);

  return res;
}

/* ################################################################### *
 * STRESS TEST
 * ################################################################### */

typedef struct thread_data {
  int range;
  int update;
  int nb_add;
  int nb_remove;
  int nb_contains;
  int nb_found;
  int diff;
  unsigned int seed;

  int tnum;
  int *stop;

  intset_t *set;
  barrier_t *barrier;
  int barrier_id;
  char padding[64];
} thread_data_t;

#ifdef POLL_T
void *polling(void *data)
{
  while (STOP == 0){
    gasnet_AMPoll();
  }
}
#endif

void *test(void *data)
{
  int val, last = 0;
  thread_data_t *d = (thread_data_t *)data;
  
  unsigned int seed;
  intset_t *set;
//  barrier_t *barrier;
  int update;
  int range;

  int tnum;
  int *pstop;
  int stop = 0;
  int barrier_id = 10000;

  int diff = 0;
  int nb_add = 0;
  int nb_remove = 0;
  int nb_found = 0;
  int nb_contains = 0;

  __tm_atomic{
    seed = d->seed;    
    set = d->set;
//    barrier = d->barrier;
    update = d->update;
    range = d->range;
    tnum = d->tnum;
//    barrier_id = d->barrier_id;
  }

  int SEED = seed;
  /* Wait on barrier */
  if(gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b == NULL){
      loc_b = (barrier_t*)malloc(sizeof(barrier_t));
      loc_b->id = barrier_id;
      barrier_init(loc_b,barrier_calculate_threads(tnum));
    }
    gasnett_mutex_unlock(&mtx_b);
    barrier_cross(loc_b,1);
  }else
    barrier_cross(d->barrier,1);

  last = -1;
  while (stop == 0) {
    if ((rand_r(&SEED) % 100) < update) {
      if (last < 0) {
        val = (rand_r(&SEED) % range) + 1;
        if (set_add(set, val, 0, UINT_MAX)) {
          diff++;
          last = val;
        }
        nb_add++;
      } else {
        if (set_remove(set, last, 0))
          diff--;
        nb_remove++;
        last = -1;
      }
    } else {
      val = (rand_r(&SEED) % range) + 1;
      if (set_contains(set, val, 0))
        nb_found++;
      nb_contains++;
    }

    __tm_atomic{
      pstop = d->stop;
      stop = *pstop;
    }
  }

/*  __tm_atomic{
    d->nb_add = nb_add;
    d->nb_remove = nb_remove;
    d->nb_found = nb_found;
    d->nb_contains = nb_contains;
    d->diff = diff;
  }*/

  if(gasnet_mynode() != 0){
    barrier_cross(loc_b,0);
  }else
    barrier_cross(d->barrier,0);

  if (gasnet_mynode() != 0){
    gasnett_mutex_lock(&mtx_b);
    if (loc_b != NULL){
      if (barrier_canbe_destroyed(loc_b)){
        /*printf("bar_sync %d ms\n",barrier_elapsed(loc_b));
	fflush(stdout);*/
        free(loc_b);
        loc_b = NULL;
      }
    }
    gasnett_mutex_unlock(&mtx_b);
  }

  __tm_atomic{
    d->nb_add = nb_add;    
    d->nb_remove = nb_remove;
    d->nb_found = nb_found;
    d->nb_contains = nb_contains;
    d->diff = diff;
  }

 /* printf("OUT #%d ads/rms/diff %d/%d/%d \n",++THREADS,nb_add,nb_remove,diff);
  fflush(stdout);*/
}

int main(int argc, char **argv)
{
  /*struct option long_options[] = {
    // These options set a flag
    {"silent",                    no_argument,       &verbose_flag, 0},
    {"verbose",                   no_argument,       &verbose_flag, 1},
    {"debug",                     no_argument,       &verbose_flag, 2},
    {"DEBUG",                     no_argument,       &verbose_flag, 3},
    // These options don't set a flag
    {"help",                      no_argument,       0, 'h'},
    {"duration",                  required_argument, 0, 'd'},
    {"initial-size",              required_argument, 0, 'i'},
    {"num-threads",               required_argument, 0, 'n'},
    {"range",                     required_argument, 0, 'r'},
    {"seed",                      required_argument, 0, 's'},
    {"update-rate",               required_argument, 0, 'u'},
    {0, 0, 0, 0}
  };*/

  intset_t *set;
  int i, c, val, size, reads, updates;
  thread_data_t *data;
  dstm_pthread_t *threads;
  dstm_pthread_attr_t attr;
  barrier_t *barrier;
  struct timeval start, end;
  struct timespec timeout;
  int duration = DEFAULT_DURATION;
  int initial = DEFAULT_INITIAL;
  int nb_threads = DEFAULT_NB_THREADS;
  int range = DEFAULT_RANGE;
  int seed = DEFAULT_SEED;
  int update = DEFAULT_UPDATE;

#ifdef POLL_T
  dstm_pthread_t *poll_thread;
#endif
  int *stop;
  /*while(1) {
    i = 0;
    c = getopt_long(argc, argv, "hd:i:n:r:s:u:", long_options, &i);

    if(c == -1)
      break;

    if(c == 0 && long_options[i].flag == 0)
      c = long_options[i].val;

    switch(c) {
     case 0:
       // Flag is automatically set
       break;
     case 'h':
       printf("Usage: %s [-d <int>] [-i <int>] [-n <int>] [-r <int>] [-s <int>] [-u <int>]\n", argv[0]);
       exit(0);
     case 'd':
       duration = atoi(optarg);
       break;
     case 'i':
       initial = atoi(optarg);
       break;
     case 'n':
       nb_threads = atoi(optarg);
       break;
     case 'r':
       range = atoi(optarg);
       break;
     case 's':
       seed = atoi(optarg);
       break;
     case 'u':
       update = atoi(optarg);
       break;
     case '?':
       printf("Use -h or --help for help\n");
       exit(0);
     default:
       exit(1);
    }
  }*/

  assert(duration >= 0);
  assert(initial >= 0);
  assert(nb_threads > 0);
  assert(range > 0);
  assert(update >= 0 && update <= 100);

  printf("Duration     : %d\n", duration);
  printf("Initial size : %d\n", initial);
  printf("Nb threads   : %d\n", nb_threads);
  printf("Value range  : %d\n", range);
  printf("Seed         : %d\n", seed);
  printf("Update rate  : %d\n", update);
  printf("int/long/ptr size: %u/%u/%u\n", (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(void*));

  timeout.tv_sec = duration / 1000;
  timeout.tv_nsec = (duration % 1000) * 1000000;

  if ((data = (thread_data_t *)dstm_malloc(nb_threads * sizeof(thread_data_t),UINT_MAX)) == NULL) {
    perror("malloc");
    return 1;
  }

  if ((stop = (int *)dstm_malloc(sizeof(int*),UINT_MAX)) == NULL) {
    perror("malloc");
    return 1;
  }

  if ((threads = (dstm_pthread_t *)malloc(nb_threads * sizeof(dstm_pthread_t))) == NULL) {
    perror("malloc");
    return 1;
  }
#ifdef POLL_T
  if ((poll_thread = (dstm_pthread_t *)malloc(sizeof(dstm_pthread_t))) == NULL) {
    perror("malloc");
    return 1;
  }
#endif
  if (seed == 0)
    srand((int)time(0));
  else
    srand(seed);
  set = set_new();

  *stop = 0;

  /* Populate set */
  printf("Adding %d entries to set\n", initial);

#ifdef POLL_T
  if (dstm_pthread_create(poll_thread,&attr, "polling", NULL) != 0) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
  usleep(1000);
#endif
 
/*  for (i = 0; i < initial; i++) {
    val = (rand() % range) + 1;
    set_add(set, val,1,-1);
  }*/

  int nodes = DEFAULT_NB_NODES;
  int up, down, pernode;
  int j = 0;
  up = 0; down = 0; pernode = initial/nodes;

  if (pernode == 0) {
    pernode = 1;
    nodes = initial;
  }

//      dump((char*)set, 160);
  for (j = 0; j<nodes;j++){
    if(j == nodes - 1)
      pernode = initial - (nodes -1)*pernode;
    up += pernode;
    for (i = down; i < up; i++) {
      val = (rand() % range) + 1;
      set_add(set, val,0,j);
      //print_set(set);
//      dump((char*)set, 160);
    }
    down = up;
  }

  size = set_size(set);
  printf("Set size  : %d\n", size);
  
  //print_set(set);

  /* Access set from all threads */
  /*barrier_init(&barrier, nb_threads + 1);*/
  barrier = (barrier_t*)malloc(sizeof(barrier_t));

  barrier->id = 10000;

  barrier_init(barrier,barrier_calculate_threads(nb_threads));


  dstm_pthread_attr_init(&attr);
  dstm_pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  printf("<<<------------->>>\n");
  fflush(stdout);
  for (i = 0; i < nb_threads; i++) {
    data[i].range = range;
    data[i].update = update;
    data[i].nb_add = 0;
    data[i].nb_remove = 0;
    data[i].nb_contains = 0;
    data[i].nb_found = 0;
    data[i].diff = 0;
    data[i].seed = rand();
    data[i].set = set;
    data[i].barrier = barrier;
    data[i].tnum = nb_threads;
    data[i].stop = stop;
    data[i].barrier_id = 10000;
  }
  
  for (i = 0; i < nb_threads; i++) {
//    printf("Creating thread %d\n", i);
//    fflush(stdout);
    if (dstm_pthread_create(&threads[i], &attr, "test", (void *)(&data[i])) != 0) {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }
  }
  
  dstm_pthread_attr_destroy(&attr);

  /* Start threads */
  barrier_cross(barrier,1);

  printf("STARTING...\n");
  fflush(stdout);

  gettimeofday(&start, NULL);
  if (nanosleep(&timeout, NULL) != 0) {
    perror("nanosleep");
    return 1;
  }
  __tm_atomic{
    *stop = 1;
  }

  printf("STOPPING...\n");
  fflush(stdout);

  barrier_cross(barrier,1);

  gettimeofday(&end, NULL);

  /* Wait for threads completion */
  for (i = 0; i < nb_threads; i++) {
    if (dstm_pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Error waiting for thread completion\n");
      return 1;
    }
  }

#ifdef POLL_T
  STOP = 1;
  if (dstm_pthread_join(*poll_thread, NULL) != 0) {
    fprintf(stderr, "Error waiting for thread completion\n");
    return 1;
  }
#endif

  duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
  reads = 0;
  updates = 0;
  int adds = 0;
  int removes = 0;
  for (i = 0; i < nb_threads; i++) {
    /*printf("Thread %d\n", i);
    printf("  #add      : %d\n", data[i].nb_add);
    printf("  #remove   : %d\n", data[i].nb_remove);
    printf("  #contains : %d\n", data[i].nb_contains);
    printf("  #found    : %d\n", data[i].nb_found);*/
    adds += data[i].nb_add;
    removes += data[i].nb_remove;
    updates += (data[i].nb_add + data[i].nb_remove);
    reads += data[i].nb_contains;
    size += data[i].diff;
  }
  int exp_size = set_size(set);

  printf("Set size    : %d (expected: %d)\n", exp_size, size); 
  printf("Ads/rms     : (%d / %d)\n", adds, removes);
  printf("Duration    : %d (ms)\n", duration);
  printf("#txs        : %d (%f / s)\n", reads + updates, (reads + updates) * 1000.0 / duration);
  printf("#read txs   : %d (%f / s)\n", reads, reads * 1000.0 / duration);
  printf("#update txs : %d (%f / s)\n", updates, updates * 1000.0 / duration);
  fflush(stdout);	
  //print_set(set);
/*  sleep(3);
  int aadds = 0;
  int rremoves = 0;
  for (i = 0; i < nb_threads; i++) {
    aadds += data[i].nb_add;
    rremoves += data[i].nb_remove;
  }
  printf("Ads/rms     : (%d / %d)\n", aadds, rremoves);*/

#ifdef ENABLE_REPORTING
  void* rh = tanger_stm_report_start("app");
  tanger_stm_report_append_string(rh, "name", "linkedlist");
  tanger_stm_report_append_int(rh, "duration_ms", duration);
  tanger_stm_report_append_int(rh, "threads", nb_threads);
  tanger_stm_report_append_int(rh, "size", initial);
  tanger_stm_report_append_int(rh, "updaterate", update);
  tanger_stm_report_append_int(rh, "range", range);
  tanger_stm_report_finish(rh, "app");
  rh = tanger_stm_report_start("throughput");
  tanger_stm_report_append_int(rh, "commits", reads + updates);
  tanger_stm_report_append_double(rh, "commits_per_s", ((double)reads + updates) *1000.0 / duration);
  tanger_stm_report_finish(rh, "throughput");
#endif

  /* Delete set */
  set_delete(set);

  free(threads);
#ifdef POLL_T
  free(poll_thread);
#endif
  dstm_free(data);

  return 0;
}
