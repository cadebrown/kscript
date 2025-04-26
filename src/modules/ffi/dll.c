/* ffi/dll.c - ffi.DLL type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/ksffi.h>

#define T_NAME "ffi.DLL"

/* C-API */

/* Internal method to load a symbol from a DLL */
static bool my_load_sym(ksffi_dll self, ks_str name, void** res) {

#ifdef WIN32
	*res = GetProcAddress(self->handle, name->data);
	if (!*res) {
		KS_THROW(kst_IOError, "Failed to load name %R: [%i]", name, GetLastError());
		return false;
	}
	
#else
    *res = dlsym(self->handle, name->data);
    if (!*res) {
        KS_THROW(kst_IOError, "Failed to load name %R: %s", name, dlerror());
        return false;
    }
#endif

    return true;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksffi_dll self;
    KS_ARGS("self:*", &self, ksffit_dll);

    KS_DECREF(self->src);

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

    return (kso)ks_fmt("<%T src=%R, handle=%p>", self, self->src, self->handle);
}

static KS_TFUNC(T, load) {
    ksffi_dll self;
    ks_str name;
    ks_type of = NULL;
    KS_ARGS("self:* name:* ?of:*", &self, ksffit_dll, &name, kst_str, &of, kst_type);

    void* res;
    if (!my_load_sym(self, name, &res)) {
        return NULL;
    }

    ks_type restype = of;
    return (kso)ksffi_wrap(restype, &res);
}

static KS_TFUNC(T, getattr) {
    ksffi_dll self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksffit_dll, &attr, kst_str);

    void* res;
    if (!my_load_sym(self, attr, &res)) {
        return NULL;
    }

    ks_type restype = ksffi_ptr_make(NULL);
    if (!restype) {
        return NULL;
    }

    KS_DECREF(restype);
    return (kso)ksffi_wrap(restype, res);
}


/* Export */

static struct ks_type_s tp;
ks_type ksffit_dll = &tp;


void _ksi_ffi_dll() {

    _ksinit(ksffit_dll, kst_object, T_NAME, sizeof(struct ksffi_dll_s), -1, "Dynamically loaded library (DLL), which wraps a C module", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        //{"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__getelem",              ksf_wrap(T_getattr_, T_NAME ".__getelem(self, attr)", "")},

        {"load",                   ksf_wrap(T_load_, T_NAME ".load(self, name, of=ffi.ptr[none])", "Loads a symbol, as a pointer type")},

    ));
}
