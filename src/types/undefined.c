/* types/undefined.c - 'undefined' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "undefined.__type"





/* Type Functions */

static KS_TFUNC(T, free) {
    kso self;
    KS_ARGS("self:*", &self, kst_undefined);

    /* We shouldn't free 'undefined', since it is a global singleton */
    self->refs = KS_REFS_INF;

    return KSO_UNDEFINED;
}

/* Export */

static struct ks_type_s tp;
ks_type kst_undefined = &tp;

static struct kso_s i_undefined;
kso ksg_undefined = &i_undefined;

void _ksi_undefined() {
    
    _ksinit(kst_undefined, kst_object, T_NAME, 0, -1, "Like 'none', but used for operations which should be reserved for another code path\n\n   For example, when overloading operators, 'undefined' can be returned and it allows the other object's type to attempt to compute the result", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
    
    KS_INCREF(kst_undefined);
    ksg_undefined->type = kst_undefined;
    ksg_undefined->refs = 1;

}
