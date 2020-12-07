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
    ));

    ks_dict_merge_ikv(self->attr, ikv);

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_module self;
    KS_ARGS("self:*", &self, kst_module);

    
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


/* Export */

static struct ks_type_s tp;
ks_type kst_module = &tp;

void _ksi_module() {
    _ksinit(kst_module, kst_object, T_NAME, sizeof(struct ks_module_s), offsetof(struct ks_module_s, attr), "Modules are organizations of code which may be imported, and seperated", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
    ));
}
