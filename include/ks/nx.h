/* ks/nx.h - header for the nx (NumeriX) library in kscript
 * 
 * This module is a numerical computation library, specifically meant for vectorized inputs and tensor
 *   math. It deals (typically) with a tensor (C-type: 'nx_t') that has a given data type ('nx_dtype', 
 *   which may be a C-style type, or 'object' for generic operations), as well as dimensions and strides.
 * 
 * Scalars can be represented with a rank of 0, which still holds one element
 *
 * Operations are applied by vectorization -- which is to say per each element independently, with results not affecting
 *   each other. So, adding vectors adds them elementwise, not appending them to the end. These are called scalar-vectorized
 *   operations. Some operations take more than single elements (they take a 1D sub-vector, called a 'pencil' or 'vector'). These
 *   will work per-pencil/per-vector, and can still be parallelized per pencil/vectory (for example, a 2D tensor would have 1D of parallelization,
 *   but 1D values being processed together). There can also be functions for any dimensional slice of data, but these are rarer (a notable
 *   example is matrix multiplication).
 * 
 * Broadcasting is a term that describes how tensors of different shapes are processed together in a kernel. Obviously, tensors
 *   of the same shape can be processed together, but broadcasting allows tensors of different sizes to be processed together, by
 *   duplicating and repeating values. Broadcasting has the following steps:
 * 
 *   * All inputs are expanded to the largest rank of any input, expanding the shape to the left with '1's. For example, (2, 3, 4) and (3, 4) are
 *     expanded to (2, 3, 4) and (1, 3, 4)
 *   * Each dimension is checked, and if each input has either '1' or one additional number (let's call 'x'), they broadcast together. Inputs with a '1' in that dimension
 *     are repeated to 'x' times. For example, given shapres of (2, 3, 4) and (1, 3, 4), the '0' dimensions are all either 1 or 2, so the '1' are duplicated, so that
 *     two copies of the (1, 3, 4) tensor are applied with the two slices of the (2, 3, 4) tensor.
 * 
 * Signatures of operations may look like: (..., N), (..., N) -> (..., N). The '...' stand for 'any broadcastable dimensions', and each output can be viewed
 *   as a stack of (N,)-shaped vectors
 * 
 * 0D to 0D (element-wise):
 *   * add (...,) -> (...,)
 *   * sub (...,) -> (...,)
 *   * mul (...,) -> (...,)
 * 
 * 1D to 0D (1D reduction):
 *   * min (..., N) -> (...)
 * 
 * 1D to 1D (vector-wise):
 *   * sort (..., N) -> (..., N)
 *
 * 2D to 0D (2D reduction)
 *   * matnorm_* (..., M, N) -> (...)
 * 
 * 2D to 2D (matrix-wise):
 *   * matmul (..., M, N), (..., N, K) -> (..., M, K) 
 *
 * ND to ND
 *   * fft (..., *XYZ) -> (..., *XYZ)
 * 
 * ND to 0D:
 *   * sum (..., *XYZ) -> (...,)

 * These transforms can sometimes be applied multiple times, or for different axes. For example, 'sum' operation can take 1 or more
 *   axis and is equivalent to reducing on each axis. You can sum over all axes to have a single scalar element, or all but one
 *   to reduce (N-1)D summations.
 * 
 * Unlike other standard modules, the prefixes for datatypes/functions/variables do not have 'ks' prefixing them,
 *   only 'nx'.
 * 
 * 
 * --- Standard Types ---
 * 
 * Provided in NumeriX are some standard datatypes, which are signed and unsigned integers of size 8, 16, 32, and 64 bits.
 *   For example, 'nx_s8' is a signed 8 bit int, and 'nx_u8' is an unsigned 8 bit int
 * 
 * There is also a boolean type, which is 'nx_bl'
 * 
 * There are builtin float types: half, float, double, long double, float128. These may or may not be present on a given
 *   architecture. When they aren't available they will be the closest type possible. These are the types:
 *   * nx_F: Single precision (float)
 *   * nx_D: Double precision (double)
 *   * nx_E: Longer double precision (extended) (long double)
 *   * nx_Q: Quad precision (__float128)
 * 
 * Also, for floating point types, macros are defined in 'nxm.h' for common operations. For example:
 *   'nx_Lfmin()' gives an 'fmin()' like function that will work with that type. This is done so that generating
 *   type-generic kernels can use 'TYPE' + 'fmin'. For example, in the C preprocessor, if 'TYPE' is 'nx_L', you can write:
 *   'TYPE##fmin(a, b)', and it will generate appropriate function calls for each type. Also see 'nxt.h' for template generation
 * 
 * Submodules:
 * 
 *   nx.rand: Random number generation
 *   nx.la: Linear algebra routines
 *   nx.fft: Fast Fourier Transform (FFT) routines
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


/** Constants **/

