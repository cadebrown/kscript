/* ks/ffi.h - header for the 'ffi' (C library, FFI) module of kscript
 *
 * Provides wrappers for interfacing with C functions/libraries, through Foreign Function Interface
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
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSFFI_H__
#define KSFFI_H__

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

/* 'ffi.DLL' - dynamically loaded library
 * 
 */
typedef struct ksffi_dll_s {
    KSO_BASE

    /* Name of the source */
    ks_str name;

    /* Handle returned by 'dlopen' function */
    void* handle;

}* ksffi_dll;


/* Do for integer types */
#define KSFFI_DO_INTS(macro) \
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

#define _KSFFI_INTTYPE(_name, _ctp) \
typedef struct ksffi_##_name##_s { \
    KSO_BASE \
    _ctp val; \
}* ksffi_##_name;

KSFFI_DO_INTS(_KSFFI_INTTYPE)



/* Do for integer types */
#define KSFFI_DO_FLOATS(macro) \
    macro(float, float) \
    macro(double, double) \
    macro(longdouble, long double) \

#define _KSFFI_FLOATTYPE(_name, _ctp) \
typedef struct ksffi_##_name##_s { \
    KSO_BASE \
    _ctp val; \
}* ksffi_##_name;

KSFFI_DO_FLOATS(_KSFFI_FLOATTYPE)


/* 'ffi.ptr[T]' - pointer type
 *
 */
typedef struct ksffi_ptr_s {
    KSO_BASE

    /* Address of the value */
    void* val;


}* ksffi_ptr;

/* 'ffi.func[ResT, ArgTs]' - callable C-style function
 *
 */
typedef struct ksffi_func_s {
    KSO_BASE

    /* Function pointer (which can be casted) */
    void (*val)();

}* ksffi_func;


/* Open/load a library, and return it
 */
KS_API ksffi_dll ksffi_open(ks_type tp, ks_str name);

/* Create a new integer type
 */
KS_API kso ksffi_new_int(ks_type tp, ks_cint val);
KS_API kso ksffi_new_intu(ks_type tp, ks_uint val);


/* Compute the size (in bytes) of a C-type, or return -1 if there was an error
 */
KS_API int ksffi_sizeof(ks_type tp);

/* sizeof(*tp)
 */
KS_API int ksffi_sizeofp(ks_type tp);


/* Wrap a generic type which should be of the 'tp's size and type
 */
KS_API kso ksffi_wrap(ks_type tp, void* val);

/* Unrap an object into a value
 */
KS_API bool ksffi_unwrap(ks_type tp, kso obj, void* val);

/* Turns an object into a C-style value wrapper
 */
KS_API kso ksffi_wrapo(kso obj, ks_type tp);

/* Create a pointer type
 */
KS_API ks_type ksffi_ptr_type(ks_type of);

/* Create a value of a pointer type
 */
KS_API ksffi_ptr ksffi_ptr_make(ks_type of, void* val);

/* Like above, but 'tp' has already been made
 */
KS_API ksffi_ptr ksffi_ptr_maken(ks_type tp, void* val);


/* Create a new floating point type
 */
KS_API kso ksffi_new_float(ks_type tp, ks_cfloat val);

#ifdef KS_HAVE_long_double
KS_API kso ksffi_new_floatld(ks_type tp, long double val);
#endif

#ifdef KS_HAVE_float128
KS_API kso ksffi_new_float128(ks_type tp, __float128 val);
#endif


/* Create a function type
 */
KS_API ks_type ksffi_func_make(ks_type restype, ks_tuple argtypes);

/* Wrap a C-style function
 */
KS_API ksffi_func ksffi_func_new(ks_type tp, void (*val)());


/* Types */
KS_API extern ks_type
#define _KSFFI_INTDECL(_name, _ctp) ksffit_##_name,
KSFFI_DO_INTS(_KSFFI_INTDECL)

#define _KSFFI_FLOATDECL(_name, _ctp) ksffit_##_name,
KSFFI_DO_FLOATS(_KSFFI_FLOATDECL)

    ksffit_ptr,
    ksffit_func,
    ksffit_dll
;

#endif /* KSFFI_H__ */
