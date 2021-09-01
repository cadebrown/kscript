/* types/str.c - 
 *
 *
 *
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>

#define T ks_str
#define T_TYPE kst_str
#define T_NAME "str"
#define T_DOC "Immutable sequence of characters (Unicode codepoint), similar to a 'bytes', but operates as characters"


/* C-API */

/* Methods */

static KS_TFUNC(T, free) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    /* Free buffer */
    ks_free(self->data);

#if KS_STR_OFF_STRIDE

    /* If we include offset stride info, free it */
    ks_free(self->strides_);

#endif

    KS_DEL(self);
    
    return KS_NONE;
}

static KS_TFUNC(T, bool) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return KS_BOOL(self->len_c != 0);
}

static KS_TFUNC(T, len) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    return (kso)ks_int_newu(self->len_c);
}

static KS_TFUNC(T, encode) {
    T self;
    KS_ARGS("self:*", &self, T_TYPE);

    /* TODO: support other encodings */

    return (kso)ks_bytes_new(NULL, self->len_b, self->data);
}

/* Export */

KS_DECL_TYPE(T_TYPE)

void ksi_str() {

    ks_init_type(T_TYPE, kst_object, sizeof(*(T)NULL), T_NAME, T_DOC, KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__bool",                 ksf_wrap(T_bool_, T_NAME ".__bool(self)", "")},
        {"__len",                  ksf_wrap(T_len_, T_NAME ".__len(self)", "")},

        {"encode",                 ksf_wrap(T_encode_, T_NAME ".encode(self)", "Encode the string into a bytes object")},

    ));
}
