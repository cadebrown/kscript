/* module.c - source code for the built-in 'mm' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/mm.h>

#define M_NAME "mm"


#ifndef KS_HAVE_libav
#warning Building kscript without libav support, so threading is disabled
#endif



/* C-API */



/* Export */

ks_module _ksi_mm() {

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'mm' - multimedia module\n\n    This module implements common media operations", KS_IKV(
        /* Types */

        /* Functions */

    ));

    return res;
}
