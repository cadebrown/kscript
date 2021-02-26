/* ffi/struct.c - 'ffi' struct types
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define T_NAME "ffi.struct"


ks_type ksffi_struct_make(int nmembers, kso* members) {
    return ks_type_template(ksffit_struct, nmembers, members);
}


/* Return the index of a specific members */
static int my_attridx(ksffi_struct self, ks_str attr) {
    ks_dict members_map = (ks_dict)kso_getattr_c((kso)self->type, "__members_map");
    if (!members_map) {
        return -1;
    }

    kso vv = ks_dict_get(members_map, (kso)attr);
    KS_DECREF(members_map);
    if (!vv) {
        kso_catch_ignore();
        KS_THROW_ATTR(self, attr);
        return -1;
    }

    ks_cint v;
    if (!kso_get_ci(vv, &v)) {
        return -1;
    }

    KS_DECREF(vv);
    return v;
}

/* Type definitions */

static KS_TFUNC(T, new) {
    ks_type tp;
    int nargs;
    kso* args;
    KS_ARGS("tp:* *nargs", &tp, kst_type, &nargs, &args);
    assert(tp->i__template != NULL && "internal error: structure type had no template arguments");

    ksffi_struct self = KSO_NEW(ksffi_struct, tp);

    int nT = tp->i__template->len;
    kso* T = tp->i__template->elems;

    if (nargs > nT) {
        KS_THROW(kst_Error, "Too many arguments given to struct constructor, expected at most %i, but got %i", (int)nT, nargs);
        return NULL;
    }

    /* Convert each one */
    ks_list members = (ks_list)kso_getattr_c((kso)tp, "__members");
    if (!members) {
        return NULL;
    }

    int i;
    for (i = 0; i < members->len && i < nargs; ++i) {
        ks_tuple mem = (ks_tuple)members->elems[i];
        kso obj_tp = mem->elems[1];
        kso obj_off = mem->elems[2];

        ks_cint off;
        if (!kso_get_ci(obj_off, &off)) {
            KS_DECREF(members);
            return NULL;
        }
        if (!ksffi_unwrap((ks_type)obj_tp, args[i], self->val + off)) {
            KS_DECREF(members);
            return NULL;
        }
    }

    KS_DECREF(members);

    return (kso)self;
}

static KS_TFUNC(T, str) {
    ksffi_struct self;
    KS_ARGS("self:*", &self, ksffit_struct);


    /* Convert each one */
    ks_list members = (ks_list)kso_getattr_c((kso)self->type, "__members");
    if (!members) {
        return NULL;
    }

    ksio_StringIO io = ksio_StringIO_new();

    ksio_add(io, "%T(", self);

    int i;
    for (i = 0; i < members->len; ++i) {
        if (i > 0) {
            ksio_add(io, ", ");
        }
        ks_tuple mem = (ks_tuple)members->elems[i];
        kso obj_tp = mem->elems[1];
        kso obj_off = mem->elems[2];

        ks_cint off;
        if (!kso_get_ci(obj_off, &off)) {
            KS_DECREF(io);
            KS_DECREF(members);
            return NULL;
        }
        void* ptr = (void*)((ks_uint)self->val + off);

    if (false) {}

    #define CASE(_tp, _ctp) else if (kso_issub((ks_type)obj_tp, ksffit_##_tp)) { \
        ksio_add(io, "%l", (ks_cint)*(_ctp*)ptr); \
    } \

    KSFFI_DO_I(CASE)
    #undef CASE

    #define CASE(_tp, _ctp) else if (kso_issub((ks_type)obj_tp, ksffit_##_tp)) { \
        ksio_add(io, "%f", (ks_cfloat)*(_ctp*)ptr); \
    } \

    KSFFI_DO_F(CASE)
    #undef CASE

        else {
            /* Convert completely to string representation */
            kso r = ksffi_wrap((ks_type)obj_tp, self->val + off);
            if (!r) {
                KS_DECREF(io);
                KS_DECREF(members);
                return NULL;
            }
            if (!ksio_add(io, "%S", r)) {
                KS_DECREF(r);
                KS_DECREF(io);
                KS_DECREF(members);
                return NULL;
            }
            KS_DECREF(r);
        }
    }
    
    ksio_add(io, ")");

    KS_DECREF(members);

    return (kso)ksio_StringIO_getf(io);
}

