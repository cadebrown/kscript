/* ks/ks.h - kscript C API definitions
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
 *   are typedef'd to the type specific for this build.
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

#ifndef KS_NO_CONFIG
 #include <ks/config.h>
#endif

/** Other Includes **/

#ifdef KS_HAVE_STDLIB_H
 #include <stdlib.h>
#endif
#ifdef KS_HAVE_STDIO_H
 #include <stdio.h>
#endif
#ifdef KS_HAVE_STDARG_H
 #include <stdarg.h>
#endif
#ifdef KS_HAVE_STDDEF_H
 #include <stddef.h>
#endif
#ifdef KS_HAVE_UNISTD_H
 #include <unistd.h>
#endif
#ifdef KS_HAVE_ERRNO_H
 #include <errno.h>
#endif

#ifdef KS_HAVE_STRING_H
 #include <string.h>
#endif

#ifdef KS_HAVE_ASSERT_H
 #include <assert.h>
#else
 #define assert(_x) do { if (!(_x)) { \
    fprintf(stderr, "assertion failed: '%s' (%s:%i)", #_x, __FILE__, __LINE__); \
    exit(1); \
 } } while (0)
#endif

#ifdef KS_HAVE_STDBOOL_H
 #include <stdbool.h>
#else
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

#ifdef KS_HAVE_STDINT_H
 #include <stdint.h>
#else
 /* TODO: add defaults? */
#endif

#ifdef KS_HAVE_LIMITS_H
 #include <limits.h>
#endif
#ifdef KS_HAVE_FLOAT_H
 #include <float.h>
#endif
#ifdef KS_HAVE_MATH_H
 #include <math.h>
#endif

#ifdef KS_HAVE_DLFCN_H
 #include <dlfcn.h>
#endif

/* WebAssembly/Emscription ()
 *
 * Adds support for compiling for the 'web' platform
 *
 */
#ifdef KS_IN_EMSCRIPTEN
 #include <emscripten.h>
#endif

/* pthreads (--with-pthreads) 
 *
 * Adds support for true threads, mutexes, and so forth
 *
 */
#ifdef KS_HAVE_PTHREADS
 #include <pthread.h>
#endif

/* GMP (--with-gmp) 
 *
 * Adds support for (faster) large integer components
 *
 */
#ifdef KS_HAVE_GMP
 #define KS_INT_GMP
 #define KS_INT_FULLGMP
 #include <gmp.h>
#else
 #define KS_INT_GMP
 #define KS_INT_MINIGMP
 #include <ks/minigmp.h>
#endif


/** Constants **/

#include <ks/const.h>
#include <ks/colors.h>


/** Type Definitions **/

#include <ks/types.h>

/** Always-Included Modules **/

#include <ks/io.h>
#include <ks/os.h>


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

KS_API extern kso
    ksg_none,
    ksg_undefined,
    ksg_dotdotdot
;
KS_API extern ks_bool
    ksg_true,
    ksg_false
;

KS_API extern ks_float
    ksg_inf,
    ksg_neginf,
    ksg_nan
;

KS_API extern ksos_mutex
    ksg_GIL
;

KS_API extern ksos_thread
    ksg_main_thread
;

KS_API extern ks_dict
    ksg_globals,
    ksg_config,
    ksg_inter_vars
;

KS_API extern ks_type

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
    kst_range,
    kst_slice,
    kst_list,
    kst_tuple,
    kst_set,
    kst_dict,
    kst_names,
    kst_graph,
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

    kst_ast,
    kst_code,

    kst_Exception,
      kst_OutOfIterException,
      kst_Error,
        kst_InternalError,

        kst_SyntaxError,
        kst_ImportError,

        kst_TypeError,
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

KS_API extern ks_func 
    ksf_any,
    ksf_all,
    ksf_min,
    ksf_max,
    ksf_sum,
    ksf_pow,

    ksf_open,
    ksf_close,

    ksf_eval,
    ksf_exec,

    ksf_print,

    ksf_hash,
    ksf_abs,
    ksf_len,
    ksf_repr,
    ksf_id,

    ksf_ord,
    ksf_chr,

    ksf_issub,
    ksf_isinst,
    
    ksf_iter,
    ksf_next
;


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

/* Throws a attr error for an object, and a key that was missing */
#define KS_THROW_ATTR(_obj, _attr) KS_THROW(kst_AttrError, "'%T' object had no attribute %R", _obj, _attr)

/* Generic index error */
#define KS_THROW_INDEX(_obj, _idx) KS_THROW(kst_KeyError, "Index out of range")

/* Throws a type conversion error */
#define KS_THROW_CONV(_from_type, _to_type) KS_THROW(kst_Error, "Could not convert '%s' object to '%s'", (_from_type)->i__name->data, (_to_type)->i__name->data)

/* Missing method (typically a '__' method) */
#define KS_THROW_METH(_obj, _meth) KS_THROW(kst_TypeError, "'%T' object had no '%s' method", _obj, _meth)


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

/* Import a module with a given name
 */
KS_API ks_module ks_import(ks_str name);

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
 * 
 */
