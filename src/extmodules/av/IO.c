/* av/IO.c - 'av.IO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>
#include <ks/av.h>


#define T_NAME "av.IO"

/* Internals */

/* C-API */


ksav_IO ksav_open(ks_type tp, ks_str src, ks_str mode) {
#ifdef KS_HAVE_libav
    ksav_IO self = KSO_NEW(ksav_IO, tp);

    /* Copy over what it was created with */
    KS_INCREF(src);
    self->src = src;
    KS_INCREF(mode);
    self->mode = mode;

    if (!ks_str_eq_c(mode, "r", 1)) {
        KS_THROW(kst_Error, "Only 'r' mode is supported right now");
        KS_DECREF(self);
        return NULL;
    }

    /* TODO: custom IO objects? */
    self->iio = KSO_NONE;

    /* libav status */
    int avst;

    /* Open the file as a media container */
    if ((avst = avformat_open_input(&self->fmtctx, src->data, NULL, NULL)) != 0) {
        KS_THROW(kst_Error, "Failed to open %R: %s [%i]", src, av_err2str(avst), avst);
        KS_DECREF(self);
        return false;
    }

    /* Find all the streams in the media container (so the data is valid) */
    if ((avst = avformat_find_stream_info(self->fmtctx, NULL)) != 0) {
        KS_THROW(kst_Error, "Failed to open %R: %s [%i]", src, av_err2str(avst), avst);
        KS_DECREF(self);
        return false;
    }

    /* Allocate data buffers */
    if (!(self->qf = av_frame_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_frame_alloc()'");
        KS_DECREF(self);
        return NULL;
    }
    if (!(self->qp = av_packet_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_packet_alloc()'");
        KS_DECREF(self);
        return NULL;
    }


    int i;

    /* Create streams */
    self->nstreams = self->fmtctx->nb_streams;
    self->streams = ks_zmalloc(sizeof(*self->streams), self->nstreams);

    for (i = 0; i < self->nstreams; ++i) {
        struct ksav_IO_stream* s = &self->streams[i];
        s->fcidx = i;

        /* Get libav structures */
        AVStream* stream = self->fmtctx->streams[s->fcidx];
        AVCodecParameters* codpar = stream->codecpar;
        AVCodec* cod = avcodec_find_decoder(codpar->codec_id);

        /* Allocate the encoding/decoding context */
        s->codctx = avcodec_alloc_context3(cod);
        if (!s->codctx) {
            KS_THROW(kst_Error, "Failed: 'avcodec_alloc_context3()'");
            KS_DECREF(self);
            return NULL;
        }

        /* Set options */
        //av_dict_set(&opt, "thread_type", "slice", 0);

        /* Set pixel format negotiator (TODO: check audio vs video) */
        s->codctx->get_format = ksav_AV_getformat;

        /* Get parameters */
        if ((avst = avcodec_parameters_to_context(s->codctx, codpar)) < 0) {
            KS_THROW(kst_Error, "Failed 'avcodec_parameters_to_context()': %s [%i]", av_err2str(avst), avst);
            KS_DECREF(self);
            return NULL;
        }

        /* Open the stream */
        if ((avst = avcodec_open2(s->codctx, cod, NULL)) < 0) {
            KS_THROW(kst_IOError, "Failed to open decoder for stream #%i in %R: %s %i", i, src, av_err2str(avst), avst);
            KS_DECREF(self);
            return NULL;
        }
    }

    return self;
#else
    KS_THROW(kst_Error, "Failed to open media %R: was not compiled with 'libav'", src);
    return NULL;
#endif
}

kso ksav_next(ksav_IO self, int* sidx) {
#ifdef KS_HAVE_libav

    /* Get quick buffers (TODO: allocate if being used) */
    AVFrame* frame = self->qf;
    self->qf = NULL;
    assert(frame);
    AVPacket* packet = self->qp;
    self->qp = NULL;
    assert(packet);

    int avst;

    /* Read a frame (still encoded) into packet */
    if ((avst = av_read_frame(self->fmtctx, packet)) < 0) {
        if (avst == AVERROR_EOF) {
            KS_OUTOFITER();
            self->qf = frame;
            self->qp = packet;
            return NULL;
        }
        self->qf = frame;
        self->qp = packet;
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    assert (packet->stream_index >= 0 && packet->stream_index < self->nstreams);
    struct ksav_IO_stream* s = &self->streams[packet->stream_index];
    assert(s->fcidx == packet->stream_index);

    AVStream* stream = self->fmtctx->streams[s->fcidx];
    *sidx = s->fcidx;

    /* Send packet to the decode */
    if ((avst = avcodec_send_packet(s->codctx, packet)) < 0) {
        if (avst == AVERROR_EOF) {
            self->qf = frame;
            self->qp = packet;
            KS_OUTOFITER();
            return NULL;
        }
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        self->qf = frame;
        self->qp = packet;
        return NULL;
    }

    /* Receive a frame back */
    if ((avst = avcodec_receive_frame(s->codctx, frame)) < 0) {
        if (avst == AVERROR_EOF) {
            KS_OUTOFITER();
            self->qf = frame;
            self->qp = packet;
            return NULL;
        }
        self->qf = frame;
        self->qp = packet;
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /** Actually decode **/
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

        /* Size of the image (in pixels) */
        int w = frame->width, h = frame->height;

        /*  Channel depth */
        int d = -1;

        /* Actual pixel format we are going to use (filter bad ones) */
        enum AVPixelFormat pix_fmt = ksav_AV_filterfmt(frame->format);

        /* Now, specify input and output */
        nx_dtype idt = NULL, odt = nxd_float;


        /* Formats that are easy to process */
        if (pix_fmt == AV_PIX_FMT_RGBA || pix_fmt == AV_PIX_FMT_RGB24 || pix_fmt == AV_PIX_FMT_RGB0) {
            /* RGBA, RGB24, and RGB0 are unsigned bytes, already in RGB order */
            idt = nxd_uchar;

            /* Stride in elements */
            int stride = 0;

            if (pix_fmt == AV_PIX_FMT_RGBA)  { 
                d = 4;
                stride = 4;
            } else if (pix_fmt == AV_PIX_FMT_RGB0) { 
                d = 3;
                stride = 4;
            } else if (pix_fmt == AV_PIX_FMT_RGB24) { 
                d = 3;
                stride = 3;
            } else {
                assert(false);
            }

            /* Construct result */
            nx_array res = nx_array_newc(nxt_array, odt, 3, (ks_size_t[]){ h, w, d }, NULL, NULL);

            if (!nx_fpcast(res->ar, (nxar_t) {
                frame->data[0],
                idt,
                3,
                (ks_size_t[]) { h, w, d },
                (ks_ssize_t[]){ frame->linesize[0], stride * idt->size, idt->size }
            })) {
                KS_DECREF(res);
                self->qf = frame;
                self->qp = packet;
                return NULL;
            }

            /* Restore quick data buffers */
            self->qf = frame;
            self->qp = packet;

            return (kso)res;


        } else {
            /* Weird format, so scale in software */

            /* Convert to grayscale (attempt) */
            enum AVPixelFormat new_fmt = AV_PIX_FMT_GRAY8;

            /* Convert to RGB if we would lose chroma */
            if (av_get_pix_fmt_loss(new_fmt, pix_fmt, true) & FF_LOSS_CHROMA) {
                new_fmt = AV_PIX_FMT_RGB24;
            }

            /* Ensure alpha is included if the format would lose it */
            if (av_get_pix_fmt_loss(new_fmt, pix_fmt, true) & FF_LOSS_ALPHA) {
                new_fmt = AV_PIX_FMT_RGBA;
            }

            /* Stride in elements */
            int stride = 0;

            if (new_fmt == AV_PIX_FMT_GRAY8)  { 
                d = 1;
                stride = 1;
                idt = nxd_uchar;
            } else if (new_fmt == AV_PIX_FMT_RGB24) { 
                d = 3;
                stride = 3;
                idt = nxd_uchar;
            } else if (new_fmt == AV_PIX_FMT_RGBA) { 
                d = 4;
                stride = 4;
                idt = nxd_uchar;
            } else {
                assert(false);
            }

            static struct SwsContext *sws_context = NULL;

            /* Get the conversion context */
            sws_context = sws_getCachedContext(sws_context,
                w, h, pix_fmt,
                w, h, new_fmt,
                0, NULL, NULL, NULL
            );

            /* Compute line size and temporary data */
            ks_size_t linesize = stride * w;
            linesize += (16 - linesize) % 16;

            char* tmp_data = ks_malloc(linesize * h);

            /* Convert data into 'tmp_data' */
            sws_scale(sws_context, (const uint8_t* const*)frame->data, frame->linesize, 0, h, (uint8_t* const[]){ tmp_data, NULL, NULL, NULL }, (int[]){ linesize, 0, 0, 0 });

            /* Construct result */
            nx_array res = nx_array_newc(nxt_array, odt, 3, (ks_size_t[]){ h, w, d }, NULL, NULL);

            if (!nx_fpcast(
                res->ar, 
            (nxar_t) {
                tmp_data,
                idt,
                3,
                (ks_size_t[]) { h, w, d },
                (ks_ssize_t[]){ linesize, stride * idt->size, idt->size }
            })) {
                KS_DECREF(res);
                self->qf = frame;
                self->qp = packet;
                free(tmp_data);
                return NULL;
            }

            free(tmp_data);
            /* Restore quick data buffers */
            self->qf = frame;
            self->qp = packet;

            return (kso)res;
        }

    } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        /* Return audio data */


        /* Restore quick data buffers */
        self->qf = frame;
        self->qp = packet;

        return KSO_NONE;
    } else {
        /* Restore quick data buffers */
        KS_THROW(kst_Error, "Don't know what to return for stream #%i in %R (unknown type, not video or audio)", *sidx, self->src);

        self->qf = frame;
        self->qp = packet;
        return NULL;
    }

#else
    KS_THROW(kst_Error, "Failed to get next from %R: was not compiled with 'libav'", self->src);
    return NULL;
#endif
}


bool ksav_isaudio(ksav_IO self, int sidx, bool* out) {
#ifdef KS_HAVE_libav
    AVStream* stream = self->fmtctx->streams[sidx];
    *out = stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
    return true;
#else
    KS_THROW(kst_Error, "Cannot tell if %R #%i is audio or not", self->src, sidx);
    return false;
#endif
}

bool ksav_isvideo(ksav_IO self, int sidx, bool* out) {
#ifdef KS_HAVE_libav
    AVStream* stream = self->fmtctx->streams[sidx];
    *out = stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
    return true;
#else
    KS_THROW(kst_Error, "Cannot tell if %R #%i is audio or not", self->src, sidx);
    return false;
#endif
}

int ksav_bestaudio(ksav_IO self) {
#ifdef KS_HAVE_libav
    int res = av_find_best_stream(self->fmtctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (res < 0) {
        KS_THROW(kst_Error, "Failed to find best audio stream in %R: %s [%i]", self->src, av_err2str(res), res);
        return -1;
    }
    return res;
#else
    KS_THROW(kst_Error, "Failed to find best audio stream in %R: not compiled with libav", self->src);
    return -1;
#endif

}

int ksav_bestvideo(ksav_IO self) {
#ifdef KS_HAVE_libav
    int res = av_find_best_stream(self->fmtctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (res < 0) {
        KS_THROW(kst_Error, "Failed to find best video stream in %R: %s [%i]", self->src, av_err2str(res), res);
        return -1;
    }
    return res;
#else
    KS_THROW(kst_Error, "Failed to find best video stream in %R: not compiled with libav", self->src);
    return -1;
#endif
}


/* Type Functions */

static KS_TFUNC(T, free) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    KS_NDECREF(self->src);
    KS_NDECREF(self->mode);


#ifdef KS_HAVE_libav
    if (self->fmtctx) {
        avformat_close_input(&self->fmtctx);
    }

    if (self->qf) {
        av_frame_free(&self->qf);
    }
    if (self->qp) {
        av_packet_free(&self->qp);

    }
#endif

    int i;
    for (i = 0; i < self->nstreams; ++i) {
        struct ksav_IO_stream* s = &self->streams[i];
#ifdef KS_HAVE_libav
        if (s->codctx) {
            avcodec_free_context(&s->codctx);
        }
#endif
    }

    ks_free(self->streams);

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    return (kso)ks_fmt("<%T (src=%R, mode=%R)>", self, self->src, self->mode);
}

static KS_TFUNC(T, next) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    int stream;
    kso res = ksav_next(self, &stream);
    if (!res) return NULL;

    return (kso)ks_tuple_newn(2, (kso[]){
        (kso)ks_int_new(stream),
        res
    });
}


static KS_TFUNC(T, isaudio) {
    ksav_IO self;
    ks_cint idx = 0;
    KS_ARGS("self:* ?idx:cint", &self, ksavt_IO, &idx);

    if (idx < 0 || idx >= self->nstreams) {
        KS_THROW_INDEX(self, _args[1]);
        return NULL;
    }

    bool res;
    if (!ksav_isaudio(self, idx, &res)) return NULL;

    return KSO_BOOL(res);
}

static KS_TFUNC(T, isvideo) {
    ksav_IO self;
    ks_cint idx = 0;
    KS_ARGS("self:* ?idx:cint", &self, ksavt_IO, &idx);

    if (idx < 0 || idx >= self->nstreams) {
        KS_THROW_INDEX(self, _args[1]);
        return NULL;
    }

    bool res;
    if (!ksav_isvideo(self, idx, &res)) return NULL;

    return KSO_BOOL(res);
}



static KS_TFUNC(T, bestaudio) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    int res = ksav_bestaudio(self);
    if (res < 0) return NULL;

    return (kso)ks_int_new(res);
}

static KS_TFUNC(T, bestvideo) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    int res = ksav_bestvideo(self);
    if (res < 0) return NULL;

    return (kso)ks_int_new(res);
}


/* Export */

static struct ks_type_s tp;
ks_type ksavt_IO = &tp;

void _ksi_av_IO() {
    _ksinit(ksavt_IO, kst_object, T_NAME, sizeof(struct ksav_IO_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},
        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},
        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},


        {"isaudio",                ksf_wrap(T_isaudio_, T_NAME ".isaudio(self, idx=0)", "Tells whether stream 'idx' is an audio stream")},
        {"isvideo",                ksf_wrap(T_isvideo_, T_NAME ".isvideo(self, idx=0)", "Tells whether stream 'idx' is a video stream")},

        {"best_audio",             ksf_wrap(T_bestaudio_, T_NAME ".best_audio(self)", "Returns the index of the best audio stream\n\n    Throws an error if there were no audio streams")},
        {"best_video",             ksf_wrap(T_bestvideo_, T_NAME ".best_video(self)", "Tells whether stream 'idx' is a video stream\n\n    Throws an error if there were no video streams")},

    ));
}


