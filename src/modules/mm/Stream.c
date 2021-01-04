/* mm/Stream.c - 'mm.Stream' type
 * 
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/mm.h>

#define T_NAME "mm.Stream"

/* Internals */

/* C-API */

nx_array ksmm_read_image(ksmm_Stream self) {

#ifdef KS_HAVE_libav

    /* Format for a 2D image */
    AVInputFormat* inpfmt = NULL;
    AVFormatContext* fmtctx = self->fmtctx->val;

    /* Get the image2 input format */
    if (!(inpfmt = av_find_input_format("image2"))) {
        KS_THROW(kst_Error, "Failed to find input format: 'image2'");
        return NULL;
    }

    /* Get the stream we are reading from */
    AVStream* stream = fmtctx->streams[self->idx];

    /* Options dictionary */
    AVDictionary* opt = NULL;

    /* Data */
    AVFrame* frame = NULL;
    AVPacket* packet = NULL;

    /* Codec */
    AVCodecParameters* codpar = stream->codecpar;

    /* libav status */
    int avst;

    /* Get a context for the specific codec 
     * TODO: is there a better way? I get deprecated warnings when using this
     */
    AVCodecContext* codctx = avcodec_alloc_context3(stream->codec->codec);
    if (!codctx) {
        KS_THROW(kst_Error, "Failed: 'avcodec_alloc_context3()'");
        avcodec_free_context(&codctx);
        return NULL;
    }

    /* Get parameters */
    if ((avst = avcodec_parameters_to_context(codctx, codpar)) < 0) {
        KS_THROW(kst_Error, "Failed 'avcodec_parameters_to_context()': %s [%i]", av_err2str(avst), avst);
        avcodec_free_context(&codctx);
        return NULL;
    }

    /* Allocate data buffers */
    if (!(frame = av_frame_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_frame_alloc()'");
        avcodec_free_context(&codctx);
        return NULL;
    }
    if (!(packet = av_packet_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_packet_alloc()'");
        avcodec_free_context(&codctx);
        av_frame_free(&frame);
        return NULL;
    }

    /* Set options */
    av_dict_set(&opt, "thread_type", "slice", 0);

    /* Set pixel format negotiator */
    codctx->get_format = ksmm_AV_getformat;


    /* Open the codec context */
    if ((avst = avcodec_open2(codctx, codctx->codec, &opt)) < 0) {
        KS_THROW(kst_Error, "Failed to open codec for %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /* Read a frame (still encoded) into packet */
    if ((avst = av_read_frame(fmtctx, packet)) < 0) {
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /* Send packet to the decode */
    if ((avst = avcodec_send_packet(codctx, packet)) < 0) {
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /* Receive a frame back */
    if ((avst = avcodec_receive_frame(codctx, frame)) < 0) {
        KS_THROW(kst_Error, "Failed to read frame from file %R: %s [%i]", self->src, av_err2str(avst), avst);
        return NULL;
    }

    /** Actually decode **/

    /* Shape of the tensor (chn < 0 indiciates it is unknown) */
    int h = frame->height, w = frame->width, out_chn = -1, in_chn = -1;

    /* Channel stride (padding per channel) */
    int in_chn_stride = -1;

    /* Input datatype */
    nx_dtype indt = NULL;

    /* Actual pixel format */
    enum AVPixelFormat pix_fmt = frame->format;

    /* Filter to replace bad formats */
    pix_fmt = ksmm_AV_filterfmt(pix_fmt);

    /* Result array */
    nx_array res = NULL;

    /* Return data type */
    nx_dtype rdt = nxd_float;

    /* Formats that are easy to process */
    if (pix_fmt == AV_PIX_FMT_RGBA || pix_fmt == AV_PIX_FMT_RGB24 || pix_fmt == AV_PIX_FMT_RGB0) {

        /* RGB[A] formats are unsigned 8 bit values */

        /**/ if (pix_fmt == AV_PIX_FMT_RGBA)  { in_chn = 4; in_chn_stride = 4; }
        else if (pix_fmt == AV_PIX_FMT_RGB0)  { in_chn = 3; in_chn_stride = 4; }
        else if (pix_fmt == AV_PIX_FMT_RGB24) { in_chn = 3; in_chn_stride = 3; }


        /* Input datatype is bytes */
        indt = nxd_uchar;
        if (out_chn <= 0) out_chn = in_chn;

        assert(in_chn > 0 && in_chn_stride > 0 && out_chn >= 0 && "layout was not calculated!");

        /* Construct result */
        res = nx_array_newc(nxt_array, rdt, 3, (ks_size_t[]){ h, w, out_chn }, NULL, NULL);

        /* Scaling factor */
        float scale_fac = 1.0;
        if (indt->kind == NX_DTYPE_KIND_CINT && (rdt->kind == NX_DTYPE_KIND_CFLOAT || rdt->kind == NX_DTYPE_KIND_CCOMPLEX)) {
            /* Converting requires a fixed point scale */
            scale_fac = 1.0 / ((1ULL << (8 * indt->size)) - 1);
        }

        /* Scale input */   
        if (!nx_mul(
            res->ar,
            (nxar_t) {
                frame->data[0],
                indt,
                3,
                (ks_size_t[]){ h, w, in_chn },
                (ks_size_t[]){ frame->linesize[0], in_chn_stride * indt->size, indt->size }
            },
            (nxar_t) {
                (void*)&scale_fac,
                nxd_float,
                0,
                NULL,
                NULL
            }
        )) {
            KS_DECREF(res);
            res = NULL;
            return NULL;
        }

    } else {
        /* Weird format, so scale in software */

        // first, try to convert to 8-bit greyscale
        /* Convert to grayscale (or RGBA) */
        enum AVPixelFormat new_fmt = AV_PIX_FMT_GRAY8;

        /* Convert to RGB if we would lose chroma */
        if (av_get_pix_fmt_loss(new_fmt, pix_fmt, true) & FF_LOSS_CHROMA) {
            new_fmt = AV_PIX_FMT_RGB24;
        }

        /* Ensure alpha is included if the format would lose it */
        if (av_get_pix_fmt_loss(new_fmt, pix_fmt, true) & FF_LOSS_ALPHA) {
            new_fmt = AV_PIX_FMT_RGBA;
        }

        /**/ if (new_fmt == AV_PIX_FMT_GRAY8)   { in_chn = 1; in_chn_stride = 1; } 
        else if (new_fmt == AV_PIX_FMT_RGB24)   { in_chn = 3; in_chn_stride = 3; } 
        else if (new_fmt == AV_PIX_FMT_RGBA)    { in_chn = 4; in_chn_stride = 4; }

        if (out_chn <= 0) out_chn = in_chn;

        /* Input is bytes */
        indt = nxd_uchar;

        assert(in_chn > 0 && in_chn_stride > 0 && out_chn >= 0 && "layout was not calculated!");

        static struct SwsContext *sws_context = NULL;

        /* Get the conversion context */
        sws_context = sws_getCachedContext(sws_context,
            w, h, pix_fmt,
            w, h, new_fmt,
            0, NULL, NULL, NULL
        );

        /* Compute line size and temporary data */
        ks_size_t linesize = in_chn_stride * w;
        linesize += (16 - linesize) % 16;

        char* tmp_data = ks_malloc(linesize * h);


        /* Convert data into 'tmp_data' */
        sws_scale(sws_context, (const uint8_t* const*)frame->data, frame->linesize, 0, h, (uint8_t* const[]){ tmp_data, NULL, NULL, NULL }, (int[]){ linesize, 0, 0, 0 });

        /* Construct result */
        res = nx_array_newc(nxt_array, rdt, 3, (ks_size_t[]){ h, w, out_chn }, NULL, NULL);


        /* Scaling factor */
        float scale_fac = 1.0;
        if (indt->kind == NX_DTYPE_KIND_CINT && (rdt->kind == NX_DTYPE_KIND_CFLOAT || rdt->kind == NX_DTYPE_KIND_CCOMPLEX)) {
            /* Converting requires a fixed point scale */
            scale_fac = 1.0 / ((1ULL << (8 * indt->size)) - 1);
        }

        /* Scale input */   
        if (!nx_mul(
            res->ar,
            (nxar_t) {
                tmp_data,
                indt,
                3,
                (ks_size_t[]){ h, w, in_chn },
                (ks_size_t[]){ frame->linesize[0], in_chn_stride * indt->size, indt->size }
            },
            (nxar_t) {
                (void*)&scale_fac,
                nxd_float,
                0,
                NULL,
                NULL
            }
        )) {
            KS_DECREF(res);
            res = NULL;
            return NULL;
        }
    }

    /* Free resources */
    avcodec_free_context(&codctx);
    av_frame_free(&frame);
    av_packet_free(&packet);

    return res;
#else
    KS_THROW(kst_Error, "Cannot read image from stream, no libav");
    return NULL;
#endif
}


bool ksmm_write_image(ksmm_Stream self, nxar_t img) {
#ifdef KS_HAVE_libav

    /* useful: 
     *   https://lists.libav.org/pipermail/libav-user/2009-April/002763.html
     *   https://libav.org/documentation/doxygen/master/encode__video_8c_source.html
     */

    /* Check and make sure the image is well formatted */
    if (img.rank != 3) {
        KS_THROW(kst_Error, "Only rank-3 tensors can be converted to images right now: (h, w, d)");
        return false;
    }

    /* libav structures */

    /* Height, width, and depth of image */
    int h = img.dims[0], w = img.dims[1], d = img.dims[2];

    /* Data buffers */
    AVFrame* frame = NULL;
    AVPacket* packet = NULL;

    /* Allocate data buffers */
    if (!(frame = av_frame_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_frame_alloc()'");
        return NULL;
    }
    if (!(packet = av_packet_alloc())) {
        KS_THROW(kst_Error, "Failed 'av_packet_alloc()'");
        av_frame_free(&frame);
        return NULL;
    }


    /* Requested pixel format */
    // TODO: allow others as well
    enum AVPixelFormat req_pix_fmt = AV_PIX_FMT_RGBA;

    /* Magic word to output */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    /* Raw output */
    FILE* fp = NULL;

    int avst;

    // open encoder
	/*
    if (avst = avcodec_open2(codec_ctx, codec, NULL)) {
        ks_throw(ks_type_InternalError, "`avcodec_open()` failed (code: %i, reason: %s)", avst, av_err2str(avst));
        goto end_write_image;
    }
    */

    /* Set timestamp information */
	frame->pts = 1;
	frame->quality = self->stream->codec->global_quality;
    
    // copy to frame
    frame->format = self->stream->codec->pix_fmt;
    frame->width = w;
    frame->height = h;

    if ((avst = av_frame_get_buffer(frame, 64)) < 0) {
        KS_THROW(kst_Error, "Failed to 'avcodec_frame_get_buffer()': %s [%i]", av_err2str(avst), avst);
        return NULL;
    }

    if ((avst = av_frame_make_writable(frame)) < 0) {
        KS_THROW(kst_Error, "Failed to 'avcodec_frame_get_buffer()': %s [%i]", av_err2str(avst), avst);
        return NULL;
    }

    enum AVPixelFormat frame_pix_fmt = frame->format;

    /* Outpu channels */
    int frame_chn = -1, frame_chn_stride = -1;
    int ar_chn = d;

    nx_dtype frame_dtype = NULL;

    // whether or not to add alpha component
    bool frame_has_alpha = false;

    // write to 'frame'
    if (frame_pix_fmt == AV_PIX_FMT_RGBA || frame_pix_fmt == AV_PIX_FMT_RGB24 || frame_pix_fmt == AV_PIX_FMT_RGB0) {
        // we can do it without using swscale

        /**/ if (false) {}        
        else if (frame_pix_fmt == AV_PIX_FMT_RGBA)  { frame_chn = 4; frame_chn_stride = 4; frame_has_alpha = true; }
        else if (frame_pix_fmt == AV_PIX_FMT_RGB24) { frame_chn = 3; frame_chn_stride = 3; }
        else if (frame_pix_fmt == AV_PIX_FMT_RGB0)  { frame_chn = 3; frame_chn_stride = 4; }

        // all these formats are uint8
        frame_dtype = nxd_uchar;


        assert (frame_chn > 0 && frame_chn_stride > 0 && frame_dtype && "layout was not computed correctly!");

        // create a nxar describing the frame data
        nxar_t frame_ar = (nxar_t){
            .data = (void*)frame->data[0],
            .dtype = frame_dtype,
            .rank = 3,
            .dim = (nx_size_t[]){ h, w, frame_chn },
            .stride = (nx_size_t[]){ frame->linesize[0], frame_chn_stride * frame_dtype->size, frame_dtype->size },
        };

        // scaling factor;
        // for converting from float->int, since
        //   integers are fixed point
        double scale_fac = 1.0;

        if (frame_ar.dtype->kind == NX_DTYPE_KIND_CINT && (img.dtype->kind == NX_DTYPE_KIND_CFLOAT || img.dtype->kind == NX_DTYPE_KIND_CCOMPLEX)) {
            scale_fac = (1ULL << (frame_ar.dtype->size * 8)) - 1;
        }

        // minimum channels
        int min_chn = frame_chn < d ? frame_chn : d;


        // set all shared channels at once
        if (!nx_T_mul(
            (nxar_t) {
                .data = img.data,
                .dtype = img.dtype,
                .rank = 3,
                .dim = (nx_size_t[]){ h, w, min_chn },
                .stride = (nx_size_t[]){ img.stride[0], img.stride[1], img.stride[2] },
            },
            (nxar_t) {
                .data = (void*)&scale_fac,
                .dtype = nx_dtype_fp64,
                .rank = 1,
                .dim = (nx_size_t[]){ 1 },
                .stride = (nx_size_t[]){ 0 },
            },
            (nxar_t) {
                .data = frame_ar.data,
                .dtype = frame_ar.dtype,
                .rank = 3,
                .dim = (nx_size_t[]){ h, w, min_chn },
                .stride = (nx_size_t[]) { frame_ar.stride[0], frame_ar.stride[1], frame_ar.stride[2] },
            }
        )) {
            goto end_write_image;
        }

        //printf("%i,%i\n", min_chn, frame_chn);
        // convert the rest over here
        while (min_chn < frame_chn) {

            // calculate fill value
            uint8_t vu8 = (min_chn == (frame_chn - 1) && frame_has_alpha) ? 255 : 0;

            // fill value up
            if (!nx_T_cast(
                (nxar_t) {
                    .data = (void*)&vu8,
                    .dtype = nx_dtype_uint8,
                    .rank = 1,
                    .dim = (nx_size_t[]){ 1 },
                    .stride = (nx_size_t[]){ 0 },
                },
                (nxar_t) {
                    .data = (void*)((intptr_t)frame_ar.data + frame_ar.stride[2] * min_chn),
                    .dtype = frame_ar.dtype,
                    .rank = 2,
                    .dim = (nx_size_t[]){ h, w },
                    .stride = (nx_size_t[]) { frame_ar.stride[0], frame_ar.stride[1] },
                }
            )) {
                goto end_write_image;
            }


            min_chn++;
        }

        frame->pts = 1;

        /* encode the image */
        if (!my_encode(codec_ctx, frame, packet, fp)) goto end_write_image;
        if (!my_encode(codec_ctx, NULL, packet, fp)) goto end_write_image;

        fwrite(endcode, 1, sizeof(endcode), fp);

    } else {
        // we need to convert what we have to it
        ks_throw(ks_type_ToDoError, "Need to implement swscale for other formats");

        goto end_write_image;
    }



    // cleanup
    if (codec_ctx) avcodec_close(codec_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet);
    if (fp) fclose(fp);

    return rst;

#else
    KS_THROW(kst_Error, "Cannot write image to stream, no libav");
    return NULL;
#endif
}





/* Type Functions */

static KS_TFUNC(T, free) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);

    KS_NDECREF(self->src);
    KS_NDECREF(self->fmtctx);

    KSO_DEL(self);
    return KSO_NONE;
}

static KS_TFUNC(T, str) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);
#ifdef KS_HAVE_libav
    return (kso)ks_fmt("<%T (src=%R, idx=%i, kind=%s)>", self, self->src, self->idx, self->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ? "AUDIO" : "VIDEO");
#else
    return (kso)ks_fmt("%O", self);
#endif
}

static KS_TFUNC(T, readimg) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);
    
    return (kso)ksmm_get_image(self);
}

static KS_TFUNC(T, isaudio) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);

