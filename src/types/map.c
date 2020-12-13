/* types/map.c - 'map' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "map"

/* C-API */

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_map self;
    KS_ARGS("self:*", &self, kst_map);

    KS_DECREF(self->it);
    KS_DECREF(self->trans);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso fn;
    kso objs;
    KS_ARGS("tp:* fn objs", &tp, kst_type, &fn, &objs);

    kso it = kso_iter(objs);
    if (!it) return NULL;
    ks_map self = KSO_NEW(ks_map, tp);
    KS_INCREF(fn);
    self->trans = fn;
    self->it = it;

    return (kso)self;
}

static KS_TFUNC(T, next) {
    ks_map self;
    KS_ARGS("self:*", &self, kst_map);

    kso v = kso_next(self->it);
    if (!v) return NULL;

    kso a = kso_call(self->trans, 1, &v);
    KS_DECREF(v);
    
    return a;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_map = &tp;

void _ksi_map() {
    _ksinit(kst_map, kst_object, T_NAME, sizeof(struct ks_map_s), -1, "Mapping iterator", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, fn, objs)", "")},
        {"__next",               ksf_wrap(T_next_, T_NAME ".__next(self)", "")},
    ));
}
