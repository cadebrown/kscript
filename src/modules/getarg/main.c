/* module.c - source code for the built-in 'getarg' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/getarg.h>

#define M_NAME "getarg"

/* Internals */

/* C-API */

/* Module Functions */

/* Export */

ks_module _ksi_getarg() {

    _ksi_getarg_Parser();
    
    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'getarg' - argument/option parser\n\n    ", KS_IKV(

        /* Types */
        {"Parser",                 KS_NEWREF(ksgat_Parser)},

        /* Constants */

    ));


    return res;
}