KS_API ks_str ks_fmt(const char* fmt, ...);
KS_API ks_str ks_fmtv(const char* fmt, va_list ap);


/*** Type Functions ***/

/* Create a new type (extending 'base') with a given name, C-size, and position of an '__attr' dictionary
 *
 * Use 'sz=0' or 'attr_pos=0' to just use base's without modification
 */
KS_API ks_type ks_type_new(const char* name, ks_type base, int sz, int attr_pos, struct ks_ikv* ikv);

/* Return a type attribute
 */
KS_API kso ks_type_get(ks_type self, ks_str attr);

/* Sets an attribute of the type
 */
KS_API bool ks_type_set(ks_type self, ks_str attr, kso val);
KS_API bool ks_type_set_c(ks_type self, const char* attr, kso val);

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


/* Compare 'L' and 'R', returning either a comparator, or a boolean telling equality
 */
KS_API int ks_str_cmp(ks_str L, ks_str R);
KS_API bool ks_str_eq(ks_str L, ks_str R);
KS_API bool ks_str_eq_c(ks_str L, const char* data, ks_ssize_t len_b);

/* Convert between strings of length 1 and ordinal codepoints
 */
KS_API ks_str ks_str_chr(ks_ucp ord);
KS_API ks_ucp ks_str_ord(ks_str chr);

/* Split a string
 */
KS_API ks_list ks_str_split(ks_str self, ks_str by);
KS_API ks_list ks_str_split_c(const char* self, const char* by);


/* Create a new 'bytes'
 */
KS_API ks_bytes ks_bytes_new(ks_ssize_t len_b, const char* data);

/* Creates a new 'bytes' object, which should absorb the data pointer passed to it
 *   (which should be allocated via 'ks_malloc()'). Do not free the data after calling this
 */
KS_API ks_bytes ks_bytes_newn(ks_ssize_t len_b, char* data);

/* Calculate the length, in characters, of a UTF-8 string
 */
KS_API ks_ssize_t ks_str_lenc(ks_ssize_t len_b, const char* data);


/* Create a new 'tuple' from elements
 * 'newn' absorbs references
 */
KS_API ks_list ks_list_new(ks_ssize_t len, kso* elems);
KS_API ks_list ks_list_newn(ks_ssize_t len, kso* elems);

/* Clears a list
 */
KS_API void ks_list_clear(ks_list self);

/* Add element to list
 */
KS_API bool ks_list_push(ks_list self, kso ob);
KS_API bool ks_list_pushu(ks_list self, kso ob);

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

/* Create a new 'tuple' from elements
 * newn absorbs references
 */
KS_API ks_tuple ks_tuple_new(ks_ssize_t len, kso* elems);
KS_API ks_tuple ks_tuple_newn(ks_ssize_t len, kso* elems);


/* Create a new C-style function wrapper
 */
KS_API kso ksf_wrap(ks_cfunc cfunc, const char* sig, const char* doc);

/* Create a new partial function with index '0' filled in
 */
KS_API ks_partial ks_partial_new(kso of, kso arg0);

/* Create a new dictionary
 * newn absorbs references to the values given
 */
KS_API ks_dict ks_dict_new(struct ks_ikv* ikv);
KS_API ks_dict ks_dict_newn(struct ks_ikv* ikv);

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



/* Add a node to a graph
 */
KS_API bool ks_graph_add_node(ks_graph self, kso val);

/* Add an edge to a graph
 */
KS_API bool ks_graph_add_edge(ks_graph self, ks_cint from, ks_cint to, kso val);

/* Clear a graph
 */
KS_API void ks_graph_clear(ks_graph self);



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
KS_API kso kso_call_ext(kso func, int nargs, kso* args, ks_dict locals);


/* Create an iterable from 'ob'
 */
KS_API kso kso_iter(kso ob);

/* Get the next item in an iterable
 */
KS_API kso kso_next(kso ob);


/* Attempt to get the '__attr__' dict from an object, returning NULL if it couldn't be determined
 * NOTE: this does NOT throw an error if it wasn't found
 */
KS_API ks_dict kso_try_getattr_dict(kso obj);

/* Get, set, or delete an attribute from an object
 */
KS_API kso kso_getattr(kso ob, ks_str attr);
KS_API bool kso_setattr(kso ob, ks_str attr);
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


/*** Operators ***/

/* Compute 'L <op> R' */
KS_API kso ks_bop_add(kso L, kso R);
KS_API kso ks_bop_sub(kso L, kso R);
KS_API kso ks_bop_mul(kso L, kso R);
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


/*** Internal Functions ***/

/* Allocation/deallocation */
KS_API kso _kso_new(ks_type tp);
KS_API void _kso_del(kso ob);
KS_API kso _ks_newref(kso ob);
KS_API void _kso_free(kso obj, const char* file, const char* func, int line);


/* Parse 'args' into a list of addresses, with optionally specifying types. Use the 'KS_ARGS' macros
 */
KS_API bool _ks_args(int nargs, kso* args, const char* fmt, ...);



#endif /* KS_H__ */
