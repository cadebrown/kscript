/* nx/main.c - 'nx' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>

#define M_NAME "nx"

/* C-API */

nx_t nx_make(void* data, nx_dtype dtype, int rank, ks_ssize_t* shape, ks_ssize_t* strides) {
    nx_t self;

    self.data = data;
    self.dtype = dtype;
    self.rank = rank;

    int i;
    for (i = 0; i < rank; ++i) {
        self.shape[i] = shape[i];
    }

    if (strides) {
        /* Copy strides */
        for (i = 0; i < rank; ++i) {
            self.strides[i] = strides[i];
        }
    } else if (dtype && rank > 0) {
        /* Calculate strides */
        self.strides[rank - 1] = dtype->size;
        for (i = rank - 2; i >= 0; --i) {
            self.strides[i] = self.strides[i + 1] * shape[i + 1];
        }
    }

    return self;
}

nx_t nx_make_bcast(int N, nx_t* args) {
    assert(N > 0);
    int i, j;

    int max_rank = args[0].rank;
    for (i = 1; i < N; ++i) {
        if (args[i].rank > max_rank) max_rank = args[i].rank;
    }

    nx_t self;
    self.rank = max_rank;

    /* Negative index (from the end of each dimensions array) */
    int ni = -1;

    while (true) {
        /* The size (other than 1) required for the current dimension, or -1 if not found yet */
        ks_ssize_t req_size = -1;
        bool had_dim = false;

        for (i = 0; i < N; ++i) {
            if (args[i].rank < -ni) {
                /* Since we are right-aligned, we are out of bounds. We will interpret the size
                 *   as 1
                 */
                continue;
            } else {
                /* Get dimension (from the right) */
                ks_size_t dim = args[i].shape[args[i].rank + ni];
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
                    ksio_StringIO sio = ksio_StringIO_new();
                    ksio_add(sio, "Shapes were not broadcastable: ");
                    for (i = 0; i < N; ++i) {
                        if (i > 0) ksio_add(sio, ", ");
                        ksio_add(sio, "(");
                        for (j = 0; j < args[i].rank; ++j) {
                            if (j > 0) ksio_add(sio, ", ");
                            ksio_add(sio, "%u", (ks_uint)args[i].shape[j]);
                        }
                        ksio_add(sio, ")");
                    }

                    ks_str rs = ksio_StringIO_getf(sio);
                    KS_THROW(kst_SizeError, "%S", rs);
                    KS_DECREF(rs);
                    self.rank = -1;
                    return self;
                }
            }
        }
        /* Exhausted the right-hand side */
        if (!had_dim) break;

        /* Set the requirement and continue */
        self.shape[max_rank + ni] = req_size < 0 ? 1 : req_size;
        ni--;
    }

    return self;
}

void* nx_szdot(void* data, int rank, ks_ssize_t* strides, ks_size_t* idxs) {
    ks_uint res = (ks_uint)data;
    int i;
    for (i = 0; i < rank; ++i) {
        res += strides[i] * idxs[i];
    }
    return (void*)res;
}


nx_dtype nx_cast2(nx_dtype X, nx_dtype Y) {
    if (X->kind == NX_DTYPE_COMPLEX || Y->kind == NX_DTYPE_COMPLEX) {
        if (X->kind == NX_DTYPE_COMPLEX && Y->kind == NX_DTYPE_COMPLEX) {
            return X->size > Y->size ? X : Y;
        } else if (X->kind == NX_DTYPE_COMPLEX && NX_ISNUM(Y)) {
            return X;
        } else if (Y->kind == NX_DTYPE_COMPLEX && NX_ISNUM(X)) {
            return Y;
        }
    } else if (X->kind == NX_DTYPE_FLOAT || Y->kind == NX_DTYPE_FLOAT) {
        if (X->kind == NX_DTYPE_FLOAT && Y->kind == NX_DTYPE_FLOAT) {
            return X->size > Y->size ? X : Y;
        } else if (X->kind == NX_DTYPE_FLOAT && NX_ISNUM(Y)) {
            return X;
        } else if (Y->kind == NX_DTYPE_FLOAT && NX_ISNUM(X)) {
            return Y;
        }
    } else if (X->kind == NX_DTYPE_INT || Y->kind == NX_DTYPE_INT) {
        if (X->kind == NX_DTYPE_INT && Y->kind == NX_DTYPE_INT) {
            return X->size > Y->size ? X : Y;
        } else if (X->kind == NX_DTYPE_INT && NX_ISNUM(Y)) {
            return X;
        } else if (Y->kind == NX_DTYPE_INT && NX_ISNUM(X)) {
            return Y;
        }
    }
    KS_THROW(kst_TypeError, "Failed calculate cast of arguments: %R, %R", X, Y);
    return NULL;
}

