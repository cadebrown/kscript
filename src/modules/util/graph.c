/* util/graph.c - 'util.Graph' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.Graph"


/* C-API */

ks_graph ks_graph_new(ks_type tp) {
    ks_graph self = KSO_NEW(ks_graph, tp);

    self->nodes = ks_dict_new(NULL);

    return self;
}

bool ks_graph_add_nodes(ks_graph self, ks_dict nodes) {
    int i;
    for (i = 0; i < nodes->len_ents; ++i) {
        if (nodes->ents[i].key) {
            if (!ks_dict_set_h(self->nodes, nodes->ents[i].key, nodes->ents[i].hash, nodes->ents[i].val)) {
                return false;
            }
        }
    }
    return true;
}

bool ks_graph_add_node(ks_graph self, kso node, bool allow_dup) {
    ks_dict mem_node = (ks_dict)ks_dict_get(self->nodes, node);
    if (!mem_node) {
        kso_catch_ignore();
        mem_node = ks_dict_new(NULL);
        ks_dict_set(self->nodes, node, (kso)mem_node);
        KS_DECREF(mem_node);
        return true;
    } else if (allow_dup) {
        ks_dict_clear(mem_node);
        KS_DECREF(mem_node);
        return true;
    } else {
        KS_THROW(kst_KeyError, "Node %R already exists", node);
        KS_DECREF(mem_node);
        return NULL;
    }
}
bool ks_graph_add_edge(ks_graph self, kso nodeA, kso nodeB, kso edge_val, bool allow_dup) {
    ks_dict dA = (ks_dict)ks_dict_get(self->nodes, nodeA);
    if (!dA) {
        return false;
    }
    assert(dA->type == kst_dict);

    ks_dict dAB = (ks_dict)ks_dict_get(dA, nodeB);    
    if (!dAB) {
        kso_catch_ignore();
        dAB = ks_dict_new(NULL);
        ks_dict_set_c(dAB, "val", edge_val);
        ks_dict_set(dA, nodeB, (kso)dAB);
        KS_DECREF(dA);
        KS_DECREF(dAB);
        return true;
    } else if (allow_dup) {
        ks_dict_clear(dAB);
        ks_dict_set_c(dAB, "val", edge_val);
        KS_DECREF(dA);
        KS_DECREF(dAB);
        return true;
    } else {
        KS_THROW(kst_KeyError, "Edge %R->%R already exists", nodeA, nodeB);
        KS_DECREF(dA);
        KS_DECREF(dAB);
        return NULL;
    }
}


void ks_graph_clear(ks_graph self) {
    ks_dict_clear(self->nodes);
}