/* Maximum rank of a tensor, used so statically allocating tensor sizes/strides is possible, reducing
 *   overhead
 */
#define NX_MAXRANK 16

/* Maximum broadcast size (i.e. maximum number of arguments to a single kernel)
 *
 */
#define NX_MAXBCS 8


/** Builtin Types & Constants **/


/* Math Constants */
#define NX_PI 3.14159265358979323846264338327950288419716939937510
#define NX_E  2.71828182845904523536028747135266249775724709369995


/*** Boolean
 *
 * Represents a true or false
 * 
 ***/

typedef   bool     nx_bl;

/* MIN value */
#define nx_blMIN  (false)

/* MAX value */
#define nx_blMAX  (true)

#define nx_blv(_val) ((_val) ? true : false)

/*** Integers - signed & unsigned
 *
 * Integer types have the properties:
 * 
 * MIN: MIN value
 * MAX: MAX value
 * 
 ***/

typedef   uint8_t  nx_u8;
typedef  uint16_t  nx_u16;
typedef  uint32_t  nx_u32;
typedef  uint64_t  nx_u64;
typedef    int8_t  nx_s8;
typedef   int16_t  nx_s16;
typedef   int32_t  nx_s32;
typedef   int64_t  nx_s64;

/* MIN value */
#define  nx_u8MIN (0U)
#define nx_u16MIN (0U)
#define nx_u32MIN (0UL)
#define nx_u64MIN (0ULL)
#define  nx_s8MIN (-128)
#define nx_s16MIN (-32768)
#define nx_s32MIN (-2147483648LL)
#define nx_s64MIN (-9223372036854775807LL-1)

/* MAX value */
#define  nx_s8MAX (127)
#define  nx_u8MAX (255)
#define nx_s16MAX (32767)
#define nx_u16MAX (65536)
#define nx_s32MAX (2147483647LL)
#define nx_u32MAX (4294967295ULL)
#define nx_s64MAX (9223372036854775807LL)
#define nx_u64MAX (1844674407370955161ULL)


/*** Floats
 *
 * Floating point types have the properties:
 * 
 * MIN: MIN value (positive)
 * MAX: MAX value (non-infinite)
 * EPS: Difference between '1.0' and the next representable number
 * INF: '+inf'
 * NAN: 'nan'
 * DIG: Number of base-10 digits fully representable
 * 
 * Specific types:
 * 
 *   S: 'single', single precision real ('float' type in C)
 *   D: 'double', double precision real ('double' type in C)
 *   E: 'extended', extended precision real ('long double' type in C)
 *   Q: 'quad', quadruple precisino real ('__float128' type in C)
 * 
 ***/

typedef        float  nx_F;
typedef       double  nx_D;
typedef  long double  nx_E;
#if defined(KS_HAVE__Float128)
  typedef  _Float128  nx_Q;
#elif defined(KS_HAVE___float128)
  typedef __float128  nx_Q;
#else
  /* Fallback: Just use 'long double' */
  typedef nx_E nx_Q;
#endif


/* Value from literal */
#define nx_Fv_(_val) _val##f
#define nx_Fv(_val) nx_Fv_(_val)
#define nx_Dv_(_val) _val
#define nx_Dv(_val) nx_Dv_(_val)
#define nx_Ev_(_val) _val##l
#define nx_Ev(_val) nx_Ev_(_val)
#if defined(KS_HAVE__Float128) || defined(KS_HAVE___float128)
  #define nx_Qv_(_val) _val##q
#else
  #define nx_Qv_ nx_Ev_
#endif
#define nx_Qv(_val) nx_Qv_(_val)


