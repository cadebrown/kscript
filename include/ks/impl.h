/* ks/impl.h - internal implementation header, only included by internal kscript files
 *
 * Don't include this! Just include `ks.h` for the officially supported API. This header
 *   may define things outside of the standard 'ks_' and 'KS_' namespace.
 *
 * For kscript devs only!
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
void _ksi_attrtuple();
void _ksi_set();
void _ksi_dict();
void _ksi_names();
void _ksi_graph();

void _ksi_module();
void _ksi_type();
void _ksi_func();
void _ksi_logger();

void _ksi_ast();
void _ksi_code();

void _ksi_map();
void _ksi_filter();
void _ksi_enumerate();

void _ksi_Exception();

ks_module _ksi_io();
void _ksi_io_BaseIO();
void _ksi_io_RawIO();
void _ksi_io_FileIO();
void _ksi_io_StringIO();
void _ksi_io_BytesIO();

ks_module _ksi_os();
void _ksi_os_stat();
void _ksi_os_mutex();
void _ksi_os_thread();
void _ksi_os_path();
void _ksi_os_walk();
void _ksi_os_frame();
void _ksi_os_proc();

ks_module _ksi_m();

ks_module _ksi_getarg();
void _ksi_getarg_Parser();

ks_module _ksi_time();
void _ksi_time_DateTime();

ks_module _ksi_net();
void _ksi_net_SocketIO();
ks_module _ksi_net_http();
void _ksi_net_http_req();
void _ksi_net_http_resp();
void _ksi_net_http_server();

ks_module _ksi_ffi();
void _ksi_ffi_dll();
void _ksi_ffi_func();
void _ksi_ffi_ptr();
void _ksi_ffi_ints();
void _ksi_ffi_floats();

ks_module _ksi_nx();
void _ksi_nx_dtype();
void _ksi_nx_array();
void _ksi_nx_view();
ks_module _ksi_nx_la();
ks_module _ksi_nx_fft();
void _ksi_nx_fft_plan();

ks_module _ksi_util();
void _ksi_queue();
void _ksi_bitset();
void _ksi_bst();

void _ksi_nk_Context();
void _ksi_nk_Image();

ks_module _ksi_nxrand();
void _ksi_nxrand_State();

ks_module _ksi_re();
ks_module _ksi_ucd();

ks_module _ksi_av();
void _ksi_av_IO();
void _ksi_av_Stream();

void _ksi_parser();
void _ksi_funcs();
void _ksi_import();

ks_module _ksi_gram();
void _ksi_gram_Token();
void _ksi_gram_Lexer();


/* Initialize type */
void _ksinit(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv);


/* String constants */
KS_API_DATA ks_str
    _ksv_io,
    _ksv_os,
    _ksv_getarg,

    _ksv_attrtuplemap,
    _ksv_attrtuplemaplist,
    _ksv_stdin,
    _ksv_stdout,
    _ksv_stderr,

#define _KSACT(_attr) _ksva##_attr,
_KS_DO_SPEC(_KSACT)
#undef _KSACT

    _ksva__src,
    _ksva__sig,
    _ksva__doc,


    _ksv_expr,
    _ksv_empty,
    _ksv_r,
    _ksv_rb,
    _ksv_w,
    _ksv_wb
;


KS_API_DATA ks_tuple
    _ksv_emptytuple
;

/* Integer constants */
KS_API_DATA ks_int
    _ksint_0,
    _ksint_1,
    _ksint_HASH_P /* KS_HASH_P */
;


#endif /* KS_IMPL_H__ */
