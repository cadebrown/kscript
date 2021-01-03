/* util.c - various numeric utilities
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

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

bool nx_getcast(nxar_t x, nx_dtype to, nxar_t* r, kso* ref) {
    if (x.dtype == to) {
        *r = x;
        *ref = NULL;
        return true;
    } else {
        /* Attempt to create new array and cast it */
        nx_array res = nx_array_newc(nxt_array, to, x.rank, x.dims, x.strides, NULL);
        if (!res) return false;

        if (!nx_cast(res->ar, x)) {
            KS_DECREF(res);
            return false;
        }

        *r = res->ar;
        *ref = (kso)res;
        return true;
    }

}

