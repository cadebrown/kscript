/* nx/main.c - 'nx' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define M_NAME "nx"

/* C-API */


ks_size_t* nx_calc_bcast(int N, nxar_t* inp, int* orank) {
    int i, j;

    int max_rank = inp[0].rank;
    for (i = 1; i < N; ++i) {
        if (inp[i].rank > max_rank) max_rank = inp[i].rank;
    }

    ks_size_t* res = ks_zmalloc(sizeof(*res), *orank = max_rank);

    /* Negative index (from the end of each dimensions array) */
    int ni = -1;

    while (true) {
        /* The size (other than 1) required for the current dimension, or -1 if not found yet */
        ks_ssize_t req_size = -1;
        bool had_dim = false;

        for (i = 0; i < N; ++i) {
            if (inp[i].rank < -ni) {
                /* Since we are right-aligned, we are out of bounds. We will interpret the size
                 *   as 1
                 */
                continue;
            } else {
                /* Get dimension (from the right) */
                ks_size_t dim = inp[i].dims[inp[i].rank + ni];
                had_dim = true;
                if (dim == 1) {
                    /* Single dimension, which can always be broadcasted. So it is valid */
                    continue;
                } else if (req_size < 0) {
                    /* No size requirement, so set it */
                    req_size = dim;
                    continue;
                } else if (dim == req_size) {
                    /* Fits the requirement */
                    continue;
                } else {
                    /* Bad size */
                    ks_free(res);
                    ksio_StringIO sio = ksio_StringIO_new();
                    ksio_add(sio, "Shapes were not broadcastable: ");
                    for (i = 0; i < N; ++i) {
                        if (i > 0) ksio_add(sio, ", ");
                        ksio_add(sio, "(");
                        for (j = 0; j < inp[i].rank; ++j) {
                            if (j > 0) ksio_add(sio, ", ");
                            ksio_add(sio, "%u", (ks_uint)inp[i].dims[j]);
                        }
                        ksio_add(sio, ")");
                    }

                    ks_str rs = ksio_StringIO_getf(sio);
                    KS_THROW(kst_SizeError, "%S", rs);
                    KS_DECREF(rs);
                    return NULL;
                }
            }
        }
        /* Exhausted the right-hand side */
        if (!had_dim) break;

        /* Set the requirement and continue */
        res[max_rank + ni] = req_size < 0 ? 1 : req_size;
        ni--;
    }

    return res;
}

nx_dtype nx_calc_numcast(nx_dtype dR, nx_dtype dX) {
    if (dR->kind == NX_DTYPE_KIND_CCOMPLEX|| dX->kind == NX_DTYPE_KIND_CCOMPLEX) {
        if (dR->kind == NX_DTYPE_KIND_CCOMPLEX && dX->kind == NX_DTYPE_KIND_CCOMPLEX) {
            if (dR->size > dX->size) return (nx_dtype)KS_NEWREF(dR);
            else return (nx_dtype)KS_NEWREF(dX);
        }
        if (dR->kind == NX_DTYPE_KIND_CCOMPLEX && NX_DTYPE_ISARITH(dX)) return (nx_dtype)KS_NEWREF(dR);
        if (dX->kind == NX_DTYPE_KIND_CCOMPLEX && NX_DTYPE_ISARITH(dR)) return (nx_dtype)KS_NEWREF(dX);
    } else if (dR->kind == NX_DTYPE_KIND_CFLOAT || dX->kind == NX_DTYPE_KIND_CFLOAT) {
        if (dR->kind == NX_DTYPE_KIND_CFLOAT && dX->kind == NX_DTYPE_KIND_CFLOAT) {
            if (dR->size > dX->size) return (nx_dtype)KS_NEWREF(dR);
            else return (nx_dtype)KS_NEWREF(dX);
        }
        if (dR->kind == NX_DTYPE_KIND_CFLOAT && NX_DTYPE_ISARITH(dX)) return (nx_dtype)KS_NEWREF(dR);
        if (dX->kind == NX_DTYPE_KIND_CFLOAT && NX_DTYPE_ISARITH(dR)) return (nx_dtype)KS_NEWREF(dX);
    } else if (dR->kind == NX_DTYPE_KIND_CINT && dX->kind == NX_DTYPE_KIND_CINT) {
        if (dR->size > dX->size) {
            KS_INCREF(dR);
            return dR;
        } else {
            KS_INCREF(dX);
            return dX;
        }
    }

    KS_THROW(kst_TypeError, "Failed to calculate numeric type for result of %R and %R", dR, dX);
    return NULL;
}


/* Module Functions */

static KS_FUNC(cast) {
    kso x;
    nx_dtype to;
    KS_ARGS("x to:*", &x, &to, nxt_dtype);

    /* Get as array descriptor */
    nxar_t aX;
    kso rX;
    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }

    /* Calculate cast */
    nxar_t aR;
    kso rR;
    if (!nx_getcast(aX, to, &aR, &rR)) {
        KS_NDECREF(rX);
        return NULL;
    }

    nx_array r = nx_array_newc(nxt_array, aR.dtype, aR.rank, aR.dims, aR.strides, aR.data);

    KS_NDECREF(rX);
    KS_NDECREF(rR);

    return (kso)r;
}

