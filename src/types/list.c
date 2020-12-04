/* types/list.c - 'list' type
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "list"



/* C-API */

ks_list ks_list_new(ks_ssize_t len, kso* elems) {
    ks_list self = KSO_NEW(ks_list, kst_list);

    self->len = len;
    self->_max_len = len;
    self->elems = ks_zmalloc(sizeof(*self->elems), len);

    ks_ssize_t i;
    for (i = 0; i < len; ++i) {
        KS_INCREF(elems[i]);
        self->elems[i] = elems[i];
    }

    return self;
}


bool ks_list_push(ks_list self, kso ob) {
    ks_size_t i = self->len++;
    if (self->len > self->_max_len) {
        self->_max_len = ks_nextsize(self->len, self->_max_len);
        self->elems = ks_zrealloc(self->elems, sizeof(*self->elems), self->_max_len);
    }
    KS_INCREF(ob);
    self->elems[i] = ob;
}




/* Export */

static struct ks_type_s tp;
ks_type kst_list = &tp;

void _ksi_list() {
    _ksinit(kst_list, kst_object, T_NAME, sizeof(struct ks_list_s), -1, NULL);
}
