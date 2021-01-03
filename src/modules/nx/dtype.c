/* dtype.c - implementation of the 'nx.dtype' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>

#define T_NAME "nx.dtype"

/* Internals */

/* C-API */

nx_dtype dtype_get_cint(const char* name, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->kind = NX_DTYPE_KIND_CINT;
    res->size = sz;

    return res;
}

nx_dtype dtype_get_cfloat(const char* name, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->kind = NX_DTYPE_KIND_CFLOAT;
    res->size = sz;

    return res;
}

nx_dtype dtype_get_ccomplex(const char* name, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->kind = NX_DTYPE_KIND_CCOMPLEX;
    res->size = sz;

    return res;
}

bool nx_dtype_enc(nx_dtype dtype, kso obj, void* out) {
    if (dtype->kind == NX_DTYPE_KIND_CINT) {
        ks_cint val;
        if (!kso_get_ci(obj, &val)) return false;
        #define LOOP(TYPE) do { \
            *(TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_DO_INTS(dtype, LOOP);
        #undef LOOP

    } else if (dtype->kind == NX_DTYPE_KIND_CFLOAT) {
        ks_cfloat val;
        if (!kso_get_cf(obj, &val)) return false;
        #define LOOP(TYPE) do { \
            *(TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_DO_FLOATS(dtype, LOOP);
        #undef LOOP

    } else if (dtype->kind == NX_DTYPE_KIND_CCOMPLEX) {
        ks_ccomplex val;
        if (!kso_get_cc(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            ((TYPE*)out)->re = val.re; \
            ((TYPE*)out)->im = val.im; \
            return true; \
        } while (0);
        NXT_DO_COMPLEXS(dtype, LOOP);
        #undef LOOP
    }

    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}

bool nx_dtype_dec(nx_dtype dtype, void* obj, kso* out) {
    if (dtype->kind == NX_DTYPE_KIND_CINT) {
        #define LOOP(TYPE) do { \
            *out = (kso)ks_int_new(*(TYPE*)obj); \
            return true; \
        } while (0);
        NXT_DO_INTS(dtype, LOOP);
        #undef LOOP

    } else if (dtype->kind == NX_DTYPE_KIND_CFLOAT) {
        #define LOOP(TYPE) do { \
            *out = (kso)ks_float_new(*(TYPE*)obj); \
            return true; \
        } while (0);
        NXT_DO_FLOATS(dtype, LOOP);
        #undef LOOP

    } else if (dtype->kind == NX_DTYPE_KIND_CCOMPLEX) {
        #define LOOP(TYPE) do { \
            *out = (kso)ks_complex_newre(((TYPE*)obj)->re, ((TYPE*)obj)->re); \
            return true; \
        } while (0);
        NXT_DO_COMPLEXS(dtype, LOOP);
        #undef LOOP
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
    nxd_schar,
    nxd_uchar,
    nxd_sshort,
    nxd_ushort,
    nxd_sint,
    nxd_uint,
    nxd_slong,
    nxd_ulong,

    nxd_float,
    nxd_double,
    nxd_longdouble,
    nxd_float128,

    nxd_complexfloat,
    nxd_complexdouble,
    nxd_complexlongdouble,
    nxd_complexfloat128
;



void _ksi_nx_dtype() {
    
    _ksinit(nxt_dtype, kst_object, T_NAME, sizeof(struct nx_dtype_s), -1, "Data type", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr__(self, attr)", "")},

    ));

    
    nxd_uchar = dtype_get_cint("uchar", sizeof(nxc_uchar));

    nxd_float = dtype_get_cfloat("float", sizeof(nxc_float));
    nxd_double = dtype_get_cfloat("double", sizeof(nxc_double));
    nxd_longdouble = dtype_get_cfloat("longdouble", sizeof(nxc_longdouble));
    nxd_float128 = dtype_get_cfloat("float128", sizeof(nxd_float128));
    nxd_complexfloat = dtype_get_ccomplex("complexfloat", sizeof(nxc_complexfloat));
    nxd_complexdouble = dtype_get_ccomplex("complexdouble", sizeof(nxc_complexdouble));
    nxd_complexlongdouble = dtype_get_ccomplex("complexlongdouble", sizeof(nxc_complexlongdouble));
    nxd_complexfloat128 = dtype_get_ccomplex("complexfloat128", sizeof(nxd_complexfloat128));
}



