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

nx_dtype nx_calc_numcast(nx_dtype da, nx_dtype db) {
    if (da->kind == NX_DTYPE_KIND_CCOMPLEX|| db->kind == NX_DTYPE_KIND_CCOMPLEX) {
        if (da->kind == NX_DTYPE_KIND_CCOMPLEX && db->kind == NX_DTYPE_KIND_CCOMPLEX) {
            if (da->size > db->size) return (nx_dtype)KS_NEWREF(da);
            else return (nx_dtype)KS_NEWREF(db);
        }
        if (da->kind == NX_DTYPE_KIND_CCOMPLEX && NX_DTYPE_ISARITH(db)) return (nx_dtype)KS_NEWREF(da);
        if (db->kind == NX_DTYPE_KIND_CCOMPLEX && NX_DTYPE_ISARITH(da)) return (nx_dtype)KS_NEWREF(db);
    } else if (da->kind == NX_DTYPE_KIND_CFLOAT || db->kind == NX_DTYPE_KIND_CFLOAT) {
        if (da->kind == NX_DTYPE_KIND_CFLOAT && db->kind == NX_DTYPE_KIND_CFLOAT) {
            if (da->size > db->size) return (nx_dtype)KS_NEWREF(da);
            else return (nx_dtype)KS_NEWREF(db);
        }
        if (da->kind == NX_DTYPE_KIND_CFLOAT && NX_DTYPE_ISARITH(db)) return (nx_dtype)KS_NEWREF(da);
        if (db->kind == NX_DTYPE_KIND_CFLOAT && NX_DTYPE_ISARITH(da)) return (nx_dtype)KS_NEWREF(db);
    } else if (da->kind == NX_DTYPE_KIND_CINT && db->kind == NX_DTYPE_KIND_CINT) {
        bool sgn = da->s_cint.sgn || db->s_cint.sgn;
        int bits = da->s_cint.bits;
        if (db->s_cint.bits > bits) bits = db->s_cint.bits;
        return nx_dtype_get_cint(bits, sgn);
    }

    KS_THROW(kst_TypeError, "Failed to calculate numeric type for result of %R and %R", da, db);
    return NULL;
}


/* Module Functions */

static KS_FUNC(add) {
    kso x, y, z = KSO_NONE;
    KS_ARGS("x y ?z", &x, &y, &z);

    nxar_t ax, ay, az;
    kso rx, ry, rz;

    if (!nxar_get(x, NULL, &ax, &rx)) {
        return NULL;
    }

    if (!nxar_get(y, NULL, &ay, &ry)) {
        KS_NDECREF(rx);
        return NULL;
    }

    if (z == KSO_NONE) {
        /* Generate output */
        int rankz;
        ks_size_t* dimz = nx_calc_bcast(2, (nxar_t[]){ ax, ay }, &rankz);
        if (!dimz) {
            KS_NDECREF(rx);
            KS_NDECREF(ry);
            return NULL;
        }

        z = (kso)nx_array_newc(nxt_array, ax.dtype, rankz, dimz, NULL, NULL);
        KS_DECREF(dimz);
    } else {
        KS_INCREF(z);
    }

    if (!nxar_get(z, NULL, &az, &rz)) {
        KS_NDECREF(rx);
        KS_NDECREF(ry);
        KS_DECREF(z);
        return NULL;
    }

    if (!nx_add(az, ax, ay)) {
        KS_NDECREF(rx);
        KS_NDECREF(ry);
        KS_NDECREF(rz);
        KS_DECREF(z);
        return NULL;
    }

    KS_NDECREF(rx);
    KS_NDECREF(ry);
    KS_NDECREF(rz);
    
    
    return z;
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
        {"add",                    ksf_wrap(add_, M_NAME ".add(x, y, z=none)", "Computes elementwise addition")},

    ));


    


    return res;
}
