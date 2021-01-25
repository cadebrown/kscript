/* onehot.c - 'onehot' kernel
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "onehot"

/* Access Macros */
#define X_(_i, _j) (pX + rsX * (_i) + csX * (_j))
#define R_(_i, _j) (pR + rsR * (_i) + csR * (_j))

#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.rank == 2 && R.rank == 2); \
    assert(X.dtype == nxd_idx); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    int Rr = R.shape[0], Rc = R.shape[1]; \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0], \
        rsR = R.strides[0]  \
    ; \
    ks_cint \
        csX = X.strides[1], \
        csR = R.strides[1]  \
    ; \
    ks_cint i; \
    for (i = 0; i < Xr; ++i) { \
        nx_idx ii = *(nx_idx*)X_(i, 0) % Rc; \
        if (ii < 0) ii += Rc; \
        if (ii >= 0 && ii < Rc) { \
            *(TYPE*)R_(i, ii) = 1; \
        } \
    } \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 2); \
    nx_t X = args[0], R = args[1]; \
    assert(X.rank == 2 && R.rank == 2); \
    assert(X.dtype == nxd_idx); \
    int Xr = X.shape[0], Xc = X.shape[1]; \
    int Rr = R.shape[0], Rc = R.shape[1]; \
    ks_uint \
        pX = (ks_uint)X.data, \
        pR = (ks_uint)R.data  \
    ; \
    ks_cint \
        rsX = X.strides[0], \
        rsR = R.strides[0]  \
    ; \
    ks_cint \
        csX = X.strides[1], \
        csR = R.strides[1]  \
    ; \
    ks_cint i; \
    for (i = 0; i < Xc; ++i) { \
        nx_idx ii = *(nx_idx*)X_(0, i) % Rc; \
        if (ii < 0) ii += Rc; \
        if (ii >= 0 && ii < Rc) { \
            ((TYPE*)R_(i, ii))->re = 1; \
            ((TYPE*)R_(i, ii))->im = 0; \
        } \
    } \
    return 0; \
}


NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nx_onehot(nx_t X, nx_t R) {
    if (!nx_zero(R)) {
        return false;
    }

/* TODO: introduce a cast for doubles?
    if (X.dtype->kind != NX_DTYPE_INT) {
        KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
        return false;
    }
*/
    nx_t cX;
    void *fX = NULL;
    if (!nx_getcast(X, nxd_idx, &cX, &fX)) {
        return false;
    }

    /* Now, we need to insert an index just before the last */
    cX = nx_with_newaxis(cX, cX.rank);

    #define LOOP(NAME) do { \
        bool res = !nx_apply_Nd(kern_##NAME, 2, (nx_t[]){ cX, R }, 2, NULL); \
        ks_free(fX); \
        return res; \
    } while (0);

    NXT_FOR_ALL(R.dtype, LOOP);
    #undef LOOP

    ks_free(fX);

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R", K_NAME, X.dtype, R.dtype);
    return false;
}
