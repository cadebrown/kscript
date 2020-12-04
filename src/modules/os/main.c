/* os/main.c - 'os' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "os"

/* C-API */

/* Export */


ksio_FileIO
    ksos_stdin,
    ksos_stdout,
    ksos_stderr
;

ks_module _ksi_os() {
    _ksi_os_mutex();
    _ksi_os_thread();
    _ksi_os_path();

    ksos_stdin = ksio_FileIO_wrap(ksiot_FileIO, stdin, false, true, false, false, _ksv_stdin);
    ksos_stdout = ksio_FileIO_wrap(ksiot_FileIO, stdout, false, false, true, false, _ksv_stdout);
    ksos_stderr = ksio_FileIO_wrap(ksiot_FileIO, stderr, false, false, true, false, _ksv_stderr);

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Operating system interface", KS_IKV(

    ));


    return res;
}
