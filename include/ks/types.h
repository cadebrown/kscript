/* ks/types.h - kscript builtin types
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_TYPES_H__
#define KS_TYPES_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/** C types **/

/* C-style signed integer (must be able to hold pointers, or sizes)
 *
 * A signed integer value, which can represent integers passed to and received from C-API calls
 * 
 */
typedef  intptr_t  ks_cint;

/* C-style unsigned integer (must be able to hold pointers, or sizes)
 *
 * An unsigned integer value, which can represent sizes, pointers, or indicies
 * 
 */
typedef uintptr_t  ks_uint;

/* C-style floating point value
 *
 * Although we could try to detect things like 'long double', '_Float128', '__float128' and so forth
 *   it is probably best for portability that the primary 
 * 
 */
typedef double ks_cfloat;

/* C-style complex floating point value
 *
 * This represents a complex value, which has a real and imaginary component. The value is 
 *   given mathmeatically as 'x := re + im * i', where 'i' is the principle square root of '-1'
 * 
 */
typedef struct {

    /* Components (real, imaginary) */
    ks_cfloat re, im;

} ks_ccomplex;

/* Unicode codepoint, which can stand for a single character
 *
 * Negative values indicate errors
 */
typedef int ks_ucp;

/* Specifically sized integers, useful for data structures (such as 'set' and 'dict') that can
 *   save memory for specifically sized inputs
 */
typedef int8_t     ks_sint8_t;
typedef uint8_t    ks_uint8_t;
typedef int16_t    ks_sint16_t;
typedef uint16_t   ks_uint16_t;
typedef int32_t    ks_sint32_t;
typedef uint32_t   ks_uint32_t;
typedef int64_t    ks_sint64_t;
typedef uint64_t   ks_uint64_t;

/* Aliases */
typedef ks_uint ks_hash_t;
typedef ks_cint ks_ssize_t;
typedef ks_uint ks_size_t;

#define KS_SINT8_MIN               ((ks_sint8_t)INT8_MIN)
#define KS_SINT8_MAX               ((ks_sint8_t)INT8_MAX)
#define KS_UINT8_MIN               ((ks_uint8_t)UINT8_MIN)
#define KS_UINT8_MAX               ((ks_uint8_t)UINT8_MAX)
#define KS_SINT16_MIN              ((ks_sint16_t)INT16_MIN)
#define KS_SINT16_MAX              ((ks_sint16_t)INT16_MAX)
#define KS_UINT16_MIN              ((ks_uint16_t)UINT16_MIN)
#define KS_UINT16_MAX              ((ks_uint16_t)UINT16_MAX)
#define KS_SINT32_MIN              ((ks_sint32_t)INT32_MIN)
#define KS_SINT32_MAX              ((ks_sint32_t)INT32_MAX)
#define KS_UINT32_MIN              ((ks_uint32_t)UINT32_MIN)
#define KS_UINT32_MAX              ((ks_uint32_t)UINT32_MAX)
#define KS_SINT64_MIN              ((ks_sint64_t)INT64_MIN)
#define KS_SINT64_MAX              ((ks_sint64_t)INT64_MAX)
#define KS_UINT64_MIN              ((ks_uint64_t)UINT64_MIN)
#define KS_UINT64_MAX              ((ks_uint64_t)UINT64_MAX)

#define KS_CINT_MAX                INTPTR_MAX
#define KS_CINT_MIN                INTPTR_MIN)

#define KS_UINT_MAX                UINTPTR_MAX
#define KS_UINT_MIN                ((ks_uint)0)

#define KS_CFLOAT(_val) _val

#define KS_CFLOAT_MAX         ((ks_cfloat)DBL_MAX)
#define KS_CFLOAT_MIN         ((ks_cfloat)DBL_MIN)
#define KS_CFLOAT_EPS         ((ks_cfloat)DBL_EPSILON)
#define KS_CFLOAT_DIG         ((int)DBL_DIG)
#define KS_CFLOAT_INF         ((ks_cfloat)INFINITY)
#define KS_CFLOAT_NAN         ((ks_cfloat)NAN)


