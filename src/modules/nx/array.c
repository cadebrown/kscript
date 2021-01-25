/* array.c - implementation of the 'nx.array' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define T_NAME "nx.array"

/* Internals */

static int kern_copy(int N, nx_t* args, int len, void* extra) {
    assert(N == 2);
    ks_cint i;
    nx_t X = args[0], R = args[1];
    ks_uint pX = (ks_uint)X.data, pR = (ks_uint)R.data;
    ks_cint sX = X.strides[0], sR = R.strides[0];

    if (sR == X.dtype->size && sX == sR) {
        /* Contiguous */
        memcpy((void*)pR, (void*)pX, R.dtype->size * len);
    } else {
        /* Non-continguous */
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            memcpy((void*)pR, (void*)pX, R.dtype->size);
        }
    }

    return 0;

}

/* C-API */
nx_array nx_array_newc(ks_type tp, void* data, nx_dtype dtype, int rank, ks_size_t* shape, ks_ssize_t* strides) {
    nx_array self = KSO_NEW(nx_array, tp);

    KS_INCREF(dtype);
    self->val.dtype = dtype;

    self->val.rank = rank;

    int i;
    for (i = 0; i < rank; ++i) {
        self->val.shape[i] = shape[i];
    }
    /* Last stride is the element size */
    if (rank > 0) self->val.strides[rank - 1] = dtype->size;

    /* Calculate strides for dense array, which are the products of the last 'N' dimensions
     *   times the bytes of each element
     */
    for (i = rank - 2; i >= 0; --i) {
        self->val.strides[i] = self->val.strides[i + 1] * shape[i + 1];
    }

    /* Calculate total size, which is the product of the dimensions times element size */
    ks_size_t tsz = dtype->size;
    for (i = 0; i < rank; ++i) tsz *= shape[i];


    self->val.data = ks_malloc(tsz);
    if (!self->val.data) {
        KS_DECREF(self);
        KS_THROW(kst_Error, "Failed to allocate tensor of large size");
        return NULL;
    }

    if (data) {
        /* Initialize with memory copying */
        if (nx_apply_elem(kern_copy, 2, (nx_t[]) {
            nx_make(data, dtype, rank, shape, strides),
            self->val,
        }, NULL)) {
            KS_DECREF(self);
            return NULL;
        }
    }

    return self;


}

/* internal method to tell the full depth of the array, then fill in corresponding dimensions once known, and finally,
 *   upon exit, set each element correctly casted
 * Returns whether it was successfull
 */
static bool I_array_fill(ks_type tp, nx_dtype dtype, nx_array* resp, kso cur, int* idx, int dep, int* max_dep, ks_size_t** dims) {
    if (dep > *max_dep) *max_dep = dep;

    if (cur->type == nxt_array/* || cur->type == nx_T_view*/) {
        KS_THROW(kst_Error, "Need to implement sub-arrays in filling");
        return false;

    } else if (kso_is_iterable(cur)) {
        /* Have an iterable object, so convert to list and recursively fill with it */
        ks_list elems = ks_list_newi(cur);
        if (!elems) return false;

        if (*resp) {
            /* Already reached max depth, so ensure we are of a correct length */
            if ((*dims)[dep] != elems->len) {
                KS_THROW(kst_SizeError, "Initializing entries had differing dimensions");
                return false;
            }
        } else {
            /* First time at this depth, so set the dimensions */
            *dims = ks_realloc(*dims, sizeof(**dims) * (dep + 1));
            (*dims)[dep] = elems->len;
        }

        int i;
        for (i = 0; i < elems->len; ++i) {
            if (!I_array_fill(tp, dtype, resp, elems->elems[i], idx, dep + 1, max_dep, dims)) {
                return false;
            }
        }

        KS_DECREF(elems);

    } else {
        int my_idx = *idx;
        *idx = *idx + 1;

        if (my_idx == 0) {
            /* first creation, so create the array */
            *resp = nx_array_newc(tp, NULL, dtype, dep, *dims, NULL);
        } else {
            /* already created, so ensure that the element was at maximum depth */
            if (dep != (*resp)->val.rank) {
                KS_THROW(kst_SizeError, "Initializing entries had differing dimensions");
                return false;
            }
        }

        /* Convert object */
        if (!nx_enc(dtype, cur, (void*)((ks_uint)(*resp)->val.data + dtype->size * my_idx))) {
            return false;
        }

        /*
        if (!nx_T_set_all((nxar_t){
            .data = (void*)addr, 
            .dtype = dtype,
            .rank = 1, 
            .dim = (nx_size_t[]){ 1 }, 
            .stride = (nx_size_t[]){ 0 }
        }, cur)) {
            return false;
        }
        */
    }

    if (dep == 0 && !*resp) {
        /* fill in automatically */
        *resp = nx_array_newc(tp, NULL, dtype, *max_dep + 1, *dims, NULL);
    }

    return true;
}


