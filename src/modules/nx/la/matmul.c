/* matmul.c - 'matmul' kernel
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

#define KERN_FUNC(_name) NXK_PASTE(kern_, _name)


struct extra_data {
    
    /* Size of the transform */
    int M, N, K;

    /* Strides for the inputs */
    ks_ssize_t srX, scX;
    ks_ssize_t srY, scY;
    ks_ssize_t srR, scR;

};


#define NXK_DO_I
#define NXK_DO_F
#define NXK_DO_C
#define NXK_FILE "matmul.kern"
#define K_NAME "matmul"
#include <ks/nxk.h>

bool nxla_matmul(nx_t X, nx_t Y, nx_t R) {
    if (X.rank < 2 || Y.rank < 2 || R.rank < 2) {
        KS_THROW(kst_SizeError, "Unsupported ranks for kernel '%s': %i, %i, %i (expected matrices to have rank >= 2)", K_NAME, X.rank, Y.rank, R.rank);
        return false;
    }

    if (X.shape[X.rank - 1] != Y.shape[Y.rank - 2] || X.shape[X.rank - 2] != R.shape[R.rank - 2] || Y.shape[Y.rank - 1] != R.shape[R.rank - 1]) {
        KS_THROW(kst_SizeError, "Unsupported sizes for kernel '%s': Did not follow pattern X:(..., M, N), Y(..., N, K) -> R:(..., M, K)", K_NAME);
        return false;
    }

    /* Since matmul is a weird operation, we have to store strides and sizes within 'extra_data'
     *
     */
    struct extra_data ed;

    ed.M = X.shape[X.rank - 2];
    ed.N = X.shape[X.rank - 1];
    ed.K = Y.shape[Y.rank - 1];

    ed.srX = X.strides[X.rank - 2];
    ed.scX = X.strides[X.rank - 1];
    ed.srY = Y.strides[Y.rank - 2];
    ed.scY = Y.strides[Y.rank - 1];
    ed.srR = R.strides[R.rank - 2];
    ed.scR = R.strides[R.rank - 1];

    /* Lower to the root of each matrix */
    nx_t 
        lX = nx_make(X.data, X.dtype, X.rank - 2, X.shape, X.strides), 
        lY = nx_make(Y.data, Y.dtype, Y.rank - 2, Y.shape, Y.strides),
        lR = nx_make(R.data, R.dtype, R.rank - 2, R.shape, R.strides)
    ;
    
    if (false) {}
    #define LOOP(TYPE, NAME) else if (R.dtype == nxd_##NAME) { \
        return !nx_apply_Nd(KERN_FUNC(NAME), 3, (nx_t[]){ lX, lY, lR }, 2, R.dtype, &ed); \
    }
    NXT_PASTE_IFC(LOOP)
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R", K_NAME, X.dtype, Y.dtype, R.dtype);
    return false;
}
