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
