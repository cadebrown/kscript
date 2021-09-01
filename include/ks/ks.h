/* ks/ks.h - Main header file for the kscript C-API
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
 *   is being incluksded in your program.
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
 * @author: Cade Brown <cade@kscript.org>
 * @license: KPL
 */

#pragma once
#ifndef KS_KS_H
#define KS_KS_H


/** Configuration / Headers **/

/* Select the configuration */
#ifdef KS_NO_CONFIG
  /* No config file, assume everything is good */
#else
  /* Config file, which may define platform specific values */
  #include <ks/config.h>
#endif


/* On Emscripten, allow 'dead' code to remain (because it may be called dynamically) */
#if defined(__EMSCRIPTEN__)
  #define KS_EMSCRIPTEN_API EMSCRIPTEN_KEEPALIVE
#else
  #define KS_EMSCRIPTEN_API
#endif

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
  /* On Windows, import/export must be defined explicitly */
  #define KS_API_IMPORT __declspec(dllimport)
  #define KS_API_EXPORT __declspec(dllexport)
#else
  /* Otherwise, nothing particular is required for import/exporting */
  #define KS_API_IMPORT 
  #define KS_API_EXPORT 
#endif

#ifdef KS_BUILD
  /* We are building, so we are exporting the API */
  #ifdef __cplusplus
    #define KS_API KS_API_EXPORT KS_EMSCRIPTEN_API extern "C"
  #else
    #define KS_API KS_API_EXPORT KS_EMSCRIPTEN_API
  #endif

  #define KS_API_DATA KS_API_EXPORT extern 
#else
  /* We are NOT building, so we are importing the API */
  #ifdef __cplusplus
    #define KS_API KS_API_IMPORT KS_EMSCRIPTEN_API extern "C"
  #else
    #define KS_API KS_API_IMPORT KS_EMSCRIPTEN_API
  #endif

  #define KS_API_DATA KS_API_IMPORT extern
#endif


/** C Standard Headers **/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

#include <stdint.h>
#include <stdbool.h>

#include <string.h>
#include <float.h>

#include <errno.h>
#include <limits.h>
#include <assert.h>

#include <math.h>



/** Optional Requirements **/

#ifdef KS_HAVE_GMP
 /* We have GMP, so use that library */
 #define KS_INT_GMP
 #include <gmp.h>
#else
 /* Don't have GMP, so use our version of minigmp */
 #define KS_INT_MINIGMP
 #include <ks/minigmp.h>
#endif



/* Set to 0 to turn off strided string offsets */
/* #define KS_STR_OFF_STRIDE 0 */
#define KS_STR_OFF_STRIDE 64


/** C-types/machine-types **/

/* Specific sized integers, useful for data structures (such as 'set' and 'dict') that can
 *   save memory for specifically sized inputs
 */
typedef int8_t     ks_s8;
typedef uint8_t    ks_u8;
typedef int16_t    ks_s16;
typedef uint16_t   ks_u16;
typedef int32_t    ks_s32;
typedef uint32_t   ks_u32;
typedef int64_t    ks_s64;
typedef uint64_t   ks_u64;

/* Machine-sized floating-point value
 */
typedef double     ks_cf;

/* Machine-size floating-point complex value
 */
typedef struct {
    double re, im;
} ks_cc;

/* Machine-sized integer (should be large enough to hold pointer value/pointer-size)
 */
typedef intptr_t   ks_sint;
typedef uintptr_t  ks_uint;

typedef ks_u8      ks_cp1;
typedef ks_u16     ks_cp2;
typedef ks_u32     ks_cp4;

#define KS_S8_MIN         ((ks_s8)INT8_MIN)
#define KS_S8_MAX         ((ks_s8)INT8_MAX)
#define KS_U8_MIN         ((ks_u8)UINT8_MIN)
#define KS_U8_MAX         ((ks_u8)UINT8_MAX)
#define KS_S16_MIN        ((ks_s16)INT16_MIN)
#define KS_S16_MAX        ((ks_s16)INT16_MAX)
#define KS_U16_MIN        ((ks_u16)UINT16_MIN)
#define KS_U16_MAX        ((ks_u16)UINT16_MAX)
#define KS_S32_MIN        ((ks_s32)INT32_MIN)
#define KS_S32_MAX        ((ks_s32)INT32_MAX)
#define KS_U32_MIN        ((ks_u32)UINT32_MIN)
#define KS_U32_MAX        ((ks_u32)UINT32_MAX)
#define KS_S64_MIN        ((ks_s64)INT64_MIN)
#define KS_S64_MAX        ((ks_s64)INT64_MAX)
#define KS_U64_MIN        ((ks_u64)UINT64_MIN)
#define KS_U64_MAX        ((ks_u64)UINT64_MAX)

