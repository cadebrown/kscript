/* lu.c - 'lu' kernel
 *
 * Factors a matrix into a permute/lower/upper matrix, specifically:
 * 
 *   X := nx.la.perm(P) @ L @ U
 * 
 * Where:
 *   P is a permutation matrix (vector of one-hot indicies)
 *   L is a lower triangular matrix
 *   U is an upper triangular matrix
 * 
 * All matrices are NxN
 * 
 * Initially, the kernel accepts (P, L, U) on entry with L containing 'X', and sets L, U, and P
 *   on exit
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define K_NAME "lu"

/* Access Macros */
#define L_(_i, _j) (pL + rsL * (_i) + csL * (_j))
#define U_(_i, _j) (pU + rsU * (_i) + csU * (_j))
#define P_(_i) (pP + sP * (_i))

#define LOOPR(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(N == 3); \
    nx_t P = args[0], L = args[1], U = args[2]; \
    assert(L.rank == 2 && U.rank == 2 && P.rank == 2); \
    assert(L.dtype == U.dtype); \
    assert(P.dtype == nxd_idx); \
    int kN = L.shape[0]; /* This works for NxN factorizations */ \
    ks_uint \
        pL = (ks_uint)L.data, \
        pU = (ks_uint)U.data, \
        pP = (ks_uint)P.data  \
    ; \
    ks_cint \
        sP = P.strides[1]  \
    ; \
    ks_cint \
        rsL = L.strides[0], \
        rsU = U.strides[0]  \
    ; \
    ks_cint \
        csL = L.strides[1], \
        csU = U.strides[1]  \
    ; \
    ks_cint i, j, k; \
    /* Initialize permutation vector */ \
    for (i = 0; i < kN; ++i) { \
        *(nx_idx*)P_(i) = i; \
    } \
    TYPE tol = 0; \
    for (j = 0; j < kN; ++j) { \
        ks_cint imax = j; \
        TYPE amax = 0; \
        for (i = j; i < kN; ++i) { \
            TYPE tm = *(TYPE*)L_(*(nx_idx*)P_(i), j); \
            if (tm < 0) tm = -tm; \
            if (tm > amax) { \
                amax = tm; \
                imax = i; \
            } \
        } \
        if (amax < tol) { \
            /* Degenerate matrix */ \
        } \
        if (imax != j) { \
            /* Exchange rows */ \
            ks_cint ti = *(nx_idx*)P_(j); \
            *(nx_idx*)P_(j) = *(nx_idx*)P_(imax); \
            *(nx_idx*)P_(imax) = ti; \
        } \
        ks_cint jj = *(nx_idx*)P_(j); \
        for (i = j + 1; i < kN; ++i) { \
            ks_cint ii = *(nx_idx*)P_(i); \
            *(TYPE*)L_(ii, j) /= *(TYPE*)L_(jj, j); \
            for (k = j + 1; k < kN; ++k) { \
                *(TYPE*)L_(ii, k) -= *(TYPE*)L_(ii, j) * *(TYPE*)L_(jj, k); \
            } \
        } \
    } \
    /* Now, undo permutation matrix by setting 'U = P**-1 @ L' */ \
    for (i = 0; i < kN; ++i) { \
        ks_cint ii = *(nx_idx*)P_(i); \
        for (j = 0; j < kN; ++j) { \
            *(TYPE*)U_(i, j) = *(TYPE*)L_(ii, j); \
        } \
    } \
    /* Now, transfer lower diagonals back to 'L' */ \
    for (i = 0; i < kN; ++i) { \
        for (j = 0; j < i; ++j) { \
            *(TYPE*)L_(i, j) = *(TYPE*)U_(i, j); \
            *(TYPE*)U_(i, j) = 0; \
        } \
        *(TYPE*)L_(i, i) = 1;\
        for (j = i + 1; j < kN; ++j) { \
            *(TYPE*)L_(i, j) = 0; \
        } \
    } \
    nx_idx* tmpP = ks_malloc(sizeof(*tmpP) * kN); \
    assert(tmpP != NULL); \
    /* Now, invert the permutation matrix onehot encoding */ \
    for (i = 0; i < kN; ++i) { \
        tmpP[*(nx_idx*)P_(i)] = i; \
    } \
    for (i = 0; i < kN; ++i) { \
        *(nx_idx*)P_(i) = tmpP[i]; \
    } \
    ks_free(tmpP); \
    return 0; \
}

#define LOOPC(TYPE, NAME) static int kern_##NAME(int N, nx_t* args, void* extra) { \
    assert(false); \
}

NXT_PASTE_I(LOOPR);
NXT_PASTE_F(LOOPR);

NXT_PASTE_C(LOOPC);

bool nxla_lu(nx_t X, nx_t P, nx_t L, nx_t U) {
    assert(L.dtype == U.dtype);
    if (P.dtype != nxd_idx) {
        KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R (%R was given when the index dtype %R was expected)", K_NAME, X.dtype, P.dtype, L.dtype, U.dtype, P.dtype, nxd_idx);
        return NULL;
    }

    if (!nx_zero(U)) return false;

    if (!nx_cast(X, L)) {
        return false;
    }

    /* Since P is a vector, we must add a rank */
    P = nx_with_newaxis(P, P.rank - 1);

    /* Initialize temporary with 'X' */

    #define LOOP(NAME) do { \
        bool res = !nx_apply_Nd(kern_##NAME, 3, (nx_t[]){ P, L, U }, 2, NULL); \
        return res; \
    } while (0);

    NXT_FOR_ALL(L.dtype, LOOP);
    #undef LOOP

    KS_THROW(kst_TypeError, "Unsupported types for kernel '%s': %R, %R, %R, %R", K_NAME, X.dtype, P.dtype, L.dtype, U.dtype);
    return false;
}
