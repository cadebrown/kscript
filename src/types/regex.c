/* types/regex.c - 'regex' type
 *
 * Regular expressions are fundamental for searches, and so is needed in kscript
 * 
 * A problem with many implementations of regexes are that they are extended to non-regular
 *   languages, with things like recursive regexes, backreferences, and so forth. This makes
 *   recognizing some patterns much easier at the expense of a very difficult simulator/matcher
 *   type (i.e. worst case performance is exponential instead of linear). 
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ucd.h>

#define T_NAME "regex"


/* Internals */

/* Regular Expression syntax:
 *
 * REGEX       : TERM ('|' TERM)*
 * 
 * TERM        : FACTOR*
 * 
 * FACTOR      : ITEM ('*' | '+' | '?' | '{' INT ','? INT? '}')*
 * 
 * ITEM        : GROUP
 *             | SET
 *             | <char>
 *             | '\\' <char>
 *             | '(' REGEX ')'
 *             | '[' (<char> | '['  ']')* ']'
 * 
 * GROUP       : '(' REGEX ')'
 * 
 * SET         : '[' '^'? SET_ITEM* ']'
 * 
 * 
 * SET_ITEM    : CHAR
 *             | '[:' <name> ':]'
 * 
 * CHAR        : [^[](){}.*\!@?$^]
 *             : '\\' .
 *             : '\x' \x{2}
 *             : '\u' \x{4}
 *             : '\U' \x{8}
 *             : '\p{' <name> '}'
 */

/* Internal NFA fragment */
struct frag {
    
    /* State */
    int s;

    /* List of states that must be backpatched */
    int n_l;
    int* l;

};

/* Internal parsing state */
struct state {

    /* NUL-terminated full expression */
    char* expr;

    int n_states;
    struct ks_regex_nfa* states;

    /* Stack of computed fragments */
    int n_stk;
    struct frag** stk;

};

#define NODE(_s) ps->states[(_s)]
static bool parse_regex(struct state* ps);

/* Parsing State Stack Methods */

static void ps_push(struct state* ps, struct frag* frag) {
    int idx = ps->n_stk++;
    ps->stk = ks_zrealloc(ps->stk, sizeof(*ps->stk), ps->n_stk);

    ps->stk[idx] = frag;
}

static struct frag* ps_pop(struct state* ps) {
    return ps->stk[--ps->n_stk];
}

static void append(struct frag* frag, struct frag* from) {
    int i = frag->n_l;
    frag->n_l += from->n_l;
    frag->l = ks_zrealloc(frag->l, sizeof(*frag->l), frag->n_l);
    int j;
    for (j = 0; j < from->n_l; ++j) {
        frag->l[i + j] = from->l[j];
    }
}

static void patch(struct state* ps, struct frag* frag, int s) {
    int i;
    for (i = 0; i < frag->n_l; ++i) {
        if (NODE(frag->l[i]).to0 < 0) {
            NODE(frag->l[i]).to0 = s;
        } else {
            NODE(frag->l[i]).to1 = s;
        }
    }
}

static struct frag* frag_new(struct state* ps, int s) {
    int i = ps->n_stk++;
    ps->stk = ks_zrealloc(ps->stk, sizeof(*ps->stk), ps->n_stk);

    /* Construct new fragment */
    struct frag* res = ps->stk[i] = ks_malloc(sizeof(*res));
    res->s = s;
    res->n_l = 1;
    res->l = ks_zmalloc(sizeof(*res->l), res->n_l);
    res->l[0] = s;

    return res;
}

static void frag_free(struct frag* frag) {
    ks_free(frag->l);
    ks_free(frag);
}


/* Create new fragments */

static int new_state(struct state* ps, int k) {
    int s = ps->n_states++;
    ps->states = ks_zrealloc(ps->states, sizeof(*ps->states), ps->n_states);

    ps->states[s].kind = k;
    ps->states[s].to0 = ps->states[s].to1 = -1;

    NODE(s).kind = k;
    NODE(s).to0 = NODE(s).to1 = -1;
    NODE(s).set.has_byte = NULL;
    NODE(s).set.has_cat = NULL;
    NODE(s).set.n_ext = 0;
    NODE(s).set.ext = NULL;

    return s;
}

