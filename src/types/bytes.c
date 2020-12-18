/* types/bytes.c - 'bytes' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "bytes"


/* C-API */



ks_bytes ks_bytes_newnt(ks_type tp, ks_ssize_t len_b, char* data) {
    ks_bytes self = KSO_NEW(ks_bytes, tp);
    
    self->len_b = len_b;
    self->data = data;
    self->v_hash = ks_hash_bytes(len_b, data);

    return self;
}
ks_bytes ks_bytes_newt(ks_type tp, ks_ssize_t len_b, const char* data) {
    char* new_data = ks_malloc(len_b);
    memcpy(new_data, data, len_b);
    return ks_bytes_newn(len_b, new_data);
}


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

ks_bytes ks_bytes_newo(ks_type tp, kso obj) {
    if (kso_issub(obj->type, tp)) return (ks_bytes)KS_NEWREF(obj);

    ks_bytes self = NULL;

    if (obj->type->i__bytes != NULL) {
        return (ks_bytes)kso_call(obj->type->i__bytes, 1, &obj);
    } else if (kso_issub(obj->type, kst_str)) {
        return ks_bytes_newt(tp, ((ks_str)obj)->len_b, ((ks_str)obj)->data);
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ks_bytes self;
    KS_ARGS("self:*", &self, kst_bytes);

    ks_free(self->data);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, new) {
    ks_type tp;
    kso obj = KSO_NONE;
    KS_ARGS("tp:* ?obj", &tp, kst_type, &obj);

    if (kso_issub(obj->type, tp)) {
        return KS_NEWREF(obj);
    } else {
        return (kso)ks_bytes_newo(tp, obj);
    }

    KS_THROW_CONV(obj->type, tp);
    return NULL;
}

static KS_TFUNC(T, decode) {
    ks_bytes self;
    KS_ARGS("self:*", &self, kst_bytes);

    /* TODO; other encodings */

    return (kso)ks_str_new(self->len_b, self->data);
}


static KS_TFUNC(T, bool) {
    ks_bytes self;
    KS_ARGS("self:*", &self, kst_bytes);

    return KSO_BOOL(self->len_b != 0);
}

static KS_TFUNC(T, len) {
    ks_bytes self;
    KS_ARGS("self:*", &self, kst_bytes);

    return (kso)ks_int_new(self->len_b);
}

/* Export */

static struct ks_type_s tp;
ks_type kst_bytes = &tp;

void _ksi_bytes() {
    _ksinit(kst_bytes, kst_object, T_NAME, sizeof(struct ks_bytes_s), -1, "Sequence of bytes ('int' in range(256)), which is immutable. Similar to a 'str' but has no notion of 'codepoints' or 'characters'", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__bool",               ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                ksf_wrap(T_len_, T_NAME ".__len(self)", "")},
        {"decode",               ksf_wrap(T_decode_, T_NAME ".decode(self)", "Decode into a string")},
    ));
}
