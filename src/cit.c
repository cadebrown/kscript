
/* cit.c - implementation of the C-iterator wrapper
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>


ks_cit ks_cit_make(kso obj) {
    ks_cit cit;

    cit.it = kso_iter(obj);
    cit.exc = cit.it == NULL;

    return cit;
}

void ks_cit_done(ks_cit* cit) {
    if (cit->it) {
        KS_DECREF(cit->it);
        cit->it = NULL;
    }
}

kso ks_cit_next(ks_cit* cit) {
    /* Don't yield anymore if something has been sent */
    if (cit->exc || !cit->it) return NULL;

    kso res = kso_next(cit->it);
    if (res) {
        /* Returns the reference */
        return res;
    } else {
        ksos_thread th = ksos_thread_get();
        if (th->exc->type == kst_OutOfIterException) {
            /* Out of elements, which is fine */
            kso_catch_ignore();
            KS_DECREF(cit->it);
            cit->it = NULL;
            return NULL;
        } else {
            /* Had other exception */
            cit->exc = true;
            return NULL;
        }
    }
}
