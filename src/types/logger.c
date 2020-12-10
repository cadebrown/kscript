/* types/logger.c - 'logger' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>

#define T_NAME "logger"



/* Internals */

/* Dictionary of all loggers created thus far, keyed on '(type, name)' */
static ks_dict all_loggers = NULL;
static ks_type E_level = NULL;

/* Colors */
static const char
    *I_col_TRACE = KS_COL_LGRY "TRACE",
    *I_col_DEBUG = KS_COL_LGRY "DEBUG",
    *I_col_INFO  = KS_COL_LBLU "INFO ",
    *I_col_WARN  = KS_COL_YLW  "WARN ",
    *I_col_ERROR = KS_COL_RED  "ERROR",
    *I_col_FATAL = KS_COL_RED KS_COL_BOLD "FATAL"
;

bool ks_logger_clogv(ks_logger self, int level, const char* file, const char* func, int line, const char* fmt, va_list ap) {
    ks_cint sl = self->level;
    if (sl <= 0) {
        /* Don't print anything */
        return true;
    }

    bool res = true;

    if (level >= self->level) {
        /* Silence output to remove recursion */
        self->level = -1;

        const char* col = "";
        /**/ if (level <= KS_LOGGER_TRACE) col = I_col_TRACE;
        else if (level <= KS_LOGGER_DEBUG) col = I_col_DEBUG;
        else if (level <= KS_LOGGER_INFO) col = I_col_INFO;
        else if (level <= KS_LOGGER_WARN) col = I_col_WARN;
        else if (level <= KS_LOGGER_ERROR) col = I_col_ERROR;
        else if (level <= KS_LOGGER_FATAL) col = I_col_FATAL;

        /* Actually add output */
        if (!ksio_add(self->output, KS_COL_RESET "[" KS_COL_MGA "%S" KS_COL_RESET ":%s" KS_COL_RESET "] ", self->name, col)) {
            res = false;
        } else {


            if (level >= KS_LOGGER_WARN) {
                /* print additional information (location in source code) */
                if (line >= 0 && file != NULL) {
                    ksio_add(self->output, "[" KS_COL_LBLU "@" "%s" KS_COL_RESET ":" KS_COL_LCYN "%i", file, line);
                }
                if (level >= KS_LOGGER_ERROR) {
                    /* add function */
                    if (func != NULL) {
                        ksio_add(self->output, KS_COL_RESET " in " KS_COL_BLU "%s()", func);
                    }
                }

                if (line >= 0 && file != NULL) {
                    ksio_add(self->output, KS_COL_RESET "] ");
                }
            }

            if (!ksio_addv(self->output, fmt, ap)) {
                res = false;
            } else {
                /* We're good */
                ksio_add(self->output, "\n");
            }
        }

        /* Restore level */
        self->level = sl;
    }

    return res;
}

bool ks_logger_clog(ks_logger self, int level, const char* file, const char* func, int line, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = ks_logger_clogv(self, level, file, func, line, fmt, ap);
    va_end(ap);
    return res;
}


/* Looks up information */
bool ks_logger_klog(ks_logger self, int level, ksos_frame frame, const char* fmt, ...) {
    ks_str fname = NULL, func = NULL;
    int line = -1;
    if (frame) {
        if (!ksos_frame_get_info(frame, &fname, &func, &line)) {
        }
    }

    va_list ap;
    va_start(ap, fmt);
    bool res = ks_logger_clogv(self, level, fname?fname->data:NULL, func?func->data:NULL, line, fmt, ap);
    va_end(ap);
    return res;
}

/* C-API */

ks_logger ks_logger_gett(ks_type tp, ks_str name) {
    ks_tuple key = ks_tuple_new(2, (kso[]){ (kso)tp, (kso)name });
    ks_logger res = (ks_logger)ks_dict_get(all_loggers, (kso)key);
    if (res) {
        KS_DECREF(key);
        return res;
    } else {
        kso_catch_ignore();

        /* Create a new logger */
        res = KSO_NEW(ks_logger, tp);

        KS_INCREF(name);
        res->name = name;
        KS_INCREF(ksos_stderr);
        res->output = (kso)ksos_stderr;

        res->level = KS_LOGGER_WARN;

        ks_dict_set(all_loggers, (kso)key, (kso)res);
        KS_DECREF(key);
        return res;
    }
}

ks_logger ks_logger_get(ks_str name) {
    return ks_logger_gett(kst_logger, name);
}
ks_logger ks_logger_get_c(const char* name) {
    ks_str t0 = ks_str_new(-1, name);
    ks_logger res = ks_logger_get(t0);
    KS_DECREF(t0);
    return res;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_logger self;
    KS_ARGS("self:*", &self, kst_logger);

    KS_DECREF(self->name);
    KS_DECREF(self->output);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    ks_str name = NULL;
    KS_ARGS("tp:* ?name:*", &tp, kst_type, &name, kst_str);

    if (!name) {
        /* Figure out logger */
        name = ks_str_new(-1, "default");
    } else {
        KS_INCREF(name);
    }
    ks_logger res = ks_logger_gett(tp, name);
    KS_DECREF(name);
    return (kso)res;
}


