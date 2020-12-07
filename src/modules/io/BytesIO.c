/* io/BytesIO.c - 'io.BytesIO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.BytesIO"


/* C-API */

ksio_BytesIO ksio_BytesIO_new() {
    ksio_BytesIO self = KSO_NEW(ksio_BytesIO, ksiot_BytesIO);

    self->len_b = 0;
    self->max_len_b = 0;

    self->is_r = self->is_w = true;
    self->sz_r = self->sz_w = false;

    self->data = NULL;

    return self;
}

ks_bytes ksio_BytesIO_get(ksio_BytesIO self) {
    return ks_bytes_new(self->len_b, self->data);
}
ks_bytes ksio_BytesIO_getf(ksio_BytesIO self) {
    ks_bytes res = NULL;
    if (self->refs == 1) {
        /* optimization: own the data, since we are about to free it */
        res = ks_bytes_newn(self->len_b, self->data);
        self->data = NULL;
    } else {
        res = ksio_BytesIO_get(self);
    }

    KS_DECREF(self);
    return res;
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ksio_BytesIO self;
    KS_ARGS("self:*", &self, ksiot_BytesIO);

    ks_free(self->data);
    KSO_DEL(self);

    return KSO_NONE;
}

/* Export */

static struct ks_type_s tp;
ks_type ksiot_BytesIO = &tp;

void _ksi_io_BytesIO() {
    _ksinit(ksiot_BytesIO, kst_object, T_NAME, sizeof(struct ksio_BytesIO_s), -1, "In memory input/output for data in the form of 'bytes' objects", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

    ));
}
