/* ks/ks.h - kscript C API definitions
 *
 * kscript is a dynamic programming language, and this is the header for the default implementation (written in C).
 * 
 * Here are the rule(s) of thumb regarding exported functions/symbols/globals:
 *
 *   - kst_*: A standard/builtin type
 *   - ksf_*: A standard/builtin func
 *   - ksm_*: A standard/builtin module
 *   - ksg_*: A builtin global value
 *   - ks_*: full public support, guaranteed to be backwards compatible in the major version
 *   - _ks*: internal function, not guaranteed to be backwards compatible (DO NOT USE THIS!)
 * 
 * To access builtin modules, include that specific header. For example, for the 'os' module, make sure '#include <ks/os.h>'
 *   is being included in your program.
 * 
 * As far as return values, if a function returns a 'kso' or other object type, it will return a new reference (unless otherwise
 *   stated in the documentation), or a NULL pointer if there was an exception thrown. Otherwise, if it returns a 'bool' (boolean),
 *   that typically means it returns whether the operation was successful, or 'false' if an error was thrown. 
 * 
 * In the case that a function you have called has thrown an error (i.e. returned 'NULL' or 'false'), you have a few ways to proceed:
 *   - You may propogate that error by first freeing any resources you have allocated in your scope, and then return 'NULL' or 'false'
 *       yourself (and so on up the chain, assuming everyone is playing nice) 
 *   - You may 'catch' the exception and then perform some action (i.e. inspect it, print it, or ignore it). See the 'ks_catch_*' functions
 *       for more information. Once you call 'ks_catch' or some similar function, there is no longer an exception and you can return whatever
 *       valid object you like.
 * 
 * Whenever using machine-specific types, it is recommended to use types like 'ks_cint', 'ks_uint', 'ks_cfloat', and 'ks_ccomplex'. These
 *   are typedef'd to the type specific for this build, and standard types can be created from them.
 * 
 * 
 * 
 * SEE: https://kscript.org
 * SEE: https://github.com/ChemicalDevelopment/kscript
 * 
 * @author:    Cade Brown <cade@kscript.org>
 * @license:   GPLv3
 */

#pragma once
#ifndef KS_H__
#define KS_H__


/* Version Information  
 * 
 * This is the header API version. The actual distributed version of kscript may be different (although,
 *   if you get a warning about this, its a good sign your package manager has messed up).
 * 
 * Developers: If you change this, change it in './configure' also
 * 
 */
#define KS_VERSION_MAJOR 0
#define KS_VERSION_MINOR 2
#define KS_VERSION_PATCH 2


/** Platform detection **/

/* On Emscripten, allow 'dead' code to remain (because it may be called dynamically) */
#if defined(__EMSCRIPTEN__)
  #define KS_EMSCRIPTEN_API EMSCRIPTEN_KEEPALIVE
#else
  #define KS_EMSCRIPTEN_API
#endif

#if defined(_WIN32) && !defined(WIN32)
  #define WIN32
#endif

/* On Windows, import/export must be defined explicitly */
#if defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
  #define KS_API_IMPORT __declspec(dllimport)
  #define KS_API_EXPORT __declspec(dllexport)
#else
  #define KS_API_IMPORT 
  #define KS_API_EXPORT 
#endif

/* Select whether we are importing or exporting */
#ifdef KS_BUILD
  #ifdef __cplusplus
    #define KS_API KS_API_EXPORT KS_EMSCRIPTEN_API extern "C"
  #else
    #define KS_API KS_API_EXPORT KS_EMSCRIPTEN_API
  #endif
#else
  #ifdef __cplusplus
    #define KS_API KS_API_IMPORT KS_EMSCRIPTEN_API extern "C"
  #else
    #define KS_API KS_API_IMPORT KS_EMSCRIPTEN_API
  #endif
#endif


/* KS_API_DATA is for data symbols, not functions */
#ifdef KS_BUILD
  #define KS_API_DATA KS_API_EXPORT extern 
#else
  #define KS_API_DATA KS_API_IMPORT extern
#endif


/* Use GNU and specific POSIX source, whenever possible */
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L


#ifndef KS_NO_CONFIG
 #include <ks/config.h>
#else
 #ifdef WIN32
  #define KS_PLATFORM "windows"
  #define KS_PLATFORM_PATHSEP "\\"
  #define KS_PLATFORM_EXTSHARED ".dll"
  #define KS_PLATFORM_EXTSTATIC ".lib"
  #define KS_PLATFORM_EXTBINARY ".exe"
 #else
  #define KS_PLATFORM "unix"
  #define KS_PLATFORM_PATHSEP "/"
  #define KS_PLATFORM_EXTSHARED ".so"
  #define KS_PLATFORM_EXTSTATIC ".a"
  #define KS_PLATFORM_EXTBINARY ""
 #endif
#endif


/** Headers **/

/* Windows headers */
#ifdef WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
#endif


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdint.h>
#include <stdbool.h>

#include <errno.h>
#include <assert.h>

#include <string.h>
#include <float.h>

#include <limits.h>
#include <math.h>


/* Platform-specific headers (detected) */
#ifdef KS_HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifdef KS_HAVE_DLFCN_H
  #include <dlfcn.h>
#endif


#if 0

#define bool int
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
#define true 1
#define false 0

#endif

#if 0
 #define assert(_x) do { if (!(_x)) { \
    fprintf(stderr, "assertion failed: '%s' (%s:%i)", #_x, __FILE__, __LINE__); \
    exit(1); \
 } } while (0)
#endif


/* Print a formatted message and crash the interpreter */
#define KS_CRASH(...) do { \
  fprintf(stderr, "(KS_CRASH): " __VA_ARGS__); \
  exit(1); \
} while (0)


/* WebAssembly/Emscription ()
 *
 * Adds support for compiling for the 'web' platform
 *
 */
#ifdef __EMSCRIPTEN__
 #include <emscripten.h>
#endif

/* pthreads (--with-pthreads) 
 *
 * Adds support for true threads, mutexes, and so forth
 *
 */
#ifdef KS_HAVE_pthreads
 #include <pthread.h>
#endif

