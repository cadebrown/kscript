/* types/module.c - 'module' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "module"


/* C-API */

ks_module ks_module_new(const char* name, const char* source, const char* doc, struct ks_ikv* ikv) {
    ks_module self = KSO_NEW(ks_module, kst_module);

    ks_dict_merge_ikv(self->attr, KS_IKV(
        {"__name",                 (kso)ks_str_new(-1, name)},
        {"__src",                  (kso)ks_str_new(-1, source)},
        {"__doc",                  (kso)ks_str_new(-1, doc)},
        {"__par",                  (kso)ks_str_new(-1, "")},
    ));

    ks_dict_merge_ikv(self->attr, ikv);

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_module self;
    KS_ARGS("self:*", &self, kst_module);

#ifdef WIN32
	if (self->dlhandle) {
		FreeLibrary(self->dlhandle);
	}
#else

    if (self->dlhandle) {
        int rc = dlclose(self->dlhandle);
    }
#endif

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_module self;
    KS_ARGS("self:*", &self, kst_module);

    kso name = ks_dict_get_h(self->attr, (kso)_ksva__name, _ksva__name->v_hash);
    assert(name != NULL);
    kso src = ks_dict_get_h(self->attr, (kso)_ksva__src, _ksva__src->v_hash);
    assert(src != NULL);

    ks_str res = ks_fmt("<%R module from %R>", name, src);

    KS_DECREF(name);
    KS_DECREF(src);

    return (kso)res;
}
static KS_TFUNC(T, getattr) {
    ks_module self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_module, &attr, kst_str);

    kso res = ks_dict_get_ih(self->attr, (kso)attr, attr->v_hash);
    if (res) {
        return res;
    } else {
        /* Attempt to import submodule */
        ks_module submod = ks_import_sub(self, attr);
        if (!submod) {
            kso_catch_ignore();
            KS_THROW_ATTR(self, attr);
            return NULL;
        } else {
            ks_dict_set_h(self->attr, (kso)attr, attr->v_hash, (kso)submod);
            return (kso)submod;
        }
    }

}

/* Export */

static struct ks_type_s tp;
ks_type kst_module = &tp;

void _ksi_module() {
    _ksinit(kst_module, kst_object, T_NAME, sizeof(struct ks_module_s), offsetof(struct ks_module_s, attr), "Modules are organizations of code which may be imported, and seperated", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));
}