static KS_TFUNC(T, str) {
    ks_logger self;
    KS_ARGS("self:*", &self, kst_logger);

    return (kso)ks_fmt("<logger %R to %R>", self->name, self->output);
}


static KS_TFUNC(T, getattr) {
    ks_logger self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_logger, &attr, kst_str);

    if (ks_str_eq_c(attr, "level", 5)) {
        ks_enum res = ks_enum_get(E_level, self->level);
        if (res) {
            return (kso)res;
        }  else {
            kso_catch_ignore();
            return (kso)ks_int_new(self->level);
        }
    } else if (ks_str_eq_c(attr, "out", 3)) {
        return KS_NEWREF(self->output);
    } else if (ks_str_eq_c(attr, "name", 4)) {
        return KS_NEWREF(self->name);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, setattr) {
    ks_logger self;
    ks_str attr;
    kso val;
    KS_ARGS("self:* attr:* val", &self, kst_logger, &attr, kst_str, &val);

    if (ks_str_eq_c(attr, "level", 5)) {
        if (!kso_get_ci(val, &self->level)) return NULL;
        return KSO_NONE;
    } else if (ks_str_eq_c(attr, "out", 3)) {
        KS_INCREF(val);
        KS_DECREF(self->output);
        self->output = val;
        return KSO_NONE;
    } else if (ks_str_eq_c(attr, "name", 4)) {
        if (!kso_issub(val->type, kst_str)) {
            KS_THROW(kst_TypeError, "Logger '.name' must be a 'str', but got '%T' object", val);
            return NULL;
        }
        KS_INCREF(val);
        KS_DECREF(self->name);
        self->name = (ks_str)val;
        return KSO_NONE;
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, log) {
    ks_logger self;
    ks_cint level;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* level:cint *objs", &self, kst_logger, &level, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, level, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, trace) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_TRACE, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, debug) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_DEBUG, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, info) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_INFO, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, warn) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_WARN, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, error) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_ERROR, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}

static KS_TFUNC(T, fatal) {
    ks_logger self;
    int n_objs;
    kso* objs;
    KS_ARGS("self:* *objs", &self, kst_logger, &n_objs, &objs);
    ksos_thread th = ksos_thread_get();
    if (!ks_logger_klog(self, KS_LOGGER_FATAL, (ksos_frame)(th->frames->len>1?th->frames->elems[th->frames->len - 2]:NULL), "%J", " ", n_objs, objs)) return NULL;
    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_logger = &tp;


void _ksi_logger() {
    E_level = ks_enum_make(T_NAME ".levels", KS_EIKV(
        {"SILENT",                 KS_LOGGER_SILENT},
        {"TRACE",                  KS_LOGGER_TRACE},
        {"DEBUG",                  KS_LOGGER_DEBUG},
        {"INFO",                   KS_LOGGER_INFO},
        {"WARN",                   KS_LOGGER_WARN},
        {"ERROR",                  KS_LOGGER_ERROR},
        {"FATAL",                  KS_LOGGER_FATAL},
    ));

    all_loggers = ks_dict_new(NULL);
    _ksinit(kst_logger, kst_object, T_NAME, sizeof(struct ks_logger_s), -1, "Logger objects can be used to format output and print to a stream to dispay messages", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, name=none)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__setattr",            ksf_wrap(T_setattr_, T_NAME ".__setattr(self, attr, val)", "")},

        {"levels",               KS_NEWREF(E_level)},

        {"log",                  ksf_wrap(T_log_, T_NAME ".log(self, level, *objs)", "Logs a message with a given logging level (it is ignored if the logger's level is greater than level)")},
        {"trace",                ksf_wrap(T_trace_, T_NAME ".trace(self, *objs)", "Emits a trace message")},
        {"debug",                ksf_wrap(T_debug_, T_NAME ".debug(self, *objs)", "Emits a debug message")},
        {"info",                 ksf_wrap(T_info_, T_NAME ".info(self, *objs)", "Emits a info message")},
        {"warn",                 ksf_wrap(T_warn_, T_NAME ".warn(self, *objs)", "Emits a warning message")},
        {"error",                ksf_wrap(T_error_, T_NAME ".error(self, *objs)", "Emits a error message")},
        {"fatal",                ksf_wrap(T_fatal_, T_NAME ".fatal(self, *objs)", "Emits a fatal message")},

    ));
}
