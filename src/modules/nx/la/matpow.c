/* matpow.c - 'matpow' kernel
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "matpow"

bool nxla_matpowi(nx_t X, int n, nx_t R) {
    if (X.rank < 2) {
        KS_THROW(kst_SizeError, "Expected input to 'matpowi' to be at least 2-D");
        return false;
    }

    /* Handle special inputs */
    if (n < 0) {
        KS_THROW(kst_Error, "TODO: Implement negative matrix powers");
        return false;
    } else if (n == 0) {
        KS_THROW(kst_Error, "TODO: Implement zero matrix powers");
        return false;
    } else if (n == 1) {
        return nx_cast(X, R);
    } else if (n == 2) {
        return nxla_matmul(X, X, R);
    }

    /* Otherwise we'll need some temporaries and just repeated-squarings*/

    /* Allocate two other buffers that can hold the matrix */
    ks_size_t tsz = nx_szprod(X.rank, X.shape) * X.dtype->size;
    void *tA = ks_malloc(tsz);
    if (!tA) {
        KS_THROW(kst_InternalError, "Failed to alloc memory in '%s'!", K_NAME);
        return false;
    }

    void *tB = ks_malloc(tsz);
    if (!tB) {
        ks_free(tB);
        KS_THROW(kst_InternalError, "Failed to alloc memory in '%s'!", K_NAME);
        return false;
    }
    void *tC = ks_malloc(tsz);
    if (!tC) {
        ks_free(tA);
        ks_free(tB);
        KS_THROW(kst_InternalError, "Failed to alloc memory in '%s'!", K_NAME);
        return false;
    }

    /* Current target flag, signalling where to store results:
     *   0: Uninitialized
     *  >0: Current result is stored in 'R'
     *  <0: Current result is stored in 'tmp'
     */
    int curf = 0;

    /* X ** 2 ** iter */
    nx_t X_2_i = nx_make(tA, X.dtype, X.rank, X.shape, NULL);
    nx_t tmp = nx_make(tB, X.dtype, X.rank, X.shape, NULL);
    nx_t tmp2 = nx_make(tC, X.dtype, X.rank, X.shape, NULL);

    if (!nx_cast(X, X_2_i)) {
        ks_free(tA);
        ks_free(tB);
        ks_free(tC);
        return false;
    }
    int it = 0;
    while (true) {
        if (n & 1) {
            /* This bit is hot, so multiply the result * X ** 2 ** iter */
            if (curf == 0) {
                /* First time, so we can copy to 'R' */
                if (!nx_cast(X_2_i, R)) {
                    ks_free(tA);
                    ks_free(tB);
                    ks_free(tC);
                    return false;
                }
                curf = 1;
            } else if (curf > 0) {
                /* Stored in 'R' */
                if (!nxla_matmul(X_2_i, R, tmp)) {
                    ks_free(tA);
                    ks_free(tB);
                    ks_free(tC);
                    return false;
                }
                curf = -1;
            } else {
                /* Stored in 'tmp' */
                if (!nxla_matmul(X_2_i, tmp, R)) {
                    ks_free(tA);
                    ks_free(tB);
                    ks_free(tC);
                    return false;
                }
                curf = 1;
            }
        }
        it++;

        /* Shift exponent */
        n >>= 1;
        if (!n) break;

        /* Square the matrix again */
        if (!nxla_matmul(X_2_i, X_2_i, tmp2)) {
            ks_free(tA);
            ks_free(tB);
            ks_free(tC);
            return false;
        }
        /* Swap buffers */
        void* t = X_2_i.data;
        X_2_i.data = tmp2.data;
        tmp2.data = t;
    }

    assert(curf != 0);
    if (curf < 0) {
        /* Load from 'tmp' into 'R' */
        if (!nx_cast(tmp, R)) {
            ks_free(tA);
            ks_free(tB);
            ks_free(tC);
            return false;
        }
    }

    ks_free(tA);
    ks_free(tB);
    ks_free(tC);

    return true;
}

