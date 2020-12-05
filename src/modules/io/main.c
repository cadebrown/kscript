/* io/main.c - 'io' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "io"

/* C-API */

/* Export */

ks_module _ksi_io() {
    _ksi_io_FileIO();
    _ksi_io_StringIO();
    _ksi_io_BytesIO();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Input/output utilities", KS_IKV(

    ));

    return res;
}