/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    int nargs;
    kso* args;
    KS_ARGS("tp:* *args", &tp, kst_type, &nargs, &args);

    return (kso)ks_graph_new(tp);
}
static KS_TFUNC(T, init) {
    ks_graph self;
    kso nodes = KSO_NONE;
    kso edges = KSO_NONE;
    KS_ARGS("self:* ?nodes ?edges", &self, kst_graph, &nodes, &edges);

    ks_graph_clear(self);

    if (kso_issub(nodes->type, kst_graph) && edges == KSO_NONE) {

        if (!ks_graph_add_nodes(self, ((ks_graph)nodes)->nodes)) {
            return NULL;
        }
    } else if (kso_issub(nodes->type, kst_dict)) {
        if (!ks_graph_add_nodes(self, (ks_dict)nodes)) return NULL;
    } else {

        ks_str t = ks_str_new(-1, "__graph");
        kso gc = kso_getattr(nodes->type, t);
        KS_DECREF(t);
        if (gc) {

            ks_graph gg = (ks_graph)kso_call(gc, 1, &nodes);
            KS_DECREF(gc);
            if (!gg) return NULL;

            if (!ks_graph_add_nodes(self, gg->nodes)) {
                KS_DECREF(gg);
                return NULL;
            }

            KS_DECREF(gg);
            return KSO_NONE;

        } else {
            kso_catch_ignore();

            if (nodes != KSO_NONE) {
                ks_cit it = ks_cit_make(nodes);
                kso ob;
                while ((ob = ks_cit_next(&it)) != NULL) {
                    if (!ks_graph_add_node(self, ob, false)) {
                        it.exc = true;
                    }
                    KS_DECREF(ob);
                }

                ks_cit_done(&it);
                if (it.exc) {
                    return NULL;
                }
            }

            if (edges != KSO_NONE) {
                ks_cit it = ks_cit_make(edges);
                kso ob;
                while ((ob = ks_cit_next(&it)) != NULL) {
                    ks_tuple tob = ks_tuple_newi(ob);
                    if (tob) {
                        kso nodeA, nodeB;
                        kso val = KSO_NONE;
                        if (tob->len == 2) {
                            nodeA = tob->elems[0];
                            nodeB = tob->elems[1];
        
                        } else if (tob->len == 3) {
                            nodeA = tob->elems[0];
                            nodeB = tob->elems[1];
                            val = tob->elems[2];
                        } else {
                            KS_THROW(kst_SizeError, "Edge initializers must be either '(from, to)' or '(from, to, val)', but got one of length %i", (int)tob->len);
                            it.exc = true;
                        }
                        if (!it.exc) {
                            it.exc = !ks_graph_add_edge(self, nodeA, nodeB, val, true);
                        }
                        KS_DECREF(tob);
                    } else {
                        it.exc = true;
                    }

                    KS_DECREF(ob);
                }

                ks_cit_done(&it);
                if (it.exc) {
                    return NULL;
                }
            }
        }

    }

    return KSO_NONE;
}

static KS_TFUNC(T, free) {
    ks_graph self;
    KS_ARGS("self:*", &self, kst_graph);

    KS_DECREF(self->nodes);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, repr) {
    ks_graph self;
    KS_ARGS("self:*", &self, kst_graph);

    return (kso)ks_fmt("%T(%R)", self, self->nodes);
}

static KS_TFUNC(T, bool) {
    ks_graph self;
    KS_ARGS("self:*", &self, kst_list);

    return KSO_BOOL(self->nodes->len_real > 0);
}

static KS_TFUNC(T, dotfile) {
    ks_graph self;
    KS_ARGS("self:*", &self, kst_graph);

    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add(sio, "digraph G {\n");

    ks_cint i, j;
    for (i = 0; i < self->nodes->len_ents; ++i) {
        kso nodeA = self->nodes->ents[i].key;
        ks_dict dA = (ks_dict)self->nodes->ents[i].val;
        if (nodeA) {
            ksio_add(sio, "    \"%S\" [label=\"%S\"];\n", nodeA, nodeA);

            for (j = 0; j < dA->len_ents; ++j) {
                kso nodeB = dA->ents[j].key;
                ks_dict dAB = (ks_dict)dA->ents[j].val;
                if (nodeB) {
                    ksio_add(sio, "    \"%S\" -> \"%S\";\n", nodeA, nodeB);
                }
            }
        }
    }

    ksio_add(sio, "}\n");

    return (kso)ksio_StringIO_getf(sio);
}


/* Export */

static struct ks_type_s tp;
ks_type kst_graph = &tp;

void _ksi_graph() {
    _ksinit(kst_graph, kst_object, T_NAME, sizeof(struct ks_graph_s), -1, "Graphs are collections of nodes and edges. Edges connect various nodes, and thus can represent relationships between nodes. Edges are directional, which means they have a 'from' and 'to' node (which may be the same node)\n\n    Bidirectional graphs may be emulated with two edges for each connection\n\n    SEE: https://en.wikipedia.org/wiki/Graph_(abstract_data_type)", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, *args)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, nodes=none, edges=none)", "")},
        {"__repr",                 ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},

        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__str",                  ksf_wrap(T_repr_, T_NAME ".__str(self)", "")},


        {"_dotfile",               ksf_wrap(T_dotfile_, T_NAME "._dotfile(self)", "Generate a Graphviz dotfile source\n\n    SEE: https://graphviz.org/")},
    ));

    kst_graph->i__hash = NULL;

}
