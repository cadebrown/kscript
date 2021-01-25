/* nx/la/main.c - 'nx.la' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define M_NAME "nx.la"

/* C-API */


static KS_TFUNC(M, norm) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;
    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 2) {
        KS_THROW(kst_SizeError, "Matrix is expected to be at least 2-D");
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = vX.dtype;
        /* Complex arguments should create real ones for abs */
        if (dtype == nxd_cH) {
            dtype = nxd_H;
        } else if (dtype == nxd_cF) {
            dtype = nxd_F;
        } else if (dtype == nxd_cD) {
            dtype = nxd_D;
        } else if (dtype == nxd_cL) {
            dtype = nxd_L;
        } else if (dtype == nxd_cQ) {
            dtype = nxd_Q;
        }

        r = (kso)nx_array_newc(nxt_array, NULL, dtype, vX.rank - 2, vX.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxla_norm_fro(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}


static KS_TFUNC(M, matmul) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nx_t vX, vY, vR;
    kso rX, rY, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 2) {
        KS_THROW(kst_SizeError, "Matrix is expected to be at least 2-D");
        KS_NDECREF(rX);
        return NULL;
    }

    if (!nx_get(y, NULL, &vY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }
    if (vY.rank < 2) {
        KS_THROW(kst_SizeError, "Matrix is expected to be at least 2-D");
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = nx_cast2(vX.dtype, vY.dtype);
        if (!dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        nx_t shape = nx_make_bcast(2, (nx_t[]) { vX, vY });
        if (shape.rank < 0) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        shape.shape[shape.rank - 2] = vX.shape[shape.rank - 2];
        shape.shape[shape.rank - 1] = vY.shape[shape.rank - 1];

        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }
    } else {
        KS_INCREF(r);

    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxla_matmul(vX, vY, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, matpow) {
    kso x, r = KSO_NONE;
    ks_cint n;
    KS_ARGS("x n:cint ?r", &x, &n, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = vX.dtype;

        r = (kso)nx_array_newc(nxt_array, NULL, dtype, vX.rank, vX.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxla_matpowi(vX, n, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, perm) {
    kso p, r = KSO_NONE;
    KS_ARGS("p ?r", &p, &r);

    nx_t vP, vR;
    kso rP, rR;

    if (!nx_get(p, NULL, &vP, &rP)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = nxd_bl;

        nx_t shape = nx_with_newaxis(vP, vP.rank - 1);
        shape.shape[vP.rank - 1] = shape.shape[vP.rank];
        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rP);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rP);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_onehot(vP, vR)) {
        KS_NDECREF(rP);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rP);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, diag) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = vX.dtype;

        nx_t shape = nx_with_newaxis(vX, vX.rank - 1);
        shape.shape[vX.rank - 1] = shape.shape[vX.rank];
        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }

    if (!nxla_diag(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}


static KS_TFUNC(M, lu) {
    kso x, p = KSO_NONE, l = KSO_NONE, u = KSO_NONE;
    KS_ARGS("x ?p ?l ?u", &x, &p, &l, &u);

    nx_t vX, vP, vL, vU;
    kso rX, rP, rL, rU;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 2) {
        KS_THROW(kst_SizeError, "Matrix is expected to be at least 2-D");
        KS_NDECREF(rX);
        return NULL;
    }

    if (p == KSO_NONE) {
        nx_t psh = vX;
        psh.rank = vX.rank - 1;
        p = (kso)nx_array_newc(nxt_array, NULL, nxd_idx, psh.rank, psh.shape, NULL);

        nx_dtype dtype = vX.dtype;
        l = (kso)nx_array_newc(nxt_array, NULL, dtype, vX.rank, vX.shape, NULL);
        u = (kso)nx_array_newc(nxt_array, NULL, dtype, vX.rank, vX.shape, NULL);

    } else {
        KS_INCREF(l);
        KS_INCREF(u);
        KS_INCREF(p);
    }
   if (!nx_get(p, NULL, &vP, &rP)) {
        KS_NDECREF(rX);
        KS_DECREF(l);
        KS_DECREF(u);
        KS_DECREF(p);
        return NULL;
    }
    if (!nx_get(l, NULL, &vL, &rL)) {
        KS_NDECREF(rX);
        KS_NDECREF(rP);
        KS_DECREF(l);
        KS_DECREF(u);
        return NULL;
    }

    if (!nx_get(u, NULL, &vU, &rU)) {
        KS_NDECREF(rX);
        KS_NDECREF(rP);
        KS_NDECREF(rL);
        KS_DECREF(l);
        KS_DECREF(u);
        return NULL;
    }

    if (!nxla_lu(vX, vP, vL, vU)) {
        KS_NDECREF(rX);
        KS_NDECREF(rP);
        KS_NDECREF(rL);
        KS_NDECREF(rU);
        return NULL;
    }

    KS_NDECREF(rP);
    KS_NDECREF(rX);
    KS_NDECREF(rL);
    KS_NDECREF(rU);
    return (kso)ks_tuple_newn(3, (kso[]) {
        p,
        l,
        u
    });
}


/* Export */

ks_module _ksi_nx_la() {

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Functions */
        {"norm",                   ksf_wrap(M_norm_, M_NAME ".norm(x, r=none)", "Creates a matrix norm")},

        {"diag",                   ksf_wrap(M_diag_, M_NAME ".diag(x, r=none)", "Creates a matrix with 'x' as the diagonal")},
        {"perm",                   ksf_wrap(M_perm_, M_NAME ".perm(p, r=none)", "Creates a permutation matrix with 'p' as the row changes")},

        {"matmul",                 ksf_wrap(M_matmul_, M_NAME ".matmul(x, y, r=none)", "Computes matrix multiplication")},
        {"matpow",                 ksf_wrap(M_matpow_, M_NAME ".matpow(x, n, r=none)", "Computes matrix power")},

        {"lu",                     ksf_wrap(M_lu_, M_NAME ".lu(x, p=none, l=none, r=none)", "Computes LU factorization with permutation indices, returns (P, L, U) such that 'x == nx.la.perm(P) @ L @ U")},

    ));

    return res;
}
