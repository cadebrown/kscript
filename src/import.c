/* import.c - Module importing in kscript
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


static ks_dict cache = NULL;




ks_module ks_import(ks_str name) {
    assert(cache != NULL);

    ks_module res = (ks_module)ks_dict_get_ih(cache, (kso)name, name->v_hash);
    if (res) return res;

    #define _BIMOD(_str) else if (ks_str_eq_c(name, #_str, sizeof(#_str) - 1)) { \
        res = _ksi_##_str(); \
    }

    if (false) {}

    _BIMOD(io)
    _BIMOD(os)
    _BIMOD(m)
    _BIMOD(getarg)
    _BIMOD(time)

    if (!res) {
        /* No builtin found, so try dynamically searching */
    }

    if (res) {
        ks_dict_set_h(cache, (kso)name, name->v_hash, (kso)res);
        return res;
    } else {
        KS_THROW(kst_ImportError, "Failed to import %R", name);
        return NULL;
    }
}


void _ksi_import() {
    cache = ks_dict_new(NULL);

}
