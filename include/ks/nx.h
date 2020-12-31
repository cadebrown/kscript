/* ks/nx.h - header for the nx (NumeriX) library in kscript
 * 
 * This module is a numerical computation library, specifically meant for vectorized inputs and tensor
 *   math. It deals (typically) with a tensor (C-type: 'nxar_t') that has a given data type ('nx_dtype', 
 *   which may be a C-style type, or 'object' for generic operations), as well as dimensions and strides.
 * 
 * Scalars can be represented with a rank of 0, which still holds 1 element
 *
 * Operations are applied by vectorization -- which is to say per each element independently, with results not affecting
 *   each other. So, adding vectors adds them elementwise, not appending them to the end. These are called scalar-vectorized
 *   operations. Some operations take more than single elements (they take a 1D sub-vector, called a 'pencil' or 'vector'). These
 *   will work per-pencil/per-vector, and can still be parallelized per pencil/vectory (for example, a 2D tensor would have 1D of parallelization,
 *   but 1D values being processed together). There can also be functions for any dimensional slice of data, but these are rarer.
 * 
 * scalar-vectorized:
 *   * add, sub, mul (arithmetics)
 * 
 * 1D-vectorized:
 *   * sort
 *
 * 2D-vectorized:
 *   * matrix multiplication
 * 
 * ND-vectorized:
 *   * generalized FFTs, NTTs, etc.
 * 
 * 
 * There also exist operations which have different input and output spaces. All of the above functions have the same input
 *   dimension as output dimension. Take, for example, reductions. These will (typically) reduce the number of dimensions.
 * 
 * 1D -> scalar operations:
 *   * min, max, fold
 * 
 * 2D -> scalar operations:
 *   * matrix norm
 * 
 * ND -> scalar operations:
 *   * sum
 * 
 * ND -> (N-1)D:
 *   * 1D -> scalar operations applied on all-but-one axis (i.e. they are (N-1)D vectorized, but applied over an axis)
 * 
 * These transforms can sometimes be applied multiple times, or for different axes. For example, 'sum' operation can take 1 or more
 *   axis and is equivalent to reducing on each axis. You can sum over all axes to have a single scalar element, or all but one
 *   to vectorize (N-1)D summations.
 * 
 * Unlike other standard modules, the prefixes for datatypes/functions/variables do not have 'ks' prefixing them,
 *   only 'nx'.
 * 
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSNX_H__
#define KSNX_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif

/* fftw3 (--with-fftw3) 
 *
 * Adds support for FFTW3, which is a very efficient FFT library. If it's not present,
 *   then FFTs may be slower.
 * 
 * Based on my benchmark(s) (on a previous version of kscript), the power-of-2 transforms are
 *   actually pretty close with my own code and FFTW3 (FFTW3 beats me by ~2x), but other transforms
 *   are more like 10x faster with FFTW3, so it's definitely recomended
 *
 */
#ifdef KS_HAVE_fftw3
 #include <fftw3.h>
#endif



/** Types **/

enum nx_dtype_kind {
    NX_DTYPE_KIND_NONE      = 0,

    /* C-style integer */
    NX_DTYPE_KIND_CINT      = 1,

    /* IEEE Formatted Float */
    NX_DTYPE_KIND_CFLOAT    = 2,

    /* Complex IEEE number */
    NX_DTYPE_KIND_CCOMPLEX  = 3,

    /* Structure of other datatypes */
    NX_DTYPE_KIND_CSTRUCT   = 4,

    /* TODO: Add structures */

};


/* 'nx.dtype' - type representing a data format
 *
 */
typedef struct nx_dtype_s* nx_dtype;

/* Describes a struct member in a datatype
 */
struct nx_dtype_struct_member {

    /* offset, in bytes, from the start of the structure */
    int offset;

    /* data type of the member */
    nx_dtype dtype;

    /* name of the member */
    ks_str name;

};


struct nx_dtype_s {
    KSO_BASE

