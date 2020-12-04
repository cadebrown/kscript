/* types/bytes.c - 'bytes' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "bytes"


/* C-API */

ks_bytes ks_bytes_new(ks_ssize_t len_b, const char* data) {
    ks_bytes self = KSO_NEW(ks_bytes, kst_bytes);
    
    self->len_b = len_b;

    self->data = ks_zmalloc(1, len_b + 1);
    memcpy(self->data, data, self->len_b);
    self->data[self->len_b] = '\0';

    self->v_hash = 1;

    return self;
}



/* Export */

static struct ks_type_s tp;
ks_type kst_bytes = &tp;

void _ksi_bytes() {
    _ksinit(kst_bytes, kst_object, T_NAME, sizeof(struct ks_bytes_s), -1, NULL);
}
