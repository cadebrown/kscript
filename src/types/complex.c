/* types/complex.c - 'complex' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "complex"

/* Precision (in digits) for regular and scientific output */
#define F_PREC_REG (KS_CFLOAT_DIG)
#define F_PREC_SCI (KS_CFLOAT_DIG)

/* Absolute value at which numbers are printed in scientific format as opposed to regular */
#define A_SCI_BIG 1.0e10
#define A_SCI_SML 1.0e-10


/* C-API */

ks_complex ks_complex_newt(ks_type tp, ks_ccomplex val) {
    ks_complex self = KSO_NEW(ks_complex, tp);

    self->val = val;

    return self;
}
ks_complex ks_complex_new(ks_ccomplex val) {
    return ks_complex_newt(kst_complex, val);
}

ks_complex ks_complex_newre(ks_cfloat re, ks_cfloat im) {
    return ks_complex_new(KS_CC_MAKE(re, im));
}

bool ks_ccomplex_from_str(const char* str, int sz, ks_ccomplex* out) {
    const char* ostr = str;
    int osz = sz;

    /* Reduce parentheses */
    while (sz >= 2 && (str[0] == '(' && str[sz- 1] == ')')) {
        str++;
        sz -= 2;
    }

    if (sz < 1) {
        KS_THROW(kst_Error, "Invalid format for complex number: '%.*s' (no value)", osz, ostr);
        return false;
    }

    /* Strip string */
    char c;
    while (*str == ' ' || *str == '\n' || *str == '\t' || *str == '\r') { str += 1; sz -= 1; }
    while (sz > 0 && ((c = str[sz - 1]) == ' ' || c == '\n' || c == '\t' || c == '\r')) sz--;


    /* Parse out real component */
    int i = 0;
    int pos_0 = i;

    bool has_sign_0 = str[i] == '+' || str[i] == '-';
    if (has_sign_0) i++;

    while (i < sz && str[i] != '+' && str[i] != '-' && str[i] != 'i') {
        if (str[i] == 'e' || str[i] == 'E') {
            i++;
        }
        i++;
    }

    
    ks_cfloat v_re = 0.0, v_im = 0.0;

    if (str[i] == 'i' || str[i] == 'I') i++;

    if (i < sz) {

        /* Parse both parts */
        if (!ks_cfloat_from_str(str+pos_0, i, &v_re)) {
            kso_catch_ignore();
            KS_THROW(kst_Error, "Invalid format for complex number: '%.*s' (invalid real component)", osz, ostr);
            return false;
        }
        if (str[sz - 1] != 'i' && str[sz - 1] != 'I') {
            KS_THROW(kst_Error, "Invalid format for complex number: '%.*s' (imaginary component had no suffix)", osz, ostr);
            return false;
        }
        if (!ks_cfloat_from_str(str+i, sz - i - 1, &v_im)) {
            kso_catch_ignore();
            KS_THROW(kst_Error, "Invalid format for complex number: '%.*s' (invalid imaginary component)", osz, ostr);
            return false;
        }

    } else {
        /* Parse single component */
        bool isImag = str[i - 1] == 'i' || str[i - 1] == 'I';
        if (!ks_cfloat_from_str(str+pos_0, isImag ? i - 1 : i, isImag ? &v_im : &v_re)) {
            kso_catch_ignore();
            KS_THROW(kst_Error, "Invalid format for complex number: '%.*s' (invalid %s component)", osz, ostr, isImag ? "imaginary" : "real");
            return false;
        }
    }

    *out = KS_CC_MAKE(v_re, v_im);
    return true;
}

int ks_ccomplex_to_str(char* str, int sz, ks_ccomplex val, bool sci_re, bool sci_im, int prec_re, int prec_im) {
    int i = 0, msz;
    if (val.re == 0.0) {
        msz = sz - i;
        if (msz < 0) msz = 0;
        int sz_im = ks_cfloat_to_str(str + i, msz, val.im, sci_im, prec_im, 10);
        i += sz_im;
        if (i < sz) str[i++] = 'i';
    } else {
        if (i < sz) str[i++] = '(';
        msz = sz - i;
        if (msz < 0) msz = 0;
        int sz_re = ks_cfloat_to_str(str + i, msz, val.re, sci_re, prec_re, 10);
        i += sz_re;
        if (i < sz) {
            if (val.im >= 0 || val.im != val.im) str[i++] = '+';
        }

        msz = sz - i;
        if (msz < 0) msz = 0;
        int sz_im = ks_cfloat_to_str(str + i, msz, val.im, sci_im, prec_im, 10);
        i += sz_im;
        if (i < sz) str[i++] = 'i';
        if (i < sz) str[i++] = ')';
    }

    return i;
}


