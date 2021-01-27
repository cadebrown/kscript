/* nx/util.c - utility functions
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nxi.h>
#include <ks/nxt.h>

bool nx_enc(nx_dtype dtype, kso obj, void* out) {
    if (dtype->kind == NX_DTYPE_INT) {
        ks_cint val;
        if (!kso_get_ci(obj, &val)) return false;

        if (false) {}
        #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
            *(TYPE*)out = val; \
            return true; \
        }
        NXT_PASTE_I(LOOP)
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_FLOAT) {
        ks_cfloat val;
        if (!kso_get_cf(obj, &val)) return false;

        #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
            *(TYPE*)out = val; \
            return true; \
        }
        NXT_PASTE_F(LOOP)
        #undef LOOP
    } else if (dtype->kind == NX_DTYPE_COMPLEX) {
        ks_ccomplex val;
        if (!kso_get_cc(obj, &val)) return false;

        #define LOOP(TYPE, NAME) else if (dtype == nxd_##NAME) { \
            ((TYPE*)out)->re = val.re; \
            ((TYPE*)out)->im = val.im; \
            return true; \
        }
        NXT_PASTE_C(LOOP)
        #undef LOOP
    }

    KS_THROW(kst_TypeError, "Unsupported dtype: %R", dtype);
    return false;
}

/* Internal quicksort algorithm */
void my_intsort(int* vals, int l, int r) {
    int t;
    if (l >= r) {
        return;
    }
    /* Quicksort */

    /* Select a pivot */
    int p = l + (r - l) / 2;
    int pv = vals[p];

    int li = l, ri = r;
    while (li <= ri) {
        while (vals[li] < pv) {
            li++;
        }
        while (vals[ri] > pv) {
            ri--;
        }
        if (li <= ri) {
            /* Swap */
            t = vals[li];
            vals[li] = vals[ri];
            vals[ri] = t;

            li++;
            ri--;
        }
    }

    /* Recursively sort halves */
    my_intsort(vals, l, ri);
    my_intsort(vals, li, r);
}

void nx_intsort(int nvals, int* vals) {
    my_intsort(vals, 0, nvals - 1);
}


nx_dtype nx_resnum(nx_dtype X, nx_dtype Y) {
    return nx_resnume(X, Y, false, false);
}

/* Tests wheter '_dtype' is a builtin numeric type (i.e. is 'int', 'float', or 'complex' kind) */
#define NX_ISNUM(_dtype) ((_dtype)->kind == NX_DTYPE_INT || (_dtype)->kind == NX_DTYPE_FLOAT || (_dtype)->kind == NX_DTYPE_COMPLEX)

nx_dtype nx_resnume(nx_dtype X, nx_dtype Y, bool r2c, bool c2r) {
    if (!X && Y) {
        return nx_resnume(Y, X, r2c, c2r);
    } else if (!X && !Y) {
        return nxd_D;
    } else if (X && !Y) {
        return X->kind == NX_DTYPE_INT ? nxd_D : X;
    }

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

nx_dtype nx_realtype(nx_dtype X) {
    if (X->kind == NX_DTYPE_FLOAT) {
        return X;
    } else if (X->kind == NX_DTYPE_COMPLEX) {
        if (X == nxd_cS) {
            return nxd_S;
        } else if (X == nxd_cD) {
            return nxd_D;
        } else if (X == nxd_cE) {
            return nxd_E;
        } else if (X == nxd_cQ) {
            return nxd_Q;
        }
    } else if (X->kind == NX_DTYPE_INT) {
        return nxd_D;
    }
    KS_THROW(kst_TypeError, "Failed to get real type from: %R", X);
    return NULL;
}

nx_dtype nx_complextype(nx_dtype X) {
    if (X->kind == NX_DTYPE_FLOAT) {
        if (X == nxd_S) {
            return nxd_cS;
        } else if (X == nxd_D) {
            return nxd_cD;
        } else if (X == nxd_E) {
            return nxd_cE;
        } else if (X == nxd_Q) {
            return nxd_cQ;
        }
    } else if (X->kind == NX_DTYPE_COMPLEX) {
        return X;
    } else if (X->kind == NX_DTYPE_INT) {
        return nxd_cD;
    }
    KS_THROW(kst_TypeError, "Failed to get complex type from: %R", X);
    return NULL;
}


bool nx_getshape(kso obj, int* rank, ks_size_t* shape) {
    if (kso_issub(obj->type, kst_none)) {
        /* Scalar */
        *rank = 0;
        return true;
    } else if (kso_is_int(obj)) {
        /* 1D array */
        *rank = 1;
        ks_cint x;
        if (!kso_get_ci(obj, &x)) {
            return false;
        }

        shape[0] = x;
        return true;
    } else {
        /* Assume iterable */
        ks_list l = ks_list_newi(obj);
        if (!l) {
            return false;
        }

        *rank = l->len;

        int i;
        for (i = 0; i < l->len; ++i) {
            kso ob = l->elems[i];
            ks_cint x;
            if (!kso_get_ci(ob, &x)) {
                KS_DECREF(l);
                return false;
            }

            shape[i] = x;
        }
        KS_DECREF(l);

        return true;
    }   
}

bool nx_getshapev(int nargs, kso* args, int* rank, ks_size_t* shape) {
    *rank = nargs;

    int i;
    for (i = 0; i < nargs; ++i) {
        kso ob = args[i];
        ks_cint x;
        if (!kso_get_ci(ob, &x)) {
            return false;
        }

        shape[i] = x;
    }

    return true;
}


bool nx_getaxes(kso obj, int rank, int* naxes, int* axes) {
    int i, j;
    if (obj == KSO_NONE) {
        *naxes = rank;
        for (i = 0; i < *naxes; ++i) {
            axes[i] = i;
        }
        return true;
    } else if (kso_is_int(obj)) {
        ks_cint v;
        if (!kso_get_ci(obj, &v)) return false;
        *naxes = 1;
        if (v >= rank) {
            KS_THROW(kst_IndexError, "Invalid axis: %i is out of range", v);
            return false;
        }
        v = (v % rank + rank) % rank;
        axes[0] = v;

        return true;
    } else if (kso_is_iterable(obj)) {
        ks_tuple li = ks_tuple_newi(obj);
        if (!li) return false;

        if (li->len > rank) {
            KS_DECREF(li);
            KS_THROW(kst_SizeError, "Iterable of axes contained too many (had %i, expected a max of %i)", (int)li->len, (int)rank);
            return false;
        }

        *naxes = li->len;
        for (i = 0; i < *naxes; ++i) {
            ks_cint v;
            if (!kso_get_ci(li->elems[i], &v)) {
                KS_DECREF(li);
                return false;
            }
            if (v >= rank) {
                KS_THROW(kst_IndexError, "Invalid axis: %i is out of range", v);
                KS_DECREF(li);
                return false;
            }
            v = (v % rank + rank) % rank;
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