#define nx_Fdtype nxd_F
#define nx_Ddtype nxd_D
#define nx_Edtype nxd_E
#define nx_Qdtype nxd_Q

/* '+inf' value */
#define nx_FINF INFINITY
#define nx_DINF INFINITY
#define nx_EINF INFINITY
#define nx_QINF INFINITY

/* 'nan' value */
#define nx_FNAN NAN
#define nx_DNAN NAN
#define nx_ENAN NAN
#define nx_QNAN NAN

/* Digits of accuracy */
#define nx_FDIG FLT_DIG
#define nx_DDIG DBL_DIG
#define nx_EDIG LDBL_DIG
#ifdef FLT128_DIG
  #define nx_QDIG FLT128_DIG
#else
  #define nx_QDIG 33
#endif

/* Difference between 1.0 and next largest number */
#define nx_FEPS FLT_EPSILON
#define nx_DEPS DBL_EPSILON
#define nx_EEPS LDBL_EPSILON
#ifdef FLT128_EPSILON
  #define nx_QEPS FLT128_EPSILON
#else
  #define nx_QEPS nx_Qv(1.92592994438723585305597794258492732e-34)
#endif

/* Minimum positive value */
#define nx_FMIN FLT_MIN
#define nx_DMIN DBL_MIN
#define nx_EMIN LDBL_MIN
#ifdef FLT128_MIN
  #define nx_QMIN FLT128_MIN
#else
  #define nx_QMIN nx_Qv(3.36210314311209350626267781732175260e-4932)
#endif

/* Maximum positive finite value */
#define nx_FMAX FLT_MAX
#define nx_DMAX DBL_MAX
#define nx_EMAX LDBL_MAX
#ifdef FLT128_MAX
  #define nx_QMAX FLT128_MAX
#else
  #define nx_QMAX nx_Qv(1.18973149535723176508575932662800702e4932)
#endif


/*** Complexes
 *
 * Complex floating point types correspond to a non-complex floating point type
 * 
 ***/

/* Structure definitions */
typedef struct { nx_F re, im; } nx_cF;
typedef struct { nx_D re, im; } nx_cD;
typedef struct { nx_E re, im; } nx_cE;
typedef struct { nx_Q re, im; } nx_cQ;


/* Aliases to 'real'/'scalar' type*/
#define nx_cFr nx_F
#define nx_cDr nx_D
#define nx_cEr nx_E
#define nx_cQr nx_Q

#define nx_cFrdtype nx_Fdtype
#define nx_cDrdtype nx_Ddtype
#define nx_cErdtype nx_Edtype
#define nx_cQrdtype nx_Qdtype

#define nx_cFrv nx_Fv
#define nx_cDrv nx_Dv
#define nx_cErv nx_Ev
#define nx_cQrv nx_Qv

#define nx_cFrINF nx_FINF
#define nx_cDrINF nx_DINF
#define nx_cErINF nx_EINF
#define nx_cQrINF nx_QINF

#define nx_cFrNAN nx_FNAN
#define nx_cDrNAN nx_DNAN
#define nx_cErNAN nx_ENAN
#define nx_cQrNAN nx_QNAN

#define nx_cFrMIN nx_FMIN
#define nx_cDrMIN nx_DMIN
#define nx_cErMIN nx_EMIN
#define nx_cQrMIN nx_QMIN

#define nx_cFrMAX nx_FMAX
#define nx_cDrMAX nx_DMAX
#define nx_cErMAX nx_EMAX
#define nx_cQrMAX nx_QMAX

#define nx_cFrEPS nx_FEPS
#define nx_cDrEPS nx_DEPS
#define nx_cErEPS nx_EEPS
#define nx_cQrEPS nx_QEPS


/** Alias datatypes **/

/* Index data type */
#define nx_idx nx_s64
#define nxd_idx nxd_s64



/** NumeriX API **/

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

    /* Attribute dictionary */
    ks_dict attr;

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


/* nx.array.__iter - Array iterator
 *
 */
typedef struct nx_array_iter_s {
    KSO_BASE

    /* Object being iterated */
    nx_array of;

    /* Position in the major-most dimension */
    ks_cint pos;

}* nx_array_iter;

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
typedef int (*nxf_elem)(int nargs, nx_t* args, int len, void* extra);

/* Function signature for broadcasting/function application of any degrees
 */
