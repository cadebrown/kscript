/* Lexer.c - implementation of the regular expression lexer type
 *
 * 
 * @author: Cade Brown <brown.cade@gmail.com>
 */

#include "ks/impl.h"
#include <ks/gram.h>

#define T_NAME "gram.Lexer"

/* Constants/Definitions/Utilities */

/* C-API Interface */

ksgram_Lexer ksgram_Lexer_new(kso src) {
    ksgram_Lexer self = KSO_NEW(ksgram_Lexer, ksgramt_Lexer);


    self->rules = ks_list_new(0, NULL);

    /* Begin with empty queue */
    self->queue = NULL;
    self->n_queue = 0;

    KS_INCREF(src);
    self->src = src;

    self->line = self->pos_b = self->pos_c = self->col = 0;

    self->_sim.n_states = 0;
    self->_sim.states = NULL;
    self->_sim.cur = self->_sim.next = NULL;
    self->_max_sim_len = 0;

    return self;
}

bool ksgram_Lexer_addrule(ksgram_Lexer self, ks_regex regex, kso action) {
    ks_tuple rule = ks_tuple_new(2, (kso[]){ (kso)regex, (kso)action });
    ks_list_push(self->rules, (kso)rule);
    KS_DECREF(rule);
}



/* Generate one token and return the object
 * If the lexer is out of input, it will return NULL and set '*is_out',
 *   but will NOT throw an OutOfIterError
 */
static kso I_token(ksgram_Lexer self, bool* is_out) {
    while (true) {
        /* Maximum match so far (rule index, byte length) */
        int maxi = -1, maxl = 0;
        *is_out = false;

        int i, j;
        for (i = 0; i < self->rules->len; ++i) {
            /* Get the rule we are simulating */
            ks_regex rule = (ks_regex)((ks_tuple)self->rules->elems[i])->elems[0];

            /* Empty the simulator */
            if (rule->n_states > self->_max_sim_len) {
                self->_max_sim_len = rule->n_states;
                self->_sim.cur = ks_zrealloc(self->_sim.cur, sizeof(*self->_sim.cur), self->_max_sim_len);
                self->_sim.next = ks_zrealloc(self->_sim.next, sizeof(*self->_sim.next), self->_max_sim_len);
            }

            self->_sim.n_states = rule->n_states;
            self->_sim.states = rule->states;

            for (j = 0; j < self->_sim.n_states; ++j) self->_sim.cur[j] = self->_sim.next[j] = false;

            self->_sim.cur[rule->s0] = true;

            /* Position from the start of self->queue */
            int pos = 0;

            bool src_out = false;
            while (!src_out) {
                char utf8[5];
                ks_ucp c = 0;
                if (pos >= self->n_queue) {
                    assert (pos == self->n_queue);
                    /* Out of the queue, so we need to read another character */
                    ks_ssize_t numc = 0;
                    ks_ssize_t rsz = ksio_reads(self->src, 1, utf8, &numc);
                    if (rsz < 1 || numc < 1) {
                        kso_catch_ignore();
                        src_out = true;
                        break;
                    }
                    int _n;
                    KS_UCP_FROM_UTF8(c, utf8, _n);

                    /* Reallocate queue buffer, if needed */
                    self->n_queue += 1;
                    if (self->n_queue > self->_max_n_queue) {
                        self->_max_n_queue = ks_nextsize(self->_max_n_queue, self->n_queue);
                        self->queue = ks_realloc(self->queue, self->_max_n_queue);
                    }

                    /* Append to queue */
                    self->queue[pos] = c;
                    pos++;

                } else {
                    /* Read from existing queue */
                    c = self->queue[pos];
                    pos++;
                }


                /* Update state variables */
                /* TODO: add an 'onchar()' method to allow custom handling of newlines */
                if (c == '\n') {
                    self->line++;
                    self->col = 0;
                } else {
                    self->col++;
                }


                /* Now, simulate the NFA */
                if (ks_regex_sim0_step(&self->_sim, c) == 0) break;

                /* Now, check if we have exceeded the maximum length */
                if (pos > maxl) {
                    /* If so, see if we are in a matching state  */
                    if (self->_sim.cur[rule->sf]) {
                        maxi = i;
                        maxl = pos;
                    }
                }
            }
        }

        /* Done with all the rules */
        if (maxi < 0) {
            /* No match was found */
            if (self->n_queue == 0) {
                /* Just out of input, but no error */
                *is_out = true;
                return NULL;
            } else {
                KS_THROW(kst_Error, "Unexpected character(s): '%.*s'", self->n_queue, self->queue);
                return NULL;
            }
        } else {
            /* We've found a match, and we know which is the longest */

            /* Construct a string from the match */
            ks_str val = ks_str_new(maxl, self->queue);

            /* Shift the queue over and consume the tokens */
            self->n_queue -= maxl;
            for (i = 0; i < self->n_queue; ++i) {
                self->queue[i] = self->queue[i + maxl];
            }

            /* Determine the action for the rule that matched */
            kso action = ((ks_tuple)self->rules->elems[maxi])->elems[1];


            if (action == KSO_NONE) {
                /* Skip the token */
            } else if (kso_is_int(action)) {
                /* Assume the action is a token type to return */
                ksgram_Token res = ksgram_Token_new(ksgramt_Token, action, val, 0, 0, 0, 0, 0, 0, 0, 0);
                KS_DECREF(val);
                return (kso)res;
            } else if (kso_is_callable(action)) {
                /* Call the action with the available match */

                kso res = kso_call(action, 1, (kso[]){ (kso)val });
                if (!res) {
                    KS_DECREF(val);
                    return NULL;
                }

                if (res == KSO_NONE) {
                    KS_DECREF(res);
                } else {
                    KS_DECREF(val);
                    return res;
                }
            } else {
                KS_THROW(kst_Error, "Expected either an integral value, or callable for a token action, but got '%T' object", action);
                KS_DECREF(val);
                return NULL;
            }

            /* If it has gotten here, we need to just repeat because we've skipped the token */
            KS_DECREF(val);
        }
    }

    assert(false);
    return NULL;
}


