/* types/tuple.c - 'tuple' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "tuple"



/* C-API */

ks_tuple ks_tuple_new(ks_ssize_t len, kso* elems) {
    ks_tuple self = KSO_NEW(ks_tuple, kst_tuple);

    self->len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    ks_ssize_t i;
    for (i = 0; i < len; ++i) {
        KS_INCREF(elems[i]);
        self->elems[i] = elems[i];
    }

    return self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_tuple = &tp;

void _ksi_tuple() {
    _ksinit(kst_tuple, kst_object, T_NAME, sizeof(struct ks_tuple_s), -1, NULL);
}
