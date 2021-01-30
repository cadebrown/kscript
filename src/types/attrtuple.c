/* types/attrtuple.c - 'attrtuple' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "attrtuple"


/* C-API */

ks_type ks_attrtuple_newtype(ks_str name, int nmem, kso* mem) {

    /* Map of attributes to index */
    ks_dict map = ks_dict_new(NULL);
    int i;
    for (i = 0; i < nmem; ++i) {
        ks_int vi = ks_int_new(i);
        if (!ks_dict_set(map, mem[i], (kso)vi)) {
            KS_DECREF(vi);
            KS_DECREF(map);
            return NULL;
        }
        KS_DECREF(vi);
    }

    ks_type tp = ks_type_new(name->data, kst_attrtuple, sizeof(struct ks_tuple_s), -1, "", NULL);
    ks_list maplist = ks_list_new(nmem, mem);
    ks_type_set(tp, _ksv_attrtuplemap, (kso)map);
    ks_type_set(tp, _ksv_attrtuplemaplist, (kso)maplist);
    KS_DECREF(maplist);
    KS_DECREF(map);
    return tp;
}


/* Type Functions */

static KS_TFUNC(T, getattr) {
    ks_tuple self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, kst_attrtuple, &attr, kst_str);

    ks_dict map = (ks_dict)ks_type_get(self->type, _ksv_attrtuplemap);
    if (!map) {
        return NULL;
    }

    ks_int vi = (ks_int)ks_dict_get(map, (kso)attr);
    if (!vi) {
        KS_DECREF(map);
        return NULL;
    }

    kso res = kso_getelem((kso)self, (kso)vi);

    KS_DECREF(map);
    KS_DECREF(vi);
    return res;
}

static KS_TFUNC(T, make) {
    ks_str name;
    kso members;
    KS_ARGS("name:* members", &name, kst_str, &members);

    ks_list lm = ks_list_newi(members);
    if (!lm) return NULL;

    ks_type res = ks_attrtuple_newtype(name, lm->len, lm->elems);
    KS_DECREF(lm);

    return (kso)res;
}

static KS_TFUNC(T, str) {
    ks_tuple self;
    KS_ARGS("self:*", &self, kst_attrtuple);

    ks_list maplist = (ks_list)ks_type_get(self->type, _ksv_attrtuplemaplist);
    if (!maplist) return NULL;

    ksio_StringIO sio = ksio_StringIO_new();

    ksio_add(sio, "%T({", self);

    int i, ct = 0;
    for (i = 0; i < maplist->len && i < self->len; ++i) {
        if (ct > 0) ksio_add(sio, ", ");
        ksio_add(sio, "%R: %R", maplist->elems[i], self->elems[i]);
        ct++;
    }
    while (i < self->len) {
        if (ct > 0) ksio_add(sio, ", ");
        ksio_add(sio, "%i: %R", i, self->elems[i]);
        ct++;
    }

    ksio_add(sio, "})");
    return (kso)ksio_StringIO_getf(sio);
}


/* Export */

static struct ks_type_s tp;
ks_type kst_attrtuple = &tp;

void _ksi_attrtuple() {

    _ksinit(kst_attrtuple, kst_tuple, T_NAME, sizeof(struct ks_tuple_s), -1, "Attribute-based tuple, which allows attributes to be translated into indices", KS_IKV(
        {"__str",                ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",               ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__getattr",            ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},

        {"make",                 ksf_wrap(T_make_, T_NAME ".make(name, members)", "Create a new subtype of 'attrtuple' with the given name and members")},
    ));
}
