/* types/func.c - 'func' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "func"

/* C-API */

kso ksf_wrap(ks_cfunc cfunc, const char* sig, const char* doc) {
    ks_func self = KSO_NEW(ks_func, kst_func);

    self->is_cfunc = true;
    self->cfunc = cfunc;

    return (kso)self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_func = &tp;

void _ksi_func() {
    _ksinit(kst_func, kst_object, T_NAME, sizeof(struct ks_func_s), 0, KS_IKV(

    ));
}
