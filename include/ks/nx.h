/* ks/nx.h - header for the nx (NumeriX) library in kscript
 * 
 * This module is a numerical computation library, specifically meant for vectorized inputs and tensor
 *   math. It deals (typically) with a tensor (C-type: 'nx_t') that has a given data type ('nx_dtype', 
 *   which may be a C-style type, or 'object' for generic operations), as well as dimensions and strides.
 * 
 * Submodules:
 * 
 *   nx.rand: Random number generation
 *   nx.la: Linear algebra routines
 *   nx.fft: Fast Fourier Transform (FFT) routines
 *   nx.cv: Computer vision routines (includes image processing)
 *   nx.ch: Computer hearing routines (includes audio processing)
 * 
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


/* Constants */

/* Maximum rank of a tensor, used so statically allocating tensor sizes/strides is possible, reducing
 *   overhead
 */
#define NX_MAXRANK 16

/* Maximum broadcast size (i.e. maximum number of arguments to a single kernel)
 *
 */
#define NX_MAXBCS 16


/* Sized-integers */
typedef   bool     nx_bl;
typedef   int8_t   nx_s8;
typedef  uint8_t   nx_u8;
typedef   int16_t  nx_s16;
typedef  uint16_t  nx_u16;
typedef   int32_t  nx_s32;
typedef  uint32_t  nx_u32;
typedef   int64_t  nx_s64;
typedef  uint64_t  nx_u64;

#define nx_blv(_val) ((_val) ? 1 : 0)

#define nx_blMIN  (0)
#define nx_s8MIN  (-128)
#define nx_u8MIN  (0)
#define nx_s16MIN (-32768)
#define nx_u16MIN (0)
#define nx_s32MIN (-2147483648LL)
#define nx_u32MIN (0ULL)
#define nx_s64MIN (-9223372036854775807LL-1)
#define nx_u64MIN (0ULL)

#define nx_blMAX  (1)
#define nx_s8MAX  (127)
#define nx_u8MAX  (255)
#define nx_s16MAX (32767)
#define nx_u16MAX (65536)
#define nx_s32MAX (2147483647LL)
#define nx_u32MAX (4294967295ULL)
#define nx_s64MAX (9223372036854775807LL)
#define nx_u64MAX (1844674407370955161ULL)


/* 'half' (H) type - half precision real
 *
 */
#if defined(KS_HAVE__Float16)
  typedef _Float16 nx_H;
#elif defined(KS_HAVE___float16)
  typedef __float16 nx_H;
#elif defined(KS_HAVE___fp16)
  typedef __fp16 nx_H;
#else
  typedef float nx_H;
#endif

/* 'float' (F) type - wrapper around C 'float'
 *
 */
typedef float       nx_F;

/* 'double' (D) type - wrapper around C 'double'
 *
 */
typedef double      nx_D;

/* 'long double' (L) type - wrapper around C 'long double'
 *
 */
typedef long double nx_L;

/* 'fp128' (E) type - 128 bit floating point value
 *
 * NOTE: This is not the 'long double' type in C
 */
#if defined(KS_HAVE__Float128)
  typedef _Float128 nx_E;
  #define nx_Eval(_val) _val##q
#elif defined(KS_HAVE___float128)
  typedef __float128 nx_E;
  #define nx_Eval(_val) _val##q
#else
  typedef long double nx_E;
  #define nx_Eval(_val) _val
#endif

#define nx_HINF INFINITY
#define nx_FINF INFINITY
#define nx_DINF INFINITY
#define nx_LINF INFINITY
#define nx_EINF INFINITY

#define nx_HNAN KS_CFLOAT_NAN
#define nx_FNAN KS_CFLOAT_NAN
#define nx_DNAN KS_CFLOAT_NAN
#define nx_LNAN KS_CFLOAT_NAN
#define nx_ENAN KS_CFLOAT_NAN

#define nx_HEPS FLT_EPSILON
#define nx_FEPS FLT_EPSILON
#define nx_DEPS DBL_EPSILON
#define nx_LEPS LDBL_EPSILON
#ifdef FLT128_EPSILON
  #define nx_EEPS FLT128_EPSILON
#else
  #define nx_EEPS nx_Eval(1.92592994438723585305597794258492732e-34)
#endif

#define nx_HMIN FLT_MIN
#define nx_FMIN FLT_MIN
#define nx_DMIN DBL_MIN
#define nx_LMIN LDBL_MIN
#ifdef FLT128_MIN
  #define nx_EMIN FLT128_MIN
