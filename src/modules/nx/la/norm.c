/* norm.c - 'norm' kernels
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define K_NAME "norm_fro"

/* Access Macros */
#define X_(_i, _j) (pX + rsX * (_i) + csX * (_j))

#define LOOPI(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype); \
    return 1; \
} \

#define LOOPF(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.rank == 2 && R.rank == 2); \
    assert(X.dtype == R.dtype); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0] \
    ; \
    ks_cint \
        csX = X.strides[1] \
    ; \
    ks_cint i, j; \
    TYPE suma = 0; \
    for (i = 0; i < Xr; ++i) { \
        for (j = 0; j < Xc; ++j) { \
            TYPE v = *(TYPE*)X_(i, j); \
            suma += v * v; \
        } \
    } \
    *(TYPE*)pR = TYPE##sqrt(suma); \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.rank == 2 && R.rank == 2); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0] \
    ; \
    ks_cint \
        csX = X.strides[1] \
    ; \
    ks_cint i, j; \
    TYPE##r suma = 0; \
    for (i = 0; i < Xr; ++i) { \
        for (j = 0; j < Xc; ++j) { \
            TYPE v = *(TYPE*)X_(i, j); \
            suma += v.re * v.re + v.im * v.im; \
        } \
    } \
    *(TYPE##r*)pR = TYPE##rsqrt(suma); \
    return 0; \
}

NXT_PASTE_I(LOOPI);
NXT_PASTE_F(LOOPF);
NXT_PASTE_C(LOOPC);

bool nxla_norm_fro(nx_t X, nx_t R) {
    if (R.rank != X.rank - 2) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': X.rank=%i, R.rank=%i (expect R.rank==X.rank-2)", K_NAME, X.rank, R.rank);
        return NULL;
    }
    nx_t cX;
    void *fX = NULL;
    if (X.dtype->kind == NX_DTYPE_COMPLEX) {
        if (!(
            (X.dtype == nxd_cH && R.dtype == nxd_H) ||
            (X.dtype == nxd_cF && R.dtype == nxd_F) ||
            (X.dtype == nxd_cD && R.dtype == nxd_D) ||
            (X.dtype == nxd_cL && R.dtype == nxd_L) ||
            (X.dtype == nxd_cQ && R.dtype == nxd_Q)
        )) {
            KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
            return false;
        }
        cX = X;
    } else {
        if (!nx_getcast(X, R.dtype, &cX, &fX)) {
            return false;
        }
    }

    /* Pad with extra dimensions */
    R = nx_with_newaxis(R, R.rank);
    R = nx_with_newaxis(R, R.rank);

    #define LOOP(NAME) do { \
        bool res = !nx_apply_Nd(kern_##NAME, 2, (nx_t[]){ cX, R }, 2, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(X.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
