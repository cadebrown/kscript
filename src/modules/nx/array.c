/* array.c - implementation of the 'nx.array' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nx.h>

#define T_NAME "nx.array"
#define TI_NAME "nx.array.__iter"

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
        }, NULL, NULL)) {
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
    if (!dtype) dtype = nxd_D;
    
    /* Get block of objects */
    int rank;
    ks_size_t shape[NX_MAXRANK];
    kso* oblk = nx_objblock(obj, &rank, shape);
    if (!oblk) {
        return NULL;
    }

    /* Construct result array */
    nx_array res = NULL;
    ks_size_t i;

    /* Convert linearly */
    if (dtype->kind == NX_DTYPE_STRUCT) {
        res = nx_array_newc(tp, NULL, dtype, rank - 1, shape, NULL);
        ks_size_t tnum = szprod(rank-1, shape);
        for (i = 0; i < tnum; ++i) {
            ks_uint outptr = (ks_uint)res->val.data + i * dtype->size;

            /* Objects to convert */
            int nobj = shape[rank - 1];
            kso* obj = &oblk[i * shape[rank - 1]];
            ks_list llo = ks_list_new(nobj, obj);
            if (!nx_enc(dtype, (kso)llo, (void*)outptr)) {
                for (i = 0; i < tnum; ++i) {
                    KS_DECREF(oblk[i]);
                }
                KS_DECREF(llo);

                KS_DECREF(res);
                ks_free(oblk);
                return NULL;
            }
            KS_DECREF(llo);
        }
    } else {
        res = nx_array_newc(tp, NULL, dtype, rank, shape, NULL);
        ks_size_t tnum = szprod(rank, shape);
        for (i = 0; i < tnum; ++i) {
            ks_uint outptr = (ks_uint)res->val.data + i * dtype->size;
            if (!nx_enc(dtype, oblk[i], (void*)outptr)) {
                for (i = 0; i < tnum; ++i) {
                    KS_DECREF(oblk[i]);
                }

                KS_DECREF(res);
                ks_free(oblk);
                return NULL;
            }
        }
    }

    ks_size_t tnum = szprod(rank, shape);
    for (i = 0; i < tnum; ++i) {
        KS_DECREF(oblk[i]);
    }

    ks_free(oblk);
    return res;

    /** OLD METHOD **/

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

static KS_TFUNC(T, float) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    if (self->val.rank != 0) {
        KS_THROW(kst_SizeError, "Only arrays of rank-0 may be converted to 'float'");
        return NULL;
    }

    ks_cfloat v;
    if (!nx_cast(
        self->val,
        nx_make(&v, nxd_D, 0, NULL, NULL)
    )) {
        return NULL;
    }

    return (kso)ks_float_new(v);
}

static KS_TFUNC(T, integral) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    if (self->val.rank != 0) {
        KS_THROW(kst_SizeError, "Only arrays of rank-0 may be converted as integers");
        return NULL;
    }
    if (self->val.dtype->kind != NX_DTYPE_INT) {
        KS_THROW(kst_SizeError, "Only arrays of integer datatype may be converted as integers");
        return NULL;
    }

    nx_s64 v;
    if (!nx_cast(
        self->val,
        nx_make(&v, nxd_s64, 0, NULL, NULL)
    )) {
        return NULL;
    }

    return (kso)ks_int_new(v);
}