static int new_state_set(struct state* ps, bool is_not) {
    int s = new_state(ps, is_not ? KS_REGEX_NFA_NOT : KS_REGEX_NFA_ANY);

    /* Initialize relevant parts */

    NODE(s).set.has_byte = ks_zmalloc(sizeof(bool), 256);
    NODE(s).set.has_cat = ks_zmalloc(sizeof(bool), 64);

    int i;
    for (i = 0; i < 256; ++i) NODE(s).set.has_byte[i] = false;
    for (i = 0; i < 64; ++i) NODE(s).set.has_cat[i] = false;

    return s;
}

static void free_nfa(int n_nfa, struct ks_regex_nfa* nfa) {
    int i;
    for (i = 0; i < n_nfa; ++i) {
        if (nfa[i].kind == KS_REGEX_NFA_NOT || nfa[i].kind == KS_REGEX_NFA_ANY) {
            ks_free(nfa[i].set.has_byte);
            ks_free(nfa[i].set.has_cat);
            ks_free(nfa[i].set.ext);
        }
    }
}


static void swapstates(struct state* ps, int u, int v) {
    if (u == v) return;
    assert(0 <= u && u < ps->n_states);
    assert(0 <= v && v < ps->n_states);
    struct ks_regex_nfa tmp = NODE(u);
    NODE(u) = NODE(v);
    NODE(v) = tmp;

    int i;
    for (i = 0; i < ps->n_states; ++i) {
        /**/ if (NODE(i).to0 == u) NODE(i).to0 = v;
        else if (NODE(i).to0 == v) NODE(i).to0 = u;
        /**/ if (NODE(i).to1 == u) NODE(i).to1 = v;
        else if (NODE(i).to1 == v) NODE(i).to1 = u;
    }
}

/* Attempts to parse a single item, and push a fragment on the stack */
static bool parse_item(struct state* ps) {
    int i;
    int s = -1;

    if (*ps->expr == '(') {
        /* Group */
        ps->expr++;

        if (!parse_regex(ps)) return NULL;

        if (*ps->expr != ')') {
            return NULL;
        }
        ps->expr++;
        return true;
    } else if (*ps->expr == '[') {
        /* Character set */
        ps->expr++;
        bool is_not = *ps->expr == '^';
        if (is_not) ps->expr++;

        s = new_state_set(ps, is_not);

        for (i = 0; i < 256; ++i) NODE(s).set.has_byte[i] = false;

        while (*ps->expr && *ps->expr != ']') {
            int c = *ps->expr;
            ps->expr++;
            if (c == '[') {
                /* Handle categories */
                if (*ps->expr != ':') return NULL;
                ps->expr++;

                while (*ps->expr == ' ') ps->expr++;

                if (strncmp(ps->expr, "alnum", 5) == 0) {
                    for (c = 'a'; c <= 'z'; ++c) NODE(s).set.has_byte[c] = true;
                    for (c = 'A'; c <= 'Z'; ++c) NODE(s).set.has_byte[c] = true;
                    for (c = '0'; c <= '9'; ++c) NODE(s).set.has_byte[c] = true;
                    ps->expr += 5;
                } else if (strncmp(ps->expr, "alpha", 5) == 0) {
                    for (c = 'a'; c <= 'z'; ++c) NODE(s).set.has_byte[c] = true;
                    for (c = 'A'; c <= 'Z'; ++c) NODE(s).set.has_byte[c] = true;
                    ps->expr += 5;
                } else if (strncmp(ps->expr, "digit", 5) == 0) {
                    for (c = '0'; c <= '9'; ++c) NODE(s).set.has_byte[c] = true;
                    ps->expr += 5;
                } else {
                    return NULL;
                }

                while (*ps->expr == ' ') ps->expr++;

                if (*ps->expr != ':') return NULL;
                ps->expr++;
                if (*ps->expr != ']') return NULL;
                ps->expr++;


            } else {
                if (c < 0) c += 256;
                NODE(s).set.has_byte[c] = true;
            }

        }

        if (*ps->expr != ']') {
            return NULL;
        }
        ps->expr++;

    } else if (*ps->expr == '\\') {
        ps->expr++;
        char c = *ps->expr;
        ps->expr++;

        /* Allow escapes */
        if (c == 't') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '\t';
        } else if (c == 'r') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '\r';
        } else if (c == 'n') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '\n';
        } else if (c == '.') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '.';
        } else if (c == '(') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '(';
        } else if (c == ')') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = ')';
        } else if (c == '[') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '[';
        } else if (c == ']') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = ']';
        } else if (c == '{') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '{';
        } else if (c == '}') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '}';
        } else if (c == '\\') {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = '\\';
        } else if (c == 's') {
            s = new_state_set(ps, false);
            NODE(s).set.has_byte[' '] = true;
            NODE(s).set.has_byte['\t'] = true;
            NODE(s).set.has_byte['\v'] = true;
            NODE(s).set.has_byte['\f'] = true;
            NODE(s).set.has_byte['\r'] = true;
            NODE(s).set.has_byte['\n'] = true;

            NODE(s).set.has_cat[ksucd_cat_Zs] = true;
        } else if (c == 'w') {
            s = new_state_set(ps, false);
            for (c = 'a'; c <= 'z'; ++c) NODE(s).set.has_byte[c] = true;
            for (c = 'A'; c <= 'Z'; ++c) NODE(s).set.has_byte[c] = true;
            for (c = '0'; c <= '9'; ++c) NODE(s).set.has_byte[c] = true;
            NODE(s).set.has_byte['_'] = true;

            for (i = ksucd_cat_L; i <= ksucd_cat_Lu; ++i) NODE(s).set.has_cat[i] = true;
        } else if (c == 'd') {
            s = new_state_set(ps, false);
            for (c = '0'; c <= '9'; ++c) NODE(s).set.has_byte[c] = true;

            NODE(s).set.has_cat[ksucd_cat_Nd] = true;
        } else {
            s = new_state(ps, KS_REGEX_NFA_UCP);
            NODE(s).ucp = c;
        }

    } else if (*ps->expr == '.') {
        ps->expr++;

        s = new_state_set(ps, true);
        NODE(s).set.has_byte['\n'] = true;
    } else if (*ps->expr == '^') {
        ps->expr++;
        s = new_state(ps, KS_REGEX_NFA_LINESTART);
    } else if (*ps->expr == '$') {
        ps->expr++;
        s = new_state(ps, KS_REGEX_NFA_LINEEND);

    } else {
        s = new_state(ps, KS_REGEX_NFA_UCP);
        NODE(s).ucp = *ps->expr++;
    }

    if (s < 0) return false;
    
    struct frag* f = frag_new(ps, s);
    return true;
}

