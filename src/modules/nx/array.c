/* array.c - implementation of the 'nx.array' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define T_NAME "nx.array"

/* Internals */

static int kern_copy(int N, nxar_t* inp, int len, void* _data) {
    assert(N == 2);

    ks_cint i;
    nx_dtype dR = inp[0].dtype, dX = inp[1].dtype;
    ks_uint pR = (ks_uint)inp[0].data, pX = (ks_uint)inp[1].data;
    ks_cint sR = inp[0].strides[0], sX = inp[1].strides[0];

    if (sR == inp[0].dtype->size && sX == sR) {
        /* Contiguous */
        memcpy((void*)pR, (void*)pX, inp[0].dtype->size * len);
    } else {
        for (i = 0; i < len; i++, pR += sR, pX += sX) {
            memcpy((void*)pR, (void*)pX, inp[0].dtype->size);
        }
    }

    return 0;

}

/* C-API */

nx_array nx_array_newc(ks_type tp, nx_dtype dtype, int rank, ks_size_t* dims, ks_ssize_t* strides, void* data) {
    nx_array self = KSO_NEW(nx_array, tp);

    KS_INCREF(dtype);
    self->ar.dtype = dtype;

    self->ar.rank = rank;

    self->ar.dims = ks_zmalloc(sizeof(*self->ar.dims), self->ar.rank);
    self->ar.strides = ks_zmalloc(sizeof(*self->ar.strides), self->ar.rank);

    memcpy(self->ar.dims, dims, sizeof(*self->ar.dims) * self->ar.rank);

    /* last stride is the element size */
    if (self->ar.rank > 0) self->ar.strides[self->ar.rank - 1] = self->ar.dtype->size;
    int i;
    /* calculate strides for dense tensor, which are products of the last 'N' dimensions times
     *   the element size
     */
    for (i = self->ar.rank - 2; i >= 0; --i) {
        self->ar.strides[i] = self->ar.strides[i + 1] * self->ar.dims[i + 1];
    }

    ks_size_t total_sz = self->ar.dtype->size;
    for (i = 0; i < self->ar.rank; ++i) total_sz *= self->ar.dims[i];

    self->ar.data = ks_malloc(total_sz);
    if (!self->ar.data) {
        KS_DECREF(self);
        KS_THROW(kst_Error, "Failed to allocate tensor of large size");
        return NULL;
    }

    
    if (data) {
        /* Initialize with memory copying */
        if (!nx_apply_elem(kern_copy, 2, (nxar_t[]){ self->ar, NXAR_(data, dtype, rank, dims, strides) }, NULL));
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
            *resp = nx_array_newc(tp, dtype, dep, *dims, NULL, NULL);
        } else {
            /* already created, so ensure that the element was at maximum depth */
            if (dep != (*resp)->ar.rank) {
                KS_THROW(kst_SizeError, "Initializing entries had differing dimensions");
                return false;
            }
        }

        /* Convert object */
        if (!nx_dtype_enc(dtype, cur, NX_gep((*resp)->ar.data, dtype->size, my_idx))) {
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
        *resp = nx_array_newc(tp, dtype, *max_dep + 1, *dims, NULL, NULL);
    }

    return true;
}


nx_array nx_array_newo(ks_type tp, kso obj, nx_dtype dtype) {
    if (kso_is_iterable(obj)) {
        if (!dtype) dtype = nxd_double;

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
        if (!dtype) dtype = nxd_double;

        nx_array res = nx_array_newc(tp, dtype, 0, NULL, NULL, NULL);
        if (!nx_dtype_enc(dtype, obj, NX_gep(res->ar.data, dtype->size, 0))) {
            return false;
        }

        return res;
    } else if (kso_is_complex(obj)) {
        if (!dtype) dtype = nxd_complexdouble;

        nx_array res = nx_array_newc(tp, dtype, 0, NULL, NULL, NULL);

        if (!nx_dtype_enc(dtype, obj, NX_gep(res->ar.data, dtype->size, 0))) {
            return false;
        }

        return res;
    } else if (kso_is_float(obj)) {
        if (!dtype) dtype = nxd_double;

        nx_array res = nx_array_newc(tp, dtype, 0, NULL, NULL, NULL);

        if (!nx_dtype_enc(dtype, obj, NX_gep(res->ar.data, dtype->size, 0))) {
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

    KS_DECREF(self->ar.dtype);
    ks_free(self->ar.data);
    ks_free(self->ar.dims);
    ks_free(self->ar.strides);
    KSO_DEL(self);

    return KSO_NONE;
}
static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj;
    nx_dtype dtype = nxd_double;
    KS_ARGS("tp:* obj ?dtype:*", &tp, kst_type, &obj, &dtype, nxt_dtype);


    return (kso)nx_array_newo(tp, obj, dtype);
}

static KS_TFUNC(T, str) {
    nx_array self;
    KS_ARGS("self:*", &self, nxt_array);

    /* TODO: add specifics */
    ksio_StringIO sio = ksio_StringIO_new();

    if (!nxar_tostr((ksio_BaseIO)sio, self->ar)) {
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
        ks_tuple res = ks_tuple_newe(self->ar.rank);

        int i;
        for (i = 0; i < self->ar.rank; ++i) {
            res->elems[i] = (kso)ks_int_new(self->ar.dims[i]);
        }

        return (kso)res;
    }
    
    KS_THROW_ATTR(self, attr);
    return NULL;

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

    ));

}



