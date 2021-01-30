/* nx/nx.c - 'nx_t' manipulation
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

nx_t nx_make(void* data, nx_dtype dtype, int rank, ks_size_t* shape, ks_ssize_t* strides) {
    nx_t self;
    self.data = data;
    self.dtype = dtype;
    self.rank = rank;

    /* Set shape */
    memcpy(self.shape, shape, sizeof(*shape) * rank);

    int i;
    if (strides) {
        for (i = 0; i < rank; ++i) {
            self.strides[i] = strides[i];
        }
    } else if (rank > 0) {
        /* Fill in with densely packed strides  */
        self.strides[rank - 1] = dtype ? dtype->size : 1;
        for (i = rank - 2; i >= 0; --i) {
            self.strides[i] = self.strides[i + 1] * shape[i + 1];
        }
    }

    return self;
}


nx_t nx_newaxis(nx_t self, int axis) {
    assert(axis >= 0);
    assert(axis <= self.rank);

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank + 1;

    /* Copy shared dimensions */
    int i, j = 0;
    for (i = 0; i < axis; ++i, ++j) {
        res.shape[i] = self.shape[j];
        res.strides[i] = self.strides[j];
    }

    /* Have this dimension not affect the contents overall */
    res.shape[i] = 1;
    res.strides[i] = 0;
    i++;

    /* Copy other dimensions */
    for (; i < res.rank; ++i, ++j) {
        res.shape[i] = self.shape[j];
        res.strides[i] = self.strides[j];
    }

    return res;
}

nx_t nx_newaxes(nx_t self, int naxes, int* axes) {
    if (naxes == 0) return self;

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank + naxes;

    /* Sort axes (so internal algorithm is faster) */
    int axess[NX_MAXRANK];
    memcpy(axess, axes, naxes * sizeof(*axes));
    nx_intsort(naxes, axess);

    int i, j = 0, p = 0;
    for (i = 0; i < res.rank; ++i) {
        if (p < naxes && i == axess[p]) {
            /* Create new axis */
            res.shape[i] = 1;
            res.strides[i] = 0;
            p++;
        } else {
            /* Copy from existing */
            res.shape[i] = self.shape[j];
            res.strides[i] = self.strides[j];
            j++;
        }
    }

    assert(j == self.rank);
    return res;
}


nx_t nx_delaxis(nx_t self, int axis) {
    assert(axis >= 0);
    assert(axis < self.rank);

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank - 1;

    /* Copy shared dimensions */
    int i, j = 0;
    for (i = 0; i < axis; ++i, ++j) {
        res.shape[i] = self.shape[j];
        res.strides[i] = self.strides[j];
    }

    /* Skip other dimension */
    j++;

    /* Copy other dimensions */
    for (; i < res.rank; ++i, ++j) {
        res.shape[i] = self.shape[j];
        res.strides[i] = self.strides[j];
    }

    return res;
}

nx_t nx_delaxes(nx_t self, int naxes, int* axes) {
    if (naxes == 0) return self;

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank - naxes;

    /* Sort axes (so internal algorithm is faster) */
    int axess[NX_MAXRANK];
    memcpy(axess, axes, naxes * sizeof(*axes));
    nx_intsort(naxes, axess);

    int i = 0, j, p = 0;
    for (j = 0; j < self.rank; ++j) {
        if (p < naxes && j == axess[p]) {
            /* Skip axis */
            p++;
        } else {
            /* Copy from existing */
            res.shape[i] = self.shape[j];
            res.strides[i] = self.strides[j];
            i++;
        }
    }

    return res;
}


nx_t nx_swapaxes(nx_t self, int a, int b) {
    assert(a >= 0 && a < self.rank);
    assert(b >= 0 && b < self.rank);

    if (a == b) return self;

    nx_t res = self;

    ks_size_t tu = res.shape[a];
    ks_ssize_t tv = res.strides[a];

    res.shape[a] = res.shape[b];
    res.strides[a] = res.strides[b];

    res.shape[b] = tu;
    res.strides[b] = tv;

    return res;
}


nx_t nx_sinkaxes(nx_t self, int naxes, int* axes) {
    assert(naxes <= self.rank);

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank;

    /* Sort axes (so internal algorithm is faster) */
    int axess[NX_MAXRANK];
    memcpy(axess, axes, naxes * sizeof(*axes));
    nx_intsort(naxes, axess);

    int i = 0, j, p = 0;
    for (j = 0; j < self.rank; ++j) {
        if (p < naxes && j == axess[p]) {
            /* Save axis for later */
            p++;
        } else {
            /* Copy from existing */
            res.shape[i] = self.shape[j];
            res.strides[i] = self.strides[j];
            i++;
        }
    }

    /* Copy the last from the axes */
    for (p = 0; p < naxes; ++p, ++i) {
        res.shape[i] = self.shape[axes[p]];
        res.strides[i] = self.strides[axes[p]];
    }

    return res;
}

