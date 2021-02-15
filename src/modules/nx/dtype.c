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
    KS_INCREF(res);

    return res;
}

static nx_dtype make_float(const char* name, const char* namecode, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->namecode = ks_str_new(-1, namecode);
    res->kind = NX_DTYPE_FLOAT;
    res->size = sz;
    KS_INCREF(res);

    return res;
}

static nx_dtype make_complex(const char* name, const char* namecode, int sz) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    res->name = ks_str_new(-1, name);
    res->namecode = ks_str_new(-1, namecode);
    res->kind = NX_DTYPE_COMPLEX;
    res->size = sz;
    KS_INCREF(res);

    return res;
}

nx_dtype nx_dtype_struct(ks_str name, kso members) {
    nx_dtype res = KSO_NEW(nx_dtype, nxt_dtype);

    KS_INCREF(name);
    res->name = name;
    KS_INCREF(name);
    res->name = name;
    res->kind = NX_DTYPE_STRUCT;

    res->size = 0;

    res->s_cstruct.n_members = 0;
    res->s_cstruct.members = NULL;

    ks_cit it = ks_cit_make(members);
    kso ob;
    while ((ob = ks_cit_next(&it)) != NULL) {
        if (!kso_issub(ob->type, kst_tuple)) {
            KS_THROW(kst_Error, "Structure member descriptor expected to be 'tuple', but given '%T' object", ob);
            it.exc = true;
        } else {
            ks_tuple to = (ks_tuple)ob;
            int off = res->size;
            if (to->len == 3) {
                ks_cint v;
                if (!kso_get_ci(to->elems[2], &v)) {
                    it.exc = true;
                }
            } else if (to->len < 2 || to->len > 3) {
                KS_THROW(kst_Error, "Expected either 2 or 3 elements in structure member descriptor, but got %i", (int)to->len);
                it.exc = true;
            }

            if (!it.exc) {
                ks_str new_name = (ks_str)to->elems[0];
                if (!kso_issub(new_name->type, kst_str)) {
                    KS_THROW(kst_Error, "Expected tuples of '(name, dtype)' in structure descriptor, but 'name' was '%T' object (expected 'str')", new_name);
                    it.exc = true;
                } else {
                    nx_dtype new_dtype = (nx_dtype)to->elems[1];
                    if (!kso_issub(new_dtype->type, nxt_dtype)) {
                        KS_THROW(kst_Error, "Expected tuples of '(name, dtype)' in structure descriptor, but 'dtype' was '%T' object (expected 'nx.dtype')", new_dtype);
                        it.exc = true;
                    } else {
                        int ii = res->s_cstruct.n_members++;
                        res->s_cstruct.members = ks_zrealloc(res->s_cstruct.members, sizeof(*res->s_cstruct.members), res->s_cstruct.n_members);
                        res->s_cstruct.members[ii].offset = off;
                        KS_INCREF(new_name);
                        res->s_cstruct.members[ii].name = new_name;
                        KS_INCREF(new_dtype);
                        res->s_cstruct.members[ii].dtype = new_dtype;

                        int new_sz = off + new_dtype->size;
                        if (new_sz > res->size) {
                            res->size = new_sz;
                        }
                    }
                }
            }
        }
        KS_DECREF(ob);
    }

    ks_cit_done(&it);
    if (it.exc) {
        KS_DECREF(res);
        return NULL;
    }

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
static KS_TFUNC(T, scalar) {
    nx_dtype self;
    kso obj;
    KS_ARGS("self:* obj", &self, nxt_dtype, &obj);

    void* res = ks_malloc(self->size);
    if (!nx_enc(self, obj, res)) {
        ks_free(res);
        return NULL;
    }

    nx_array rr = nx_array_newc(nxt_array, res, self, 0, NULL, NULL);
    ks_free(res);

    return (kso)rr;
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

    nxd_F,
    nxd_D,
    nxd_E,
    nxd_Q,

    nxd_cF,
    nxd_cD,
    nxd_cE,
    nxd_cQ
;

void _ksi_nx_dtype() {
    
    _ksinit(nxt_dtype, kst_object, T_NAME, sizeof(struct nx_dtype_s), offsetof(struct nx_dtype_s, attr), "Data type", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__call",                 ksf_wrap(T_call_, T_NAME ".__call(self, obj)", "")},


        {"scalar",                 ksf_wrap(T_scalar_, T_NAME ".scalar(self, obj)", "Creates a scalar from 'obj'")},

    ));
    
    nxd_bl = make_int("nx.bool", "bool", sizeof(nx_bl));
    nxd_s8 = make_int("nx.s8", "s8", sizeof(nx_s8));
    nxd_u8 = make_int("nx.u8", "u8", sizeof(nx_u8));
    nxd_s16 = make_int("nx.s16", "s16", sizeof(nx_s16));
    nxd_u16 = make_int("nx.u16", "u16", sizeof(nx_u16));
    nxd_s32 = make_int("nx.s32", "s32", sizeof(nx_s32));
    nxd_u32 = make_int("nx.u32", "u32", sizeof(nx_u32));
    nxd_s64 = make_int("nx.s64", "s64", sizeof(nx_s64));
    nxd_u64 = make_int("nx.u64", "u64", sizeof(nx_u64));

    nxd_F = make_float("nx.float", "F", sizeof(nx_F));
    nxd_D = make_float("nx.double", "D", sizeof(nx_D));
    nxd_E = make_float("nx.longdouble", "E", sizeof(nx_E));
    nxd_Q = make_float("nx.quad", "Q", sizeof(nx_Q));

    nxd_cF = make_complex("nx.complexfloat", "cF", sizeof(nx_cF));
    nxd_cD = make_complex("nx.complexdouble", "cD", sizeof(nx_cD));
    nxd_cE = make_complex("nx.complexlongdouble", "cE", sizeof(nx_cE));
    nxd_cQ = make_complex("nx.complexquad", "cQ", sizeof(nx_cQ));

    #define LOOP(TYPE, NAME) do { \
        nx_array arv = nx_array_newc(nxt_array, (TYPE[]){ TYPE##MIN }, nxd_##NAME, 0, NULL, NULL); \
        ks_dict_set_c(nxd_##NAME->attr, "MIN", (kso)arv); \
        KS_DECREF(arv); \
        arv = nx_array_newc(nxt_array, (TYPE[]){ TYPE##MAX }, nxd_##NAME, 0, NULL, NULL); \
        ks_dict_set_c(nxd_##NAME->attr, "MAX", (kso)arv); \
        KS_DECREF(arv); \
        arv = nx_array_newc(nxt_array, (TYPE[]){ TYPE##EPS }, nxd_##NAME, 0, NULL, NULL); \
        ks_dict_set_c(nxd_##NAME->attr, "EPS", (kso)arv); \
        KS_DECREF(arv); \
        ks_int arvi = ks_int_new(TYPE##DIG); \
        ks_dict_set_c(nxd_##NAME->attr, "DIG", (kso)arvi); \
        KS_DECREF(arvi); \
    } while (0);

    NXT_PASTE_F(LOOP)
    #undef LOOP

}