#define KS_CF_MAX         ((ks_cf)DBL_MAX)
#define KS_CF_MIN         ((ks_cf)DBL_MIN)
#define KS_CF_EPS         ((ks_cf)DBL_EPSILON)
#define KS_CF_DIG         ((int)DBL_DIG)
#define KS_CF_INF         ((ks_cf)INFINITY)
#define KS_CF_NAN         ((ks_cf)NAN)

#define KS_SINT_MAX       INTPTR_MAX
#define KS_SINT_MIN       INTPTR_MIN

#define KS_UINT_MAX       UINTPTR_MAX
#define KS_UINT_MIN       ((ks_uint)0)

/* Creates a list of NULL-terminated initializers for a (key, val) entry */
#define KS_IKV(...) ((struct ks_ikv[]){ __VA_ARGS__ { NULL, NULL} })


/* C-style initializers for (enum) string key, and C-style integer values 
 * See 'KS_EIKV'
 */
struct ks_eikv {

    /* NUL-terminated C-style string */
    const char* key;

    /* Value */
    ks_sint val;

};

/* Creates a list of NULL-terminated initializers for an enumeration creation */
#define KS_EIKV(...) ((struct ks_eikv[]){ __VA_ARGS__ { NULL, 0 } })


/** Types **/

/* Include this macro at the beginning of structures describing kscript objects */
#define KSO_BASE ks_sint refs; ks_type type;

/* type - a type/class in OOP, which defines a set of attributes and functions for objects 
 *            which adhere to it
 *
 * For a lot of objects, their type does not say which attributes are present (they have an '__attr__'
 *   dict per instance which can hold any attribute), but specific methods on a type may be overriden
 *   such that this behavior is changed. 
 *
 */
typedef struct ks_type_s* ks_type;

/* str - String type, represents an immutable piece of text
 *
 */
typedef struct ks_str_s* ks_str;


/* object - the base type which all other types inherit from
 *
 * All kscript objects have a type which derives from 'object'
 * 
 * You can cast any valid kscript object to a 'kso', like: '(kso)X' and it will be valid
 *
 */
typedef struct kso_s {
    KSO_BASE
}* kso;




/* C-style initializers for string key, and values 
 * See 'KS_IKV'
 */
struct ks_ikv {

    /* NUL-terminated C-style string */
    const char* key;

    /* Value */
    kso val;

};

/* C-style function signature */
typedef kso (*ks_cfunc)(int nargs_, kso* args_);


/* int - (immutable) a whole number, not limited by machine precision
 * 
 */
typedef struct ks_int_s {
    KSO_BASE

#if defined(KS_INT_GMP)

    /* Internal integer structure for GMP */
    mpz_t val;

#elif defined(KS_INT_MINIGMP)

    /* Internal integer structure for MINIGMP */
    mpz_t val;

#else

    #error No KS_INT_XXX defined (no implementation found)

#endif

}* ks_int;

/* enum - base class of all enumerations
 *
 */
typedef struct ks_enum_s {
    struct ks_int_s s_int;

    /* Name of the enumeration member */
    ks_str name;

}* ks_enum;

/* bool - (immutable) represents either true or false, yes/no
 *
 * This is a subtype of 'enum' and 'int', so it behaves exactly as expected
 * 
 */
typedef ks_enum ks_bool;

/* float - floating point, real number
 *
 */
typedef struct ks_float_s {
    KSO_BASE

    ks_cf val;

}* ks_float;

/* complex - floating point, complex number with a 'real' and 'imaginary' part
 * 
 */
typedef struct ks_complex_s {
    KSO_BASE

    ks_cc val;

}* ks_complex;

struct ks_str_s {
    KSO_BASE

    /* Length, in characters/bytes, of the string */
    ks_uint len_c, len_b;

    /* Hash of the string contents*/
    ks_uint v_hash;

    /* NUL-terminated UTF8 stream
     */
    ks_u8* data;

#if KS_STR_OFF_STRIDE

    /* Stride array, wheres 'strides_[i]' gives the byte position of the 
     * (KS_STR_OFF_STRIDE * i)th character in the string. If len_c==len_b (i.e.
     * an ASCII string), this may be NULL, INTERNAL USE ONLY */
    ks_uint* strides_;

#else

    /* No offsets, save the storage. But, some string operations will be expensive for non-ASCII strings */

#endif

};

/* bytes - (immutable) string of bytes
 * 
 * 
 */
typedef struct ks_bytes_s {
    KSO_BASE

    /* Length of the data */
    ks_uint len_b;

    /* Hash of the bytes contents */
    ks_uint v_hash;

    /* Array of byte data */
    ks_u8* data;

}* ks_bytes;

/* slice - Slice indexer
 *
 */
typedef struct ks_slice_s {
    KSO_BASE

    kso start, end, step;

}* ks_slice;


/** Collection/Structure Types **/

