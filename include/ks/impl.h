/* ks/impl.h - kscript internal implementation header
 *
 * NOTE: THIS FILE IS FOR INTERNAL USE ONLY... MAY BREAK OTHER PROJECTS
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_IMPL_H
#define KS_IMPL_H

/* kscript C-API */
#include <ks/ks.h>





void ksi_object();
void ksi_dotdotdot();
void ksi_undefined();
void ksi_none();

void ksi_number();
void ksi_int();
void ksi_enum();
void ksi_bool();
void ksi_float();
void ksi_complex();
void ksi_rational();

void ksi_str();
void ksi_bytes();
void ksi_regex();
void ksi_range();
void ksi_slice();

void ksi_list();
void ksi_tuple();
void ksi_set();
void ksi_dict();

void ksi_map();
void ksi_filter();
void ksi_zip();
void ksi_batch();

void ksi_type();
void ksi_func(); /* + func.partial */
void ksi_exc();

void ksi_logger();
void ksi_ast();
void ksi_code();

void ksi_queue();
void ksi_bst();
void ksi_graph();


/** Modules **/

void ksi_os();
void ksi_os_mutex();
void ksi_os_thread();
void ksi_os_frame();
void ksi_os_proc();
void ksi_os_stat();
void ksi_os_path();
void ksi_os_walk();

void ksi_io();
void ksi_io_io();
void ksi_io_stringio();
void ksi_io_bytesio();
void ksi_io_streamio();

void ksi_net();
void ksi_net_socketio();
void ksi_net_http();
void ksi_net_http_req();
void ksi_net_http_resp();
void ksi_net_http_server();

void ksi_ffi();
void ksi_ffi_lib();
void ksi_ffi_ptr();
void ksi_ffi_func();
void ksi_ffi_struct();
void ksi_ffi_int();
void ksi_ffi_float();

void ksi_time();
void ksi_time_delta();
void ksi_time_datetime();

void ksi_ucd();

void ksi_gram();
void ksi_gram_token();
void ksi_gram_lexer();

void ksi_m();

void ksi_nx();
void ksi_nx_dtype();
void ksi_nx_array();
void ksi_nx_view();
void ksi_nx_la();
void ksi_nx_rand();
void ksi_nx_rand_state();
void ksi_nx_fft();
void ksi_nx_fft_plan();


#endif /* KS_IMPL_H */
