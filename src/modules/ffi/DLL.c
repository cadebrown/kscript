/* ffi/DLL.c - 'ffi.DLL' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ffi.h>

#define T_NAME "ffi.DLL"


/* C-API */


/* Type Functions */

static KS_TFUNC(T, free) {
    ksffi_dll self;
    KS_ARGS("self:*", &self, ksffit_dll);

    KS_DECREF(self->name);

#ifdef WIN32
	if (self->handle) FreeLibrary(self->handle);
#else
	if (self->handle) dlclose(self->handle);
#endif

    KSO_DEL(self);

    return KSO_NONE;
}


static KS_TFUNC(T, str) {
    ksffi_dll self;
    KS_ARGS("self:*", &self, ksffit_dll);

    return (kso)ks_fmt("<%T name=%R, handle=%p>", self, self->name, self->handle);
}

static KS_TFUNC(T, getattr) {
    ksffi_dll self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksffit_dll, &attr, kst_str);

#ifdef WIN32
	void* sym = GetProcAddress(self->handle, attr->data);
	if (!sym) {
		KS_THROW(kst_IOError, "Failed to load name %R: [%i]", attr, GetLastError());
		return NULL;
	}
	
	return (kso)ksffi_ptr_make(NULL, sym);
#else
    void* sym = dlsym(self->handle, attr->data);
    if (!sym) {
        KS_THROW(kst_IOError, "Failed to load name %R: %s", attr, dlerror());
        return NULL;
    }
	return (kso)ksffi_ptr_make(NULL, sym);
#endif

    //return (kso)ks_int_newu((ks_uint)sym);
}


/* Export */

static struct ks_type_s tp;
ks_type ksffit_dll = &tp;


void _ksi_ffi_dll() {

    _ksinit(ksffit_dll, kst_object, T_NAME, sizeof(struct ksffi_dll_s), -1, "Dynamically loaded library (DLL), which wraps a C module", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__getelem",              ksf_wrap(T_getattr_, T_NAME ".__getelem(self, attr)", "")},
    ));
}
