/* nx/main.c - 'nx' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxt.h>
#include <ks/nxm.h>

#define M_NAME "nx"

/* C-API */

nx_t nx_make(void* data, nx_dtype dtype, int rank, ks_size_t* shape, ks_ssize_t* strides) {
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

nx_t nx_with_newaxis(nx_t from, int axis) {
    assert(from.rank < NX_MAXRANK);
    assert(axis >= 0);
    assert(axis <= from.rank);
    nx_t res = nx_make(from.data, from.dtype, from.rank + 1, from.shape, from.strides);

    /* Scoot over existing shape/strides */
    int i;
    for (i = res.rank - 1; i > axis; --i) {
        res.shape[i] = from.shape[i - 1];
        res.strides[i] = from.strides[i - 1];
    }

    res.shape[axis] = 1;
    res.strides[axis] = 0;

    return res;
}

nx_t nx_without_axes(nx_t self, int naxes, int* axes) {
    nx_t res = self;

    /* Check for quick reductions to a scalar */
    if (naxes >= self.rank) {
        res.rank = 0;
        return res;
    }

    res.rank = self.rank - naxes;

    /* Now, remove axes */
    int i, j, rp = 0;
    for (i = 0; i < self.rank; ++i) {
        bool skip = false;
        for (j = 0; j < naxes; ++j) {
            if (axes[j] == i) {
                skip = true;
                break;
            }
        }
        if (skip) {
        } else {
            res.shape[rp] = self.shape[i];
            res.strides[rp] = self.strides[i];
            rp++;
        }
    }
    //assert(rp == res.rank);

    return res;
}

nx_t nx_with_axes(nx_t self, int naxes, int* axes) {
    nx_t res = self;
    res.rank = 0;

    int i, j, p = 0;
    for (i = 0; i < self.rank + naxes; ++i) {
        bool usethis = false;
        /* TODO: sort method for axes? */
        for (j = 0; j < naxes; ++j) {
            if (axes[j] == i) {
                usethis = true;
                break;
            }
        }
        if (usethis) {
            res.shape[res.rank] = 1;
            res.strides[res.rank] = 0;
            res.rank++;
        } else {
            res.shape[res.rank] = self.shape[p];
            res.strides[res.rank] = self.strides[p];
            res.rank++;
            p++;
        }
    }

    return res;
}

nx_t nx_switch_axes(nx_t self, int a, int b) {
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


nx_t nx_axes_at_end(nx_t self, int naxes, int* axes) {
    assert(naxes <= self.rank);
    nx_t res = self;
    
    int i, j, p = 0;
    for (i = 0; i < self.rank; ++i) {
        bool skip = false;
        for (j = 0; j < naxes; ++j) {
            if (i == axes[j]) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            res.shape[p] = self.shape[i];
            res.strides[p] = self.strides[i];
            p++;
        }
    }

    for (i = 0; i < naxes; ++i) {
        res.shape[p] = self.shape[axes[i]];
        res.strides[p] = self.strides[axes[i]];
        p++;
    }
    assert(p == res.rank);

    return res;
}

nx_t nx_axes_from_end(nx_t self, int naxes, int* axes) {
    assert(naxes <= self.rank);
    nx_t res = self;
    
    int i, j, p = 0, q = self.rank - naxes;
    for (i = 0; i < self.rank; ++i) {
        bool skip = false;
        for (j = 0; j < naxes; ++j) {
            if (i == axes[j]) {
                skip = true;
                break;
            }
        }

        if (skip) {
            /* Take from end */
            res.shape[i] = self.shape[q];
            res.strides[i] = self.strides[q];
            q++;
        } else {
            res.shape[i] = self.shape[p];
            res.strides[i] = self.strides[p];
            p++;
        }
    }

    assert(q == res.rank);

    return res;
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
                        if (j == 1) ksio_add(sio, ",");
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

nx_t nx_getelem(nx_t self, int nargs, kso* args) {
    nx_t res = nx_make(NULL, self.dtype, 0, NULL, NULL);
    ks_uint data = (ks_uint)self.data;

    bool had_dotdotdot = false;
    int i, p = 0;
    for (i = 0; i < nargs; ++i) {
        if (kso_is_int(args[i])) {
            ks_cint v;
            if (!kso_get_ci(args[i], &v)) {
                res.rank = -1;
                return res;
            }

            if (v < 0) {
                v = (v % self.shape[p] + self.shape[p]) % self.shape[p];
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

void* nx_szdot(void* data, int rank, ks_ssize_t* strides, ks_size_t* idxs) {
    ks_uint res = (ks_uint)data;
    int i;
    for (i = 0; i < rank; ++i) {
        res += strides[i] * idxs[i];
    }
    return (void*)res;
}

ks_size_t nx_szprod(int rank, ks_size_t* shape) {
    ks_size_t res = 1;
    int i;
    for (i = 0; i < rank; ++i) {
        res *= shape[i];
    }
    return res;
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
nx_dtype nx_complextype(nx_dtype dtype) {
    if (dtype == nxd_H) {
        return nxd_cH;
    } else if (dtype == nxd_F) {
        return nxd_cF;
    } else if (dtype == nxd_D) {
        return nxd_cD;
    } else if (dtype == nxd_L) {
        return nxd_cL;
    } else if (dtype == nxd_Q) {
        return nxd_cQ;
    } else {
        return nxd_cD;
    }

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

nx_t nx_as_shape(kso obj) {
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


KS_API bool nx_as_axes(nx_t self, kso obj, int* naxes, int* axes) {
    int i, j;
    if (obj == KSO_NONE) {
        *naxes = self.rank;
        for (i = 0; i < *naxes; ++i) {
            axes[i] = i;
        }
        return true;
    } else if (kso_is_int(obj)) {
        ks_cint v;
        if (!kso_get_ci(obj, &v)) return false;
        *naxes = 1;
        if (v >= self.rank) {
            KS_THROW(kst_IndexError, "Invalid axis: %i is out of range", v);
            return false;
        }
        v = (v % self.rank + self.rank) % self.rank;
        axes[0] = v;

        return true;
    } else if (kso_is_iterable(obj)) {
        ks_tuple li = ks_tuple_newi(obj);
        if (!li) return false;

        if (li->len > self.rank) {
            KS_DECREF(li);
            KS_THROW(kst_SizeError, "Iterable of axes contained too many (had %i, expected a max of %i)", (int)li->len, (int)self.rank);
            return false;
        }

        *naxes = li->len;
        for (i = 0; i < *naxes; ++i) {
            ks_cint v;
            if (!kso_get_ci(li->elems[i], &v)) {
                KS_DECREF(li);
                return false;
            }
            if (v >= self.rank) {
                KS_THROW(kst_IndexError, "Invalid axis: %i is out of range", v);
                KS_DECREF(li);
                return false;
            }
            v = (v % self.rank + self.rank) % self.rank;
            for (j = 0; j < i; ++j) {
                if (axes[j] == v) {
                    KS_THROW(kst_IndexError, "Invalid axis: %i specified multiple times", v);
                    KS_DECREF(li);
                    return false;
                }
            }
            axes[i] = v;
        }

        KS_DECREF(li);
        return true;
    } else {
        KS_THROW(kst_TypeError, "Invalid object describing which axes to use, expected 'none', an integer, or an iterable but got '%T' object", obj);
        return false;
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

/* Adds a single character */
#define ADDC(_c) do { \
    if (i < sz) { \
        str[i] = _c; \
    } \
    i++; \
} while (0)

#define ADDS(_s) do { \
    char* _cp = _s; \
    while (*_cp) { \
        ADDC(*_cp); \
        _cp++; \
    } \
} while (0)


#define LOOP(TYPE, NAME) static int my_e2str_##NAME(char* str, int sz, TYPE val, bool sci, int prec, int base) { \
    int i = 0, j, k; /* current position */ \
    if (val != val) { \
        /* 'nan' */ \
        ADDS("nan"); \
        return i; \
    } else if (val == TYPE##INF) { \
        /* 'inf' */ \
        ADDS("inf"); \
        return i; \
    } else if (val == -TYPE##INF) { \
        /* '-inf' */ \
        ADDS("-inf"); \
        return i; \
    } \
    /* Now, we are working with a real number */ \
    bool is_neg = val < 0; \
    if (is_neg) { \
        val = -val; \
        ADDC('-'); \
    } \
    /* Handle 0.0 */ \
    if (val == 0.0) { \
        ADDS("0.0"); \
        return i; \
    } \
    /* Now, val > 0 */ \
    /**/ if (base ==  2) ADDS("0b"); \
    else if (base ==  8) ADDS("0o"); \
    else if (base == 16) ADDS("0x"); \
    int sciexp = 0; \
    /* extract exponent */ \
    if (sci) { \
        while (val >= base) { \
            sciexp++; \
            val /= base; \
        } \
        while (val < 1) { \
            sciexp--; \
            val *= base; \
        } \
    } \
    /* Handle actual digits, break into integer and floating point type */ \
    static const char digc[] = "0123456789ABCDEF"; \
    if (base == 10) { \
        char fmt[64]; \
        int sz_fmt = snprintf(fmt, sizeof(fmt) - 1, "%%.%ilf", prec); \
		assert(sz_fmt <= sizeof(fmt) - 1); \
		char vs[256]; \
		int sz_vs = snprintf(vs, sizeof(vs) - 1, fmt, (double)val); \
		assert(sz_vs <= sizeof(vs) - 1); \
		ADDS(vs); \
    } else { \
        int i_num = i; \
        ks_cfloat vi; \
        /* TODO: use 'modf'? */ \
        ks_cfloat vf = TYPE##fmod(val, 1); \
        vi = val - vf; \
        /* Integral part */ \
        do { \
            ks_cfloat digf = TYPE##fmod(vi, base); \
            int dig = (int)TYPE##floor(digf); \
            vi = (vi - digf) / base; \
            ADDC(digc[dig]); \
        } while (vi > 0.5); \
        /* Reverse digit order */ \
        if (i < sz) for (j = i_num, k = i - 1; j < k; ++j, --k) { \
            char t = str[j]; \
            str[j] = str[k]; \
            str[k] = t; \
        } \
        ADDC('.'); \
        /* Shift over, and generate fractional part */ \
        vf *= base; \
        /* number of digits */ \
        int ndl = prec <= 0 ? sz - i : prec; \
        do { \
            ks_cfloat digf = TYPE##floor(vf); \
            int dig = (((int)digf) % base + base) % base; \
            assert(dig >= 0 && dig < base);  \
            vf = (vf - digf) * base; \
            ADDC(digc[dig]); \
        } while (vf > 1e-9 && ndl-- > 0); \
    } \
    /* Now, remove trailing zeros */ \
    if (i < sz) { \
        while (i > 2 && str[i - 1] == '0' && str[i - 2] != '.') i--; \
    } \
    /* add sciexp */ \
    if (sci) { \
        ADDC('e'); \
        ADDC(sciexp >= 0 ? '+' : '-'); \
        if (sciexp < 0) sciexp = -sciexp; \
        j = i; \
        /* always exponent in base-10 */ \
        do { \
            int sdig = sciexp % 10; \
            ADDC(digc[sdig]); \
            sciexp /= 10; \
        } while (sciexp > 0); \
        /* Reverse digits */ \
        if (i < sz) for (k = i - 1; j < k; j++, k--) { \
            char t = str[j]; \
            str[j] = str[k]; \
            str[k] = t; \
        } \
    } \
    return i; \
}

NXT_PASTE_F(LOOP)
#undef LOOP

#define LOOP(TYPE, NAME) \
static char my_strfrom_fmt_##NAME[256];
NXT_PASTE_F(LOOP)
#undef LOOP


/* Adds a single element to the IO object */
static bool my_getstr_addelem(ksio_BaseIO bio, nx_dtype dtype, void* ptr) {
    /*if (dtype == nxd_bl) {
        return ksio_add(bio, "%s", *(nx_bl*)ptr ? "true" : "false");
    }*/
    
    char tmp[256];
    #define LOOP(TYPE) do { \
        return ksio_add(bio, "%l", (ks_cint)*(nx_##TYPE*)ptr); \
    } while (0);
    NXT_FOR_I(dtype, LOOP);
    #undef LOOP

    #define LOOP(TYPE) do { \
        int sz = nx_##TYPE##strfrom(tmp, sizeof(tmp) - 1, my_strfrom_fmt_##TYPE, *(nx_##TYPE*)ptr); \
        assert(sz < sizeof(tmp) - 1); \
        while (sz > 3 && tmp[sz - 1] == '0' && tmp[sz - 2] != '.') sz--; \
        /* int sz = my_e2str_##TYPE(tmp, sizeof(tmp) - 1, *(nx_##TYPE*)ptr, false, nx_##TYPE##DIG - 2, 10); */ \
        return ksio_addbuf(bio, sz, tmp); \
    } while (0);
    NXT_FOR_F(dtype, LOOP);
    #undef LOOP
    
    #define LOOP(TYPE) do { \
        nx_##TYPE v = *(nx_##TYPE*)ptr; \
        my_getstr_addelem(bio, nx_##TYPE##rdtype, &v.re); \
        if (v.im >= 0 || v.im != v.im || v.im == nx_##TYPE##rINF) { ksio_add(bio, "+"); } \
        my_getstr_addelem(bio, nx_##TYPE##rdtype, &v.im); \
        ksio_add(bio, "i"); \
    } while (0);
    NXT_FOR_C(dtype, LOOP);
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

bool nx_getstr(ksio_BaseIO bio, nx_t X) {
    return my_getstr(bio, X, 0);
}

bool nx_enc(nx_dtype dtype, kso obj, void* out) {
    if (dtype->kind == NX_DTYPE_INT) {
        ks_cint val;
        if (!kso_get_ci(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            *(nx_##TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_FOR_I(dtype, LOOP);
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_FLOAT) {
        ks_cfloat val;
        if (!kso_get_cf(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            *(nx_##TYPE*)out = val; \
            return true; \
        } while (0);
        NXT_FOR_F(dtype, LOOP);
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_COMPLEX) {
        ks_ccomplex val;
        if (!kso_get_cc(obj, &val)) return false;

        #define LOOP(TYPE) do { \
            ((nx_##TYPE*)out)->re = val.re; \
            ((nx_##TYPE*)out)->im = val.im; \
            return true; \
        } while (0);
        NXT_FOR_C(dtype, LOOP);
        #undef LOOP
    }

    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}



/* Module Functions */


static KS_TFUNC(M, zeros) {
    kso shape = KSO_NONE;
    nx_dtype dtype = nxd_D;
    KS_ARGS("?shape ?dtype:*", &shape, &dtype, nxt_dtype);

    nx_t ns = nx_as_shape(shape);
    if (ns.rank < 0) return NULL;

    nx_array res = nx_array_newc(nxt_array, NULL, dtype, ns.rank, ns.shape, NULL);
    if (!nx_zero(res->val)) {
        KS_DECREF(res);
        return NULL;
    }

    return (kso)res;
}

static KS_TFUNC(M, onehot) {
    ks_cint newdim;
    kso x, r = KSO_NONE;
    KS_ARGS("x newdim:cint ?r", &x, &newdim, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = nxd_bl;
        nx_t shape = nx_with_newaxis(vX, vX.rank);

        shape.shape[vX.rank] = newdim;
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

    if (!nx_onehot(vX, vR)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}


static KS_TFUNC(M, sum) {
    kso x, oaxes = KSO_NONE, r = KSO_NONE;
    KS_ARGS("x ?axes ?r", &x, &oaxes, &r);

    nx_t vX, vR;
    kso rX, rR;

    if (!nx_get(x, NULL, &vX, &rX)) {
        return NULL;
    }

    if (vX.rank < 1) {
        KS_THROW(kst_SizeError, "Expected argument to be at least 1-D");
        KS_NDECREF(rX);
        return NULL;
    }

    int naxes;
    int axes[NX_MAXRANK];
    if (!nx_as_axes(vX, oaxes, &naxes, axes)) {
        KS_NDECREF(rX);
        return NULL;
    }

    if (r == KSO_NONE) {
        /* Generate output */
        nx_dtype dtype = vX.dtype;
        /* Complex arguments should create real ones for abs */

        nx_t shape = nx_without_axes(vX, naxes, axes);
        
        //shape.rank = 0;

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

    if (!nx_sum(vX, vR, naxes, axes)) {
        KS_NDECREF(rX);
        KS_NDECREF(rR);
        KS_DECREF(r);
        return NULL;
    }

    KS_NDECREF(rX);
    KS_NDECREF(rR);
    return r;
}



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
        } else if (dtype == nxd_cL) {
            dtype = nxd_L;
        } else if (dtype == nxd_cQ) {
            dtype = nxd_Q;
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
    _ksi_nx_array();
    _ksi_nx_dtype();
    _ksi_nx_view();

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "NumeriX module", KS_IKV(

        /* Submodules */
        {"rand",                   (kso)_ksi_nxrand()},
        {"la",                     (kso)_ksi_nx_la()},
        {"fft",                    (kso)_ksi_nx_fft()},
        
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
        {"Q",                      KS_NEWREF(nxd_Q)},

        {"cH",                     KS_NEWREF(nxd_cH)},
        {"cF",                     KS_NEWREF(nxd_cF)},
        {"cD",                     KS_NEWREF(nxd_cD)},
        {"cL",                     KS_NEWREF(nxd_cL)},
        {"cQ",                     KS_NEWREF(nxd_cQ)},

        {"half",                   KS_NEWREF(nxd_H)},
        {"float",                  KS_NEWREF(nxd_F)},
        {"double",                 KS_NEWREF(nxd_D)},
        {"quad",                   KS_NEWREF(nxd_Q)},
        {"fp128",                  KS_NEWREF(nxd_Q)},

        /* Functions */

        {"zeros",                  ksf_wrap(M_zeros_, M_NAME ".zeros(shape=none, dtype=nx.double)", "Create an array of zeros")},
        {"onehot",                 ksf_wrap(M_onehot_, M_NAME ".onehot(x, newdim, r=none)", "Computes one-hot encoding, where 'x' are the indices, 'newdim' is the new dimension which the indices point to\n\n    Indices in 'x' are taken modulo 'newdim'")},

        {"sum",                    ksf_wrap(M_sum_, M_NAME ".sum(x, axes=none, r=none)", "Sum elements")},

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

    snprintf(my_strfrom_fmt_H, sizeof(my_strfrom_fmt_H) - 1, "%%.%if", nx_HDIG - 2);
    snprintf(my_strfrom_fmt_F, sizeof(my_strfrom_fmt_F) - 1, "%%.%if", nx_FDIG - 2);
    snprintf(my_strfrom_fmt_D, sizeof(my_strfrom_fmt_D) - 1, "%%.%if", nx_DDIG - 2);
    snprintf(my_strfrom_fmt_L, sizeof(my_strfrom_fmt_L) - 1, "%%.%if", nx_LDIG - 2);
    snprintf(my_strfrom_fmt_Q, sizeof(my_strfrom_fmt_Q) - 1, "%%.%if", nx_QDIG - 2);

    return res;
}
