/* ffi/main.c - 'ffi' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define M_NAME "ffi"

/* C-API */


kso ksffi_wrap(ks_type tp, void* val) {
    if (tp == kst_none || (kso)tp == KSO_NONE) {
        KS_THROW(kst_TypeError, "Cannot create value from 'void' type (perhaps your pointer is a 'void*')", tp);
        return NULL;
    }

    if (false) {}

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        return (kso)ksffi_new_int(tp, *(_ctp*)val); \
    }
    KSFFI_DO_INTS(_KSCASE)
    #undef _KSCASE

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        return (kso)ksffi_new_float(tp, *(_ctp*)val); \
    }
    KSFFI_DO_FLOATS(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(tp, ksffit_ptr)) {
        return (kso)ksffi_ptr_maken(tp, val);
    } else {

        KS_THROW(kst_TypeError, "Cannot create '%R' from C-style address", tp);
        return NULL;
    }
}

bool ksffi_unwrap(ks_type tp, kso obj, void* val) {
    if (tp == kst_none || (kso)tp == KSO_NONE) {
        KS_THROW(kst_TypeError, "Cannot unwrap value to 'void' type (perhaps your pointer is a 'void*')", tp);
        return NULL;
    }

    if (false) {}

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        ks_cint v; \
        if (!kso_get_ci(obj, &v)) return NULL; \
        *(_ctp*)val = (_ctp)v; \
        return true; \
    }
    KSFFI_DO_INTS(_KSCASE)
    #undef _KSCASE

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        ks_cfloat v; \
        if (!kso_get_cf(obj, &v)) return NULL; \
        *(_ctp*)val = v; \
        return true; \
    }
    KSFFI_DO_FLOATS(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(tp, ksffit_ptr)) {
        ks_cint v;
        if (!kso_get_ci(obj, &v)) return NULL;
        *(void**)val = (void*)v;
        return true;
    } else {

        KS_THROW(kst_TypeError, "Cannot unwrap '%R' to C-style address", tp);
        return false;
    }
}

kso ksffi_wrapo(kso obj, ks_type tp) {
    if (tp == NULL || (obj->type == kst_str || obj->type == kst_bytes)) {
        tp = obj->type;
        #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
            return KS_NEWREF(obj); \
        }
        KSFFI_DO_INTS(_KSCASE)
        #undef _KSCASE

        #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
            return KS_NEWREF(obj); \
        }
        KSFFI_DO_FLOATS(_KSCASE)
        #undef _KSCASE

        if (kso_is_float(obj)) {
            ks_cfloat v;
            if (!kso_get_cf(obj, &v)) return NULL;
            return (kso)ksffi_new_float(ksffit_double, v);
        } else if (kso_is_int(obj)) {
            ks_cint v;
            if (!kso_get_ci(obj, &v)) return NULL;
            return (kso)ksffi_new_int(ksffit_sint, v);
        } else if (kso_issub(obj->type, kst_str)) {
            return (kso)ksffi_ptr_make(ksffit_schar, ((ks_str)obj)->data);
        } else if (kso_issub(obj->type, kst_bytes)) {
            return (kso)ksffi_ptr_make(ksffit_uchar, ((ks_bytes)obj)->data);
        }

        KS_THROW(kst_TypeError, "Failed to wrap '%T' object", obj);
        return NULL;
    } else {
        if (false) {}

        #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
            ks_cint v; \
            if (!kso_get_ci(obj, &v)) return NULL; \
            return (kso)ksffi_new_int(tp, v); \
        }
        KSFFI_DO_INTS(_KSCASE)
        #undef _KSCASE

        #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
            ks_cfloat v; \
            if (!kso_get_cf(obj, &v)) return NULL; \
            return (kso)ksffi_new_float(tp, v); \
        }
        KSFFI_DO_FLOATS(_KSCASE)
        #undef _KSCASE

        else if (kso_issub(tp, ksffit_ptr)) {
            ks_cint v;
            if (!kso_get_ci(obj, &v)) return NULL;
            return (kso)ksffi_ptr_maken(tp, (void*)v);
        } 

        KS_THROW(kst_TypeError, "Failed to wrap '%T' object as '%R'", obj, tp);
        return NULL;
    }
}




