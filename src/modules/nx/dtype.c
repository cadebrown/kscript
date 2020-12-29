/* dtype.c - implementation of the 'nx.dtype' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define T_NAME "nx.dtype"

/* Internals */

/* map of builtin scalar types */
static ks_dict I_scalars = NULL;

/* C-API */

nx_dtype nx_dtype_get_cint(int nbits, bool is_signed) {
    ks_str name = ks_fmt("%sint%i", is_signed ? "s" : "u", nbits);
    nx_dtype res = (nx_dtype)ks_dict_get_ih(I_scalars, (kso)name, name->v_hash);
    if (res) {
        KS_DECREF(name);
        return res;
    }

    /* make it for first time */
    res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = name;
    res->kind = NX_DTYPE_KIND_CINT;
    res->size = (nbits + 7) / 8;
    res->s_cint.bits = nbits;
    res->s_cint.sgn = is_signed;

    ks_dict_set(I_scalars, (kso)name, (kso)res);

    return res;
}
nx_dtype nx_dtype_get_cfloat(int nbits) {
    ks_str name = ks_fmt("fp%i", nbits);
    nx_dtype res = (nx_dtype)ks_dict_get_ih(I_scalars, (kso)name, name->v_hash);
    if (res) {
        KS_DECREF(name);
        return res;
    }

    /* make it for first time */
    res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = name;
    res->kind = NX_DTYPE_KIND_CFLOAT;

    res->size = (nbits + 7) / 8;
    res->s_cfloat.nbits = nbits;

    ks_dict_set(I_scalars, (kso)name, (kso)res);

    return res;
}

nx_dtype nx_dtype_get_ccomplex(int nbits) {
    ks_str name = ks_fmt("cplx%i", nbits);
    nx_dtype res = (nx_dtype)ks_dict_get_ih(I_scalars, (kso)name, name->v_hash);
    if (res) {
        KS_DECREF(name);
        return res;
    }

    /* make it for first time */
    res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = name;
    res->kind = NX_DTYPE_KIND_CCOMPLEX;

    res->size = 2 * ((nbits + 7) / 8);
    res->s_ccomplex.nbits = nbits;

    ks_dict_set(I_scalars, (kso)name, (kso)res);

    return res;
}

bool nx_dtype_enc(nx_dtype dtype, kso obj, void* out) {
    if (dtype->kind == NX_DTYPE_KIND_CINT) {
        ks_cint val;
        if (!kso_get_ci(obj, &val)) return false;
        #define INTC(_tp, _s, _b) else if (dtype->s_cint.sgn == _s && dtype->s_cint.bits == _b) { \
            *(_tp*)out = val;\
            return true; \
        }

        if (false) {}
        INTC(ks_uint8_t, 0, 8)
        INTC(ks_uint16_t, 0, 16)
        INTC(ks_uint32_t, 0, 32)
        INTC(ks_uint64_t, 0, 64)
        INTC(ks_sint8_t, 1, 8)
        INTC(ks_sint16_t, 1, 16)
        INTC(ks_sint32_t, 1, 32)
        INTC(ks_sint64_t, 1, 64)

        #undef INTC
        

    } else if (dtype->kind == NX_DTYPE_KIND_CFLOAT) {
        ks_cfloat val;
        if (!kso_get_cf(obj, &val)) return false;
        #define FLTC(_tp) else if (dtype->size == sizeof(_tp)) { \
            *(_tp*)out = val; \
            return true; \
        }

        if (false) {}
        FLTC(ks_cfloat)
        FLTC(float)
        FLTC(double)

        #ifdef KS_HAVE_long_double
        FLTC(long double)
        #endif
        #ifdef KS_HAVE_float128
        //FLTC(float128)
        #endif

        #undef FLTC
    } else if (dtype->kind == NX_DTYPE_KIND_CCOMPLEX) {
        ks_ccomplex val;

        if (!kso_get_cc(obj, &val)) return false;
        #define CPLXC(_tp) else if (dtype->size == 2 * sizeof(_tp)) { \
            ((_tp*)out)[0] = val.re; \
            ((_tp*)out)[1] = val.im; \
            return true; \
        }

        if (false) {}
        CPLXC(ks_cfloat)
        CPLXC(float)
        CPLXC(double)

        #ifdef KS_HAVE_long_double
        CPLXC(long double)
        #endif
        #ifdef KS_HAVE_float128
        //CPLXC(float128)
        #endif

        #undef CPLXC
    }

    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}

