/* ffi/func.c - 'ffi.func' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define T_NAME "ffi.func"


/* C-API */

ks_type ksffi_func_make(ks_type restype, ks_tuple argtypes) {
    return ks_type_template(ksffit_func, 2, (kso[]){ (kso)restype, (kso)argtypes });
}

ksffi_func ksffi_func_new(ks_type tp, void (*val)()) {
    ksffi_func self = KSO_NEW(ksffi_func, tp);

    self->val = val;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_cint val = 0;
    KS_ARGS("tp:* ?val:cint", &tp, kst_type, &val);

    void* vv = (void*)val;
    void (**v)() = (void(**)())&vv;

    return (kso)ksffi_func_new(tp, *v);
}

static KS_TFUNC(T, str) {
    ksffi_func self;
    KS_ARGS("self:*", &self, ksffit_func);

    return (kso)ks_fmt("%T(%p)", self, *(void**)&self->val);
}


/* FFI utils */
#ifdef KS_HAVE_ffi

/* Convert type to FFI type */
static bool my_ffi_cvt(ks_type tp, kso obj, ffi_type** otp, void** oval) {
    if (!tp) {
        assert(obj);
        tp = obj->type;
    }

    if ((kso)tp == KSO_NONE || tp == kst_none) {
        *otp = &ffi_type_void;
        return true;
    }

    #define CASE(_tp, _ctp, _ftp) else if (kso_issub(tp, _tp)) { \
        *otp = _ftp; \
        if (oval) *oval = &((_ctp)obj)->val; \
        return true; \
    }
    
    if (false) {}

    CASE(ksffit_schar, ksffi_schar, &ffi_type_schar)
    CASE(ksffit_schar, ksffi_uchar, &ffi_type_uchar)
    CASE(ksffit_sshort, ksffi_sshort, &ffi_type_sshort)
    CASE(ksffit_ushort, ksffi_ushort, &ffi_type_ushort)
    CASE(ksffit_sint, ksffi_sint, &ffi_type_sint)
    CASE(ksffit_uint, ksffi_uint, &ffi_type_uint)
    CASE(ksffit_slong, ksffi_slong, sizeof(signed long) == sizeof(signed int) ? &ffi_type_sint : &ffi_type_slong)
    CASE(ksffit_ulong, ksffi_ulong, sizeof(unsigned long) == sizeof(unsigned int) ? &ffi_type_uint : &ffi_type_ulong)
    CASE(ksffit_slonglong, ksffi_slonglong, &ffi_type_slong)
    CASE(ksffit_ulonglong, ksffi_ulonglong, &ffi_type_ulong)

    CASE(ksffit_float, ksffi_float, &ffi_type_float)
    CASE(ksffit_double, ksffi_double, &ffi_type_double)
    CASE(ksffit_longdouble, ksffi_longdouble, &ffi_type_longdouble)

    CASE(ksffit_ptr, ksffi_ptr, &ffi_type_pointer)

    KS_THROW(kst_TypeError, "'%R' was not a valid FFI/C-style value type", tp);
    return false;
}

#endif

static KS_TFUNC(T, call) {
    ksffi_func self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, ksffit_func, &nargs, &args);

#ifdef KS_HAVE_ffi

    assert(self->type->i__template->len == 2);
    ks_tuple ta = (ks_tuple)self->type->i__template->elems[1];
    if (!kso_issub(ta->type, kst_tuple)) {
        KS_THROW(kst_TypeError, "'%T.__template[1]' was not a tuple of argument types, but was a '%T' object", ta);
        return NULL;
    }

    /* Number of arguments and variadic-ness */
    int req_nargs = ta->len;
    bool is_va = false;
    if (ta->len > 0 && ta->elems[ta->len - 1] == KSO_DOTDOTDOT) {
        /* Variadic function */
        is_va = true;
        /* Don't include that */
        req_nargs--;
    }

    if (is_va) {
        if (nargs < req_nargs) {
            KS_THROW(kst_Error, "Variadic C-style function expected at least %i arguments, but only got %i", req_nargs, nargs);
            return NULL;
        }
    } else if (nargs != req_nargs) {
        KS_THROW(kst_Error, "C-style function expected %i arguments, but got %i", req_nargs, nargs);
        return NULL;
    }

    /* Wrap arguments */
    ks_list wraps = ks_list_new(0, NULL);
    int i;
    for (i = 0; i < nargs; ++i) {
        kso w = ksffi_wrapo(args[i], i < req_nargs ? (ks_type)ta->elems[i] : NULL);
        if (!w) {
            KS_DECREF(wraps);
            return NULL;
        }
        ks_list_pushu(wraps, w);
    }

    ks_type rtp = (ks_type)self->type->i__template->elems[0];
    /* Get types */
    ffi_type* f_restype;
    if (!my_ffi_cvt(rtp, NULL, &f_restype, NULL)) {
        KS_DECREF(wraps);
        return NULL;
    }

    ffi_type** f_argtypes = ks_zmalloc(sizeof(*f_argtypes), nargs);
    void** f_args = ks_zmalloc(sizeof(*f_args), nargs);
    for (i = 0; i < nargs; ++i) {
        if (!my_ffi_cvt(NULL, wraps->elems[i], &f_argtypes[i], &f_args[i])) {
            ks_free(f_argtypes);
            ks_free(f_args);
            KS_DECREF(wraps);
            return NULL;
        }
    }

    /* Prepare CIF */
    ffi_cif f_cif;

    /* Prepare for variadic or normal */
    int abi = FFI_DEFAULT_ABI;
    int f_rc = is_va 
        ? ffi_prep_cif_var(&f_cif, abi, req_nargs, nargs, f_restype, f_argtypes)
        : ffi_prep_cif(&f_cif, abi, nargs, f_restype, f_argtypes);
    
    if (f_rc != FFI_OK) {
        ks_free(f_argtypes);
        ks_free(f_args);
        KS_DECREF(wraps);
        KS_THROW(kst_Error, "Failed to create FFI structure: ffi_prep_cif() returned %i", f_rc);
        return NULL;
    }

    void* res = ks_malloc(f_restype->size < 16 ? 16 : f_restype->size);

    ffi_call(&f_cif, self->val, res, f_args);

    /* Free temporary buffers */
    ks_free(f_argtypes);
    ks_free(f_args);
    KS_DECREF(wraps);

    bool is_ptr = kso_issub(rtp->type, kst_type) && kso_issub(rtp, ksffit_ptr);

    kso rr = ((kso)rtp == KSO_NONE || rtp == kst_none) ? KSO_NONE : ksffi_wrap(rtp, is_ptr ? ((void**)res)[0] : res);
    ks_free(res);
    return rr;
#else
    KS_THROW(kst_PlatformWarning, "Failed to call C-style function wrapper: Platform had no libffi support (was not compiled with '--with-ffi')");
    return NULL;
#endif


}


/* Export */

static struct ks_type_s tp;
ks_type ksffit_func = &tp;


void _ksi_ffi_func() {

    _ksinit(ksffit_func, kst_object, T_NAME, sizeof(struct ksffi_func_s), -1, "C-style function wrapper", KS_IKV(
        {"__template",             (kso)ks_tuple_new(2, (kso[]){ KSO_NONE, KSO_NONE })},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, val=0)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__call",                 ksf_wrap(T_call_, T_NAME ".__call(self, *args)", "")},

    ));
}
