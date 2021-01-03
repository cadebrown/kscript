/* sqrt.c - 'sqrt' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>
#include <ks/nxt.h>

#define K_NAME "sqrt"


/* 'x' with the sign of 'y' */
#define I_COPYSIGN(_x, _y) (((_y) < 0 == (_x) < 0) ? (_x) : -(_x))


static int kern(int N, nxar_t* inp, int len, void* _data) {
    assert(N == 2);
    assert(inp[0].dtype == inp[1].dtype);
    assert(inp[0].dtype->kind == NX_DTYPE_KIND_CFLOAT || inp[0].dtype->kind == NX_DTYPE_KIND_CCOMPLEX);

    ks_cint i;
    nx_dtype dtype = inp[0].dtype;
    ks_uint pR = (ks_uint)inp[0].data, pX = (ks_uint)inp[1].data;
    ks_cint sR = inp[0].strides[0], sX = inp[1].strides[0];

    if (dtype == nxd_float) {
        #define TYPE nxc_float
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
        #ifdef KS_HAVE_sqrtf
            *(TYPE*)pR = sqrtf(vX);
        #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtf", K_NAME, dtype);
            return 1;
        #endif
        }

        return 0;
        #undef TYPE
    } else if (dtype == nxd_double) {
        #define TYPE nxc_double
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
        #ifdef KS_HAVE_sqrt
            *(TYPE*)pR = sqrt(vX);
        #elif defined(KS_HAVE_sqrtf)
            *(TYPE*)pR = sqrtf(vX);
        #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrt, sqrtf", K_NAME, dtype);
            return 1;
        #endif
        }

        return 0;
        #undef TYPE
    } else if (dtype == nxd_longdouble) {
        #define TYPE nxc_longdouble
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
        #ifdef KS_HAVE_sqrtl
            *(TYPE*)pR = sqrtl(vX);
        #elif defined(KS_HAVE_sqrt)
            *(TYPE*)pR = sqrt(vX);
        #elif defined(KS_HAVE_sqrtf)
            *(TYPE*)pR = sqrtf(vX);
        #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtl, sqrt, sqrtf", K_NAME, dtype);
            return 1;
        #endif
        }

        return 0;
        #undef TYPE
    } else if (dtype == nxd_float128) {
        #define TYPE nxc_float128
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
        #ifdef KS_HAVE_sqrtf128
            *(TYPE*)pR = sqrtf128(vX);
        #elif defined(KS_HAVE_sqrtl)
            *(TYPE*)pR = sqrtl(vX);
        #elif defined(KS_HAVE_sqrt)
            *(TYPE*)pR = sqrt(vX);
        #elif defined(KS_HAVE_sqrtf)
            *(TYPE*)pR = sqrtf(vX);
        #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtf128, sqrtl, sqrt, sqrtf", K_NAME, dtype);
            return 1;
        #endif
        }

        return 0;
        #undef TYPE
    } else if (dtype == nxd_complexfloat) {
        #define TYPE nxc_complexfloat
        #define TYPER nxc_float
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
            TYPER ar = vX.re / 8, ai = vX.im;
            if (ar < 0) ar = -ar;
            if (ai < 0) ai = -ai;
            #ifdef KS_HAVE_sqrtf
            TYPER s = 2 * sqrtf(ar + 
              #ifdef KS_HAVE_hypotf
                hypotf(ar, ai / 8)
              #else
                sqrtf(ar * ar + ai * ai / 64)
              #endif
            );
            TYPER d = ai / (2 * s);
            TYPER rr, ri;
            if (vX.re >= 0.0) {
                ((TYPE*)pR)->re = s;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            } else {
                ((TYPE*)pR)->re = d;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            }

            #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtf", K_NAME, dtype);
            return 1;
            #endif
        }
        return 0;
        #undef TYPE
        #undef TYPER
    } else if (dtype == nxd_complexdouble) {
        #define TYPE nxc_complexdouble
        #define TYPER nxc_double
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
            TYPER ar = vX.re / 8, ai = vX.im;
            if (ar < 0) ar = -ar;
            if (ai < 0) ai = -ai;
            #ifdef KS_HAVE_sqrt
            TYPER s = 2 * sqrt(ar + 
              #ifdef KS_HAVE_hypot
                hypot(ar, ai / 8)
              #else
                sqrt(ar * ar + ai * ai / 64)
              #endif
            );
            TYPER d = ai / (2 * s);
            TYPER rr, ri;
            if (vX.re >= 0.0) {
                ((TYPE*)pR)->re = s;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            } else {
                ((TYPE*)pR)->re = d;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            }

            #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrt", K_NAME, dtype);
            return 1;
            #endif
        }
        return 0;
        #undef TYPE
        #undef TYPER
    } else if (dtype == nxd_complexlongdouble) {
        #define TYPE nxc_complexlongdouble
        #define TYPER nxc_longdouble
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
            TYPER ar = vX.re / 8, ai = vX.im;
            if (ar < 0) ar = -ar;
            if (ai < 0) ai = -ai;
            #ifdef KS_HAVE_sqrtl
            TYPER s = 2 * sqrtl(ar + 
              #ifdef KS_HAVE_hypotl
                hypotl(ar, ai / 8)
              #else
                sqrtl(ar * ar + ai * ai / 64)
              #endif
            );
            TYPER d = ai / (2 * s);
            TYPER rr, ri;
            if (vX.re >= 0.0) {
                ((TYPE*)pR)->re = s;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            } else {
                ((TYPE*)pR)->re = d;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            }

            #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtl", K_NAME, dtype);
            return 1;
            #endif
        }
        return 0;
        #undef TYPE
        #undef TYPER
    } else if (dtype == nxd_complexfloat128) {
        #define TYPE nxc_complexfloat128
        #define TYPER nxc_float128
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            TYPE vX = *(TYPE*)pX;
            TYPER ar = vX.re / 8, ai = vX.im;
            if (ar < 0) ar = -ar;
            if (ai < 0) ai = -ai;
            #ifdef KS_HAVE_sqrtf128
            TYPER s = 2 * sqrtf128(ar + 
              #ifdef KS_HAVE_hypotf128
                hypotf128(ar, ai / 8)
              #else
                sqrtf128(ar * ar + ai * ai / 64)
              #endif
            );
            TYPER d = ai / (2 * s);
            TYPER rr, ri;
            if (vX.re >= 0.0) {
                ((TYPE*)pR)->re = s;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            } else {
                ((TYPE*)pR)->re = d;
                ((TYPE*)pR)->im = I_COPYSIGN(d, vX.im);
            }

            #else
            KS_THROW(kst_Error, "Kernel '%s' is not supported for dtype %R, function(s) not found: sqrtf128", K_NAME, dtype);
            return 1;
            #endif
        }
        return 0;
        #undef TYPE
        #undef TYPER
    }

    KS_THROW(kst_Error, "Unsupported type for kernel '%s': %R", K_NAME, dtype);
    return 1;
}

bool nx_sqrt(nxar_t r, nxar_t x) {
    /* Determine result type */
    if (r.dtype->kind != NX_DTYPE_KIND_CFLOAT && r.dtype->kind != NX_DTYPE_KIND_CCOMPLEX) {
        KS_THROW(kst_Error, "Result type for '%s' must be float or complex, but was %R", K_NAME, r.dtype->name);
        return false;
    }

    nxar_t tX;
    kso rX;
    if (!nx_getcast(x, r.dtype, &tX, &rX)) {
        return false;
    }

    bool res = !nx_apply_elem(kern, 2, (nxar_t[]){ r, tX }, NULL);

    KS_NDECREF(rX);
    return res;
}


