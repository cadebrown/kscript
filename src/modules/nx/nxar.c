/* nxar.c - implementation of various 'nxar_t' functionality
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>



/* internal method */
bool I_tostr(ksio_BaseIO sb, nxar_t A, int dep) {

    /* truncation size for max length of a single vector */
    int trunc_sz = 20;

    /* Adds an element of the dtype to the result */
    #define ADDELEM(_ptr) do { \
        void* ptr = (_ptr); \
        if (A.dtype == nxd_double) { \
            ksio_add(sb, "%f", (ks_cfloat)*(double*)ptr); \
        } else if (A.dtype == nxd_float) { \
            ksio_add(sb, "%f", (ks_cfloat)*(float*)ptr); \
        } else if (A.dtype == nxd_complexfloat) { \
            ks_cfloat re = ((float*)ptr)[0], im = ((float*)ptr)[1]; \
            bool is_neg = im < 0; \
            if (is_neg) im = -im; \
            ksio_add(sb, "%f%s%fi", re, is_neg ? "-" : "+", im); \
        } else if (A.dtype == nxd_complexdouble) { \
            ks_cfloat re = ((double*)ptr)[0], im = ((double*)ptr)[1]; \
            bool is_neg = im < 0; \
            if (is_neg) im = -im; \
            ksio_add(sb, "%f%s%fi", re, is_neg ? "-" : "+", im); \
        } else { \
            /* Unknown! */ \
        } \
    } while (0)

    if (A.rank == 0) {
        ADDELEM(A.data);

        return true;

    } else if (A.rank == 1) {
        /* 1d, output a list-like structure */
        ksio_add(sb, "[");

        ks_size_t i;
        for (i = 0; i < A.dims[0]; ++i) {
            if (i > 0) ksio_add(sb, ", ");

            ADDELEM(NX_gep(A.data, A.strides[0], i));
        }
        return ksio_add(sb, "]");
    } else {
        /* must use recursion */

        ksio_add(sb, "[");

        /* loop over outer dimension, adding each inner dimension*/
        ks_size_t i;
        nxar_t inner = NXAR_(A.data, A.dtype, A.rank-1, A.dims+1, A.strides+1, NULL);
        for (i = 0; i < A.dims[0]; ++i, inner.data = (void*)((ks_uint)inner.data + A.strides[0])) {
            if (i > 0) ksio_add(sb, ",\n%.*c", dep+1, ' ');

            if (!I_tostr(sb, inner, dep+1)) return false;
        }

        return ksio_add(sb, "]");
    }
}

bool nxar_tostr(ksio_BaseIO bio, nxar_t A) {
    return I_tostr(bio, A, 0);
}

bool nxar_get(kso obj, nx_dtype dtype, nxar_t* res, kso* ref) {
    if (kso_issub(obj->type, nxt_array)) {
        /* Already exists, TODO: check if cast is needed */
        *res = ((nx_array)obj)->ar;
        *ref = NULL;
        return true;
    } else if (kso_issub(obj->type, nxt_view)) {
        /* Already exists, TODO: check if cast is needed */
        *res = ((nx_view)obj)->ar;
        *ref = NULL;
        return true;
    } else {

        nx_array newarr = nx_array_newo(nxt_array, obj, dtype);
        if (!newarr) {
            return false;
        }
        *res = newarr->ar;

        /* Return reference */
        *ref = (kso)newarr;
        return true;
    }
}

