/* types/float.c - 'float' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "float"

/* Precision (in digits) for regular and scientific output */
#define F_PREC_REG (KS_CFLOAT_DIG)
#define F_PREC_SCI (KS_CFLOAT_DIG)

/* Absolute value at which numbers are printed in scientific format as opposed to regular */
#define A_SCI_BIG 1.0e10
#define A_SCI_SML 1.0e-10


/* Internals */

/* Returns digit value */
static int I_digv(ks_ucp c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'z') {
        return (c - 'a') + 10;
    } else if (c >= 'A' && c <= 'Z') {
        return (c - 'A') + 10;
    } else {
        /* invalid */
        return -1;
    }
}


/* C-API */

ks_float ks_float_newt(ks_type tp, ks_cfloat val) {
    ks_float self = KSO_NEW(ks_float, tp);

    self->val = val;

    return self;
}
ks_float ks_float_new(ks_cfloat val) {
    return ks_float_newt(kst_float, val);
}


bool ks_cfloat_from_str(const char* str, int sz, ks_cfloat* out) {
    bool nt = sz < 0;
    if (nt) sz = strlen(str);
    const char* o_str = str;
    int o_sz = sz;

    /* Consume '_n' characters */
    #define EAT(_n) do { \
        str += _n; \
        sz -= _n; \
    } while (0)

    /* Strip string */
    char c;
    while ((c = *str) == ' ' || c == '\n' || c == '\t' || c == '\r') EAT(1);
    while (sz > 0 && ((c = str[sz - 1]) == ' ' || c == '\n' || c == '\t' || c == '\r')) sz--;

    /* Take out a sign */
    bool isNeg = *str == '-';
    if (isNeg || *str == '+') EAT(1);

    int base = 0;
    if (sz >= 2 && str[0] == '0') {
        c = str[1];
        /**/ if (c == 'b' || c == 'B') base =  2;
        else if (c == 'o' || c == 'O') base =  8;
        else if (c == 'x' || c == 'X') base = 16;
        else if (c == 'd' || c == 'D') base = 10;
    }

    if (base == 0) base = 10;
    else {
        EAT(2);
    }


    if (base == 10) {
        char* rve = NULL;
        int st = 0;
        if (nt) {
            *out = strtod(str, &rve);
            if (rve != str + sz) {
                KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s' (invalid digits)", base, o_sz, o_str);
                return false;
            }

        } else {
            char* ts = ks_malloc(sz + 1);
            memcpy(ts, str, sz);
            ts[sz] = '\0';
            *out = strtod(ts, &rve);
            ks_free(ts);
            if (rve != ts + sz) {
                KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s' (invalid digits)", base, o_sz, o_str);
                return false;
            }
        }

        if (isNeg) {
            *out = -*out;
        }

        return true;
    }

    /* Parse integral part */
    int i = 0;
    ks_cfloat val = 0;
    while (i < sz && str[i] != '.' && !(base == 10 && (str[i] == 'e' || str[i] == 'E'))) {
        int dig = I_digv(str[i]);
        if (dig < 0 || dig >= base) {
            KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s' (invalid digits)", base, o_sz, o_str);
            return false;
        }

        val = base * val + dig;
        i++;
    }

    if (str[i] == '.') {
        /* Parse fractional part */
        i++;
        ks_cfloat frac = 1.0;

        while (i < sz && !(base == 10 && (str[i] == 'e' || str[i] == 'E'))) {
            int dig = I_digv(str[i]);
            if (dig < 0 || dig >= base) {
                KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s' (invalid digits)", base, o_sz, o_str);
                return false;
            }

            frac /= base;
            val += frac * dig;
            i++;
        }
    }

    if (str[i] == 'e' || str[i] == 'E') {
        /* parse base-10 exponent */
        i++;

        bool is_neg_exp = str[i] == '-';
        if (is_neg_exp || str[i] == '+') i++;

        /* Parse exponent */
        int i_exp = 0;
        int _nec = 0;
        while (i < sz) {
            int cdig = I_digv(str[i]);
            if (cdig < 0 || cdig >= 10) break;

            /* overflow check? */
            i_exp = 10 * i_exp + cdig;
            i++;
            _nec++;
        }
        if (_nec <= 0) {
            KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s' (invalid exponent)", base, o_sz, o_str);
            return false;
        }

        /* range check? */
        /* apply exponent */
        ks_cfloat xscl = pow(10, i_exp);
        if (is_neg_exp) {
            val /= xscl;
        } else {
            val *= xscl;
        }
        /* weird rounding; prefer this for small numbers */
        /*
        if ((i_exp < KSF_DIG / 4 && i_exp > -KSF_DIG / 4)) val *= (double)xscl;
        else val *= xscl;
        */
    }


    if (i != sz) {
        KS_THROW(kst_ValError, "Invalid format for base %i float: '%.*s'", base, o_sz, o_str);
        return false;
    }

    *out = isNeg ? -val : val;
    return true;
}