nx_t nx_unsinkaxes(nx_t self, int naxes, int* axes) {
    assert(naxes <= self.rank);

    nx_t res;
    res.data = self.data;
    res.dtype = self.dtype;
    res.rank = self.rank;

    /* Sort axes (so internal algorithm is faster) */
    int axess[NX_MAXRANK];
    memcpy(axess, axes, naxes * sizeof(*axes));
    nx_intsort(naxes, axess);

    int i, j = 0, p = 0, q = self.rank - naxes;
    for (i = 0; i < res.rank; ++i) {
        if (p < naxes && i == axess[p]) {
            /* Take from end */
            res.shape[i] = self.shape[q];
            res.strides[i] = self.strides[q];
            q++;
            p++;
        } else {
            /* Copy from existing */
            res.shape[i] = self.shape[j];
            res.strides[i] = self.strides[j];
            j++;
        }
    }

    assert(j == self.rank);
    return res;
}

nx_t nx_getevo(nx_t self, int nargs, kso* args) {
    nx_t res;
    ks_uint data = (ks_uint)self.data;
    res.dtype = self.dtype;
    res.rank = 0;

    bool had_dotdotdot = false;
    int i, p = 0;
    for (i = 0; i < nargs; ++i) {
        if (kso_is_int(args[i])) {
            ks_cint v;
            if (!kso_get_ci(args[i], &v)) {
                res.rank = -1;
                return res;
            }
            ks_cint dim = self.shape[p];

            if (v < 0) {
                v = ((v % dim) + dim) % dim;
            } else if (v >= self.shape[p]) {
                KS_THROW(kst_IndexError, "Index #%i out of range (value: %i)", i, (int)v);
                res.rank = -1;
                return res;
            }

            data += self.strides[p] * v;
            p++;
        } else if (kso_issub(args[i]->type, kst_slice)) {
            ks_cint start, stop, delta;
            if (!ks_slice_get_citer((ks_slice)args[i], self.shape[p], &start, &stop, &delta)) {
                res.rank = -1;
                return res;
            }

            res.rank += 1;
            data += self.strides[p] * start;
            res.shape[res.rank - 1] = (stop - start) / delta;
            res.strides[res.rank - 1] = self.strides[p] * delta;
            p++;

        } else if (args[i] == KSO_DOTDOTDOT) {
            if (had_dotdotdot) {
                KS_THROW(kst_IndexError, "Only one '...' allowed per indexing operation");
                res.rank = -1;
                return res;
            }
            had_dotdotdot = true;

            int nmid = self.rank - nargs + 1;
            if (nmid < 0) {
                KS_THROW(kst_IndexError, "Too many indices!");
                res.rank = -1;
                return res;
            }

            /* Fill in middles */

            int j;
            for (j = 0; j < nmid; ++j) {
                res.rank += 1;
                res.shape[res.rank - 1] = self.shape[p];
                res.strides[res.rank - 1] = self.strides[p];
                p++;
            }

        } else {
            KS_THROW(kst_TypeError, "Objects of type '%T' cannot be used as indices", args[i]);
            res.rank = -1;
            return res; 
        }
    }

    for (; p < self.rank; ++p) {
        res.rank += 1;
        res.shape[res.rank - 1] = self.shape[p];
        res.strides[res.rank - 1] = self.strides[p];
    }

    res.data = (void*)data;
    return res;
}