    /* name of the type */
    ks_str name;

    /* size in bytes of the data type */
    int size;


    /* the kind of data type */
    enum nx_dtype_kind kind;

    /* specific kind information */
    union {

        /* when kind==NX_DTYPE_KIND_CINT */
        struct {
            
            /* whether the type is signed */
            bool sgn;

            /* number of bits in the size, must be divisible by 8 */
            int bits;

        } s_cint;

        /* when kind==NX_DTYPE_KIND_CFLOAT */
        struct {

            /* number of bits, must be divisible by 8
             * TODO: should this be generalized for support of any IEEE-like
             *   floating point?
             */
            int nbits;

        } s_cfloat;

        /* when kind==NX_DTYPE_KIND_CCOMPLEX */
        struct {

            int nbits;

        } s_ccomplex;

        /* when kind==NX_DTYPE_KIND_CSTRUCT */
        struct {

            /* number of members */
            int n_members;

            /* array of members */
            struct nx_dtype_struct_member* members;
            
        } s_cstruct;

    };
};

/* 'nxar_t' - C array descriptor, generic representation of any tensor (nx.array, nx.view,
 *              existing C pointers, etc)
 *
 * Functions should accept these, that way the data can come from anywhere (either kscript objects,
 *   or C objects)
 *
 */
typedef struct {
    
    /* Pointer to the start of the data 
     * May only be NULL if the array was empty
     */
    void* data;

    /* The format of the 'data'. This also contains the size of each
     *   element
     */
    nx_dtype dtype;
    
    /* Number of dimensions of the array */
    int rank;
    
    /* Array of dimensions/sizes
     *
     * The units are elements
     * 
     * So, the total number of elements is the product of these
     *   numbers
     * 
     */
    ks_size_t* dims;
    
    /* Array of strides (used for index calculation)
     *
     * The units are BYTES not elements
     * 
     */
    ks_ssize_t* strides;

    /* Object the data comes from, or NULL
     * 
     */
    kso obj;

} nxar_t;

/* 'nx.array' - N-dimensional tensor/array type
 *
 * This is a dense array of a given data type, which is allocated for this object
 * 
 */
typedef struct nx_array_s {
    KSO_BASE

    /* Array descriptor, which owns the 'data' pointer, and 'obj' should be NULL */
    nxar_t ar;

}* nx_array;


/* Create 'nxar_t' from C-style initializers
 */
#define NXAR_(_data, _dtype, _rank, _dims, _strides, __obj) ((nxar_t){ \
    .data = (_data), \
    .dtype = (_dtype), \
    .rank = (_rank), \
    .dims = (_dims), \
    .strides = (_strides), \
    .obj = (kso)(__obj), \
})

/* Return whether the array descriptor is a scalar (0D), vector (1D), or matrix (2D) */
#define NXAR_ISSCALAR(_ar) ((_ar).rank == 0)
#define NXAR_ISVECTOR(_ar) ((_ar).rank == 1)
#define NXAR_ISMATRIX(_ar) ((_ar).rank == 2)


/* 'nx.view' - Tensor view of a data source
 *
 * This just points to data -- nothing is allocated
 * 
 * Additionally, 'strides' may be 0, negative, or non-multiples of the element
 *   size
 *
 */
typedef struct nx_view_s {
    KSO_BASE

    /* Array descriptor, which does NOT own the 'data' pointer, and 'obj' contains the object
     *   that we should free once it is over
     */
    nxar_t ar;

}* nx_view;

/* Calculate whether a dtype is C-arithmetic-able */
#define NX_DTYPE_ISCARITH(_dt) ((_dt)->kind == NX_DTYPE_KIND_CINT || (_dt)->kind == NX_DTYPE_KIND_CFLOAT)
#define NX_DTYPE_ISARITH(_dt) (NX_DTYPE_ISCARITH(_dt) || (_dt)->kind == NX_DTYPE_KIND_CCOMPLEX)


