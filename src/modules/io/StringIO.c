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

    self->is_r = self->is_w = true;
    self->sz_r = self->sz_w = false;

    self->data = NULL;

    return self;
}

ks_str ksio_StringIO_get(ksio_StringIO self) {
    ks_str res = ks_str_new(self->len_b, self->data);
    return res;
}
ks_str ksio_StringIO_getf(ksio_StringIO self) {
    ks_str res = ksio_StringIO_get(self);
    KS_DECREF(self);
    return res;
}



/* Type Functions */

/* Export */

static struct ks_type_s tp;
ks_type ksiot_StringIO = &tp;

void _ksi_io_StringIO() {
    _ksinit(ksiot_StringIO, kst_object, T_NAME, sizeof(struct ksio_StringIO_s), -1, NULL);

    
}