static KS_TFUNC(T, integral) {
    ksffi_ptr self;
    KS_ARGS("self:*", &self, ksffit_ptr);

    return (kso)ks_int_newu((ks_uint)self->val);
}

static KS_TFUNC(T, getattr) {
    ksffi_struct self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksffit_struct, &attr, kst_str);

    ks_list members = (ks_list)kso_getattr_c((kso)self->type, "__members");
    if (!members) {
        return NULL;
    }

    int idx = my_attridx(self, attr);
    if (idx < 0) {
        KS_DECREF(members);
        return NULL;
    }
    ks_cint off;
    if (!kso_get_ci(((ks_tuple)members->elems[idx])->elems[2], &off)) {
        KS_DECREF(members);
        return NULL;
    }

    kso r = ksffi_wrap((ks_type)((ks_tuple)members->elems[idx])->elems[1], self->val + off);
    if (!r) {
        KS_DECREF(members);
        return NULL;
    }

    KS_DECREF(members);
    return r;
}

static KS_TFUNC(T, setattr) {
    ksffi_struct self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, ksffit_struct, &attr, kst_str, &val);

    ks_list members = (ks_list)kso_getattr_c((kso)self->type, "__members");
    if (!members) {
        return NULL;
    }

    int idx = my_attridx(self, attr);
    if (idx < 0) {
        KS_DECREF(members);
        return NULL;
    }
    ks_cint off;
    if (!kso_get_ci(((ks_tuple)members->elems[idx])->elems[2], &off)) {
        KS_DECREF(members);
        return NULL;
    }

    if (!ksffi_unwrap((ks_type)((ks_tuple)members->elems[idx])->elems[1], val, self->val + off)) {
        KS_DECREF(members);
        return NULL;
    }

    KS_DECREF(members);
    return KS_NEWREF(val);
}

static KS_TFUNC(T, getelem) {
    ksffi_ptr self;
    ks_cint idx;
    KS_ARGS("self:* idx:cint", &self, ksffit_ptr, &idx);

    int sz = ksffi_sizeofp(self->type);
    if (sz < 0) return NULL;

    return (kso)ksffi_wrap((ks_type)self->type->i__template->elems[0], (void*)(((ks_cint)self->val) + sz * idx));
}

static KS_TFUNC(T, setelem) {
    ksffi_ptr self;
    ks_cint idx;
    kso val;
    KS_ARGS("self:* idx:cint val", &self, ksffit_ptr, &idx, &val);

    int sz = ksffi_sizeofp(self->type);
    if (sz < 0) return NULL;
    void* res = (void*)(((ks_cint)self->val) + sz * idx);

    if (!ksffi_unwrap((ks_type)self->type->i__template->elems[0], val, res)) {
        return NULL;
    }

    return KSO_NONE;
}