#define KS_CC_MAKE(_re, _im) ((ks_ccomplex){ .re = (_re), .im = (_im) })

/* Check if two are equal */
#define KS_CC_EQ(_A, _B) ((_A).re == (_B).re && (_A).im == (_B).im)
#define KS_CC_EQRI(_A, _re, _im) ((_A).re == (_re) && (_A).im == _im)

/* Compute absolute value, phase, etc. */
#define KS_CC_ABS(_A) (hypot((_A).re, (_A).im))
#define KS_CC_SQABS(_A) ((_A).re*(_A).re + (_A).im*(_A).im)
#define KS_CC_PHASE(_A) (atan2((_A).im, (_A).re))
#define KS_CC_NEG(_A) KS_CC_MAKE(-(_A).re, -(_A).im)
#define KS_CC_CONJ(_A) KS_CC_MAKE((_A).re, -(_A).im)
#define KS_CC_POLAR(_rad, _phase) KS_CC_MAKE((_rad) * cos(_phase), (_rad) * sin(_phase))


/* abs(KS_CINT_MIN), but as a 'ks_uint', so that it does not mess up the value
 *   when using two's complement
 */
#define KS_CINT_MIN_ABS            (1 + (ks_uint)(-(KS_CINT_MIN+1)))


/** Object Types **/

/* Include this macro at the beginning of structures describing kscript objects */
#define KSO_BASE ks_cint refs; ks_type type; 

/* 'type' - a type/class in OOP, which defines a set of attributes and functions for objects 
 *            which adhere to it
 *
 * For a lot of objects, their type does not say which attributes are present (they have an '__attr__'
 *   dict per instance which can hold any attribute), but specific methods on a type may be overriden
 *   such that this behavior is changed. 
 *
 */
typedef struct ks_type_s* ks_type;
typedef struct ks_str_s* ks_str;



/* 'object' - the base type which all other types inherit from
 *
 * All kscript objects have a type which derives from 'object'
 * 
 * You can cast any valid kscript object to a 'kso', like: '(kso)X' and it will be valid
 *
 */
typedef struct kso_s {
    KSO_BASE
}* kso;


/** Numeric Types **/

/* 'number' - abstract base type of all builtin numbers
 *
 */
typedef kso ks_number;


/* 'int' - (immutable) a whole number, not limited by machine precision
 * 
 */
typedef struct ks_int_s {
    KSO_BASE

    #if defined(KS_INT_GMP)

    /* Internal integer for GMP */
    mpz_t val;

    #elif defined(KS_INT_MINIGMP)

    /* Internal integer for MINIGMP */
    mpz_t val;

    #endif

}* ks_int;

/* 'enum' - base class of all enumerations
 *
 */
typedef struct ks_enum_s {
    struct ks_int_s s_int;

    /* Name of the enumeration member */
    ks_str name;

}* ks_enum;


/* 'bool' - (immutable) represents either true or false, yes/no
 *
 * This is a subtype of 'int', so it behaves exactly as expected
 * 
 */
typedef ks_enum ks_bool;

/* 'float' - floating point, real number
 *
 */
typedef struct ks_float_s {
    KSO_BASE

    ks_cfloat val;

}* ks_float;

/* 'complex' - floating point, complex number with a 'real' and 'imaginary' part
 * 
 */
typedef struct ks_complex_s {
    KSO_BASE

    ks_ccomplex val;

}* ks_complex;

/** Collection/Iterable Types **/

/* 'str' - (immutable) string of unicode characters
 *
 * 
 */
struct ks_str_s {
    KSO_BASE

    /* Length, in bytes, of the string (for ASCII strings, this is also the number of characters) */
    ks_size_t len_b;

    /* Length, in characters, of the string */
    ks_size_t len_c;

    /* Hash of the string contents (ks_hash_bytes(x->chr, x->len_b)) */
    ks_hash_t v_hash;

