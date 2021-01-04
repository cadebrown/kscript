/* mm/AVFormatContext.c - 'mm._AVFormatContext' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>
#include <ks/mm.h>
#ifdef KS_HAVE_libav

#define T_NAME "mm._AVFormatContext"

/* Internals */

/* C-API */


ksmm_AVFormatContext ksmm_AVFormatContext_wrap(AVFormatContext* val) {
    ksmm_AVFormatContext self = KSO_NEW(ksmm_AVFormatContext, ksmmt_AVFormatContext);

    self->val = val;

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksmm_AVFormatContext self;
    KS_ARGS("self:*", &self, ksmmt_AVFormatContext);

    if (self->val) {
        avformat_close_input(&self->val);
    }

    KSO_DEL(self);
    return KSO_NONE;
}


/* Export */

static struct ks_type_s tp;
ks_type ksmmt_AVFormatContext = &tp;

void _ksi_mm_AVFormatContext() {
    _ksinit(ksmmt_AVFormatContext, kst_object, T_NAME, sizeof(struct ksmm_AVFormatContext_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

    ));
}

#endif