/* range - (immutable) integral range
 *
 * Represents all numbers from 'start' (including 'start'), up to but not including 'end',
 *   and includes every 'step'th number (if step==1, all integers that fit the description
 *   are in the range)
 * 
 * TODO: add 'rangex' type for support with floating point numbers
 */
typedef struct ks_range_s {
    KSO_BASE

    /* The start (inclusive), end (exclusive), and step (default: 1)
     */
    ks_int start, end, step;

}* ks_range;


/* list - collection of objects
 *
 */
typedef struct ks_list_s {
    KSO_BASE

    /* Length, in elements, of the list */
    ks_uint len;

    /* Array of elements */
    kso* elems;

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_uint _max_len;

}* ks_list;

/* tuple - (immutable) collection of objects
 */
typedef struct ks_tuple_s {
    KSO_BASE

    /* Length, in elements, of the tuple */
    ks_uint len;

    /* Array of elements */
    kso* elems;

}* ks_tuple;

/* set - collection of unique objects
 *
 * Ordered by insertion order, but sets with objects in a different order are still treated equally
 */
typedef struct ks_set_s {
    KSO_BASE

    /* Length, in elements, of the set */
    ks_uint len_real;

    /* Length of the 'ents' array, which may included deleted entries */
    ks_uint len_ents;

    /* Array of entries, ordered by first insertion */
    struct ks_set_ent {

        /* hash(key) */
        ks_uint hash;

        /* The entries key (keeps reference), or NULL if this entry has been deleted */
        kso key;

    }* ents;


    /* Length of the array of buckets, which is one of 'buks*' members */
    ks_uint len_buks;

    /* Union of different bucket arrays, discriminated based on size */
    union {

        /* when 'len_buks - 1 <= KS_S8_MAX' */ 
        ks_s8*  buks8;
        /* when 'len_buks - 1 <= KS_S16_MAX' */
        ks_s16* buks16;
        /* when 'len_buks - 1 <= KS_S32_MAX' */
        ks_s32* buks32;
        /* when 'len_buks - 1 <= KS_S64_MAX' */
        ks_s64* buks64;

    };

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_uint _max_len_ents, _max_len_buk;

}* ks_set;

/* dict - collection of unique key-value pairs
 *
 * Ordered by insertion order
 */
typedef struct ks_dict_s {
    KSO_BASE

    /* Length, in elements, of the dict */
    ks_uint len_real;

    /* Length of the 'ents' array, which may included deleted entries */
    ks_uint len_ents;

    /* Array of entries, ordered by first insertion */
    struct ks_dict_ent {

        /* hash(key) */
        ks_uint hash;

        /* The entries key (keeps reference), or NULL if this entry has been deleted */
        kso key;

        /* The entries value (keeps reference), or NULL */
        kso val;

    }* ents;


    /* Length of the array of buckets, which is one of 'buks*' members */
    ks_uint len_buks;

    /* Union of different bucket arrays, discriminated based on size */
    union {

        /* when 'len_buks - 1 <= KS_S8_MAX' */ 
        ks_s8*  buks8;
        /* when 'len_buks - 1 <= KS_S16_MAX' */
        ks_s16* buks16;
        /* when 'len_buks - 1 <= KS_S32_MAX' */
        ks_s32* buks32;
        /* when 'len_buks - 1 <= KS_S64_MAX' */
        ks_s64* buks64;

    };

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_uint _max_len_ents, _max_len_buks;

}* ks_dict;


/* graph - Directed graph structure
 *
 * The list of all nodes is given by 'nodes.keys()', and to get a dictionary of edges from
 *   a node 'key', you can use 'nodes[key]'. For example, to get the edge value from 'A'
 *   to 'B', use 'nodes[A][B]'
 */
typedef struct ks_graph_s {
    KSO_BASE

    /* main dictionary, of node mappings */
    ks_dict nodes;

}* ks_graph;

/* bst - Binary Search Tree
 *
 * A sorted (by key) tree
 *
 */
typedef struct ks_bst_s {
    KSO_BASE
    
    /* Structure describing a single node within the BST
     * 'root' is the top of the tree
     */
    struct ks_bst_node {

        /* Pointer to child nodes, L (less than this node), and R (greater than this node) */
        struct ks_bst_node *L, *R;

        /* Key and value of this node */
        kso key, val;
    } *root;

}* ks_bst;


/* queue - Queue structure, more efficient than using a list
 *
 */
typedef struct ks_queue_s {
    KSO_BASE

    /* Structure describing a single node within the queue 
     * 'L' and 'R' are pointers to the first and last nodes in the queue
     */
    struct ks_queue_node {

        /* Pointer to other nodes in the linked list, L (prev), and R (next) */
        struct ks_queue_node *L, *R;

        /* Value of this node */
        kso val;
    } *L, *R;

}* ks_queue;

