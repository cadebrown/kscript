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


/* Export */

static struct ks_type_s tp;
ks_type kst_func = &tp;

static struct ks_type_s tpp;
ks_type kst_partial = &tpp;

void _ksi_func() {
    _ksinit(kst_partial, kst_object, TP_NAME, sizeof(struct ks_partial_s), -1, KS_IKV(
        {"__free",                 ksf_wrap(TP_free_, TP_NAME ".__free(self)", "")},

    ));

    _ksinit(kst_func, kst_object, T_NAME, sizeof(struct ks_func_s), offsetof(struct ks_func_s, attr), KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"partial",                KS_NEWREF(kst_partial)},
    ));
}
