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


/* Type Functions */

static KS_TFUNC(T, free) {
    nx_view self;
    KS_ARGS("self:*", &self, nxt_view);

    KS_DECREF(self->ar.dtype);
    ks_free(self->ar.dims);
    ks_free(self->ar.strides);
    KS_DECREF(self->ar.obj);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    nx_view self;
    KS_ARGS("self:*", &self, nxt_view);

    /* TODO: add specifics */
    ksio_StringIO sio = ksio_StringIO_new();

    if (!nxar_tostr((ksio_BaseIO)sio, self->ar)) {
        KS_DECREF(sio);
        return NULL;
    }

    return (kso)ksio_StringIO_getf(sio);
}


/* Export */

static struct ks_type_s tp;
ks_type nxt_view = &tp;

void _ksi_nx_view() {
    
    _ksinit(nxt_view, kst_object, T_NAME, sizeof(struct nx_view_s), -1, "Multidimesional array view", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        //{"__init__",               kso_func_new(T_init_, T_NAME ".__init__(self, name, version, desc, authors)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

    ));
}