int ks_cfloat_to_str(char* str, int sz, ks_cfloat val, bool sci, int prec, int base) {

    int i = 0, j, k; /* current position */
    /* Adds a single character */
    #define ADDC(_c) do { \
        if (i < sz) { \
            str[i] = _c; \
        } \
        i++; \
    } while (0)

    #define ADDS(_s) do { \
        char* _cp = _s; \
        while (*_cp) { \
            ADDC(*_cp); \
            _cp++; \
        } \
    } while (0)

    if (val != val) {
        /* 'nan' */
        ADDS("nan");
        return i;
    } else if (val == KS_CFLOAT_INF) {
        /* 'inf' */
        ADDS("inf");
        return i;
    } else if (val == -KS_CFLOAT_INF) {
        /* '-inf' */
        ADDS("-inf");
        return i;
    }

    /* Now, we are working with a real number */
    bool is_neg = val < 0;
    if (is_neg) {
        val = -val;
        ADDC('-');
    }

    /* Handle 0.0 */
    if (val == 0.0) {
        ADDS("0.0");
        return i;
    }

    /* Now, val > 0 */

    /**/ if (base ==  2) ADDS("0b");
    else if (base ==  8) ADDS("0o");
    else if (base == 16) ADDS("0x");

    int sciexp = 0;

    /* extract exponent */
    if (sci) {
        while (val >= base) {
            sciexp++;
            val /= base;
        }
        while (val < 1) {
            sciexp--;
            val *= base;
        }
    }

    /* Handle actual digits, break into integer and floating point type */
    static const char digc[] = "0123456789ABCDEF";

    if (base == 10) {
        char fmt[64];
        int sz_fmt = snprintf(fmt, sizeof(fmt) - 1, "%%.%ilf", prec);
		assert(sz_fmt <= sizeof(fmt) - 1);
		char vs[256];
		int sz_vs = snprintf(vs, sizeof(vs) - 1, fmt, (double)val);
		assert(sz_vs <= sizeof(vs) - 1);
		ADDS(vs);

		//i += strfromd(str+i, sz-i, fmt, val);

    } else {
        int i_num = i;
        ks_cfloat vi;
        ks_cfloat vf = modf(val, &vi);

        /* Integral part */
        do {
            ks_cfloat digf = fmod(vi, base);
            int dig = (int)floor(digf);
            
            vi = (vi - digf) / base;

            ADDC(digc[dig]);

        } while (vi > 0.5);

        /* Reverse digit order */
        if (i < sz) for (j = i_num, k = i - 1; j < k; ++j, --k) {
            char t = str[j];
            str[j] = str[k];
            str[k] = t;
        }

        ADDC('.');

        /* Shift over, and generate fractional part */
        vf *= base;

        /* number of digits */
        int ndl = prec <= 0 ? sz - i : prec;

        do {
            ks_cfloat digf = floor(vf);
            int dig = (((int)digf) % base + base) % base;
            assert(dig >= 0 && dig < base); 
            vf = (vf - digf) * base;

            ADDC(digc[dig]);
        } while (vf > 1e-9 && ndl-- > 0);
    }

    /* Now, remove trailing zeros */
    if (i < sz) {
        while (i > 2 && str[i - 1] == '0' && str[i - 2] != '.') i--;
    }
    /* add sciexp */
    if (sci) {
        ADDC('e');
        ADDC(sciexp >= 0 ? '+' : '-');
        if (sciexp < 0) sciexp = -sciexp;

        j = i;

        /* always exponent in base-10 */

        do {
            int sdig = sciexp % 10;

            ADDC(digc[sdig]);
            sciexp /= 10;

        } while (sciexp > 0);

        /* Reverse digits */
        if (i < sz) for (k = i - 1; j < k; j++, k--) {
            char t = str[j];
            str[j] = str[k];
            str[k] = t;
        }
    }
    return i;
}

