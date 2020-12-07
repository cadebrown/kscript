/* types/type.c - 'type' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "type"


/* C-API */



/* Initialize a type that has already been allocated or has memory */
void type_init(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv, bool is_new) {
    if (is_new) {

    } else {
        /* May have been allocated in constant storage, so initialize that memory */
        memset(self, 0, sizeof(*self));
        self->refs = 1;
        KS_INCREF(kst_type);
        self->type = kst_type;

        self->attr = ks_dict_new(NULL);
    }

    #define ACT(_attr) self->i##_attr = base->i##_attr;
    _KS_DO_SPEC(ACT)
    #undef ACT

    /* Now, actually set up type  */

    self->ob_sz = sz == 0 ? base->ob_sz : sz;
    self->ob_attr = attr == 0 ? base->ob_attr : attr;
    ks_type_set(self, _ksva__base, (kso)base);

    kso tmp = (kso)ks_str_new(-1, name);
    ks_type_set(self, _ksva__name, tmp);
    ks_type_set(self, _ksva__fullname, tmp);
    KS_DECREF(tmp);

    tmp = (kso)ks_str_new(-1, doc);
    ks_type_set(self, _ksva__doc, tmp);
    KS_DECREF(tmp);


    /* Add to 'subs' */
    if (self != base) {
        int idx = base->n_subs++;
        base->subs = ks_zrealloc(base->subs, sizeof(*base->subs), base->n_subs);
        base->subs[idx] = self;
    }

    /* Add attributes */
    if (ikv) {
        while (ikv->key) {
            ks_str k = ks_str_new(-1, ikv->key);
            ks_type_set(self, k, ikv->val);
            KS_DECREF(k);
            ikv++;
        }
    }

}

void _ksinit(ks_type self, ks_type base, const char* name, int sz, int attr, const char* doc, struct ks_ikv* ikv) {
    type_init(self, base, name, sz, attr, doc, ikv, false);
}


ks_type ks_type_new(const char* name, ks_type base, int sz, int attr_pos, const char* doc, struct ks_ikv* ikv) {
    ks_type self = KSO_NEW(ks_type, kst_type);

    type_init(self, base, name, sz, attr_pos, doc, ikv, true);

    return self;
}
kso ks_type_get(ks_type self, ks_str attr) {
    kso res = ks_dict_get_ih(self->attr, (kso)attr, attr->v_hash);
    if (res) return res;

    if (self->i__base != self) return ks_type_get(self->i__base, attr);
    else {
        KS_THROW_ATTR(self, attr);
        return NULL;
    }
}

bool ks_type_set(ks_type self, ks_str attr, kso val) {
    if (attr->len_b > 2 && attr->data[0] == '_' && attr->data[1] == '_') {
        /* Handle special names */
        #define ACT(_attr) else if (ks_str_eq_c(attr, #_attr, sizeof(#_attr) - 1)) { \
            *(kso*)&self->i##_attr = val; \
        }
        if (false) {}
        _KS_DO_SPEC(ACT)
        #undef ACTss
    }

    ks_dict_set_h(self->attr, (kso)attr, attr->v_hash, val);
    return true;
}

bool ks_type_set_c(ks_type self, const char* attr, kso val) {
    ks_str k = ks_str_new(-1, attr);
    bool res = ks_type_set(self, k, val);
    KS_DECREF(k);
    return res;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_type = &tp;

void _ksi_type() {
    _ksinit(kst_type, kst_object, T_NAME, sizeof(struct ks_type_s), offsetof(struct ks_type_s, attr), "Represents a type, which is a descriptor of objects which are instances of the type", KS_IKV(

    ));
}