/* C function type for an element-wise function application
 *
 * This function will be called with the number of inputs (N), and an array of that many array descriptors
 *   (inp), which are all 1-dimensional (i.e. rank==1), and an auxiliary data pointer which may be whatever
 *   the caller gives to the function
 *
 * This function should calculate, within the 'inp' array, the result of the computation. For example, the 'add'
 *   kernel would take 'A, B, R' inputs, and compute 'R[:] = A[:] + B[:]', elementwise
 * 
 * Flow:
 *   scalar, ... -> scalar, ...
 * 
 */
typedef int (*nx_elem_cf)(int N, nxar_t* inp, int len, void* _data);

/* C function type for a vector-wise function application
 *
 * While given the same arguments as 'nx_elem_cf', this type of function is used to indicate that the operation
 *   is on the entire vector instead of applied to each element. 
 *
 * For example, if given the 'qsort' kernel, it should take arguments 'A, R', and compute 'R[:] = qsort(A[:])'
 * 
 * Flow:
 *   [N,], ... -> [M,], ...
 * 
 */
typedef int (*nx_vec_cf)(int N, nxar_t* inp, int len, void* _data);


/** Macro/Utils **/


/* Get-Element-Pointer (GEP) of an element of an array (located at '_data'),
 *   with a stride (of bytes) of '_stride', at index '_idx' (in elements)
 */
#define NX_gep(_data, _stride, _idx) ((void*)((ks_uint)(_data) + (_stride) * (_idx)))

/* Get-Element (GE) of an array (located in '_data', as C-type '_tp'), with the given stride
 *   (in bytes), and index (in elements)
 */
#define NX_ge(_tp, _data, _stride, _idx) (*(_tp*)NX_gep((_data), (_stride), (_idx)))

/* Get-Element of 1D-array
 *
 * NOTE: This macro is not well defined for other dimensional arrays -- only call
 *   with 1D arrays
 * 
 */
#define NXAR_ge1(_tp, _nxar, _idx) NX_ge(_tp, (_nxar).data, (_nxar).strides[0], (_idx))

/* Get-Element-Pointer of N-dimensional array
 *
 * NOTE: '_idxes' must be the same length as the rank of tensor, and be of type:
 *   'ks_uint*' (or 'ks_size_t*')
 * 
 * 
 */
#define NXAR_gepN(_tp, _to, _nxar, _idxes) do { \
    ks_uint _gNr = (ks_uint)(_nxar).data; \
    ks_uint* _gNidxes = _idxes; \
    int _gNi; \
    for (_gNi = 0; _gNi < (_nxar).rank; ++_gNi) { \
        _gNr += (_nxar).strides[_idx] * _gNidxes[i]; \
    } \
    _to = (_tp*)_gNr; \
} while (0)


/** Functions **/

/*** Configuration/Operation Helpers ***/


/* Calculate a broadcast size, returning an array of the dimsnions and setting '*orank' to the
 *   the rank (and thus size) of the result array.
 * 
 * The result is allocated with 'ks_malloc()', and should be 'ks_free()''d, or if it was NULL,
 *   an error was thrown explaining the problem
 */
KS_API ks_size_t* nx_calc_bcast(int N, nxar_t* inp, int* orank);

/* Return the datatype that should be used as the result of a numeric calculations between 2 types
 *
 * NOTE: This is only valid when they are expected to be the same type. For division and floor division,
 *   custom care should be taken
 */
KS_API nx_dtype nx_calc_numcast(nx_dtype da, nx_dtype db);

/** Utils **/

/* Adds the string representation to an IO object
 */
KS_API bool nxar_tostr(ksio_BaseIO bio, nxar_t A);


/** nx.dtype **/

/* Return a reference to a datatype */
KS_API nx_dtype nx_dtype_get_cint(int nbits, bool is_signed);
KS_API nx_dtype nx_dtype_get_cfloat(int nbits);
KS_API nx_dtype nx_dtype_get_ccomplex(int nbits);

