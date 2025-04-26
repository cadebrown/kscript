/* ffi/float.c - FFI float types
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ksffi.h>

/* Type definitions */

#define _KSCASE(_name, _type) \
static KS_TFUNC(T_##_name, new) { \
    ks_type tp; \
    ks_cfloat val = 0; \
    KS_ARGS("tp:* ?val:cfloat", &tp, kst_type, &val); \
    ksffi_##_name self = KSO_NEW(ksffi_##_name, tp); \
    self->val = val; \
    return (kso)self; \
} \
static KS_TFUNC(T_##_name, str) { \
    ksffi_##_name self; \
    KS_ARGS("self:*", &self, ksffit_##_name); \
    return (kso)ks_fmt("%f", (ks_cfloat)self->val);\
} \
static KS_TFUNC(T_##_name, float) { \
    ksffi_##_name self; \
    KS_ARGS("self:*", &self, ksffit_##_name); \
    return (kso)ks_float_new(self->val); \
}

KSFFI_DO_F(_KSCASE)

#undef _KSCASE


/* Export */

#define _KSCASE(_name, _type) \
static struct ks_type_s tp_##_name; \
ks_type ksffit_##_name = &tp_##_name;

KSFFI_DO_F(_KSCASE)

#undef _KSCASE


void _ksi_ffi_float() {

    #define _KSCASE(_name, _type) \
    _ksinit(ksffit_##_name, kst_number, "ffi." #_name, sizeof(struct ksffi_##_name##_s), -1, "C-style float of type " #_type, KS_IKV( \
        {"__new",                  ksf_wrap(T_##_name##_new_, "ffi." #_name ".__new(tp, val=0)", "")}, \
        {"__repr",                 ksf_wrap(T_##_name##_str_, "ffi." #_name ".__repr(self)", "")}, \
        {"__str",                  ksf_wrap(T_##_name##_str_, "ffi." #_name ".__str(self)", "")}, \
        {"__float",                ksf_wrap(T_##_name##_float_, "ffi." #_name ".__float(self)", "")}, \
        {"__sizeof",               (kso)ks_int_newu(sizeof(_type))}, \
    ));

    KSFFI_DO_F(_KSCASE)

    #undef _KSCASE

}


