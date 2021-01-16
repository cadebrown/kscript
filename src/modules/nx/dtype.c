/* dtype.c - implementation of the 'nx.dtype' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define T_NAME "nx.dtype"

/* Internals */

/* C-API */

static nx_dtype make_int(const char* name, const char* namecode, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->namecode = ks_str_new(-1, namecode);
    res->kind = NX_DTYPE_INT;
    res->size = sz;

    return res;
}

static nx_dtype make_float(const char* name, const char* namecode, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->namecode = ks_str_new(-1, namecode);
    res->kind = NX_DTYPE_FLOAT;
    res->size = sz;

    return res;
}

static nx_dtype make_complex(const char* name, const char* namecode, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->namecode = ks_str_new(-1, namecode);
    res->kind = NX_DTYPE_COMPLEX;
    res->size = sz;

    return res;
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

static KS_TFUNC(T, call) {
    nx_dtype self;
    kso obj;
    KS_ARGS("self:* obj", &self, nxt_dtype, &obj);

    return (kso)nx_array_newo(nxt_array, obj, self);
}

/* Export */

static struct ks_type_s tp;
ks_type nxt_dtype = &tp;

nx_dtype
    nxd_bl,
    nxd_s8,
    nxd_u8,
    nxd_s16,
    nxd_u16,
    nxd_s32,
    nxd_u32,
    nxd_s64,
    nxd_u64,

    nxd_H,
    nxd_F,
    nxd_D,
    nxd_L,
    nxd_E,

    nxd_cH,
    nxd_cF,
    nxd_cD,
    nxd_cL,
    nxd_cE
;


void _ksi_nx_dtype() {
    
    _ksinit(nxt_dtype, kst_object, T_NAME, sizeof(struct nx_dtype_s), -1, "Data type", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__call",                 ksf_wrap(T_call_, T_NAME ".__call(self, obj)", "")},

    ));
    
    nxd_bl = make_int("bool", "bool", sizeof(nx_bl));
    nxd_s8 = make_int("s8", "s8", sizeof(nx_s8));
    nxd_u8 = make_int("u8", "u8", sizeof(nx_u8));
    nxd_s16 = make_int("s16", "s16", sizeof(nx_s16));
    nxd_u16 = make_int("u16", "u16", sizeof(nx_u16));
    nxd_s32 = make_int("s32", "s32", sizeof(nx_s32));
    nxd_u32 = make_int("u32", "u32", sizeof(nx_u32));
    nxd_s64 = make_int("s64", "s64", sizeof(nx_s64));
    nxd_u64 = make_int("u64", "u64", sizeof(nx_u64));

    nxd_H = make_float("half", "H", sizeof(nx_H));
    nxd_F = make_float("float", "F", sizeof(nx_F));
    nxd_D = make_float("double", "D", sizeof(nx_D));
    nxd_L = make_float("longdouble", "L", sizeof(nx_L));
    nxd_E = make_float("fp128", "E", sizeof(nx_E));

    nxd_cH = make_complex("complexhalf", "cH", sizeof(nx_cH));
    nxd_cF = make_complex("complexfloat", "cF", sizeof(nx_cF));
    nxd_cD = make_complex("complexdouble", "cD", sizeof(nx_cD));
    nxd_cL = make_complex("complexlongdouble", "cL", sizeof(nx_cL));
    nxd_cE = make_complex("complexfp128", "cE", sizeof(nx_cE));
}
