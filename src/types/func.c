/* types/func.c - 'func' type and 'func.partial' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "func"
#define TP_NAME "func.partial"

/* C-API */

kso ksf_wrap(ks_cfunc cfunc, const char* sig, const char* doc) {
    ks_func self = KSO_NEW(ks_func, kst_func);

    self->is_cfunc = true;
    self->cfunc = cfunc;

    ks_ssize_t sz_fullname = 0;
    while (sig[sz_fullname] && sig[sz_fullname] != '(') {
        sz_fullname++;
    }

    ks_ssize_t sz_name = sz_fullname;
    while (sz_name > 0 && sig[sz_name] != '.') {
        sz_name--;
    }

    if (sig[sz_name] == '.') sz_name++;

    ks_dict_merge_ikv(self->attr, KS_IKV(
        {"__sig", (kso)ks_str_new(-1, sig)},

        {"__doc", (kso)ks_str_new(-1, doc)},
        {"__name", (kso)ks_str_new(sz_fullname - sz_name, sig + sz_name)},
        {"__fullname", (kso)ks_str_new(sz_fullname, sig)},

    ));
    
    return (kso)self;
}


ks_func ks_func_new_k(kso bc, ks_tuple args, int n_defa, kso* defa, int vararg_idx, ks_str sig, ks_str doc) {
    ks_func self = KSO_NEW(ks_func, kst_func);
    ks_ssize_t sl = sig->len_b;

    int first_lpar = -1, first_dot = -1;

    int i;
    for (i = 0; i < sl; ++i) {
        if (sig->data[i] == '(' && first_lpar < 0) {
            first_lpar = i;
        } else if (sig->data[i] == '.' && first_lpar < 0) {
            first_dot = i;
        }
    }

    ks_dict_merge_ikv(self->attr, KS_IKV(
        {"__sig", KS_NEWREF(sig)},

        {"__doc", (kso)KS_NEWREF(doc)},
        {"__name", (kso)(first_dot < 0 ? ks_str_new(first_lpar, sig->data) : ks_str_new(first_lpar - first_dot - 1, sig->data + first_dot + 1))},
        {"__fullname", (kso)ks_str_new(first_lpar, sig->data)},
    ));

    self->is_cfunc = false;
    KS_INCREF(bc);
    self->bfunc.bc = bc;

    self->bfunc.n_pars = args->len;
    self->bfunc.n_req = vararg_idx >= 0 ? args->len - 1 : args->len;
    self->bfunc.pars = ks_zmalloc(sizeof(*self->bfunc.pars), self->bfunc.n_pars);

    self->bfunc.vararg_idx = vararg_idx;

    self->bfunc.closure = NULL;

    for (i = 0; i < self->bfunc.n_pars; ++i) {
        KS_INCREF(args->elems[i]);
        self->bfunc.pars[i].name = (ks_str)args->elems[i];
        self->bfunc.pars[i].defa = NULL;
    }

    /* Take off arguments which have a default as a requirement */
    while (self->bfunc.n_req > 0 && self->bfunc.pars[self->bfunc.n_req - 1].defa != NULL) self->bfunc.n_req--;

    return self;
}


void ks_func_setdefa(ks_func self, int n_defa, kso* defa) {
    assert(!self->is_cfunc);
    assert(self->bfunc.vararg_idx < 0);
    assert(n_defa <= self->bfunc.n_pars);

    int i, j;
    for (i = 0; i < self->bfunc.n_pars; ++i) {
        if (self->bfunc.pars[i].defa) KS_DECREF(self->bfunc.pars[i].defa);
        self->bfunc.pars[i].defa = NULL;
    }

    for (i = 0, j = self->bfunc.n_pars - n_defa; i < n_defa; ++i, ++j) {
        KS_INCREF(defa[i]);
        self->bfunc.pars[j].defa = defa[i];
    }

    self->bfunc.n_req = self->bfunc.n_pars;
    /* Take off arguments which have a default as a requirement */
    while (self->bfunc.n_req > 0 && self->bfunc.pars[self->bfunc.n_req - 1].defa != NULL) self->bfunc.n_req--;
}

ks_partial ks_partial_new(kso of, kso arg0) {
    ks_partial self = KSO_NEW(ks_partial, kst_partial);

    KS_INCREF(of);
    self->of = of;

    self->n_args = 1;
    self->args = ks_zmalloc(sizeof(*self->args), self->n_args);

    self->args[0].idx = 0;
    KS_INCREF(arg0);
    self->args[0].val = arg0;

    return self;
}


/* Type functions */

