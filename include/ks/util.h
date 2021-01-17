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
    ksutilt_Queue
;

#endif /* KSIO_H__ */
