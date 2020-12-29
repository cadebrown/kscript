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


/* Export */

ks_module _ksi_nx() {
    _ksi_nx_dtype();
    _ksi_nx_view();
    _ksi_nx_array();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Datatypes */
        {"float32",                (kso)(nx_float32 = nx_dtype_get_cfloat(32))},
        {"float64",                (kso)(nx_float64 = nx_dtype_get_cfloat(64))},
        {"complex32",              (kso)(nx_complex32 = nx_dtype_get_ccomplex(32))},
        {"complex64",              (kso)(nx_complex64 = nx_dtype_get_ccomplex(64))},

        /* Aliases */
        {"float",                  KS_NEWREF(nxd_float = nx_float32)},
        {"double",                 KS_NEWREF(nxd_double = nx_float64)},
        {"complexfloat",           KS_NEWREF(nxd_complexfloat = nx_complex32)},
        {"complexdouble",          KS_NEWREF(nxd_complexdouble = nx_complex64)},

        /* Types */

        {"dtype",                  KS_NEWREF(nxt_dtype)},
        
        {"array",                  KS_NEWREF(nxt_array)},
        {"view",                   KS_NEWREF(nxt_view)},
    

        /* Functions */
    
    ));


    


    return res;
}
