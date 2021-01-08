/* av/IO.c - 'av.IO' type
 *
 * 
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>
#include <ks/av.h>


#define T_NAME "av.IO"

/* Internals */

#ifdef KS_HAVE_libav

/* libav IO 'read' callback */
static int my_av_read(void* opaque, uint8_t* buf, int nbuf) {
    ksav_IO self = (ksav_IO)opaque;

    ks_ssize_t rsz = ksio_readb(self->iio, nbuf, buf);
    if (rsz < 0) {
        self->cbexc = true;
        return -1;
    }

    return rsz;
}

/* libav IO 'write' callback */
static int my_av_write(void* opaque, uint8_t* buf, int nbuf) {
    ksav_IO self = (ksav_IO)opaque;

    ks_ssize_t rsz = ksio_writeb(self->iio, nbuf, buf);
    if (rsz < 0) {
        self->cbexc = true;
        return -1;
    }

    return rsz;
}

/* libav IO 'seek' callback */
static int64_t my_av_seek(void* opaque, int64_t pos, int whence) {
    ksav_IO self = (ksav_IO)opaque;

    int ww;
    if (whence == SEEK_SET) {
        ww = KSIO_SEEK_SET;
    } else if (whence == SEEK_CUR) {
        ww = KSIO_SEEK_CUR;
    } else if (whence == SEEK_END) {
        ww = KSIO_SEEK_END;
    } else {
        return -1;
    }

    if (!ksio_seek(self->iio, pos, ww)) {
        self->cbexc = true;
        return -1;
    }

    return ksio_tell(self->iio);
}

#endif


/* C-API */


ksav_IO ksav_open(ks_type tp, kso iio, ks_str src, ks_str mode) {
#ifdef KS_HAVE_libav
    ksav_IO self = KSO_NEW(ksav_IO, tp);

    /* Copy over what it was created with */
    KS_INCREF(src);
    self->src = src;
    KS_INCREF(mode);
    self->mode = mode;

    self->cbexc = false;

    /* Create a mutex */
    self->mut = ksos_mutex_new(ksost_mutex);

    /* Initialize the queue to empty */
    self->queue_first = self->queue_last = NULL;

    if (!ks_str_eq_c(mode, "r", 1)) {
        KS_THROW(kst_Error, "Only 'r' mode is supported right now");
        KS_DECREF(self);
        return NULL;
    }

    /* TODO: custom IO objects? */
    if (iio == KSO_NONE) {
        self->iio = (kso)kso_call((kso)ksiot_FileIO, 2, (kso[]){ (kso)src, (kso)mode });
        if (!self->iio) {
            KS_DECREF(self);
            return NULL;
        }
    } else {
        KS_INCREF(iio);
        self->iio = iio;
    }


    /* Lock the mutex to create anything */
    ksos_mutex_lock(self->mut);

    /* libav status */
    int avst;

    /* Create an IO context for our custom functions */
    if (!(self->avioctx = avio_alloc_context(NULL, 0, 0, (kso)self, my_av_read, my_av_write, my_av_seek))) {
        KS_THROW(kst_Error, "Failed to allocate AVIO context");
        KS_DECREF(self);
        return false;
    }

    if (!(self->fmtctx = avformat_alloc_context())) {
        KS_THROW(kst_Error, "Failed to allocate AV format context");
        KS_DECREF(self);
        return false;
    }

    /* Set our custom IO objects */
    self->fmtctx->pb = self->avioctx;

    /* Open the file as a media container */
    if ((avst = avformat_open_input(&self->fmtctx, src->data, NULL, NULL)) != 0) {
        self->fmtctx = NULL;
        if (self->cbexc) {
            KS_DECREF(self);
            return NULL;
        }
        KS_THROW(kst_Error, "Failed to open %R: %s [%i]", src, av_err2str(avst), avst);
        KS_DECREF(self);
        return false;
    }

    /* Find all the streams in the media container (so the data is valid) */
    if ((avst = avformat_find_stream_info(self->fmtctx, NULL)) != 0) {
        if (self->cbexc) {
            KS_DECREF(self);
            return NULL;
        }
        KS_THROW(kst_Error, "Failed to open %R: %s [%i]", src, av_err2str(avst), avst);
        KS_DECREF(self);
        return false;
    }

    /* Allocate data buffers */
    if (!(self->frame = av_frame_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_frame_alloc()'");
        KS_DECREF(self);
        return NULL;
    }
    if (!(self->packet = av_packet_alloc())) {
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
            if (self->cbexc) {
                KS_DECREF(self);
                return NULL;
            }
            KS_THROW(kst_Error, "Failed 'avcodec_parameters_to_context()': %s [%i]", av_err2str(avst), avst);
            KS_DECREF(self);
            return NULL;
        }

        /* Open the stream */
        if ((avst = avcodec_open2(s->codctx, cod, NULL)) < 0) {
            if (self->cbexc) {
                KS_DECREF(self);
                return NULL;
            }
            KS_THROW(kst_IOError, "Failed to open decoder for stream #%i in %R: %s %i", i, src, av_err2str(avst), avst);
            KS_DECREF(self);
            return NULL;
        }
    }

    /* Done creating stuff */
    ksos_mutex_unlock(self->mut);
    return self;
#else
    KS_THROW(kst_Error, "Failed to open media %R: was not compiled with 'libav'", src);
    return NULL;
#endif
}