nx_t nx_getshape(kso obj) {
    nx_t self;
    if (kso_issub(obj->type, kst_none)) {
        /* Scalar */
        self.rank = 0;
        return self;
    } else if (kso_is_int(obj)) {
        /* 1D array */
        self.rank = 1;
        ks_cint x;
        if (!kso_get_ci(obj, &x)) {
            self.rank = -1;
            return self;
        }

        self.shape[0] = x;

        return self;
    } else {
        /* Assume iterable */
        ks_list l = ks_list_newi(obj);
        if (!l) {
            self.rank = -1;
            return self;
        }

        self.rank = l->len;

        int i;
        for (i = 0; i < l->len; ++i) {
            kso ob = l->elems[i];
            ks_cint x;
            if (!kso_get_ci(ob, &x)) {
                KS_DECREF(l);
                self.rank = -1;
                return self;
            }

            self.shape[i] = x;
        }
        KS_DECREF(l);

        return self;
    }   
}

ks_size_t* nx_getsize(kso obj, int* num) {
    if (kso_issub(obj->type, kst_none)) {
        /* Scalar size */
        *num = 0;
        return (ks_size_t*)ks_malloc(1);
    } else if (kso_is_int(obj)) {
        /* 1D size */
        ks_cint x;
        if (!kso_get_ci(obj, &x)) {
            return NULL;
        }
        ks_size_t* res = ks_zmalloc(sizeof(*res), 1);
        *num = 1;
        res[0] = x;

        return res;
    } else {
        /* Assume iterable */
        ks_list l = ks_list_newi(obj);
        if (!l) return NULL;

        ks_size_t* res = ks_zmalloc(sizeof(*res), l->len);
        *num = l->len;

        int i;
        for (i = 0; i < l->len; ++i) {
            kso ob = l->elems[i];
            ks_cint x;
            if (!kso_get_ci(ob, &x)) {
                ks_free(res);
                KS_DECREF(l);
                return NULL;
            }

            res[i] = x;
        }
        KS_DECREF(l);

        return res;
    }
}

bool nx_getcast(nx_t X, nx_dtype dtype, nx_t* R, void** tofree) {
    if (X.dtype == dtype) {
        *R = X;
        *tofree = NULL;
        return true;
    } else {
        /* Attempt to create new array and cast it */
        ks_size_t tsz = dtype->size;
        int i;
        for (i = 0; i < X.rank; ++i) tsz *= X.shape[i];

        *tofree = ks_malloc(tsz);
        if (!*tofree) {
            KS_THROW(kst_Error, "Failed to allocate array...");
            return NULL;
        }

        if (!nx_cast(
            X,
            *R = nx_make(*tofree, dtype, X.rank, X.shape, NULL)
        )) {
            ks_free(*tofree);
            return false;
        }

        return true;
    }
}

bool nx_get(kso obj, nx_dtype dtype, nx_t* res, kso* ref) {
    if (kso_issub(obj->type, nxt_array)) {
        /* Already exists, TODO: check if cast is needed */
        *res = ((nx_array)obj)->val;
        *ref = NULL;
        return true;

    } else {
        nx_array newarr = nx_array_newo(nxt_array, obj, dtype);
        if (!newarr) {
            return false;
        }
        *res = newarr->val;

        /* Return reference */
        *ref = (kso)newarr;
        return true;
    }
}

