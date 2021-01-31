/* types/Exception.c - 'Exception' type (and subtypes)
 *
 * 
 * TODO: OSError template enum: https://stackoverflow.com/questions/19885360/how-can-i-print-the-symbolic-name-of-an-errno-in-c
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "Exception"

/* C-API */

ks_Exception ks_Exception_new_c(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ks_Exception res = ks_Exception_new_cv(tp, cfile, cfunc, cline, fmt, ap);
    va_end(ap);
    return res;
}

ks_Exception ks_Exception_new_cv(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, va_list ap) {
    assert(kso_issub(tp, kst_Exception));
    ks_str what = ks_fmtv(fmt, ap);
    if (!what) return NULL;

    ks_Exception self = KSO_NEW(ks_Exception, tp);

    self->inner = NULL;
    self->frames = ks_list_new(0, NULL);

    self->args = ks_list_new(0, NULL);
    self->what = what;

    return self;
}



/* Type Functions */

static KS_TFUNC(T, init) {
    ks_Exception self;
    ks_str what = NULL;
    KS_ARGS("self:* ?what:*", &self, kst_Exception, &what, kst_str);

    self->inner = NULL;
    self->frames = ks_list_new(0, NULL);
    if (what) {
        KS_INCREF(what);
        self->what = what;
    } else {
        self->what = ks_str_new(0, NULL);
    } 
    self->args = ks_list_new(0, NULL);

    return KSO_NONE;
}

static KS_TFUNC(T, free) {
    ks_Exception self;
    KS_ARGS("self:*", &self, kst_Exception);

    KS_DECREF(self->frames);
    KS_DECREF(self->args);
    KS_DECREF(self->what);

    if (self->inner) KS_DECREF(self->inner);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ks_Exception self;
    KS_ARGS("self:*", &self, kst_Exception);

    return KS_NEWREF(self->what);
}

static KS_TFUNC(T, repr) {
    ks_Exception self;
    KS_ARGS("self:*", &self, kst_Exception);

    return (kso)ks_fmt("%T(%R, %R, %R, %R)", self, self->what, self->args, self->frames, self->inner ? self->inner : KSO_NONE);
}

/* Export */

static struct ks_type_s tp;
ks_type kst_Exception = &tp;

/* Expects 'name' */
#define DO_SUBTYPES(_macro) \
    _macro(OutOfIterException, Exception) \
    _macro(Error, Exception) \
    _macro(InternalError, Error) \
    _macro(SyntaxError, Error) \
    _macro(ImportError, Error) \
    _macro(TypeError, Error) \
    _macro(TemplateError, TypeError) \
    _macro(NameError, Error) \
    _macro(AttrError, Error) \
    _macro(KeyError, Error) \
    _macro(IndexError, KeyError) \
    _macro(ValError, Error) \
    _macro(AssertError, Error) \
    _macro(MathError, ValError) \
    _macro(OverflowError, Error) \
    _macro(ArgError, Error) \
    _macro(SizeError, Error) \
    _macro(IOError, Error) \
    _macro(OSError, Error) \
    _macro(Warning, Exception) \
    _macro(PlatformWarning, Warning) \
    _macro(SyntaxWarning, Warning) \


#define DECL(_name, _par) \
    static struct ks_type_s itp_##_name; \
    ks_type kst_##_name = &itp_##_name; 

DO_SUBTYPES(DECL)


void _ksi_Exception() {
    _ksinit(kst_Exception, kst_object, T_NAME, sizeof(struct ks_Exception_s), -1, "Exception, which is an anomoly/error, is something that can be 'thrown' up the call stack. Only objects of types deriving from 'Exception' may be thrown\n\n    To throw an exception, use the 'throw' statement, like so: 'throw Exception(\"Reasoning here\")'", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__init",               ksf_wrap(T_init_, T_NAME ".__init(self, what='')", "")},
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",               ksf_wrap(T_repr_, T_NAME ".__repr(self)", "")},
    ));
    
    #define INIT(_name, _par) \
        _ksinit(kst_##_name, kst_##_par, #_name, sizeof(struct ks_Exception_s), -1, "", NULL);
         
    DO_SUBTYPES(INIT)


    /* For errno */
    ks_type_set(kst_OSError, _ksva__template, (kso)ks_tuple_new(1, (kso[]){ KSO_NONE }));

}
