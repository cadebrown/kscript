/* types/bytes.c - 'bytes' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "bytes"


/* C-API */

ks_bytes ks_bytes_new(ks_ssize_t len_b, const char* data) {
    char* new_data = ks_malloc(len_b);
    memcpy(new_data, data, len_b);
    return ks_bytes_newn(len_b, new_data);
}

ks_bytes ks_bytes_newn(ks_ssize_t len_b, char* data) {
    ks_bytes self = KSO_NEW(ks_bytes, kst_bytes);
    
    self->len_b = len_b;
    self->data = data;
    self->v_hash = ks_hash_bytes(len_b, data);

    return self;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_bytes self;
    KS_ARGS("self:*", &self, kst_bytes);

    ks_free(self->data);

    KSO_DEL(self);

    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_bytes = &tp;

void _ksi_bytes() {
    _ksinit(kst_bytes, kst_object, T_NAME, sizeof(struct ks_bytes_s), -1, "Sequence of bytes ('int' in range(256)), which is immutable. Similar to a 'str' but has no notion of 'codepoints' or 'characters'", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
    ));
}
