/* types/bytes.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_bytes
#define T_TYPE kst_bytes
#define T_NAME "bytes"
#define T_DOC "Immutable sequence of bytes (8-bit integers 0 <= x < 256), similar to a 'str' but has no notion of 'characters'"


/* C-API */

ks_bytes ks_bytes_new(ks_type tp, ks_uint len_b, const ks_u8* data) {

    /* Allocate (and check) new buffer that can be owned by the 'bytes' object */
    ks_u8* new_data = ks_malloc(len_b);
    if (!new_data) {
        KS_THROW_OOM("While allocating byte buffer");
        return NULL;
    }

    /* Copy data to new pointer */
    memcpy(new_data, data, len_b);

    return ks_bytes_newn(tp, len_b, new_data);
}

ks_bytes ks_bytes_newn(ks_type tp, ks_uint len_b, ks_u8* data) {
    if (!tp) tp = T_TYPE;

    ks_bytes self = KS_NEW(ks_bytes, tp);
    if (!self) return NULL;

    self->len_b = len_b;
    self->data = data;

    /* Compute hash of the bytes */
    self->v_hash = ks_hash_bytes(len_b, data);

    return self;
}


/* Methods */

static KS_TFUNC(T, free) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    /* Free buffer */
    ks_free(self->data);

    KS_DEL(self);
    
    return KS_NONE;
}

static KS_TFUNC(T, bool) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return KS_BOOL(self->len_b != 0);
}

static KS_TFUNC(T, len) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return (kso)ks_int_newu(NULL, self->len_b);
}


static KS_TFUNC(T, decode) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    /* TODO: support other encodings */

    return (kso)ks_str_new(NULL, self->len_b, self->data);
}

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_bytes() {

    ks_init_type(T_TYPE, NULL, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__hash(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},

        {"decode",                 ksf_wrap(T_decode_, T_NAME ".decode(self)", "Decode the bytes object into a string")},

    ));
}