#else
  #define nx_EMIN nx_Eval(3.36210314311209350626267781732175260e-4932)
#endif

#define nx_HMAX FLT_MAX
#define nx_FMAX FLT_MAX
#define nx_DMAX DBL_MAX
#define nx_LMAX LDBL_MAX
#ifdef FLT128_MAX
  #define nx_EMAX FLT128_MAX
#else
  #define nx_EMAX nx_Eval(1.18973149535723176508575932662800702e4932)
#endif


/* 'complex half' (cH) - complex half precision
 *
 */
typedef struct {
    nx_H re, im;
} nx_cH;

/* 'complex float' (cF) - complex single precision
 *
 */
typedef struct {
    nx_F re, im;
} nx_cF;

/* 'complex double' (cD) - complex double precision
 *
 */
typedef struct {
    nx_D re, im;
} nx_cD;

/* 'complex long double' (cD) - complex double precision
 *
 */
typedef struct {
    nx_L re, im;
} nx_cL;

/* 'complex fp128' (cE) - complex 128 bit floating precision
 *
 */
typedef struct {
    nx_E re, im;
} nx_cE;


/* Aliases to realtype */
#define nx_cHr nx_E
#define nx_cFr nx_F
#define nx_cDr nx_D
#define nx_cLr nx_L
#define nx_cEr nx_E

#define nx_cHrINF nx_HINF
#define nx_cFrINF nx_FINF
#define nx_cDrINF nx_DINF
#define nx_cLrINF nx_LINF
#define nx_cErINF nx_EINF

#define nx_cHrNAN nx_HNAN 
#define nx_cFrNAN nx_FNAN 
#define nx_cDrNAN nx_DNAN 
#define nx_cLrNAN nx_LNAN 
#define nx_cErNAN nx_ENAN 

#define nx_cHrMIN nx_HMIN
#define nx_cFrMIN nx_FMIN
#define nx_cDrMIN nx_DMIN
#define nx_cLrMIN nx_LMIN
#define nx_cErMIN nx_EMIN

#define nx_cHrMAX nx_HMAX
#define nx_cFrMAX nx_FMAX
#define nx_cDrMAX nx_DMAX
#define nx_cLrMAX nx_LMAX
#define nx_cErMAX nx_EMAX

#define nx_cHrEPS nx_HEPS
#define nx_cFrEPS nx_FEPS
#define nx_cDrEPS nx_DEPS
#define nx_cLrEPS nx_LEPS
#define nx_cErEPS nx_EEPS



/** Types **/

enum nx_dtype_kind {
    NX_DTYPE_NONE      = 0,

    /* C-style integer type */
    NX_DTYPE_INT      = 1,

    /* C-style floating point type */
    NX_DTYPE_FLOAT    = 2,

    /* C-style complex floating point type, with two components
     *
     * Should act like a structure defined as:
     * 
     * struct ComplexType {
     *   RealType re, im;
     * };
     * 
     */
    NX_DTYPE_COMPLEX  = 3,

    /* C-style structure, with arbitrary number of members
     *
     */
    NX_DTYPE_STRUCT   = 4,

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

    /* Name of the type */
    ks_str name;

    /* Special code for the name, which is either the name, or a shortened version (i.e. 'F' for float) */
    ks_str namecode;

    /* Size (in bytes) of the data type */
    int size;

    /* the kind of data type */
    enum nx_dtype_kind kind;

    /* specific kind information */
    union {

        /* when kind==NX_DTYPE_KIND_CSTRUCT */
        struct {

            /* number of members */
            int n_members;

            /* array of members */
            struct nx_dtype_struct_member* members;
            
        } s_cstruct;

    };
};

/* 'nx_t' - Array descriptor object, which is a generic representation of a tensor
 * 
 */
typedef struct {

    /* Pointer to the start of the data 
     *
     * May be NULL if the array was empty, or returned as a result of a size
     *   calculation (i.e. data does not matter)
     */
    void* data;

    /* The format of the 'data'. This also contains the size of each
     *   element
     */
    nx_dtype dtype;

    /* Number of dimensions of the array (i.e. the rank) */
    int rank;

    /* Shape of the tensor, which is the length in each dimension (in elements)
     *
     * Only the first 'rank' members are initialized
     * 
     * The total number of elements is the product of the first 'rank' numbers
     */
    ks_size_t shape[NX_MAXRANK];

    /* Strides of the tensor, which is the distance between elements in each dimension (in bytes)
     *
     * Only the first 'rank' members are initialized
     * 
     * For dense arrays (i.e. all contained in this descriptor), set up is:
     *   strides[rank - 1] = dtype->size
     *   ...
     *   strides[i] = strides[i + 1] * shape[i + 1]
     * 
     * (i.e. row-major)
     */
    ks_ssize_t strides[NX_MAXRANK];

} nx_t;

