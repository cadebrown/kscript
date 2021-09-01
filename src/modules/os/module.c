/* modules/os/module.c - 'os' module implementation
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define M_MODULE ksg_os
#define M_NAME "os"
#define M_DOC "Operating system utilities"


/* C-API */


/* Functions */



/* Export */

KS_DECL_MODULE(M_MODULE)

void ksi_os() {

    ks_init_module(M_MODULE, NULL, M_NAME, M_DOC, KS_IKV(

    ));
    
}