bool nx_dtype_dec(nx_dtype dtype, void* obj, kso* out) {
    if (dtype->kind == NX_DTYPE_KIND_CINT) {
        #define INTC(_tp, _s, _b) else if (dtype->s_cint.sgn == _s && dtype->s_cint.bits == _b) { \
            *out = (kso)ks_int_new(*(_tp*)obj);\
            return true; \
        }

        if (false) {}
        INTC(ks_uint8_t, 0, 8)
        INTC(ks_uint16_t, 0, 16)
        INTC(ks_uint32_t, 0, 32)
        INTC(ks_uint64_t, 0, 64)
        INTC(ks_sint8_t, 1, 8)
        INTC(ks_sint16_t, 1, 16)
        INTC(ks_sint32_t, 1, 32)
        INTC(ks_sint64_t, 1, 64)

        #undef INTC

    } else if (dtype->kind == NX_DTYPE_KIND_CFLOAT) {
        

    } else if (dtype->kind == NX_DTYPE_KIND_CCOMPLEX) {
        #define CPLXC(_tp) else if (dtype->size == 2 * sizeof(_tp)) { \
            *out = (kso)ks_complex_newre(((_tp*)obj)[0], ((_tp*)obj)[1]); \
            return true; \
        }

        if (false) {}
        CPLXC(ks_cfloat)
        CPLXC(float)
        CPLXC(double)

        #ifdef KS_HAVE_long_double
        CPLXC(long double)
        #endif
        #ifdef KS_HAVE_float128
        //CPLXC(float128)
        #endif

        #undef CPLXC
    }


    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}



/* Type Functions */

static KS_TFUNC(T, free) {
    nx_dtype self;
    KS_ARGS("self:*", &self, nxt_dtype);

    KS_DECREF(self->name);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    nx_dtype self;
    KS_ARGS("self:*", &self, nxt_dtype);

    return KS_NEWREF(self->name);
}

static KS_TFUNC(T, int) {
    ks_cint bits = 32;
    bool sgn = true;
    KS_ARGS("?bits:cint ?sgn:bool", &bits, &sgn);

    return (kso)nx_dtype_get_cint(bits, sgn);
}

static KS_TFUNC(T, getattr) {
    nx_dtype self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, nxt_dtype, &attr, kst_str);

    if (ks_str_eq_c(attr, "size", 4)) {
        return (kso)ks_int_new(self->size);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}



/* Export */

static struct ks_type_s tp;
ks_type nxt_dtype = &tp;


nx_dtype
    nx_uint8,
    nx_sint8,
    nx_uint16,
    nx_sint16,
    nx_uint32,
    nx_sint32,
    nx_uint64,
    nx_sint64,

    nx_float32,
    nx_float64,
    /*
    nx_float80,
    nx_float128,
    */
    
    nx_complex32,
    nx_complex64

;

nx_dtype
    nxd_float,
    nxd_double,

    nxd_complexfloat,
    nxd_complexdouble
;



void _ksi_nx_dtype() {
    
    I_scalars = ks_dict_new(NULL);

    _ksinit(nxt_dtype, kst_object, T_NAME, sizeof(struct nx_dtype_s), -1, "Data type", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr__(self, attr)", "")},

        {"int",                    ksf_wrap(T_int_, T_NAME ".int(bits=32, sgn=true)", "")},
    ));
}