static KS_TFUNC(T, int) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    if (self->val.rank != 0) {
        KS_THROW(kst_SizeError, "Only arrays of rank-0 may be converted as integers");
        return NULL;
    }

    nx_s64 v;
    if (!nx_cast(
        self->val,
        nx_make(&v, nxd_s64, 0, NULL, NULL)
    )) {
        return NULL;
    }

    return (kso)ks_int_new(v);
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
    }else if (ks_str_eq_c(attr, "T", 1)) {
        nx_t sT = self->val;
        if (sT.rank == 0) {
        } else if (sT.rank == 1) {
            sT = nx_newaxis(sT, 1);
        } else {
            sT = nx_swapaxes(sT, sT.rank - 2, sT.rank - 1);
        }

        return (kso)nx_view_newo(nxt_view, sT, (kso)self);
    } else if (self->val.dtype->kind == NX_DTYPE_STRUCT) {
        /* Find attribute and return view */

        int i;
        for (i = 0; i < self->val.dtype->s_cstruct.n_members; ++i) {
            if (ks_str_eq(attr, self->val.dtype->s_cstruct.members[i].name)) {
                /* Construct view */
                ks_uint ptr = (ks_uint)self->val.data + self->val.dtype->s_cstruct.members[i].offset;
                nx_t res = nx_make((void*)ptr, self->val.dtype->s_cstruct.members[i].dtype, self->val.rank, self->val.shape, self->val.strides);
                return (kso)nx_view_newo(nxt_view, res, (kso)self);
            }

        }

    }
    
    KS_THROW_ATTR(self, attr);
    return NULL;
}

static KS_TFUNC(T, getelem) {
    nx_array self;
    int nargs;
    kso* args;
    KS_ARGS("self:* *args", &self, nxt_array, &nargs, &args);
    
    nx_t res = nx_getevo(self->val, nargs, args);
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

    nx_t res = nx_getevo(self->val, nargs - 1, args);
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


/** Templates **/


/* Arithmetic function taking 2 arguments */
#define T_A2v(_name, _func) static KS_TFUNC(T, _name) { \
    kso ax, ay = KSO_NONE; \
    KS_ARGS("x y", &ax, &ay); \
    nx_t x, y; \
    kso xr, yr; \
    if (!nx_get(ax, NULL, &x, &xr)) { \
        return NULL; \
    } \
    if (!nx_get(ay, NULL, &y, &yr)) { \
        KS_NDECREF(xr); \
        return NULL; \
    } \
    nx_dtype dtype = nx_resnum(x.dtype, y.dtype); \
    if (!dtype) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        return NULL; \
    } \
    int rank; \
    ks_size_t shape[NX_MAXRANK]; \
    if (!nx_getbc(2, (nx_t[]) { x, y }, &rank, shape)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        return NULL; \
    } \
    nx_array ar = nx_array_newc(nxt_array, NULL, dtype, rank, shape, NULL); \
    if (!ar) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        return NULL; \
    } \
    if (!_func(x, y, ar->val)) { \
        KS_NDECREF(xr); \
        KS_NDECREF(yr); \
        KS_DECREF(ar); \
        return NULL; \
    } \
    KS_NDECREF(xr); \
    KS_NDECREF(yr); \
    return (kso)ar; \
}

#define T_A2(_name) T_A2v(_name, nx_##_name)


/* Arithmetic function taking 1 arguments */
#define T_A1(_name) static KS_TFUNC(T, _name) { \
    nx_array self; \
    KS_ARGS("self:*", &self, nxt_array); \
    nx_array ar = nx_array_newc(nxt_array, NULL, self->val.dtype, self->val.rank, self->val.shape, NULL); \
    if (!ar) { \
        return NULL; \
    } \
    if (!nx_##_name(self->val, ar->val)) { \
        KS_DECREF(ar); \
        return NULL; \
    } \
    return (kso)ar; \
}


/* Arithmetic function taking 1 arguments */
#define T_A1r2c(_name) static KS_TFUNC(T, _name) { \
    nx_array self; \
    KS_ARGS("self:*", &self, nxt_array); \
    nx_dtype dtype = nx_realtype(self->val.dtype); \
    if (!dtype) return NULL; \
    nx_array ar = nx_array_newc(nxt_array, NULL, dtype, self->val.rank, self->val.shape, NULL); \
    if (!ar) { \
        return NULL; \
    } \
    if (!nx_##_name(self->val, ar->val)) { \
        KS_DECREF(ar); \
        return NULL; \
    } \
    return (kso)ar; \
}