/* GMP (--with-gmp) 
 *
 * Adds support for (faster) large integer components
 *
 */
#ifdef KS_HAVE_gmp
 #define KS_INT_GMP
 #define KS_INT_FULLGMP
 #include <gmp.h>
#else
 #define KS_INT_GMP
 #define KS_INT_MINIGMP
 #include <ks/iminigmp.h>
#endif


/** Constants **/

#include <ks/const.h>
#include <ks/colors.h>


/** Type Definitions **/

#include <ks/types.h>


/** Always Included Modules **/

#include <ks/io.h>
#include <ks/os.h>
#include <ks/util.h>


/** Globals **/

#define KSO_NONE ((kso)ksg_none)
#define KSO_UNDEFINED ((kso)ksg_undefined)
#define KSO_DOTDOTDOT ((kso)ksg_dotdotdot)

#define KSO_TRUE ((kso)ksg_true)
#define KSO_FALSE ((kso)ksg_false)
#define KSO_INF ((kso)ksg_inf)
#define KSO_NEGINF ((kso)ksg_neginf)
#define KSO_NAN ((kso)ksg_nan)

#define KSO_BOOL(_cond) ((_cond) ? KSO_TRUE : KSO_FALSE)

KS_API_DATA kso
    ksg_none,
    ksg_undefined,
    ksg_dotdotdot
;
KS_API_DATA ks_bool
    ksg_true,
    ksg_false
;

KS_API_DATA ks_float
    ksg_inf,
    ksg_neginf,
    ksg_nan
;

KS_API_DATA ksos_mutex
    ksg_GIL
;

KS_API_DATA ksos_thread
    ksg_main_thread
;

KS_API_DATA ks_dict
    ksg_globals,
    ksg_config,
    ksg_inter_vars
;

KS_API_DATA ks_list
    ksg_path
;

KS_API_DATA ks_type
    kst_object,
    kst_none,
    kst_undefined,
    kst_dotdotdot,
    kst_number,
      kst_int,
        kst_enum,
          kst_bool,
      kst_float,
      kst_rational,
      kst_complex,
    kst_str,
    kst_bytes,
    kst_regex,
    kst_range,
    kst_slice,
    kst_list,
    kst_tuple,
    kst_attrtuple,
    kst_set,
    kst_dict,
    kst_names,
    kst_map,
    kst_filter,
    kst_enumerate,

    kst_str_iter,
    kst_bytes_iter,
    kst_range_iter,
    kst_list_iter,
    kst_tuple_iter,
    kst_set_iter,
    kst_dict_iter,

    kst_type,
    kst_func,
    kst_partial,
    kst_module,

    kst_logger,

    kst_ast,
    kst_code,

    kst_Exception,
      kst_OutOfIterException,
      kst_Error,
        kst_InternalError,

        kst_SyntaxError,
        kst_ImportError,

        kst_TypeError,
          kst_TemplateError,
        kst_NameError,
        kst_AttrError,
        kst_KeyError,
          kst_IndexError,
        kst_ValError,
          kst_AssertError,
          kst_MathError,
            kst_OverflowError,
        kst_ArgError,
        kst_SizeError,
        kst_IOError,
        kst_OSError,
      kst_Warning,
        kst_PlatformWarning,
        kst_SyntaxWarning
;

KS_API_DATA ks_func
    ksf_any,
    ksf_all,
    ksf_min,
    ksf_max,
    ksf_sum,
    ksf_pow,

    ksf_open,

    ksf_eval,
    ksf_exec,
    ksf_exit,

    ksf_print,
    ksf_printf,

    ksf_hash,
    ksf_abs,
    ksf_len,
    ksf_repr,
    ksf_id,
    
    ksf_bin,
    ksf_oct,
    ksf_hex,

    ksf_ord,
    ksf_chr,

    ksf_issub,
    ksf_isinst,
    ksf_input,
    
    ksf_iter,
    ksf_next
;

KS_API_DATA int ksg_logger_level_default;



/** C-style helpers **/

struct ks_ikv {

    /* NUL-terminated C-style string */
    const char* key;

    /* Value */
    kso val;

};

struct ks_eikv {

    /* NUL-terminated C-style string */
    const char* key;

    /* Value */
    ks_cint val;

};

/* Creates a list of NULL-terminated initializers for a (key, val) entry */
#define KS_IKV(...) ((struct ks_ikv[]){ __VA_ARGS__ { NULL, NULL} })

/* Creates a list of NULL-terminated initializers for an enumeration creation */
#define KS_EIKV(...) ((struct ks_eikv[]){ __VA_ARGS__ { NULL, 0} })


/* Creates a new object (via standard allocation methods), and initializes the basic fields (type, refs, and attribute dict if available)
 */
#define KSO_NEW(_ctp, _tp) ((_ctp)_kso_new(_tp))

/* Deletes an object allocated with 'KSO_NEW'
 */
#define KSO_DEL(_ob) (_kso_del((kso)(_ob)))

/* Record a new reference to a given object */
#define KS_INCREF(_obj) do { ++(_obj)->refs; } while(0)

/* NULL-safe increment */
#define KS_NINCREF(_obj) do { if ((_obj)) { ++(_obj)->refs; } } while(0)

/* Delete a reference to a given object, and then free the object if the object has become unreachable */
#define KS_DECREF(_obj) do {                                           \
    kso _kso_obj = (kso)(_obj);                                        \
    if (--_kso_obj->refs <= 0) {                                       \
        _kso_free(_kso_obj, __FILE__, __func__, __LINE__);             \
    }                                                                  \
} while (0)


/* Decref, NULL-safe version */
#define KS_NDECREF(_obj) do { \
    kso _to = (kso)(_obj); \
    if (_to != NULL) KS_DECREF(_to); \
} while (0)

/* Yield a value representing a new reference of an object, downcasted to 'kso' */
#define KS_NEWREF(_obj) (_ks_newref((kso)(_obj)))


/* Throws an exception type, generated with a C-style format string (see 'ks_fmt()') 
 * After doing this, you should return NULL or otherwise indicate something was thrown
 */
