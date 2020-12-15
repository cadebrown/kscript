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

    char* new_data = ks_zmalloc(1, len_b + 1);
    memcpy(new_data, data, len_b);
    new_data[len_b] = '\0';

    return ks_str_newn(len_b, new_data);
}

ks_str ks_str_newnt(ks_type tp, ks_ssize_t len_b, char* data) {
    if (len_b < 0) len_b = strlen(data);

    ks_str self = KSO_NEW(ks_str, tp);

    
    self->len_b = len_b;
    self->len_c = ks_str_lenc(len_b, data);;

    self->data = data;
    self->data[len_b] = '\0';

    self->v_hash = ks_hash_bytes(len_b, (const unsigned char*)data);

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
        r += (data[p++] & 0xC0) != 0x80;
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

ks_ssize_t ks_str_find(ks_str self, ks_str substr, ks_ssize_t min_c, ks_ssize_t max_c, ks_ssize_t* idx_b) {
    if (substr->len_b == 0) return -1;
    else if (self->len_b < substr->len_b) return -1;

    if (KS_STR_IS_ASCII(self)) {
        // otherwise, search for it
        /* ASCII search */
        ks_ssize_t i;
        for (i = min_c; i <= max_c - substr->len_b; ++i) {
            if (self->data[i] == *substr->data) {
                // possible match, now check full substring
                if (memcmp(self->data + i, substr->data, substr->len_b) == 0) {
                    if (idx_b != NULL) *idx_b = i;
                    return i;
                }
            }
        }

        // not found
        return -1;
    } else {
        /* Generial unicode search */
        struct ks_str_citer cit = ks_str_citer_make(self);

        ks_size_t i = 0;
        ks_str_citer_seek(&cit, min_c);

        while (!cit.done && cit.cchi <= max_c) {
            if (self->data[cit.cbyi] == *substr->data) {
                // possible match, now check full substring
                if (memcmp(self->data + cit.cbyi, substr->data, substr->len_b) == 0) {
                    if (idx_b != NULL) *idx_b = cit.cbyi;
                    return cit.cchi;
                }
            }

            ks_str_citer_next(&cit);
        }

        return -1;
    }
}

ks_str ks_str_join(ks_str sep, kso objs) {
    ksio_StringIO sio = ksio_StringIO_new();
    ks_cit cit = ks_cit_make(objs);
    kso ob = NULL;
    int ct = 0;
    while ((ob = ks_cit_next(&cit)) != NULL) {

        if (ct > 0) ksio_add(sio, "%S", sep);
        if (!ksio_add(sio, "%S", ob)) {
            KS_DECREF(sio);
            return NULL;
        }

        ct++;
        KS_DECREF(ob);
    }

    ks_cit_done(&cit);
    if (cit.exc) {
        KS_DECREF(sio);
        return NULL;
    }

    return ksio_StringIO_getf(sio);
}




/* C-style string iteration */

struct ks_str_citer ks_str_citer_make(ks_str self) {
    struct ks_str_citer cit;
    cit.self = self;
    
    cit.done = self->len_c == 0;
    cit.err = 0;
    cit.lcbyi = cit.cbyi = cit.cchi = 0;

    return cit;
}

ks_ucp ks_str_citer_next(struct ks_str_citer* cit) {

    // ensure we are still in range
    if (cit->cchi >= cit->self->len_c) {
        cit->err = 1;
        return -1;
    }

    cit->lcbyi = cit->cbyi;
    cit->cchi++;
    cit->done = cit->cchi >= cit->self->len_c;
    ks_ucp r;
    int sz;
    KS_UCP_FROM_UTF8(r, cit->self->data + cit->cbyi, sz);

    if (sz < 0) {
        // TODO: add more specific errors
        cit->err = 1;
        return -1;
    } else {
        cit->cbyi += sz;
        return r;
    }
}

// peek at current character, but don't change state
ks_ucp ks_str_citer_peek(struct ks_str_citer* cit) {
    ks_ucp r;
    int sz;
    KS_UCP_FROM_UTF8(r, cit->self->data + cit->cbyi, sz);

    if (sz < 0) {
        // TODO: add more specific errors
        cit->err = 1;
        return -1;
    } else {
        return r;
    }
}

ks_ucp ks_str_citer_peek_n(struct ks_str_citer* cit, int n) {
    struct ks_str_citer new_cit = *cit;

    int i;
    for (i = 0; i < n; ++i) ks_str_citer_next(&new_cit);

    return ks_str_citer_peek(&new_cit);
}

bool ks_str_citer_seek(struct ks_str_citer* cit, ks_ssize_t idx) {
    // check bounds
    if (idx < 0 || idx >= cit->self->len_c) {
        cit->err = 1;
        return false;
    }

    // nothing to do; already at correct position
    if (idx == cit->cchi) return true;

    // check for restarting the iterator
    if (idx == 0) {
        cit->lcbyi = cit->cbyi = cit->cchi = 0;
        cit->done = cit->self->len_c == 0;
        cit->err = 0;
        return true;
    }

    if (KS_STR_IS_ASCII(cit->self)) {
        // ASCII only, so O(1)
        cit->lcbyi = cit->cbyi = cit->cchi = idx;

        // calculate whether it was 'done'
        cit->done = cit->cchi >= cit->self->len_c;

        return true;
    } else {

        ks_ssize_t naive_dist = idx - cit->cchi;

        #if KS_STR_OFF_EVERY

        ks_ssize_t offi = idx / KS_STR_OFF_EVERY;
        ks_ssize_t offe = idx % KS_STR_OFF_EVERY;

        if (!cit->self->_offs || (naive_dist > 0 && naive_dist < offe)) {
            // it's more efficient to just probe from the current position, so do nothing

        } else {
            // we should use offsets to do it, so fast forward the iterator to the given offset
            cit->cchi = KS_STR_OFF_EVERY * offi;
            cit->lcbyi = cit->cbyi = cit->self->_offs[offi];

        }
        #endif /* KS_STR_OFF_EVERY */

        // we need to seek forward
        while (cit->cchi < idx) {
            // check for errors
            ks_ucp c = ks_str_citer_next(cit);
            if (c < 0) return false;
        }

        return true;
    }
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

    return (kso)ks_fmt("%S", extra[0]);
}

static KS_TFUNC(T, len) {
    ks_str self;
    KS_ARGS("self:*", &self, kst_str);

    return (kso)ks_int_newu(self->len_c);
}

static KS_TFUNC(T, add) {
    kso L, R;
    KS_ARGS("L R", &L, &R);

    return (kso)ks_fmt("%S%S", L, R);
}


static KS_TFUNC(T, startswith) {
    ks_str self;
    kso obj;
    KS_ARGS("self:* obj", &self, kst_str, &obj);

    if (obj->type == kst_str) {
        ks_str sobj = (ks_str)obj;

        bool res = self->len_b >= sobj->len_b && strncmp(self->data, sobj->data, sobj->len_b) == 0;
        return KSO_BOOL(res);

    } else if (kso_issub(obj->type, kst_tuple)) {
        ks_tuple tobj = (ks_tuple)obj;
        int i;
        for (i = 0; i < tobj->len; ++i) {
            ks_str sobj = (ks_str)tobj->elems[i];
            if (sobj->type != kst_str) {
                KS_THROW(kst_ArgError, "Expected 'obj' to be a 'str' or 'tuple' of strings, but got '%T' object", sobj);   
                return NULL;
            }
            // found match
            if (self->len_b >= sobj->len_b && strncmp(self->data, sobj->data, sobj->len_b) == 0) return KSO_TRUE;
        }

        // no match
        return KSO_FALSE;

    } else {
        KS_THROW(kst_ArgError, "Expected 'obj' to be a 'str' or 'tuple' of strings, but got '%T' object", obj);   
        return NULL;
    }
}

static KS_TFUNC(T, endswith) {
    ks_str self;
    kso obj;
    KS_ARGS("self:* obj", &self, kst_str, &obj);

    if (obj->type == kst_str) {
        ks_str sobj = (ks_str)obj;

        bool res = self->len_b >= sobj->len_b && strncmp(self->data + self->len_b - sobj->len_b, sobj->data, sobj->len_b) == 0;
        return KSO_BOOL(res);
    } else if (kso_issub(obj->type, kst_tuple)) {
        ks_tuple tobj = (ks_tuple)obj;
        int i;
        for (i = 0; i < tobj->len; ++i) {
            ks_str sobj = (ks_str)tobj->elems[i];
            if (sobj->type != kst_str) {
                KS_THROW(kst_ArgError, "Expected 'obj' to be a 'str' or 'tuple' of strings, but got '%T' object", sobj);   
                return NULL;
            }
            // found match
            if (self->len_b >= sobj->len_b && strncmp(self->data + self->len_b - sobj->len_b, sobj->data, sobj->len_b) == 0) return KSO_TRUE;
        }

        // no match
        return KSO_FALSE;

    } else {
        KS_THROW(kst_ArgError, "Expected 'obj' to be a 'str' or 'tuple' of strings, but got '%T' object", obj);   
        return NULL;
    }
}

static KS_TFUNC(T, find) {
    ks_str self;
    ks_str sub;
    ks_cint start = 0, end = KS_CINT_MAX;
    KS_ARGS("self:* sub:* ?start:cint ?end:cint", &self, kst_str, &sub, kst_str, &start, &end);

    if (start < 0) start = 0;
    if (start >= self->len_c) start = self->len_c;
    if (end < 0) end = 0;
    if (end >= self->len_c) end = self->len_c;

    return (kso)ks_int_new(ks_str_find(self, sub, start, end, NULL));
}

static KS_TFUNC(T, replace) {
    ks_str self;
    ks_str sub, by;
    KS_ARGS("self:* sub:* by:*", &self, kst_str, &sub, kst_str, &by, kst_str);

    /* General search/replace */
    ksio_StringIO sio = ksio_StringIO_new();

    ks_size_t i = 0, j = 0;
    for (i = 0; i <= self->len_b - sub->len_b; ) {
        if (memcmp(self->data + i, sub->data, sub->len_b) == 0) {
            /* Add literal */
            ksio_addbuf(sio, i - j, self->data + j);
            /* Found substring, so emit the replaced by */
            ksio_addbuf(sio, by->len_b, by->data);

            /* Skip over it */
            j = i += sub->len_b;
        } else {
            i++;
        }
    }
    i = self->len_b;
    ksio_addbuf(sio, i - j, self->data + j);
    return (kso)ksio_StringIO_getf(sio);
}


static KS_TFUNC(T, split) {
    ks_str self;
    ks_str by;
    KS_ARGS("self:* by:*", &self, kst_str, &by, kst_str);

    return (kso)ks_str_split(self, by);
}
static KS_TFUNC(T, join) {
    ks_str self;
    kso objs;
    KS_ARGS("self:* objs", &self, kst_str, &objs);

    return (kso)ks_str_join(self, objs);
}

static bool I_isspace(ks_ucp c) {
    return c == ' ' || c == '\t' || c == '\t' || c == '\v' || c == '\r' || c == '\n';
}

static KS_TFUNC(T, trim) {
    ks_str self;
    KS_ARGS("self:*", &self, kst_str);

    int pl = 0, pr = self->len_b;
    while (pl < pr && I_isspace(self->data[pl])) {
        pl++;
    }
    while (pr > pl && I_isspace(self->data[pr - 1])) {
        pr--;
    }

    return (kso)ks_str_new(pr - pl, self->data + pl);
}




static KS_TFUNC(T, mod) {
    ks_str self;
    ks_tuple args;
    KS_ARGS("self:* args:*", &self, kst_str, &args, kst_tuple);

    ksio_StringIO sio = ksio_StringIO_new();

    int p = 0, l = 0;
    int ai = 0;
    kso a;
    while (p < self->len_b) {
        l = p;
        while (p < self->len_b && self->data[p] != '%') {
            p++;
        }
        ksio_addbuf(sio, p - l, self->data + l);

        if (self->data[p] == '%') {
            p++;
            char c = self->data[p++];
            if (c == '%') {
                ksio_add(sio, "%%");
            } else if (c == 's') {
                if (ai >= args->len) {
                    KS_THROW(kst_Error, "More format specifiers than arguments");
                    KS_DECREF(sio);
                    return NULL;
                }
                a = args->elems[ai++];
                if (!ksio_add(sio, "%S", a)) {
                    KS_DECREF(sio);
                    return NULL;
                }
            } else if (c == 'T') {
                if (ai >= args->len) {
                    KS_THROW(kst_Error, "More format specifiers than arguments");
                    KS_DECREF(sio);
                    return NULL;
                }
                a = args->elems[ai++];
                if (!ksio_add(sio, "%T", a)) {
                    KS_DECREF(sio);
                    return NULL;
                }

            } else {
                KS_DECREF(sio);
                KS_THROW(kst_Error, "Unknown format specifier: '%%%c'", c);
                return NULL;
            }
        }
    }

    if (ai < args->len) {
        KS_THROW(kst_Error, "More arguments than format specifiers");
        KS_DECREF(sio);
        return NULL;

    }
    return (kso)ksio_StringIO_getf(sio);

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
        {"__new",                ksf_wrap(T_new_, T_NAME ".__new(tp, obj=none)", "")},
        {"__len",                ksf_wrap(T_len_, T_NAME ".__len(self)", "")},

        {"__iter",               KS_NEWREF(kst_str_iter)},

        {"__add",                ksf_wrap(T_add_, T_NAME ".__add(self)", "")},
        {"__mod",                ksf_wrap(T_mod_, T_NAME ".__mod(self, args)", "")},

        {"startswith",           ksf_wrap(T_startswith_, T_NAME ".startswith(self, obj)", "Computes whether a string starts with another string, or any of a tuple of strings")},
        {"endswith",             ksf_wrap(T_endswith_, T_NAME ".endswith(self, obj)", "Computes whether a string ends with another string, or any of a tuple of strings")},
        
        {"join",                 ksf_wrap(T_join_, T_NAME ".join(self, objs)", "Joins an iterable by a seperator")},
        {"split",                ksf_wrap(T_split_, T_NAME ".split(self, by)", "Splits a string on a given input")},
        
        {"find",                 ksf_wrap(T_find_, T_NAME ".find(self, sub, start=none, end=none)", "Find a substring within 'self[start:end]'")},
        {"replace",              ksf_wrap(T_replace_, T_NAME ".replace(self, sub, by)", "Replace instances of 'sub' with 'by'")},

        {"trim",                 ksf_wrap(T_trim_, T_NAME ".trim(self)", "Trims the left and right sides of 'self' of spaces, and returns what is left")},
        
    ));
}