nx_array nx_array_newo(ks_type tp, kso obj, nx_dtype dtype) {
    if (kso_issub(obj->type, nxt_array)) {
        nx_t of = ((nx_array)obj)->val;
        nx_array res = nx_array_newc(tp, NULL, dtype, of.rank, of.shape, NULL);

        if (!nx_cast(of, res->val)) {
            KS_DECREF(res);
            return NULL;
        }

        return res;
    } else if (kso_is_iterable(obj)) {
        if (!dtype) dtype = nxd_D;

        /* set up temporaries and fill array */
        int idx = 0, max_dep = 0;
        ks_size_t* dims = NULL;
        nx_array res = NULL;

        if (!I_array_fill(tp, dtype, &res, obj, &idx, 0, &max_dep, &dims)) {
            if (res) KS_DECREF(res);
            res = NULL;
        }

        ks_free(dims);
        return res;
    } else if (kso_is_int(obj)) {
        if (!dtype) dtype = nxd_D;

        nx_array res = nx_array_newc(tp, NULL, dtype, 0, NULL, NULL);
        if (!nx_enc(dtype, obj, res->val.data)) {
            return false;
        }

        return res;
    } else if (kso_is_complex(obj)) {
        if (!dtype) dtype = nxd_cD;

        nx_array res = nx_array_newc(tp, NULL, dtype, 0, NULL, NULL);

        if (!nx_enc(dtype, obj, res->val.data)) {
            return false;
        }

        return res;
    } else if (kso_is_float(obj)) {
        if (!dtype) dtype = nxd_D;

        nx_array res = nx_array_newc(tp, NULL, dtype, 0, NULL, NULL);

        if (!nx_enc(dtype, obj, res->val.data)) {
            return false;
        }
        return res;
    }


    KS_THROW_CONV(obj->type, tp);
    return NULL;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    KS_DECREF(self->val.dtype);
    ks_free(self->val.data);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj;
    nx_dtype dtype = nxd_D;
    KS_ARGS("tp:* obj ?dtype:*", &tp, kst_type, &obj, &dtype, nxt_dtype);

    return (kso)nx_array_newo(tp, obj, dtype);
}

static KS_TFUNC(T, str) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    /* TODO: add specifics */
    ksio_StringIO sio = ksio_StringIO_new();

    if (!nx_getstr((ksio_BaseIO)sio, self->val)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}

static KS_TFUNC(T, getattr) {
    nx_array self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, nxt_array, &attr, kst_str);
    
    if (ks_str_eq_c(attr, "shape", 5)) {
        ks_tuple res = ks_tuple_newe(self->val.rank);

        int i;
        for (i = 0; i < self->val.rank; ++i) {
            res->elems[i] = (kso)ks_int_new(self->val.shape[i]);
        }

        return (kso)res;
    } else if (ks_str_eq_c(attr, "strides", 7)) {

        ks_tuple res = ks_tuple_newe(self->val.rank);

        int i;
        for (i = 0; i < self->val.rank; ++i) {
            res->elems[i] = (kso)ks_int_new(self->val.strides[i]);
        }

        return (kso)res;
    } else if (ks_str_eq_c(attr, "rank", 4)) {
        return (kso)ks_int_new(self->val.rank);
    } else if (ks_str_eq_c(attr, "dtype", 5)) {
        return KS_NEWREF(self->val.dtype);
    } else if (ks_str_eq_c(attr, "data", 4)) {
        return (kso)ks_int_newu((ks_uint)self->val.data);
    }
    
    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, getelem) {
    nx_array self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, nxt_array, &nargs, &args);
    
    nx_t res = nx_getelem(self->val, nargs, args);
    if (res.rank < 0) return NULL;

    return (kso)nx_view_newo(nxt_view, res, (kso)self);
}
static KS_TFUNC(T, setelem) {
    nx_array self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, nxt_array, &nargs, &args);
    
    if (nargs < 1) {
        KS_THROW(kst_ArgError, "Setting elements requires at least one argument");
        return NULL;
    }

    nx_t val;
    kso vref;
    if (!nx_get(args[nargs - 1], NULL, &val, &vref)) {
        return NULL;
    }

    nx_t res = nx_getelem(self->val, nargs - 1, args);
    if (res.rank < 0) {
        KS_NDECREF(vref);
        return NULL;
    }

    if (!nx_cast(val, res)) {
        KS_NDECREF(vref);
        return NULL;
    }

    KS_NDECREF(vref);
    return KSO_NONE;
}