static KS_TFUNC(T, free) {
    ks_func self;
    KS_ARGS("self:*", &self, kst_func);

    if (self->is_cfunc) {

    } else {
        int i;
        for (i = 0; i < self->bfunc.n_pars; ++i) {
            KS_DECREF(self->bfunc.pars[i].name);
            if (self->bfunc.pars[i].defa) KS_DECREF(self->bfunc.pars[i].defa);
        }
        ks_free(self->bfunc.pars);

        KS_DECREF(self->bfunc.bc);

        if (self->bfunc.closure) KS_DECREF(self->bfunc.closure);
    }

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_func self;
    KS_ARGS("self:*", &self, kst_func);

    kso sig = ks_dict_get_h(self->attr, (kso)_ksva__sig, _ksva__sig->v_hash);
    ks_str res = ks_fmt("<%T %R>", self, sig);
    KS_DECREF(sig);

    return (kso)res;
}


/** Partial **/

static KS_TFUNC(TP, free) {
    ks_partial self;
    KS_ARGS("self:*", &self, kst_partial);

    KS_DECREF(self->of);

    ks_size_t i;
    for (i = 0; i < self->n_args; ++i) {
        KS_DECREF(self->args[i].val);
    }

    ks_free(self->args);

    KSO_DEL(self);
    return KSO_NONE;
}


static KS_TFUNC(TP, new) {
    ks_type tp;
    kso of;
    kso args;
    KS_ARGS("tp:* of args", &tp, kst_type, &of, &args);

    ks_partial self = KSO_NEW(ks_partial, tp);

    KS_INCREF(of);
    self->of = of;

    self->n_args = 0;
    self->args = NULL;

    ks_cit it = ks_cit_make(args);
    kso ob;
    while ((ob = ks_cit_next(&it)) != NULL) {
        ks_tuple idxarg = ks_tuple_newi(ob);
        if (idxarg) {
            if (idxarg->len == 2) {
                ks_cint idx;
                if (kso_get_ci(idxarg->elems[0], &idx)) {

                    int i = self->n_args++;
                    self->args = ks_zrealloc(self->args, sizeof(*self->args), self->n_args);
                    self->args[i].idx = idx;
                    KS_INCREF(idxarg->elems[1]);
                    self->args[i].val = idxarg->elems[1];

                } else {
                    it.exc = true;
                }
            } else {
                KS_THROW(kst_ArgError, "'args' should be length-2 sequences containing '(idx, arg)', but got one of length-%i", (int)idxarg->len);
            }

            KS_DECREF(idxarg);
        } else {
            it.exc = true;
        }

        KS_DECREF(ob);
    }

    ks_cit_done(&it);
    if (it.exc) {
        KS_DECREF(self);
        return NULL;
    }

    return (kso)self;
}

static KS_TFUNC(TP, str) {
    ks_partial self;
    KS_ARGS("self:*", &self, kst_partial);

    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add((ksio_BaseIO)sio, "<%T of=%R, args=(", self, self->of);
    int i;
    for (i = 0; i < self->n_args; ++i) {
        if (i > 0) ksio_add((ksio_BaseIO)sio, ", ");
        ksio_add((ksio_BaseIO)sio, "%i: %R", self->args[i].idx, self->args[i].val);
    }

    ksio_add((ksio_BaseIO)sio, ")>");

    return (kso)ksio_StringIO_getf(sio);
}

/* Export */

static struct ks_type_s tp;
ks_type kst_func = &tp;

static struct ks_type_s tpp;
ks_type kst_partial = &tpp;

void _ksi_func() {
    _ksinit(kst_partial, kst_object, TP_NAME, sizeof(struct ks_partial_s), -1, "Partial application of a function, which has some of the arguments filled in, and when called, applies the extra arguments given and combines with the pre-filled ones\n\n    Most of the time, is used as a wrapper to create a member function object, partially filling the member instance\n\n    SEE: https://en.wikipedia.org/wiki/Partial_application", KS_IKV(
        {"__free",                 ksf_wrap(TP_free_, TP_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(TP_new_, TP_NAME ".__new(tp, of, args)", "")},
        {"__repr",                 ksf_wrap(TP_str_, TP_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(TP_str_, TP_NAME ".__str(self)", "")},

    ));

    _ksinit(kst_func, kst_object, T_NAME, sizeof(struct ks_func_s), offsetof(struct ks_func_s, attr), "Builtin function type, which can be called with a various number of arguments\n\n    Internally, it may either wrap a C-style function (implemented using the C-API), or a bytecode function (which is defined in kscript source code). Either way, the API is the same and they can be treated equally", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"partial",                KS_NEWREF(kst_partial)},
    ));
}
