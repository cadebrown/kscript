/* ffi/func.c - 'ffi.func' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define T_NAME "ffi.func"


#ifndef KS_HAVE_ffi
//#warning Building kscript without ffi support, so calling C-style functions from kscript may throw errors or cause problems
#endif

/* C-API */

ks_type ksffi_func_make(ks_type restype, int nargtypes, ks_type* argtypes, bool isvararg) {
    ks_type res = NULL;
    if (isvararg) {
        kso* targs = ks_smalloc(sizeof(*targs) * (1 + nargtypes));
        int i;
        for (i = 0; i < nargtypes; ++i) {
            targs[i] = (kso)argtypes[i];
        }
        targs[i] = KSO_DOTDOTDOT;

        ks_tuple t = ks_tuple_new(nargtypes + 1, targs);
        res = ks_type_template(ksffit_func, 2, (kso[]){ (kso)restype, (kso)t });
        KS_DECREF(t);
        ks_free(targs);
    } else {
        ks_tuple t = ks_tuple_new(nargtypes, (kso*)argtypes);
        res = ks_type_template(ksffit_func, 2, (kso[]){ (kso)restype, (kso)t });
        KS_DECREF(t);
    }

    return res;
}

/* Type Functions */

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_cint val = 0;
    KS_ARGS("tp:* ?val:cint", &tp, kst_type, &val);

    void* vv = (void*)val;
    return (kso)ksffi_wrap(tp, &vv);
}

static KS_TFUNC(T, str) {
    ksffi_func self;
    KS_ARGS("self:*", &self, ksffit_func);

    return (kso)ks_fmt("%T(%p)", self, *(void**)&self->val);
}


static KS_TFUNC(T, call) {
    ksffi_func self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, ksffit_func, &nargs, &args);

#ifdef KS_HAVE_ffi

    assert(self->type->i__template->len == 2);
    ks_tuple ta = (ks_tuple)self->type->i__template->elems[1];
    if (!kso_issub(ta->type, kst_tuple)) {
        KS_THROW(kst_TypeError, "'%T.__template[1]' was not a tuple of argument types, but was a '%T' object", self, ta);
        return NULL;
    }

    /* Number of arguments and variadic-ness */
    int req_nargs = ta->len;
    bool is_va = false;
    if (ta->len > 0 && ta->elems[ta->len - 1] == KSO_DOTDOTDOT) {
        /* Variadic function */
        is_va = true;
        /* Don't include that in the required arguments */
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

    /* Now, convert arguments to C-style members */

    /* Result type */
    ffi_type* f_restype = NULL;
    ks_type rtp = (ks_type)self->type->i__template->elems[0];
    if (!ksffi_libffi_type(rtp, &f_restype)) {
        return NULL;
    }

    /* Argument types */
    ffi_type** f_argtypes = ks_zmalloc(sizeof(*f_argtypes), nargs);
    /* Argument values */
    void** f_argvals = ks_zmalloc(sizeof(*f_argvals), nargs);
    
    char* s = NULL;
    /* Populate arrays */
    int i, j;
    for (i = 0; i < nargs; ++i) {
        /* Determine type to cast to */
        ks_type ctype = NULL;
        if (i < req_nargs) {
            ctype = (ks_type)ta->elems[i];
            KS_INCREF(ctype);
        } else {
            ctype = ksffi_typeof(args[i]);
            if (!ctype) {
                for (j = 0; j < i; ++j) ks_free(f_argvals[j]);
                ks_free(f_argtypes);
                ks_free(f_argvals);
                return NULL;
            }
        }

        /* Convert to FFI type */
        if (!ksffi_libffi_type(ctype, &f_argtypes[i])) {
            KS_DECREF(ctype);
            for (j = 0; j < i; ++j) ks_free(f_argvals[j]);
            ks_free(f_argtypes);
            ks_free(f_argvals);
            return NULL;
        }

        /* Get size */
        int ctype_sz = ksffi_sizeof(ctype);
        if (ctype_sz < 0) {
            KS_DECREF(ctype);
            for (j = 0; j < i; ++j) ks_free(f_argvals[j]);
            ks_free(f_argtypes);
            ks_free(f_argvals);
            return NULL;
        }

        /* Allocate argument and unwrap object */
        f_argvals[i] = ks_smalloc(ctype_sz);
        if (
            !ksffi_unwrap(ctype, args[i], f_argvals[i])
        ) {
            KS_DECREF(ctype);
            ks_free(f_argvals[i]);
            for (j = 0; j < i; ++j) ks_free(f_argvals[j]);
            ks_free(f_argtypes);
            ks_free(f_argvals);
            return NULL;
        }

        KS_DECREF(ctype);
    }


    /* Prepare CIF */
    ffi_cif f_cif;

    /* Prepare for variadic or normal */

    /* TODO: Allow different ABIs? */
    int abi = FFI_DEFAULT_ABI;
    int f_rc = is_va 
        ? ffi_prep_cif_var(&f_cif, abi, req_nargs, nargs, f_restype, f_argtypes)
        : ffi_prep_cif(&f_cif, abi, nargs, f_restype, f_argtypes);

    if (f_rc != FFI_OK) {
        ks_free(f_argtypes);
        ks_free(f_argvals);
        for (j = 0; j < nargs; ++j) ks_free(f_argvals[j]);
        KS_THROW(kst_Error, "Failed to create FFI structure: ffi_prep_cif() returned %i", f_rc);
        return NULL;
    }

    /* Result data */
    void* res = ks_malloc(f_restype->size < 16 ? 16 : f_restype->size);

    /* Perform call */
    ffi_call(&f_cif, self->val, res, f_argvals);

    /* Free temporary buffers */
    for (j = 0; j < nargs; ++j) ks_free(f_argvals[j]);
    ks_free(f_argtypes);
    ks_free(f_argvals);

    bool is_ptr = kso_issub(rtp->type, kst_type) && kso_issub(rtp, ksffit_ptr);

    kso rr = f_restype == &ffi_type_void ? KSO_NONE : ksffi_wrap(rtp, res);
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
