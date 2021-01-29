/* nx/main.c - 'nx' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define M_NAME "nx"

/* C-API */


/* Module Functions */


/* Templates */


/* Arithmetic function taking 1 argument */
#define T_A1(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    KS_ARGS("x ?r", &ax, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nx_resnum(x.dtype, NULL); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        int rank; \
        ks_size_t shape[NX_MAXRANK]; \
        if (!nx_getbc(1, (nx_t[]) { x }, &rank, shape)) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}

/* Arithmetic function taking 1 argument, to real conversion */
#define T_A1_r(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    KS_ARGS("x ?r", &ax, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nx_resnum(x.dtype, NULL); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        dtype = nx_realtype(dtype); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        int rank; \
        ks_size_t shape[NX_MAXRANK]; \
        if (!nx_getbc(1, (nx_t[]) { x }, &rank, shape)) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}



/* Vector to matrix function taking a new dimension (expanding operation, i.e. onehot) */
#define T_A1_v2m_nd(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    ks_cint newdim; \
    KS_ARGS("x ?newdim:cint ?r", &ax, &newdim, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (x.rank < 1) { \
        KS_THROW(kst_SizeError, "Expected vector to have rank >= 1"); \
        KS_NDECREF(xr); \
        return NULL; \
    } \
    if (_nargs < 2) { \
        /* Default: square matrix */ \
        newdim = x.shape[x.rank - 1]; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nxd_bl; \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        int rank; \
        ks_size_t shape[NX_MAXRANK]; \
        if (!nx_getbc(1, (nx_t[]) { x }, &rank, shape)) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        shape[rank] = newdim; \
        rank++; \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}



/* Arithmetic function taking 2 arguments */
#define T_A2(_name) static KS_TFUNC(M, _name) { \
    kso ax, ay, ar = KSO_NONE; \
    KS_ARGS("x y ?r", &ax, &ay, &ar); \
    nx_t x, y, r; \
    kso xr, yr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (!nx_get(ay, NULL, &y, &yr)) { \
        KS_NDECREF(xr); \
        return NULL; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nx_resnum(x.dtype, y.dtype); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            KS_NDECREF(yr); \
            return NULL; \
        } \
        int rank; \
        ks_size_t shape[NX_MAXRANK]; \
        if (!nx_getbc(2, (nx_t[]) { x, y }, &rank, shape)) { \
            KS_NDECREF(xr); \
            KS_NDECREF(yr); \
            return NULL; \
        } \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            KS_NDECREF(yr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, y, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(yr); \
    KS_NDECREF(rr); \
    return ar; \
}

/* Conversion function taking 2 argument (array, and result type) */
#define T_C2(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    nx_dtype dtype; \
    KS_ARGS("x dtype:* ?r", &ax, &dtype, nxt_dtype, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (ar == KSO_NONE) { \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, x.rank, x.shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}


/* Reduction function taking 1 argument, to real conversion */
#define T_R1(_name) static KS_TFUNC(M, _name) { \
    kso ax, aaxes = KSO_NONE, ar = KSO_NONE; \
    KS_ARGS("x ?axes ?r", &ax, &aaxes, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    int naxes; \
    int axes[NX_MAXRANK]; \
    if (!nx_getaxes(aaxes, x.rank, &naxes, axes)) { \
        KS_NDECREF(xr); \
        return NULL; \
    } \
    if (ar == KSO_NONE) { \
        nx_dtype dtype = nx_resnum(x.dtype, NULL); \
        if (!dtype) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
        nx_t rsar = nx_delaxes(x, naxes, axes); \
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rsar.rank, rsar.shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r, naxes, axes)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}
/* Pencil taking 1 axis argument */
#define T_SORT1(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    ks_cint axis = -1; \
    KS_ARGS("x ?axis:cint ?r", &ax, &axis, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    axis = (axis % x.rank + x.rank) % x.rank; \
    if (ar == KSO_NONE) { \
        ar = (kso)nx_array_newc(nxt_array, NULL, x.dtype, x.rank, x.shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_cast(x,r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(r, axis)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}

/* Pencil taking 1 axis argument */
#define T_SORT2(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    ks_cint axis = -1; \
    KS_ARGS("x ?axis:cint ?r", &ax, &axis, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    axis = (axis % x.rank + x.rank) % x.rank; \
    if (ar == KSO_NONE) { \
        ar = (kso)nx_array_newc(nxt_array, NULL, x.dtype, x.rank, x.shape, NULL); \
        if (!ar) { \
            KS_NDECREF(xr); \
            return NULL; \
        } \
    } else { \
        KS_INCREF(ar); \
    } \
    if (!nx_get(ar, NULL, &r, &rr)) { \
        KS_NDECREF(xr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    if (!nx_##_name(x, r, axis)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}

/* Shape and datatype */
#define T_SD(_name, _func) static KS_TFUNC(M, _name) { \
    kso ashape = KSO_NONE; \
    nx_dtype dtype = nxd_bl; \
    KS_ARGS("?shape ?dtype:*", &ashape, &dtype, nxt_dtype); \
    int rank; \
    ks_size_t shape[NX_MAXRANK]; \
    if (!nx_getshape(ashape, &rank, shape)) { \
        return NULL; \
    } \
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
    if (!res) { \
        return NULL; \
    } \
    if (!nx_##_func(res->val)) { \
        KS_DECREF(res); \
        return NULL; \
    } \
    return (kso)res; \
}

T_SD(zeros, zero)
T_SD(ones, one)

T_C2(cast)
T_C2(fpcast)

T_A2(add)
T_A2(sub)
T_A2(mul)
T_A2(mod)
T_A2(pow)

T_A2(fmin)
T_A2(fmax)

T_A1(exp)
T_A1(log)

T_A1_r(abs)
T_A1(conj)
T_A1(neg)

T_A1_v2m_nd(onehot)

T_R1(min)
T_R1(max)
T_R1(sum)
T_R1(prod)

T_SORT2(cumsum)

T_SORT1(sort)


/* Export */

ks_module _ksi_nx() {
    _ksi_nx_array();
    _ksi_nx_dtype();
    _ksi_nx_view();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Submodules */
        {"la",                     (kso)_ksi_nx_la()},
        {"rand",                   (kso)_ksi_nxrand()},
        {"fft",                    (kso)_ksi_nx_fft()},

        /* Types */

        {"dtype",                  KS_NEWREF(nxt_dtype)},
        {"array",                  KS_NEWREF(nxt_array)},
        {"view",                   KS_NEWREF(nxt_view)},
    
        /* Datatypes */

        {"bool",                   KS_NEWREF(nxd_bl)},
        {"s8",                     KS_NEWREF(nxd_s8)},
        {"u8",                     KS_NEWREF(nxd_u8)},
        {"s16",                    KS_NEWREF(nxd_s16)},
        {"u16",                    KS_NEWREF(nxd_u16)},
        {"s32",                    KS_NEWREF(nxd_s32)},
        {"u32",                    KS_NEWREF(nxd_u32)},
        {"s64",                    KS_NEWREF(nxd_s64)},
        {"u64",                    KS_NEWREF(nxd_u64)},

        {"F",                      KS_NEWREF(nxd_F)},
        {"D",                      KS_NEWREF(nxd_D)},
        {"E",                      KS_NEWREF(nxd_E)},
        {"Q",                      KS_NEWREF(nxd_Q)},

        {"cF",                     KS_NEWREF(nxd_cF)},
        {"cD",                     KS_NEWREF(nxd_cD)},
        {"cE",                     KS_NEWREF(nxd_cE)},
        {"cQ",                     KS_NEWREF(nxd_cQ)},

        /** Aliases **/
        {"float",                  KS_NEWREF(nxd_F)},
        {"double",                 KS_NEWREF(nxd_D)},
        {"quad",                   KS_NEWREF(nxd_Q)},

        {"float32",                KS_NEWREF(nxd_F)},
        {"float64",                KS_NEWREF(nxd_D)},
        {"float128",               KS_NEWREF(nxd_Q)},

        {"complex32",              KS_NEWREF(nxd_cF)},
        {"complex64",              KS_NEWREF(nxd_cD)},
        {"complex128",             KS_NEWREF(nxd_cQ)},

        /* Functions */

        {"zeros",                  ksf_wrap(M_zeros_, M_NAME ".zeros(shape=none, dtype=nx.double)", "Create an array of zeros")},
        {"ones",                   ksf_wrap(M_ones_, M_NAME ".zeros(shape=none, dtype=nx.double)", "Create an array of ones")},

        {"onehot",                 ksf_wrap(M_onehot_, M_NAME ".onehot(x, newdim=none, r=none)", "Computes one-hot encoding, where 'x' are the indices, 'newdim' is the new dimension which the indices point to (default: last dimension of 'x')\n\n    Indices in 'x' are taken modulo 'newdim'")},

        {"abs",                    ksf_wrap(M_abs_, M_NAME ".abs(x, r=none)", "Computes elementwise absolute value")},
        {"conj",                   ksf_wrap(M_conj_, M_NAME ".conj(x, r=none)", "Computes elementwise conjugation")},
        {"neg",                    ksf_wrap(M_neg_, M_NAME ".neg(x, r=none)", "Computes elementwise negation")},

        {"add",                    ksf_wrap(M_add_, M_NAME ".add(x, y, r=none)", "Computes elementwise addition")},
        {"sub",                    ksf_wrap(M_sub_, M_NAME ".sub(x, y, r=none)", "Computes elementwise subtraction")},
        {"mul",                    ksf_wrap(M_mul_, M_NAME ".mul(x, y, r=none)", "Computes elementwise multiplication")},
        {"mod",                    ksf_wrap(M_mod_, M_NAME ".mod(x, y, r=none)", "Computes elementwise modulo")},
        {"pow",                    ksf_wrap(M_pow_, M_NAME ".pow(x, y, r=none)", "Computes elementwise power")},

        {"exp",                    ksf_wrap(M_exp_, M_NAME ".exp(x, y, r=none)", "Computes elementwise exponential")},
        {"log",                    ksf_wrap(M_log_, M_NAME ".log(x, y, r=none)", "Computes elementwise logarithm")},

        {"min",                    ksf_wrap(M_min_, M_NAME ".min(x, axes=none, r=none)", "Minimum of elements")},
        {"max",                    ksf_wrap(M_max_, M_NAME ".max(x, axes=none, r=none)", "Maximum of elements")},
        {"sum",                    ksf_wrap(M_sum_, M_NAME ".sum(x, axes=none, r=none)", "Sum elements")},
        {"prod",                   ksf_wrap(M_prod_, M_NAME ".prod(x, axes=none, r=none)", "Product elements")},

        {"cast",                   ksf_wrap(M_cast_, M_NAME ".cast(x, dtype, r=none)", "Casts to a datatype")},
        {"fpcast",                 ksf_wrap(M_fpcast_, M_NAME ".fpcast(x, dtype=none, r=none)", "Casts to a datatype, with automatic fixed-point and floating-point conversion")},

        {"sort",                   ksf_wrap(M_sort_, M_NAME ".sort(x, axis=-1, r=none)", "Sorts")},
        {"cumsum",                 ksf_wrap(M_cumsum_, M_NAME ".cumsum(x, axis=-1, r=none)", "Computes cumulative sum")},


    /*


        {"neg",                    ksf_wrap(M_neg_, M_NAME ".neg(x, r=none)", "Computes elementwise negation")},
        {"abs",                    ksf_wrap(M_abs_, M_NAME ".abs(x, r=none)", "Computes elementwise absolute value")},
        {"conj",                   ksf_wrap(M_conj_, M_NAME ".conj(x, r=none)", "Computes elementwise conjugation")},

        {"fmin",                   ksf_wrap(M_fmin_, M_NAME ".fmin(x, y, r=none)", "Computes elementwise minimum")},
        {"fmax",                   ksf_wrap(M_fmax_, M_NAME ".fmin(x, y, r=none)", "Computes elementwise maximum")},
        {"clip",                   ksf_wrap(M_clip_, M_NAME ".clip(x, y, z, r=none)", "Computes elementwise clipping between 'y' (minimum) and 'z' (maximum)\n\n    Equivalent to 'nx.fmin(z, nx.fmax(y, x))'")},

        {"add",                    ksf_wrap(M_add_, M_NAME ".add(x, y, r=none)", "Computes elementwise addition")},
        {"sub",                    ksf_wrap(M_sub_, M_NAME ".sub(x, y, r=none)", "Computes elementwise subtraction")},
        {"mul",                    ksf_wrap(M_mul_, M_NAME ".mul(x, y, r=none)", "Computes elementwise multiplication")},
        {"mod",                    ksf_wrap(M_mod_, M_NAME ".mod(x, y, r=none)", "Computes elementwise modulo")},
        {"div",                    ksf_wrap(M_div_, M_NAME ".div(x, y, r=none)", "Computes elementwise division (true division, not floor)")},
        {"floordiv",               ksf_wrap(M_floordiv_, M_NAME ".floordiv(x, y, r=none)", "Computes elementwise floored division")},

        {"pow",                    ksf_wrap(M_pow_, M_NAME ".pow(x, y, r=none)", "Computes elementwise exponentiation")},
        
        {"sqrt",                   ksf_wrap(M_sqrt_, M_NAME ".sqrt(x, y, r=none)", "Computes elementwise square root")},
        {"exp",                    ksf_wrap(M_exp_, M_NAME ".exp(x, r=none)", "Computes elementwise exponential function (base-e)")},
        {"log",                    ksf_wrap(M_log_, M_NAME ".log(x, r=none)", "Computes elementwise logarithm function (base-e)")},
        
        {"sin",                    ksf_wrap(M_sin_, M_NAME ".sin(x, r=none)", "Computes elementwise sine")},
        {"cos",                    ksf_wrap(M_cos_, M_NAME ".cos(x, r=none)", "Computes elementwise cosine")},
        {"tan",                    ksf_wrap(M_tan_, M_NAME ".tan(x, r=none)", "Computes elementwise tangent")},
        {"asin",                   ksf_wrap(M_asin_, M_NAME ".asin(x, r=none)", "Computes elementwise inverse sine")},
        {"acos",                   ksf_wrap(M_acos_, M_NAME ".acos(x, r=none)", "Computes elementwise inverse cosine")},
        {"atan",                   ksf_wrap(M_atan_, M_NAME ".atan(x, r=none)", "Computes elementwise inverse tangent")},
        {"sinh",                   ksf_wrap(M_sinh_, M_NAME ".sinh(x, r=none)", "Computes elementwise hyperbolic sine")},
        {"cosh",                   ksf_wrap(M_cosh_, M_NAME ".cosh(x, r=none)", "Computes elementwise hyperbolic cosine")},
        {"tanh",                   ksf_wrap(M_tanh_, M_NAME ".tanh(x, r=none)", "Computes elementwise hyperbolic tangent")},
        {"asinh",                  ksf_wrap(M_asinh_, M_NAME ".asinh(x, r=none)", "Computes elementwise inverse hyperbolic sine")},
        {"acosh",                  ksf_wrap(M_acosh_, M_NAME ".acosh(x, r=none)", "Computes elementwise inverse hyperbolic cosine")},
        {"atanh",                  ksf_wrap(M_atanh_, M_NAME ".atanh(x, r=none)", "Computes elementwise inverse hyperbolic tangent")},

    */
    ));

    return res;
}
