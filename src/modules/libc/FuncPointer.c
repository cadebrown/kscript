/* libc/FuncPointer.c - 'libc.FuncPointer' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/libc.h>

#define T_NAME "libc.FuncPointer"


/* C-API */

kslibc_fp kslibc_fp_wrap(ks_type tp, void (*val)(), ks_type restype, ks_tuple argtypes) {
    kslibc_fp self = KSO_NEW(kslibc_fp, tp);

    self->val = val;
    KS_INCREF(restype);
    self->restype = restype;
    KS_INCREF(argtypes);
    self->argtypes = argtypes;


    return self;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    kslibc_fp self;
    KS_ARGS("self:*", &self, kslibct_fp);

    KS_DECREF(self->restype);
    KS_DECREF(self->argtypes);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    kslibc_fp self;
    KS_ARGS("self:*", &self, kslibct_fp);

    return (kso)ks_fmt("<%T (%p) %R%R>", self, *(void**)&self->val, self->restype, self->argtypes);
}

static KS_TFUNC(T, call) {
    kslibc_fp self;
    int nargs;
    kso* args;
    KS_ARGS("self:* ?args", &self, kslibct_fp, &nargs, &args);

    int stat = ((int(*)(const char*))self->val)("example\n");

    return (kso)ks_int_new(stat);


    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type kslibct_fp = &tp;


void _ksi_libc_fp() {

    _ksinit(kslibct_fp, kst_object, T_NAME, sizeof(struct kslibc_fp_s), -1, "C-style function pointer", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__call",                 ksf_wrap(T_call_, T_NAME ".__call(self, *args)", "")},

    ));
}
