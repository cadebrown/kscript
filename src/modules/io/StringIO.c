/* io/StringIO.c - 'io.StringIO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>

#define T_NAME "io.StringIO"


/* C-API */

ksio_StringIO ksio_StringIO_new() {
    ksio_StringIO self = KSO_NEW(ksio_StringIO, ksiot_StringIO);

    self->len_b = self->len_c = 0;
    self->max_len_b = 0;

    self->is_r = self->is_w = true;
    self->sz_r = self->sz_w = false;

    self->data = NULL;

    return self;
}

ks_str ksio_StringIO_get(ksio_StringIO self) {
    return ks_str_new(self->len_b, self->data);
}
ks_str ksio_StringIO_getf(ksio_StringIO self) {
    ks_str res = NULL;
    if (self->refs == 1) {
        /* optimization: own the data, since we are about to free it */
        if (self->max_len_b < self->len_b + 1) {
            self->data = ks_realloc(self->data, self->len_b + 1);
        }
        res = ks_str_newn(self->len_b, self->data);
        self->data = NULL;
    } else {
        res = ksio_StringIO_get(self);
    }

    KS_DECREF(self);
    return res;
}



/* Type Functions */

static KS_TFUNC(T, free) {
    ksio_StringIO self;
    KS_ARGS("self:*", &self, ksiot_StringIO);

    ks_free(self->data);
    KSO_DEL(self);

    return KSO_NONE;
}

/* Export */

static struct ks_type_s tp;
ks_type ksiot_StringIO = &tp;

void _ksi_io_StringIO() {
    _ksinit(ksiot_StringIO, kst_object, T_NAME, sizeof(struct ksio_StringIO_s), -1, "In-memory string-based input/output which can append and build strings", KS_IKV(
        {"__free",               ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

    ));
}