#define KS_THROW(_tp, ...) do { \
    kso_throw_c(_tp, __FILE__, __func__, __LINE__, __VA_ARGS__); \
} while (0)

/* Throws a key error for an object, and a key that was missing */
#define KS_THROW_KEY(_obj, _key) KS_THROW(kst_KeyError, "%R", _key)

#define KS_THROW_VAL(_obj, _key) KS_THROW(kst_ValError, "%R", _key)

/* Throws a attr error for an object, and a key that was missing */
#define KS_THROW_ATTR(_obj, _attr) KS_THROW(kst_AttrError, "'%T' object had no attribute %R", _obj, _attr)

/* Generic index error */
#define KS_THROW_INDEX(_obj, _idx) KS_THROW(kst_IndexError, "Index out of range")

/* Throws a type conversion error */
#define KS_THROW_CONV(_from_type, _to_type) KS_THROW(kst_TypeError, "Could not convert '%s' object to '%s'", (_from_type)->i__name->data, (_to_type)->i__name->data)

/* Missing method (typically a '__' method) */
#define KS_THROW_METH(_obj, _meth) KS_THROW(kst_TypeError, "'%T' object had no '%s' method", _obj, _meth)

/* Throws a return status given by 'errno'
 */
#define KS_THROW_ERRNO(_errno_val, ...) do { \
    /* Get a template of 'OSError', for a specific errno */ \
    int _errno_valc = _errno_val; \
    ks_int _errno_valo = ks_int_new(_errno_valc); \
    ks_type _tet = ks_type_template(kst_OSError, 1, (kso[]){ (kso)_errno_valo }); \
    assert(_tet != NULL); \
    KS_DECREF(_errno_valo); \
    /* Format the requirements */ \
    ks_str _msgval = ks_fmt(__VA_ARGS__); \
    ks_str _genmsgval = ksos_strerr(_errno_valc); \
    KS_THROW(_tet, "%S (%S)", _msgval, _genmsgval); \
    KS_DECREF(_msgval); \
    KS_DECREF(_genmsgval); \
    KS_DECREF(_tet); \
} while (0)


/* Throw an 'OutOfIter' Exception
 */
#define KS_OUTOFITER() do { \
    KS_THROW(kst_OutOfIterException, ""); \
} while (0)


/* Define a C function which can be wrapped into a kscript function. Appends an '_' after the name
 */
#define KS_FUNC(_name) kso _name##_(int _nargs, kso* _args)

/* Define a C function which can be wrapped into a kscript function. Allows a type and function name
 */
#define KS_TFUNC(_type, _name) kso _type##_##_name##_(int _nargs, kso* _args)

/* Parse function args, and returns 'NULL' from the current function if they did not parse correctly */
#define KS_ARGS(...) do { \
    if (!_ks_args(_nargs, _args, __VA_ARGS__)) return NULL; \
} while(0)

/* Lock the GIL (blocking until the lock is acquired) */
#define KS_GIL_LOCK() do { \
    ksos_mutex_lock(ksg_GIL); \
} while (0)

/* Unlock the GIL (assumes the GIL is held by the current thread) */
#define KS_GIL_UNLOCK() do { \
    ksos_mutex_unlock(ksg_GIL); \
} while (0)


/** Functions **/

/* Initialize kscript, returns success
 */
KS_API bool ks_init();

/* Return true if kscript has been fully initialized
 */
KS_API bool ks_has_init();

/* Get a static version string describing the kscript implementation
 */
KS_API const char* ks_get_verstr();

/* Print a formatted string to 'stdout'
 * See 'ks_fmt()' for format strings
 */
KS_API bool ks_printf(const char* fmt, ...);



/*** Memory ***/

/* Allocate a block of 'sz' bytes
 *
 * This should only be reallocated or freed by other kscript memory functions
 * 
 * DONT USE MALLOC AND FREE WITH THIS
 */
KS_API void* ks_malloc(ks_size_t sz);

/* Equivalent to 'ks_malloc' but crashes if it fails
 */
KS_API void* ks_smalloc(ks_size_t sz);

/* Equivalent to 'ks_malloc(sz * num)' */
KS_API void* ks_zmalloc(ks_size_t sz, ks_size_t num);

/* Reallocate 'ptr' to have at least 'sz' bytes, which may internally
 *   return the pointer if it already has enough space, or it may duplicate and extend the memory
 * 
 * 'ks_realloc(NULL, sz)' is always equivalent to 'ks_malloc(sz)'
 * 
 * ONLY USE THIS FUNCTION with pointers generated from 'ks_*' 
 */
KS_API void* ks_realloc(void* ptr, ks_size_t sz);

/* Equivalent to 'ks_realloc' but crashes if it fails
 */
KS_API void* ks_srealloc(void* ptr, ks_size_t sz);

/* Equivalent to 'ks_realloc(ptr, sz * num)' */
KS_API void* ks_zrealloc(void* ptr, ks_size_t sz, ks_size_t num);


/* Frees a pointer to memory allocated via 'ks_*' allocation functions
 *
 * Guaranteed to be a no-op when 'ptr==NULL'
 */
KS_API void ks_free(void* ptr);

/* Calculate the next size in the default kscript reallocation scheme
 *
 * The main purpose is for mutable collections and arrays to resize and keep a max length
 *   (typically as '_max_len' or some private variable) which may be longer than the actual contents.
 *   This function is typically tuned to give good performing sequences, i.e. which typically allow
 *   appending to an array to be amortized to O(1). You can use it with anything:
 * 
 * ```c
 * int len = 0, _max_len = 0;
 * char* data = NULL;
 * while (true) {
 *   char* line = ...;
 *   int line_len = ...;
 *   
 *   len += line_len;
 *   if (len >= _max_len) {
 *     _max_len = ks_nextsize(_max_len, len);
 *     data = ks_realloc(data, _max_len);
 *   }
 *   ...
 * }
 * 
 * ```
 */
KS_API ks_ssize_t ks_nextsize(ks_ssize_t cur_sz, ks_ssize_t req);


/** Util **/

/* Returns the next prime > x
 * 
 */
KS_API ks_ssize_t ks_nextprime(ks_ssize_t x);