/* Adds a single element to the IO object */
static bool my_getstr_addelem(ksio_BaseIO bio, nx_dtype dtype, void* ptr) {
    /*if (dtype == nxd_bl) {
        return ksio_add(bio, "%s", *(nx_bl*)ptr ? "true" : "false");
    }*/
    char tmp[256];

    if (false) {

    }
    else if (dtype == nxd_bl) {
        return ksio_addbuf(bio, 1, *(nx_bl*)ptr ? "1" : "0");
    }

    #define SHOULD_SCI(_val) (_val != 0 && ((_val > 1e9 || _val < -1e9) || (_val < 1e-9 && _val > -1e-9)))

    /* Integers */
    #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
        return ksio_add(bio, "%l", (ks_cint)*(TYPE*)ptr); \
    }
    NXT_PASTE_I(LOOP)
    #undef LOOP

    /* Floats */
    #define _GENFMTSTR_SCI(_dig) "%." #_dig "e"
    #define _GENFMTSTR(_dig) "%." #_dig "f"
    #define GENFMTSTR_SCI(_dig) _GENFMTSTR_SCI(_dig)
    #define GENFMTSTR(_dig) _GENFMTSTR(_dig)

    #define LOOP(TYPE, NAME) if (dtype == nxd_##NAME) { \
        TYPE val = *(TYPE*)ptr; \
        int sz = -1; \
        if (SHOULD_SCI(val)) { \
            sz = TYPE##strfrom(tmp, sizeof(tmp) - 1, GENFMTSTR_SCI(TYPE##DIG), val); \
        } else { \
            sz = TYPE##strfrom(tmp, sizeof(tmp) - 1, GENFMTSTR(TYPE##DIG), val); \
        } \
        assert(sz < sizeof(tmp) - 1); \
        while (sz > 3 && tmp[sz - 1] == '0' && tmp[sz - 2] != '.') sz--; \
        return ksio_addbuf(bio, sz, tmp); \
    }
    NXT_PASTE_F(LOOP)
    #undef LOOP

    #define LOOP(TYPE, NAME) if (dtype == nxd_##NAME) { \
        TYPE v = *(TYPE*)ptr; \
        my_getstr_addelem(bio, TYPE##rdtype, &v.re); \
        if (v.im >= 0 || v.im != v.im || v.im == TYPE##rINF) { ksio_add(bio, "+"); } \
        my_getstr_addelem(bio, TYPE##rdtype, &v.im); \
        ksio_add(bio, "i"); \
    }
    NXT_PASTE_C(LOOP)
    #undef LOOP

    return true;
}

/* Internal method */
bool my_getstr(ksio_BaseIO bio, nx_t X, int dep) {

    /* truncation threshold */
    int trunc_thresh = 64;

    /* truncation size for max length of a single vector */
    int trunc_sz = 8;

    if (X.rank == 0) {
        return my_getstr_addelem(bio, X.dtype, X.data);
    } else if (X.rank == 1) {
        /* 1d, output a list-like structure */
        ksio_add(bio, "[");
        ks_size_t i;

        if (X.shape[0] <= trunc_thresh) {
            for (i = 0; i < X.shape[0]; ++i) {
                if (i > 0) ksio_add(bio, ", ");
                if (!my_getstr_addelem(bio, X.dtype, (void*)((ks_uint)X.data + X.strides[0] * i))) return false;
            }
        } else {
            /* Truncate elements */
            int nb = trunc_sz / 2;
            for (i = 0; i < nb; ++i) {
                if (i > 0) ksio_add(bio, ", ");
                if (!my_getstr_addelem(bio, X.dtype, (void*)((ks_uint)X.data + X.strides[0] * i))) return false;
            }

            ksio_add(bio, ", ...");

            for (i = X.shape[0] - nb; i < X.shape[0]; ++i) {
                if (i > 0) ksio_add(bio, ", ");
                if (!my_getstr_addelem(bio, X.dtype, (void*)((ks_uint)X.data + X.strides[0] * i))) return false;
            }

        }

        return ksio_add(bio, "]");

    } else {
        /* Use recursion */

        ksio_add(bio, "[");
        /* loop over outer dimension, adding each inner dimension*/
        ks_size_t i;
        nx_t inner = nx_make(X.data, X.dtype, X.rank-1, X.shape+1, X.strides+1);

        if (X.shape[0] <= trunc_thresh) {
            for (i = 0; i < X.shape[0]; ++i) {
                if (i > 0) ksio_add(bio, ",\n%.*c", dep+1, ' ');
                inner.data = (void*)(((ks_uint)X.data) + X.strides[0] * i);
                if (!my_getstr(bio, inner, dep+1)) return false;
            }

        } else {
            /* Truncate elements */
            int nb = trunc_thresh / 2;
            for (i = 0; i < nb; ++i) {
                if (i > 0) ksio_add(bio, ",\n%.*c", dep+1, ' ');
                inner.data = (void*)(((ks_uint)X.data) + X.strides[0] * i);
                if (!my_getstr(bio, inner, dep+1)) return false;
            }

            ksio_add(bio, ",\n%.*c...", dep+1, ' ');

            for (i = X.shape[0] - nb; i < X.shape[0]; ++i) {
                if (i > 0) ksio_add(bio, ",\n%.*c", dep+1, ' ');
                inner.data = (void*)(((ks_uint)X.data) + X.strides[0] * i);
                if (!my_getstr(bio, inner, dep+1)) return false;
            }
        }

        return ksio_add(bio, "]");
    }
}

bool nx_getstr(ksio_BaseIO io, nx_t self) {
    return my_getstr(io, self, 0);
}

ks_str nx_getbs(nx_t x) {
    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add(sio, "<nx_t data=%p, dtype=%R, rank=%i, shape=(", x.data, x.dtype, x.rank);

    int i;
    for (i = 0; i < x.rank; ++i) {
        if (i > 0) ksio_add(sio, ", ");
        ksio_add(sio, "%u", x.shape[i]);
    }
    if (x.rank < 2) ksio_add(sio, ","); 
    ksio_add(sio, "), strides=(");
    for (i = 0; i < x.rank; ++i) {
        if (i > 0) ksio_add(sio, ", ");
        ksio_add(sio, "%l", x.strides[i]);
    }
    if (x.rank < 2) ksio_add(sio, ","); 
    ksio_add(sio, ")>"); 

    return ksio_StringIO_getf(sio);
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

bool nx_getas(nx_t self, nx_dtype dtype, nx_t* res, void** tofree) {
    *tofree = NULL;
    if (self.dtype == dtype) {
        *res = self;
        return true;
    } else {
        /* Attempt to create new array and cast it */
        *tofree = ks_malloc(dtype->size * szprod(self.rank, self.shape));
        if (!*tofree) {
            KS_THROW(kst_Error, "Failed to allocate array...");
            return NULL;
        }

        if (!nx_cast(
            self,
            *res = nx_make(*tofree, dtype, self.rank, self.shape, NULL)
        )) {
            ks_free(*tofree);
            *tofree = NULL;
            return false;
        }

        return true;
    }
}