/* ast - Abstract Syntax Tree representing program structure
 *
 * Sub-types of AST (i.e. AddAST) should be used for specific operations. 
 * Some methods that need to be defined:
 * 
 * TODO: document these
 */
typedef struct ks_ast_s {
    KSO_BASE

    /* Sub-nodes (should be a list of ASTs) */
    ks_list sub;

}* ks_ast;


/* func - represents an executable function object in the kscript VM
 *
 */
typedef struct ks_func_s {
    KSO_BASE

    /* Generic attribute dictionary */
    ks_dict attr;


    /* Number of formal parameters to the function, and the number that is always required */
    int n_pars, n_pars_req;

    /* Index of the spread parameter (or -1 if there is none) */
    int idx_vararg;

    /* Function describing a single function parameter */
    struct ks_func_par {

        /* Name of the parameter, for debugging */
        ks_str name;

        /* The default value for the parameter, or NULL if the parameter is always required 
         *
         */
        kso defa;

    }* pars;

    /* Tells whether this function is a C-style function, or a bytecode function */
    bool is_cfunc;

    union {
        /* C-style function pointer that can be called (only valid if is_cfunc==true) */
        ks_cfunc cfunc;

        /* kscript bytecode function structure (only valid if is_cfunc==false) */
        struct {
            /* Bytecode object attached */
            kso bc;

            /* Closure it is attached to (or NULL if none)
             * type: os.frame
             */
            kso closure;

        } kfunc;
    };

}* ks_func;


/* module - Basic unit of code organization
 *
 */
typedef struct ks_module_s {
    KSO_BASE




}* ks_module;

struct ks_type_s {
    KSO_BASE

    /* Attribute dictionary (.__attr__)
     *
     * A dictionary of all (local) attributes to the type
     * 
     */
    ks_dict attr;

    /* Types which are children of this type
     * (no reference is held to these, since it is a weak reference)
     */
    ks_sint n_subs;
    ks_type* subs;

    /* Integer (byte)size and attribute dictionary offsets of instances */
    ks_sint ob_size, ob_attr;

    /* Number of objects created and deleted */
    ks_sint n_obs_new, n_obs_del;


    /** Special Values (saved as variables here, but also in 'attr') **/
    
    /* Base type of this type */
    ks_type i__base;

    /* Strings representing the shallow name and full name (including modules, subtypes, etc) */
    ks_str i__name, i__fullname;

    /* Template arguments for the type */
    ks_tuple i__template;

    /** Constructors/Destructors **/

    /* In kscript, types are their own constructor; you call them like a function to create a value of that type.
     *
     * For example, 'int("42")'. This is done via the following rules:
     *   * First, an instance is generated via 'tmp = T.__new(T, *args)'
     *   * If 'T.__init' is present, then: 'T.__init(T, *args)' is called as well 
     * 
     */

    /* __new(tp, *args), __init(self, *args), __free(self), __call(self, *args) */
    kso i__new, i__init, i__free, i__call;

    /* __on_template(tp) */
    kso i__on_template;


    /* iter(A), next(A), call(A, *args) */
    kso i__iter, i__next;

    /** Special Functions **/

    /* Builtin Conversions (Numeric) */
    kso i__number, i__integral, i__bool, i__int, i__float, i__complex;
    
    /* Builtin Conversions (Misc) */
    kso i__str, i__bytes, i__set, i__dict;

    /* Attributes */
    kso i__getattr, i__setattr, i__delattr;
    
    /* Elements */
    kso i__getelem, i__setelem, i__delelem;

    /* Contains */
    kso i__contains;

    /* Misc. Properties */
    kso i__hash, i__abs, i__len, i__repr;

    /** Binary Operator Overloads **/

    /* A+B, A-B, A*B, A@B, A/B, A//B, A%B, A**B */
    kso i__add, i__sub, i__mul, i__matmul, i__div, i__floordiv, i__mod, i__pow;

    /* A<=>B, A==B, A!=B, A<B, A<=B, A>B, A>=B */
    kso i__cmp, i__eq, i__ne, i__lt, i__le, i__gt, i__ge;
    
    /* A<<B, A>>B, A|B, A^B, A&B */
    kso i__lsh, i__rsh, i__binior, i__binxor, i__binand;


    /** Unary Operator Overloads **/

    /* +A, -A, ~A */
    kso i__pos, i__neg, i__sqig;

};


/* func.partial - Partial application of a function, which contains arguments that should be 'filled in'
 *
 */
typedef struct ks_partial_s {
    KSO_BASE

    /* The base function/callable */
    kso of;

    /* Number of arguments to be filled in */
    int n_args;

    /* Argument fill in */
    struct {
        /* Index being filled in */
        int idx;

        /* Value used as argument */
        kso val;

    }* args;

}* ks_partial;