/* hash a sequence of bytes
 */
KS_API ks_hash_t ks_hash_bytes(ks_ssize_t len_b, const unsigned char* data);



/* Converts a string (in 'str', size of 'sz', or if '-1', then it is NUL-terminated) to a 'ks_cfloat' 
 *   (in full precision), and stores in 'out'
 *
 * 'str' may have a prefix:
 *   0b: base 2
 *         0[bB][01]*\.?[01]*([pP][\+\-]?[0-9]+)
 *   0o: base 8
 *         0[bB][0-7]*\.?[0-7]*([pP][\+\-]?[0-9]+)
 *   0x: base 16
 *         0[xX][0-9a-fA-F]*\.?[0-9a-fA-F]*([pP][\+\-]?[0-9]+)
 *   0d: base 10 (default)
 *         (0[dD])?[0-9]*\.?[0-9]*([eE][\+\-]?[0-9]+)
 * 
 * NOTE: If 'false' is returned, then an error was thrown 
 */
KS_API bool ks_cfloat_from_str(const char* str, int sz, ks_cfloat* out);

/* Converts a 'ks_cfloat' to a string (in 'str', writing a maximum number of 'sz' bytes)
 *
 * If 'sci', then the output is in scientific format, otherwise it is always in normal format with no
 *   exponent.
 * 
 * This function returns the total number of bytes required. If 'result > sz', then the output has been truncated,
 *   and may not be correct. Therefore, you should allocate a buffer of at least 'sz', and then call this function
 *   again with the same inputs, but with the new buffer
 */
KS_API int ks_cfloat_to_str(char* str, int sz, ks_cfloat val, bool sci, int prec, int base);

/* strfromd-like wrapper
 */
KS_API int ks_strfromd(char* str, size_t n, char* fmt, ks_cfloat val);

/* Return whether 'x' is a regular floating point number (not inf, -inf, or nan)
 */
KS_API bool ks_cfloat_isreg(ks_cfloat x);

/* Converts a string (in 'str', of 'sz', or if '-1', it is NUL-terminated) to a 'ks_ccomplex', stored in '*out'
 */
KS_API bool ks_ccomplex_from_str(const char* str, int sz, ks_ccomplex* out);

/* Converts a 'ks_ccomplex' to a string
 */
KS_API int ks_ccomplex_to_str(char* str, int sz, ks_ccomplex val, bool sci_re, bool sci_im, int prec_re, int prec_im);

/** Object API **/

/* Tell whether something is a subtype (or equal to the type)
 */
KS_API bool kso_issub(ks_type a, ks_type b);
KS_API bool kso_isinst(kso a, ks_type b);


/*** Execution ***/

/* Throws an exception up the call stack.
 * This function always returns 'NULL' for convenience
 */
KS_API void* kso_throw(ks_Exception exc);
KS_API void* kso_throw_c(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, ...);


/* Catches and returns the exception on the current thread
 * If it returns NULL, no exception is thrown
 */
KS_API ks_Exception kso_catch();

/* Catch and ignore any error
 */
KS_API bool kso_catch_ignore();

/* Catch and ignore any error, printing out if there was one
 */
KS_API bool kso_catch_ignore_print();

/* If there was something thrown, print it out and abort the process
 */
KS_API void kso_exit_if_err();


/*** Importing Modules ***/

/* Import a base module (i.e. should not have any '.' in the name)
 */
KS_API ks_module ks_import(ks_str name);

/* Import submodule of another module
 */
KS_API ks_module ks_import_sub(ks_module of, ks_str sub);


/* Run the interactive shell
 */
KS_API bool ks_inter();

/*** Conversions ***/

/* Attempt to convert 'ob' to the specified C-style value
 */
KS_API bool kso_get_ci(kso ob, ks_cint* val);
KS_API bool kso_get_ui(kso ob, ks_uint* val);
KS_API bool kso_get_cf(kso ob, ks_cfloat* val);
KS_API bool kso_get_cc(kso ob, ks_ccomplex* val);

/* Convert 'obj' to a boolean
 */
KS_API bool kso_truthy(kso obj, bool* out);

/* Computes comparison sign (-1, 0, or +1) and stores in '*out'
 */
KS_API bool kso_cmp(kso L, kso R, int* out);

/* Calculate whether 'L == R'
 */
KS_API bool kso_eq(kso L, kso R, bool* out);

/* Get the hash of an object
 */
KS_API bool kso_hash(kso ob, ks_hash_t* val);

/* Get the length of an object
 */
KS_API bool kso_len(kso ob, ks_ssize_t* val);

/* Conversions to common types (and repr())
 */
KS_API ks_str kso_str(kso ob);
KS_API ks_bytes kso_bytes(kso ob);
KS_API ks_str kso_repr(kso ob);
KS_API ks_number kso_number(kso ob);
KS_API ks_int kso_int(kso ob);


/* Return whether an object is numeric */
KS_API bool kso_is_num(kso obj);

/* Return whether an object is integral */
KS_API bool kso_is_int(kso obj);

/* Return whether an object is floating-point (not including integral)*/
KS_API bool kso_is_float(kso obj);

/* Return whether an object is complex (not including reals) */
KS_API bool kso_is_complex(kso obj);

/* Return whether an object is considered iterable */
KS_API bool kso_is_iterable(kso obj);

/* Return whether an object is considered callable */
KS_API bool kso_is_callable(kso obj);


/* Apply C-style printf-like formatting, and return as a string
 *
 * Format specifiers:
 *   %i: C-style 'int' 
 *   %c: 'char', or 'ks_ucp'
 *   %l: 'ks_cint' (+aliases)
 *   %u: 'ks_uint' (+aliases)
 *   %f: 'ks_cfloat'
 *   %p: 'void*' (+pointer types)
 *   %s: 'char*' (NUL-terminated string, or use '%.*s' for string of a given length (in bytes))
 *   %O: 'kso', formats object in raw mode, which never calls anything else
 *   %T: 'kso', formats the type name
 *   %S: 'kso', formats 'str(ob)'
 *   %R: 'kso', formats 'repr(ob)'
 *   %J: char*, int, 'kso', formats 's.join(objs)'
 * 
 */
