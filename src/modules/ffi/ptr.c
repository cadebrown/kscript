/* ffi/ptr.c - 'ffi' pointer types
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define T_NAME "ffi.ptr"



ks_type ksffi_ptr_make(ks_type of) {
    if (!of) {
        of = (ks_type)KSO_NONE;
    }
    return ks_type_template(ksffit_ptr, 1, (kso[]){ (kso)of });
}

/* Type definitions */

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_cint val = 0;
    KS_ARGS("tp:* ?val:cint", &tp, kst_type, &val);

    return (kso)ksffi_wrap(tp, (void*)&val);
}

static KS_TFUNC(T, str) {
    ksffi_ptr self;
    KS_ARGS("self:*", &self, ksffit_ptr);

    return (kso)ks_fmt("%T(%p)", self, self->val);
}

static KS_TFUNC(T, integral) {
    ksffi_ptr self;
    KS_ARGS("self:*", &self, ksffit_ptr);

    return (kso)ks_int_newu((ks_uint)self->val);
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

static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    if (kso_issub(L->type, ksffit_ptr) && !kso_issub(R->type, ksffit_ptr)) {
        int sz = ksffi_sizeofp(L->type);
        if (sz < 0) return NULL;

        if (!kso_is_int(R)) {
            KS_THROW(kst_TypeError, "For pointer arithmetic, a pointer and integral value are required (not '%T')", R);
            return NULL;
        }

        ks_cint ri;
        if (!kso_get_ci(R, &ri)) return NULL;

        void* res = (void*)((ks_uint)((ksffi_ptr)L)->val + sz * ri);
        return (kso)ksffi_wrap(L->type, &res);

    } else if (!kso_issub(L->type, ksffit_ptr) && kso_issub(R->type, ksffit_ptr)) {
        int sz = ksffi_sizeofp(R->type);
        if (sz < 0) return NULL;

        if (!kso_is_int(L)) {
            KS_THROW(kst_TypeError, "For pointer arithmetic, a pointer and integral value are required (not '%T')", L);
            return NULL;
        }

        ks_cint li;
        if (!kso_get_ci(L, &li)) return NULL;
        
        void* res = (void*)(sz * li + (ks_uint)((ksffi_ptr)R)->val);
        return (kso)ksffi_wrap(L->type, &res);

    } else {
        KS_THROW(kst_TypeError, "Cannot add 2 pointers");
        return NULL;
    }
}

/* Export */

static struct ks_type_s tp;
ks_type ksffit_ptr = &tp;
ks_type ksffit_ptr_void;

void _ksi_ffi_ptr() {
    _ksinit(ksffit_ptr, kst_number, T_NAME, sizeof(struct ksffi_ptr_s), -1, "C-style pointer", KS_IKV(
        {"__template",             (kso)ks_tuple_new(1, (kso[]){ KSO_NONE })},

        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, val=0)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__integral",             ksf_wrap(T_integral_, T_NAME ".__integral(self)", "")},
        {"__add",                  ksf_wrap(T_add_, T_NAME ".__add(L, R)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, idx)", "")},
        {"__setelem",              ksf_wrap(T_setelem_, T_NAME ".__setelem(self, idx, val)", "")},

        {"__sizeof",               (kso)ks_int_newu(sizeof(void*))},
        {"of",                     KS_NEWREF(kst_none)},

    ));

    ksffit_ptr_void = ksffi_ptr_make(NULL);
}

