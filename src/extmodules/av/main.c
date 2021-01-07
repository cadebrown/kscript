/* main.c - source code for the built-in 'av' module
 *
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/av.h>
#include <ks/cext.h>

#define M_NAME "mm"

#ifndef KS_HAVE_libav
#warning Building kscript without libav support, so most media formats are not supported
#endif

/* Utils */

/* C-API */

#ifdef KS_HAVE_libav

enum AVPixelFormat ksav_AV_getformat(struct AVCodecContext* codctx, const enum AVPixelFormat* fmt) {
    /* List of formats we can handle */
    static const enum AVPixelFormat best_formats[] = {
        AV_PIX_FMT_RGBA,
        AV_PIX_FMT_RGB24,
        AV_PIX_FMT_RGB0,
        -1,
    };
    
    /* Current supported iterator
     *
     */
    const enum AVPixelFormat* it = &fmt[0];

    while (*it > 0) {
        const enum AVPixelFormat* target = &best_formats[0];

        /* Now, see if the supported formats matches one we can process */
        while (*target > 0) {
            if (*it == *target) return *it;
            target++;
        }

        it++;
    }

    /*  Return default format otherwise*/
    return avcodec_default_get_format(codctx, fmt);
}

enum AVPixelFormat ksav_AV_filterfmt(enum AVPixelFormat pix_fmt) {

    #define _PFC(_old, _new) else if (pix_fmt == _old) return _new;

    if (false) {}

    /* YUV-JPEG formats are the same as normal YUV, but for some reason codecs don't like them at all */
    _PFC(AV_PIX_FMT_YUVJ411P, AV_PIX_FMT_YUV411P)
    _PFC(AV_PIX_FMT_YUVJ420P, AV_PIX_FMT_YUV420P)
    _PFC(AV_PIX_FMT_YUVJ422P, AV_PIX_FMT_YUV422P)
    _PFC(AV_PIX_FMT_YUVJ444P, AV_PIX_FMT_YUV444P)
    _PFC(AV_PIX_FMT_YUVJ440P, AV_PIX_FMT_YUV440P)

    #undef _PFC

    // otherwise, return what was given, as it's probably just fine
    /* Otherwise, default to it */
    return pix_fmt;
}

#endif




/* Module Functions */

static KS_TFUNC(M, open) {
    kso fname;
    ks_str mode = _ksv_r;
    KS_ARGS("fname ?mode:*", &fname, &mode, kst_str);

    ks_str sf = NULL;
    if (kso_issub(fname->type, kst_str)) {
        KS_INCREF(fname);
        sf = (ks_str)fname;
    } else if (kso_issub(fname->type, ksost_path)) {
        sf = ks_fmt("%S", fname);
    } else {
        KS_THROW(kst_TypeError, "Expected 'fname' to be either 'str' or 'os.path', but got '%T' object", fname);
        return NULL;
    }

    if (!sf) return NULL;

    ksav_IO res = ksav_open(ksavt_IO, sf, mode);
    KS_DECREF(sf);
    if (!res) return NULL;

    return (kso)res;
}


/* Export */

static ks_module get() {
    _ksi_av_IO();
    _ksi_av_Stream();

    ks_str nxk = ks_str_new(-1, "nx");
    if (!ks_import(nxk)) {
        return NULL;
    }
    KS_DECREF(nxk);

    ks_module res = ks_module_new(M_NAME, KS_BIMOD_SRC, "'av' - audio-video module\n\n    This module implements common media operations", KS_IKV(
        /* Types */
        {"IO",                     (kso)ksavt_IO},
        {"Stream",                 (kso)ksavt_Stream},

        /* Functions */
        {"open",                   ksf_wrap(M_open_, M_NAME ".open(fname, mode='r')", "Opens a media file")},

    ));

    return res;
}

/* Loader function */
KS_CEXT_DECL(get);