typedef int (*nxf_Nd)(int nargs, nx_t* args, int rank, ks_size_t* shape, void* extra);


/* Functions */


/** 'nx_t' operations (nx.c) **/

/* Create an array descriptor
 */
KS_API nx_t nx_make(void* data, nx_dtype dtype, int rank, ks_size_t* shape, ks_ssize_t* strides);


/* Create a new array descriptor with a new axis inserted at 'axis'
 *
 * The new array descriptor has 'shape[axis] == 1' and 'stride[axis] == 0'. Axes after 'axis' are 
 *   shifted over by one to make room 
 * 
 * Another way to say it is that 'axis' is the position of the axis in the *result* array descriptor
 */
KS_API nx_t nx_newaxis(nx_t self, int axis);

/* Create a new array descriptor with extra axes inserted, where the 'axes' array are the indices
 *   of the axes in the *result* array descriptor
 */
KS_API nx_t nx_newaxes(nx_t self, int naxes, int* axes);


/* Create a new array descriptor with the given axis deleted
 */
KS_API nx_t nx_delaxis(nx_t self, int axis);

/* Create a new array descriptor with the given axes deleted
 */
KS_API nx_t nx_delaxes(nx_t self, int naxes, int* axes);


/* Create a new array descriptor with axes 'a' and 'b' swapped
 */
KS_API nx_t nx_swapaxes(nx_t self, int a, int b);


/* Create a new array descriptor with 'axes' put at the end of the shape
 */
KS_API nx_t nx_sinkaxes(nx_t self, int naxes, int* axes);

/* Create an array descriptor with 'axes' taken from the end of the shape
 *  (inverse of 'nx_sinkaxes')
 */
KS_API nx_t nx_unsinkaxes(nx_t self, int naxes, int* axes);


/* Creates a new array descriptor from subscripting 'self', which returns an
 *   element view (ev) from object arguments
 * 
 * Each element of 'args' may be:
 *   * An integer, in which case one panel from that dimension is returned in the view
 *   * A 'slice' object, in which case the axes described by the 'slice'
 *   * '...' (only one of these is allowed in one 'args'), which spreads the objects to the
 *       left and right and doesn't affect the interior axes
 * 
 * TODO: Support 'nx.newaxis' to insert an axis
 * TODO: Also support tuple/iterables to build arrays (i.e. x[(1, 3, 4)] to grab specific rows)
 */
KS_API nx_t nx_getevo(nx_t self, int nargs, kso* args);

/* Appends the string representation of 'self' to an IO-like object
 */
KS_API bool nx_getstr(ksio_BaseIO io, nx_t self);

/* Appends the raw bytes of 'self' to an IO-like object
 */
KS_API bool nx_getbytes(ksio_BaseIO io, nx_t self);


/* Get low-level string represnting 'x'
 */
KS_API ks_str nx_getbs(nx_t x);

/* Convert an object into an array descriptor of a given datatype (or NULL to calculate
 *   a default)
 *
 * Sets '*res' to the array, and '*ref' to a reference that must be held and destroyed
 *   while using the array descriptor (or NULL for none needed)
 * 
 * Return success
 */
KS_API bool nx_get(kso obj, nx_dtype dtype, nx_t* res, kso* ref);

/* Cast 'X' to a given data type ('dtype') (keeping as-is if it can), and store in '*R'
 * 
 * '*tofree' is set to a pointer that should be passed to 'ks_free()' after you are done
 *   with '*R'. It may be set to NULL if no extra data allocation was needed
 */
KS_API bool nx_getas(nx_t self, nx_dtype dtype, nx_t* res, void** tofree);


/** Utilities **/

/* Encode object into memory
 *
 * Ensure that the 'out' holds 'dtype->size' bytes
 */
KS_API bool nx_enc(nx_dtype dtype, kso obj, void* out);

/* Sorts an array of 'int' in place
 */
KS_API void nx_intsort(int nvals, int* vals);

/* Get the result datatype of a numeric operation, and does not return a reference
 *
 * 'e' is extended, which allows flags:
 *   'r2c': If this is given, then real results are translated to complex types
 *   'c2r': If this is given, then complex results are translated to real types
 */
