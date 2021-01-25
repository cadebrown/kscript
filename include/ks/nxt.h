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
    _loop(nx_bl, bl) \
    _loop(nx_s8, s8) \
    _loop(nx_u8, u8) \
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
    _loop(nx_H, H) \
    _loop(nx_F, F) \
    _loop(nx_D, D) \
    _loop(nx_L, L) \
    _loop(nx_Q, Q) \


/* Paste '_loop' applied to each complex floating point datatype
 *
 * Calls '_loop(TYPE, NAME)'
 */
#define NXT_PASTE_C(_loop) \
    _loop(nx_cH, cH) \
    _loop(nx_cF, cF) \
    _loop(nx_cD, cD) \
    _loop(nx_cL, cL) \
    _loop(nx_cQ, cQ) \



/* Perform '_loop' on '_dtype', if it is an int datatype
 *
 * Specifically, calls '_loop(TYPE)', where 'TYPE' is the C-style
 *   integer type
 * 
 */
#define NXT_FOR_I(_dtype, _loop) do { \
    nx_dtype _dt = _dtype; \
    /**/ if (_dt == nxd_bl) { _loop(bl) } \
    else if (_dt == nxd_s8) { _loop(s8) } \
    else if (_dt == nxd_u8) { _loop(u8) } \
    else if (_dt == nxd_s16) { _loop(s16) } \
    else if (_dt == nxd_u16) { _loop(u16) } \
    else if (_dt == nxd_s32) { _loop(s32) } \
    else if (_dt == nxd_u32) { _loop(u32) } \
    else if (_dt == nxd_s64) { _loop(s64) } \
    else if (_dt == nxd_u64) { _loop(u64) } \
} while (0)

/* Perform '_loop' on '_dtype', if it is a float datatype
 *
 * Specifically, calls '_loop(TYPE, NAME)', where 'TYPE' is the C-style
 *   floating point type
 * 
 */
#define NXT_FOR_F(_dtype, _loop) do { \
    nx_dtype _dt = _dtype; \
    /**/ if (_dt == nxd_H) { _loop(H) } \
    else if (_dt == nxd_F) { _loop(F) } \
    else if (_dt == nxd_D) { _loop(D) } \
    else if (_dt == nxd_L) { _loop(L) } \
    else if (_dt == nxd_Q) { _loop(Q) } \
} while (0)

/* Perform '_loop' on '_dtype', if it is a complex datatype
 *
 * Specifically, calls '_loop(TYPE, NAME)', where 'TYPE' is the C-style
 *   complex point type (structure containing 're' and 'im' members)
 * 
 */
#define NXT_FOR_C(_dtype, _loop) do { \
    nx_dtype _dt = _dtype; \
    /**/ if (_dt == nxd_cH) { _loop(cH) } \
    else if (_dt == nxd_cF) { _loop(cF) } \
    else if (_dt == nxd_cD) { _loop(cD) } \
    else if (_dt == nxd_cL) { _loop(cL) } \
    else if (_dt == nxd_cQ) { _loop(cQ) } \
} while (0)


/* Perform '_loop' on all numeric datatypes
 */
#define NXT_FOR_ALL(_dtypeA, _loop) do { \
    nx_dtype _dA = _dtypeA; \
    /**/ if (_dA == nxd_bl) { _loop(bl) } \
    else if (_dA == nxd_s8) { _loop(s8) } \
    else if (_dA == nxd_u8) { _loop(u8) } \
    else if (_dA == nxd_u16) { _loop(u16) } \
    else if (_dA == nxd_s16) { _loop(s16) } \
    else if (_dA == nxd_u32) { _loop(u32) } \
    else if (_dA == nxd_s32) { _loop(s32) } \
    else if (_dA == nxd_u64) { _loop(u64) } \
    else if (_dA == nxd_s64) { _loop(s64) } \
    else if (_dA == nxd_H) { _loop(H) } \
    else if (_dA == nxd_F) { _loop(F) } \
    else if (_dA == nxd_D) { _loop(D) } \
    else if (_dA == nxd_L) { _loop(L) } \
    else if (_dA == nxd_Q) { _loop(Q) } \
    else if (_dA == nxd_cH) { _loop(cH) } \
    else if (_dA == nxd_cF) { _loop(cF) } \
    else if (_dA == nxd_cD) { _loop(cD) } \
    else if (_dA == nxd_cL) { _loop(cL) } \
    else if (_dA == nxd_cQ) { _loop(cQ) } \
} while (0)

/* Perform '_loop' on all numeric datatypes
 */
