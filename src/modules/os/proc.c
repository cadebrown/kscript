/* os/proc.c - 'os.proc' type
 *
 * 
 * @author:    Gregory Croisdale <greg@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.proc"

int ksos_waitpid(pid_t pid);

int ksos_kill(pid_t pid, int sig);

int ksos_isalive(pid_t pid);

/* C-API */

int ksos_waitpid(pid_t pid) {
#ifdef KS_HAVE_waitpid
    int status;

    if (waitpid(pid, &status, 0) < 0) {
        KS_THROW(kst_OSError, "Failed to waitpid: error using waitpid(%i)", pid);
    }

    return status;
#else
    KS_THROW(kst_OSError, "Failed to waitpid: platform did not provide a 'waitpid()' function");
    return -1;
#endif
}

int ksos_kill(pid_t pid, int sig) {
#ifdef KS_HAVE_kill
    int res = kill(pid, sig);

    if (res < 0) {
            KS_THROW(kst_OSError, "Failed to kill(%i, %i): %s", pid, sig, strerror(errno));
    }

    return res;
#else
    KS_THROW(kst_OSError, "Failed to kill: platform did not provide a 'kill()' function");
    return -1;
#endif
}

int ksos_isalive(pid_t pid) {
#ifdef KS_HAVE_kill
    // attempt send empty signal
    int res = kill(pid, 0);

    // if res < 0 and ESRCH, pid has no process. Otherwise, print error.a
    if (res < 0) {
        if (errno == ESRCH) return 0;
        else {
            KS_THROW(kst_OSError, "Failed to kill(%i, %i): %s", pid, 0, strerror(errno));
        }
    // if res >= 0, pid has a process
    } else {
        return 1;
    }
#else
    KS_THROW(kst_OSError, "Failed to isalive(%i): platform did not provide a 'kill()' function", pid);
    return -1;
#endif
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    KS_NDECREF(self->argv);

    KS_NDECREF(self->v_in);
    KS_NDECREF(self->v_out);
    KS_NDECREF(self->v_err);

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, init) {
    ksos_proc self;
    kso args;

    KS_ARGS("self:* argv", &self, ksost_proc, &args);

    self->argv = NULL;

    if (kso_issub(args->type,  kst_str)) {
        ks_list tmp = ks_str_split_c(((ks_str)args)->data, " ");
        self->argv = ks_tuple_newi((kso) tmp);
        KS_DECREF(tmp);
    } else {
        self->argv = ks_tuple_newi(args);
    }

    if (!self->argv) return NULL;

    char** cargv = ks_malloc((self->argv->len + 1) * sizeof(*cargv));
    cargv[self->argv->len] = NULL;
    
    int i;
    for (i = 0; i < self->argv->len; i++) {
        ks_str s = (ks_str) self->argv->elems[i];

        if (!kso_issub(s->type, kst_str)) {
            ks_free(cargv);
            KS_THROW(kst_TypeError, "expected all arguments to be of type 'str', but got object of type '%T'", s->type);
            return NULL;
        }

        cargv[i] = s->data;
    }

    int cstdin[2]  = {-1, -1};
    int cstdout[2] = {-1, -1};
    int cstderr[2] = {-1, -1};

    int res;

    /* attempt to open 3 pipes and fork
    */
    if (ksos_pipe(cstdin) < 0 || ksos_pipe(cstdout) < 0 || ksos_pipe(cstderr) < 0 || (res = ksos_fork()) < 0) {
        ks_free(cargv);

        if (cstdin[0] >= 0) close(cstdin[0]);
        if (cstdin[1] >= 0) close(cstdin[1]);
        if (cstdout[0] >= 0) close(cstdout[0]);
        if (cstdout[1] >= 0) close(cstdout[1]);
        if (cstderr[0] >= 0) close(cstderr[0]);
        if (cstderr[1] >= 0) close(cstderr[1]);
        return NULL;
    }

    if (res == 0) {
        /* close irrelevant fds and replace stdin, stdout, stderr with pipe
        */
        close(cstdin[1]);
        close(cstdout[0]);
        close(cstderr[0]);

        if (ksos_dup2(cstdin[0], STDIN_FILENO) < 0) exit(1);
        if (ksos_dup2(cstdout[1], STDOUT_FILENO) < 0) exit(1);
        if (ksos_dup2(cstderr[1], STDERR_FILENO) < 0) exit(1);

        exit(execv(cargv[0], cargv + 1));
        assert(false);
    }
    self->pid = res;

    close(cstdin[0]);
    close(cstdout[1]);
    close(cstderr[1]);

    ks_str v_in_src = ks_fmt("%i:stdin", self->pid);
    ks_str v_out_src = ks_fmt("%i:stdout", self->pid);
    ks_str v_err_src = ks_fmt("%i:stderr", self->pid);

    self->v_in = ksio_FileIO_fdopen(cstdin[1], false, true, true, v_in_src);
    self->v_out = ksio_FileIO_fdopen(cstdout[0], true, false, true, v_out_src);
    self->v_err = ksio_FileIO_fdopen(cstderr[0], true, false, true, v_err_src);

    KS_DECREF(v_in_src);
    KS_DECREF(v_out_src);
    KS_DECREF(v_err_src);

    return KSO_NONE;
}

