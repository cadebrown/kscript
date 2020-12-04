/* types/complex.c - 'complex' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "complex"



/* C-API */

ks_complex ks_complex_new(ks_ccomplex val) {
    ks_complex self = KSO_NEW(ks_complex, kst_complex);

    self->val = val;

    return self;
}

ks_complex ks_complex_newre(ks_cfloat re, ks_cfloat im) {
    return ks_complex_new(KS_CC_MAKE(re, im));
}




/* Export */

static struct ks_type_s tp;
ks_type kst_complex = &tp;

void _ksi_complex() {
    _ksinit(kst_complex, kst_number, T_NAME, sizeof(struct ks_complex_s), -1, NULL);
}
