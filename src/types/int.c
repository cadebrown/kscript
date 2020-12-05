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

    mpz_init(self->val);
    my_mpz_set_ci(self->val, val);

    return self;
}

ks_int ks_int_newu(ks_uint val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    mpz_init(self->val);
    my_mpz_set_ui(self->val, val);

    return self;
}

ks_int ks_int_news(ks_ssize_t sz, const char* src, int base) {
    bool nt = sz < 0;
    if (nt) sz = strlen(src);

    const char* o_src = src;
    ks_ssize_t o_sz = sz;

    char c;

    /* Extract sign information */
    bool is_neg = *src == '-';
    if (is_neg || *src == '+') {
        src += 1;
        sz -= 1;
    }

    /* Take prefix off */
    if (sz > 2 && src[0] == '0') {
        c = src[1];
        int new_base = 0;

        /**/ if (c == 'd' || c == 'D') new_base = 10;
        else if (c == 'x' || c == 'X') new_base = 16;
        else if (c == 'o' || c == 'O') new_base = 8;
        else if (c == 'b' || c == 'B') new_base = 2;

        /* Shift off prefix */
        if (new_base != 0) {
            src += 2;
            sz -= 2;
        }
        if (base == 0) {
            base = new_base;
        }
    }

    /* Ask GMP to parse it */
    mpz_t r;
    mpz_init(r);

    int gmp_err = 0;
    if (nt) {
        /* Already NUL-terminated */
        gmp_err = mpz_set_str(r, src, base);
    } else {
        /* Can't assume it is NUL-terminated */
        char* tmp = ks_malloc(sz + 1);
        memcpy(tmp, src, sz);
        tmp[sz] = '\0';
        gmp_err = mpz_set_str(r, tmp, base);
        ks_free(tmp);
    }

    /* Ensure it was successful */
    if (gmp_err < 0) {
        mpz_clear(r);
        KS_THROW(kst_Error, "Invalid format for base %i int: '%.*s'", base, (int)o_sz, o_src);
    }

    /* Final conversion, and eat 'r' */
    if (is_neg) mpz_neg(r, r);
    return ks_int_newzn(r);
}

ks_int ks_int_newz(mpz_t val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    mpz_init(self->val);
    mpz_set(self->val, val);

    return self;
}

ks_int ks_int_newzn(mpz_t val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    *self->val = *val;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_int self;
    KS_ARGS("self:*", &self, kst_int);

    mpz_clear(self->val);

    KSO_DEL(self);

    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_int = &tp;

void _ksi_int() {
    _ksinit(kst_int, kst_number, T_NAME, sizeof(struct ks_int_s), -1, KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
}
