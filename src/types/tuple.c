/* types/tuple.c - 'tuple' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "tuple"
#define TI_NAME T_NAME ".__iter"



/* C-API */

ks_tuple ks_tuple_new(ks_ssize_t len, kso* elems) {
    ks_tuple res = ks_tuple_newn(len, elems);
    ks_size_t i;
    for (i = 0; i < len; ++i) KS_INCREF(elems[i]);

    return res;
}
ks_tuple ks_tuple_newn(ks_ssize_t len, kso* elems) {
    ks_tuple self = KSO_NEW(ks_tuple, kst_tuple);

    self->len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    ks_ssize_t i;
    for (i = 0; i < len; ++i) {
        self->elems[i] = elems[i];
    }

    return self;
}
ks_tuple ks_tuple_newe(ks_ssize_t len) {
    ks_tuple self = KSO_NEW(ks_tuple, kst_tuple);

    self->len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    return self;
}

ks_tuple ks_tuple_newit(ks_type tp, kso objs) {
    ks_tuple res = KSO_NEW(ks_tuple, tp);

    /* Build the tuple */
    res->len = 0;
    res->elems = NULL;
    ks_ssize_t max_len = 0;

    ks_cit it = ks_cit_make(objs);
    kso ob;
    while ((ob = ks_cit_next(&it)) != NULL) {
        ks_ssize_t idx = res->len++;
        if (res->len > max_len) {
            max_len = ks_nextsize(res->len, max_len);
            res->elems = ks_zrealloc(res->elems, sizeof(*res->elems), max_len);
        }

        /* Absorb reference */
        res->elems[idx] = ob;
    }

    ks_cit_done(&it);
    if (it.exc) {
        KS_DECREF(res);
        return NULL;
    }

    return res;
}
ks_tuple ks_tuple_newi(kso objs) {
    if (kso_issub(objs->type, kst_tuple)) return (ks_tuple)KS_NEWREF(objs);

    return ks_tuple_newit(kst_tuple, objs);
}




/* Type Functions */

static KS_TFUNC(T, free) {
    ks_tuple self;
    KS_ARGS("self:*", &self, kst_tuple);

    ks_size_t i;
    for (i = 0; i < self->len; ++i) {
        KS_DECREF(self->elems[i]);
    }
    ks_free(self->elems);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso objs = KSO_NONE;
    KS_ARGS("tp:* ?objs", &tp, kst_type, &objs);

    return (kso)ks_tuple_newit(tp, objs);
}
static KS_TFUNC(T, bool) {
    ks_tuple self;
    KS_ARGS("self:*", &self, kst_tuple);

    return (kso)KSO_BOOL(self->len != 0);
}

static KS_TFUNC(T, len) {
    ks_tuple self;
    KS_ARGS("self:*", &self, kst_tuple);

    return (kso)ks_int_newu(self->len);
}

static KS_TFUNC(T, eq) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, kst_tuple) && kso_issub(R->type, kst_tuple)) {
        if (L == R) return KSO_TRUE;
        ks_tuple lL = (ks_tuple)L, lR = (ks_tuple)R;
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
    ks_tuple self;
    kso elem;
    KS_ARGS("self:* elem", &self, kst_tuple, &elem);

    ks_cint i;
    for (i = 0; i < self->len; ++i) {
        kso ob = self->elems[i];
        if (ob == elem) return KSO_TRUE;
   
        bool eq;
        if (!kso_eq(elem, ob, &eq)) {
            kso_catch_ignore();
        } else if (eq) {
            return KSO_TRUE;
        }
    }

    return KSO_FALSE;
}



/** Iterator **/

static KS_TFUNC(TI, free) {
    ks_tuple_iter self;
    KS_ARGS("self:*", &self, kst_tuple_iter);

    KS_DECREF(self->of);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, new) {
    ks_type tp;
    ks_tuple of;
    KS_ARGS("tp:* of:*", &tp, kst_type, &of, kst_tuple);

    ks_tuple_iter self = KSO_NEW(ks_tuple_iter, tp);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return (kso)self;
}
/* Export */

static struct ks_type_s tp;
ks_type kst_tuple = &tp;


static struct ks_type_s tp_iter;
ks_type kst_tuple_iter = &tp_iter;

void _ksi_tuple() {

    _ksinit(kst_tuple_iter, kst_object, TI_NAME, sizeof(struct ks_tuple_iter_s), -1, "", KS_IKV(
        {"__free",               ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(TI_new_, TI_NAME ".__new(tp, of)", "")},
    ));
    _ksinit(kst_tuple, kst_object, T_NAME, sizeof(struct ks_tuple_s), -1, "Like 'list', but immutable", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(self, objs=none)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__contains",             ksf_wrap(T_contains_, T_NAME ".__contains(self, elem)", "")},
        {"__eq",                   ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"__iter",                 KS_NEWREF(kst_tuple_iter)},

    ));
}