static bool parse_factor(struct state* ps) {
    int i;
    if (!parse_item(ps)) return false;

    while (*ps->expr) {

        if (*ps->expr == '?') {
            ps->expr++;

            struct frag* u = ps_pop(ps);
            int s = new_state(ps, KS_REGEX_NFA_EPS);
            NODE(s).to1 = u->s;

            struct frag* t = frag_new(ps, s);
            append(t, u);
            frag_free(u);
        } else if (*ps->expr == '*') {
            ps->expr++;

            struct frag* u = ps_pop(ps);
            int s = new_state(ps, KS_REGEX_NFA_EPS);
            NODE(s).to1 = u->s;
            patch(ps, u, s);

            struct frag* t = frag_new(ps, s);
            append(t, u);
            frag_free(u);

        } else if (*ps->expr == '+') {
            ps->expr++;

            struct frag* u = ps_pop(ps);
            int s = new_state(ps, KS_REGEX_NFA_EPS);
            NODE(s).to1 = u->s;
            patch(ps, u, s);

            struct frag* t = frag_new(ps, u->s);
            t->l[0] = s;
            append(t, u);
            frag_free(u);

        } else break;
    }

    return true;
}

static bool parse_term(struct state* ps) {
    if (!parse_factor(ps)) return false;

    while (*ps->expr && *ps->expr != '|' && *ps->expr != ')') {
        if (!parse_factor(ps)) return false;
        
        struct frag* v = ps_pop(ps);
        struct frag* u = ps_pop(ps);
        patch(ps, u, v->s);

        struct frag* f = frag_new(ps, u->s);
        f->n_l = 0;
        append(f, v);

        frag_free(u);
        frag_free(v);
    }

    return true;
}

