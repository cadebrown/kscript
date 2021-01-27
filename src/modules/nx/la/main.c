/* nx/la/main.c - 'nx.la' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define M_NAME "nx.la"

/* C-API */

/* Module Functions */

/* Templates */


/* Matrix/vector norm function */
#define T_A1_n(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    KS_ARGS("x ?r", &ax, &ar); \
    nx_t x, r; \
    kso xr, rr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (x.rank < 2) { \
        KS_THROW(kst_SizeError, "Expected matrix to have rank >= 2"); \
        KS_NDECREF(xr); \
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
        ar = (kso)nx_array_newc(nxt_array, NULL, dtype, rank - 2, shape, NULL); \
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
    if (!nxla_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}



/* Vector to matrix function (expanding operation, i.e. diag) */
#define T_A1_v2m(_name) static KS_TFUNC(M, _name) { \
    kso ax, ar = KSO_NONE; \
    KS_ARGS("x ?r", &ax, &ar); \
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
        shape[rank] = shape[rank - 1]; \
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
    if (!nxla_##_name(x, r)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(rr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(rr); \
    return ar; \
}


/* Matrix-Matrix function (i.e. matmul) */
#define T_A2_mm(_name) static KS_TFUNC(M, _name) { \
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
    if (x.rank < 2 || y.rank < 2) { \
        KS_THROW(kst_SizeError, "Expected matrix to have rank >= 1"); \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
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
        nx_t lx = nx_make(NULL, NULL, x.rank - 2, x.shape, x.strides); \
        nx_t ly = nx_make(NULL, NULL, y.rank - 2, y.shape, y.strides); \
        if (!nx_getbc(2, (nx_t[]) { lx, ly }, &rank, shape)) { \
            KS_NDECREF(xr); \
            KS_NDECREF(yr); \
            return NULL; \
        } \
        shape[rank++] = x.shape[x.rank - 2]; \
        shape[rank++] = y.shape[y.rank - 1]; \
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
    if (!nxla_##_name(x, y, r)) { \
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


T_A1_n(norm_fro)

T_A1_v2m(diag)
T_A1_v2m(perm)

T_A2_mm(matmul)

/* Export */

ks_module _ksi_nx_la() {

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Functions */
        {"norm",                   ksf_wrap(M_norm_fro_, M_NAME ".norm_fro(x, r=none)", "Calculates the Frobenius norm of the matrix")},

        {"diag",                   ksf_wrap(M_diag_, M_NAME ".diag(x, r=none)", "Creates a matrix with 'x' as the diagonal")},
        {"perm",                   ksf_wrap(M_perm_, M_NAME ".perm(p, r=none)", "Creates a permutation matrix with 'p' as the row changes")},
        {"matmul",                 ksf_wrap(M_matmul_, M_NAME ".matmul(x, y, r=none)", "Computes matrix multiplication")},

/*


        {"matpow",                 ksf_wrap(M_matpow_, M_NAME ".matpow(x, n, r=none)", "Computes matrix power")},

        {"lu",                     ksf_wrap(M_lu_, M_NAME ".lu(x, p=none, l=none, r=none)", "Computes LU factorization with permutation indices, returns (P, L, U) such that 'x == nx.la.perm(P) @ L @ U")},

*/
    ));

    return res;
}
