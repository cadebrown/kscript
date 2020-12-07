/* types/graph.c - 'graph' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "graph"


/* Export */

static struct ks_type_s tp;
ks_type kst_graph = &tp;

void _ksi_graph() {
    _ksinit(kst_graph, kst_object, T_NAME, sizeof(struct ks_graph_s), -1, "Graphs are collections of nodes and edges. Edges connect various nodes, and thus can represent relationships between nodes. Edges are directional, which means they have a 'from' and 'to' node (which may be the same node)\n\n    Bidirectional graphs may be emulated with two edges for each connection\n\n    SEE: https://en.wikipedia.org/wiki/Graph_(abstract_data_type)", KS_IKV(

    ));
}