static bool parse_regex(struct state* ps) {
    if (!parse_term(ps)) return false;

    /* Parse conditionals */
    while (*ps->expr && *ps->expr == '|') {
        ps->expr++;
        if (!parse_term(ps)) return false;

        struct frag* v = ps_pop(ps);
        struct frag* u = ps_pop(ps);

        int s = new_state(ps, KS_REGEX_NFA_EPS);
        NODE(s).to0 = u->s;
        NODE(s).to1 = v->s;

        struct frag* t = frag_new(ps, s);
        t->n_l = 0;

        append(t, u);
        append(t, v);

        frag_free(u);
        frag_free(v);
    }

    return true;
}


/* C-API */

ks_regex ks_regex_newt(ks_type tp, ks_str expr) {

    struct state ps_;
    struct state* ps = &ps_;

    ps->expr = expr->data;

    ps->n_states = 0;
    ps->states = NULL;

    ps->n_stk = 0;
    ps->stk = NULL;

    int i;
    if (!parse_regex(ps)) {
        for (i = 0; i < ps->n_stk; ++i) {
            frag_free(ps->stk[i]);
        }
        ks_free(ps->stk);
        free_nfa(ps->n_states, ps->states);
        ks_free(ps->states);
        KS_THROW(kst_Error, "Invalid regex: %R", expr);
        return NULL;
    }

    if (*ps->expr) {
        for (i = 0; i < ps->n_stk; ++i) {
            frag_free(ps->stk[i]);
        }
        ks_free(ps->stk);
        free_nfa(ps->n_states, ps->states);
        ks_free(ps->states);
        KS_THROW(kst_Error, "Invalid regex: %R", expr);
        return NULL;
    }

    ks_regex self = KSO_NEW(ks_regex, tp);

    KS_INCREF(expr);
    self->expr = expr;


    self->sf = new_state(ps, KS_REGEX_NFA_END);
    struct frag* f = ps_pop(ps);
    self->s0 = f->s;
    patch(ps, f, self->sf);

    frag_free(f);

    for (i = 0; i < ps->n_stk; ++i) {
        frag_free(ps->stk[i]);
    }
    ks_free(ps->stk);

    /* Now, swap states for more reasonable indices */
    int new_s0 = 0, new_sf = ps->n_states - 1;
    swapstates(ps, new_s0, self->s0);
    swapstates(ps, new_sf, self->sf);
    self->s0 = new_s0;
    self->sf = new_sf;

    self->n_states = ps->n_states;
    self->states = ps->states;
    return self;
}

ks_regex ks_regex_new(ks_str expr) {
    return ks_regex_newt(kst_regex, expr);
}




/* High level interface */

bool ks_regex_exact(ks_regex self, ks_str str) {
    ks_regex_sim0 sim;
    ks_regex_sim0_init(&sim, self->n_states, self->states);

    int i;

    /* Start at the initial state */
    ks_regex_sim0_addcur(&sim, self->s0);

    ks_regex_sim0_step_linestart(&sim);


    /* Step through input */
    const char* p = str->data;
    while (*p) {
        ks_regex_sim0_step(&sim, *p);
        p++;
    }

    /* Check if a valid end state */
    ks_regex_sim0_step_lineend(&sim);

    bool res = sim.cur[self->sf];
    ks_regex_sim0_free(&sim);
    return res;
}

bool ks_regex_matches(ks_regex self, ks_str str) {
    ks_regex_sim0 sim;
    ks_regex_sim0_init(&sim, self->n_states, self->states);

    int i;
    ks_regex_sim0_addcur(&sim, self->s0);
    ks_regex_sim0_step_linestart(&sim);


    /* Step through input */
    const char* p = str->data;
    while (*p) {
        ks_regex_sim0_step(&sim, *p);
        if (sim.cur[self->sf]) {
            ks_regex_sim0_free(&sim);
            return true;
        }
        p++;
        ks_regex_sim0_addcur(&sim, self->s0);
    }
    
    ks_regex_sim0_addcur(&sim, self->s0);
    ks_regex_sim0_step_lineend(&sim);

    bool res = sim.cur[self->sf];
    ks_regex_sim0_free(&sim);
    return res;
}



/* sim0 */