KS_API nx_dtype nx_resnum(nx_dtype X, nx_dtype Y);
KS_API nx_dtype nx_resnume(nx_dtype X, nx_dtype Y, bool r2c, bool c2r);

/* Returns the corresonding 'real' type
 * For complex types, returns the corresponding float type
 * For integer types, returns 'double'
 */
KS_API nx_dtype nx_realtype(nx_dtype X);

/* Returns the corresonding 'complex' type
 * For float types, returns the corresponding complex type
 * For integer types, returns 'complexdouble'
 */
KS_API nx_dtype nx_complextype(nx_dtype X);

/* Interprets 'obj' as a shape parameter to a function
 *
 * 'none' is the same as '()'
 * integers are rank==1
 * tuples result in a shape of their length, with elements being integers
 */
KS_API bool nx_getshape(kso obj, int* rank, ks_size_t* shape);
KS_API bool nx_getshapev(int nargs, kso* args, int* rank, ks_size_t* shape);

/* Interprets 'obj' as a list of axes referring to a rank 'rank' tensor
 *
 * 'none' is the same as all axes
 * integers are rank==1
 * tuples result in a shape of their length, with elements being integers
 */
KS_API bool nx_getaxes(kso obj, int rank, int* naxes, int* axes);


/** Broadcasting/Apply Routines **/

/* Calculate a broadcast shape, and store the 'rank' in '*rank' and the shape in 'shape'
 * Returns success, or throws an error
 */
KS_API bool nx_getbc(int nargs, nx_t* args, int* rank, ks_size_t* shape);

/* Apply an element-wise function application on the arguments
 *
 * If 'dtype' is non-NULL, then chunks are casted to that before execution. If dtypeidx < 0,
 *   then all arguments are converted. Otherwise, only that argument is converted
 * 
 * Returns either 0 if all executions returned 0, or the first non-zero code
 *   encountered by calling 'func()'
 */
KS_API int nx_apply_elem(nxf_elem func, int nargs, nx_t* args, nx_dtype dtype, void* extra);
KS_API int nx_apply_eleme(nxf_elem func, int nargs, nx_t* args, nx_dtype dtype, int dtypeidx, void* extra);

/* Apply an Nd-wise function to the arguments, where 'M' is the number of dimensions
 *
 * Returns either 0 if all executions returned 0, or the first non-zero code
 *   encountered by calling 'func()'
 */
KS_API int nx_apply_Nd(nxf_Nd func, int nargs, nx_t* args, int M, nx_dtype dtype, void* extra);
KS_API int nx_apply_Nde(nxf_Nd func, int nargs, nx_t* args, int M, nx_dtype dtype, int dtypeidx, void* extra);



/** Creation Routines **/


/* Expand an iterable recursively to create a huge block of objects
 */
KS_API kso* nx_objblock(kso objs, int* rank, ks_size_t* shape);


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

/* Create a new view object from a value and reference
 *
 */
KS_API nx_view nx_view_newo(ks_type tp, nx_t val, kso ref);

/** Datatypes **/

/* Create a structure-style datatype
 * 'members' should be an iterable of tuples of '(name, type)' or '(name, type, offset)'
 */
KS_API nx_dtype nx_dtype_struct(ks_str name, kso members);



/** Operations **/

/* Fill R with zeros
 */
KS_API bool nx_zero(nx_t R);

/* Fill R with ones
 */
KS_API bool nx_one(nx_t R);

/* R[X[i] % R.dim] = 1, 0 otherwise
 * 
 * X[...]
 * R[..., N]
 */
KS_API bool nx_onehot(nx_t X, nx_t R);

/* R[:] = sum(X)
 * 
 */
KS_API bool nx_sum(nx_t X, nx_t R, int naxes, int* axes);

/* R[:] = prod(X)
 * 
 */
KS_API bool nx_prod(nx_t X, nx_t R, int naxes, int* axes);

/* R[:] = cumsum(X)
 */
KS_API bool nx_cumsum(nx_t X, nx_t R, int axis);

/* R[:] = cumprod(X)
 */
KS_API bool nx_cumprod(nx_t X, nx_t R, int axis);

/* R[:] = min(X)
 *
 */
KS_API bool nx_min(nx_t X, nx_t R, int naxes, int* axes);

/* R[:] = max(X)
 *
 */