/* 'nx.array' - Dense multi-dimensional array object
 * 
 */
typedef struct nx_array_s {
    KSO_BASE

    /* Array descriptor
     *
     * This data pointer is owned, so it is freed when the array is freed
     * 
     * A reference is held to 'val->dtype'
     */
    nx_t val;

}* nx_array;

/* 'nx.view' - View of a multi-dimensional array
 *
 * This just points to data -- nothing is allocated, but a reference is held to an
 *   object to which it points
 * 
 * Additionally, 'val.strides' may be 0, negative, or non-multiples of the element
 *   size
 *
 */
typedef struct nx_view_s {
    KSO_BASE

    /* Array descriptor
     *
     * A reference is held to 'val->dtype'
     */
    nx_t val;

    /* Opaque reference held
     *
     * The data in 'val' requires this object to be alive, so a reference is held
     *   while the view is active
     */
    kso ref;

}* nx_view;


/* Function signature for broadcasting/function application to inputs
 *
 * This function will be called with the number of inputs ('N'), and the array descriptors for 
 *   each input ('args'), and an opaque pointer that is passed ('extra'), which may hold any data, or
 *   be NULL. 'len' is given as the length  
 * 
 * This function should calculate, within the 'args' array, the result of the computation. For example,
 *   the 'add' kernel would take (A, B, R) as 'args', and compute 'R[i] = A[i] + B[i]' for each input
 *   index
 */
typedef int (*nxf_elem)(int N, nx_t* args, int len, void* extra);

/* Function signature for broadcasting/function application of any degrees
 */
typedef int (*nxf_Nd)(int N, nx_t* args, void* extra);



/* Utility macros */

/* Tests wheter '_dtype' is a builtin numeric type (i.e. is 'int', 'float', or 'complex' kind) */
#define NX_ISNUM(_dtype) ((_dtype)->kind == NX_DTYPE_INT || (_dtype)->kind == NX_DTYPE_FLOAT || (_dtype)->kind == NX_DTYPE_COMPLEX)


/* Functions */

/* Create an array descriptor (does not create a reference to 'dtype')
 */
KS_API nx_t nx_make(void* data, nx_dtype dtype, int rank, ks_ssize_t* shape, ks_ssize_t* strides);

/* Create an array descriptor with a new axis inserted
 */
KS_API nx_t nx_with_newaxis(nx_t from, int axis);

/* Calculate a broadcast shape, returning a shape-only array descriptor
 *
 * If result.rank < 0, an error is thrown and you should either catch it
 *   or signal it. Otherwise, the result contains 'rank' and 'dims' initialized to the size of the
 *   broadcast result. All other fields are uninitialized
 */
KS_API nx_t nx_make_bcast(int N, nx_t* args);

/* Calculate the result of a numeric operation on two types, 'X' and 'Y'
 *
 * A new reference is not returned, so don't dereference the result! Only works
 *   for numeric types
 */
KS_API nx_dtype nx_cast2(nx_dtype X, nx_dtype Y);

/* Calculate a shape, returning a shape-only array descriptor
 *
 * If 'obj' is 'none' the resulting shape is a scalar
 * If 'obj' is an integer, the resulting shape is a 1D array
 * If 'obj' is an iterable, the resulting shape is 'obj'
 *
 * If result.rank < 0, an error is thrown and you should either catch it
 *   or signal it. Otherwise, the result contains 'rank' and 'dims' initialized to the size of the
 *   broadcast result. All other fields are uninitialized
 */
KS_API nx_t nx_getshape(kso obj);

/* Computes 'data + strides[:] * idxs[:]'
 *
 */
KS_API void* nx_szdot(void* data, int rank, ks_ssize_t* strides, ks_size_t* idxs);

/* Get the size of the product of the shape
 */
KS_API ks_size_t nx_szprod(int rank, ks_size_t* shape);

/* Cast 'X' to a given data type ('dtype') (keeping as-is if it can), and store in '*R'
 * 
 * '*tofree' is set to a pointer that should be passed to 'ks_free()' after you are done
 *   with '*R'. It may be set to NULL if no extra data allocation was needed
 */
