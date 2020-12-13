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


static void my_mpz_set_ui(mpz_t self, ks_uint v) {
    mpz_import(self, 1, 1, sizeof(v), 0, 0, &v);
}

static void my_mpz_set_ci(mpz_t self, ks_cint v) {
    if (v < 0) {
        my_mpz_set_ui(self, -(v + 1) + 1);
        mpz_neg(self, self);
    } else {
        my_mpz_set_ui(self, v);
    }
}

#endif


/* C-API */

ks_int ks_int_newt(ks_type tp, ks_cint val) {
    ks_int self = KSO_NEW(ks_int, tp);

    mpz_init(self->val);
    my_mpz_set_ci(self->val, val);

    return self;
}
ks_int ks_int_new(ks_cint val) {
    return ks_int_newt(kst_int, val);
}

ks_int ks_int_newu(ks_uint val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    mpz_init(self->val);
    my_mpz_set_ui(self->val, val);

    return self;
}


ks_int ks_int_newft(ks_type tp, ks_cfloat val) {
    val = floor(val);
    if ((ks_cint)val == val) {
        return ks_int_newt(tp, (ks_cint)val);
    } else {
        /* use GMP */
        mpz_t res;
        mpz_init(res);
        mpz_set_d(res, val);
        return ks_int_newznt(tp, res);
    }
}


ks_int ks_int_newf(ks_cfloat val) {
    return ks_int_newft(kst_int, val);
}


ks_int ks_int_newst(ks_type tp, ks_ssize_t sz, const char* src, int base) {
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
    if (base == 0) base = 10;

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
        return NULL;
    }

    /* Final conversion, and eat 'r' */
    if (is_neg) mpz_neg(r, r);
    return ks_int_newzn(r);
}

ks_int ks_int_news(ks_ssize_t sz, const char* src, int base) {
    return ks_int_newst(kst_int, sz, src, base);
}

ks_int ks_int_newz(mpz_t val) {
    ks_int self = KSO_NEW(ks_int, kst_int);

    mpz_init(self->val);
    mpz_set(self->val, val);

    return self;
}

ks_int ks_int_newznt(ks_type tp, mpz_t val) {
    ks_int self = KSO_NEW(ks_int, tp);

    *self->val = *val;

    return self;
}

ks_int ks_int_newzn(mpz_t val) {
    return ks_int_newznt(kst_int, val);
}

int ks_int_cmp(ks_int L, ks_int R) {
    return mpz_cmp(L->val, R->val);
}

int ks_int_cmp_c(ks_int L, ks_cint r) {
    return mpz_cmp_si(L->val, r);
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_int self;
    KS_ARGS("self:*", &self, kst_int);

    mpz_clear(self->val);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    ks_cint base = 0;
    KS_ARGS("tp:* ?obj ?base:cint", &tp, kst_type, &obj, &base);

    if (kso_issub(obj->type, kst_str)) {
        /* String conversion */
        return (kso)ks_int_newst(tp, -1, ((ks_str)obj)->data, base);
    }
    if (_nargs > 2) {
        KS_THROW(kst_ArgError, "'base' can only be given when 'obj' is a 'str' object, but it was a '%T' object", obj);
        return NULL;
    }
    if (obj == KSO_NONE) {
        return (kso)ks_int_newt(tp, 0);
    } else if (kso_issub(obj->type, tp)) {
        return KS_NEWREF(obj);
    } else if (kso_is_float(obj)) {
        ks_cfloat x;
        if (!kso_get_cf(obj, &x)) return NULL;
        return (kso)ks_int_newft(tp, x);
    } else if (kso_is_int(obj)) {
        ks_int v = kso_int(obj);
        if (!v) return NULL;

        if (kso_issub(v->type, tp)) return (kso)v;

        ks_int r = KSO_NEW(ks_int, tp);

        mpz_init(r->val);
        mpz_set(r->val, v->val);

        KS_DECREF(v);

        return (kso)r;
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}

static KS_TFUNC(T, str) {
    ks_int self;
    ks_cint base = 10;
    KS_ARGS("self:* ?base:cint", &self, kst_int, &base);

    /* Allocate a temporary buffer according to GMP */
    ks_size_t mlb = 16 + mpz_sizeinbase(self->val, base);
    char* out = ks_malloc(mlb);
    int i = 0;
    bool is_neg = mpz_cmp_si(self->val, 0) < 0;
    
    if (is_neg) {
        out[i++] = '-';
    }

    if (base == 2) {
        out[i++] = '0';
        out[i++] = 'b';
    } else if (base == 8) {
        out[i++] = '0';
        out[i++] = 'o';
    } else if (base == 16) {
        out[i++] = '0';
        out[i++] = 'x';
    }
    /* For the sign, we need to just replace it afterwards */
    char t = out[i - (is_neg?1:0)];
    mpz_get_str(&out[i - (is_neg?1:0)], base, self->val);
    if (is_neg) out[i - (is_neg?1:0)] = t;

    /* Replace with upper-case letters */
    while (out[i]) {
        if ('a' <= out[i] && out[i] <= 'z') {
            out[i] += 'A' - 'a';
        }
        i++;
    }

    /* Convert to object */
    ks_str res = ks_str_new(-1, out);
    ks_free(out);
    return (kso)res;
}




/* Export */

static struct ks_type_s tp;
ks_type kst_int = &tp;

void _ksi_int() {
    _ksinit(kst_int, kst_number, T_NAME, sizeof(struct ks_int_s), -1, "Integer quanitity, which is a whole number. Can be any positive integer, negative integer, or 0\n\n    Unlike many programming languages, the 'int' type (and subtypes) are not limited to machine register limits, but are only limited to how much memory can be allocated", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(self, obj=none, base=10)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self, base=10)", "")},

        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self, base=10)", "")},
    ));
}