/* map - Represents an iterable, async mapping of a function applied to an iterable
 *
 */
typedef struct ks_map_s {
    KSO_BASE

    /* Iterable being iterated over */
    kso it;

    /* Function/callable being applied to each item */
    kso fn;

}* ks_map;


/* filter - Represents a filtered iterable, which only emits elements that pass a filter
 *
 */
typedef struct ks_filter_s {
    KSO_BASE

    /* Iterable being iterated over */
    kso it;

    /* Function/callable being applied to each item, if it returns a truthy value
     *   then the object is emitted
     */
    kso fn;


}* ks_filter;


/* zip - Represents a zipped iterable, which takes a list of iterables and returns tuples
 *         of 1 from each iterable
 *
 * Default behavior is to stop once any of the iterables have stopped. However, if 
 */
typedef struct ks_zip_s {
    KSO_BASE

    /* Number of iterables being zipped through
     */
    int n_its;

    /* List of iterables, may contain NULL if 'extend' is true
     */
    kso* its;

    /* The object to 'extend' the other iterables with once they run out, or NULL
     *   if they should not be extended
     */
    kso extend;

}* ks_zip;

/* batch - Batches iterators into tuples
 *
 */
typedef struct ks_batch_s {
    KSO_BASE

    /* Iterable being iterated over */
    kso it;

    /* The max size of a batch */
    int size;

    /* Whether or not to emit the last 'partial' batch, if it was not 'size' length
     * If true, then behavior depends on 'extend' (either extend it to 'size' with that value, or just emit a shorter batch)
     * If false, the last 'partial' batch is just discarded and not emitted
     */
    bool do_partial;

    /* The object to 'extend' the last incomplete batch with (if there were not enough to make a size of 'batch'), or NULL
     *   if the last batch should be discarded
     * NOTE: This will be IGNORED if 'do_partial==false'
     */
    kso extend;

}* ks_batch;




/* logger - Represents a utility object for logging
 *
 */
typedef struct ks_logger_s {
    KSO_BASE

    /* The (writeable) IO object to output messages to */
    kso io;

    /* The full name of the logger */
    ks_str name;

    /* Current verbosity level */
    ks_sint level;

}* ks_logger;

/* Exception - base type used to capture information about an exception breakpoint
 *
 */
typedef struct ks_exc_s {
    KSO_BASE

    /* The inner exception (or NULL if there was none). This is relevant if an exception
     *   is thrown during the handling of another exception
     */
    struct ks_exc_s* inner;

    /* Arguments to the constructor, relevant for inspecting */
    ks_list args;

    /* String description of what went wrong (if relevant) */
    ks_str what;

}* ks_exc;


/** Macros **/


/* Define a C function which can be wrapped into a kscript function. Appends an '_' after the name
 */
#define KS_FUNC(_name) kso _name##_(int nargs_, kso* args_)

/* Define a C function which can be wrapped into a kscript function. Allows a type and function name
 */
#define KS_TFUNC(_type, _name) kso _type##_##_name##_(int nargs_, kso* args_)

/* Declare a type in C, using static declaration
 */
#define KS_DECL_TYPE(_name) static struct ks_type_s _name##_ss; ks_type _name = &_name##_ss;

/* Declare a module in C, using static declaration
 */
#define KS_DECL_MODULE(_name) static struct ks_module_s _name##_ss; ks_module _name = &_name##_ss;


/* Parse function args, and returns 'NULL' from the current function if they did not parse correctly */
#define KS_ARGS(...) do { \
    if (!_ks_args(nargs_, args_, __VA_ARGS__)) return NULL; \
} while(0)


/* Throws an exception type, generated with a C-style format string (see 'ks_fmt()') 
 * After doing this, you should return NULL or otherwise indicate something was thrown
 */
#define KS_THROW(_tp, ...) do { \
    ks_throw_c(_tp, __FILE__, __func__, __LINE__, __VA_ARGS__); \
} while (0)

/* Throw an 'StopIter' Exception
 */
#define KS_THROW_STOPITER() do { \
    KS_THROW(kst_StopIter, ""); \
} while (0)

/* Throws a key error for an object, and a key that was missing */
#define KS_THROW_KEY(_obj, _key) KS_THROW(kst_KeyError, "%R", _key)

/* Throws a math error */
#define KS_THROW_VAL(_obj, _key) KS_THROW(kst_ValError, "%R", _key)

/* Throws a attr error for an object, and a key that was missing */
#define KS_THROW_ATTR(_obj, _attr) KS_THROW(kst_AttrError, "'%T' object had no attribute %R", _obj, _attr)

/* Generic index error */
#define KS_THROW_INDEX(_obj, _idx) KS_THROW(kst_IndexError, "Index out of range")

/* Throws a type conversion error */
#define KS_THROW_CONV(_from_type, _to_type) KS_THROW(kst_TypeError, "Could not convert '%s' object to '%s'", (_from_type)->i__name->data, (_to_type)->i__name->data)