KS_API bool nx_getcast(nx_t X, nx_dtype dtype, nx_t* R, void** tofree);

/* Convert an object into an array descriptor of a given datatype (or NULL to calculate
 *   a default)
 *
 * Sets '*res' to the array, and '*ref' to a reference that must be held and destroyed
 *   while using the array descriptor (or NULL for none needed)
 * 
 * Return success
 */
KS_API bool nx_get(kso obj, nx_dtype dtype, nx_t* res, kso* ref);

/* Adds the string representation of 'X' to an IO-like object
 *
 */
KS_API bool nx_getstr(ksio_BaseIO bio, nx_t X);

/* Encode object into memory
 *
 * Ensure that the 'out' holds 'dtype->size' bytes
 */
KS_API bool nx_enc(nx_dtype dtype, kso obj, void* data);

/* Apply an element-wise function application on the arguments
 *
 * Returns either 0 if all executions returned 0, or the first non-zero code
 *   encountered by calling 'func()'
 */
KS_API int nx_apply_elem(nxf_elem func, int N, nx_t* args, void* extra);

/* Apply an Nd-wise function to the arguments, where 'M' is the number of dimensions
 *
 * Returns either 0 if all executions returned 0, or the first non-zero code
 *   encountered by calling 'func()'
 */
KS_API int nx_apply_Nd(nxf_Nd func, int N, nx_t* args, int M, void* extra);





/** Creation Routines **/

/* Create a new dense array from 'data', or initialized to zeros if 'data == NULL'. The new array
 *   will have the same rank and shape, but it will be tightly packed and so the strides may be different if
 *   'data' was not tightly packed
 *
 */
KS_API nx_array nx_array_newc(ks_type tp, void* data, nx_dtype dtype, int rank, ks_size_t* shape, ks_ssize_t* strides);

/* Create a new dense array from an object, with a given datatype (may be NULL for a default)
 *
 * If 'obj' is iterable, it will be converted recursively and dimensions will
 *   be checked, and each non-iterable object is converted to a scalar value
 * Otherwise, if 'obj' is not iterable, it will be converted to a scalar of rank 0
 *
 */
KS_API nx_array nx_array_newo(ks_type tp, kso objh, nx_dtype dtype);


/** Operations **/

/* Fill R with zeros
 */
KS_API bool nx_zero(nx_t R);


/* Fill R[X[i]] = 1
 */
KS_API bool nx_onehot(nx_t X, nx_t R);


/* Compute 'R = X', but type casted to 'R's type. Both must be numeric */
KS_API bool nx_cast(nx_t X, nx_t R);

/* Compute 'R = X', but with floating/fixed point conversions automatic
 *
 * For examples, floats are normally in the range [-1, 1], and integers have
 *   their own ranges. 
 * 
 * Unsigned integers are converted into their appropriate float range in [0, 1], 
 *   and signed integers are converted to floats between [-1, 1]. The reverse is also true
 */
KS_API bool nx_fpcast(nx_t X, nx_t R);

/*** Math Operations ***/

/* R = -X */
KS_API bool nx_neg(nx_t X, nx_t R);

/* R = abs(X) 
 * This kernel is special: If you give it a complex -> real abs, you must provide
 *   the arguments in the correct type
 */
KS_API bool nx_abs(nx_t X, nx_t R);

/* R = ~X  (conjugation) */
KS_API bool nx_conj(nx_t X, nx_t R);

/* R = fmin(X, Y) */
KS_API bool nx_fmin(nx_t X, nx_t Y, nx_t R);
/* R = fmax(X, Y) */
KS_API bool nx_fmax(nx_t X, nx_t Y, nx_t R);


/* R = clip(X, min=Y, max=Z) */
KS_API bool nx_clip(nx_t X, nx_t Y, nx_t Z, nx_t R);

/* R = X + Y */
KS_API bool nx_add(nx_t X, nx_t Y, nx_t R);

/* R = X - Y */
KS_API bool nx_sub(nx_t X, nx_t Y, nx_t R);

/* R = X * Y */
KS_API bool nx_mul(nx_t X, nx_t Y, nx_t R);

/* R = X / Y */
KS_API bool nx_div(nx_t X, nx_t Y, nx_t R);

/* R = X // Y */
KS_API bool nx_floordiv(nx_t X, nx_t Y, nx_t R);

/* R = X % Y */
KS_API bool nx_mod(nx_t X, nx_t Y, nx_t R);

/* R = exp(X) */
KS_API bool nx_exp(nx_t X, nx_t R);