/* Adds a single element to the IO object */
static bool my_getstr_addelem(ksio_BaseIO bio, nx_dtype dtype, void* ptr) {
    /*if (dtype == nxd_bl) {
        return ksio_add(bio, "%s", *(nx_bl*)ptr ? "true" : "false");
    }*/

    #define LOOP(TYPE) do { \
        return ksio_add(bio, "%l", (ks_cint)*(TYPE*)ptr); \
    } while (0);
    NXT_FOR_I(dtype, LOOP);
    #undef LOOP

    #define LOOP(TYPE) do { \
        return ksio_add(bio, "%f", (ks_cfloat)*(TYPE*)ptr); \
    } while (0);
    NXT_FOR_F(dtype, LOOP);
    #undef LOOP
    
    #define LOOP(TYPE) do { \
        TYPE v = *(TYPE*)ptr; \
        bool isneg = v.im < 0; \
        if (isneg) v.im = -v.im; \
        return ksio_add(bio, "%f%s%fi", (ks_cfloat)v.re, isneg ? "-" : "+", (ks_cfloat)v.im); \
    } while (0);
    NXT_FOR_C(dtype, LOOP);
    #undef LOOP

    return true;
}

/* Internal method */
bool my_getstr(ksio_BaseIO bio, nx_t X, int dep) {

    /* truncation size for max length of a single vector */
    int trunc_sz = 20;

    if (X.rank == 0) {
        return my_getstr_addelem(bio, X.dtype, X.data);

    } else if (X.rank == 1) {
        /* 1d, output a list-like structure */
        ksio_add(bio, "[");

        ks_size_t i;
        for (i = 0; i < X.shape[0]; ++i) {
            if (i > 0) ksio_add(bio, ", ");
            if (!my_getstr_addelem(bio, X.dtype, (void*)((ks_uint)X.data + X.strides[0] * i))) return false;
        }
        return ksio_add(bio, "]");
    } else {
        /* Use recursion */

        ksio_add(bio, "[");
        /* loop over outer dimension, adding each inner dimension*/
        ks_size_t i;
        nx_t inner = nx_make(X.data, X.dtype, X.rank-1, X.shape+1, X.strides+1);
        for (i = 0; i < X.shape[0]; ++i, inner.data = (void*)((ks_uint)inner.data + X.strides[0])) {
            if (i > 0) ksio_add(bio, ",\n%.*c", dep+1, ' ');

            if (!my_getstr(bio, inner, dep+1)) return false;
        }

        return ksio_add(bio, "]");
    }
}

bool nx_getstr(ksio_BaseIO bio, nx_t X) {
    return my_getstr(bio, X, 0);
}

