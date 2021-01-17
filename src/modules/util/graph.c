/* util/graph.c - 'util.Graph' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.Graph"


/* C-API */

bool ks_graph_add_node(ks_graph self, kso val) {
    int i = self->n_nodes++;
    self->nodes = ks_zrealloc(self->nodes, sizeof(*self->nodes), self->n_nodes);

    self->nodes[i].n_edges = 0;
    self->nodes[i].edges = NULL;
    
    KS_INCREF(val);
    self->nodes[i].val = val;
    
    return true;
}

bool ks_graph_add_edge(ks_graph self, ks_cint from, ks_cint to, kso val) {
    if (from < 0 || from >= self->n_nodes) {
        KS_THROW(kst_IndexError, "Edge index out of range for '%T' object: %l", self, from);
        return false;
    }
    if (to < 0 || to >= self->n_nodes) {
        KS_THROW(kst_IndexError, "Edge index out of range for '%T' object: %l", self, to);
        return false;
    }

    struct ks_graph_node* node = &self->nodes[from];

    /* Binary search for the position that we should add the edge (since they are sorted) */
    ks_cint l = 0, r = node->n_edges - 1;

    while (l <= r) {
        int mid = (l + r) / 2;

        if (node->edges[mid].to < to) {
            l = mid + 1;
        } else if (node->edges[mid].to > to) {
            r = mid - 1;
        } else {
            /* Already exists, so replace the value */
            KS_DECREF(node->edges[mid].val);
            KS_INCREF(val);
            node->edges[mid].val = val;
            return true;
        }
    }

    /* Otherwise, it doesn't exist, so insert it */
    int idx_insert = l >= node->n_edges ? node->n_edges : (node->edges[l].to > to ? l : l + 1);

    node->edges = ks_zrealloc(node->edges, sizeof(*node->edges), ++node->n_edges);

    /* Scoot over */
    int i;
    for (i = node->n_edges - 1; i > idx_insert; --i) {
        node->edges[i] = node->edges[i - 1];
    }

    /* Set the node inserting at */
    node->edges[i].to = to;
    KS_INCREF(val);
    node->edges[i].val = val;
    
    return true;
}

void ks_graph_clear(ks_graph self) {
    ks_cint i, j;
    for (i = 0; i < self->n_nodes; ++i) {
        KS_DECREF(self->nodes[i].val);
        for (j = 0; j < self->nodes[i].n_edges; ++j) {
            KS_DECREF(self->nodes[i].edges[j].val);
        }
        ks_free(self->nodes[i].edges);
    }

    self->n_nodes = 0;
}


/* Type Functions */

