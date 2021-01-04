/* mm/MediaFile.c - 'mm.MediaFile' type

 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/mm.h>

#define T_NAME "mm.MediaFile"

/* Internals */

/* C-API */


ksmm_MediaFile ksmm_MediaFile_open(ks_type tp, ks_str src) {
    ksmm_MediaFile self = KSO_NEW(ksmm_MediaFile, tp);

    self->streams = ks_list_new(0, NULL);

    KS_INCREF(src);
    self->src = src;

#ifdef KS_HAVE_libav
    self->fmtctx = ksmm_AVFormatContext_wrap(avformat_alloc_context());

    if (avformat_open_input(&self->fmtctx->val, src->data, NULL, NULL) != 0) {
        KS_THROW(kst_Error, "Could not open file: %R", src);
        KS_DECREF(self);
        return false;
    }

    int i;
    for (i = 0; i < self->fmtctx->val->nb_streams; ++i) {
        ksmm_Stream s = KSO_NEW(ksmm_Stream, ksmmt_Stream);

        KS_INCREF(src);
        s->src = src;
        s->is_open = false;
        s->idx = i;
        s->stream = self->fmtctx->val->streams[i];
        s->fmtctx = self->fmtctx;
        KS_INCREF(self->fmtctx);

        AVCodec* codec = avcodec_find_decoder(s->stream->codec->codec_id);

        if (avcodec_open2(s->stream->codec, codec, NULL) < 0) {
            KS_THROW(kst_IOError, "Could not open decoder for stream #%i in file %R", i, src);
            KS_DECREF(self);
            KS_DECREF(s);
            return NULL;
        }

        ks_list_pushu(self->streams, (kso)s);
    }

#endif

    return self;
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksmm_MediaFile self;
    KS_ARGS("self:*", &self, ksmmt_MediaFile);

    KS_NDECREF(self->fmtctx);
    KS_NDECREF(self->streams);
    KS_NDECREF(self->src);

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksmm_MediaFile self;
    KS_ARGS("self:*", &self, ksmmt_MediaFile);

    return (kso)ks_fmt("<%T src=%R>", self, self->src);
}

static KS_TFUNC(T, getattr) {
    ksmm_MediaFile self;
    ks_str attr;
    KS_ARGS("self:* attr:*", &self, ksmmt_MediaFile, &attr, kst_str);

    if (ks_str_eq_c(attr, "streams", 7)) {
        return KS_NEWREF(self->streams);
    }

    KS_THROW_ATTR(self, attr);
    return NULL;
}

/* Export */

static struct ks_type_s tp;
ks_type ksmmt_MediaFile = &tp;

void _ksi_mm_MediaFile() {
    _ksinit(ksmmt_MediaFile, kst_object, T_NAME, sizeof(struct ksmm_MediaFile_s), -1, "Media file", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__getattr",              ksf_wrap(T_getattr_, T_NAME ".__getattr(self, attr)", "")},
    ));
}

