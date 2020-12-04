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
    _ksv_r
;


static ks_module
    G_io = NULL,
    G_os = NULL
;


bool ks_init() {
    if (has_init) return true;


    /* Initialize types */

    _ksi_str();
    _ksi_bytes();
    _ksi_object();
    _ksi_type();
    
    _ksi_number();
      _ksi_int();
        _ksi_enum();
          _ksi_bool();
      _ksi_float();
        _ksi_complex();

    _ksi_none();

    _ksi_dict();
    _ksi_list();
    _ksi_tuple();
    _ksi_module();
    _ksi_Exception();

    /* String constants */
    #define _CONST(_v, _s) _v = ks_str_new(sizeof(_s) - 1, _s);

    _CONST(_ksv_io, "io");
    _CONST(_ksv_os, "os");
    _CONST(_ksv_r, "r");
    _CONST(_ksv_stdin, "stdin");
    _CONST(_ksv_stdout, "stdout");
    _CONST(_ksv_stderr, "stderr");

    /* Initialize standard modules */
    G_io = ks_import(_ksv_io);
    assert(G_io != NULL);
    G_os = ks_import(_ksv_os);
    assert(G_os != NULL);

    return has_init = true;
}

bool ks_has_init() {
    return has_init;
}
