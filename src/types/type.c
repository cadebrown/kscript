/* types/type.c - 'type' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

/* C-API */

/* Initialize a type that has already been allocated or has memory */
void type_init(ks_type self, ks_type base, const char* name, int sz, int attr, struct ks_ikv* ikv, bool is_new) {
    if (is_new) {

    } else {
        /* May have been allocated in constant storage, so initialize that memory */
        memset(self, 0, sizeof(*self));
        self->refs = 1;
        KS_INCREF(kst_type);
        self->type = kst_type;

        self->attr = ks_dict_new(NULL);
    }
    
    self->ob_sz = sz == 0 ? base->ob_sz : sz;
    self->ob_attr = attr == 0 ? base->ob_attr : attr;

    /* Now, actually set up type  */

    self->i__name = ks_str_new(-1, name);
    KS_INCREF(base);
    self->i__base = base;

    /* Add to 'subs' */
    if (self != base) {
        int idx = base->n_subs++;
        base->subs = ks_zrealloc(base->subs, sizeof(*base->subs), base->n_subs);
        base->subs[idx] = self;
    }

}

void _ksinit(ks_type self, ks_type base, const char* name, int sz, int attr, struct ks_ikv* ikv) {
    type_init(self, base, name, sz, attr, ikv, false);
}


ks_type ks_type_new(const char* name, ks_type base, int sz, int attr_pos, struct ks_ikv* ikv) {
    ks_type self = KSO_NEW(ks_type, kst_type);

    type_init(self, base, name, sz, attr_pos, ikv, true);

    return self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_type = &tp;

void _ksi_type() {
    
}
