/* types/str.c - 'str' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "object"


/* C-API */


/* Type Functions */

static KS_TFUNC(T, hash) {
    kso self;
    KS_ARGS("self", &self);
    /* This should not be called */

    return (kso)ks_int_newu((ks_uint)self);
}

/* Export */

static struct ks_type_s tp;
ks_type kst_object = &tp;

void _ksi_object() {
    _ksinit(kst_object, kst_object, T_NAME, sizeof(struct kso_s), -1, "Base type of all other types in kscript (everything is an object), which is used as a building block for other types", KS_IKV(
        {"__hash",                 ksf_wrap(T_hash_, T_NAME ".__hash(self)", "")},
    ));
}