void ks_regex_sim0_init(ks_regex_sim0* sim, int n_states, struct ks_regex_nfa* states) {
    sim->n_states = n_states;
    sim->states = states;

    sim->cur = ks_zmalloc(sizeof(*sim->cur), n_states);
    sim->next = ks_zmalloc(sizeof(*sim->next), n_states);

    int i;
    for (i = 0; i < n_states; ++i) sim->cur[i] = sim->next[i] = false;

}

void ks_regex_sim0_free(ks_regex_sim0* sim) {
    ks_free(sim->cur);
    ks_free(sim->next);
}

static void add_state(ks_regex_sim0* sim, bool* ptr, int s) {
    if (s < 0 || ptr[s]) return;
    ptr[s] = true;
    if (sim->states[s].kind == KS_REGEX_NFA_EPS) {
        add_state(sim, ptr, sim->states[s].to0);
        add_state(sim, ptr, sim->states[s].to1);
        return;
    }
}

void ks_regex_sim0_addcur(ks_regex_sim0* sim, int s) {
    add_state(sim, sim->cur, s);
}
void ks_regex_sim0_addnext(ks_regex_sim0* sim, int s) {
    add_state(sim, sim->next, s);
}

int ks_regex_sim0_step_linestart(ks_regex_sim0* sim) {
    int i, j, ct = 0;
    for (i = 0; i < sim->n_states; ++i) sim->next[i] = sim->cur[i];
    for (i = 0; i < sim->n_states; ++i) {
        if (sim->cur[i]) {
            struct ks_regex_nfa* s = &sim->states[i];
            bool good = false;

            /* Check classes */
            if (s->kind == KS_REGEX_NFA_LINESTART) {
                ks_regex_sim0_addnext(sim, s->to0);
                ct++;
            }
        }
    }

    /* Now, swap 'next' and 'cur' */
    void* t = sim->cur;
    sim->cur = sim->next;
    sim->next = t;

    return ct;
}
int ks_regex_sim0_step_lineend(ks_regex_sim0* sim) {
    int i, j, ct = 0;
    for (i = 0; i < sim->n_states; ++i) sim->next[i] = sim->cur[i];
    for (i = 0; i < sim->n_states; ++i) {
        if (sim->cur[i]) {
            struct ks_regex_nfa* s = &sim->states[i];
            bool good = false;

            /* Check classes */
            if (s->kind == KS_REGEX_NFA_LINEEND) {
                ks_regex_sim0_addnext(sim, s->to0);
                ct++;
            }
        }
    }

    /* Now, swap 'next' and 'cur' */
    void* t = sim->cur;
    sim->cur = sim->next;
    sim->next = t;

    return ct;
}

/* Apply a single character to the simulator
 */
int ks_regex_sim0_step(ks_regex_sim0* sim, ks_ucp c) {
    int i, j, ct = 0;
    for (i = 0; i < sim->n_states; ++i) sim->next[i] = false;
    for (i = 0; i < sim->n_states; ++i) {
        if (sim->cur[i]) {
            struct ks_regex_nfa* s = &sim->states[i];
            bool good = false;

            /* Check classes */
            if (s->kind == KS_REGEX_NFA_ANY) {
                if (c < 256) {
                    good = s->set.has_byte[c];
                } else {
                    for (j = 0; j < s->set.n_ext; ++j) {
                        if (c == s->set.ext[j]) {
                            good = true;
                            break;
                        }
                    }
                }

                if (good) {
                    ct++;
                    ks_regex_sim0_addnext(sim, s->to0);
                }
            } else if (s->kind == KS_REGEX_NFA_NOT) {
                if (c < 256) {
                    good = s->set.has_byte[c];
                } else {
                    for (j = 0; j < s->set.n_ext; ++j) {
                        if (c == s->set.ext[j]) {
                            good = true;
                            break;
                        }
                    }
                }

                if (!good) {
                    ct++;
                    ks_regex_sim0_addnext(sim, s->to0);
                }
            } else if (s->kind == KS_REGEX_NFA_UCP) {
                if (s->ucp == c) {
                    ct++;
                    ks_regex_sim0_addnext(sim, s->to0);
                }
            } else if (s->kind == KS_REGEX_NFA_CAT) {
                struct ksucd_info info;
                ks_ucp cp = ksucd_get_info(&info, c);
                if (cp > 0) {
                    if (s->set.has_cat[info.cat_gen]) {
                        ct++;
                        ks_regex_sim0_addnext(sim, s->to0);
                    }
                }
            }
        }
    }

    /* Now, swap 'next' and 'cur' */
    void* t = sim->cur;
    sim->cur = sim->next;
    sim->next = t;

    return ct;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_regex self;
    KS_ARGS("self:*", &self, kst_regex);

    KS_DECREF(self->expr);

    free_nfa(self->n_states, self->states);
    ks_free(self->states);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_str expr;
    KS_ARGS("tp:* expr:*", &tp, kst_type, &expr, kst_str);

    return (kso)ks_regex_newt(tp, expr);
}