    #if KS_STR_OFF_EVERY

    /*
     * KS_STR_OFF_EVERY - controls string offset calculations, used to amortize string operations
     *   to O(1), to avoid (for example) string iteration to be O(N^2)
     * 
     * This is achieved by creating a per-string lookup table to quickly seek to a check point
     *   (which is at most `KS_STR_OFF_EVERY/2` characters away from any given point), then
     *   manually skipping over characters until the current index is the sought after one
     */
    ks_size_t* _offs;

    #endif /* KS_STR_OFF_EVERY */

    /* String data (UTF8)
     *
     * These are the bytes of the string, in UTF8 encoding
     * 
     */
    char* data;

};

/* Tell whether a string contains ASCII-only data (i.e. bytes==characters) */
#define KS_STR_IS_ASCII(_str) ((_str)->len_b == (_str)->len_c)

/* String iterator type */
typedef struct ks_str_iter_s {
    KSO_BASE

    ks_str of;

    /* Current position (in bytes) that the iterator is at */
    ks_cint pos;

}* ks_str_iter;


/* 'bytes' - (immutable) string of bytes
 * 
 * 
 */
typedef struct ks_bytes_s {
    KSO_BASE

    /* Length of the data */
    ks_size_t len_b;

    /* Hash of the bytes contents (ks_hash_bytes(x->byt, x->len_b)) */
    ks_hash_t v_hash;

    /* Array of byte data */
    unsigned char* data;

}* ks_bytes;



/* Regex NFA types */
enum {

    /* End/accepting state */
    KS_REGEX_NFA_END = 0,

    /* Epsilon transitions on line start and end */
    KS_REGEX_NFA_LINESTART,
    KS_REGEX_NFA_LINEEND,

    /* Epsilon transition on a word break */
    KS_REGEX_NFA_WORDBREAK,

    /* Epsilon transition at any time */
    KS_REGEX_NFA_EPS,

    /* Matches a single (unicode) character */
    KS_REGEX_NFA_UCP,

    /* Matches a single (unicode) category */
    KS_REGEX_NFA_CAT,

    /* Matches any characters within a character set (inverted for 'KS_REGEX_NFA_NOT') */
    KS_REGEX_NFA_ANY,
    KS_REGEX_NFA_NOT

};


/* 'regex' - regular expression pattern, which can be used to match strings
 *
 * 
 * 
 */
typedef struct ks_regex_s {
    KSO_BASE

    /* Expression which generated the regex */
    ks_str expr;

    /* Number of states in the regular expression */
    int n_states;

    /* Array of states in the regular expression */
    struct ks_regex_nfa {

        /* Kind of NFA node, see 'KS_REGEX_NFA_*' values */
        int kind;

        /* Outgoing edges to other states, or -1 if they don't exist */
        int to0, to1;

        union {

            /* For 'KS_REGEX_NFA_UCP' */
            ks_ucp ucp;


            /* For 'KS_REGEX_NFA_ANY' and 'KS_REGEX_NFA_NOT' */
            struct {

                /* Bitset of whether the set contains a byte. Only valid
                 *   for ASCII characters, and bytes when in binary mode
                 */
                bool* has_byte;

                /* Bitset of whether the set contains all characters within
                 *   a categoriy. Indexes are 'ksucd_cat'
                 */
                bool* has_cat;

                /* Array of unicode characters in the set (TODO: hashtable?) */
                int n_ext;
                ks_ucp* ext;

            } set;

        };

    }* states;

    /* Initial and final states of the regular expression */
    int s0, sf;

}* ks_regex;


/* 'range' - (immutable) integral range
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

/* 'list' - collection of objects
 *
 */
typedef struct ks_list_s {
    KSO_BASE

    /* Length, in elements, of the list */
    ks_size_t len;

    /* Array of elements */
    kso* elems;

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_size_t _max_len;

}* ks_list;

/* 'list.__iter' iterator type */
typedef struct ks_list_iter_s {
    KSO_BASE

    ks_list of;

    /* Current position (in bytes) that the iterator is at */
    ks_cint pos;

}* ks_list_iter;