/* Missing method (typically a '__' method) */
#define KS_THROW_METH(_obj, _meth) KS_THROW(kst_TypeError, "'%T' object had no '%s' method", _obj, _meth)


/* Missing method (typically a '__' method) */
#define KS_THROW_OOM(_desc) KS_THROW(kst_InternalError, "Out of memory: %s", _desc)



/* Creates a new object (via standard allocation methods), and initializes the basic fields (type, refs, and attribute dict if available)
 */
#define KS_NEW(_ctp, _tp) ((_ctp)_ks_new(_tp))

/* Deletes an object allocated with 'KSO_NEW'
 */
#define KS_DEL(_ob) (_ks_del((kso)(_ob)))

/* Record a new reference to a given object */
#define KS_INCREF(_obj) do { ++(_obj)->refs; } while(0)

/* NULL-safe increment */
#define KS_NINCREF(_obj) do { if ((_obj)) { ++(_obj)->refs; } } while(0)

/* Delete a reference to a given object, and then free the object if the object has become unreachable */
#define KS_DECREF(_obj) do {                                           \
    kso _ks_obj = (kso)(_obj);                                        \
    if (--_ks_obj->refs <= 0) {                                       \
        _ks_free(_ks_obj, __FILE__, __func__, __LINE__);             \
    }                                                                  \
} while (0)


/* Decref, NULL-safe version */
#define KS_NDECREF(_obj) do { \
    kso _to = (kso)(_obj); \
    if (_to != NULL) KS_DECREF(_to); \
} while (0)



/** C-API Functions **/

/*** High Level Utilities ***/

/* Initialize the kscript API/runtime */
KS_API void ks_init();

/* Initializes a type (typically used in a C-style pattern, see `types/object.c`) given initializer list
 * NOTE: Expects allocated 'tp', see 'KS_DECL_TYPE' macro
 */
KS_API void ks_init_type(ks_type tp, ks_type base, int sz, const char* name, const char* doc, struct ks_ikv* ikv);


/* Initializes a type (typically used in a C-style pattern, see `types/object.c`) given initializer list
 */
KS_API void ks_init_module(ks_module self, ks_module base, const char* name, const char* doc, struct ks_ikv* ikv);


/*** Low Level Utilities ***/

/* Allocate a pointer to at least 'sz' bytes, or NULL if it can't be allocated
 * NOTE: You can realloc/free with ks_realloc/ks_free
 */
KS_API void* ks_malloc(ks_uint sz);

/* Re-allocate a pointer to hold at least 'sz' bytes
 */
KS_API void* ks_realloc(void* ptr, ks_uint sz);

/* Free a pointer allocated from 'ks_malloc'
 */

KS_API void ks_free(void* ptr);

/* Returns the next prime > x
 */
KS_API ks_uint ks_nextprime(ks_uint x);

/* Returns the hash of a byte array
 */
KS_API ks_uint ks_hash_bytes(ks_uint len, ks_u8* data);

/* Returns the hash of C-style values
 */
KS_API ks_uint ks_hash_cf(ks_cf val);
KS_API ks_uint ks_hash_cc(ks_cc val);

/* Computes the next size in the default kscript reallocation scheme. Given 'cur' current size,
 *   and 'req' required size
 *
 * The main use of this function is in mutable containers that need to amortize their internal
 *   reallocation to O(1) instead of O(n). For example, if a list called 'realloc' every append,
 *   it would be O(n). This function uses an exponential function tuned for best performance/memory tradeoff
 */
KS_API ks_sint ks_nextsize(ks_sint cur, ks_sint req);

/* Parse a string and convert to a C-style float 
 * If len<0, len = strlen(str)
 * Success or error is returned
 */
KS_API bool ks_cf_fromstr(int len, const char* str, ks_cf* res);

/* Convert a C-style float into a C-style string buffer
 * Writes a maximum of 'len' bytes into 'str', returns number of bytes required. If 'result > len',
 *   then output was truncated and NOT accurate. If that happens, you should allocate a larger buffer
 *   and call the function again
 * sci: Whether or not to use scientific format
 * base: Numeric base to output (2, 8, 10, or 16)
 * prec: Number of places after the decimal point
 */
KS_API int ks_cf_tostr(ks_cf val, int len, char* str, bool sci, int base, int prec);

/* Parse a string and convert to a C-style complex
 * See ks_cf_fromstr
 */
KS_API bool ks_cc_fromstr(int len, const char* str, ks_cc* res);

/* Convert a C-style complex to a C-style string buffer
 * See ks_cf_tostr
 */
KS_API int ks_cc_tostr(ks_cc val, int len, char* str, bool sci, int base, int prec);

/*** Object Manipulation ***/