KS_API bool nx_max(nx_t X, nx_t R, int naxes, int* axes);

/* Sorts 'X' in place
 *
 */
KS_API bool nx_sort(nx_t X, int axis);
KS_API bool nx_sort_quick(nx_t X, int axis);


/*** Conversion Operations ***/

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


/*** Arithmetic Operations ***/

/* R = -X */
KS_API bool nx_neg(nx_t X, nx_t R);

/* R = abs(X) 
 *
 * This kernel is special: If you give it a complex -> real abs, you must provide
 *   the arguments in the correct type
 */
KS_API bool nx_abs(nx_t X, nx_t R);

/* R = ~X (conjugation) */
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
 */
KS_API bool nxrand_normal(nxrand_State self, nx_t R);


/** Submodule: 'nx.la' (linear algebra) **/

/* Create matrices with 'X' as the diagonal (expands a dimension)
 *
 * X[..., N]
 * R[..., N, N] 
 */
KS_API bool nxla_diag(nx_t X, nx_t R);


/* perm==onehot */
#define nxla_perm nx_onehot

/* Calculates Frobenius norm (sometimes called Euclidean norm) of 'R' and stores it in 'X'
 *
 * To calculate for vectors, call with 'nx_newaxis(X, X.rank - 1)'
 * 
 * X[..., M, N]
 * R[...]
 */
KS_API bool nxla_norm_fro(nx_t X, nx_t R);

/* R = X @ Y, matrix multiplication
 *
 * X[..., M, N]
 * Y[..., N, K]
 * R[..., M, K]
 */
KS_API bool nxla_matmul(nx_t X, nx_t Y, nx_t R);

/* R = X @ Y[..., nx.newaxis], matrix-vector multiplication, treating 'Y' as a column vector
 *
 * X[..., M, N]
 * Y[..., N]
 * R[..., M]
 */
KS_API bool nxla_matmulv(nx_t X, nx_t Y, nx_t R);

/* R = X ** n, matrix power for square matrices
 *
 * X[..., N, N]
 * R[..., N, N]
 */
KS_API bool nxla_matpowi(nx_t X, int n, nx_t R);

/* Performs LU factorization. Factors 'X := nx.la.perm(P) @ L @ U', with 'L' being lower triangular
 *   and 'U' being upper triangular, and 'P' being a vector of integers used for a permutation matrix
 * 
 * X's shape should be [..., N, N]
 * P's shape should be [..., N]
 * L's shape should be [..., N, N]
 * U's shape should be [..., N, N]
 */
KS_API bool nxla_factlu(nx_t X, nx_t P, nx_t L, nx_t U);


/** Submodule: 'nx.fft' (Fast Fourier Transform) **/

/* FFT Plan kinds (computed X:(*XYZ) from x:(*XYZ)) */
enum {

    /* Empty/error FFT kind */
    NXFFT_NONE         = 0,

    /* 1D dense plan, which is a matrix-vector product between the roots of unity
     *   and the input vector.
     * 
     * Only valid when rank==1, but works for any 'N'
     * 
     * Time: O(N^2)
     * Formula: X = W @ x
     */
    NXFFT_1D_DENSE     = 1,

    /* 1D Bluestein algorithm via the Chirp-Z transform
     *
     * Only valid when rank==1, but works for any 'N'
     * 
     * Although this has the same O() of 'NXFFT_1D_BFLY', this algorithm is about
     *   5x-15x slower than that algorithm.
     * 
     * Time: O(N*log(N))
     * Algorithm: ... see 'nx/fft/plan.c'
     */
    NXFFT_1D_BLUE      = 2,

    /* 1D Cooley-Tukey butterfly algorithm
     *
     * Only valid when rank==1 and N == 2 ** x
     * 
     * This is by far the fastest (custom) plan I've implemented and should be preferred
     * 
     * Time: O(N*log(N))
     * Algorithm: ... see 'nx/fft/plan.c'
     * 
     */
    NXFFT_1D_BFLY      = 3,

    /* ND generic algorithm which recursively uses 1D algorithms over each dimension
     *
     */
    NXFFT_ND_DEFAULT   = 10,

    /* ND FFTW3 wrapper
     *
     * Only valid when FFTW3 was compiled with kscript
     */
    NXFFT_ND_FFTW3     = 11,

};