/* 'tuple' - (immutable) collection of objects
 */
typedef struct ks_tuple_s {
    KSO_BASE

    /* Length, in elements, of the tuple */
    ks_size_t len;

    /* Array of elements */
    kso* elems;

}* ks_tuple;

/* 'set' - collection of unique objects
 *
 * Ordered by insertion order, but sets with objects in a different order are still treated equally
 *
 */
typedef struct ks_set_s {
    KSO_BASE

    /* Length, in elements, of the set */
    ks_size_t len_real;

    /* Length of the 'ents' array, which may included deleted entries */
    ks_size_t len_ents;

    /* Array of entries, ordered by first insertion */
    struct ks_set_ent {

        /* hash(key) */
        ks_hash_t hash;

        /* The entries key (keeps reference), or NULL if this entry has been deleted */
        kso key;

    }* ents;


    /* Length of the array of buckets, which is one of 'buckets_*' members */
    ks_size_t len_buckets;

    /* Union of different bucket arrays, discriminated based on size */
    union {

        /* when 'len_buckets - 1 <= KS_SINT8_MAX' */ 
        ks_sint8_t * buckets_s8;
        /* when 'len_buckets - 1 <= KS_SINT16_MAX' */
        ks_sint16_t* buckets_s16;
        /* when 'len_buckets - 1 <= KS_SINT32_MAX' */
        ks_sint32_t* buckets_s32;
        /* when 'len_buckets - 1 <= KS_SINT64_MAX' */
        ks_sint64_t* buckets_s64;

    };

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_size_t _max_len_ents, _max_len_buckets_b;

}* ks_set;

/* 'dict' - mapping of keys to values, where keys are unique
 *
 * Ordered by insertion order
 * 
 */
typedef struct ks_dict_s {
    KSO_BASE
    /* Length, in elements, of the dict */
    ks_size_t len_real;

    /* Length of the 'ents' array, which may included deleted entries */
    ks_size_t len_ents;

    /* Array of entries, ordered by first insertion */
    struct ks_dict_ent {

        /* hash(key) */
        ks_hash_t hash;

        /* The entries key and value (keeps reference), or NULL if this entry has been deleted */
        kso key, val;

    }* ents;


    /* Length of the array of buckets, which is one of 'buckets_*' members */
    ks_size_t len_buckets;

    /* Union of different bucket arrays, discriminated based on size */
    union {

        /* when 'len_buckets - 1 <= KS_SINT8_MAX' */
        ks_sint8_t * buckets_s8;
        /* when 'len_buckets - 1 <= KS_SINT16_MAX' */
        ks_sint16_t* buckets_s16;
        /* when 'len_buckets - 1 <= KS_SINT32_MAX' */
        ks_sint32_t* buckets_s32;
        /* when 'len_buckets - 1 <= KS_SINT64_MAX' */
        ks_sint64_t* buckets_s64;

    };

    /* Maximum size allocated (via 'ks_nextsize()') */
    ks_size_t _max_len_ents, _max_len_buckets_b;
    
}* ks_dict;


/* 'names' - attribute-based namespace
 *
 */
typedef struct ks_names_s {
    KSO_BASE

    /* Attribute dictionary (.__attr__)
     * All names in the namespace
     */
    ks_dict attr;

}* ks_names;


/* Represents a single (directed) edge within a dense graph
 *
 * The index of the 'from' node is implicit in which node the edge is stored in
 * 
 */
struct ks_graph_edge {

    /* Index of the node which this edge points towards
     */
    ks_size_t to;

    /* Value stored on the edge (default: none) 
     * A reference is held to this
     */
    kso val;

};

/* Represents a single node within a dense graph
 * 
 * The index which this node refers to is implicit in its location within the 'nodes' array
 * 
 */
struct ks_graph_node {

    /* Number of outgoing edges */
    ks_size_t n_edges;

