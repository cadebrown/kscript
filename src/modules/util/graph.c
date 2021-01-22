/* util/graph.c - 'util.Graph' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "util.Graph"
#define TB_NAME "util.Graph.bfs"
#define TD_NAME "util.Graph.dfs"

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

    if (!allow_dup) {
        bool g;
        if (!ks_dict_has(dA, nodeB, &g)) {
            KS_DECREF(dA);
            return false;
        }

        if (g) {
            KS_THROW(kst_KeyError, "Edge %R->%R already exists", nodeA, nodeB);
            KS_DECREF(dA);
            return false;
        }
    }

    if (!ks_dict_set(dA, nodeB, edge_val)) {
        KS_DECREF(dA);
        return false;
    }
    KS_DECREF(dA);
    return true;
}


bool ks_graph_has_node(ks_graph self, kso node, bool* out) {
    return ks_dict_has(self->nodes, node, out);
}

bool ks_graph_has_edge(ks_graph self, kso nodeA, kso nodeB, bool* out) {

    ks_dict dA = (ks_dict)ks_dict_get(self->nodes, nodeA);
    if (!dA) return false;

    if (!ks_dict_has(dA, nodeB, out)) {
        KS_DECREF(dA);
        return false;
    }
    KS_DECREF(dA);
    return true;
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
        kso gc = ks_type_get(nodes->type, t);
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

static KS_TFUNC(T, str) {
    ks_graph self;
    KS_ARGS("self:*", &self, kst_graph);
    ksio_StringIO sio = ksio_StringIO_new();
    ksio_add(sio, "%T([", self);

    ks_cint i, j, ct;
    for (ct = i = 0; i < self->nodes->len_ents; ++i) {
        kso nodeA = self->nodes->ents[i].key;
        if (nodeA) {
            if (ct > 0) ksio_add(sio, ", ");
            ksio_add(sio, "%R", nodeA);
            ct++;
        }
    }

    ksio_add(sio, "], [");

    for (ct = i = 0; i < self->nodes->len_ents; ++i) {
        kso nodeA = self->nodes->ents[i].key;
        ks_dict dA = (ks_dict)self->nodes->ents[i].val;
        if (nodeA) {
            for (j = 0; j < dA->len_ents; ++j) {
                kso nodeB = dA->ents[j].key;
                ks_dict dAB = (ks_dict)dA->ents[j].val;
                if (nodeB) {
                    if (ct > 0) ksio_add(sio, ", ");
                    ksio_add(sio, "(%R, %R, %R)", nodeA, nodeB, dAB);
                    ct++;
                }
            }
        }
    }
    ksio_add(sio, "])", self);

    return (kso)ksio_StringIO_getf(sio);
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


static KS_TFUNC(T, getelem) {
    ks_graph self;
    kso u = KSO_NONE, v = KSO_NONE;
    KS_ARGS("self:* u ?v", &self, kst_graph, &u, &v);

    if (_nargs == 2) {
        /* Just get a node dictionary */

        /* Get edge value */
        ks_dict dU = (ks_dict)ks_dict_get(self->nodes, u);
        if (!dU) {
            return NULL;
        }
        return (kso)dU;

    } else {
        /* Get edge value */
        ks_dict dU = (ks_dict)ks_dict_get(self->nodes, u);
        if (!dU) {
            return NULL;
        }
        kso res = ks_dict_get(dU, v);
        KS_DECREF(dU);
        return res;
    }
}


static KS_TFUNC(T, setelem) {
    ks_graph self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, kst_graph, &nargs, &args);

    if (nargs == 2) {
        /* self, u, val */
        KS_THROW(kst_ArgError, "Expected 4 arguments (self, u, v, val) to 'setelem'");
        return NULL;
    } else if (nargs == 3) {
        /* self, u, v, val */
        ks_dict dU = (ks_dict)ks_dict_get(self->nodes, args[0]);
        if (!dU) {
            return NULL;
        }

        if (!ks_dict_set(dU, args[1], args[2])) {
            KS_DECREF(dU);
            return NULL;
        }
        return KSO_NONE;

    } else {
        KS_THROW(kst_ArgError, "Expected 4 arguments (self, u, v, val) to 'setelem'");
        return NULL;
    }
}


/* Breadth-First-Search (BFS) */

static KS_TFUNC(TB, free) {
    ks_graph_bfs self;
    KS_ARGS("self:*", &self, kst_graph_bfs);

    KS_DECREF(self->of);
    KS_DECREF(self->queue);
    KS_DECREF(self->visited);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TB, init) {
    ks_graph_bfs self;
    ks_graph of;
    kso u = NULL;
    KS_ARGS("self:* of:* ?start", &self, kst_graph_bfs, &of, kst_graph, &u);

    KS_INCREF(of);
    self->of = of;

    self->queue = ks_queue_new(kst_queue);
    self->visited = ks_set_new(0, NULL);

    if (u) {
        if (!ks_queue_push(self->queue, u)) {
            return NULL;
        }
    } else {
        ks_cint i;
        for (i = 0; i < of->nodes->len_ents; ++i) {
            if (of->nodes->ents[i].key) {
                if (!ks_queue_push(self->queue, of->nodes->ents[i].key)) {
                    return NULL;
                }
                break;
            }
        }
    }

    return KSO_NONE;
}