/* nx.fft.plan - FFT precomputed coefficient
 *
 */
typedef struct nxfft_plan_s* nxfft_plan;
struct nxfft_plan_s {
    KSO_BASE

    /* Rank of the transformation */
    int rank;

    /* Shape of the transformation */
    ks_size_t shape[NX_MAXRANK];

    /* Kind of transformation (see 'NXFFT_*') */
    int kind;
  
    /* Whether it is an inverse transform */
    bool is_inv;

    /* Discriminated union based on 'kind' */
    union {

        /* 1D dense matrix-vector product
         *
         * When kind==NXFFT_1D_DENSE
         */
        struct {

            /* W:(N, N): Array of roots of unity
             *
             * W[j, k] = exp(-2*pi*i*j*k/N)
             * (or, if inverse, the reciprocal)
             * 
             * FFT(x) = W @ x
             */
            nx_t W;

        } k1D_DENSE;

        /* 1D Bluestein algorithm via Chirp-Z
         *
         * 
         * When kind==NXFFT_1D_BLUE
         */
        struct {

            /* Internal convolution length, satisfies: 
             * M >= 2*N+1, and M==2**x
             */
            ks_size_t M;

            /* FFT for (M,) size
             *
             * Internally, Bluestein's algorithm uses a convolutino of length M, so
             *   we compute that plan as well, and observe that:
             * CONV(x, y) = IFFT(FFT(x)*FFT(Y))
             * And:
             * IFFT(x) = (FFT(x)[0], FFT(x)[1:][::-1]) / N
             * 
             * So we can apply the forward plan twice and adjust
             */
            nxfft_plan planM;


            /* Roots of unity to squared indices power:
             *
             * Ws[j] = exp(-pi*i*j**2/N)
             *   (or, for inverse, the reciprocal)
             */
            nx_t Ws;

            /* Temporary buffer, of size 2*M for 'tA' and 'tB'
             *
             * Partitioned like: [*tA, *tB]
             * tA=&tmpbuf[0], tB=&tmpbuf[M]
             */
            nx_t tmp;

        } k1D_BLUE;

        /* 1D Cooley-Tukey Butterfly transform
         *
         * TODO: perhaps also precompute a lookup table for bit-reversal indices, another O(N) cost,
         *   but saves O(N*log(N)) operation on each invocation, but also this means it can't be done
         *   in place
         * 
         * When kind==NXFFT_1D_BFLY
         */
        struct {

            /* W:(N,): Array of roots of unity
             *
             * W[j, k] = exp(-2*pi*i*j*k/N)
             * (or, if inverse, the reciprocal)
             * 
             * FFT(x) = W @ x
             */
            nx_t W;

        } k1D_BFLY;

        /* ND Default transform
         *
         * Uses 1D transform for each axis
         * 
         * 
         * When kind==NXFFT_ND_DEFAULT
         */
        struct {

            /* Plan for each dimension in 'rank' */
            nxfft_plan* plans;

        } kND_DEFAULT;

#ifdef KS_HAVE_fftw3

        /* ND plan using FFTW3
         *
         */
        struct {

            /* FFTW plan */
            fftw_plan plan;

  #ifdef KS_HAVE_fftw3f
            fftwf_plan planf;
  #endif
  #ifdef KS_HAVE_fftw3l
            fftwl_plan planl;
  #endif
  #ifdef KS_HAVE_fftw3q
            fftwq_plan planq;
  #endif
            /* Temporary storage */
            nx_t tmp;

        } kND_FFTW3;

#endif

    };

};


/* Create an FFT plan for a given transform size
 *
 */
KS_API nxfft_plan nxfft_make(nx_dtype dtype, int rank, ks_size_t* dims, bool is_inv);

/* Execute an FFT plan, and compute:
 * R = FFT(X)
 */
KS_API bool nxfft_exec(nx_t X, nx_t R, int naxes, int* axes, nxfft_plan plan, bool is_inv);



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
    nxt_array_iter,
    nxt_view,

    nxrandt_State,

    nxfftt_plan

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

    nxd_F,
    nxd_D,
    nxd_E,
    nxd_Q,

    nxd_cF,
    nxd_cD,
    nxd_cE,
    nxd_cQ
;


#endif /* KSNX_H__ */