static KS_TFUNC(T, init) {
    ks_graph self;
    kso nodes = KSO_NONE;
    kso edges = KSO_NONE;
    KS_ARGS("self:* ?nodes ?edges", &self, ksutilt_Graph, &nodes, &edges);

    ks_graph_clear(self);


    if (kso_issub(nodes->type, ksutilt_Graph) && edges == KSO_NONE) {
        ks_cint i, j;
        for (i = 0; i < ((ks_graph)nodes)->n_nodes; ++i) {
            ks_graph_add_node(self, ((ks_graph)nodes)->nodes[i].val);
        }
        for (i = 0; i < ((ks_graph)nodes)->n_nodes; ++i) {
            for (j = 0; j < ((ks_graph)nodes)->nodes[i].n_edges; ++j) {
                ks_graph_add_edge(self, i, ((ks_graph)nodes)->nodes[i].edges[j].to, ((ks_graph)nodes)->nodes[i].edges[j].val);
            }
        }

    } else {
        ks_str t = ks_str_new(-1, "__graph");
        kso gc = kso_getattr(nodes, t);
        KS_DECREF(t);
        if (gc) {

            ks_graph gg = (ks_graph)kso_call(gc, 0, NULL);
            KS_DECREF(gc);
            if (!gg) return NULL;

            kso res = T_init_(2, (kso[]){ (kso)self, (kso)gg });
            KS_DECREF(gg);
            return res;

        } else {
            kso_catch_ignore();

            self->n_nodes = 0;
            self->nodes = NULL;

            if (nodes != KSO_NONE) {
                ks_cit it = ks_cit_make(nodes);
                kso ob;
                while ((ob = ks_cit_next(&it)) != NULL) {
                    ks_graph_add_node(self, ob);
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
                        ks_cint from, to;
                        kso val = KSO_NONE;
                        if (tob->len == 2) {
                            if (kso_get_ci(tob->elems[0], &from) && kso_get_ci(tob->elems[1], &to)) {

                            } else {
                                it.exc = true;
                            }
                        } else if (tob->len == 3) {
                            if (kso_get_ci(tob->elems[0], &from) && kso_get_ci(tob->elems[1], &to)) {

                            } else {
                                it.exc = true;
                            }
                            val = tob->elems[2];
                        } else {
                            KS_THROW(kst_SizeError, "Edge initializers must be either '(from, to)' or '(from, to, val)', but got one of length %i", (int)tob->len);
                            it.exc = true;
                        }
                        if (!it.exc) {
                            it.exc = !ks_graph_add_edge(self, from, to, val);
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
    KS_ARGS("self:*", &self, ksutilt_Graph);

    ks_cint i, j;
    for (i = 0; i < self->n_nodes; ++i) {
        for (j = 0; j < self->nodes[i].n_edges; ++j) {
            KS_DECREF(self->nodes[i].edges[j].val);
        }
        ks_free(self->nodes[i].edges);
        KS_DECREF(self->nodes[i].val);
    }

    ks_free(self->nodes);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, repr) {
    ks_graph self;
    KS_ARGS("self:*", &self, ksutilt_Graph);

    ksio_StringIO sio = ksio_StringIO_new();
    ksio_add((ksio_BaseIO)sio, "%T(", self);

    if (kso_inrepr((kso)self)) {
        ksio_add((ksio_BaseIO)sio, "%T(", KS_REPR_SELF);
    } else {
        ks_cint i, j, ct;
        ksio_add((ksio_BaseIO)sio, "[");
        for (i = 0; i < self->n_nodes; ++i) {
            if (i > 0) ksio_add((ksio_BaseIO)sio, ", ");
            ksio_add((ksio_BaseIO)sio, "%R", self->nodes[i].val);
        }
        
        ksio_add((ksio_BaseIO)sio, "], [");
        for (ct = i = 0; i < self->n_nodes; ++i) {
            for (j = 0; j < self->nodes[i].n_edges; ++j) {
                if (ct++ > 0) ksio_add((ksio_BaseIO)sio, ", ");
                kso val = self->nodes[i].edges[j].val;
                if (val == KSO_NONE) {
                    ksio_add((ksio_BaseIO)sio, "(%l, %l)", i, self->nodes[i].edges[j].to);
                } else {
                    ksio_add((ksio_BaseIO)sio, "(%l, %l, %R)", i, self->nodes[i].edges[j].to, val);
                }
            }
        }
        ksio_add((ksio_BaseIO)sio, "]");
    }

    ksio_add((ksio_BaseIO)sio, ")");
    return (kso)ksio_StringIO_getf(sio);
}

static KS_TFUNC(T, bool) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    return KSO_BOOL(self->len > 0);
}

static KS_TFUNC(T, eq) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, kst_list) && kso_issub(R->type, kst_list)) {
        if (L == R) return KSO_TRUE;
        ks_list lL = (ks_list)L, lR = (ks_list)R;
        if (lL->len != lR->len) return KSO_FALSE;

        ks_cint i;
        for (i = 0; i < lL->len; ++i) {
            kso a = lL->elems[i], b = lR->elems[i];
            if (a == b) {
                /* Do nothing */
            } else {
                bool eq;
                if (!kso_eq(a, b, &eq)) return NULL;
                if (!eq) return KSO_FALSE;
            }
        }

        return KSO_TRUE;
    }

    return KSO_UNDEFINED;
}
static KS_TFUNC(T, contains) {
    ks_list self;
    kso obj;
    KS_ARGS("self:* obj", &self, kst_list, &obj);

    ks_size_t i;
    for (i = 0; i < self->len; ++i) {
        kso ob = self->elems[i];
        if (ob == obj) return KSO_TRUE;

        bool is_eq;
        if (!kso_eq(obj, ob, &is_eq)) return NULL;
        else if (is_eq) return KSO_TRUE;
    }

    return KSO_FALSE;
}

static KS_TFUNC(T, getitem) {
    ks_list self;
    kso idx;
    KS_ARGS("self:* idx", &self, kst_list, &idx);

    if (kso_is_int(idx)) {
        ks_cint idx_c;
        if (!kso_get_ci(idx, &idx_c)) return NULL;

        if (idx_c < 0) idx_c += self->len;
        if (idx_c >= 0 && idx_c < self->len) {
            return KS_NEWREF(self->elems[idx_c]);
        }

    } else {
        KS_THROW(kst_ArgError, "'idx' should be 'int' or 'slice', but got '%T'", idx);
        return NULL;
    }

    KS_THROW_INDEX(self, idx);
    return NULL;
}
static KS_TFUNC(T, len) {
    ks_list self;
    KS_ARGS("self:*", &self, kst_list);

    return (kso)ks_int_newu(self->len);
}


static KS_TFUNC(T, dotfile) {
    ks_graph self;
    KS_ARGS("self:*", &self, ksutilt_Graph);

    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add((ksio_BaseIO)sio, "digraph G {\n");

    ks_cint i, j;
    for (i = 0; i < self->n_nodes; ++i) {
        if (self->nodes[i].val == KSO_NONE) {
            ksio_add((ksio_BaseIO)sio, "    %i;\n", (int)i);
        } else {
            ksio_add((ksio_BaseIO)sio, "    %i [label=\"%S\"];\n", (int)i, self->nodes[i].val);
        }
        for (j = 0; j < self->nodes[i].n_edges; ++j) {
            if (self->nodes[i].edges[j].val == KSO_NONE) {
                ksio_add((ksio_BaseIO)sio, "    %i -> %i;\n", (int)i, (int)self->nodes[i].edges[j].to);
            } else {
                ksio_add((ksio_BaseIO)sio, "    %i -> %i [label=\"%S\"];\n", (int)i, (int)self->nodes[i].edges[j].to, self->nodes[i].edges[j].val);
            }
        }
    }

    ksio_add((ksio_BaseIO)sio, "}\n");

    return (kso)ksio_StringIO_getf(sio);
}


/* Export */

static struct ks_type_s tp;
ks_type ksutilt_Graph = &tp;

void _ksi_graph() {
    _ksinit(ksutilt_Graph, kst_object, T_NAME, sizeof(struct ks_graph_s), -1, "Graphs are collections of nodes and edges. Edges connect various nodes, and thus can represent relationships between nodes. Edges are directional, which means they have a 'from' and 'to' node (which may be the same node)\n\n    Bidirectional graphs may be emulated with two edges for each connection\n\n    SEE: https://en.wikipedia.org/wiki/Graph_(abstract_data_type)", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, nodes=none, edges=none)", "")},
        {"__repr",                 ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},

        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__str",                  ksf_wrap(T_repr_, T_NAME ".__str(self)", "")},

        {"__eq",                   ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},
        {"__contains",             ksf_wrap(T_contains_, T_NAME ".__contains(self, obj)", "")},
        {"__getitem",              ksf_wrap(T_getitem_, T_NAME ".__getitem(self, idx)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},

        {"_dotfile",               ksf_wrap(T_dotfile_, T_NAME "._dotfile(self)", "Generate a Graphviz dotfile source\n\n    SEE: https://graphviz.org/")},
    ));

    ksutilt_Graph->i__hash = NULL;

}
