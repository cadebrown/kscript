/* kpm/cext/main.c - 'kpm.cext' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/kpm.h>

#define M_NAME "kpm.cext"

/* C-API */


/* Module functions */

/* Export */

ks_module _ksi_kpm_cext() {

    _ksi_kpm_cext_project();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "C-extension manager", KS_IKV(
        
        /* Types */

        {"Project",                (kso)kpm_cextt_project},

        /* Aliases */

        /* Functions */

    ));

    return res;
}