static KS_TFUNC(TB, next) {
    ks_graph_bfs self;
    KS_ARGS("self:*", &self, kst_graph_bfs);

    if (ks_queue_empty(self->queue)) {
        KS_OUTOFITER();
        return NULL;
    }

    /* This is the resulting node */
    kso res = ks_queue_pop(self->queue);

    /* Now, add unvisited neighbors */
    ks_dict dU = (ks_dict)ks_dict_get(self->of->nodes, res);
    if (!dU) {
        KS_DECREF(res);
        return NULL;
    }

    ks_cint i;
    for (i = 0; i < dU->len_ents; ++i) {
        if (dU->ents[i].key) {
            /* Check if neighbor has been visited */
            bool g;
            if (!ks_set_has_h(self->visited, dU->ents[i].key, dU->ents[i].hash, &g)) {
                KS_DECREF(res);
                KS_DECREF(dU);
                return NULL;
            }

            if (!g) {
                /* It hasn't been seen, so mark it and then add to the queue */
                if (!ks_set_add_h(self->visited, dU->ents[i].key, dU->ents[i].hash)) {
                    KS_DECREF(res);
                    KS_DECREF(dU);
                    return NULL;
                }
                if (!ks_queue_push(self->queue, dU->ents[i].key)) {
                    KS_DECREF(res);
                    KS_DECREF(dU);
                    return NULL;
                }
            }

        }
    }
    KS_DECREF(dU);
    return res;
}



/* Depth-First-Search (DFS) */

static KS_TFUNC(TD, free) {
    ks_graph_dfs self;
    KS_ARGS("self:*", &self, kst_graph_dfs);

    KS_DECREF(self->of);
    KS_DECREF(self->stk);
    KS_DECREF(self->visited);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TD, init) {
    ks_graph_dfs self;
    ks_graph of;
    kso u = NULL;
    KS_ARGS("self:* of:* ?start", &self, kst_graph_dfs, &of, kst_graph, &u);

    KS_INCREF(of);
    self->of = of;

    self->stk = ks_list_new(0, NULL);
    self->visited = ks_set_new(0, NULL);

    if (u) {
        if (!ks_list_push(self->stk, u)) {
            return NULL;
        }
    } else {
        ks_cint i;
        for (i = 0; i < of->nodes->len_ents; ++i) {
            if (of->nodes->ents[i].key) {
                if (!ks_list_push(self->stk, of->nodes->ents[i].key)) {
                    return NULL;
                }
                break;
            }
        }
    }

    return KSO_NONE;
}

static KS_TFUNC(TD, next) {
    ks_graph_dfs self;
    KS_ARGS("self:*", &self, kst_graph_dfs);

    while (self->stk->len > 0) {
        /* Find top node and see if it is visited yet */
        kso res = ks_list_pop(self->stk);
        bool g;
        if (!ks_set_has(self->visited, res, &g)) {
            KS_DECREF(res);
            return NULL;
        }

        if (g) {
            /* Already been visited, we don't care */
            KS_DECREF(res);
        } else {
            /* Not visited, so, emit this one after adding others to the stack */
            ks_dict dU = (ks_dict)ks_dict_get(self->of->nodes, res);
            if (!dU) {
                KS_DECREF(res);
                return NULL;
            }

            /* It hasn't been seen, so mark it and then add to the stack */
            if (!ks_set_add(self->visited, res)) {
                KS_DECREF(res);
                KS_DECREF(dU);
                return NULL;
            }

            ks_cint i;
            for (i = 0; i < dU->len_ents; ++i) {
                if (dU->ents[i].key) {
                    /* Check if neighbor has been visited */
                    bool g;
                    if (!ks_set_has_h(self->visited, dU->ents[i].key, dU->ents[i].hash, &g)) {
                        KS_DECREF(res);
                        KS_DECREF(dU);
                        return NULL;
                    }

                    if (!g) {
 
                        if (!ks_list_push(self->stk, dU->ents[i].key)) {
                            KS_DECREF(res);
                            KS_DECREF(dU);
                            return NULL;
                        }
                    }
                }
            }
            return res;
        }
    }

    KS_OUTOFITER();
    return NULL;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_graph = &tp;

static struct ks_type_s tpb;
ks_type kst_graph_bfs = &tpb;

static struct ks_type_s tpd;
ks_type kst_graph_dfs = &tpd;

void _ksi_graph() {
    _ksinit(kst_graph_bfs, kst_object, TB_NAME, sizeof(struct ks_graph_bfs_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TB_free_, TB_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TB_init_, TB_NAME ".__init(self, of, start=none)", "")},
        {"__next",                 ksf_wrap(TB_next_, TB_NAME ".__next(self)", "")},
    ));
    _ksinit(kst_graph_dfs, kst_object, TD_NAME, sizeof(struct ks_graph_dfs_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(TD_free_, TD_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TD_init_, TD_NAME ".__init(self, of, start=none)", "")},
        {"__next",                 ksf_wrap(TD_next_, TD_NAME ".__next(self)", "")},
    ));

    _ksinit(kst_graph, kst_object, T_NAME, sizeof(struct ks_graph_s), -1, "Graphs are collections of nodes and edges. Edges connect various nodes, and thus can represent relationships between nodes. Edges are directional, which means they have a 'from' and 'to' node (which may be the same node)\n\n    Bidirectional graphs may be emulated with two edges for each connection\n\n    SEE: https://en.wikipedia.org/wiki/Graph_(abstract_data_type)", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, *args)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, nodes=none, edges=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, *args)", "Use like: '__getelem(self, u)' or '__getelem(self, u, v)'")},
        {"__setelem",              ksf_wrap(T_getelem_, T_NAME ".__setelem(self, *args)", "")},

        {"_dotfile",               ksf_wrap(T_dotfile_, T_NAME "._dotfile(self)", "Generate a Graphviz dotfile source\n\n    SEE: https://graphviz.org/")},
        {"bfs",                    KS_NEWREF(kst_graph_bfs)},
        {"dfs",                    KS_NEWREF(kst_graph_dfs)},
    ));

    kst_graph->i__hash = NULL;
}