/* R = log(X) */
KS_API bool nx_log(nx_t X, nx_t R);

/* R = sqrt(X) */
KS_API bool nx_sqrt(nx_t X, nx_t R);

/* R = cbrt(X) */
KS_API bool nx_cbrt(nx_t X, nx_t R);

/* R = pow(X, Y) */
KS_API bool nx_pow(nx_t X, nx_t Y, nx_t R);

/* R = hypot(X, Y) */
KS_API bool nx_hypot(nx_t X, nx_t Y, nx_t R);

/* R = sin(X) */
KS_API bool nx_sin(nx_t X, nx_t R);

/* R = cos(X) */
KS_API bool nx_cos(nx_t X, nx_t R);

/* R = tan(X) */
KS_API bool nx_tan(nx_t X, nx_t R);

/* R = asin(X) */
KS_API bool nx_asin(nx_t X, nx_t R);

/* R = acos(X) */
KS_API bool nx_acos(nx_t X, nx_t R);

/* R = atan(X) */
KS_API bool nx_atan(nx_t X, nx_t R);

/* R = sinh(X) */
KS_API bool nx_sinh(nx_t X, nx_t R);

/* R = cosh(X) */
KS_API bool nx_cosh(nx_t X, nx_t R);

/* R = tanh(X) */
KS_API bool nx_tanh(nx_t X, nx_t R);

/* R = asinh(X) */
KS_API bool nx_asinh(nx_t X, nx_t R);

/* R = acosh(X) */
KS_API bool nx_acosh(nx_t X, nx_t R);

/* R = atanh(X) */
KS_API bool nx_atanh(nx_t X, nx_t R);



/** Submodule: 'nx.rand' **/

/* Number of elements in the state */
#define NXRAND_N 1024

/* nx.rand.State - Random number generator
 *
 * Based on a variety of algorithms, this is essentially a buffered generator which generates
 *   'NXRAND_N' random numbers every time it needs to be filled up, and stores in 'data'
 * 
 * Algorithms used:
 *   * Xorshift
 * 
 */
typedef struct nxrand_State_s {
    KSO_BASE

    /* Position within 'data' (once it hits 'NXRAND_MT_N', it needs to be refilled) */
    int pos;

    /* Current data */
    ks_uint data[NXRAND_N];


    /* Last word (used in xorshift) */
    ks_uint lw;

}* nxrand_State;

/* Create a new random number generator state */
KS_API nxrand_State nxrand_State_new(ks_uint seed);

/* Re-start a state and re-seed it */
KS_API void nxrand_State_seed(nxrand_State self, ks_uint seed);

/* Generate 'nout' uniformly distributed bytes */
KS_API bool nxrand_randb(nxrand_State self, int nout, unsigned char* out);


/** Random Number Generation **/

/* Fills 'R' with random, uniform floats in [0, 1)
 */
KS_API bool nxrand_randf(nxrand_State self, nx_t R);

/* Fills 'R' with values in a normal (Guassian) distribution
 *
 *   u: The mean (default=0.0)
 *   o: The standard deviation (default=1.0)
 */
KS_API bool nxrand_normal(nxrand_State self, nx_t R, nx_t u, nx_t o);


/** Submodule: 'nx.la' (linear algebra) **/

/* Create matrices with 'X' as the diagonal (expands a dimension)
 */
KS_API bool nxla_diag(nx_t X, nx_t R);


/* R = X @ Y */
KS_API bool nxla_matmul(nx_t X, nx_t Y, nx_t R);


/* R = X ** n (matrix power) */
KS_API bool nxla_matpowi(nx_t X, int n, nx_t R);


/** Submodule: 'nx.cv' (computer vision / image processing) **/


/** Submodule: 'nx.ch' (computer hearing / audio processing) **/


/*  */

/* Globals */


KS_API_DATA nxrand_State 
    nxrand_State_default /* Default random state */
;

/* Types */
KS_API_DATA ks_type
    nxt_dtype,
    nxt_array,
    nxt_view,

    nxrandt_State
;

/* C types */
KS_API_DATA nx_dtype
    nxd_bl,

    nxd_s8,
    nxd_u8,
    nxd_s16,
    nxd_u16,
    nxd_s32,
    nxd_u32,
    nxd_s64,
    nxd_u64,

    nxd_H,
    nxd_F,
    nxd_D,
    nxd_L,
    nxd_E,

    nxd_cH,
    nxd_cF,
    nxd_cD,
    nxd_cL,
    nxd_cE
;


#endif /* KSNX_H__ */