/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    kso imag = KSO_NONE;
    KS_ARGS("tp:* ?obj ?imag", &tp, kst_type, &obj, &imag);

    if (imag != KSO_NONE) {
        ks_cfloat re, im;
        if (!kso_get_cf(obj, &re) || !kso_get_cf(imag, &im)) return NULL;

        return (kso)ks_complex_newt(tp, KS_CC_MAKE(re, im));
    }

    if (obj == KSO_NONE) {
        return (kso)ks_complex_newt(tp, KS_CC_MAKE(0, 0));
    } else if (kso_issub(obj->type, tp)) {
        return KS_NEWREF(obj);
    } else if (kso_is_complex(obj) || kso_is_float(obj) || kso_is_int(obj)) {
        ks_ccomplex v;
        if (!kso_get_cc(obj, &v)) return NULL;
        return (kso)ks_complex_newt(tp, v);
    } else if (obj->type == kst_str) {
        ks_ccomplex v;
        if (!ks_ccomplex_from_str(((ks_str)obj)->data, ((ks_str)obj)->len_b, &v)) return NULL;
        return (kso)ks_complex_newt(tp, v);
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}

static KS_TFUNC(T, str) {
    ks_complex self;
    KS_ARGS("self:*", &self, kst_complex);

    /* First, decompose */
    ks_cfloat re = self->val.re, im = self->val.im;

    /* Check which should be printed in scientific mode */
    ks_cfloat a_re = fabs(re), a_im = fabs(im);
    bool sci_re = a_re > 0 && (a_re >= A_SCI_BIG || a_re <= A_SCI_SML);
    bool sci_im = a_im > 0 && (a_im >= A_SCI_BIG || a_im <= A_SCI_SML);

    /* Buffer, current position (this is just a useful case for small numbers,
     *   sometimes the buffer must be reallocated) 
     */
    char tmp[256];
    int rsz = ks_ccomplex_to_str(tmp, sizeof(tmp) - 1, self->val, sci_re, sci_im, sci_re?F_PREC_SCI:F_PREC_REG, sci_im?F_PREC_SCI:F_PREC_REG);

    if (rsz >= sizeof(tmp) - 1) {
        char* atmp = ks_malloc(rsz + 2);
        int new_rsz = ks_ccomplex_to_str(atmp, rsz + 1, self->val, sci_re, sci_im, sci_re?F_PREC_SCI:F_PREC_REG, sci_im?F_PREC_SCI:F_PREC_REG);
        assert(new_rsz == rsz);
        ks_str res = ks_str_new(rsz, atmp);
        ks_free(atmp);
        return (kso)res;
    } else {
        return (kso)ks_str_new(rsz, tmp);
    }
}


static KS_TFUNC(T, getattr) {
    ks_complex self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_complex, &attr, kst_str);

    if (ks_str_eq_c(attr, "real", 4) || ks_str_eq_c(attr, "re", 2)) {
        return (kso)ks_float_new(self->val.re);
    } else if (ks_str_eq_c(attr, "imag", 4) || ks_str_eq_c(attr, "im", 2)) {
        return (kso)ks_float_new(self->val.im);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}
/* Export */

static struct ks_type_s tp;
ks_type kst_complex = &tp;

void _ksi_complex() {
    _ksinit(kst_complex, kst_number, T_NAME, sizeof(struct ks_complex_s), -1, "Complex numbers represent numbers that have two components: a 'real' and 'imaginary'. They are often written as 'a+b*i', where 'i' is the imaginary unit (the principal square root of -1)\n\n    SEE: https://en.wikipedia.org/wiki/Complex_number", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, obj=none, imag=none)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));
}