int ksffi_sizeof(ks_type tp) {
    /* sizeof(void) == 1 */
    if (tp == kst_none || (kso)tp == KSO_NONE) return 1;

    static ks_str key = NULL;
    if (!key) key = ks_str_new(-1, "__sizeof");
    kso sz = ks_type_get(tp, key);
    if (!sz) {
        return -1;
    }

    ks_cint res;
    if (!kso_get_ci(sz, &res)) {
        KS_DECREF(sz);
        return -1;
    }
    KS_DECREF(sz);
    return res;
}

int ksffi_sizeofp(ks_type tp) {
    if (!kso_issub(tp, ksffit_ptr)) {
        KS_THROW(kst_TypeError, "Cannot get sizeof pointer element of non-pointer type '%R'", tp);
        return -1;
    }

    return ksffi_sizeof((ks_type)tp->i__template->elems[0]);
}


ksffi_dll ksffi_open(ks_type tp, ks_str name) {
    void* handle = dlopen(name->data, RTLD_LAZY);
    if (!handle) {
        KS_THROW(kst_IOError, "Failed to dlopen %R: %s", name, dlerror());
        return NULL;
    }

    ksffi_dll self = KSO_NEW(ksffi_dll, tp);

    KS_INCREF(name);
    self->name = name;
    self->handle = handle;

    return self;
}

/* Module functions */


static KS_TFUNC(M, open) {
    ks_str name;
    KS_ARGS("name:*", &name, kst_str);

    return (kso)ksffi_open(ksffit_dll, name);
}

static KS_TFUNC(M, wrap) {
    kso obj;
    ks_type tp = NULL;
    KS_ARGS("obj ?tp:*", &obj, &tp, kst_type);

    return ksffi_wrapo(obj, tp);
}

static KS_TFUNC(M, sizeof) {
    kso obj;
    KS_ARGS("obj", &obj);

    int r = ksffi_sizeof(kso_issub(obj->type, kst_type) ? (ks_type)obj : obj->type);
    if (r < 0) return NULL;

    return (kso)ksffi_new_int(ksffit_ulonglong, r);
}


static KS_TFUNC(M, addr) {
    kso obj;
    KS_ARGS("obj", &obj);

    if (false) {}

    #define _KSCASE(_name, _ctp) if (kso_issub(obj->type, ksffit_##_name)) { \
        return (kso)ksffi_ptr_make(obj->type, &((ksffi_##_name)obj)->val); \
    }
    KSFFI_DO_INTS(_KSCASE)
    #undef _KSCASE

    #define _KSCASE(_name, _ctp) if (kso_issub(obj->type, ksffit_##_name)) { \
        return (kso)ksffi_ptr_make(obj->type, &((ksffi_##_name)obj)->val); \
    }
    KSFFI_DO_FLOATS(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(obj->type, ksffit_ptr)) {
        return (kso)ksffi_ptr_make(obj->type, ((ksffi_ptr)obj)->val);
    } 

    KS_THROW(kst_TypeError, "Cannot take address of '%T' object", obj);
    return NULL;
}


/* Export */

ks_module _ksi_ffi() {
    _ksi_ffi_ints();
    _ksi_ffi_floats();
    _ksi_ffi_ptr();
    _ksi_ffi_func();
    _ksi_ffi_dll();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Foreign Function Interface (FFI) library", KS_IKV(
        
        /* Types */
        
        {"DLL",                    KS_NEWREF(ksffit_dll)},

        #define _KSCASE(_name, _ctp) {#_name,  KS_NEWREF(ksffit_##_name)},
        KSFFI_DO_INTS(_KSCASE)
        KSFFI_DO_FLOATS(_KSCASE)

        /* Aliases */
        {"size_t",                 KS_NEWREF(ksffit_ulonglong)},
        {"ssize_t",                KS_NEWREF(ksffit_slonglong)},

        {"func",                   KS_NEWREF(ksffit_func)},
        {"ptr",                    KS_NEWREF(ksffit_ptr)},

        /* Functions */

        {"open",                   ksf_wrap(M_open_, M_NAME ".open(name)", "Opens a dynamically loaded library")},
        {"wrap",                   ksf_wrap(M_wrap_, M_NAME ".wrap(obj, type=none)", "Wraps an object in a C-style 'ffi' type")},
        {"addr",                   ksf_wrap(M_addr_, M_NAME ".addr(obj)", "Computes the address of the value")},
        {"sizeof",                 ksf_wrap(M_sizeof_, M_NAME ".sizeof(obj)", "Computes the size (in bytes) of an object (or, if given a type, the type)")},

    ));

    return res;
}
