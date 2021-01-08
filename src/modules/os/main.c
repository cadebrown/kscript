/* os/main.c - 'os' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 *             Gregory Croisdale <greg@kscript.org>
 */
#include <ks/impl.h>

#define M_NAME "os"

/* C-API */

kso ksos_getenv(ks_str name, kso defa) {
#ifdef KS_HAVE_getenv
    char* res = getenv(name->data);
    if (res) {
        return (kso)ks_str_new(-1, res);
    } else {
        if (defa) {
            return KS_NEWREF(defa);
        } else {
            KS_THROW(kst_KeyError, "Key %R not present in the environment", name);
            return NULL;
        }
    }
#else
    KS_THROW(kst_OSError, "Failed to getenv %R: platform did not provide a 'getenv()' function", name);
    return false;
#endif
}

bool ksos_setenv(ks_str name, ks_str val) {
#ifdef KS_HAVE_putenv
    ks_ssize_t sl = name->len_b + val->len_b + 4;
    char* tmp = malloc(sl);

    snprintf(tmp, sl - 1, "%s=%s", name->data, val->data);


    int rc = putenv(tmp);
    if (rc != 0) {
        KS_THROW(kst_OSError, "Failed to set %R in environment: %s", name, strerror(errno));
        return false;
    } else {
        return true;
    }
#else
    KS_THROW(kst_OSError, "Failed to getenv %R: platform did not provide a 'putenv()' function", name);
    return false;
#endif
}

ksos_path ksos_getcwd() {
#ifdef KS_HAVE_getcwd
    char buf[KSOS_PATH_MAX + 1];
    
    if (getcwd(buf, sizeof(buf) - 1) == NULL) {
        KS_THROW(kst_OSError, "Failed to getcwd: %s", strerror(errno));
        return NULL;
    } else {
        return ksos_path_new(-1, buf, KSO_NONE);
    }

#else
    KS_THROW(kst_OSError, "Failed to getcwd: platform did not provide a 'getcwd()' function");
    return false;
#endif
}

int ksos_exec(ks_str cmd) {
#ifdef KS_HAVE_system
    int res = system(cmd->data);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to exec %R: %s", cmd, strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to exec %R: platform did not provide a 'system()' function", cmd);
    return -1;
#endif
}

int ksos_fork() {
#ifdef KS_HAVE_fork
    int res = fork();

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to fork: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to fork: platform did not provide a 'fork()' function");
    return -1;
#endif
}

int ksos_pipe(int* fd) {
#ifdef KS_HAVE_pipe
    int res = pipe(fd);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to pipe: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to pipe: platform did not provide a 'pipe(int*)' function");
    return -1;
#endif
}

int ksos_dup2(int oldfd, int newfd) {
#ifdef KS_HAVE_dup2
    int res = dup2(oldfd, newfd);

    if (res < 0) {
        KS_THROW(kst_OSError, "Failed to dup2: %s", strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to dup2: platform did not provide a 'dup2(int, int)' function");
    return -1;
#endif
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

static KS_TFUNC(M, getcwd) {
    KS_ARGS("");
    return (kso) ksos_getcwd();
}

static KS_TFUNC(M, exec) {
    ks_str cmd;

    KS_ARGS("cmd:*", &cmd, kst_str);

    int res = ksos_exec(cmd);
    if (res < 0) return NULL;

    return (kso) ks_int_new(res);
}

static KS_TFUNC(M, fork) {
    int res = ksos_fork();
    if (res < 0) return NULL;

    return (kso) ks_int_new(res);
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
    _ksi_os_proc();

    ksos_argv = ks_list_new(0, NULL);
    ks_str tmp = ks_str_new(1, "-");
    ks_list_push(ksos_argv, (kso)tmp);
    KS_DECREF(tmp);

    ksos_stdin = ksio_FileIO_wrap(ksiot_FileIO, stdin, false, _ksv_stdin, _ksv_r);
    ksos_stdout = ksio_FileIO_wrap(ksiot_FileIO, stdout, false, _ksv_stdout, _ksv_w);
    ksos_stderr = ksio_FileIO_wrap(ksiot_FileIO, stderr, false, _ksv_stderr, _ksv_w);

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "Operating system interface", KS_IKV(

        /* Types */
        {"path",                   KS_NEWREF(ksost_path)},

        {"proc",                   KS_NEWREF(ksost_proc)},

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
        {"getcwd",                 ksf_wrap(M_getcwd_, M_NAME ".getcwd()", "Returns current working directory")},
        {"exec",                   ksf_wrap(M_exec_, M_NAME ".exec(cmd)", "Attempts to execute a command as if typed in console - returns exit code")},
        {"fork",                   ksf_wrap(M_fork_, M_NAME ".fork()", "Creates a new process by duplicating the calling process - returns 0 in the child, PID > 0 in the parent, and -1 if there was an error")},
    ));


    return res;
}
