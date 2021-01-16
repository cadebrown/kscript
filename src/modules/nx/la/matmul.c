/* matmul.c - 'matmul' kernel
 *
 * Uses the last two axes in a double loop.
 * 
 * TODO: Detect BLAS and/or higher performance algorithms
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "matmul"

/* Access Macros */
#define X_(_i, _j) (pX + rsX * (_i) + csX * (_j))
#define Y_(_i, _j) (pY + rsY * (_i) + csY * (_j))
#define R_(_i, _j) (pR + rsR * (_i) + csR * (_j))

#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 3); \
    nx_t X = args[0], Y = args[1], R = args[2]; \
    assert(X.rank == 2 && Y.rank == 2 && R.rank == 2); \
    assert(X.dtype == Y.dtype && Y.dtype == R.dtype); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    int Yr = Y.shape[0], Yc = Y.shape[1]; \
    int Rr = R.shape[0], Rc = R.shape[1]; \
    int kM = Xr, kN = Xc, kK = Yc; \
    assert(Xc == Yr && Rr == Xr && Rc == Yc); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pY = (ks_uint)Y.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0], \
        rsY = Y.strides[0], \
        rsR = R.strides[0]  \
    ; \
    ks_cint \
        csX = X.strides[1], \
        csY = Y.strides[1], \
        csR = R.strides[1]  \
    ; \
    ks_cint i, j, k; \
    for (i = 0; i < kM; ++i) { \
        for (j = 0; j < kK; ++j) { \
            TYPE dp = 0; \
            for (k = 0; k < kN; ++k) { \
                dp += *(TYPE*)X_(i, k) * *(TYPE*)Y_(k, j); \
            } \
            *(TYPE*)R_(i, j) = dp; \
        } \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 3); \
    nx_t X = args[0], Y = args[1], R = args[2]; \
    assert(X.rank == 2 && Y.rank == 2 && R.rank == 2); \
    assert(X.dtype == Y.dtype && Y.dtype == R.dtype); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    int Yr = Y.shape[0], Yc = Y.shape[1]; \
    int Rr = R.shape[0], Rc = R.shape[1]; \
    int kM = Xr, kN = Xc, kK = Yc; \
    assert(Xc == Yr && Rr == Xr && Rc == Yc); \
    ks_uint \
        pX = (ks_uint)X.data, \
        pY = (ks_uint)Y.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0], \
        rsY = Y.strides[0], \
        rsR = R.strides[0]  \
    ; \
    ks_cint \
        csX = X.strides[1], \
        csY = Y.strides[1], \
        csR = R.strides[1]  \
    ; \
    ks_cint i, j, k; \
    for (i = 0; i < kM; ++i) { \
        for (j = 0; j < kK; ++j) { \
            TYPE dp = {0, 0}; \
            for (k = 0; k < kN; ++k) { \
                TYPE x = *(TYPE*)X_(i, k), y = *(TYPE*)Y_(k, j); \
                dp.re += x.re*y.re - x.im*y.im; \
                dp.im += x.re*y.im - x.im*y.re; \
            } \
            *(TYPE*)R_(i, j) = dp; \
        } \
    } \
    return 0; \
}

NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nxla_matmul(nx_t X, nx_t Y, nx_t R) {
    nx_t cX, cY;
    void *fX = NULL, *fY = NULL;
    if (!nx_getcast(X, R.dtype, &cX, &fX)) {
        return false;
    }
    if (!nx_getcast(Y, R.dtype, &cY, &fY)) {
        ks_free(fX);
        return false;
    }

    #define LOOP(NAME) do { \
        bool res = !nx_apply_Nd(kern_##NAME, 3, (nx_t[]){ cX, cY, R }, 2, NULL); \
        ks_free(fX); \
        ks_free(fY); \
        return res; \
    } while (0);

    NXT_PASTE_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);
    ks_free(fY);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype);
    return false;
}
