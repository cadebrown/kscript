/* rand/main.c - 'nx.rand' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define M_NAME "nx.rand"

/* C-API */


/* Export */

ks_module _ksi_nxrand() {
    _ksi_nxrand_State();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Random module", KS_IKV(

        /* Types */

        {"State",                  (kso)nxrandt_State},

        /* Functions */
    
    ));

    return res;
}
