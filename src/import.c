/* import.c - Module importing in kscript
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>



ks_module ks_import(ks_str name) {
    
    #define _BIMOD(_str) else if (ks_str_eq_c(name, #_str, sizeof(#_str) - 1)) { \
        return _ksi_##_str(); \
    }

    if (false) {}

    _BIMOD(io)
    _BIMOD(os)


    /* No builtin module found */

    KS_THROW(kst_ImportError, "Failed to import %R", name);
    return NULL;
}