KS_API ks_str ks_fmt(const char* fmt, ...);
KS_API ks_str ks_fmtv(const char* fmt, va_list ap);

/* Formatting with purely kscript objects --
 *
 * TODO: document format specifiers
 */
KS_API bool ks_fmt2(ksio_BaseIO bio, const char* fmt, int nargs, kso* args);


/*** Type Functions ***/

/* Create a new type (extending 'base') with a given name, C-size, and position of an '__attr' dictionary
 *
 * Use 'sz=0' or 'attr_pos=0' to just use base's without modification
 */
KS_API ks_type ks_type_new(const char* name, ks_type base, int sz, int attr_pos, const char* doc, struct ks_ikv* ikv);

/* Template a type, with the given arguments
 */
KS_API ks_type ks_type_template(ks_type base, int nargs, kso* args);

/* Return a type attribute
 */
KS_API kso ks_type_get(ks_type self, ks_str attr);

/* Sets an attribute of the type
 */
KS_API bool ks_type_set(ks_type self, ks_str attr, kso val);
KS_API bool ks_type_set_c(ks_type self, const char* attr, kso val);


/* Construct a new 'number' object from a C-style string
 *
 * This will construct an 'int', 'float', or 'complex' (with precedence in that order)
 */
KS_API ks_number ks_number_news(const char* src, ks_ssize_t len_b, int base);


/* Create a new integer
 */
KS_API ks_int ks_int_new(ks_cint val);
KS_API ks_int ks_int_newu(ks_uint val);

/* Create a new integer from flooring a C-style float
 */
KS_API ks_int ks_int_newf(ks_cfloat val);

/* Create a new integer from a string representation
 * Give 'base==0' for default/detected
 */
KS_API ks_int ks_int_news(ks_ssize_t sz, const char* src, int base);

/* Create from 'mpz_t', with 'zn' absorbing the reference
 */
KS_API ks_int ks_int_newz(mpz_t val);
KS_API ks_int ks_int_newzn(mpz_t val);
KS_API ks_int ks_int_newznt(ks_type tp, mpz_t val);

/* Compare two kscript integers, returning a comparator
 */
KS_API int ks_int_cmp(ks_int L, ks_int R);

/* Compare a kscript int to a C-style integer, return a sign indicating the comparison
 */
KS_API int ks_int_cmp_c(ks_int L, ks_cint r);


/* Create a new float
 */
KS_API ks_float ks_float_new(ks_cfloat val);

/* Create a new complex
 */
KS_API ks_complex ks_complex_new(ks_ccomplex val);
KS_API ks_complex ks_complex_newre(ks_cfloat re, ks_cfloat im);


/* Create an enumeration type
 */
KS_API ks_type ks_enum_make(const char* name, struct ks_eikv* eikv);

/* Retrieve an element of an enumeration
 * (by value, and name)
 */
KS_API ks_enum ks_enum_get(ks_type tp, ks_cint val);

/* Create a new string from UTF-8 data (also works for ASCII)
 * If 'len_b < 0', it is assumed to be NUL-terminated, otherwise that is the length in bytes
 */
KS_API ks_str ks_str_new(ks_ssize_t len_b, const char* data);

/* Create a new string from 'data', which should be the *root* of 'ks_malloc()'/friends,
 *   and the newly created string object will own that data (i.e. you should NOT free it)
 * 'data' should have space for a NUL-terminator after it
 */
KS_API ks_str ks_str_newn(ks_ssize_t len_b, char* data);

/* Calculate the length, in characters, of a UTF-8 string
 */
KS_API ks_ssize_t ks_str_lenc(ks_ssize_t len_b, const char* data);

/* Compare 'L' and 'R', returning either a comparator, or a boolean telling equality
 */
KS_API int ks_str_cmp(ks_str L, ks_str R);
KS_API bool ks_str_eq(ks_str L, ks_str R);
KS_API bool ks_str_eq_c(ks_str L, const char* data, ks_ssize_t len_b);


/* Attempts to find 'substr' in 'self' (within 'min_c' and 'max_c', inclusive and exclusive, respectively)
 *
 * 
 * Returns -1 if not found
 */
KS_API ks_ssize_t ks_str_find(ks_str self, ks_str substr, ks_ssize_t min_c, ks_ssize_t max_c, ks_ssize_t* idx_b);


/* Convert between strings of length 1 and ordinal codepoints
 */
KS_API ks_str ks_str_chr(ks_ucp ord);
KS_API ks_ucp ks_str_ord(ks_str chr);

/* Split a string
 */
KS_API ks_list ks_str_split(ks_str self, ks_str by);
KS_API ks_list ks_str_split_c(const char* self, const char* by);
KS_API ks_list ks_str_split_any(ks_str self, int nby, ks_str* by);

/* Convert a string to all upper-case, using 'ucd'
 */
KS_API ks_str ks_str_upper(ks_str self);

/* Convert a string to all lower-case, using 'ucd'
 */
KS_API ks_str ks_str_lower(ks_str self);

/* Calculate whether all characters are space
 */
KS_API bool ks_str_isspace(ks_str self);

/* Calculate whether all characters are printable
 */
KS_API bool ks_str_isprint(ks_str self);

/* Calculate whether all characters are numeric
 */
KS_API bool ks_str_isnum(ks_str self);

/* Calculate whether all characters are letters
 */
KS_API bool ks_str_isalpha(ks_str self);

/* Calculate whether all characters are letters or numeric
 */
KS_API bool ks_str_isalnum(ks_str self);

/* Calculate whether 'self' is a valid identifier
 */
KS_API bool ks_str_isident(ks_str self);



/* Create a new 'bytes'
 */
KS_API ks_bytes ks_bytes_new(ks_ssize_t len_b, const char* data);

/* Creates a new 'bytes' object, which should absorb the data pointer passed to it
 *   (which should be allocated via 'ks_malloc()'). Do not free the data after calling this
 */
KS_API ks_bytes ks_bytes_newn(ks_ssize_t len_b, char* data);

/* Creates a new bytes from an object
 */
KS_API ks_bytes ks_bytes_newo(ks_type tp, kso obj);


