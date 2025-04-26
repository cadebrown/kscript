/* ffi/main.c - 'ffi' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ksffi.h>

#define M_NAME "ffi"

/* C-API */

/* FFI utilities */
#ifdef KS_HAVE_ffi

/* Converts type to FFI type, seting '*res' */

bool ksffi_libffi_type(ks_type tp, ffi_type** res) {
    if (tp == (ks_type)KSO_NONE || tp == KSO_NONE->type) {
        *res = &ffi_type_void;
        return true;
    }

    #define CASE(_type, _ftype) else if (kso_issub(tp, _type)) { \
        *res = _ftype; \
        return true; \
    }

    if (false) {}

    CASE(ksffit_s8,  &ffi_type_sint8)
    CASE(ksffit_u8,  &ffi_type_uint8)
    CASE(ksffit_s16, &ffi_type_sint16)
    CASE(ksffit_u16, &ffi_type_uint16)
    CASE(ksffit_s32, &ffi_type_sint32)
    CASE(ksffit_u32, &ffi_type_uint32)
    CASE(ksffit_s64, &ffi_type_sint64)
    CASE(ksffit_u64, &ffi_type_uint64)

    CASE(ksffit_float,  &ffi_type_float)
    CASE(ksffit_double,  &ffi_type_double)
    CASE(ksffit_longdouble, &ffi_type_longdouble)
    
    CASE(ksffit_ptr, &ffi_type_pointer)

    else if (kso_issub(tp, ksffit_struct)) {
        kso desc = kso_getattr_c((kso)tp, "__ffi_desc");
        if (!desc) {
            return false;
        }

        ffi_type* f_desc;
        if (!ksffi_unwrap(ksffit_ptr, desc, (void*)&f_desc)) {
            KS_DECREF(desc);
            return false;
        }

        *res = f_desc;

        KS_DECREF(desc);
        return true;
    }

    #undef CASE

    KS_THROW(kst_TypeError, "'%R' was not a valid FFI/C-style value type", tp);
    return false;
}

#endif



