/* libc/ints.c - 'libc' integer types
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/libc.h>

#define T_NAME "libc.DLL"


kso kslibc_new_int(ks_type tp, ks_cint val) {
    #define _KSCASE(_name, _ctp) else if (kso_issub(tp, kslibct_##_name)) { \
        kslibc_##_name r = KSO_NEW(kslibc_##_name, tp); \
        r->val = val; \
        return (kso)r; \
    }

    if (false) {}
    KSLIBC_DO_INTS(_KSCASE);
    #undef _KSCASE

    KS_THROW(kst_TypeError, "Failed to create C-style integer of type '%T'", tp);
    return NULL;
}
kso kslibc_new_intu(ks_type tp, ks_uint val) {
    #define _KSCASE(_name, _ctp) else if (kso_issub(tp, kslibct_##_name)) { \
        kslibc_##_name r = KSO_NEW(kslibc_##_name, tp); \
        r->val = val; \
        return (kso)r; \
    }

    if (false) {}
    KSLIBC_DO_INTS(_KSCASE);
    #undef _KSCASE

    KS_THROW(kst_TypeError, "Failed to create C-style integer of type '%T'", tp);
    return NULL;
}

/* Type definitions */

#define _KSCASE(_name, _ctp) \
static KS_TFUNC(T_##_name, new) { \
    ks_type tp; \
    ks_cint val = 0; \
    KS_ARGS("tp:* val:cint", &tp, kst_type, &val); \
    kslibc_##_name self = KSO_NEW(kslibc_##_name, tp); \
    self->val = val; \
    return (kso)self; \
} \
static KS_TFUNC(T_##_name, str) { \
    kslibc_##_name self; \
    KS_ARGS("self:*", &self, kslibct_##_name); \
    return (kso)ks_fmt("%l", (ks_cint)self->val);\
} \
static KS_TFUNC(T_##_name, integral) { \
    kslibc_##_name self; \
    KS_ARGS("self:*", &self, kslibct_##_name); \
    return (kso)ks_int_new(self->val); \
}

KSLIBC_DO_INTS(_KSCASE)

#undef _KSCASE


/* Export */

#define _KSCASE(_name, _ctp) \
static struct ks_type_s tp_##_name; \
ks_type kslibct_##_name = &tp_##_name;

KSLIBC_DO_INTS(_KSCASE)

#undef _KSCASE


void _ksi_libc_ints() {

    #define _KSCASE(_name, _ctp) \
    _ksinit(kslibct_##_name, kst_number, "libc." #_name, sizeof(struct kslibc_##_name##_s), -1, "C-style integer of type " #_ctp, KS_IKV( \
        {"__new",                  ksf_wrap(T_##_name##_new_, "libc." #_name ".__new(tp, val=0)", "")}, \
        {"__repr",                 ksf_wrap(T_##_name##_str_, "libc." #_name ".__repr(self)", "")}, \
        {"__str",                  ksf_wrap(T_##_name##_str_, "libc." #_name ".__str(self)", "")}, \
        {"__integral",             ksf_wrap(T_##_name##_integral_, "libc." #_name ".__integral(self)", "")}, \
    ));

    KSLIBC_DO_INTS(_KSCASE)

    #undef _KSCASE

}