int ks_strfromd(char* str, size_t n, char* fmt, ks_cfloat val) {
    int i = 0;
    assert(fmt[i] == '%');
    i++;

    int prec = 6;
    bool is_sci = false;

    int sl = strlen(fmt);
    if (fmt[i] == '.') {
        i++;
        prec = 0;
        while (i < sl && ('0' <= fmt[i] && fmt[i] <= '9')) {
            prec = prec * 10 + (fmt[i] - '0');
            i++;
        }
    }

    return ks_cfloat_to_str(str, n, val, is_sci, prec, 10);
}


bool ks_cfloat_isreg(ks_cfloat x) {
    return x != KS_CFLOAT_INF && x != -KS_CFLOAT_INF && x == x;
}

/* Type functions */


static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    ks_cint base = 0;
    KS_ARGS("tp:* ?obj", &tp, kst_type, &obj);

    if (kso_issub(obj->type, kst_str)) {
        /* String conversion */
        ks_cfloat x;
        if (!ks_cfloat_from_str(((ks_str)obj)->data, ((ks_str)obj)->len_b, &x)) return NULL;

        return (kso)ks_float_newt(tp, x);
    }
    if (_nargs > 2) {
        KS_THROW(kst_ArgError, "'base' can only be given when 'obj' is a 'str' object, but it was a '%T' object", obj);
        return NULL;
    }
    if (obj == KSO_NONE) {
        return (kso)ks_float_newt(tp, 0);
    } else if (kso_issub(obj->type, tp)) {
        return KS_NEWREF(obj);
    } else if (kso_is_float(obj) || kso_is_int(obj)) {
        ks_cfloat x;
        if (!kso_get_cf(obj, &x)) return NULL;
        return (kso)ks_float_newt(tp, x);
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}

static KS_TFUNC(T, isnan) {
    ks_float self;
    KS_ARGS("self:*", &self, kst_float);

    return KSO_BOOL(self->val != self->val);
}

static KS_TFUNC(T, str) {
    ks_float self;
    KS_ARGS("self:*", &self, kst_float);

    /* Determine printing mode */
    ks_cfloat a_v = fabs(self->val);
    bool sci = a_v > 0 && (a_v >= A_SCI_BIG || a_v <= A_SCI_SML);

    char tmp[256];
    
    int sz = ks_cfloat_to_str(tmp, sizeof(tmp) - 1, self->val, sci, sci ? F_PREC_SCI : F_PREC_REG, 10);
    if (sz >= sizeof(tmp) - 1) {
        char* atmp = ks_malloc(sz + 5);
        int asz = ks_cfloat_to_str(atmp, sz+4, self->val, sci, sci ? F_PREC_SCI : F_PREC_REG, 10);
        //assert(sz == asz);
        ks_str res = ks_str_new(asz, atmp);
        ks_free(atmp);
        return (kso)res;
    }

    return (kso)ks_str_new(sz, tmp);
}



/* Export */

static struct ks_type_s tp;
ks_type kst_float = &tp;


ks_float ksg_nan, ksg_inf;

void _ksi_float() {
    kst_float->ob_sz = sizeof(struct ks_float_s);
    _ksinit(kst_float, kst_number, T_NAME, sizeof(struct ks_float_s), -1, "Floating point (real) number, which is a quanitity represented by a 'double' in C\n\n    SEE: https://en.wikipedia.org/wiki/Floating-point_arithmetic", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, obj=non, base=10)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},

        {"isnan",                  ksf_wrap(T_isnan_, T_NAME ".isnan(self)", "Calculate whether a float is NaN (i.e. Not-A-Number)")},

        /* Constants */
        {"EPS",                    (kso)ks_float_new(KS_CFLOAT_EPS)},
        {"MIN",                    (kso)ks_float_new(KS_CFLOAT_MIN)},
        {"MAX",                    (kso)ks_float_new(KS_CFLOAT_MAX)},
        {"DIG",                    (kso)ks_int_new(KS_CFLOAT_DIG)},

    ));

    ksg_nan = ks_float_new(KS_CFLOAT_NAN);
    ksg_inf = ks_float_new(KS_CFLOAT_INF);
}
