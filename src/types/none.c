/* types/none.c - 'none' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "none.__type"


/* Type Functions */

static KS_TFUNC(T, free) {
    kso self;
    KS_ARGS("self:*", &self, kst_none);

    /* We shouldn't free 'none', since it is a global singleton */
    self->refs = KS_REFS_INF;

    return KSO_NONE;
}

static KS_TFUNC(T, next) {
    kso self;
    KS_ARGS("self:*", &self, kst_none);

    KS_OUTOFITER();
    return NULL;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_none = &tp;

static struct kso_s i_none;
kso ksg_none = &i_none;

void _ksi_none() {
    
    _ksinit(kst_none, kst_object, T_NAME, 0, -1, "None/nil/null are all represented as this type\n\n    Technically, 'null' isn't the best description of the type, since 'none' is a valid object (it is a valid reference), so operations are still defined on 'none', but they are similar enough to consider", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__next",               ksf_wrap(T_next_, T_NAME ".__next(self)", "")},
    ));
    
    KS_INCREF(kst_none);
    ksg_none->type = kst_none;
    ksg_none->refs = 1;

}