    /* Array of edges outwards, sorted by 'to' index 
     *
     * This should always be kept sorted, preferably by insertion sort, or if a batch operation is
     *   given, perhaps adding them all and using timsort/mergesort variant to quickly sort them.
     * For example, you could sort just the edges being added, then merge that with the existing list
     *   to give closer to O(N log(N)) time for N insertions (instead of O(N^2))
     */
    struct ks_graph_edge* edges;

    /* Value stored on the node (default: none) 
     * A reference is held to this
     */
    kso val;

};

/* 'graph' - dense, directed graph representing a list of nodes and lists of connections (edges) between them
 *
 * Internally implemented with sorted adjacency lists per node. So, every node holds a list of connections outwards,
 *   but not inwards.
 * 
 * Memory: O(|V|+|E|)
 */
typedef struct ks_graph_s {
    KSO_BASE

    /* number of nodes within the graph */
    ks_size_t n_nodes;

    /* array of nodes, where the index is their ID */
    struct ks_graph_node* nodes;

}* ks_graph;

/* 'logger' - represents a utility object which can log with different levels to the output stream
 *
 */
typedef struct ks_logger_s {
    KSO_BASE

    /* The name of the logger, which is a full name
     * 'ks'
     * 'my.name'
     * 'other.name.with.more.parts'
     */
    ks_str name;

    /* Current verbosity level, normally one of 'KS_LOGGER_*' */
    ks_cint level;

    /* Stream that the output that passes the level gets redirected to
     * Should normally be a 'os.FileStream' object
     */
    kso output;

}* ks_logger;

/* 'Exception' - object typically thrown up the call stack
 *
 * 
 */
typedef struct ks_Exception_s {
    KSO_BASE

    /* Inner exception, which is the exception being handled while this one was thrown 
     * or, NULL if it was the first exception thrown currently
     */
    struct ks_Exception_s* inner;

    /* The frames when the exception was thrown */
    ks_list frames;

    /* Arguments to the exception's constructor */
    ks_list args;

    /* String describing the error */
    ks_str what;

}* ks_Exception;

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
    ks_cint n_subs;
    ks_type* subs;

    /* Integer sizes and attribute dictionary offsets of instances */
    ks_cint ob_sz, ob_attr;

    /* Number of objects created and deleted */
    ks_cint num_obs_new, num_obs_del;


    /** Special Values (saved as variables here) **/
    
    /* Base type of this type */
    ks_type i__base;

    /* Strings representing the shallow name and full name (including modules, subtypes, etc) */
    ks_str i__name, i__fullname;


    /** Constructors/Destructors **/

    /* In kscript, types are their own constructor; you call them like a function to create a value of that type.
     *
     * For example, 'int("42")'. This is done via the following rules:
     *   * First, an instance is generated via 'tmp = T.__new__(T, *args)'
     *   * If 'T.__init__' is present, then: 'T.__init__(T, *args)' is called as well 
     * 
     */

    /* __new__(tp, *args), __init__(self, *args), __free__(self), __call__(self, *args) */
    kso i__new, i__init, i__free, i__call;

    /* iter(A), next(A), call(A, *args) */
    kso i__iter, i__next;

    /** Special Methods **/

    /* Builtin Conversions (Numeric) */
    kso i__number, i__integral, i__bool, i__int, i__float, i__complex;
    
    /* Builtin Conversions (Misc) */
    kso i__str, i__bytes, i__set, i__dict;

    /* Attributes */
    kso i__getattr, i__setattr, i__delattr;
    
    /* Elements */
    kso i__getelem, i__setelem, i__delelem;

    /* Misc. Properties */
    kso i__hash, i__abs, i__len, i__repr;

    /** Binary Operator Overloads **/

    /* A+B, A-B, A*B, A/B, A//B, A%B, A**B */
    kso i__add, i__sub, i__mul, i__div, i__floordiv, i__mod, i__pow;

    /* A==B, A!=B, A<B, A<=B, A>B, A>=B */
    kso i__eq, i__ne, i__lt, i__le, i__gt, i__ge;
    
    /* A<<B, A>>B, A|B, A^B, A&B */
    kso i__lsh, i__rsh, i__binior, i__binxor, i__binand;


    /** Unary Operator Overloads **/

    /* +A, -A, ~A */
    kso i__pos, i__neg, i__sqig;

};



