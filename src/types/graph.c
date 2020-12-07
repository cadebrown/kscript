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
    _ksinit(kst_graph, kst_object, T_NAME, sizeof(struct ks_graph_s), -1, KS_IKV(

    ));
}