/* Encode object into datatype, returning status
 *
 * Ensure that the 'out' holds 'dtype->size' bytes
 */
KS_API bool nx_dtype_enc(nx_dtype dtype, kso obj, void* out);

/* Decode low-level value into kscript object
 *
 * Ensure that the 'out' holds 'dtype->size' bytes, and is valid
 */
KS_API bool nx_dtype_dec(nx_dtype dtype, void* obj, kso* out);




/** nx.array **/

/* Create a new, dense array from the data in 'data', or initialized to the dtype's 'default' value if 'data==NULL'
 * The new array will have the same dims and rank, but it will be tightly packed, so the strides may be different if 'data' was not
 *   tightly packed
 */
KS_API nx_array nx_array_newc(ks_type tp, nx_dtype dtype, int rank, ks_size_t* dims, ks_ssize_t* strides, void* data);

/* Create a new array from an object, with a given datatype (may be NULL for a default)
 *
 * If 'obj' is not iterable, it will be converted to a [,] scalar. Otherwise,
 *   it is searched recursively, and it is assumed to have equal dimensions,
 *   and then each non-iterable object is converted to the corresponding element
 *   in the resultant array
 */
KS_API nx_array nx_array_newo(ks_type tp, kso obj, nx_dtype dtype);


/* Return a pointer to a specific element */
static void* nx_get_ptr(void* data, int N, ks_size_t* dims, ks_ssize_t* strides, ks_ssize_t* idxs) {
    ks_uint res = (ks_uint)data;
    int i;
    for (i = 0; i < N; ++i) {
        res += idxs[i] * strides[i];
    }

    return (void*)res;
}



/** Submodule: 'nx.rand' **/





/* nx.rand.State - Random number generator
 *
 * Based on the mersenne twister algorithm
 * 
 */
typedef struct nxrand_State_s {
    KSO_BASE

/* Length of state vector (in units) */
#define NXRAND_MT_N 624
/* Period parameter */
#define NXRAND_MT_M 397
/* Magic constant */
#define NXRAND_MT_K 0x9908B0DFULL

    /* State of generated words */
    ks_uint32_t state[NXRAND_MT_N + 1];

    /* Position within 'state' */
    int pos;

}* nxrand_State;

/* Create a new random number generator state */
KS_API nxrand_State nxrand_State_new(ks_uint seed);

/* Fill 'A' with random numbers (for integers, random of their range, and for floats, random between 0.0 and 1.0) */
KS_API bool nxrand_get_a(nxrand_State self, nxar_t A);

/* Generate 'nout' uniformly distributed bytes */
KS_API bool nxrand_get_b(nxrand_State self, int nout, unsigned char* out);

/* Generate 'nout' uniformly distributed 'ks_uint's (i.e. from 0 to 'KS_UINT_MAX') and store in 'out' */
KS_API bool nxrand_get_i(nxrand_State self, int nout, ks_uint* out);

/* Generate 'nout' uniformly distribute 'ks_cfloat's between 0.0 (inclusive) and 1.0 (exclusive) */
KS_API bool nxrand_get_f(nxrand_State self, int nout, ks_cfloat* out);





/*  */

/* Types */
KS_API extern ks_type
    nxt_dtype,
    nxt_array,
    nxt_view,

    nxrandt_State

;

KS_API extern nx_dtype
    nx_uint8,
    nx_sint8,
    nx_uint16,
    nx_sint16,
    nx_uint32,
    nx_sint32,
    nx_uint64,
    nx_sint64,

    nx_float32,
    nx_float64,
    /*
    nx_float80,
    nx_float128,
    */
    
    nx_complex32,
    nx_complex64
;


/* Aliases */
KS_API extern nx_dtype
    nxd_float,
    nxd_double,

    nxd_complexfloat,
    nxd_complexdouble
;


#endif /* KSNX_H__ */