/* Create a new regular-expression from a descriptor string
 */
KS_API ks_regex ks_regex_new(ks_str expr);

/* Create a new regular-expression to match a string literal
 */
KS_API ks_regex ks_regex_newlit(ks_str expr);


/* Initialize a 'sim0'
 */
KS_API void ks_regex_sim0_init(ks_regex_sim0* sim, int n_states, struct ks_regex_nfa* states);

/* Free resources created by a 'sim0'
 */
KS_API void ks_regex_sim0_free(ks_regex_sim0* sim);

/* Add a state to the simulator
 */
KS_API void ks_regex_sim0_addcur(ks_regex_sim0* sim, int s);
KS_API void ks_regex_sim0_addnext(ks_regex_sim0* sim, int s);

/* Apply a single character to the simulator
 * Returns the number of states it is in now
 */
KS_API int ks_regex_sim0_step(ks_regex_sim0* sim, ks_ucp c);

/* Special steppers for epsilon transitions */
KS_API int ks_regex_sim0_step_linestart(ks_regex_sim0* sim);
KS_API int ks_regex_sim0_step_lineend(ks_regex_sim0* sim);


/* Returns whether the regex matches exactly
 */
KS_API bool ks_regex_exact(ks_regex self, ks_str str);

/* Returns whether the regex matched anywhere
 */
KS_API bool ks_regex_matches(ks_regex self, ks_str str);




/* Create new 'slice'
 */
KS_API ks_slice ks_slice_new(ks_type tp, kso start, kso end, kso step);


/* Get C-style iterator indices
 *
 * if (!ks_slice_getci(slice, array_len, &first, &last, &delta)
 * for (i = first; i != last; i += delta) { ... do operation ... }
 */
KS_API bool ks_slice_get_citer_c(ks_cint start, ks_cint end, ks_cint step, ks_cint len, ks_cint* first, ks_cint* last, ks_cint* delta);
KS_API bool ks_slice_get_citer(ks_slice self, ks_cint len, ks_cint* first, ks_cint* last, ks_cint* delta);

/* Create new 'range'
 */
KS_API ks_range ks_range_new(ks_type tp, ks_int start, ks_int end, ks_int step);


/* Create a new 'tuple' from elements
 * 'newn' absorbs references
 */
KS_API ks_list ks_list_new(ks_ssize_t len, kso* elems);
KS_API ks_list ks_list_newn(ks_ssize_t len, kso* elems);

/* Create a new list from an iterable
 */
KS_API ks_list ks_list_newit(ks_type tp, kso objs);
KS_API ks_list ks_list_newi(kso objs);


/* Clears a list
 */
KS_API void ks_list_clear(ks_list self);

/* Ensures a list can hold 'cap' number of elements
 */
KS_API bool ks_list_reserve(ks_list self, int cap);


/* Add element to list
 */
KS_API bool ks_list_push(ks_list self, kso ob);
KS_API bool ks_list_pushu(ks_list self, kso ob);

/* Push array
 */
KS_API bool ks_list_pusha(ks_list self, ks_cint len, kso* objs);
KS_API bool ks_list_pushan(ks_list self, ks_cint len, kso* objs);

/* Push all elements of an iterable onto a list
 */
KS_API bool ks_list_pushall(ks_list self, kso objs);

/* Inserts an object at a given position
 */
KS_API bool ks_list_insert(ks_list self, ks_cint idx, kso ob);
KS_API bool ks_list_insertu(ks_list self, ks_cint idx, kso ob);

/* Pop an item and return a reference
 */
KS_API kso ks_list_pop(ks_list self);

/* Pop an unused reference from a list
 */
KS_API void ks_list_popu(ks_list self);

/* Delete a given index
 */
KS_API bool ks_list_del(ks_list self, ks_cint idx);

/* Create a new 'attrtuple' subtype
 */
KS_API ks_type ks_attrtuple_newtype(ks_str name, int nmem, kso* mem);

/* Create a new 'tuple' from elements
 * newn absorbs references
 */
KS_API ks_tuple ks_tuple_new(ks_ssize_t len, kso* elems);
KS_API ks_tuple ks_tuple_newn(ks_ssize_t len, kso* elems);


/* Create empty, uninitialize tuple: WARNING: only internal use
 */
KS_API ks_tuple ks_tuple_newe(ks_ssize_t len);

/* Creates a tuple from an iterable, converting all elements
 * NOTE: if the object is already a tuple, a new reference is returned
 */
KS_API ks_tuple ks_tuple_newi(kso objs);

/* Create a new C-style function wrapper
 */
KS_API kso ksf_wrap(ks_cfunc cfunc, const char* sig, const char* doc);

/* Create a new kscript bytecode function
 * 'args' is the tuple of (str) names of all parameters
 * 'n_defa' is the number of default arguments (on the right side of 'args')
 * 'vararg_idx' is the index of the vararg (or -1 if none exists)
 */
KS_API ks_func ks_func_new_k(kso bc, ks_tuple args, int n_defa, kso* defa, int vararg_idx, ks_str sig, ks_str doc);

/* Set defaults for a function (last 'n_defa' parameters)
 */
KS_API void ks_func_setdefa(ks_func self, int n_defa, kso* defa);


/* Create a new partial function with index '0' filled in
 */
KS_API ks_partial ks_partial_new(kso of, kso arg0);


/* Construct a new 'set' object from a list of elements
 */
KS_API ks_set ks_set_new(ks_cint len, kso* elems);

/* Clear a set
 */
void ks_set_clear(ks_set self);


/* Add an object to the set
 */
KS_API bool ks_set_add(ks_set self, kso obj);
KS_API bool ks_set_add_h(ks_set self, kso obj, ks_hash_t hash);

/* Delete an object from the set, and set '*found' to whether or not the item
 *   was in the set
 */
KS_API bool ks_set_del(ks_set self, kso obj, bool* found);
KS_API bool ks_set_del_h(ks_set self, kso obj, ks_hash_t hash, bool* found);

/* Returns whether an object is in the set
 */
