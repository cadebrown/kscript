/* libc/main.c - 'libc' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/libc.h>

#define M_NAME "libc"

/* C-API */

kslibc_dll kslibc_open(ks_type tp, ks_str name) {
    void* handle = dlopen(name->data, RTLD_LAZY);
    if (!handle) {
        KS_THROW(kst_IOError, "Failed to dlopen %R: %s", name, dlerror());
        return NULL;
    }

    kslibc_dll self = KSO_NEW(kslibc_dll, tp);

    KS_INCREF(name);
    self->name = name;
    self->handle = handle;

    return self;
}

/* Module functions */


static KS_TFUNC(M, open) {
    ks_str name;
    KS_ARGS("name:*", &name, kst_str);

    return (kso)kslibc_open(kslibct_dll, name);
}

/* Export */

ks_module _ksi_libc() {
    _ksi_libc_ints();
    _ksi_libc_fp();
    _ksi_libc_dll();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "C library and FFI wrappers", KS_IKV(
        
        /* Types */
        
        {"DLL",                    KS_NEWREF(kslibct_dll)},

        {"schar",                  KS_NEWREF(kslibct_schar)},
        {"uchar",                  KS_NEWREF(kslibct_uchar)},
        {"sshort",                 KS_NEWREF(kslibct_sshort)},
        {"ushort",                 KS_NEWREF(kslibct_ushort)},
        {"sint",                   KS_NEWREF(kslibct_sint)},
        {"uint",                   KS_NEWREF(kslibct_uint)},
        {"slong",                  KS_NEWREF(kslibct_slong)},
        {"ulong",                  KS_NEWREF(kslibct_ulong)},
        {"slonglong",              KS_NEWREF(kslibct_slonglong)},
        {"ulonglong",              KS_NEWREF(kslibct_ulonglong)},


        /* Functions */

        {"open",                   ksf_wrap(M_open_, M_NAME ".open(name)", "Opens a dynamically loaded library")},

    ));

    return res;
}