static KS_TFUNC(T, on_template) {
    ks_type tp;
    KS_ARGS("tp:*", &tp, kst_type);

    assert(tp->i__template != NULL);

    int nargs = tp->i__template->len;
    kso* args = tp->i__template->elems;
    ks_dict map = ks_dict_new(NULL);
    ks_type_set_c(tp, "__members_map", (kso)map);
    KS_DECREF(map);

    size_t* offsets = ks_malloc(sizeof(*offsets) * nargs);

#ifdef KS_HAVE_ffi

    int f_abi = FFI_DEFAULT_ABI;

    ffi_type f_tp;

    f_tp.size = 0;
    f_tp.alignment = 0;
    f_tp.type = FFI_TYPE_STRUCT;
    f_tp.elements = ks_malloc(sizeof(*f_tp.elements) * (nargs + 1));

    /* Add structure members */
    int i;
    for (i = 0; i < nargs; ++i) {
        ks_tuple a = (ks_tuple)args[i];
        if (!kso_issub(a->type, kst_tuple)) {
            ks_free(offsets);
            ks_free(f_tp.elements);
            KS_THROW(kst_TypeError, "Expected template parameters to be a tuple of the form '(type, name)', but instead got '%T' object", a);
            return NULL;
        }

        if (a->len != 2) {
            ks_free(offsets);
            ks_free(f_tp.elements);
            KS_THROW(kst_TypeError, "Expected template parameters to be a tuple of the form '(type, name)', but instead got one of length %i", (int)a->len);
            return NULL;
        }

        kso name = a->elems[0];
        kso obj_tp = a->elems[1];
        ks_int vi = ks_int_new(i);
        if (!ks_dict_set(map, a->elems[0], (kso)vi)) {
            ks_free(offsets);
            ks_free(f_tp.elements);
            KS_DECREF(vi);
            return NULL;
        }
        KS_DECREF(vi);

        if (!ksffi_libffi_type((ks_type)obj_tp, &f_tp.elements[i])) {
            ks_free(offsets);
            ks_free(f_tp.elements);
            return NULL;
        }
    }
    f_tp.elements[nargs] = NULL;

#ifdef KS_HAVE_ffi_get_struct_offsets
    /* Get structure offsets */
    if (ffi_get_struct_offsets(f_abi, &f_tp, offsets) != FFI_OK) {
        ks_free(offsets);
        ks_free(f_tp.elements);
        KS_THROW(kst_Error, "Failure from libffi: failed to get structure offsets");
        return NULL;
    }
#else
    ffi_cif f_cif;
    if (ffi_prep_cif(&f_cif, f_abi, 0, &f_tp, NULL) != FFI_OK) {
        ks_free(offsets);
        ks_free(f_tp.elements);
        KS_THROW(kst_Error, "Failure from libffi: failed to prep CIF");
        return NULL;
    }

    int c = 0;
    for (i = 0; i < nargs; ++i) {
        offsets[i] = c;
        c += f_tp.elements[i]->size;
    }

#endif

    assert(f_tp.size != 0);

    ks_int vsz = ks_int_new(f_tp.size);
    ks_type_set_c(tp, "__sizeof", (kso)vsz);
    KS_DECREF(vsz);

    if (tp->ob_sz < f_tp.size + sizeof(*((kso)NULL))) {
        tp->ob_sz = f_tp.size + sizeof(*((kso)NULL));
    }

    /* Wrap FFI descriptor */
    ffi_type* f_desca = ks_malloc(sizeof(*f_desca));
    kso f_desco = ksffi_wrap(ksffit_ptr_void, (void*)&f_desca);
    ks_type_set_c(tp, "__ffi_desc", (kso)f_desco);
    KS_DECREF(f_desco);

#else

    int sz = 0;

    /* Add structure members */
    int i;
    for (i = 0; i < nargs; ++i) {
        ks_tuple a = (ks_tuple)args[i];
        if (!kso_issub(a->type, kst_tuple)) {
            ks_free(offsets);
            KS_THROW(kst_TypeError, "Expected template parameters to be a tuple of the form '(type, name)', but instead got '%T' object", a);
            return NULL;
        }

        if (a->len != 2) {
            ks_free(offsets);
            KS_THROW(kst_TypeError, "Expected template parameters to be a tuple of the form '(type, name)', but instead got one of length %i", (int)a->len);
            return NULL;
        }
        ks_int vi = ks_int_new(i);
        if (!ks_dict_set(map, a->elems[0], (kso)vi)) {
            ks_free(offsets);
            KS_DECREF(vi);
            return NULL;
        }
        KS_DECREF(vi);

        offsets[i] = sz;
        int szof = ksffi_sizeof((ks_type)a->elems[1]);
        if (szof < 0) {
            ks_free(offsets);
            return NULL;
        }
        sz += szof;
    }
    ks_int vsz = ks_int_new(sz);
    ks_type_set_c(tp, "__sizeof", (kso)vsz);
    KS_DECREF(vsz);


#endif

    /* Create list of members */
    ks_list vmems = ks_list_new(0, NULL);
    for (i = 0; i < nargs; ++i) {
        ks_tuple mem = ks_tuple_newn(3, (kso[]){ 
            KS_NEWREF(((ks_tuple)args[i])->elems[0]),
            KS_NEWREF(((ks_tuple)args[i])->elems[1]),
            (kso)ks_int_new(offsets[i])
        });
        ks_list_pushu(vmems, (kso)mem);
    }

    ks_type_set_c(tp, "__members", (kso)vmems);
    KS_DECREF(vmems);


    ks_free(offsets);


    return KSO_NONE;
}

