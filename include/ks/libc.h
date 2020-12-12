/* ks/libc.h - header for the 'libc' (C library, FFI) module of kscript
 *
 * Provides wrappers for interfacing with C functions/libraries
 * 
 * Builtin types:
 *   char
 *   unsigned char
 *   short
 *   unsigned short
 *   int
 *   unsigned int
 *   long
 *   unsigned long
 *   long long
 *   unsigned long long
 * 
 * 
 * 
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSLIBC_H__
#define KSLIBC_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/* FFI (--with-ffi) 
 *
 * Adds support for foreign-function-interface, for wrapping functions
 *
 */
#ifdef KS_HAVE_ffi
 #include <ffi.h>
#endif



/* Types */

/* 'libc.DLL' - dynamically loaded library
 * 
 */
typedef struct kslibc_dll_s {
    KSO_BASE

    /* Name of the source */
    ks_str name;

    /* Handle returned by 'dlopen' function */
    void* handle;

}* kslibc_dll;


/* Do for integer types */
#define KSLIBC_DO_INTS(macro) \
    macro(schar, signed char) \
    macro(uchar, unsigned char) \
    macro(sshort, signed short) \
    macro(ushort, unsigned short) \
    macro(sint, signed int) \
    macro(uint, unsigned int) \
    macro(slong, signed long) \
    macro(ulong, unsigned long) \
    macro(slonglong, signed long long) \
    macro(ulonglong, unsigned long long) \

#define _KSLIBC_INTTYPE(_name, _ctp) \
typedef struct kslibc_##_name##_s { \
    KSO_BASE \
    _ctp val; \
}* kslibc_##_name;

KSLIBC_DO_INTS(_KSLIBC_INTTYPE)

/* 'libc.Pointer' - address of other type
 *
 */
typedef struct kslibc_ptr_s {
    KSO_BASE

    /* Address of the value */
    void* val;

    /* What type is the pointer of (None==void) */
    ks_type of;


}* kslibc_ptr;

/* 'libc.FuncPointer'
 *
 */
typedef struct kslibc_fp_s {
    KSO_BASE

    /* Value stored at */
    void (*val)();

    /* Result and argument types */
    ks_type restype;
    ks_tuple argtypes;

}* kslibc_fp;


/* Open/load a library, and return it
 */
KS_API kslibc_dll kslibc_open(ks_type tp, ks_str name);


/* Create a new integer type
 */
KS_API kso kslibc_new_int(ks_type tp, ks_cint val);
KS_API kso kslibc_new_intu(ks_type tp, ks_uint val);

/* Wrap a C-style function
 */
KS_API kslibc_fp kslibc_fp_wrap(ks_type tp, void (*val)(), ks_type restype, ks_tuple argtypes);


/* Types */
KS_API extern ks_type
    kslibct_schar,
    kslibct_uchar,
    kslibct_sshort,
    kslibct_ushort,
    kslibct_sint,
    kslibct_uint,
    kslibct_slong,
    kslibct_ulong,
    kslibct_slonglong,
    kslibct_ulonglong,

    kslibct_dll,
    kslibct_fp
;

#endif /* KSLIBC_H__ */
