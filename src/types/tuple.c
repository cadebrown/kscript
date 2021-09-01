/* types/tuple.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_tuple
#define T_TYPE kst_tuple
#define T_NAME "tuple"
#define T_DOC "Immutable collection of other objects, arranged in a linear fashion and indexable"


/* C-API */

ks_tuple ks_tuple_new(ks_type tp, ks_uint len, kso* elems) {
    if (!tp) tp = T_TYPE;

    /* Create new elements array first, so we don't allocate the object too */
    kso* new_elems = ks_malloc(sizeof(*new_elems) * len);
    if (!new_elems) {
        KS_THROW_OOM("While allocating tuple");
        return NULL;
    }

    /* Allocate new object */
    ks_tuple self = KS_NEW(ks_tuple, tp);
    if (!self) {
        ks_free(new_elems);
        return NULL;
    }

    /* Now, fill in fields (and the allocated buffer 'new_elems' should now be freed in the 'free' method) */
    self->len = len;
    self->elems = new_elems;

    return self;
}


/* Methods */

static KS_TFUNC(T, free) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    /* Clear references */
    ks_uint i;
    for (i = 0; i < self->len; ++i) {
        KS_DECREF(self->elems[i]);
    }

    /* Free array of references */
    ks_free(self->elems);

    KS_DEL(self);
    return KS_NONE;
}

static KS_TFUNC(T, int) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return (kso)ks_int_newu(self->len);
}

static KS_TFUNC(T, bool) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return KS_BOOL(self->len > 0);
}


/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_tuple() {

    ks_init_type(T_TYPE, kst_object, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

    ));
}