/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    kso rules = KSO_NONE;
    kso src = (kso)ksos_stdin;
    KS_ARGS("tp:* rules ?src", &tp, kst_type, &rules, &src);
    ksgram_Lexer res = ksgram_Lexer_new(src);

    if (rules != KSO_NONE) {
        ks_cit it = ks_cit_make(rules);
        kso ob;
        while ((ob = ks_cit_next(&it)) != NULL) {
            /* Attempt to parse rule here */
            if (kso_issub(ob->type, kst_tuple)) {
                ks_tuple r = (ks_tuple)ob;
                if (r->len == 2) {

                    ks_regex regex = NULL;
                    if (kso_issub(r->elems[0]->type, kst_regex)) {
                        regex = (ks_regex)r->elems[0];
                        KS_INCREF(regex);
                    } else if (kso_issub(r->elems[0]->type, kst_str)) {
                        regex = (ks_regex)ks_regex_newlit((ks_str)r->elems[0]);
                    } else {
                        KS_THROW(kst_Error, "Item #0 in (regex, action) must either be a 'Regex', or a 'str' object, but got '%T'", r->elems[0]);
                    }
                    if (!regex) {
                        it.exc = true;
                    } else {
                        /* Add rule to lexer */
                        ksgram_Lexer_addrule(res, regex, r->elems[1]);
                        KS_DECREF(regex);
                    }

                } else {
                    KS_THROW(kst_TypeError, "Tuple for rule had length %i, but expected length 2 (format: (regex, action))", (int)r->len);
                    it.exc = true;
                }
            } else {
                KS_THROW(kst_TypeError, "Unexpected type for rule, expected a tuple (of '(regex, action)'), but got '%T'", ob);
                it.exc = true;
            }

            KS_DECREF(ob);
        }

        ks_cit_done(&it);
        if (it.exc) {
            KS_DECREF(res);
            return NULL;
        }
    }

    return (kso)res;
}


static KS_TFUNC(T, free) {
    ksgram_Lexer self;
    KS_ARGS("self:*", &self, ksgramt_Lexer);

    KS_DECREF(self->rules);
    ks_free(self->queue);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, repr) {
    ksgram_Lexer self;
    KS_ARGS("self:*", &self, ksgramt_Lexer);

    return (kso)ks_fmt("%T(%R, %R)", self, self->rules, self->src);
}

static KS_TFUNC(T, next) {
    ksgram_Lexer self;
    KS_ARGS("self:*", &self, ksgramt_Lexer);

    bool out;
    kso res = I_token(self, &out);
    if (!res) {
        if (out) {
            KS_OUTOFITER();
        }
        return NULL;
    }

    return res;
}

static KS_TFUNC(T, all) {
    ksgram_Lexer self;
    KS_ARGS("self:*", &self, ksgramt_Lexer);

    ks_list res = ks_list_new(0, NULL);
    kso tok;
    bool out;
    while ((tok = I_token(self, &out)) != NULL) {
        ks_list_push(res, tok);
        KS_DECREF(tok);
    }

    if (out) {
        /* This is good; it should be out*/
        return (kso)res;
    } else {
        /* Another error occurred */
        KS_DECREF(res);
        return NULL;
    }
}


/* Export */

static struct ks_type_s tp;
ks_type ksgramt_Lexer = &tp;

void _ksi_gram_Lexer() {

    _ksinit(ksgramt_Lexer, kst_object, T_NAME, sizeof(struct ksgram_Lexer_s), -1, "Lexer", KS_IKV(

        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(self, rules, src=os.stdin)", "")},
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

        {"__repr",                 ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_repr_, T_NAME ".__str(self)", "")},
        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},
        {"all",                    ksf_wrap(T_all_, T_NAME ".all(self)", "")},

    ));

}