static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_add(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, sub) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_sub(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, mul) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_mul(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, matmul) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }

    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    if (aL.rank == 0) {
        KS_THROW(kst_TypeError, "Scalars are not allowed in matrix multiplication");
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    } else if (aL.rank == 1) {
        aL = nx_with_newaxis(aL, 1);
    }
    if (aR.rank == 0) {
        KS_THROW(kst_TypeError, "Scalars are not allowed in matrix multiplication");
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    } else if (aR.rank == 1) {
        aR = nx_with_newaxis(aR, 1);
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    int M = aL.shape[aL.rank - 2], N = aL.shape[aL.rank - 1], K = aR.shape[aR.rank - 1];

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    shape.shape[shape.rank - 1] = K;
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nxla_matmul(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, div) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = (aL.dtype->kind == NX_DTYPE_INT && aR.dtype->kind == NX_DTYPE_INT) ? nxd_D : nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_div(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}
static KS_TFUNC(T, floordiv) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_floordiv(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, mod) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_mod(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}

static KS_TFUNC(T, pow) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    nx_t aL, aR;
    kso rL, rR;
    if (!nx_get(L, NULL, &aL, &rL)) {
        return NULL;
    }
    if (!nx_get(R, NULL, &aR, &rR)) {
        KS_NDECREF(rL);
        return NULL;
    }

    nx_dtype dtype = nx_cast2(aL.dtype, aR.dtype);
    if (!dtype) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    nx_t shape = nx_make_bcast(2, (nx_t[]){ aL, aR });
    if (shape.rank < 0) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }
    nx_array res = nx_array_newc(nxt_array, NULL, dtype, shape.rank, shape.shape, NULL);
    if (!res) {
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    if (!nx_pow(aL, aR, res->val)) {
        KS_DECREF(res);
        KS_NDECREF(rL);
        KS_NDECREF(rR);
        return NULL;
    }

    KS_NDECREF(rL);
    KS_NDECREF(rR);
    return (kso)res;
}


/* Export */

static struct ks_type_s tp;
ks_type nxt_array = &tp;

void _ksi_nx_array() {
    
    _ksinit(nxt_array, kst_object, T_NAME, sizeof(struct nx_array_s), -1, "Multidimesional array", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, obj, dtype=nx.float64)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, *keys)", "")},
        {"__setelem",              ksf_wrap(T_setelem_, T_NAME ".__setelem(self, *keys, val)", "")},

        {"__add",                  ksf_wrap(T_add_, T_NAME ".__add(L, R)", "")},
        {"__sub",                  ksf_wrap(T_sub_, T_NAME ".__sub(L, R)", "")},
        {"__mul",                  ksf_wrap(T_mul_, T_NAME ".__mul(L, R)", "")},
        {"__matmul",               ksf_wrap(T_matmul_, T_NAME ".__matmul(L, R)", "")},
        {"__div",                  ksf_wrap(T_div_, T_NAME ".__div(L, R)", "")},
        {"__floordiv",             ksf_wrap(T_floordiv_, T_NAME ".__floordiv(L, R)", "")},
        {"__mod",                  ksf_wrap(T_mod_, T_NAME ".__mod(L, R)", "")},
        {"__pow",                  ksf_wrap(T_pow_, T_NAME ".__pow(L, R)", "")},

/*
        {"__neg",                  ksf_wrap(T_neg_, T_NAME ".__neg(V)", "")},
        {"__abs",                  ksf_wrap(T_abs_, T_NAME ".__abs(V)", "")},
        {"__sqig",                 ksf_wrap(T_sqig_, T_NAME ".__sqig(V)", "")},

     

        {"isscalar",               ksf_wrap(T_isscalar_, T_NAME ".isscalar(self)", "Computes whether 'self' is a scalar (i.e. 0-rank array)")},
        {"isvec",                  ksf_wrap(T_isvec_, T_NAME ".isvec(self)", "Computes whether 'self' is a vector (i.e. 1-rank array)")},
        {"ismat",                  ksf_wrap(T_ismat_, T_NAME ".ismat(self)", "Computes whether 'self' is a matrix (i.e. 2-rank array)")},

        */

    ));

}



