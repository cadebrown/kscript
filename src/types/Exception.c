/* types/Exception.c - 'Exception' type (and subtypes)
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

    self->args = ks_list_new(0, NULL);
    self->what = what;

    self->inner = NULL;

    return self;
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
    _macro(NameError, Error) \
    _macro(AttrError, Error) \
    _macro(KeyError, Error) \
    _macro(IndexError, Error) \
    _macro(ValError, Error) \
    _macro(AssertError, Error) \
    _macro(MathError, Error) \
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
    _ksinit(kst_Exception, kst_object, T_NAME, sizeof(struct ks_Exception_s), -1, KS_IKV(
        
    ));
    
    #define INIT(_name, _par) \
        _ksinit(kst_##_name, kst_##_par, #_name, sizeof(struct ks_Exception_s), -1, NULL);
         
    DO_SUBTYPES(INIT)

}