T_A2(add)
T_A2(sub)
T_A2(mul)
T_A2(div)
T_A2(floordiv)
T_A2(mod)
T_A2(pow)
T_A2v(matmul, nxla_matmul)


T_A1(neg)
T_A1(conj)
T_A1r2c(abs)


/* Iterator Type */


static KS_TFUNC(TI, free) {
    nx_array_iter self;
    KS_ARGS("self:*", &self, nxt_array_iter);

    KS_NDECREF(self->of);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, init) {
    nx_array_iter self;
    nx_array of;
    KS_ARGS("self:* of:*", &self, nxt_array_iter, &of, nxt_array);

    KS_INCREF(of);
    self->of = of;

    self->pos = 0;

    return KSO_NONE;
}
static KS_TFUNC(TI, next) {
    nx_array_iter self;
    KS_ARGS("self:*", &self, nxt_array_iter);

    if (self->of->val.rank < 1 || self->pos >= self->of->val.shape[0]) {
        KS_OUTOFITER();
        return NULL;
    }

    /* Get position */
    ks_cint p = self->pos++;

    ks_uint ptr = (ks_uint)self->of->val.data + p * self->of->val.strides[0];
    nx_t res = nx_make((void*)ptr, self->of->val.dtype, self->of->val.rank - 1, self->of->val.shape + 1, self->of->val.strides + 1);

    return (kso)nx_view_newo(nxt_view, res, (kso)self->of);
}


/* Export */

static struct ks_type_s tp;
ks_type nxt_array = &tp;

static struct ks_type_s tpi;
ks_type nxt_array_iter = &tpi;

void _ksi_nx_array() {
    
    _ksinit(nxt_array_iter, kst_object, TI_NAME, sizeof(struct nx_array_iter_s), -1, "Array iterator", KS_IKV(
        {"__free",                 ksf_wrap(TI_free_, TI_NAME ".__free(self)", "")},
        {"__init",                 ksf_wrap(TI_init_, TI_NAME ".__init(self, of)", "")},
        {"__next",                 ksf_wrap(TI_next_, TI_NAME ".__next(self)", "")},
    ));

    _ksinit(nxt_array, kst_object, T_NAME, sizeof(struct nx_array_s), -1, "Multidimesional array", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, obj, dtype=nx.float64)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__float",                ksf_wrap(T_float_, T_NAME ".__float(self)", "")},
        {"__int",                  ksf_wrap(T_int_, T_NAME ".__int(self)", "")},
        {"__iter",                 (kso)nxt_array_iter},

        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

        {"__getelem",              ksf_wrap(T_getelem_, T_NAME ".__getelem(self, *keys)", "")},
        {"__setelem",              ksf_wrap(T_setelem_, T_NAME ".__setelem(self, *keys, val)", "")},

        {"__abs",                  ksf_wrap(T_abs_, T_NAME ".__abs(self)", "")},
        {"__neg",                  ksf_wrap(T_abs_, T_NAME ".__neg(self)", "")},
        {"__sqig",                 ksf_wrap(T_conj_, T_NAME ".__sqig(self)", "")},

        {"__add",                  ksf_wrap(T_add_, T_NAME ".__add(L, R)", "")},
        {"__sub",                  ksf_wrap(T_sub_, T_NAME ".__sub(L, R)", "")},
        {"__mul",                  ksf_wrap(T_mul_, T_NAME ".__mul(L, R)", "")},
        {"__matmul",               ksf_wrap(T_matmul_, T_NAME ".__matmul(L, R)", "")},
        {"__div",                  ksf_wrap(T_div_, T_NAME ".__div(L, R)", "")},
        {"__floordiv",             ksf_wrap(T_floordiv_, T_NAME ".__floordiv(L, R)", "")},
        {"__mod",                  ksf_wrap(T_mod_, T_NAME ".__mod(L, R)", "")},
        {"__pow",                  ksf_wrap(T_pow_, T_NAME ".__pow(L, R)", "")},

    ));

}



