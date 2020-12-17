/* types/tuple.c - 'tuple' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "tuple"



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


/* Export */

static struct ks_type_s tp;
ks_type kst_tuple = &tp;

void _ksi_tuple() {
    _ksinit(kst_tuple, kst_object, T_NAME, sizeof(struct ks_tuple_s), -1, "Like 'list', but immutable", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(self, objs=none)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
    ));
}
