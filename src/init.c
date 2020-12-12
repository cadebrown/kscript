/* init.c - initializes kscript types, functions, globals, and so forth
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

static bool has_init = false;

ks_str
    _ksv_io,
    _ksv_os,
    _ksv_getarg,
    _ksv_stdin,
    _ksv_stdout,
    _ksv_stderr,

    _ksva__src,
    _ksva__sig,
    _ksva__doc,

#define _KSACT(_attr) _ksva##_attr,
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _ksv_r
;

ks_tuple
    _ksv_emptytuple
;

ks_dict
    ksg_config,
    ksg_inter_vars,
    ksg_globals
;

ks_int
    _ksint_0, _ksint_1
;

static ks_module
    G_io = NULL,
    G_os = NULL,
    G_getarg = NULL
;


bool ks_init() {
    if (has_init) return true;


    kst_func->ob_sz = sizeof(struct ks_func_s);
    kst_func->ob_attr = offsetof(struct ks_func_s, attr);
    kst_str->ob_sz = sizeof(struct ks_str_s);

    /* Initialize types */

    /* String constants */
    #define _CONST(_v, _s) _v = ks_str_new(sizeof(_s) - 1, _s);

#define _KSACT(_attr) _CONST(_ksva##_attr, #_attr);
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _CONST(_ksv_io, "io");
    _CONST(_ksv_os, "os");
    _CONST(_ksv_getarg, "getarg");
    _CONST(_ksv_r, "r");
    _CONST(_ksv_stdin, "<stdin>");
    _CONST(_ksv_stdout, "<stdout>");
    _CONST(_ksv_stderr, "<stderr>");

    _CONST(_ksva__src, "__src");
    _CONST(_ksva__sig, "__sig");
    _CONST(_ksva__doc, "__doc");

    _ksi_object();
    _ksi_type();
    
    _ksi_number();
      _ksi_int();
        _ksi_enum();
          _ksi_bool();
      _ksi_float();
        _ksi_complex();

    _ksi_none();
    _ksi_undefined();

    _ksi_str();
    _ksi_bytes();
    _ksi_regex();

    _ksi_range();
    
    _ksi_set();
    _ksi_dict();
    _ksi_graph();
    _ksi_list();
    _ksi_tuple();
    _ksi_module();

    _ksi_func();
    _ksi_names();
 
    _ksi_logger();
 
    _ksi_Exception();

    _ksv_emptytuple = ks_tuple_new(0, NULL);

    _ksi_ast();
    _ksi_code();

    _ksi_parser();
    _ksi_funcs();
    _ksi_import();

    _ksint_0 = ks_int_new(0);
    _ksint_1 = ks_int_new(1);

    /* Initialize standard modules */
    G_io = ks_import(_ksv_io);
    assert(G_io != NULL);
    G_os = ks_import(_ksv_os);
    assert(G_os != NULL);
    G_getarg = ks_import(_ksv_getarg);
    assert(G_getarg != NULL);


    ksg_config = ks_dict_new(KS_IKV(
        {"prompt0",                (kso)ks_str_new(-1, ">>> ")},
        {"prompt1",                (kso)ks_str_new(-1, "... ")},
        {"prompt2",                (kso)ks_str_new(-1, "_")},
    ));


    ksg_inter_vars = ks_dict_new(KS_IKV(
        {"_",                      KSO_NONE},
    ));


    ksg_globals = ks_dict_new(KS_IKV(
        {"__config", KS_NEWREF(ksg_config)},
        {"__argv", KS_NEWREF(ksos_argv)},

        {"object",                 (kso)kst_object},

        {"module",                 (kso)kst_module},
        {"type",                   (kso)kst_type},
        {"func",                   (kso)kst_func},

        {"logger",                 (kso)kst_logger},

        {"int",                    (kso)kst_int},
        {"enum",                   (kso)kst_enum},
        {"bool",                   (kso)kst_bool},
        {"float",                  (kso)kst_float},
        {"complex",                (kso)kst_complex},

        {"str",                    (kso)kst_str},
        {"bytes",                  (kso)kst_bytes},
        {"regex",                  (kso)kst_regex},

        {"range",                  (kso)kst_range},
        {"list",                   (kso)kst_list},
        {"tuple",                  (kso)kst_tuple},
        {"set",                    (kso)kst_set},
        {"dict",                   (kso)kst_dict},
        {"graph",                  (kso)kst_graph},

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
        {"open", (kso)ksf_open},
        
        {"print", (kso)ksf_print},
        {"pow", (kso)ksf_pow},

        {"hash", (kso)ksf_hash},
        {"len", (kso)ksf_len},
        {"repr", (kso)ksf_repr},

        {"iter", (kso)ksf_iter},
        {"next", (kso)ksf_next},
    
        {"chr", (kso)ksf_chr},
        {"ord", (kso)ksf_ord},
    
    ));

    kst_type->refs = KS_REFS_INF;

    return has_init = true;
}

bool ks_has_init() {
    return has_init;
}
