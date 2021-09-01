/* types/int.c - 
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_int
#define T_TYPE kst_int
#define T_NAME "int"
#define T_DOC "Immutable numerical quantity representing an integral value, with no limitations but system memory"


/* C-API */

ks_int ks_int_new(ks_type tp, int len, const char* src, int base) {
    if (!tp) tp = T_TYPE;

    /* Extra buffer storage, used if 'src' is not NUL-terminated (i.e. a temporary copy is made) */
    char* extbuf = NULL;

    /* Determin if 'src' is NUL-terminated (otherwise, set 'len' via 'strlen') */
    bool is_nt = len < 0;

    /* Now, create a 'src' variable, but ensure it is NUL-terminated */
    const char* src_nt = NULL;
    if (is_nt) {
        /* Just use 'src' as it is, no need to allocate storage */
        len = strlen(src);
        src_nt = src;

    } else {
        /* Allocate temporary storage, enough for the data and NUL-terminator */
        extbuf = ks_malloc(len + 1);
        if (!src_nt) {
            KS_THROW_OOM("While allocating temporary buffer to parse an int");
            return NULL;
        }

        /* Copy data and NUL-terminated */
        memcpy(extbuf, src, len);
        extbuf[len] = '\0';

        /* Set NUL-terminated source */
        src_nt = extbuf;
    }

    ks_int self = KS_NEW(ks_int, tp);
    if (!self) {
        ks_free(extbuf);
        return NULL;
    }

    /* Initialize and set the mpz object */
    mpz_init(self->val);
    int rc = mpz_set_str(self->val, src_nt, base);
    if (rc != 0) {
        /* ERR: Bad format */
        KS_THROW(kst_ValError, "Bad format for base-%i int: '%*s'", base, len, src);
        KS_DECREF(self);
        ks_free(extbuf);
        return NULL;
    }

    /* Free temporary buffer (if it was allocated) */
    ks_free(extbuf);
    return self;
}

ks_int ks_int_newz(ks_type tp, mpz_t val) {
    if (!tp) tp = T_TYPE;

    ks_int self = KS_NEW(ks_int, tp);
    if (!self) {
        return NULL;
    }

    /* Initialize and set the mpz object */
    mpz_init(self->val);
    mpz_set(self->val, val);

    return self;
}

ks_int ks_int_news(ks_sint val) {
    ks_int self = KS_NEW(ks_int, T_TYPE);
    if (!self) {
        return NULL;
    }

    /* Initialize and set the mpz object */
    mpz_init(self->val);
    mpz_set_si(self->val, val);

    return self;

}

ks_int ks_int_newu(ks_uint val) {
    ks_int self = KS_NEW(ks_int, T_TYPE);
    if (!self) {
        return NULL;
    }

    /* Initialize and set the mpz object */
    mpz_init(self->val);
    mpz_set_ui(self->val, val);

    return self;
}


/* Functions */

static KS_TFUNC(T, free) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

#if defined(KS_INT_GMP)

    mpz_clear(self->val);

#elif defined(KS_INT_MINIGMP)

    mpz_clear(self->val);

#endif

    KS_DEL(self);
    return KS_NONE;
}

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_int() {

    ks_init_type(T_TYPE, kst_number, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

    ));
}
