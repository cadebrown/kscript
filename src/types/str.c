/* types/str.c - 'str' type
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "str"
#define TI_NAME "str.__iter"


/* C-API */



ks_str ks_str_newt(ks_type tp, ks_ssize_t len_b, const char* data) {
    if (len_b < 0) len_b = strlen(data);

    char* new_data= ks_zmalloc(1, len_b + 1);
    memcpy(new_data, data, len_b);
    new_data[len_b] = '\0';

    return ks_str_newn(len_b, new_data);
}

ks_str ks_str_newnt(ks_type tp, ks_ssize_t len_b, char* data) {
    if (len_b < 0) len_b = strlen(data);

    ks_str self = KSO_NEW(ks_str, tp);
    
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


ks_str ks_str_new(ks_ssize_t len_b, const char* data) {
    return ks_str_newt(kst_str, len_b, data);
}

ks_str ks_str_newn(ks_ssize_t len_b, char* data) {
    return ks_str_newnt(kst_str, len_b, data);
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
    char utf8[5];
    int n;
    KS_UCP_TO_UTF8(utf8, n, ord);
    return ks_str_new(n, utf8);
}
ks_ucp ks_str_ord(ks_str chr) {
    if (chr->len_c != 1) {
        KS_THROW(kst_Error, "Only strings of length 1 are allowed in 'ord()'");
        return -1;
    }
    ks_ucp res;
    int n;
    KS_UCP_FROM_UTF8(res, chr->data, n);

    return res;
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


ks_list ks_str_split(ks_str self, ks_str by) {
    ks_list res = ks_list_new(0, NULL);
    if (self->len_b == 0) {
        ks_list_push(res, (kso)self);
        return res;
    }

    ks_size_t i, j = 0;
    for (i = 0; i <= self->len_b - by->len_b; ++i) {
        if (memcmp(self->data + i, by->data, by->len_b) == 0) {
            /* Add characters pased until this */
            ks_str ss = ks_str_new(i - j, self->data + j);
            ks_list_push(res, (kso)ss);
            KS_DECREF(ss);

            i += by->len_b - 1;
            j = i + 1;
        } else {
        }
    }

    i = self->len_b;

    if (i >= j) {
        ks_str ss = ks_str_new(i - j, self->data + j);
        ks_list_push(res, (kso)ss);
        KS_DECREF(ss);
    }

    return res;
}

ks_list ks_str_split_c(const char* self, const char* by) {
    ks_str s0 = ks_str_new(-1, self), s1 = ks_str_new(-1, by);
    ks_list res = ks_str_split(s0, s1);
    KS_DECREF(s0);
    KS_DECREF(s1);
    return res;
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ks_str self;
    KS_ARGS("self:*", &self, kst_str);

    ks_free(self->data);

    KSO_DEL(self);

    return KSO_NONE;
}
static KS_TFUNC(T, new) {
    ks_type tp;
    int n_extra;
    kso* extra;
    KS_ARGS("tp:* *args", &tp, kst_type, &n_extra, &extra);

    if (n_extra == 0) {
        return (kso)ks_str_newt(tp, 0, NULL);
    } else {
        if (kso_issub(extra[0]->type, tp)) {
            return KS_NEWREF(extra[0]);
        } else if (extra[0]->type->i__str) {
            ks_str res = (ks_str)kso_call(extra[0]->type->i__str, n_extra, extra);
            if (!res) return NULL;
            
            if (!kso_issub(res->type, kst_str)) {
                KS_THROW(kst_TypeError, "'%T.__str()' returned non-str object of type '%T'", extra[0], res);
                KS_DECREF(res);
                return NULL;
            }

            if (kso_issub(res->type, tp)) {
                return (kso)res;
            } else {
                ks_str rr = ks_str_newt(tp, res->len_b, res->data);
                KS_DECREF(res);
                return (kso)rr;
            }
        }
    }

    KS_THROW_CONV(extra[0]->type, tp);
    return NULL;
}


static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    return (kso)ks_fmt("%S%S", L, R);
}



/** Iterator **/

static KS_TFUNC(TI, free) {
    ks_str_iter self;
    KS_ARGS("self:*", &self, kst_str_iter);

    KS_DECREF(self->of);
    KSO_DEL(self);

    return KSO_NONE;
}

static KS_TFUNC(TI, new) {
    ks_type tp;
    ks_str of;
    KS_ARGS("tp:* of:*", &tp, kst_type, &of, kst_str);

    ks_str_iter self = KSO_NEW(ks_str_iter, tp);

    KS_INCREF(of);
    self->of = of;


    return (kso)self;
}


/* Export */

static struct ks_type_s tp;
ks_type kst_str = &tp;

static struct ks_type_s tp_iter;
ks_type kst_str_iter = &tp_iter;


void _ksi_str() {

    _ksinit(kst_str_iter, kst_object, TI_NAME, sizeof(struct ks_str_iter_s), -1, "", KS_IKV(
        {"__free",               ksf_wrap(TI_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(TI_new_, T_NAME ".__new(tp, of)", "")},
    ));

    _ksinit(kst_str, kst_object, T_NAME, sizeof(struct ks_str_s), -1, "String (i.e. a collection of Unicode characters)\n\n    Indicies, operations, and so forth take character positions, not byte positions", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(self)", "")},

        {"__iter",               KS_NEWREF(kst_str_iter)},

        {"__add",                ksf_wrap(T_add_, T_NAME ".__add(self)", "")},
    ));
}