#ifdef KS_HAVE_libav

/* Internally return the object that should be returned from a packet */
static kso ksav_decode(ksav_IO self, int sidx, AVPacket* packet) {
    struct ksav_IO_stream* s = &self->streams[sidx];
    AVStream* stream = self->fmtctx->streams[s->fcidx];
    int avst;

    /* Send packet to the decoder */
    if ((avst = avcodec_send_packet(s->codctx, packet)) < 0) {
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /* Receive a frame back */
    if ((avst = avcodec_receive_frame(s->codctx, self->frame)) < 0) {
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /** Actually decode **/
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

        /* Size of the image (in pixels) */
        int w = self->frame->width, h = self->frame->height;

        /*  Channel depth */
        int d = -1;

        /* Actual pixel format we are going to use (filter bad ones) */
        enum AVPixelFormat pix_fmt = ksav_AV_filterfmt(self->frame->format);

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
                self->frame->data[0],
                idt,
                3,
                (ks_size_t[]) { h, w, d },
                (ks_ssize_t[]){ self->frame->linesize[0], stride * idt->size, idt->size }
            })) {
                KS_DECREF(res);
                return NULL;
            }

            /* Restore quick data buffers */
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

            /* Get the conversion context */
            self->swsctx = sws_getCachedContext(self->swsctx,
                w, h, pix_fmt,
                w, h, new_fmt,
                0, NULL, NULL, NULL
            );

            /* Compute line size and temporary data */
            ks_size_t linesize = stride * w;
            linesize += (16 - linesize) % 16;

            char* tmp_data = ks_malloc(linesize * h);

            /* Convert data into 'tmp_data' */
            sws_scale(self->swsctx, (const uint8_t* const*)self->frame->data, self->frame->linesize, 0, h, (uint8_t* const[]){ tmp_data, NULL, NULL, NULL }, (int[]){ linesize, 0, 0, 0 });

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
                ks_free(tmp_data);
                return NULL;
            }

            ks_free(tmp_data);
            return (kso)res;
        }

    } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        /* Return audio data */

        /* Number of channels in the audio */
        int channels = s->codctx->channels;

        /* Sample rate */
        int hz = s->codctx->sample_rate;

        /* Number of datapoints */
        int nsamp = self->frame->nb_samples;

        /* Format of each sample */
        enum AVSampleFormat smpfmt = self->frame->format;

        /* Create data to hold the result */
        nxc_float* tmp_data = ks_zmalloc(sizeof(*tmp_data), channels * nsamp);

        /* The samples to use (depends on the sample format) */
        void** samples = (void**)self->frame->data; 

        int i, j;

        switch (smpfmt)
        {
        case AV_SAMPLE_FMT_FLTP:
            for (j = 0; j < channels; ++j) {
                for (i = 0; i < nsamp; ++i) {
                    tmp_data[i * channels + j] = ((float**)samples)[j][i];
                }
            }
            break;
        
        default:
            ks_free(tmp_data);
            KS_THROW(kst_Error, "Unknown sample format from libav: %s", av_get_sample_fmt_name(smpfmt));
            break;
        }


        /* Construct result */
        nx_array res = nx_array_newc(nxt_array, nxd_float, 2, (ks_size_t[]){ nsamp, channels }, (ks_ssize_t[]){ channels * sizeof(*tmp_data), sizeof(*tmp_data) }, tmp_data);
        ks_free(tmp_data);

        return (kso)res;
    } else {
        /* Restore quick data buffers */
        KS_THROW(kst_Error, "Don't know what to return for stream #%i in %R (unknown type, not video or audio)", sidx, self->src);
        return NULL;
    }
}

#endif