/* Instantiate macro for all special attributes 
 * Macro should take 'name'
 */
#define _KS_DO_SPEC(m) \
    m(__base) \
    m(__name) \
    m(__fullname) \
    m(__new) \
    m(__init) \
    m(__free) \
    m(__call) \
    m(__iter) \
    m(__next) \
    m(__number) \
    m(__integral) \
    m(__bool) \
    m(__int) \
    m(__float) \
    m(__complex) \
    m(__str) \
    m(__bytes) \
    m(__set) \
    m(__dict) \
    m(__getattr) \
    m(__setattr) \
    m(__delattr) \
    m(__getelem) \
    m(__setelem) \
    m(__delelem) \
    m(__hash) \
    m(__abs) \
    m(__len) \
    m(__repr) \
    m(__add) \
    m(__sub) \
    m(__mul) \
    m(__div) \
    m(__floordiv) \
    m(__mod) \
    m(__pow) \
    m(__eq) \
    m(__ne) \
    m(__lt) \
    m(__le) \
    m(__gt) \
    m(__ge) \
    m(__lsh) \
    m(__rsh) \
    m(__binior) \
    m(__binxor) \
    m(__binand) \
    m(__pos) \
    m(__neg) \
    m(__sqig) \



/* 'module' - represents an imported or builtin module
 */
typedef struct ks_module_s {
    KSO_BASE

    ks_dict attr;

}* ks_module;


/* C-style function wrapper */
typedef kso (*ks_cfunc)(int _nargs, kso* _args);

/* 'func' - callable function type
 *
 */
typedef struct ks_func_s {
    KSO_BASE

    ks_dict attr;

    /* If true, then a C-style function */
    bool is_cfunc;

    union {
        /* if it is a C-function */
        ks_cfunc cfunc;

        /* if it is a bytecode-function */
        struct {

            /* Bytecode object attached */
            //ks_bytecode bc;

            /* Number of formal parameters to the function */
            int n_pars;

            /* Number of required (minimum) */
            int n_req;

            /* Index of vararg */
            int vararg_idx;

            /* List of parameters the function has */
            struct ks_func_par {
                
                /* String name of the function */
                ks_str name;

                /* Default value (or 'NULL' if there was none) */
                kso defa;

            }* pars;

            /* Frame it is located in (os.frame) */
            kso closure;

        } bfunc;
    };

}* ks_func;


/* 'func.partial' - a partial application of a function, which has some arguments auto filled
 */
typedef struct ks_partial_s {
    KSO_BASE

    /* The function being filled in */
    kso of;

    /* Number of arguments filled in */
    int n_args;

    /* Array of filled in arguments, sorted by index */
    struct {

        /* Index being filled in */
        int idx;

        /* Value being filled in */
        kso val;

    }* args;

}* ks_partial;

/* 'map' - represents an async mapping of a function applied to an iterable
 */
typedef struct ks_map_s {
    KSO_BASE

    /* function being applied to each iterable */
    kso trans;

    /* Iterator */
    kso it;

}* ks_map;

/* 'filter' - represents an async mapping where objects that pass the filter are yielded
 *
 */
typedef struct ks_filter_s {
    KSO_BASE

    /* transformation on which the iterator is filtered */
    kso trans;

    /* Iterator */
    kso it;

}* ks_filter;

/* 'enumerate' - represents a mapping 
 *
 */
typedef struct ks_enumerate_s {
    KSO_BASE

    /* transformation on which the iterator is filtered */
    kso trans;

    /* Iterator */
    kso it;

    /* Current value to yield along the the iterable */
    ks_int counter;

}* ks_enumerate;


#endif /* KS_TYPES_H__ */