static KS_TFUNC(T, getattr) {
    ksos_proc self;
    ks_str attr;

    KS_ARGS("self:* attr:*", &self, ksost_proc, &attr, kst_str);

    if (ks_str_eq_c(attr, "argv", sizeof("argv") - 1)) {
        return KS_NEWREF(self->argv);
    } else if (ks_str_eq_c(attr, "pid", sizeof("pid") - 1)) {
        return (kso)ks_int_new(self->pid);
    } else if (ks_str_eq_c(attr, "stdin", sizeof("stdin") - 1)) {
        return KS_NEWREF(self->v_in);
    } else if (ks_str_eq_c(attr, "stdout", sizeof("stdout") - 1)) {
        return KS_NEWREF(self->v_out);
    } else if (ks_str_eq_c(attr, "stderr", sizeof("stderr") - 1)) {
        return KS_NEWREF(self->v_err);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, str) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    return (kso) ks_fmt("<%T: %R (pid=%i)>", self, self->argv, (int)self->pid);
}

static KS_TFUNC(T, join) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    return (kso) ks_int_new(ksos_waitpid(self->pid));
}

static KS_TFUNC(T, isalive) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    if (ksos_kill(self->pid, 0) < 0) {
        if (errno == ESRCH) (kso) ks_int_new(0);
        else {
            KS_THROW(kst_OSError, "Failed to kill(%i, %i): %s", self->pid, 0, strerror(errno));
        }
    } else {
        return (kso) ks_int_new(1);
    }
}

static KS_TFUNC(T, signal) {
    ksos_proc self;
    ks_cint sig;
    KS_ARGS("self:* signal:cint", &self, ksost_proc, &sig);

    return (kso) ks_int_new(ksos_kill(self->pid, (int) sig));
}

static KS_TFUNC(T, kill) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    return (kso) ks_int_new(ksos_kill(self->pid, SIGKILL));
}

/* Export */

static struct ks_type_s tp;
ks_type ksost_proc = &tp;

void _ksi_os_proc() {

    _ksinit(ksost_proc, kst_object, T_NAME, sizeof(struct ksos_proc_s), -1, "A process which can be executed and redirected", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(T_init_, T_NAME ".__init(self, argv)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},

        {"join",                   ksf_wrap(T_join_, T_NAME ".join(self)", "Waits for the process to finish executing and returns the return code")},
        {"isalive",                ksf_wrap(T_isalive_, T_NAME ".isalive(self)", "Returns True if process is still running, False if dead")},
        {"signal",                 ksf_wrap(T_signal_, T_NAME ".signal(self, code)", "Sends a signal to process and returns the return code")},
        {"kill",                   ksf_wrap(T_kill_, T_NAME ".kill(self)", "Sends -9 to the process and returns the return code")},
    ));
}