#define NXT_FOR_ALL2(_dtypeA, _dtypeB, _loop) do { \
    nx_dtype _dA = _dtypeA, _dB = _dtypeB; \
    /**/ if (_dA == nxd_bl && _dB == nxd_bl) { _loop(bl, bl) } \
    else if (_dA == nxd_bl && _dB == nxd_s8) { _loop(bl, s8) } \
    else if (_dA == nxd_bl && _dB == nxd_u8) { _loop(bl, u8) } \
    else if (_dA == nxd_bl && _dB == nxd_u16) { _loop(bl, u16) } \
    else if (_dA == nxd_bl && _dB == nxd_s16) { _loop(bl, s16) } \
    else if (_dA == nxd_bl && _dB == nxd_u32) { _loop(bl, u32) } \
    else if (_dA == nxd_bl && _dB == nxd_s32) { _loop(bl, s32) } \
    else if (_dA == nxd_bl && _dB == nxd_u64) { _loop(bl, u64) } \
    else if (_dA == nxd_bl && _dB == nxd_s64) { _loop(bl, s64) } \
    else if (_dA == nxd_bl && _dB == nxd_H) { _loop(bl, H) } \
    else if (_dA == nxd_bl && _dB == nxd_F) { _loop(bl, F) } \
    else if (_dA == nxd_bl && _dB == nxd_D) { _loop(bl, D) } \
    else if (_dA == nxd_bl && _dB == nxd_L) { _loop(bl, L) } \
    else if (_dA == nxd_bl && _dB == nxd_Q) { _loop(bl, Q) } \
    else if (_dA == nxd_bl && _dB == nxd_cH) { _loop(bl, cH) } \
    else if (_dA == nxd_bl && _dB == nxd_cF) { _loop(bl, cF) } \
    else if (_dA == nxd_bl && _dB == nxd_cD) { _loop(bl, cD) } \
    else if (_dA == nxd_bl && _dB == nxd_cL) { _loop(bl, cL) } \
    else if (_dA == nxd_bl && _dB == nxd_cQ) { _loop(bl, cQ) } \
    else if (_dA == nxd_s8 && _dB == nxd_bl) { _loop(s8, bl) } \
    else if (_dA == nxd_s8 && _dB == nxd_s8) { _loop(s8, s8) } \
    else if (_dA == nxd_s8 && _dB == nxd_u8) { _loop(s8, u8) } \
    else if (_dA == nxd_s8 && _dB == nxd_u16) { _loop(s8, u16) } \
    else if (_dA == nxd_s8 && _dB == nxd_s16) { _loop(s8, s16) } \
    else if (_dA == nxd_s8 && _dB == nxd_u32) { _loop(s8, u32) } \
    else if (_dA == nxd_s8 && _dB == nxd_s32) { _loop(s8, s32) } \
    else if (_dA == nxd_s8 && _dB == nxd_u64) { _loop(s8, u64) } \
    else if (_dA == nxd_s8 && _dB == nxd_s64) { _loop(s8, s64) } \
    else if (_dA == nxd_s8 && _dB == nxd_H) { _loop(s8, H) } \
    else if (_dA == nxd_s8 && _dB == nxd_F) { _loop(s8, F) } \
    else if (_dA == nxd_s8 && _dB == nxd_D) { _loop(s8, D) } \
    else if (_dA == nxd_s8 && _dB == nxd_L) { _loop(s8, L) } \
    else if (_dA == nxd_s8 && _dB == nxd_Q) { _loop(s8, Q) } \
    else if (_dA == nxd_s8 && _dB == nxd_cH) { _loop(s8, cH) } \
    else if (_dA == nxd_s8 && _dB == nxd_cF) { _loop(s8, cF) } \
    else if (_dA == nxd_s8 && _dB == nxd_cD) { _loop(s8, cD) } \
    else if (_dA == nxd_s8 && _dB == nxd_cL) { _loop(s8, cL) } \
    else if (_dA == nxd_s8 && _dB == nxd_cQ) { _loop(s8, cQ) } \
    else if (_dA == nxd_u8 && _dB == nxd_bl) { _loop(u8, bl) } \
    else if (_dA == nxd_u8 && _dB == nxd_s8) { _loop(u8, s8) } \
    else if (_dA == nxd_u8 && _dB == nxd_u8) { _loop(u8, u8) } \
    else if (_dA == nxd_u8 && _dB == nxd_u16) { _loop(u8, u16) } \
    else if (_dA == nxd_u8 && _dB == nxd_s16) { _loop(u8, s16) } \
    else if (_dA == nxd_u8 && _dB == nxd_u32) { _loop(u8, u32) } \
    else if (_dA == nxd_u8 && _dB == nxd_s32) { _loop(u8, s32) } \
    else if (_dA == nxd_u8 && _dB == nxd_u64) { _loop(u8, u64) } \
    else if (_dA == nxd_u8 && _dB == nxd_s64) { _loop(u8, s64) } \
    else if (_dA == nxd_u8 && _dB == nxd_H) { _loop(u8, H) } \
    else if (_dA == nxd_u8 && _dB == nxd_F) { _loop(u8, F) } \
    else if (_dA == nxd_u8 && _dB == nxd_D) { _loop(u8, D) } \
    else if (_dA == nxd_u8 && _dB == nxd_L) { _loop(u8, L) } \
    else if (_dA == nxd_u8 && _dB == nxd_Q) { _loop(u8, Q) } \
    else if (_dA == nxd_u8 && _dB == nxd_cH) { _loop(u8, cH) } \
    else if (_dA == nxd_u8 && _dB == nxd_cF) { _loop(u8, cF) } \
    else if (_dA == nxd_u8 && _dB == nxd_cD) { _loop(u8, cD) } \
    else if (_dA == nxd_u8 && _dB == nxd_cL) { _loop(u8, cL) } \
    else if (_dA == nxd_u8 && _dB == nxd_cQ) { _loop(u8, cQ) } \
    else if (_dA == nxd_u16 && _dB == nxd_bl) { _loop(u16, bl) } \
    else if (_dA == nxd_u16 && _dB == nxd_s8) { _loop(u16, s8) } \
    else if (_dA == nxd_u16 && _dB == nxd_u8) { _loop(u16, u8) } \
    else if (_dA == nxd_u16 && _dB == nxd_u16) { _loop(u16, u16) } \
    else if (_dA == nxd_u16 && _dB == nxd_s16) { _loop(u16, s16) } \
    else if (_dA == nxd_u16 && _dB == nxd_u32) { _loop(u16, u32) } \
    else if (_dA == nxd_u16 && _dB == nxd_s32) { _loop(u16, s32) } \
    else if (_dA == nxd_u16 && _dB == nxd_u64) { _loop(u16, u64) } \
    else if (_dA == nxd_u16 && _dB == nxd_s64) { _loop(u16, s64) } \
    else if (_dA == nxd_u16 && _dB == nxd_H) { _loop(u16, H) } \
    else if (_dA == nxd_u16 && _dB == nxd_F) { _loop(u16, F) } \
    else if (_dA == nxd_u16 && _dB == nxd_D) { _loop(u16, D) } \
    else if (_dA == nxd_u16 && _dB == nxd_L) { _loop(u16, L) } \
    else if (_dA == nxd_u16 && _dB == nxd_Q) { _loop(u16, Q) } \
    else if (_dA == nxd_u16 && _dB == nxd_cH) { _loop(u16, cH) } \
    else if (_dA == nxd_u16 && _dB == nxd_cF) { _loop(u16, cF) } \
    else if (_dA == nxd_u16 && _dB == nxd_cD) { _loop(u16, cD) } \
    else if (_dA == nxd_u16 && _dB == nxd_cL) { _loop(u16, cL) } \
    else if (_dA == nxd_u16 && _dB == nxd_cQ) { _loop(u16, cQ) } \
    else if (_dA == nxd_s16 && _dB == nxd_bl) { _loop(s16, bl) } \
    else if (_dA == nxd_s16 && _dB == nxd_s8) { _loop(s16, s8) } \
    else if (_dA == nxd_s16 && _dB == nxd_u8) { _loop(s16, u8) } \
    else if (_dA == nxd_s16 && _dB == nxd_u16) { _loop(s16, u16) } \
    else if (_dA == nxd_s16 && _dB == nxd_s16) { _loop(s16, s16) } \
    else if (_dA == nxd_s16 && _dB == nxd_u32) { _loop(s16, u32) } \
    else if (_dA == nxd_s16 && _dB == nxd_s32) { _loop(s16, s32) } \
    else if (_dA == nxd_s16 && _dB == nxd_u64) { _loop(s16, u64) } \
    else if (_dA == nxd_s16 && _dB == nxd_s64) { _loop(s16, s64) } \
    else if (_dA == nxd_s16 && _dB == nxd_H) { _loop(s16, H) } \
    else if (_dA == nxd_s16 && _dB == nxd_F) { _loop(s16, F) } \
    else if (_dA == nxd_s16 && _dB == nxd_D) { _loop(s16, D) } \
    else if (_dA == nxd_s16 && _dB == nxd_L) { _loop(s16, L) } \
    else if (_dA == nxd_s16 && _dB == nxd_Q) { _loop(s16, Q) } \
    else if (_dA == nxd_s16 && _dB == nxd_cH) { _loop(s16, cH) } \
    else if (_dA == nxd_s16 && _dB == nxd_cF) { _loop(s16, cF) } \
    else if (_dA == nxd_s16 && _dB == nxd_cD) { _loop(s16, cD) } \
    else if (_dA == nxd_s16 && _dB == nxd_cL) { _loop(s16, cL) } \
    else if (_dA == nxd_s16 && _dB == nxd_cQ) { _loop(s16, cQ) } \
    else if (_dA == nxd_u32 && _dB == nxd_bl) { _loop(u32, bl) } \
    else if (_dA == nxd_u32 && _dB == nxd_s8) { _loop(u32, s8) } \
    else if (_dA == nxd_u32 && _dB == nxd_u8) { _loop(u32, u8) } \
    else if (_dA == nxd_u32 && _dB == nxd_u16) { _loop(u32, u16) } \
    else if (_dA == nxd_u32 && _dB == nxd_s16) { _loop(u32, s16) } \
    else if (_dA == nxd_u32 && _dB == nxd_u32) { _loop(u32, u32) } \
    else if (_dA == nxd_u32 && _dB == nxd_s32) { _loop(u32, s32) } \
    else if (_dA == nxd_u32 && _dB == nxd_u64) { _loop(u32, u64) } \
    else if (_dA == nxd_u32 && _dB == nxd_s64) { _loop(u32, s64) } \
    else if (_dA == nxd_u32 && _dB == nxd_H) { _loop(u32, H) } \
    else if (_dA == nxd_u32 && _dB == nxd_F) { _loop(u32, F) } \
    else if (_dA == nxd_u32 && _dB == nxd_D) { _loop(u32, D) } \
    else if (_dA == nxd_u32 && _dB == nxd_L) { _loop(u32, L) } \
    else if (_dA == nxd_u32 && _dB == nxd_Q) { _loop(u32, Q) } \
    else if (_dA == nxd_u32 && _dB == nxd_cH) { _loop(u32, cH) } \
    else if (_dA == nxd_u32 && _dB == nxd_cF) { _loop(u32, cF) } \
    else if (_dA == nxd_u32 && _dB == nxd_cD) { _loop(u32, cD) } \
    else if (_dA == nxd_u32 && _dB == nxd_cL) { _loop(u32, cL) } \
    else if (_dA == nxd_u32 && _dB == nxd_cQ) { _loop(u32, cQ) } \
    else if (_dA == nxd_s32 && _dB == nxd_bl) { _loop(s32, bl) } \
    else if (_dA == nxd_s32 && _dB == nxd_s8) { _loop(s32, s8) } \
    else if (_dA == nxd_s32 && _dB == nxd_u8) { _loop(s32, u8) } \
    else if (_dA == nxd_s32 && _dB == nxd_u16) { _loop(s32, u16) } \
    else if (_dA == nxd_s32 && _dB == nxd_s16) { _loop(s32, s16) } \
    else if (_dA == nxd_s32 && _dB == nxd_u32) { _loop(s32, u32) } \
    else if (_dA == nxd_s32 && _dB == nxd_s32) { _loop(s32, s32) } \
    else if (_dA == nxd_s32 && _dB == nxd_u64) { _loop(s32, u64) } \
    else if (_dA == nxd_s32 && _dB == nxd_s64) { _loop(s32, s64) } \
    else if (_dA == nxd_s32 && _dB == nxd_H) { _loop(s32, H) } \
    else if (_dA == nxd_s32 && _dB == nxd_F) { _loop(s32, F) } \
    else if (_dA == nxd_s32 && _dB == nxd_D) { _loop(s32, D) } \
    else if (_dA == nxd_s32 && _dB == nxd_L) { _loop(s32, L) } \
    else if (_dA == nxd_s32 && _dB == nxd_Q) { _loop(s32, Q) } \
    else if (_dA == nxd_s32 && _dB == nxd_cH) { _loop(s32, cH) } \
    else if (_dA == nxd_s32 && _dB == nxd_cF) { _loop(s32, cF) } \
    else if (_dA == nxd_s32 && _dB == nxd_cD) { _loop(s32, cD) } \
    else if (_dA == nxd_s32 && _dB == nxd_cL) { _loop(s32, cL) } \
    else if (_dA == nxd_s32 && _dB == nxd_cQ) { _loop(s32, cQ) } \
    else if (_dA == nxd_u64 && _dB == nxd_bl) { _loop(u64, bl) } \
    else if (_dA == nxd_u64 && _dB == nxd_s8) { _loop(u64, s8) } \
    else if (_dA == nxd_u64 && _dB == nxd_u8) { _loop(u64, u8) } \
    else if (_dA == nxd_u64 && _dB == nxd_u16) { _loop(u64, u16) } \
    else if (_dA == nxd_u64 && _dB == nxd_s16) { _loop(u64, s16) } \
    else if (_dA == nxd_u64 && _dB == nxd_u32) { _loop(u64, u32) } \
    else if (_dA == nxd_u64 && _dB == nxd_s32) { _loop(u64, s32) } \
    else if (_dA == nxd_u64 && _dB == nxd_u64) { _loop(u64, u64) } \
    else if (_dA == nxd_u64 && _dB == nxd_s64) { _loop(u64, s64) } \
    else if (_dA == nxd_u64 && _dB == nxd_H) { _loop(u64, H) } \
    else if (_dA == nxd_u64 && _dB == nxd_F) { _loop(u64, F) } \
    else if (_dA == nxd_u64 && _dB == nxd_D) { _loop(u64, D) } \
    else if (_dA == nxd_u64 && _dB == nxd_L) { _loop(u64, L) } \
    else if (_dA == nxd_u64 && _dB == nxd_Q) { _loop(u64, Q) } \
    else if (_dA == nxd_u64 && _dB == nxd_cH) { _loop(u64, cH) } \
    else if (_dA == nxd_u64 && _dB == nxd_cF) { _loop(u64, cF) } \
    else if (_dA == nxd_u64 && _dB == nxd_cD) { _loop(u64, cD) } \
    else if (_dA == nxd_u64 && _dB == nxd_cL) { _loop(u64, cL) } \
    else if (_dA == nxd_u64 && _dB == nxd_cQ) { _loop(u64, cQ) } \
    else if (_dA == nxd_s64 && _dB == nxd_bl) { _loop(s64, bl) } \
    else if (_dA == nxd_s64 && _dB == nxd_s8) { _loop(s64, s8) } \
    else if (_dA == nxd_s64 && _dB == nxd_u8) { _loop(s64, u8) } \
    else if (_dA == nxd_s64 && _dB == nxd_u16) { _loop(s64, u16) } \
    else if (_dA == nxd_s64 && _dB == nxd_s16) { _loop(s64, s16) } \
    else if (_dA == nxd_s64 && _dB == nxd_u32) { _loop(s64, u32) } \
    else if (_dA == nxd_s64 && _dB == nxd_s32) { _loop(s64, s32) } \
    else if (_dA == nxd_s64 && _dB == nxd_u64) { _loop(s64, u64) } \
    else if (_dA == nxd_s64 && _dB == nxd_s64) { _loop(s64, s64) } \
    else if (_dA == nxd_s64 && _dB == nxd_H) { _loop(s64, H) } \
    else if (_dA == nxd_s64 && _dB == nxd_F) { _loop(s64, F) } \
    else if (_dA == nxd_s64 && _dB == nxd_D) { _loop(s64, D) } \
    else if (_dA == nxd_s64 && _dB == nxd_L) { _loop(s64, L) } \
    else if (_dA == nxd_s64 && _dB == nxd_Q) { _loop(s64, Q) } \
    else if (_dA == nxd_s64 && _dB == nxd_cH) { _loop(s64, cH) } \
    else if (_dA == nxd_s64 && _dB == nxd_cF) { _loop(s64, cF) } \
    else if (_dA == nxd_s64 && _dB == nxd_cD) { _loop(s64, cD) } \
    else if (_dA == nxd_s64 && _dB == nxd_cL) { _loop(s64, cL) } \
    else if (_dA == nxd_s64 && _dB == nxd_cQ) { _loop(s64, cQ) } \
    else if (_dA == nxd_H && _dB == nxd_bl) { _loop(H, bl) } \
    else if (_dA == nxd_H && _dB == nxd_s8) { _loop(H, s8) } \
    else if (_dA == nxd_H && _dB == nxd_u8) { _loop(H, u8) } \
    else if (_dA == nxd_H && _dB == nxd_u16) { _loop(H, u16) } \
    else if (_dA == nxd_H && _dB == nxd_s16) { _loop(H, s16) } \
    else if (_dA == nxd_H && _dB == nxd_u32) { _loop(H, u32) } \
    else if (_dA == nxd_H && _dB == nxd_s32) { _loop(H, s32) } \
    else if (_dA == nxd_H && _dB == nxd_u64) { _loop(H, u64) } \
    else if (_dA == nxd_H && _dB == nxd_s64) { _loop(H, s64) } \
    else if (_dA == nxd_H && _dB == nxd_H) { _loop(H, H) } \
    else if (_dA == nxd_H && _dB == nxd_F) { _loop(H, F) } \
    else if (_dA == nxd_H && _dB == nxd_D) { _loop(H, D) } \
    else if (_dA == nxd_H && _dB == nxd_L) { _loop(H, L) } \
    else if (_dA == nxd_H && _dB == nxd_Q) { _loop(H, Q) } \
    else if (_dA == nxd_H && _dB == nxd_cH) { _loop(H, cH) } \
    else if (_dA == nxd_H && _dB == nxd_cF) { _loop(H, cF) } \
    else if (_dA == nxd_H && _dB == nxd_cD) { _loop(H, cD) } \
    else if (_dA == nxd_H && _dB == nxd_cL) { _loop(H, cL) } \
    else if (_dA == nxd_H && _dB == nxd_cQ) { _loop(H, cQ) } \
    else if (_dA == nxd_F && _dB == nxd_bl) { _loop(F, bl) } \
    else if (_dA == nxd_F && _dB == nxd_s8) { _loop(F, s8) } \
    else if (_dA == nxd_F && _dB == nxd_u8) { _loop(F, u8) } \
    else if (_dA == nxd_F && _dB == nxd_u16) { _loop(F, u16) } \
    else if (_dA == nxd_F && _dB == nxd_s16) { _loop(F, s16) } \
    else if (_dA == nxd_F && _dB == nxd_u32) { _loop(F, u32) } \
    else if (_dA == nxd_F && _dB == nxd_s32) { _loop(F, s32) } \
    else if (_dA == nxd_F && _dB == nxd_u64) { _loop(F, u64) } \
    else if (_dA == nxd_F && _dB == nxd_s64) { _loop(F, s64) } \
    else if (_dA == nxd_F && _dB == nxd_H) { _loop(F, H) } \
    else if (_dA == nxd_F && _dB == nxd_F) { _loop(F, F) } \
    else if (_dA == nxd_F && _dB == nxd_D) { _loop(F, D) } \
    else if (_dA == nxd_F && _dB == nxd_L) { _loop(F, L) } \
    else if (_dA == nxd_F && _dB == nxd_Q) { _loop(F, Q) } \
    else if (_dA == nxd_F && _dB == nxd_cH) { _loop(F, cH) } \
    else if (_dA == nxd_F && _dB == nxd_cF) { _loop(F, cF) } \
    else if (_dA == nxd_F && _dB == nxd_cD) { _loop(F, cD) } \
    else if (_dA == nxd_F && _dB == nxd_cL) { _loop(F, cL) } \
    else if (_dA == nxd_F && _dB == nxd_cQ) { _loop(F, cQ) } \
    else if (_dA == nxd_D && _dB == nxd_bl) { _loop(D, bl) } \
    else if (_dA == nxd_D && _dB == nxd_s8) { _loop(D, s8) } \
    else if (_dA == nxd_D && _dB == nxd_u8) { _loop(D, u8) } \
    else if (_dA == nxd_D && _dB == nxd_u16) { _loop(D, u16) } \
    else if (_dA == nxd_D && _dB == nxd_s16) { _loop(D, s16) } \
    else if (_dA == nxd_D && _dB == nxd_u32) { _loop(D, u32) } \
    else if (_dA == nxd_D && _dB == nxd_s32) { _loop(D, s32) } \
    else if (_dA == nxd_D && _dB == nxd_u64) { _loop(D, u64) } \
    else if (_dA == nxd_D && _dB == nxd_s64) { _loop(D, s64) } \
    else if (_dA == nxd_D && _dB == nxd_H) { _loop(D, H) } \
    else if (_dA == nxd_D && _dB == nxd_F) { _loop(D, F) } \
    else if (_dA == nxd_D && _dB == nxd_D) { _loop(D, D) } \
    else if (_dA == nxd_D && _dB == nxd_L) { _loop(D, L) } \
    else if (_dA == nxd_D && _dB == nxd_Q) { _loop(D, Q) } \
    else if (_dA == nxd_D && _dB == nxd_cH) { _loop(D, cH) } \
    else if (_dA == nxd_D && _dB == nxd_cF) { _loop(D, cF) } \
    else if (_dA == nxd_D && _dB == nxd_cD) { _loop(D, cD) } \
    else if (_dA == nxd_D && _dB == nxd_cL) { _loop(D, cL) } \
    else if (_dA == nxd_D && _dB == nxd_cQ) { _loop(D, cQ) } \
    else if (_dA == nxd_L && _dB == nxd_bl) { _loop(L, bl) } \
    else if (_dA == nxd_L && _dB == nxd_s8) { _loop(L, s8) } \
    else if (_dA == nxd_L && _dB == nxd_u8) { _loop(L, u8) } \
    else if (_dA == nxd_L && _dB == nxd_u16) { _loop(L, u16) } \
    else if (_dA == nxd_L && _dB == nxd_s16) { _loop(L, s16) } \
    else if (_dA == nxd_L && _dB == nxd_u32) { _loop(L, u32) } \
    else if (_dA == nxd_L && _dB == nxd_s32) { _loop(L, s32) } \
    else if (_dA == nxd_L && _dB == nxd_u64) { _loop(L, u64) } \
    else if (_dA == nxd_L && _dB == nxd_s64) { _loop(L, s64) } \
    else if (_dA == nxd_L && _dB == nxd_H) { _loop(L, H) } \
    else if (_dA == nxd_L && _dB == nxd_F) { _loop(L, F) } \
    else if (_dA == nxd_L && _dB == nxd_D) { _loop(L, D) } \
    else if (_dA == nxd_L && _dB == nxd_L) { _loop(L, L) } \
    else if (_dA == nxd_L && _dB == nxd_Q) { _loop(L, Q) } \
    else if (_dA == nxd_L && _dB == nxd_cH) { _loop(L, cH) } \
    else if (_dA == nxd_L && _dB == nxd_cF) { _loop(L, cF) } \
    else if (_dA == nxd_L && _dB == nxd_cD) { _loop(L, cD) } \
    else if (_dA == nxd_L && _dB == nxd_cL) { _loop(L, cL) } \
    else if (_dA == nxd_L && _dB == nxd_cQ) { _loop(L, cQ) } \
    else if (_dA == nxd_Q && _dB == nxd_bl) { _loop(Q, bl) } \
    else if (_dA == nxd_Q && _dB == nxd_s8) { _loop(Q, s8) } \
    else if (_dA == nxd_Q && _dB == nxd_u8) { _loop(Q, u8) } \
    else if (_dA == nxd_Q && _dB == nxd_u16) { _loop(Q, u16) } \
    else if (_dA == nxd_Q && _dB == nxd_s16) { _loop(Q, s16) } \
    else if (_dA == nxd_Q && _dB == nxd_u32) { _loop(Q, u32) } \
    else if (_dA == nxd_Q && _dB == nxd_s32) { _loop(Q, s32) } \
    else if (_dA == nxd_Q && _dB == nxd_u64) { _loop(Q, u64) } \
    else if (_dA == nxd_Q && _dB == nxd_s64) { _loop(Q, s64) } \
    else if (_dA == nxd_Q && _dB == nxd_H) { _loop(Q, H) } \
    else if (_dA == nxd_Q && _dB == nxd_F) { _loop(Q, F) } \
    else if (_dA == nxd_Q && _dB == nxd_D) { _loop(Q, D) } \
    else if (_dA == nxd_Q && _dB == nxd_L) { _loop(Q, L) } \
    else if (_dA == nxd_Q && _dB == nxd_Q) { _loop(Q, Q) } \
    else if (_dA == nxd_Q && _dB == nxd_cH) { _loop(Q, cH) } \
    else if (_dA == nxd_Q && _dB == nxd_cF) { _loop(Q, cF) } \
    else if (_dA == nxd_Q && _dB == nxd_cD) { _loop(Q, cD) } \
    else if (_dA == nxd_Q && _dB == nxd_cL) { _loop(Q, cL) } \
    else if (_dA == nxd_Q && _dB == nxd_cQ) { _loop(Q, cQ) } \
    else if (_dA == nxd_cH && _dB == nxd_bl) { _loop(cH, bl) } \
    else if (_dA == nxd_cH && _dB == nxd_s8) { _loop(cH, s8) } \
    else if (_dA == nxd_cH && _dB == nxd_u8) { _loop(cH, u8) } \
    else if (_dA == nxd_cH && _dB == nxd_u16) { _loop(cH, u16) } \
    else if (_dA == nxd_cH && _dB == nxd_s16) { _loop(cH, s16) } \
    else if (_dA == nxd_cH && _dB == nxd_u32) { _loop(cH, u32) } \
    else if (_dA == nxd_cH && _dB == nxd_s32) { _loop(cH, s32) } \
    else if (_dA == nxd_cH && _dB == nxd_u64) { _loop(cH, u64) } \
    else if (_dA == nxd_cH && _dB == nxd_s64) { _loop(cH, s64) } \
    else if (_dA == nxd_cH && _dB == nxd_H) { _loop(cH, H) } \
    else if (_dA == nxd_cH && _dB == nxd_F) { _loop(cH, F) } \
    else if (_dA == nxd_cH && _dB == nxd_D) { _loop(cH, D) } \
    else if (_dA == nxd_cH && _dB == nxd_L) { _loop(cH, L) } \
    else if (_dA == nxd_cH && _dB == nxd_Q) { _loop(cH, Q) } \
    else if (_dA == nxd_cH && _dB == nxd_cH) { _loop(cH, cH) } \
    else if (_dA == nxd_cH && _dB == nxd_cF) { _loop(cH, cF) } \
    else if (_dA == nxd_cH && _dB == nxd_cD) { _loop(cH, cD) } \
    else if (_dA == nxd_cH && _dB == nxd_cL) { _loop(cH, cL) } \
    else if (_dA == nxd_cH && _dB == nxd_cQ) { _loop(cH, cQ) } \
    else if (_dA == nxd_cF && _dB == nxd_bl) { _loop(cF, bl) } \
    else if (_dA == nxd_cF && _dB == nxd_s8) { _loop(cF, s8) } \
    else if (_dA == nxd_cF && _dB == nxd_u8) { _loop(cF, u8) } \
    else if (_dA == nxd_cF && _dB == nxd_u16) { _loop(cF, u16) } \
    else if (_dA == nxd_cF && _dB == nxd_s16) { _loop(cF, s16) } \
    else if (_dA == nxd_cF && _dB == nxd_u32) { _loop(cF, u32) } \
    else if (_dA == nxd_cF && _dB == nxd_s32) { _loop(cF, s32) } \
    else if (_dA == nxd_cF && _dB == nxd_u64) { _loop(cF, u64) } \
    else if (_dA == nxd_cF && _dB == nxd_s64) { _loop(cF, s64) } \
    else if (_dA == nxd_cF && _dB == nxd_H) { _loop(cF, H) } \
    else if (_dA == nxd_cF && _dB == nxd_F) { _loop(cF, F) } \
    else if (_dA == nxd_cF && _dB == nxd_D) { _loop(cF, D) } \
    else if (_dA == nxd_cF && _dB == nxd_L) { _loop(cF, L) } \
    else if (_dA == nxd_cF && _dB == nxd_Q) { _loop(cF, Q) } \
    else if (_dA == nxd_cF && _dB == nxd_cH) { _loop(cF, cH) } \
    else if (_dA == nxd_cF && _dB == nxd_cF) { _loop(cF, cF) } \
    else if (_dA == nxd_cF && _dB == nxd_cD) { _loop(cF, cD) } \
    else if (_dA == nxd_cF && _dB == nxd_cL) { _loop(cF, cL) } \
    else if (_dA == nxd_cF && _dB == nxd_cQ) { _loop(cF, cQ) } \
    else if (_dA == nxd_cD && _dB == nxd_bl) { _loop(cD, bl) } \
    else if (_dA == nxd_cD && _dB == nxd_s8) { _loop(cD, s8) } \
    else if (_dA == nxd_cD && _dB == nxd_u8) { _loop(cD, u8) } \
    else if (_dA == nxd_cD && _dB == nxd_u16) { _loop(cD, u16) } \
    else if (_dA == nxd_cD && _dB == nxd_s16) { _loop(cD, s16) } \
    else if (_dA == nxd_cD && _dB == nxd_u32) { _loop(cD, u32) } \
    else if (_dA == nxd_cD && _dB == nxd_s32) { _loop(cD, s32) } \
    else if (_dA == nxd_cD && _dB == nxd_u64) { _loop(cD, u64) } \
    else if (_dA == nxd_cD && _dB == nxd_s64) { _loop(cD, s64) } \
    else if (_dA == nxd_cD && _dB == nxd_H) { _loop(cD, H) } \
    else if (_dA == nxd_cD && _dB == nxd_F) { _loop(cD, F) } \
    else if (_dA == nxd_cD && _dB == nxd_D) { _loop(cD, D) } \
    else if (_dA == nxd_cD && _dB == nxd_L) { _loop(cD, L) } \
    else if (_dA == nxd_cD && _dB == nxd_Q) { _loop(cD, Q) } \
    else if (_dA == nxd_cD && _dB == nxd_cH) { _loop(cD, cH) } \
    else if (_dA == nxd_cD && _dB == nxd_cF) { _loop(cD, cF) } \
    else if (_dA == nxd_cD && _dB == nxd_cD) { _loop(cD, cD) } \
    else if (_dA == nxd_cD && _dB == nxd_cL) { _loop(cD, cL) } \
    else if (_dA == nxd_cD && _dB == nxd_cQ) { _loop(cD, cQ) } \
    else if (_dA == nxd_cL && _dB == nxd_bl) { _loop(cL, bl) } \
    else if (_dA == nxd_cL && _dB == nxd_s8) { _loop(cL, s8) } \
    else if (_dA == nxd_cL && _dB == nxd_u8) { _loop(cL, u8) } \
    else if (_dA == nxd_cL && _dB == nxd_u16) { _loop(cL, u16) } \
    else if (_dA == nxd_cL && _dB == nxd_s16) { _loop(cL, s16) } \
    else if (_dA == nxd_cL && _dB == nxd_u32) { _loop(cL, u32) } \
    else if (_dA == nxd_cL && _dB == nxd_s32) { _loop(cL, s32) } \
    else if (_dA == nxd_cL && _dB == nxd_u64) { _loop(cL, u64) } \
    else if (_dA == nxd_cL && _dB == nxd_s64) { _loop(cL, s64) } \
    else if (_dA == nxd_cL && _dB == nxd_H) { _loop(cL, H) } \
    else if (_dA == nxd_cL && _dB == nxd_F) { _loop(cL, F) } \
    else if (_dA == nxd_cL && _dB == nxd_D) { _loop(cL, D) } \
    else if (_dA == nxd_cL && _dB == nxd_L) { _loop(cL, L) } \
    else if (_dA == nxd_cL && _dB == nxd_Q) { _loop(cL, Q) } \
    else if (_dA == nxd_cL && _dB == nxd_cH) { _loop(cL, cH) } \
    else if (_dA == nxd_cL && _dB == nxd_cF) { _loop(cL, cF) } \
    else if (_dA == nxd_cL && _dB == nxd_cD) { _loop(cL, cD) } \
    else if (_dA == nxd_cL && _dB == nxd_cL) { _loop(cL, cL) } \
    else if (_dA == nxd_cL && _dB == nxd_cQ) { _loop(cL, cQ) } \
    else if (_dA == nxd_cQ && _dB == nxd_bl) { _loop(cQ, bl) } \
    else if (_dA == nxd_cQ && _dB == nxd_s8) { _loop(cQ, s8) } \
    else if (_dA == nxd_cQ && _dB == nxd_u8) { _loop(cQ, u8) } \
    else if (_dA == nxd_cQ && _dB == nxd_u16) { _loop(cQ, u16) } \
    else if (_dA == nxd_cQ && _dB == nxd_s16) { _loop(cQ, s16) } \
    else if (_dA == nxd_cQ && _dB == nxd_u32) { _loop(cQ, u32) } \
    else if (_dA == nxd_cQ && _dB == nxd_s32) { _loop(cQ, s32) } \
    else if (_dA == nxd_cQ && _dB == nxd_u64) { _loop(cQ, u64) } \
    else if (_dA == nxd_cQ && _dB == nxd_s64) { _loop(cQ, s64) } \
    else if (_dA == nxd_cQ && _dB == nxd_H) { _loop(cQ, H) } \
    else if (_dA == nxd_cQ && _dB == nxd_F) { _loop(cQ, F) } \
    else if (_dA == nxd_cQ && _dB == nxd_D) { _loop(cQ, D) } \
    else if (_dA == nxd_cQ && _dB == nxd_L) { _loop(cQ, L) } \
    else if (_dA == nxd_cQ && _dB == nxd_Q) { _loop(cQ, Q) } \
    else if (_dA == nxd_cQ && _dB == nxd_cH) { _loop(cQ, cH) } \
    else if (_dA == nxd_cQ && _dB == nxd_cF) { _loop(cQ, cF) } \
    else if (_dA == nxd_cQ && _dB == nxd_cD) { _loop(cQ, cD) } \
    else if (_dA == nxd_cQ && _dB == nxd_cL) { _loop(cQ, cL) } \
    else if (_dA == nxd_cQ && _dB == nxd_cQ) { _loop(cQ, cQ) } \
} while (0);


#endif /* KSNXT_H__ */
