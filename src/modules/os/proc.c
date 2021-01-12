/* os/proc.c - 'os.proc' type
 *
 *
 * SEE: https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output
 * 
 * @author:    Gregory Croisdale <greg@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "os.proc"

/* C-API */


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

#ifdef WIN32
	KS_THROW(kst_OSError, "Process spawning not supported on Windows yet...");
	ks_free(cargv);
	return NULL;
#else
    /* Create pipes */
    int pin[2] = { -1, -1 };
    int pout[2] = { -1, -1 };
    int perr[2] = { -1, -1 };

    /* Close all created so far */
    #define CLOSEALL() do { \
        if (pin[0] >= 0) close(pin[0]); \
        if (pin[1] >= 0) close(pin[1]); \
        if (pout[0] >= 0) close(pout[0]); \
        if (pout[1] >= 0) close(pout[1]); \
        if (perr[0] >= 0) close(perr[0]); \
        if (perr[1] >= 0) close(perr[1]); \
    } while (0)


    /* Attempt to open 3 new pipes (TODO: allow specifying which) */
    if (!ksos_pipe(&pin[0], &pin[1]) < 0 || !ksos_pipe(&pout[0], &pout[1]) || !ksos_pipe(&perr[0], &perr[1])) {
        CLOSEALL();
        ks_free(cargv);
        return NULL;
    }

    /* Fork the process, so we can communicate */
    int pid;
    if ((pid = ksos_fork()) < 0) {
        CLOSEALL();
        ks_free(cargv);
        return NULL;
    }

    if (pid == 0) {
        /* We are the child process */

        /* Duplicate those existing pipes we created before we forked, so the child process writes to those */
        if (!ksos_dup(pin[0], STDIN_FILENO) || !ksos_dup(pout[1], STDOUT_FILENO) || !ksos_dup(perr[1], STDERR_FILENO)) {
            CLOSEALL();
            return NULL;
        }

        /* Close everything, since we are done and now the stdin/stdout/stderr are set up */
        CLOSEALL();

        /* Execute the process */
        int rc = execvp(cargv[0], cargv);
        exit(0);
    }

    /* Close just the unused pipes */
    close(pin[0]);
    close(pout[1]);
    close(perr[1]);

    ks_str v_in_src = ks_fmt("%i:stdin", self->pid);
    ks_str v_out_src = ks_fmt("%i:stdout", self->pid);
    ks_str v_err_src = ks_fmt("%i:stderr", self->pid);

    /* Set the PID */
    self->pid = pid;

    self->v_in = ksio_FileIO_fdopen(pin[1], v_in_src, _ksv_w);
    self->v_out = ksio_FileIO_fdopen(pout[0], v_out_src, _ksv_r);
    self->v_err = ksio_FileIO_fdopen(perr[0], v_err_src, _ksv_r);

    KS_DECREF(v_in_src);
    KS_DECREF(v_out_src);
    KS_DECREF(v_err_src);
    free(cargv);

    return KSO_NONE;
#endif
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

    int res;
    if (!ksos_waitpid(self->pid, &res)) return NULL;

    return (kso) ks_int_new(res);
}

static KS_TFUNC(T, isalive) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

    bool g;
    if (!ksos_isalive(self->pid, &g)) return NULL;

    return KSO_BOOL(g);
}

static KS_TFUNC(T, signal) {
    ksos_proc self;
    ks_cint sig;
    KS_ARGS("self:* signal:cint", &self, ksost_proc, &sig);

    if (!ksos_signal(self->pid, sig)) return NULL;

    return KSO_NONE;
}

static KS_TFUNC(T, kill) {
    ksos_proc self;
    KS_ARGS("self:*", &self, ksost_proc);

#ifdef SIGKILL
	if (!ksos_signal(self->pid, SIGKILL)) return NULL;
#else
	if (!ksos_signal(self->pid, 9)) return NULL;
#endif

    return KSO_NONE;
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
        {"signal",                 ksf_wrap(T_signal_, T_NAME ".signal(self, code)", "Sends a signal to process")},
        {"kill",                   ksf_wrap(T_kill_, T_NAME ".kill(self)", "Attempts to kill the process (by sending 'SIGKILL', typically 9) to the process")},
    ));
}