/* Test whether 'L' is a subtype of R (or L is an instance of R)
 */
KS_API bool ks_issub(ks_type L, ks_type R);
KS_API bool ks_isinst(kso L, ks_type R);

/* Attempt 'hash(L)' (and store in '*res')
 * Returns success or error
 */
KS_API bool ks_hash(kso L, ks_uint* res);

/* Attempt to convert 'L' to a truth value (and store in '*res')
 * Returns success or error
 */
KS_API bool ks_truth(kso L, bool* res);

/* Attempt 'L==R' (and store in '*res')
 * Returns success or error
 */
KS_API bool ks_eq(kso L, kso R, bool* res);

/* Attempt 'L<=>R' (and store in '*res')
 * Returns success or error
 */
KS_API bool ks_cmp(kso L, kso R, int* res);

/* Attempt 'iter(L)', which creates an iterable from an object
 * Returns result, or NULL for error
 */
KS_API kso ks_iter(kso L);

/* Attempt 'next(L)', which returns the next iterable from an object
 * Returns result, or NULL for error
 */
KS_API kso ks_next(kso L);

/* Attempt 'L.key'
 * Returns result, or NULL for error
 */
KS_API kso ks_getattr(kso L, kso key);

/* Attempt 'L.key = val'
 * Returns result, or NULL for error
 */
KS_API kso ks_setattr(kso L, kso key, kso val);

/* Attempt 'del L.key'
 * Returns result, or NULL for error
 */
KS_API kso ks_delattr(kso L, kso key);

/* Attempt 'L[key]'
 * Returns result, or NULL for error
 */
KS_API kso ks_getelem(kso L, kso key);

/* Attempt 'L[key] = val'
 * Returns result, or NULL for error
 */
KS_API kso ks_setelem(kso L, kso key, kso val);

/* Attempt 'del L[key]'
 * Returns result, or NULL for error
 */
KS_API kso ks_delelem(kso L, kso key);

/* Attempts to convert 'L' into a specific C-type
 * Returns success or error
 */
KS_API bool ks_get_si(kso L, ks_sint* res);
KS_API bool ks_get_ui(kso L, ks_uint* res);
KS_API bool ks_get_cf(kso L, ks_cf* res);
KS_API bool ks_get_cc(kso L, ks_cc* res);


/*** Higher Level Utilities ***/

/* Parse 'args' into a list of addresses, with optionally specifying types
 * NOTE: Use 'KS_ARGS' for this function, see documentation there
 */
KS_API bool _ks_argsv(int kk, int nargs, kso* args, const char* fmt, va_list ap);
KS_API bool _ks_args(int nargs, kso* args, const char* fmt, ...);

/* Apply C-style printf-like formatting, and return as a string
 *
 * Format specifiers:
 *   %i: C-style 'int' 
 *   %c: 'char', or 'ks_ucp'
 *   %l: 'ks_sint' (+aliases)
 *   %u: 'ks_uint' (+aliases)
 *   %f: 'ks_cfloat'
 *   %p: 'void*': (+pointer types)
 *   %s: 'char*': (NUL-terminated string, or use '%.*s' for string of a given length (in bytes))
 *   %O: 'kso': formats object in raw mode, which never calls anything else
 *   %T: 'kso': formats the type name
 *   %S: 'kso': formats 'str(ob)'
 *   %R: 'kso': formats 'repr(ob)'
 *   %J: char*, int, kso*: formats 's.join(objs)'
 * 
 */
KS_API ks_str ks_fmt(const char* fmt, ...);
KS_API ks_str ks_fmtv(const char* fmt, va_list ap);

/* Formatting with purely kscript objects --
 *
 * TODO: document format specifiers
 */
KS_API bool ks_fmto(kso io, const char* fmt, int nargs, kso* args);


/** Type Utilities **/

/* Create a new C-style function wrapper, given the signature (<fullname>(<pars>)), and documentation string
 */
KS_API kso ksf_wrap(ks_cfunc cfunc, const char* sig, const char* doc);

/* Constructors for 'int' */
KS_API ks_int ks_int_new(ks_type tp, int len, const char* src, int base);
KS_API ks_int ks_int_newz(ks_type tp, mpz_t val);
KS_API ks_int ks_int_news(ks_sint val);
KS_API ks_int ks_int_newu(ks_uint val);

/* Constructors for 'float' */
KS_API ks_float ks_float_new(ks_type tp, int len, const char* src, int base);
KS_API ks_float ks_float_newf(ks_type tp, ks_cf val);

/* Constructors for 'complex' */
KS_API ks_complex ks_complex_new(ks_type tp, int len, const char* src, int base);
KS_API ks_complex ks_complex_newf(ks_type tp, ks_cf re, ks_cf im);

