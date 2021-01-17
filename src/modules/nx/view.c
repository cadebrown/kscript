/* view.c - implementation of the 'nx.view' type
 * 
 *
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/nx.h>

#define T_NAME "nx.view"

/* Internals */


/* C-API */

nx_view nx_view_newo(ks_type tp, nx_t val, kso ref) {
    nx_view self = KSO_NEW(nx_view, tp);

    self->val = val;
    if (ref) KS_INCREF(ref);
    self->ref = ref;

    return self;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    nx_view self;
    KS_ARGS("self:*", &self, nxt_view);

    KS_DECREF(self->val.dtype);
    KS_NDECREF(self->ref);

    KSO_DEL(self);

    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type nxt_view = &tp;

void _ksi_nx_view() {
    
    _ksinit(nxt_view, nxt_array, T_NAME, sizeof(struct nx_view_s), -1, "Multidimesional array view", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

    ));
}