#ifdef KS_HAVE_libav
    return KSO_BOOL(self->stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO);
#else
    return KSO_FALSE;
#endif
}

static KS_TFUNC(T, isvideo) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);

#ifdef KS_HAVE_libav
    return KSO_BOOL(self->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO);
#else
    return KSO_FALSE;
#endif
}

static KS_TFUNC(T, next) {
    ksmm_Stream self;
    KS_ARGS("self:*", &self, ksmmt_Stream);

#ifdef KS_HAVE_libav
    if (self->stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        return (kso)ksmm_get_image(self);
    } else {
        return (kso)ksmm_get_image(self);
    }

#else
    KS_OUTOFITER();
    return NULL;
#endif
}


/* Export */

static struct ks_type_s tp;
ks_type ksmmt_Stream = &tp;

void _ksi_mm_Stream() {
    _ksinit(ksmmt_Stream, kst_object, T_NAME, sizeof(struct ksmm_Stream_s), -1, "", KS_IKV(
        {"__free",                 ksf_wrap(T_free_, T_NAME ".__free(self)", "")},

        {"__repr",                 ksf_wrap(T_str_, T_NAME ".__repr(self)", "")},
        {"__str",                  ksf_wrap(T_str_, T_NAME ".__str(self)", "")},

        {"__next",                 ksf_wrap(T_next_, T_NAME ".__next(self)", "")},

        {"isaudio",                ksf_wrap(T_isaudio_, T_NAME ".isaudio(self)", "Returns whether the stream is an audio stream")},
        {"isvideo",                ksf_wrap(T_isvideo_, T_NAME ".isvideo(self)", "Returns whether the stream is a video stream")},

        {"readimg",                ksf_wrap(T_readimg_, T_NAME ".readimg(self)", "Reads an image")},
    ));
}