static KS_FUNC(add) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nxar_t aX, aY, aR;
    kso rX, rY, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (!nxar_get(y, NULL, &aY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = nx_calc_numcast(aX.dtype, aY.dtype);
        if (!r_dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        int r_rank;
        ks_size_t* r_dims = nx_calc_bcast(2, (nxar_t[]){ aX, aY }, &r_rank);
        if (!r_dims) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_DECREF(r_dtype);
            return NULL;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, r_rank, r_dims, NULL, NULL);
        ks_free(r_dims);
        KS_DECREF(r_dtype);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_add(aR, aX, aY)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rR);

    return r;
}

static KS_FUNC(sub) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nxar_t aX, aY, aR;
    kso rX, rY, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (!nxar_get(y, NULL, &aY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = nx_calc_numcast(aX.dtype, aY.dtype);
        if (!r_dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        int r_rank;
        ks_size_t* r_dims = nx_calc_bcast(2, (nxar_t[]){ aX, aY }, &r_rank);
        if (!r_dims) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_DECREF(r_dtype);
            return NULL;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, r_rank, r_dims, NULL, NULL);
        ks_free(r_dims);
        KS_DECREF(r_dtype);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_sub(aR, aX, aY)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rR);

    return r;
}


static KS_FUNC(mul) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nxar_t aX, aY, aR;
    kso rX, rY, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (!nxar_get(y, NULL, &aY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = nx_calc_numcast(aX.dtype, aY.dtype);
        if (!r_dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        int r_rank;
        ks_size_t* r_dims = nx_calc_bcast(2, (nxar_t[]){ aX, aY }, &r_rank);
        if (!r_dims) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_DECREF(r_dtype);
            return NULL;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, r_rank, r_dims, NULL, NULL);
        ks_free(r_dims);
        KS_DECREF(r_dtype);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_mul(aR, aX, aY)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rR);

    return r;
}

static KS_FUNC(mod) {
    kso x, y, r = KSO_NONE;
    KS_ARGS("x y ?r", &x, &y, &r);

    nxar_t aX, aY, aR;
    kso rX, rY, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (!nxar_get(y, NULL, &aY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = nx_calc_numcast(aX.dtype, aY.dtype);
        if (!r_dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            return NULL;
        }

        int r_rank;
        ks_size_t* r_dims = nx_calc_bcast(2, (nxar_t[]){ aX, aY }, &r_rank);
        if (!r_dims) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_DECREF(r_dtype);
            return NULL;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, r_rank, r_dims, NULL, NULL);
        ks_free(r_dims);
        KS_DECREF(r_dtype);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_mod(aR, aX, aY)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rR);

    return r;
}

static KS_FUNC(sqrt) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nxar_t aX, aR;
    kso rX, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = aX.dtype;
        if (r_dtype->kind == NX_DTYPE_KIND_CINT) {
            r_dtype = nxd_double;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, aX.rank, aX.dims, NULL, NULL);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_sqrt(aR, aX)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);

    return r;
}


static KS_FUNC(sin) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nxar_t aX, aR;
    kso rX, rR;

    if (!nxar_get(x, NULL, &aX, &rX)) {
        return NULL;
    }
    if (r == KSO_NONE) {
        /* Generate output array */
        nx_dtype r_dtype = aX.dtype;
        if (r_dtype->kind == NX_DTYPE_KIND_CINT) {
            r_dtype = nxd_double;
        }

        r = (kso)nx_array_newc(nxt_array, r_dtype, aX.rank, aX.dims, NULL, NULL);
    } else {
        KS_INCREF(r);
    }

    if (!nxar_get(r, NULL, &aR, &rR)) {
        KS_NDECREF(rX);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_sin(aR, aX)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_NDECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);

    return r;
}

/* Export */

ks_module _ksi_nx() {
    _ksi_nx_dtype();
    _ksi_nx_view();
    _ksi_nx_array();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Submodules */
        {"rand",                   (kso)_ksi_nxrand()},
        
        /* Types */

        {"dtype",                  KS_NEWREF(nxt_dtype)},
        
        {"array",                  KS_NEWREF(nxt_array)},
        {"view",                   KS_NEWREF(nxt_view)},
    
        /* Datatypes */
        {"float",                  (kso)nxd_float},
        {"double",                 (kso)nxd_double},
        {"longdouble",             (kso)nxd_longdouble},
        {"float128",               (kso)nxd_float128},
        {"complexfloat",           (kso)nxd_complexfloat},
        {"complexdouble",          (kso)nxd_complexdouble},
        {"complexlongdouble",      (kso)nxd_complexlongdouble},
        {"complexfloat128",        (kso)nxd_complexfloat128},

        /* Functions */
        {"cast",                   ksf_wrap(cast_, M_NAME ".cast(dtype, obj)", "Casts to a datatype")},
        {"add",                    ksf_wrap(add_, M_NAME ".add(x, y, r=none)", "Computes elementwise addition")},
        {"sub",                    ksf_wrap(sub_, M_NAME ".sub(x, y, r=none)", "Computes elementwise subtraction")},
        {"mul",                    ksf_wrap(mul_, M_NAME ".mul(x, y, r=none)", "Computes elementwise multiplication")},
        {"mod",                    ksf_wrap(mod_, M_NAME ".mod(x, y, r=none)", "Computes elementwise modulo")},

        {"sqrt",                   ksf_wrap(sqrt_, M_NAME ".sqrt(x, y, r=none)", "Computes elementwise square root")},

        {"sin",                    ksf_wrap(sin_, M_NAME ".sin(x, r=none)", "Computes elementwise sine")},

    ));

    return res;
}