/* Check whether two objects of the same type are equal */
static int my_iseq(ks_type tp, void* a, void* b) {

    if (false) {}

    #define CASE(_tp, _ctp) else if (kso_issub(tp, ksffit_##_tp)) { \
        return *(_ctp*)a == *(_ctp*)b; \
    }
    KSFFI_DO_I(CASE)
    KSFFI_DO_F(CASE)

    else if (kso_issub(tp, ksffit_struct)) {
        ks_list members = (ks_list)kso_getattr_c((kso)tp, "__members");
        if (!members) {
            return -1;
        }

        int i;
        for (i = 0; i < members->len; ++i) {
            ks_type mt = (ks_type)((ks_tuple)members->elems[i])->elems[1];
            ks_cint off;
            if (!kso_get_ci(((ks_tuple)members->elems[i])->elems[1], &off)) {
                KS_DECREF(members);
                return -1;
            }

            int r = my_iseq(mt, (void*)((ks_uint)a + off), (void*)((ks_uint)b + off));
            if (r < 0) {
                KS_DECREF(members);
                return -1;
            } else if (!r) {
                KS_DECREF(members);
                return 0;
            }
        }

        KS_DECREF(members);
        return 1;

    } else {
        KS_THROW(kst_Error, "Don't know how to tell equality for '%T' objects", tp);
        return -1;
    }

}

static KS_TFUNC(T, eq) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, ksffit_struct) && kso_issub(R->type, ksffit_struct) && L->type == R->type) {
        /* Compare whether structures are equal */
        
        int r = my_iseq(L->type, ((ksffi_struct)L)->val, ((ksffi_struct)R)->val);
        if (r < 0) {
            return NULL;
        }

        return KSO_BOOL(r);
    }

    return KSO_UNDEFINED;
}

/* Export */

static struct ks_type_s tp;
ks_type ksffit_struct = &tp;

void _ksi_ffi_struct() {
    _ksinit(ksffit_struct, kst_object, T_NAME, sizeof(struct ksffi_struct_s), -1, "C-style structure", KS_IKV(
        {"__template",             (kso)ks_tuple_new(0, NULL)},
        {"__on_template",          ksf_wrap(T_on_template_, T_NAME ".__on_template(tp)", "")},

        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, val=0)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__setattr",              ksf_wrap(T_setattr_, T_NAME ".__setattr(self, attr, val)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, idx)", "")},
        {"__setelem",              ksf_wrap(T_setelem_, T_NAME ".__setelem(self, idx, val)", "")},
        {"__eq",                   ksf_wrap(T_eq_, T_NAME ".__eq(L, R)", "")},

        {"__sizeof",               (kso)ks_int_newu(0)},
        {"of",                     KS_NEWREF(kst_none)},

    ));

}