bool nx_enc(nx_dtype dtype, kso obj, void* out) {
    if (dtype->kind == NX_DTYPE_INT) {
        ks_cint val;
        if (!kso_get_ci(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            *(TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_FOR_I(dtype, LOOP);
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_FLOAT) {
        ks_cfloat val;
        if (!kso_get_cf(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            *(TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_FOR_F(dtype, LOOP);
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_COMPLEX) {
        ks_ccomplex val;
        if (!kso_get_cc(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            ((TYPE*)out)->re = val.re; \
            ((TYPE*)out)->im = val.im; \
            return true; \
        } while (0);
        NXT_FOR_C(dtype, LOOP);
        #undef LOOP
    }

    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}



/* Module Functions */

static KS_TFUNC(M, neg) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        r = (kso)nx_array_newc(nxt_array, NULL, vX.dtype, vX.rank, vX.shape, NULL);
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

    if (!nx_neg(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, conj) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        r = (kso)nx_array_newc(nxt_array, NULL, vX.dtype, vX.rank, vX.shape, NULL);
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

    if (!nx_conj(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, abs) {
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
        /* Complex arguments should create real ones for abs */
        if (dtype == nxd_cH) {
            dtype = nxd_H;
        } else if (dtype == nxd_cF) {
            dtype = nxd_F;
        } else if (dtype == nxd_cD) {
            dtype = nxd_D;
        } else if (dtype == nxd_cE) {
            dtype = nxd_E;
        }

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

    if (!nx_abs(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, cast) {
    nx_dtype dtype;
    kso x;
    KS_ARGS("dtype:* x", &dtype, nxt_dtype, &x);

    nx_t vX;
    kso rX;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    nx_array r = nx_array_newc(nxt_array, NULL, dtype, vX.rank, vX.shape, NULL);
    if (!r) {
        KS_NDECREF(rX);
        return NULL;
    }


    if (!nx_cast(vX, r->val)) {
        KS_DECREF(r);
        KS_NDECREF(rX);
        return NULL;
    }

    KS_NDECREF(rX);
    
    return (kso)r;
}

static KS_TFUNC(M, fpcast) {
    nx_dtype dtype;
    kso x;
    KS_ARGS("dtype:* x", &dtype, nxt_dtype, &x);

    nx_t vX;
    kso rX;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    nx_array r = nx_array_newc(nxt_array, NULL, dtype, vX.rank, vX.shape, NULL);
    if (!r) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (!nx_fpcast(vX, r->val)) {
        KS_DECREF(r);
        KS_NDECREF(rX);
        return NULL;
    }

    KS_NDECREF(rX);
    
    return (kso)r;
}
static KS_TFUNC(M, fmin) {
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

    if (!nx_fmin(vX, vY, vR)) {
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

static KS_TFUNC(M, fmax) {
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

    if (!nx_fmax(vX, vY, vR)) {
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


static KS_TFUNC(M, clip) {
    kso x, y, z, r = KSO_NONE;
    KS_ARGS("x y z ?r", &x, &y, &z, &r);

    nx_t vX, vY, vZ, vR;
    kso rX, rY, rZ, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (!nx_get(y, NULL, &vY, &rY)) {
        KS_NDECREF(rX);
        return NULL;
    }
    if (!nx_get(z, NULL, &vZ, &rZ)) {
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
            KS_NDECREF(rZ);
            return NULL;
        }
        dtype = nx_cast2(dtype, vZ.dtype);
        if (!dtype) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_NDECREF(rZ);
            return NULL;
        }

        nx_t shape = nx_make_bcast(3, (nx_t[]) { vX, vY, vZ });
        if (shape.rank < 0) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_NDECREF(rZ);
            return NULL;
        }

        r = (kso)nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
        if (!r) {
            KS_NDECREF(rX);
            KS_NDECREF(rY);
            KS_NDECREF(rZ);
            return NULL;
        }
    } else {
        KS_INCREF(r);
    }

    if (!nx_get(r, NULL, &vR, &rR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rZ);
        KS_DECREF(r);
        return NULL;
    }

    if (!nx_clip(vX, vY, vZ, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rY);
        KS_NDECREF(rZ);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rY);
    KS_NDECREF(rZ);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, add) {
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

    if (!nx_add(vX, vY, vR)) {
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

static KS_TFUNC(M, sub) {
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

    if (!nx_sub(vX, vY, vR)) {
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


static KS_TFUNC(M, mul) {
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

    if (!nx_mul(vX, vY, vR)) {
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
static KS_TFUNC(M, mod) {
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

    if (!nx_mod(vX, vY, vR)) {
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


static KS_TFUNC(M, div) {
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
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT && vY.dtype->kind == NX_DTYPE_INT) ? nxd_D : nx_cast2(vX.dtype, vY.dtype);
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

    if (!nx_div(vX, vY, vR)) {
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


static KS_TFUNC(M, floordiv) {
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

    if (!nx_floordiv(vX, vY, vR)) {
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


static KS_TFUNC(M, pow) {
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

    if (!nx_pow(vX, vY, vR)) {
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

static KS_TFUNC(M, sqrt) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_sqrt(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}


static KS_TFUNC(M, exp) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_exp(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}



static KS_TFUNC(M, log) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_log(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}




static KS_TFUNC(M, sin) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_sin(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, cos) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_cos(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, tan) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_tan(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, asin) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_asin(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, acos) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_acos(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, atan) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_atan(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, sinh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_sinh(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, cosh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_cosh(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, tanh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_tanh(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}

static KS_TFUNC(M, asinh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_asinh(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, acosh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_acosh(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}
static KS_TFUNC(M, atanh) {
    kso x, r = KSO_NONE;
    KS_ARGS("x ?r", &x, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = (vX.dtype->kind == NX_DTYPE_INT) ? nxd_D : vX.dtype;

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

    if (!nx_atanh(vX, vR)) {
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

ks_module _ksi_nx() {
    _ksi_nx_dtype();
    _ksi_nx_array();
    _ksi_nx_view();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Submodules */
        {"rand",                   (kso)_ksi_nxrand()},
        
        /* Types */

        {"dtype",                  KS_NEWREF(nxt_dtype)},
        
        {"array",                  KS_NEWREF(nxt_array)},
        {"view",                   KS_NEWREF(nxt_view)},
    
        /* Datatypes */

        {"bool",                   KS_NEWREF(nxd_bl)},
        {"s8",                     KS_NEWREF(nxd_s8)},
        {"u8",                     KS_NEWREF(nxd_u8)},
        {"s16",                    KS_NEWREF(nxd_s16)},
        {"u16",                    KS_NEWREF(nxd_u16)},
        {"s32",                    KS_NEWREF(nxd_s32)},
        {"u32",                    KS_NEWREF(nxd_u32)},
        {"s64",                    KS_NEWREF(nxd_s64)},
        {"u64",                    KS_NEWREF(nxd_u64)},

        {"H",                      KS_NEWREF(nxd_H)},
        {"F",                      KS_NEWREF(nxd_F)},
        {"D",                      KS_NEWREF(nxd_D)},
        {"L",                      KS_NEWREF(nxd_L)},
        {"E",                      KS_NEWREF(nxd_E)},

        {"cH",                     KS_NEWREF(nxd_cH)},
        {"cF",                     KS_NEWREF(nxd_cF)},
        {"cD",                     KS_NEWREF(nxd_cD)},
        {"cL",                     KS_NEWREF(nxd_cL)},
        {"cE",                     KS_NEWREF(nxd_cE)},

        {"half",                   KS_NEWREF(nxd_H)},
        {"float",                  KS_NEWREF(nxd_F)},
        {"double",                 KS_NEWREF(nxd_D)},
        {"fp128",                  KS_NEWREF(nxd_E)},

        /* Functions */

        {"neg",                    ksf_wrap(M_neg_, M_NAME ".neg(x, r=none)", "Computes elementwise negation")},
        {"abs",                    ksf_wrap(M_abs_, M_NAME ".abs(x, r=none)", "Computes elementwise absolute value")},
        {"conj",                   ksf_wrap(M_conj_, M_NAME ".conj(x, r=none)", "Computes elementwise conjugation")},

        {"fmin",                   ksf_wrap(M_fmin_, M_NAME ".fmin(x, y, r=none)", "Computes elementwise minimum")},
        {"fmax",                   ksf_wrap(M_fmax_, M_NAME ".fmin(x, y, r=none)", "Computes elementwise maximum")},
        {"clip",                   ksf_wrap(M_clip_, M_NAME ".clip(x, y, z, r=none)", "Computes elementwise clipping between 'y' (minimum) and 'z' (maximum)\n\n    Equivalent to 'nx.fmin(z, nx.fmax(y, x))'")},

        {"add",                    ksf_wrap(M_add_, M_NAME ".add(x, y, r=none)", "Computes elementwise addition")},
        {"sub",                    ksf_wrap(M_sub_, M_NAME ".sub(x, y, r=none)", "Computes elementwise subtraction")},
        {"mul",                    ksf_wrap(M_mul_, M_NAME ".mul(x, y, r=none)", "Computes elementwise multiplication")},
        {"mod",                    ksf_wrap(M_mod_, M_NAME ".mod(x, y, r=none)", "Computes elementwise modulo")},
        {"div",                    ksf_wrap(M_div_, M_NAME ".div(x, y, r=none)", "Computes elementwise division (true division, not floor)")},
        {"floordiv",               ksf_wrap(M_floordiv_, M_NAME ".floordiv(x, y, r=none)", "Computes elementwise floored division")},

        {"pow",                    ksf_wrap(M_pow_, M_NAME ".pow(x, y, r=none)", "Computes elementwise exponentiation")},
        
        {"sqrt",                   ksf_wrap(M_sqrt_, M_NAME ".sqrt(x, y, r=none)", "Computes elementwise square root")},
        {"exp",                    ksf_wrap(M_exp_, M_NAME ".exp(x, r=none)", "Computes elementwise exponential function (base-e)")},
        {"log",                    ksf_wrap(M_log_, M_NAME ".log(x, r=none)", "Computes elementwise logarithm function (base-e)")},
        
        {"sin",                    ksf_wrap(M_sin_, M_NAME ".sin(x, r=none)", "Computes elementwise sine")},
        {"cos",                    ksf_wrap(M_cos_, M_NAME ".cos(x, r=none)", "Computes elementwise cosine")},
        {"tan",                    ksf_wrap(M_tan_, M_NAME ".tan(x, r=none)", "Computes elementwise tangent")},
        {"asin",                   ksf_wrap(M_asin_, M_NAME ".asin(x, r=none)", "Computes elementwise inverse sine")},
        {"acos",                   ksf_wrap(M_acos_, M_NAME ".acos(x, r=none)", "Computes elementwise inverse cosine")},
        {"atan",                   ksf_wrap(M_atan_, M_NAME ".atan(x, r=none)", "Computes elementwise inverse tangent")},
        {"sinh",                   ksf_wrap(M_sinh_, M_NAME ".sinh(x, r=none)", "Computes elementwise hyperbolic sine")},
        {"cosh",                   ksf_wrap(M_cosh_, M_NAME ".cosh(x, r=none)", "Computes elementwise hyperbolic cosine")},
        {"tanh",                   ksf_wrap(M_tanh_, M_NAME ".tanh(x, r=none)", "Computes elementwise hyperbolic tangent")},
        {"asinh",                  ksf_wrap(M_asinh_, M_NAME ".asinh(x, r=none)", "Computes elementwise inverse hyperbolic sine")},
        {"acosh",                  ksf_wrap(M_acosh_, M_NAME ".acosh(x, r=none)", "Computes elementwise inverse hyperbolic cosine")},
        {"atanh",                  ksf_wrap(M_atanh_, M_NAME ".atanh(x, r=none)", "Computes elementwise inverse hyperbolic tangent")},

        {"cast",                   ksf_wrap(M_cast_, M_NAME ".cast(dtype, obj)", "Casts to a datatype")},
        {"fpcast",                 ksf_wrap(M_fpcast_, M_NAME ".fpcast(dtype, obj)", "Casts to a datatype, with automatic fixed-point and floating-point conversion")},
        /*

        {"add",                    ksf_wrap(add_, M_NAME ".add(x, y, r=none)", "Computes elementwise addition")},
        {"sub",                    ksf_wrap(sub_, M_NAME ".sub(x, y, r=none)", "Computes elementwise subtraction")},
        {"mul",                    ksf_wrap(mul_, M_NAME ".mul(x, y, r=none)", "Computes elementwise multiplication")},
        {"mod",                    ksf_wrap(mod_, M_NAME ".mod(x, y, r=none)", "Computes elementwise modulo")},
        */

    ));

    return res;
}