kso ksffi_wrap(ks_type tp, void* val) {
    if (!tp) {
        tp = ksffit_ptr_void;
    }

    if (tp == kst_none || (kso)tp == KSO_NONE) {
        KS_THROW(kst_TypeError, "Cannot create value from 'void' type (perhaps you are dereferencing a 'void*' value)", tp);
        return NULL;
    }

    if (false) {}

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        ksffi_##_name res = KSO_NEW(ksffi_##_name, tp); \
        res->val = *(_ctp*)val; \
        return (kso)res; \
    }
    KSFFI_DO_I(_KSCASE)
    #undef _KSCASE

    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        ksffi_##_name res = KSO_NEW(ksffi_##_name, tp); \
        res->val = *(_ctp*)val; \
        return (kso)res; \
    }
    KSFFI_DO_F(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(tp, ksffit_ptr)) {
        ksffi_ptr res = KSO_NEW(ksffi_ptr, tp);
        res->val = *(void**)val;
        return (kso)res;
    } else if (kso_issub(tp, ksffit_func)) {
        ksffi_func res = KSO_NEW(ksffi_func, tp);
        res->val = *(void(**)())val;
        return (kso)res;

    } else if (kso_issub(tp, ksffit_struct)) {
        ks_cint sz = ksffi_sizeof(tp);
        if (sz < 0) {
            return NULL;
        }
        ksffi_struct res = KSO_NEW(ksffi_struct, tp);
        memcpy(res->val, val, sz);
        return (kso)res;


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
        if (kso_issub(obj->type, ksffit_##_name)) { \
            *(_ctp*)val = ((ksffi_##_name)obj)->val; \
            return true; \
        } else { \
            ks_cint v; \
            if (!kso_get_ci(obj, &v)) { \
                return NULL; \
            } \
            *(_ctp*)val = v; \
            return true; \
        } \
    }
    KSFFI_DO_I(_KSCASE)
    #undef _KSCASE


    #define _KSCASE(_name, _ctp) if (kso_issub(tp, ksffit_##_name)) { \
        if (kso_issub(obj->type, ksffit_##_name)) { \
            *(_ctp*)val = ((ksffi_##_name)obj)->val; \
            return true; \
        } else { \
            ks_cfloat v; \
            if (!kso_get_cf(obj, &v)) { \
                return NULL; \
            } \
            *(_ctp*)val = v; \
            return true; \
        } \
    }
    KSFFI_DO_F(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(tp, ksffit_ptr)) {
        if (kso_issub(obj->type, ksffit_ptr)) {
            *(void**)val = ((ksffi_ptr)obj)->val;
            return true;
        } else if (kso_issub(obj->type, kst_str)) {
            *(void**)val = ((ks_str)obj)->data;
            return true;
        } else if (kso_issub(obj->type, kst_bytes)) {
            *(void**)val = ((ks_bytes)obj)->data;
            return true;
        } else {
            ks_cint v;
            if (!kso_get_ci(obj, &v)) return NULL;
            *(void**)val = (void*)v;
            return true;
        }
    } else if (kso_issub(tp, ksffit_func)) {
        if (kso_issub(obj->type, ksffit_ptr)) {
            *(void(**)())val = ((ksffi_func)obj)->val;
            return true;
        } else {
            ks_cint v;
            if (!kso_get_ci(obj, &v)) return NULL;
            void* vp = (void*)v;
            void (**vpp)() = (void (**)())&vp;
            *(void(**)())val = *vpp;
            return true;
        }
    } else if (kso_issub(tp, ksffit_struct)) {
        if (kso_issub(obj->type, tp)) {
            ks_cint sz = ksffi_sizeof(tp);
            if (sz < 0) {
                return NULL;
            }
            memcpy(val, ((ksffi_struct)obj)->val, sz);
            return true;
        } else {
            KS_THROW(kst_Error, "Failed to unwrap '%R', argument of invalid type '%T'", tp, obj);
            return false;
        }

    } else {

        KS_THROW(kst_TypeError, "Cannot unwrap '%R' to C-style address", tp);
        return false;
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


ks_type ksffi_typeof(kso obj) {
    #define CASE(_name, _type) else if (kso_issub(obj->type, ksffit_##_name)) { \
        KS_INCREF(obj->type); \
        return obj->type; \
    } \

    if (false) {}
    KSFFI_DO_I(CASE)
    KSFFI_DO_F(CASE)

    if (kso_is_float(obj)) {
        KS_INCREF(ksffit_double);
        return ksffit_double;
    } else if (kso_is_int(obj)) {
        KS_INCREF(ksffit_s64);
        return ksffit_s64;
    }

    KS_THROW(kst_TypeError, "Don't know corresponding FFI type for objects of type '%T'", obj);
    return NULL;
}


ksffi_dll ksffi_open(ks_str src) {
#ifdef WIN32
	HMODULE handle = LoadLibrary(src->data);
	if (!handle) {
		KS_THROW(kst_IOError, "Failed to LoadLibrary %R: [%i]", src, (int)GetLastError());
		return NULL;
	}

	ksffi_dll self = KSO_NEW(ksffi_dll, ksffit_dll);

	KS_INCREF(src);
	self->src = src;
	self->handle = handle;

	return self;

#else
    void* handle = dlopen(src->data, RTLD_LAZY);
    if (!handle) {
        KS_THROW(kst_IOError, "Failed to dlopen %R: %s", src, dlerror());
        return NULL;
    }

    ksffi_dll self = KSO_NEW(ksffi_dll, ksffit_dll);

    KS_INCREF(src);
    self->src = src;
    self->handle = handle;

    return self;
#endif
}

/* Module functions */


static KS_TFUNC(M, open) {
    ks_str src;
    KS_ARGS("src:*", &src, kst_str);

    return (kso)ksffi_open(src);
}

static KS_TFUNC(M, sizeof) {
    kso obj;
    KS_ARGS("obj", &obj);

    int r = ksffi_sizeof(kso_issub(obj->type, kst_type) ? (ks_type)obj : obj->type);
    if (r < 0) return NULL;

    return (kso)ksffi_wrap(ksffit_size_t, (size_t[]){ r });
}

static KS_TFUNC(M, addr) {
    kso obj;
    KS_ARGS("obj", &obj);

    if (false) {}

    #define _KSCASE(_name, _ctp) if (kso_issub(obj->type, ksffit_##_name)) { \
        ks_type pt = ksffi_ptr_make(obj->type); \
        void* tmp = &((ksffi_##_name)obj)->val; \
        kso res = ksffi_wrap(pt, &tmp);\
        KS_DECREF(pt); \
        return res; \
    }
    KSFFI_DO_I(_KSCASE)
    #undef _KSCASE

    #define _KSCASE(_name, _ctp) if (kso_issub(obj->type, ksffit_##_name)) { \
        ks_type pt = ksffi_ptr_make(obj->type); \
        void* tmp = &((ksffi_##_name)obj)->val; \
        kso res = ksffi_wrap(pt, &tmp);\
        KS_DECREF(pt); \
        return res; \
    }
    KSFFI_DO_F(_KSCASE)
    #undef _KSCASE

    else if (kso_issub(obj->type, ksffit_ptr)) {
        ks_type pt = ksffi_ptr_make(obj->type);
        void* tmp = &((ksffi_ptr)obj)->val;
        kso res = ksffi_wrap(pt, &tmp);
        KS_DECREF(pt);
        return res;
    } 

    KS_THROW(kst_TypeError, "Cannot take address of '%T' object", obj);
    return NULL;
}


static KS_TFUNC(M, fromUTF8) {
    kso addr;
    KS_ARGS("addr", &addr);

    if (!kso_issub(addr->type, ksffit_ptr)) {
        KS_THROW(kst_TypeError, "'addr' must be a pointer type, but is of type '%T'", addr);
        return NULL;
    } else {
        return (kso)ks_str_new(-1, ((ksffi_ptr)addr)->val);
    }
}

static KS_TFUNC(M, toUTF8) {
    kso obj;
    kso addr = NULL;
    ks_cint sz = -1;
    KS_ARGS("obj ?addr ?sz:cint", &obj, &addr, &sz);

    if (!kso_issub(obj->type, kst_str) && !kso_issub(obj->type, kst_bytes)) {
        KS_THROW(kst_TypeError, "Expected 'obj' to be a 'str' or 'bytes' object, but got '%T' instead", obj);
        return NULL;
    }

    ks_cint sz_req = -1;
    if (kso_issub(obj->type, kst_str)) {
        sz_req = ((ks_str)obj)->len_b + 1;
    } else if (kso_issub(obj->type, kst_bytes)) {
        sz_req = ((ks_bytes)obj)->len_b + 1;
    }
    assert(sz_req >= 0);

    void* ptr = NULL;
    if (!addr) {
        ptr = malloc(sz_req);
    } else {
        if (!ksffi_unwrap(ksffit_ptr_void, addr, (void*)&ptr)) {
            return NULL;
        }
    }

    if (sz < 0) {
        sz = sz_req;
    }
    if (sz > sz_req) {
        sz = sz_req;
    }

    if (kso_issub(obj->type, kst_str)) {
        memcpy(ptr, ((ks_str)obj)->data, sz);
    } else if (kso_issub(obj->type, kst_bytes)) {
        if (sz > ((ks_bytes)obj)->len_b) {
            memcpy(ptr, ((ks_bytes)obj)->data, ((ks_bytes)obj)->len_b);
            memcpy((char*)ptr + ((ks_bytes)obj)->len_b, "\0", 1);
        }
    }
    return (kso)ksffi_wrap(ksffit_ptr_void, (void*)&ptr);
}

static KS_TFUNC(M, malloc) {
    ks_cint sz;
    KS_ARGS("sz:cint", &sz);

    void* res = malloc(sz);

    return (kso)ksffi_wrap(ksffit_ptr_void, (void*)&res);
}

static KS_TFUNC(M, realloc) {
    ks_cint ptr;
    ks_cint sz;
    KS_ARGS("ptr:cint sz:cint", &ptr, &sz);

    void* res = realloc((void*)ptr, sz);

    return (kso)ksffi_wrap(ksffit_ptr_void, (void*)&res);
}

static KS_TFUNC(M, free) {
    ks_cint ptr;
    KS_ARGS("ptr:cint", &ptr);

    free((void*)ptr);

    return KSO_NONE;
}

/* Export */

ks_module _ksi_ffi() {
    _ksi_ffi_int();
    _ksi_ffi_float();
    _ksi_ffi_ptr();
    _ksi_ffi_func();
    _ksi_ffi_struct();
    _ksi_ffi_dll();

#define _KSCASE(_name, _ctp) {#_name,  KS_NEWREF(ksffit_##_name)},

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Foreign Function Interface (FFI) library", KS_IKV(
        
        /* Types */
        
        {"DLL",                    KS_NEWREF(ksffit_dll)},

        KSFFI_DO_I(_KSCASE)
        KSFFI_DO_F(_KSCASE)

        /* Aliases */

        {"char",                   KS_NEWREF(ksffit_char)},
        {"uchar",                  KS_NEWREF(ksffit_uchar)},
        {"short",                  KS_NEWREF(ksffit_short)},
        {"ushort",                 KS_NEWREF(ksffit_ushort)},
        {"int",                    KS_NEWREF(ksffit_int)},
        {"uint",                   KS_NEWREF(ksffit_uint)},
        {"long",                   KS_NEWREF(ksffit_long)},
        {"ulong",                  KS_NEWREF(ksffit_ulong)},
        {"longlong",               KS_NEWREF(ksffit_longlong)},
        {"ulonglong",              KS_NEWREF(ksffit_ulonglong)},


        {"size_t",                 KS_NEWREF(ksffit_size_t)},
        {"ssize_t",                KS_NEWREF(ksffit_ssize_t)},

        {"func",                   KS_NEWREF(ksffit_func)},
        {"ptr",                    KS_NEWREF(ksffit_ptr)},
        {"struct",                 KS_NEWREF(ksffit_struct)},

        /* Functions */

        {"open",                   ksf_wrap(M_open_, M_NAME ".open(name)", "Opens a dynamically loaded library")},
        
        {"addr",                   ksf_wrap(M_addr_, M_NAME ".addr(obj)", "Computes the address of the value")},
        
        {"sizeof",                 ksf_wrap(M_sizeof_, M_NAME ".sizeof(obj)", "Computes the size (in bytes) of an object (or, if given a type, the type)")},
        {"fromUTF8",               ksf_wrap(M_fromUTF8_, M_NAME ".fromUTF8(addr)", "Returns a string created from UTF8 bytes at 'addr' (NUL-terminated)")},
        {"toUTF8",                 ksf_wrap(M_toUTF8_, M_NAME ".toUTF8(addr, addr=none, sz=-1)", "Returns a string created from UTF8 bytes at 'addr' (NUL-terminated)")},


        {"malloc",                 ksf_wrap(M_malloc_, M_NAME ".malloc(sz)", "Wrapper for 'malloc'")},
        {"realloc",                ksf_wrap(M_realloc_, M_NAME ".realloc(ptr, sz)", "Wrapper for 'realloc'")},
        {"free",                   ksf_wrap(M_free_, M_NAME ".free(ptr)", "Wrapper for 'free'")},


    ));

    return res;
}
