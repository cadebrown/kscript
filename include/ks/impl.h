/* ks/impl.h - internal implementation header, only included by internal kscript files
 *
 * Don't include this! Just include `ks.h` for the officially supported API. This header
 *   may define things outside of the standard 'ks_' and 'KS_' namespace.
 *
 * @author:    Cade Brown <cade@kscript.org>
 * @license:   GPLv3
 */
#pragma once
#ifndef KS_IMPL_H__
#define KS_IMPL_H__

#ifndef KS_H__
 #include <ks/ks.h>
#endif


/** Initializer functions (internal use only) **/

void _ksi_object();

void _ksi_none();
void _ksi_undefined();
void _ksi_dotdotdot();
void _ksi_number();
void _ksi_int();
void _ksi_bool();
void _ksi_enum();
void _ksi_float();
void _ksi_complex();
void _ksi_rational();

void _ksi_str();
void _ksi_bytes();
void _ksi_regex();

void _ksi_range();
void _ksi_slice();

void _ksi_list();
void _ksi_tuple();
void _ksi_set();
void _ksi_dict();
void _ksi_names();
void _ksi_graph();

void _ksi_module();
void _ksi_type();
void _ksi_func();
void _ksi_partial();

void _ksi_ast();
void _ksi_code();

void _ksi_map();
void _ksi_filter();
void _ksi_enumerate();

void _ksi_Exception();

ks_module _ksi_io();
void _ksi_io_BaseIO();
void _ksi_io_FileIO();
void _ksi_io_StringIO();
void _ksi_io_BytesIO();

ks_module _ksi_os();
void _ksi_os_mutex();
void _ksi_os_thread();
void _ksi_os_path();
void _ksi_os_frame();

ks_module _ksi_m();

ks_module _ksi_getarg();
void _ksi_getarg_Parser();

ks_module _ksi_time();
void _ksi_time_struct();

ks_module _ksi_net();
void _ksi_net_SocketIO();
ks_module _ksi_net_http();


ks_module _ksi_nx();
ks_module _ksi_re();
ks_module _ksi_gram();
ks_module _ksi_ucd();

void _ksi_parser();
void _ksi_funcs();
void _ksi_import();

/* Initialize type */
void _ksinit(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv);


/* String constants */
KS_API extern ks_str
    _ksv_io,
    _ksv_os,
    _ksv_getarg,

    _ksv_stdin,
    _ksv_stdout,
    _ksv_stderr,

#define _KSACT(_attr) _ksva##_attr,
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _ksva__src,
    _ksva__sig,
    _ksva__doc,


    _ksv_r
;


KS_API extern ks_tuple
    _ksv_emptytuple
;

/* Integer constants */
KS_API extern ks_int
    _ksint_0,
    _ksint_1,
    _ksint_HASH_P /* KS_HASH_P */
;


#endif /* KS_IMPL_H__ */
