/* init.c - initializes kscript types, functions, globals, and so forth
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

static bool has_init = false;

ks_str
    _ksv_io,
    _ksv_os,
    _ksv_stdin,
    _ksv_stdout,
    _ksv_stderr,

#define _KSACT(_attr) _ksva##_attr,
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _ksv_r
;

ks_tuple
    _ksv_emptytuple
;

ks_dict
    ksg_globals
;

static ks_module
    G_io = NULL,
    G_os = NULL
;


bool ks_init() {
    if (has_init) return true;


    kst_func->ob_sz = sizeof(struct ks_func_s);
    kst_str->ob_sz = sizeof(struct ks_str_s);

    /* Initialize types */

    /* String constants */
    #define _CONST(_v, _s) _v = ks_str_new(sizeof(_s) - 1, _s);

#define _KSACT(_attr) _CONST(_ksva##_attr, #_attr);
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _CONST(_ksv_io, "io");
    _CONST(_ksv_os, "os");
    _CONST(_ksv_r, "r");
    _CONST(_ksv_stdin, "stdin");
    _CONST(_ksv_stdout, "stdout");
    _CONST(_ksv_stderr, "stderr");

    _ksi_object();
    _ksi_type();
    
    _ksi_number();
      _ksi_int();
        _ksi_enum();
          _ksi_bool();
      _ksi_float();
        _ksi_complex();

    _ksi_none();

    _ksi_str();
    _ksi_bytes();

    _ksi_dict();
    _ksi_list();
    _ksi_tuple();
    _ksi_module();

    _ksi_func();
    _ksi_names();
 
    _ksi_Exception();

    _ksv_emptytuple = ks_tuple_new(0, NULL);

    _ksi_ast();
    _ksi_code();

    _ksi_parser();
    _ksi_funcs();

    /* Initialize standard modules */
    G_io = ks_import(_ksv_io);
    assert(G_io != NULL);
    G_os = ks_import(_ksv_os);
    assert(G_os != NULL);

    ksg_globals = ks_dict_new(KS_IKV(
        {"object",                 (kso)kst_object},

        {"module",                 (kso)kst_module},
        {"type",                   (kso)kst_type},
        {"func",                   (kso)kst_func},

        {"int",                    (kso)kst_int},
        {"enum",                   (kso)kst_enum},
        {"bool",                   (kso)kst_bool},
        {"float",                  (kso)kst_float},
        {"complex",                (kso)kst_complex},

        {"str",                    (kso)kst_str},
        {"bytes",                  (kso)kst_bytes},

        {"list",                   (kso)kst_list},
        {"tuple",                  (kso)kst_tuple},
        {"set",                    (kso)kst_tuple},
        {"dict",                   (kso)kst_tuple},

        /* Exception Types */
        {"Exception", (kso)kst_Exception},
        {"OutOfIterException", (kso)kst_OutOfIterException},
        {"Error", (kso)kst_Error},
        {"InternalError", (kso)kst_InternalError},
        {"SyntaxError", (kso)kst_SyntaxError},
        {"ImportError", (kso)kst_ImportError},
        {"TypeError", (kso)kst_TypeError},
        {"NameError", (kso)kst_NameError},
        {"AttrError", (kso)kst_AttrError},
        {"KeyError", (kso)kst_KeyError},
        {"IndexError", (kso)kst_IndexError},
        {"ValError", (kso)kst_ValError},
        {"AssertError", (kso)kst_AssertError},
        {"MathError", (kso)kst_MathError},
        {"OverflowError", (kso)kst_OverflowError},
        {"ArgError", (kso)kst_ArgError},
        {"SizeError", (kso)kst_SizeError},
        {"IOError", (kso)kst_IOError},
        {"OSError", (kso)kst_OSError},
        {"Warning", (kso)kst_Warning},
        {"PlatformWarning", (kso)kst_PlatformWarning},
        {"SyntaxWarning", (kso)kst_SyntaxWarning},


        /* Functions */
        {"print", (kso)ksf_print},
    ));

    return has_init = true;
}

bool ks_has_init() {
    return has_init;
}
