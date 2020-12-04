/* types/int.c - 'int' type
 *
 * TODO: check for weirdness on platforms without 'ks_cint == long' (i.e. Windows)
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "int"


#ifdef KS_INT_GMP


/* Internal routines to handle setting on all platforms */
static void my_mpz_set_ci(mpz_t self, ks_cint v) {
    mpz_import(self, 1, 1, sizeof(v), 0, 0, &v);
}

static void my_mpz_set_ui(mpz_t self, ks_uint v) {
    mpz_import(self, 1, 1, sizeof(v), 0, 0, &v);
}

#endif


/* C-API */

ks_int ks_int_new(ks_cint val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    #ifdef KS_INT_GMP

    mpz_init(self->val);
    my_mpz_set_ci(self->val, val);

    #endif

    return self;
}

ks_int ks_int_newu(ks_uint val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    #ifdef KS_INT_GMP

    mpz_init(self->val);
    my_mpz_set_ui(self->val, val);

    #endif

    return self;
}



/* Export */

static struct ks_type_s tp;
ks_type kst_int = &tp;

void _ksi_int() {
    _ksinit(kst_int, kst_number, T_NAME, sizeof(struct ks_int_s), -1, NULL);
}
