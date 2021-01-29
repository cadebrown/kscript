/* ks/nxt.h - NumeriX template header library
 * 
 * Used internally to provide static functions and macros for code generation
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNXT_H__
#define KSNXT_H__

#include <ks/nx.h>


/* Paste '_loop' applied to each integer datatype
 *
 * Calls '_loop(TYPE, NAME)'
 */
#define NXT_PASTE_I(_loop) \
    _loop( nx_bl, bl) \
    _loop( nx_s8, s8) \
    _loop( nx_u8, u8) \
    _loop(nx_s16, s16) \
    _loop(nx_u16, u16) \
    _loop(nx_s32, s32) \
    _loop(nx_u32, u32) \
    _loop(nx_s64, s64) \
    _loop(nx_u64, u64) \

/* Paste '_loop' applied to each floating point datatype
 *
 * Calls '_loop(TYPE, NAME)'
 */
#define NXT_PASTE_F(_loop) \
    _loop(nx_F, F) \
    _loop(nx_D, D) \
    _loop(nx_E, E) \
    _loop(nx_Q, Q) \

/* Paste '_loop' applied to each complex floating point datatype
 *
 * Calls '_loop(TYPE, NAME)'
 */
#define NXT_PASTE_C(_loop) \
    _loop(nx_cF, cF) \
    _loop(nx_cD, cD) \
    _loop(nx_cE, cE) \
    _loop(nx_cQ, cQ) \


/* Paste all numeric types */
#define NXT_PASTE_IFC(_loop) \
    NXT_PASTE_I(_loop) \
    NXT_PASTE_F(_loop) \
    NXT_PASTE_C(_loop) \

#define NXT_PASTE_IF(_loop) \
    NXT_PASTE_I(_loop) \
    NXT_PASTE_F(_loop) \

#define NXT_PASTE_FC(_loop) \
    NXT_PASTE_F(_loop) \
    NXT_PASTE_C(_loop) \

#endif /* KSNXT_H__ */