kso ksav_next(ksav_IO self, int* sidx, int nvalid, int* valid) {
#ifdef KS_HAVE_libav
    int i, avst;

    /* Iterate over the queue of items already read, but not consumed */
    struct ksav_IO_queue_item *it = NULL, *lit = NULL;
    it = self->queue_first;
    int ct =0;
    while (it) {
        ct++;
        it = it->next;
    }

    it = self->queue_first;
    lit = NULL;
    while (it) {
        /* Check if this stream should be consumed */
        bool g = nvalid < 0;
        if (!g) {
            for (i = 0; !g && i < nvalid; ++i) {
                g = valid[i] == it->sidx;
            }
        }
        if (g) {
            /* Found a match, so consume it */

            /* Delete 'it' from the queue */
            if (lit) {
                lit->next = it->next;
            } else {
                /* It was the first element */
                self->queue_first = it->next;
            }
            if (it == self->queue_last) self->queue_last = lit;

            /* Set the information and decode it */
            *sidx = it->packet->stream_index;
            kso res = ksav_decode(self, *sidx, it->packet);

            /* Free queue item */
            av_packet_free(&it->packet);
            ks_free(it);

            return res;
        }

        /* Advance iterator */
        lit = it;
        it = it->next;
    }

    /* Not found, so now consume encoded frames until we find one that matches */

    while (true) {
        /* Read a frame (still encoded) into packet */
        if ((avst = av_read_frame(self->fmtctx, self->packet)) < 0) {
            if (avst == AVERROR_EOF) {
                KS_OUTOFITER();
                return NULL;
            }
            KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
            return NULL;
        }

        /* Check if this stream should be consumed */
        bool g = nvalid < 0;
        if (!g) {
            for (i = 0; !g && i < nvalid; ++i) {
                g = valid[i] == self->packet->stream_index;
            }
        }
        if (g) {
            /* Found a match, so consume it */
            *sidx = self->packet->stream_index;
            kso res = ksav_decode(self, *sidx, self->packet);

            /* Done with packet, but don't free it since we are using it as a quick buffer */
            av_packet_unref(self->packet);
            return res;
        }

        /* Append to the end of the queue */
        it = ks_malloc(sizeof(*self->queue_last));
        it->next = NULL;
        it->sidx = self->packet->stream_index;
        it->packet = self->packet;

        if (!self->queue_first) self->queue_first = it;
        if (self->queue_last) self->queue_last->next = it;
        self->queue_last = it;

        if (!(self->packet = av_packet_alloc())) {
            /* Failed to allocate */
            KS_THROW(kst_Error, "Failed to allocate packet via 'av_packet_alloc()");
            return NULL;
        }
    }

    assert(false);
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
    KS_NDECREF(self->mut);

#ifdef KS_HAVE_libav
    
    avformat_close_input(&self->fmtctx);
    avio_context_free(&self->avioctx);

    if (self->frame) {
        av_frame_free(&self->frame);
    }
    if (self->packet) {
        av_packet_free(&self->packet);
    }

    struct ksav_IO_queue_item* it = self->queue_first;
    while (it) {
        struct ksav_IO_queue_item* t = it;
        it = t->next;
        
        av_packet_free(&t->packet);
        ks_free(t);
    }

    sws_freeContext(self->swsctx);

#endif

    int i;
    for (i = 0; i < self->nstreams; ++i) {
        struct ksav_IO_stream* s = &self->streams[i];
#ifdef KS_HAVE_libav
        avcodec_free_context(&s->codctx);
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
    kso res = ksav_next(self, &stream, -1, NULL);
    if (!res) return NULL;

    return (kso)ks_tuple_newn(2, (kso[]){
        (kso)ks_int_new(stream),
        res
    });
}

static KS_TFUNC(T, stream) {
    ksav_IO self;
    ks_cint idx = 0;
    KS_ARGS("self:* ?idx:cint", &self, ksavt_IO, &idx);

    return (kso)ksav_getstream(self, idx);
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

    return (kso)ksav_getstream(self, res);
}

static KS_TFUNC(T, bestvideo) {
    ksav_IO self;
    KS_ARGS("self:*", &self, ksavt_IO);

    int res = ksav_bestvideo(self);
    if (res < 0) return NULL;
    
    return (kso)ksav_getstream(self, res);
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

        {"stream",                 ksf_wrap(T_stream_, T_NAME ".stream(self, idx=0)", "Gets the 'idx'th stream")},

        {"isaudio",                ksf_wrap(T_isaudio_, T_NAME ".isaudio(self, idx=0)", "Tells whether stream 'idx' is an audio stream")},
        {"isvideo",                ksf_wrap(T_isvideo_, T_NAME ".isvideo(self, idx=0)", "Tells whether stream 'idx' is a video stream")},

        {"best_audio",             ksf_wrap(T_bestaudio_, T_NAME ".best_audio(self)", "Returns the best audio stream in the media container")},
        {"best_video",             ksf_wrap(T_bestvideo_, T_NAME ".best_video(self)", "Returns the best video stream in the media container")},

    ));
}


