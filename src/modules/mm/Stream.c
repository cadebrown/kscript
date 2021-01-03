/* mm/Stream.c - 'mm.Stream' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/mm.h>

#define T_NAME "mm.Stream"

/* Internals */

/* C-API */


ksmm_Stream ksmm_Stream_open(ks_type tp, ks_str url) {
    ksmm_Stream self = KSO_NEW(ksmm_Stream, tp);

    self->fmtctx = avformat_alloc_context();

    if (avformat_open_input(&self->fmtctx, url, NULL, NULL) != 0) {
        KS_THROW(kst_Error, "Could not open file: %R", url);
        KS_DECREF(self);
        return false;
    }

    self->nsub = self->fmtctx->nb_streams;
    self->sub = ks_zmalloc(sizeof(*self->sub), self->nsub);

    int i;
    for (i = 0; i < self->nsub; ++i) {
        self->sub[i].is_open = false;
        self->sub[i].stream = self->fmtctx->streams[i];
        self->sub[i].codec_ctx = self->sub[i].stream->codec;

     //   if (avcodec_open2(self->sub[i]))

    }



    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksost_thread);


    KSO_DEL(self);
    return KSO_NONE;
}


/* Export */

/*&
static struct ks_type_s tp;
ks_type ksost_thread = &tp;

void _ksi_os_thread() {
    _ksinit(ksost_thread, kst_object, T_NAME, sizeof(struct ksos_thread_s), -1, "Thread of execution, which is a single strand of execution happening (possibly) at the same time as other threads\n\n    Although these are typically wrapped by OS-level threads, there is also the Global Interpreter Lock (GIL) which prevents bytecodes from executing at the same time", KS_IKV(
        {"__new",                  ksf_wrap(T_new_, T_NAME ".__new(tp, of, args=none)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},

        {"start",                  ksf_wrap(T_start_, T_NAME ".start(self)", "Start executing a thread")},
        {"join",                   ksf_wrap(T_join_, T_NAME ".join(self)", "Wait for a thread to finish executing")},
    ));
}




*/