static KS_TFUNC(T, str) {
    ks_regex self;
    KS_ARGS("self:*", &self, kst_regex);


    return (kso)ks_fmt("%T(%R)", self, self->expr);
}

static KS_TFUNC(T, graph) {
    ks_regex self;
    KS_ARGS("self:*", &self, kst_regex);

    ks_graph res = (ks_graph)kso_call((kso)kst_graph, 0, NULL);
    if (!res) return NULL;
    assert(kso_issub(res->type, kst_graph));

    ks_cint i, j;
    for (i = 0; i < self->n_states; ++i) {
        ks_str s = ks_fmt("s%i", (int)i);
        ks_graph_add_node(res, (kso)s);
        KS_DECREF(s);
    }

    /* Generate edge transitions */
    for (i = 0; i < self->n_states; ++i) {
        int u = self->states[i].to0, v = self->states[i].to1;
        if (self->states[i].kind == KS_REGEX_NFA_EPS) {
            if (u >= 0) ks_graph_add_edge(res, i, u, KSO_NONE);
            if (v >= 0) ks_graph_add_edge(res, i, v, KSO_NONE);
        } else if (self->states[i].kind == KS_REGEX_NFA_UCP) {
            ks_str c = ks_str_chr(self->states[i].ucp);
            ks_graph_add_edge(res, i, u, (kso)c);
            KS_DECREF(c);
        } else if (self->states[i].kind == KS_REGEX_NFA_ANY || self->states[i].kind == KS_REGEX_NFA_NOT) {
            ks_set r = ks_set_new(0, NULL);

            for (j = 0; j < 256; ++j) {
                if (self->states[i].set.has_byte[j]) {
                    ks_str c = ks_str_chr(j);
                    ks_set_add_h(r, (kso)c, c->v_hash);
                    KS_DECREF(c);
                }
            }

            if (self->states[i].kind == KS_REGEX_NFA_NOT) {
                ks_str ts = ks_str_new(-1, "!");
                ks_tuple tr = ks_tuple_new(2, (kso[]){ (kso)ts, (kso)r  });
                ks_graph_add_edge(res, i, u, (kso)tr);
                KS_DECREF(tr);
            } else {
                ks_graph_add_edge(res, i, u, (kso)r);
            }
            KS_DECREF(r);
        }
    }

    return (kso)res;
}


static KS_TFUNC(T, exact) {
    ks_regex self;
    ks_str src;
    KS_ARGS("self:* src:*", &self, kst_regex, &src, kst_str);

    bool res = ks_regex_exact(self, src);

    return KSO_BOOL(res);
}

static KS_TFUNC(T, matches) {
    ks_regex self;
    ks_str src;
    KS_ARGS("self:* src:*", &self, kst_regex, &src, kst_str);

    bool res = ks_regex_matches(self, src);

    return KSO_BOOL(res);
}


/* Export */

static struct ks_type_s tp;
ks_type kst_regex = &tp;


void _ksi_regex() {

    _ksinit(kst_regex, kst_object, T_NAME, sizeof(struct ks_str_s), -1, "String (i.e. a collection of Unicode characters)\n\n    Indicies, operations, and so forth take character positions, not byte positions", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, expr)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__graph",              ksf_wrap(T_graph_, T_NAME ".__graph(self)", "")},
        {"exact",                ksf_wrap(T_exact_, T_NAME ".exact(self, src)", "Calculate whether the regular expression matches the string exactly")},
        {"matches",              ksf_wrap(T_matches_, T_NAME ".matches(self, src)", "Calculate whether the regular expression matches the string anywhere (this returns a bool)")},

    ));
}
