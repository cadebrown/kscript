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
ks_list
    ksos_argv
;

ks_module _ksi_os() {
    _ksi_os_mutex();
    _ksi_os_thread();
    _ksi_os_path();
    _ksi_os_frame();

    ksos_argv = ks_list_new(0, NULL);
    ks_str tmp = ks_str_new(1, "-");
    ks_list_push(ksos_argv, (kso)tmp);
    KS_DECREF(tmp);

    ksos_stdin = ksio_FileIO_wrap(ksiot_FileIO, stdin, false, true, false, false, _ksv_stdin);
    ksos_stdout = ksio_FileIO_wrap(ksiot_FileIO, stdout, false, false, true, false, _ksv_stdout);
    ksos_stderr = ksio_FileIO_wrap(ksiot_FileIO, stderr, false, false, true, false, _ksv_stderr);

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Operating system interface", KS_IKV(

        /* Types */
        {"path",                   KS_NEWREF(ksost_path)},

        {"thread",                 KS_NEWREF(ksost_thread)},
        {"frame",                  KS_NEWREF(ksost_frame)},
        {"mutex",                  KS_NEWREF(ksost_mutex)},

        /* Variables */
        {"argv",                   KS_NEWREF(ksos_argv)},
    
        /* Functions */
    ));


    return res;
}
