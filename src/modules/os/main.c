/* os/main.c - 'os' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "os"

/* C-API */



kso ksos_getenv(ks_str name, kso defa) {
    /* TODO: detect getenv */
    char* res = getenv(name->data);

    if (res == NULL) {
        if (defa) {
            return KS_NEWREF(defa);
        } else {
            KS_THROW(kst_KeyError, "Key %R not present in the environment", name);
            return NULL;
        }
    } else {
        return (kso)ks_str_new(-1, res);
    }
}

bool ksos_setenv(ks_str name, ks_str val) {
    ks_ssize_t sl = name->len_b + val->len_b + 4;
    char* tmp = malloc(sl);

    snprintf(tmp, sl - 1, "%s=%s", name->data, val->data);

    /* TODO: detect setenv/putenv */
    int rs = putenv(tmp);

    if (rs != 0) {
        KS_THROW(kst_OSError, "Failed to set %R in environment: %s", name, strerror(errno));
        return false;
    }else {
        return true;
    }
}


/* Module Functions */

static KS_TFUNC(M, getenv) {
    ks_str key;
    kso defa = NULL;
    KS_ARGS("key:* ?defa", &key, kst_str, &defa);

    return ksos_getenv(key, defa);
}


static KS_TFUNC(M, setenv) {
    ks_str key, val;
    KS_ARGS("key:* val:*", &key, kst_str, &val, kst_str);

    if (!ksos_setenv(key, val)) return NULL;

    return KSO_NONE;
}

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

        {"stdin",                  KS_NEWREF(ksos_stdin)},
        {"stdout",                 KS_NEWREF(ksos_stdout)},
        {"stderr",                 KS_NEWREF(ksos_stderr)},
    
        /* Functions */
        {"getenv",                 ksf_wrap(M_getenv_, M_NAME ".getenv(key, defa=none)", "Retrieves the environment entry indicated by 'key', or a default if it was not found\n\n    If 'defa' was not given, then an error is thrown")},
        {"setenv",                 ksf_wrap(M_setenv_, M_NAME ".setenv(key, val)", "Sets an environment entry to another string value")},
    ));


    return res;
}
