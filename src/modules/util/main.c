/* main.c - source for the 'util' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/util.h>

#define M_NAME "util"


/* C-API */

/* Export */

ks_module _ksi_util() {
    _ksi_queue();
    _ksi_bitset();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'util' - utility module", KS_IKV(
        /* Constants */
        {"Queue",  (kso)kst_queue},
        {"Bitset",  (kso)kst_bitset},

        /* Types */
        
        /* Functions */

    ));

    return res;
}
