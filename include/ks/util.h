/* ks/io.h - header for the 'io' (input/output) module of kscript
 *
 * Provides general interfaces to file streams, buffer streams, and more
 * 
 * General methods:
 *   s.read(sz=none): Read a given size (default: everything left) message and return it
 *   s.write(msg): Write a (string, bytes, or object) to the stream
 *   close(s): Close a stream
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

/* Structure representing a node in a queue
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
 * Internally, a doubly-linked-list is used
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

    /* Current position in the queue */
    struct ks_queue_item *cur;

}* ks_queue_iter;


/* Represents a single (directed) edge within a dense graph
 *
 * The index of the 'from' node is implicit in which node the edge is stored in
 * 
 */
struct ks_graph_edge {

    /* Index of the node which this edge points towards
     */
    ks_size_t to;

    /* Value stored on the edge (default: none) 
     * A reference is held to this
     */
    kso val;

};

/* Represents a single node within a dense graph
 * 
 * The index which this node refers to is implicit in its location within the 'nodes' array
 * 
 */
struct ks_graph_node {

    /* Number of outgoing edges */
    ks_size_t n_edges;

    /* Array of edges outwards, sorted by 'to' index 
     *
     * This should always be kept sorted, preferably by insertion sort, or if a batch operation is
     *   given, perhaps adding them all and using timsort/mergesort variant to quickly sort them.
     * For example, you could sort just the edges being added, then merge that with the existing list
     *   to give closer to O(N log(N)) time for N insertions (instead of O(N^2))
     */
    struct ks_graph_edge* edges;

    /* Value stored on the node (default: none) 
     * A reference is held to this
     */
    kso val;

};

/* 'util.Graph' - dense, directed graph representing a list of nodes and lists of connections (edges) between them
 *
 * Internally implemented with sorted adjacency lists per node. So, every node holds a list of connections outwards,
 *   but not inwards.
 * 
 * Memory: O(|V|+|E|)
 */
typedef struct ks_graph_s {
    KSO_BASE

    /* number of nodes within the graph */
    ks_size_t n_nodes;

    /* array of nodes, where the index is their ID */
    struct ks_graph_node* nodes;

}* ks_graph;


/** Functions **/


/* Create a new 'util.Bitset' object, empty
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


/* Create a new 'util.Queue' object, with empty
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


/* Add a node to a graph
 */
KS_API bool ks_graph_add_node(ks_graph self, kso val);

/* Add an edge to a graph
 */
KS_API bool ks_graph_add_edge(ks_graph self, ks_cint from, ks_cint to, kso val);

/* Clear a graph
 */
KS_API void ks_graph_clear(ks_graph self);




/* Types */
KS_API_DATA ks_type
    ksutilt_Graph,
    kst_queue,
    kst_queue_iter,
    kst_bitset,
    kst_bitset_iter
;

#endif /* KSIO_H__ */