KS_API bool ks_set_has(ks_set self, kso obj, bool* out);
KS_API bool ks_set_has_h(ks_set self, kso obj, ks_hash_t hash, bool* out);


/* Add every element of the iterable to the set
 */
KS_API bool ks_set_addall(ks_set self, kso objs);
KS_API bool ks_set_addn(ks_set self, ks_cint len, kso* objs);

/* Delete every element of the iterable from the set
 */
KS_API bool ks_set_delall(ks_set self, kso objs);


/* Return a list of the entries array
 */
KS_API ks_list ks_set_calc_ents(ks_set self);

/* Return a list of the buckets array
 */
KS_API ks_list ks_set_calc_buckets(ks_set self);



/* Create a new dictionary
 * newn absorbs references to the values given
 */
KS_API ks_dict ks_dict_new(struct ks_ikv* ikv);
KS_API ks_dict ks_dict_newn(struct ks_ikv* ikv);

/* Create a new dictionary from key, val interleaved
 */
KS_API ks_dict ks_dict_newkv(int nargs, kso* args);

/* Empty the dictionary
 */
KS_API void ks_dict_clear(ks_dict self);

/* Merge entries from 'from' into 'self'
 */
KS_API bool ks_dict_merge(ks_dict self, ks_dict from);

/* Merge in entries that are in 'ikv'
 */
KS_API bool ks_dict_merge_ikv(ks_dict self, struct ks_ikv* ikv);


/* Look up a key in the dictionary, returning a new reference to it
 * '_ih' ignores lookup errors
 */
KS_API kso ks_dict_get(ks_dict self, kso key);
KS_API kso ks_dict_get_h(ks_dict self, kso key, ks_hash_t hash);
KS_API kso ks_dict_get_ih(ks_dict self, kso key, ks_hash_t hash);
KS_API kso ks_dict_get_c(ks_dict self, const char* ckey);

/* Set a given key to a given value, or update the existing value for that key if it already existed
 *
 */
KS_API bool ks_dict_set(ks_dict self, kso key, kso val);
KS_API bool ks_dict_set_h(ks_dict self, kso key, ks_hash_t hash, kso val);
KS_API bool ks_dict_set_c(ks_dict self, const char* ckey, kso val);

/* Set value, and absorb reference from 'val' */
KS_API bool ks_dict_set_c1(ks_dict self, const char* ckey, kso val);

/* Calculate whether a dictionary contains a given key
 *
 */
KS_API bool ks_dict_has(ks_dict self, kso key, bool* exists);
KS_API bool ks_dict_has_h(ks_dict self, kso key, ks_hash_t hash, bool* exists);
KS_API bool ks_dict_has_c(ks_dict self, const char* key, bool* exists);

/* Delete a given key (and its value) from 'self', if it existed.
 *
 * Attempting to delete a key that didn't exist will not throw an error; if you want to, you should
 *   pass in a reference to a boolean to '*existed' and check that
 * 
 */
KS_API bool ks_dict_del(ks_dict self, kso key, bool* existed);
KS_API bool ks_dict_del_h(ks_dict self, kso key, ks_hash_t hash, bool* existed);


/* Return a list of the entries array
 */
KS_API ks_list ks_dict_calc_ents(ks_dict self);

/* Return a list of the buckets array
 */
KS_API ks_list ks_dict_calc_buckets(ks_dict self);


/* Create a new 'names' object, which wraps a dictionary
 */
KS_API ks_names ks_names_new(ks_dict of, bool copy);


/* Return the logger for a given name
 */
KS_API ks_logger ks_logger_get(ks_str name);
KS_API ks_logger ks_logger_get_c(const char* name);

/* C-style logging functions
 */
KS_API bool ks_logger_clogv(ks_logger self, int level, const char* file, const char* func, int line, const char* fmt, va_list ap);
KS_API bool ks_logger_clog(ks_logger self, int level, const char* file, const char* func, int line, const char* fmt, ...);


/* Template */
#define _ks_logT(_name, _level, ...) do { \
    ks_logger _log = ks_logger_get_c(_name); \
    assert(_log != NULL); \
    bool _haderr = !ks_logger_clog(_log, _level, __FILE__, __func__, __LINE__, __VA_ARGS__); \
    KS_DECREF(_log); \
    assert(!_haderr); \
} while (0);

#define ks_trace(_name, ...) _ks_logT(_name, KS_LOGGER_TRACE, __VA_ARGS__)
#define ks_debug(_name, ...) _ks_logT(_name, KS_LOGGER_DEBUG, __VA_ARGS__)
#define ks_info(_name, ...) _ks_logT(_name, KS_LOGGER_INFO, __VA_ARGS__)
#define ks_warn(_name, ...) _ks_logT(_name, KS_LOGGER_WARN, __VA_ARGS__)
#define ks_error(_name, ...) _ks_logT(_name, KS_LOGGER_ERROR, __VA_ARGS__)
#define ks_fatal(_name, ...) _ks_logT(_name, KS_LOGGER_FATAL, __VA_ARGS__)



/* Create an exception (for returning exceptions from C code)
 */
KS_API ks_Exception ks_Exception_new_c(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, ...);
KS_API ks_Exception ks_Exception_new_cv(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, va_list ap);

/* Create a new module
 */
KS_API ks_module ks_module_new(const char* name, const char* source, const char* doc, struct ks_ikv* ikv);


/*** Misc. Functions ***/

/* Call a a function or function-like object
 */
KS_API kso kso_call(kso func, int nargs, kso* args);

/* Extended calling method which allows you to pass in locals to forward to it
 */
KS_API kso kso_call_ext(kso func, int nargs, kso* args, ks_dict locals, ksos_frame closure);

/* Evaluate a string with a given source name, returning the result
 */
KS_API kso kso_eval(ks_str src, ks_str fname, ks_dict locals);


/* Create an iterable from 'ob'
 */
KS_API kso kso_iter(kso ob);

/* Get the next item in an iterable
 */
KS_API kso kso_next(kso ob);

/* Parse a format string and values, similar to 'KS_ARGS', but for any list of argu
 */
KS_API bool kso_parse(int nargs, kso* args, const char* fmt, ...);

