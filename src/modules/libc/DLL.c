/* libc/DLL.c - 'libc.DLL' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/libc.h>

#define T_NAME "libc.DLL"


/* C-API */


/* Type Functions */

static KS_TFUNC(T, free) {
    kslibc_dll self;
    KS_ARGS("self:*", &self, kslibct_dll);

    KS_DECREF(self->name);

    if (self->handle) dlclose(self->handle);

    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, str) {
    kslibc_dll self;
    KS_ARGS("self:*", &self, kslibct_dll);

    return (kso)ks_fmt("<%T name=%R, handle=%p>", self, self->name, self->handle);
}

static KS_TFUNC(T, getelem) {
    kslibc_dll self;
    ks_str name;
    KS_ARGS("self:* name:*", &self, kslibct_dll, &name, kst_str);

    void (*sym)() = dlsym(self->handle, name->data);
    if (!sym) {
        KS_THROW(kst_IOError, "Failed to load name %R: %s", name, dlerror());
        return NULL;
    }

    return (kso)kslibc_fp_wrap(kslibct_fp, sym, kslibct_sint, _ksv_emptytuple);
    return (kso)ks_int_newu((ks_uint)sym);
}


/* Export */

static struct ks_type_s tp;
ks_type kslibct_dll = &tp;


void _ksi_libc_dll() {

    _ksinit(kslibct_dll, kst_object, T_NAME, sizeof(struct kslibc_dll_s), -1, "Dynamically loaded library (DLL), which wraps a C module", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, name)", "")},
    ));
}
