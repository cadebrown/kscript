/* nx/la/main.c - 'nx.la' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define M_NAME "nx.la"

/* C-API */

static KS_TFUNC(M, matmul) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nx_t vX, vY, vR;
    kso rX, rY, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (!nx_get(y, NULL, &vY, &rY)) {
        KS_NDECREF(rX);
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

/* Export */

ks_module _ksi_nx_la() {

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Functions */
        {"diag",                   ksf_wrap(M_diag_, M_NAME ".diag(x, r=none)", "Creates a matrix with 'x' as the diagonal")},

        {"matmul",                 ksf_wrap(M_matmul_, M_NAME ".matmul(x, y, r=none)", "Computes matrix multiplication")},
        {"matpow",                 ksf_wrap(M_matpow_, M_NAME ".matpow(x, n, r=none)", "Computes matrix power")},

    ));

    return res;
}