/* Attempt to get the '__attr__' dict from an object, returning NULL if it couldn't be determined
 * NOTE: this does NOT throw an error if it wasn't found
 */
KS_API ks_dict kso_try_getattr_dict(kso obj);

/* Get, set, or delete an attribute from an object
 */
KS_API kso kso_getattr(kso ob, ks_str attr);
KS_API kso kso_getattr_c(kso ob, const char* attr);
KS_API bool kso_setattr(kso ob, ks_str attr, kso val);
KS_API bool kso_delattr(kso ob, ks_str attr);

/* Get, set, or delete an element from an object
 *
 * Variations that take extra parameters are for supporting index operations that take
 *   multiple indices. For 'kso_setelems', the last object in 'keys' is assumed to be the value
 *   it is being set to, and the first is the item itself
 */
KS_API kso kso_getelem(kso ob, kso key);
KS_API bool kso_setelem(kso ob, kso key, kso val);
KS_API bool kso_delelem(kso ob, kso key);
KS_API kso kso_getelems(int n_keys, kso* keys);
KS_API bool kso_setelems(int n_keys, kso* keys);
KS_API bool kso_delelems(int n_keys, kso* keys);


/* Declare that the object is in the 'repr' stack, and return whether it was already in the repr stack,
 *   otherwise, add it
 */
KS_API bool kso_inrepr(kso obj);

/* Stop repr'ing 
 */
KS_API void kso_outrepr();

/* Specific sort types
 */
KS_API bool ks_sort_merge(ks_size_t n, kso* elems, kso* keys, kso cmpfunc);
KS_API bool ks_sort_insertion(ks_size_t n, kso* elems, kso* keys, kso cmpfunc);


/* Sorts 'elems' in place according to 'keys' (may be '==elems'), according to 'cmpfunc'
 *
 * If `cmpfunc` is not `NULL`, then it should be a callable object which has the signature:
 *   (L, R) -> L <= R
 * And returns whether its first argument is less than or equal to its second argument
 */
KS_API bool ks_sort(ks_size_t n, kso* elems, kso* keys, kso cmpfunc);




/** C Iterator API  **/

/* ks_cit - C-iterator type, for quick iteration in C
 *
 * Here's how you use it:
 * ```c
 * kso over;
 * ks_cit it = ks_cit_new(over);
 * kso ob;
 * while (ob = ks_cit_next(&it)) {
 *   ... do stuff with 'ob' ...
 *   KS_DECREF(ob);
 * }
 * ks_cit_done(&it);
 * if (it.exc) {
 *   ... there is an exception, you should probably return 'NULL'
 * }
 * 
 * ```
 * 
 */
typedef struct {

    /* Iterator being iterated over, generated from 'iter(obj)' when the struct
     *   was created
     */
    kso it;

    /* If true, there was an unhandled exception that you must now handle
     * NOTE: This will not be set if the exception was 'OutOfIterException'; that is swallowed
     */
    bool exc;

} ks_cit;

/* Create a new C-style iterator from 'obj'
 */
KS_API ks_cit ks_cit_make(kso obj);

/* Finalize all resources in 'cit', but keep 'exc' as it was
 */
KS_API void ks_cit_done(ks_cit* cit);

/* Return a new reference to the next object in 'cit'
 */
KS_API kso ks_cit_next(ks_cit* cit);



/*** Operators ***/

/* Compute 'L <op> R' */
KS_API kso ks_bop_add(kso L, kso R);
KS_API kso ks_bop_sub(kso L, kso R);
KS_API kso ks_bop_mul(kso L, kso R);
KS_API kso ks_bop_matmul(kso L, kso R);
KS_API kso ks_bop_div(kso L, kso R);
KS_API kso ks_bop_floordiv(kso L, kso R);
KS_API kso ks_bop_mod(kso L, kso R);
KS_API kso ks_bop_pow(kso L, kso R);
KS_API kso ks_bop_binior(kso L, kso R);
KS_API kso ks_bop_binand(kso L, kso R);
KS_API kso ks_bop_binxor(kso L, kso R);
KS_API kso ks_bop_lsh(kso L, kso R);
KS_API kso ks_bop_rsh(kso L, kso R);
KS_API kso ks_bop_lt(kso L, kso R);
KS_API kso ks_bop_gt(kso L, kso R);
KS_API kso ks_bop_le(kso L, kso R);
KS_API kso ks_bop_ge(kso L, kso R);

/* Compute '<op> V' */
KS_API kso ks_uop_pos(kso V);
KS_API kso ks_uop_neg(kso V);
KS_API kso ks_uop_sqig(kso V);

/*  */
KS_API kso ks_contains(kso L, kso R);


/*** C-style string iterator ***/


KS_API struct ks_str_citer ks_str_citer_make(ks_str self);
KS_API ks_ucp ks_str_citer_next(struct ks_str_citer* cit);
KS_API ks_ucp ks_str_citer_peek(struct ks_str_citer* cit);
KS_API ks_ucp ks_str_citer_peek_n(struct ks_str_citer* cit, int n);
KS_API bool ks_str_citer_seek(struct ks_str_citer* cit, ks_ssize_t idx);

/*** Internal Functions ***/

/* Allocation/deallocation */
KS_API kso _kso_new(ks_type tp);
KS_API void _kso_del(kso ob);
KS_API kso _ks_newref(kso ob);
KS_API void _kso_free(kso obj, const char* file, const char* func, int line);

/* For emscripten */
KS_API void _ksem_incref_(kso obj);
KS_API void _ksem_decref_(kso obj);
KS_API char* _ksem_str_c_(kso obj);
KS_API char* _ksem_repr_c_(kso obj);
KS_API ks_cint _ksem_refs_c_(kso obj);
KS_API ks_type _ksem_type_c_(kso obj);
KS_API ks_cint _ksem_iohash_c_();

/* Parse 'args' into a list of addresses, with optionally specifying types. Use the 'KS_ARGS' macros
 */
KS_API bool _ks_argsv(int kk, int nargs, kso* args, const char* fmt, va_list ap);
KS_API bool _ks_args(int nargs, kso* args, const char* fmt, ...);



#endif /* KS_H__ */
