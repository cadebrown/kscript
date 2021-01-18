/* util.c - Utility functions
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

/* Calculate primality. TODO: Consider miller rabin? */
static bool is_prime(ks_ssize_t x) {
    /**/ if (x < 2) return false;
    else if (x == 2 || x == 3 || x == 5) return true;
    else if (x % 2 == 0 || x % 3 == 0 || x % 5 == 0) return false;

    int i = 3;
    while (i * i <= x) {
        if (x % i == 0) return false;
        i += 2;
    }

    return true;
}

ks_ssize_t ks_nextprime(ks_ssize_t x) {
    if (x < 2) return 2;
    ks_ssize_t i = x % 2 == 0 ? x + 1 : x + 2;

    // search for primes
    while (!is_prime(i)) {
        i += 2;
    }

    return i;
}



ks_str ks_fmt(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ks_str res = ks_fmtv(fmt, ap);
    va_end(ap);
    return res;
}

ks_str ks_fmtv(const char* fmt, va_list ap) {
    ksio_StringIO sio = ksio_StringIO_new();

    if (!ksio_addv((ksio_BaseIO)sio, fmt, ap)) {
        KS_DECREF(sio);
        return NULL;
    }

    return ksio_StringIO_getf(sio);
}

bool ks_printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool res = ksio_addv((ksio_BaseIO)ksos_stdout, fmt, ap);
    va_end(ap);
    return res;
}


ks_hash_t ks_hash_bytes(ks_ssize_t len_b, const unsigned char* data) {
    /* djb2-based algorithm */
    ks_hash_t res = 5381;

    ks_ssize_t i;
    for (i = 0; i < len_b; ++i) {
        res = (33 * res) + data[i];
    }

    return res;
}


/* Utilities for emscripten/other projects */

void _ksem_incref_(kso obj) {
    KS_INCREF(obj);
}

void _ksem_decref_(kso obj) {
    KS_DECREF(obj);
}

char* _ksem_str_c_(kso obj) {
    ks_str res = ks_fmt("%S", obj);
    if (!res) return NULL;

    char* rr = ks_malloc(res->len_b + 1);
    memcpy(rr, res->data, res->len_b + 1);

    KS_DECREF(res);
    return rr;
}

char* _ksem_repr_c_(kso obj) {
    ks_str res = ks_fmt("%R", obj);
    if (!res) return NULL;

    char* rr = ks_malloc(res->len_b + 1);
    memcpy(rr, res->data, res->len_b + 1);

    KS_DECREF(res);
    return rr;
}

ks_cint _ksem_refs_c_(kso obj) {
    return obj->refs;
}
ks_type _ksem_type_c_(kso obj) {
    return obj->type;
}

