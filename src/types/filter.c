/* types/filter.c - 'filter' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "filter"

/* C-API */

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_filter self;
    KS_ARGS("self:*", &self, kst_filter);

    KS_DECREF(self->it);
    KS_DECREF(self->trans);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso fn;
    kso objs;
    KS_ARGS("tp:* fn ?objs", &tp, kst_type, &fn, &objs);
    if (_nargs == 2) {
        objs = fn;
        fn = KSO_NONE;
    }

    kso it = kso_iter(objs);
    if (!it) return NULL;
    ks_filter self = KSO_NEW(ks_filter, tp);
    KS_INCREF(fn);
    self->trans = fn;
    self->it = it;

    return (kso)self;
}

static KS_TFUNC(T, next) {
    ks_filter self;
    KS_ARGS("self:*", &self, kst_filter);

    while (true) {
        kso v = kso_next(self->it);
        if (!v) return NULL;

        kso a = NULL;
        if (self->trans == KSO_NONE) {
            KS_INCREF(v);
            a = v;
        } else {
            a = kso_call(self->trans, 1, &v);
        }
        if (!a) {
            KS_DECREF(v);
            return NULL;
        }

        bool t;
        if (!kso_truthy(a, &t)) {
            KS_DECREF(a);
            KS_DECREF(v);
            return NULL;
        }
        KS_DECREF(a);

        /* Found something which matched the filter */
        if (t) return v;
        KS_DECREF(v);
    }

    assert(false);
    return NULL;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_filter = &tp;

void _ksi_filter() {
    _ksinit(kst_filter, kst_object, T_NAME, sizeof(struct ks_filter_s), -1, "Filter iterator", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, fn, objs)", "")},
        {"__next",               ksf_wrap(T_next_, T_NAME ".__next(self)", "")},
    ));
}
