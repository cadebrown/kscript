/* types/str.c - 'str' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "str"


/* C-API */

ks_str ks_str_new(ks_ssize_t len_b, const char* data) {
    if (len_b < 0) len_b = strlen(data);

    char* new_data= ks_zmalloc(1, len_b + 1);
    memcpy(new_data, data, len_b);
    new_data[len_b] = '\0';

    return ks_str_newn(len_b, new_data);
}


ks_str ks_str_newn(ks_ssize_t len_b, char* data) {
    if (len_b < 0) len_b = strlen(data);

    ks_str self = KSO_NEW(ks_str, kst_str);
    
    self->len_b = len_b;
    self->len_c = 0;

    self->data = data;
    self->data[len_b] = '\0';

    self->v_hash = ks_hash_bytes(len_b, (const unsigned char*)data);

    const char* p = self->data;
    while (*p) {
        /* Found another character */
        self->len_c++;

        /* Skip continuation bytes */
        while (*p < 0) p++;

        /* Ensure we haven't hit the NUL-terminator */
        if (*p) p++;
    }

    return self;
}




int ks_str_cmp(ks_str L, ks_str R) {
    if (L == R) return 0;

    ks_size_t min_len_b = L->len_b > R->len_b ? R->len_b : L->len_b;
    int c0 = memcmp(L->data, R->data, min_len_b+1);
    return c0 < 0 ? -1 : (c0 > 0 ? 1 : 0);
}
bool ks_str_eq(ks_str L, ks_str R) {
    return ks_str_eq_c(L, R->data, R->len_b);
}
bool ks_str_eq_c(ks_str L, const char* data, ks_ssize_t len_b) {
    if (len_b < 0) len_b = strlen(data);
    return L->len_b == len_b && memcmp(L->data, data, len_b) == 0;
}
ks_str ks_str_chr(ks_ucp ord) {

}
ks_ucp ks_str_ord(ks_str chr) {

}
ks_ssize_t ks_str_lenc(ks_ssize_t len_b, const char* data) {
    if (len_b < 0) len_b = strlen(data);
    ks_ssize_t p = 0, r = 0;
    while (p < len_b) {
        /* Found another character */
        r++;

        /* Skip continuation bytes */
        while (p < len_b && data[p] < 0) p++;

        /* Ensure we haven't hit the NUL-terminator or end */
        if (p < len_b) p++;
    }

    return r;
}

/* Type Functions */

static KS_TFUNC(T, free) {
    ks_str self;
    KS_ARGS("self:*", &self, kst_str);

    ks_free(self->data);

    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    return (kso)ks_fmt("%S%S", L, R);
}


/* Export */

static struct ks_type_s tp;
ks_type kst_str = &tp;


void _ksi_str() {
    _ksinit(kst_str, kst_object, T_NAME, sizeof(struct ks_str_s), -1, KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__add",                ksf_wrap(T_add_, T_NAME ".__add(self)", "")},
    ));
}