/* Constructor for new 'bytes' object 
 *   ks_bytes_newn: Create such that the new object owns 'data' (which must be allocated by 'ks_malloc'). DO NOT FREE AFTER CALLING THIS!
 */
KS_API ks_bytes ks_bytes_new(ks_type tp, ks_uint len_b, const ks_u8* data);
KS_API ks_bytes ks_bytes_newn(ks_type tp, ks_uint len_b, ks_u8* data);

/* Constructors for 'str', given an encoding 
 * 
 *   ks_str_new: Assume 'data' is in UTF-8 encoding, and if 'len_b < 0', then len_b=strlen(data),
 *                 and 'data' must be NUL-terminated
 */
KS_API ks_str ks_str_new_UCS2(ks_type tp, ks_sint len_c, const ks_cp2* data);
KS_API ks_str ks_str_new_UCS4(ks_type tp, ks_sint len_c, const ks_cp4* data);
KS_API ks_str ks_str_new(ks_type tp, ks_sint len_b, const ks_cp1* data);


/* String conversion routines, which convert a range of string into
 *   a C-style buffer of appropriate length. The number returned is the number
 *   of *bytes* written to 'data' or '*data'
 * For UCS2, UCS4, the 'data' buffer is assumed to be fully allocated
 * For UTF8, '*data' should be NULL or allocated via 'ks_malloc', and must be freed
 *   with 'ks_free' only
 */
KS_API int ks_str_cvt_UCS2(ks_str L, ks_uint start, ks_uint len_c, ks_cp2* data);
KS_API int ks_str_cvt_UCS4(ks_str L, ks_uint start, ks_uint len_c, ks_cp4* data);
KS_API int ks_str_cvt_UTF8(ks_str L, ks_uint start, ks_uint len_c, ks_cp1** data);

/* Constructors for 'bytes' */
KS_API ks_bytes ks_bytes_new(ks_type tp, ks_uint len, const ks_cp1* data);

KS_API ks_tuple ks_tuple_new(ks_type tp, ks_uint len, kso* elems);

KS_API ks_list ks_list_new(ks_type tp, ks_uint len, kso* elems);

KS_API ks_set ks_set_new(ks_type tp, ks_uint len, kso* elems);

/* Create a new dictionary from a C-style key-value initializer
 */
KS_API ks_dict ks_dict_new(ks_type tp, struct ks_ikv* ikv);


/* Creates a new 'zip' */
KS_API ks_zip ks_zip_new(ks_type tp, ks_uint len, kso* its, kso extend);

/* Creates a new 'batch' */
KS_API ks_batch ks_batch_new(ks_type tp, kso it, int size, bool do_partial, kso extend);

/* Creates a new 'map' */
KS_API ks_map ks_map_new(ks_type tp, kso it, kso fn);

/* Creates a new 'filter' */
KS_API ks_filter ks_filter_new(ks_type tp, kso it, kso fn);

/* Create a new AST */
KS_API ks_ast ks_ast_new(ks_type tp, int n_sub, kso* sub);


/** Internal Utilities **/

KS_API kso _ks_new(ks_type tp);
KS_API void _ks_del(kso ob);
KS_API kso _ks_newref(kso ob);
KS_API void _ks_free(kso obj, const char* file, const char* func, int line);




/** Global Variables **/

#define KS_NONE                    (ksg_none)
#define KS_DOTDOTDOT               (ksg_dotdotdot)
#define KS_UNDEFINED               (ksg_undefined)
#define KS_TRUE                    (ksg_true)
#define KS_FALSE                   (ksg_false)
#define KS_INF                     (ksg_inf)
#define KS_NEGINF                  (ksg_neginf)
#define KS_NAN                     (ksg_nan)

#define KS_BOOL(_cond)             ((kso)((_cond) ? KS_TRUE : KS_FALSE))


KS_API_DATA kso
    ksg_none,
    ksg_dotdotdot,
    ksg_undefined
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

KS_API_DATA ks_list
    ksg_path
;

KS_API_DATA ks_dict 
    ksg_globals,
    ksg_config,
    ksg_inter_vars
;

/* Modules */
KS_API_DATA ks_module
    ksg_os,
    ksg_m
;

/* Types */
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
      kst_complex,
    kst_str,
    kst_bytes,
    kst_regex,
    kst_range,
    kst_slice,
    kst_tuple,
    kst_list,
    kst_set,
    kst_dict,
    kst_map,
    kst_filter,
    kst_zip,

    kst_type,
    kst_func,
    kst_partial,
    
    kst_logger,
    kst_queue,
    kst_bst,
    kst_graph,
    
    kst_ast,
    kst_code,

    kst_exc,
      kst_StopIter,
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
            kst_DivError,
          kst_ArgError,
          kst_SizeError,
        kst_OSError,
      kst_Warning,
        kst_PlatformWarning,
        kst_SyntaxWarning
;

#endif /* KS_CONFIG_H */
