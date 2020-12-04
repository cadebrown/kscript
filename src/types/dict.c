/* types/dict.c - 'dict' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "dict"


/* C-API */

ks_dict ks_dict_new(struct ks_ikv* ikv) {
    ks_dict self = ks_zmalloc(1, sizeof(*self));
    KS_INCREF(kst_dict);
    self->type = kst_dict;
    self->refs = 1;

    self->len_buckets = self->len_ents = self->len_real = 0;
    self->_max_len_buckets_b = self->_max_len_ents = 0;

    self->ents = NULL;
    self->buckets_s8 = NULL;

    /* Initialize elements */
    if (ikv) {
        struct ks_ikv* p = ikv;
        while (p->key) {

            p++;
        }
    }


    return self;
}




/* Export */

static struct ks_type_s tp;
ks_type kst_dict = &tp;

void _ksi_dict() {
    _ksinit(kst_dict, kst_object, T_NAME, sizeof(struct ks_dict_s), -1, NULL);
    
}
