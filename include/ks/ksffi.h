/* ks/ksffi.h - header for the 'ffi' (Foreign Function Interface) module in kscript
 *
 * Provides wrappers for interfacing with C functions/libraries, through Foreign Function Interface
 * 
 * Builtin types:
 *   s8
 *   u8
 *   s16
 *   u16
 *   s32
 *   u32
 *   s64
 *   u64
 *   f
 *   d
 *   ld
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


/* ffi.DLL - Dynamically loaded library
 * 
 */
typedef struct ksffi_dll_s {
    KSO_BASE

    /* Where the library was loaded from */
    ks_str src;


    /* Opaque handle, which is returned by 'dlopen()' or a
     *   similar function
     */
    void* handle;

}* ksffi_dll;



/* Paste a macro for C-style integers */
#define KSFFI_DO_I(macro) \
    macro(s8, int8_t) \
    macro(u8, uint8_t) \
    macro(s16, int16_t) \
    macro(u16, uint16_t) \
    macro(s32, int32_t) \
    macro(u32, uint32_t) \
    macro(s64, int64_t) \
    macro(u64, uint64_t) \

/* Paste a macro for C-style floats */
#define KSFFI_DO_F(macro) \
    macro(float, float) \
    macro(double, double) \
    macro(longdouble, long double) \


/** Define integer types **/

#define _KSFFI_TYPE_I(_name, _type) \
typedef struct ksffi_##_name##_s { \
    KSO_BASE \
    _type val; \
}* ksffi_##_name;

KSFFI_DO_I(_KSFFI_TYPE_I)

#define _KSFFI_TYPE_F(_name, _type) \
typedef struct ksffi_##_name##_s { \
    KSO_BASE \
    _type val; \
}* ksffi_##_name;

KSFFI_DO_F(_KSFFI_TYPE_F)


/** Aliases **/


#define ksffit_char ksffit_s8
#define ksffit_uchar ksffit_u8
#define ksffit_short ksffit_s16
#define ksffit_ushort ksffit_u16
#define ksffit_int ksffit_s32
#define ksffit_uint ksffit_u32
#define ksffit_longlong ksffit_s64
#define ksffit_ulonglong ksffit_u64

/* TODO: check platforms for longs */
#define ksffit_long ksffit_s32
#define ksffit_ulong ksffit_u32

#define ksffit_size_t ksffit_u64
#define ksffit_ssize_t ksffit_s64


/* ffi.ptr[T] - pointer type to another type
 *
 */
typedef struct ksffi_ptr_s {
    KSO_BASE

    /* Address of the value */
    void* val;

}* ksffi_ptr;

/* ffi.func[ResT, ArgsT] - C-style function type
 *
 */
typedef struct ksffi_func_s {
    KSO_BASE

    /* Function pointer (which can be casted to others) */
    void (*val)();

}* ksffi_func;

/* ffi.struct[MembersT] - C-style structure type
 *
 */
typedef struct ksffi_struct_s {
    KSO_BASE

    /* Array allocate for the structure (the size will depend
     *   on what kind of structure)
     */
    char val[0];

}* ksffi_struct;


/** Functions **/

#ifdef KS_HAVE_ffi

/* Converts 'tp' to the compatible libffi type descriptor
 */
KS_API bool ksffi_libffi_type(ks_type tp, ffi_type** res);

#endif


/* Open a DLL and return it
 */
KS_API ksffi_dll ksffi_open(ks_str src);

/* Computes 'sizeof(tp)' (in bytes) of a C-style type
 * Returns -1 if 'tp' was not a C-style type
 * 'sizeofp' calculates the size of '*tp'
 */
KS_API int ksffi_sizeof(ks_type tp);
KS_API int ksffi_sizeofp(ks_type tp);

/* Gets the FFI type which 'obj' should be turned into
 */
KS_API ks_type ksffi_typeof(kso obj);

/* Wrap a C-style type and value into an object
 */
KS_API kso ksffi_wrap(ks_type tp, void* val);

/* Unwrap an object into a C-style type, storing in 'val', which
 *   should be allocated to 'ksffi_sizeof(tp)'
 */
KS_API bool ksffi_unwrap(ks_type tp, kso obj, void* val);

/* Make a pointer type to type 'of'
 */
KS_API ks_type ksffi_ptr_make(ks_type of);

/* Make a structure type
 * Each of 'members' should be a tuple of '(name, type[, offset])'
 */
KS_API ks_type ksffi_struct_make(int nmembers, kso* members);

/* Make a function type returning 'restype', and taking a number of arguments
 */
KS_API ks_type ksffi_func_make(ks_type restype, int nargtypes, ks_type* argtypes, bool isvararg);


/* Types */
KS_API_DATA ks_type
#define _KSFFI_DECL_I(_name, _type) ksffit_##_name,
KSFFI_DO_I(_KSFFI_DECL_I)

#define _KSFFI_DECL_F(_name, _type) ksffit_##_name,
KSFFI_DO_F(_KSFFI_DECL_F)

    ksffit_ptr_void,
    ksffit_ptr,
    ksffit_func,
    ksffit_struct,
    ksffit_dll
;


#endif /* KSFFI_H__ */
