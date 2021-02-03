/* ks/util.h - header for the 'util' builtin module in kscript
 *
 * Provides general datastructures not included as builtins, such as graph, binary search tree,
 *   bitset, and more. The goal is to have efficient implementations with best case Big-Oh 
 *   performance.
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSUTIL_H__
#define KSUTIL_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif


/** Constants **/


/** Types **/


/* util.Bitset - efficient implementation of a set of integers
 *
 */
typedef struct ks_bitset_s {
    KSO_BASE

    /* bit-set of integers */
    mpz_t val;

}* ks_bitset;



/* util.Bitset.__iter - efficient implementation of a set of integers
 *
 */
typedef struct ks_bitset_iter_s {
    KSO_BASE

    /* bitset being iterated */
    ks_bitset of;

    /* position being searched */
    ks_cint pos;

}* ks_bitset_iter;

/* Structure representing a node in a queue (i.e. a node in a doubly linked list)
 *
 */
struct ks_queue_item {

    /* Pointers to the next and previous items in the queue, or NULL if they don't exist */
    struct ks_queue_item *next, *prev;

    /* Object value (a reference is held to this) */
    kso val;

};

/* util.Queue - double-ended queue implementation
 *
 * Internally, a doubly-linked-list is used, and pointers to the first and last element are held
 */
typedef struct ks_queue_s {
    KSO_BASE

    /* First and last item in the queue */
    struct ks_queue_item *first, *last;

}* ks_queue;

/* util.Queue.__iter - iterator type
 *
 */
typedef struct ks_queue_iter_s {
    KSO_BASE

    /* Queue being iterated over */
    ks_queue of;

    /* Current position in the queue (or NULL if empty) */
    struct ks_queue_item *cur;

}* ks_queue_iter;


/* Single node within a binary search tree
 *
 */
struct ks_bst_item {

    /* Pointers to elements less than (left) and more than (right)  */
    struct ks_bst_item *left, *right;

    /* Key and val at this node */
    kso key, val;

};

/* util.BST - Binary Search Tree structure
 *
 */
typedef struct ks_bst_s {
    KSO_BASE

    /* Root node */
    struct ks_bst_item* root;

}* ks_bst;


/* util.BST.__iter - Binary Search Tree iterator
 *
 */
typedef struct ks_bst_iter_s {
    KSO_BASE

    /* Binary search tree being iterated */
    ks_bst of;

    /* Current position */
    struct ks_bst_item* cur;

    /* Current stack of nodes */
    int nstk;
    struct ks_bst_item** stk;

}* ks_bst_iter;


/* 'util.Graph' - directed graph type representing nodes and connections between them
 *
 * Internally implemented as a dict-of-dicts, where the primary dictionary ('nodes') has
 *   nodes as keys, and 'nodes[key]' gives a dictionary describing the neighbors of 'key'
 *   within the graph. Then, 'nodes[key][to]' returns the edge value for 'key -> to'
 * 
 * TODO: have an inverse neighbors dictionary?
 * 
 * Memory: O(|V|+|E|)
 */
typedef struct ks_graph_s {
    KSO_BASE

    /* Main dictionary */
    ks_dict nodes;

}* ks_graph;


/* util.Graph.bfs - Breadth-First-Search (BFS) iterator
 *
 */
typedef struct ks_graph_bfs_s {
    KSO_BASE

    /* Object being iterated */
    ks_graph of;

    /* Set of visited nodes */
    ks_set visited;

    /* Queue of nodes being visited */
    ks_queue queue;

}* ks_graph_bfs;


/* util.Graph.dfs - Depth-First-Search (DFS) iterator
 *
 */
typedef struct ks_graph_dfs_s {
    KSO_BASE

    /* Object being iterated */
    ks_graph of;

    /* Set of visited nodes */
    ks_set visited;

    /* Stack of nodes being visited */
    ks_list stk;

}* ks_graph_dfs;


/** Functions **/


/* Create a new (empty) 'util.Bitset' object
 */
KS_API ks_bitset ks_bitset_new(ks_type tp);

/* Add an element to the bitset
 */
KS_API bool ks_bitset_add(ks_bitset self, ks_cint elem);

/* Remove an element from the bitset
 */
KS_API bool ks_bitset_del(ks_bitset self, ks_cint elem);

/* Tells whether a bitset has an element
 */
KS_API bool ks_bitset_has(ks_bitset self, ks_cint elem);


/* Create a new (empty) 'util.Queue' object
 */
KS_API ks_queue ks_queue_new(ks_type tp);

/* Push a new object on to the back of a queue
 */
KS_API bool ks_queue_push(ks_queue self, kso obj);

/* Pop from the front of the queue
 */
KS_API kso ks_queue_pop(ks_queue self);

/* Tell whether a queue is empty
 */
KS_API bool ks_queue_empty(ks_queue self);


/* Create a new (empty) 'util.BST' object
 */
KS_API ks_bst ks_bst_new(ks_type tp);

/* Add an element to the binary search tree
 */
KS_API bool ks_bst_set(ks_bst self, kso key, kso val);

/* Get an element to the binary search tree
 */
KS_API kso ks_bst_get(ks_bst self, kso key);


/* Create a new (empty) 'util.Graph' type
 */
KS_API ks_graph ks_graph_new(ks_type tp);

/* Adds a dictionary of nodes to the graph
 */
KS_API bool ks_graph_add_nodes(ks_graph self, ks_dict nodes);

/* Add a node to a graph, which must be hashable
 *
 * If 'allow_dup' is given, the a duplicate node is allowed, and the node is
 *   cleared
 */
KS_API bool ks_graph_add_node(ks_graph self, kso node, bool allow_dup);

/* Adds an edge from 'nodeA' to 'nodeB', with a '.val' of 'edge_val'
 *
 * If 'allow_dup' is given, then a duplicate edge is allowed, and the edge is
 *   cleared
 */
KS_API bool ks_graph_add_edge(ks_graph self, kso nodeA, kso nodeB, kso edge_val, bool allow_dup);

/* Calculate whether a graph has a given node
 */
KS_API bool ks_graph_has_node(ks_graph self, kso node, bool* out);

/* Calculate whether a graph has a given edge
 */
KS_API bool ks_graph_has_edge(ks_graph self, kso nodeA, kso nodeB, bool* out);


/* Clear a graph
 */
KS_API void ks_graph_clear(ks_graph self);


/* Types */
KS_API_DATA ks_type
    kst_graph,
    kst_graph_bfs,
    kst_graph_dfs,
    kst_queue,
    kst_queue_iter,
    kst_bitset,
    kst_bitset_iter,
    kst_bst,
    kst_bst_iter
;

#endif /* KSIO_H__ */
