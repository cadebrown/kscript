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
//#warning Building kscript without libav support, so most media formats are not supported
#endif

#ifndef KS_HAVE_libav

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "./stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./stb_image_write.h"

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

#else

/* STB callbacks */


/* Data for STB callback */
struct my_stbi_data_s {
    /* Output stream */
    ksio_BaseIO out;

    /* Whether it had an exception */
    bool exc;

};


/* Read callback for 'stb_image.h' */
int my_stbi_read(void* context, char* data, int size) {
    struct my_stbi_data_s* d = context;
    if (d->exc) return -1;
    ks_ssize_t rsz = ksio_readb(d->out, size, data);
    if (rsz < 0) {
        d->exc = true;
        return -1;
    }

    return rsz;
}

/* Skip callback for 'stb_image.h' */
void my_stbi_skip(void* context, int size) {
    struct my_stbi_data_s* d = context;
    if (d->exc) return;

    if (!ksio_seek(d->out, size, KSIO_SEEK_CUR)) {
        d->exc = true;
    }
}

/* EOF callback for 'stb_image.h' */
int my_stbi_eof(void* context) {
    struct my_stbi_data_s* d = context;
    if (d->exc) return -1;

    bool g;
    if (!ksio_eof(d->out, &g)) {
        d->exc = true;
        return -1;
    }

    return g;
}


/* Write callback for 'stb_image_write.h' */
void my_stbi_write(void* context, void* data, int size) {
    struct my_stbi_data_s* d = context;
    if (!d->exc && !ksio_writeb(d->out, size, data)) {
        d->exc = true;
    }

}
#endif


kso ksav_imread(kso src) {
#ifdef KS_HAVE_libav
    ks_str ss = ks_fmt("%S", src);
    if (!ss) return NULL;
    ksav_IO io = ksav_open(ksavt_IO, kso_issub(src->type, kst_str) ? KSO_NONE : src, ss, _ksv_r);
    KS_DECREF(ss);
    if (!io) return NULL;

    /* Find video stream */
    int vs = ksav_bestvideo(io);
    if (vs < 0) {
        KS_DECREF(io);
        return NULL;
    }

    /* Read first frame */
    int rsidx;
    kso res = ksav_next(io, &rsidx, 1, &vs);
    KS_DECREF(io);
    if (rsidx < 0) {
        return NULL;
    }

    assert(rsidx == vs);

    return res;
#else
    stbi_io_callbacks stbio;
    stbio.read = my_stbi_read;
    stbio.skip = my_stbi_skip;
    stbio.eof = my_stbi_eof;

    struct my_stbi_data_s sbd;
    sbd.exc = false;
    if (kso_issub(src->type, kst_str)) {
        sbd.out = kso_call((kso)ksiot_FileIO, 2, (kso[]){ (kso)src, (kso)_ksv_rb });        
    } else {
        KS_INCREF(src);
        sbd.out = src;
    }

    int x, y, c;
    unsigned char* d = stbi_load_from_callbacks(&stbio, &sbd, &x, &y, &c, 0);
    KS_DECREF(sbd.out);
    if (sbd.exc) {
        STBI_FREE(d);
        return NULL;
    }
    if (!d) {
        KS_THROW(kst_Error, "Failed to read image from %R: %s", src, stbi_failure_reason());
        return NULL;
    }

    nx_array rr = nx_array_newc(nxt_array, nxd_uchar, 3, (ks_size_t[]){ y, x, c }, (ks_ssize_t[]){ x * c, c, 1 }, d);
    STBI_FREE(d);
    return (kso)rr;

#endif
}

bool ksav_imwrite(kso src, nxar_t data, kso fmt) {

    if (!(data.rank == 2 || data.rank == 3)) {
        KS_THROW(kst_Error, "Invalid dimensions, images must have rank 2 or 3, but it was of rank %i", data.rank);
        return false;
    }

    /* TODO: use lowercase */
    ks_str sf = NULL;
    if (fmt == KSO_NONE) {
        if (kso_issub(src->type, kst_str)) {
            ks_str ss = (ks_str)src;
            int p = ss->len_b;
            while (p > 0) {
                if (ss->data[p - 1] == '.') break;
                p--;
            }
            sf = ks_str_new(ss->len_b - p, ss->data + p);
        } else {
            KS_THROW(kst_TypeError, "Failed to determine format for output: %R", src);
            return false;
        }
    } else if (kso_issub(fmt->type, kst_str)) {
        KS_INCREF(fmt);
        sf = (ks_str)fmt;
    } else {
        KS_THROW(kst_TypeError, "Expected 'fmt' to be a 'str' or 'none', but got '%T' object", fmt);
        return NULL;
    }

#ifdef KS_HAVE_libav
    KS_THROW(kst_Error, "TODO: fix libav imwrite");
    KS_DECREF(sf);
    return false;
#else
    struct my_stbi_data_s ds;
    ds.exc = false;
    if (kso_issub(src->type, kst_str)) {
        ksio_FileIO fio = (ksio_FileIO)kso_call((kso)ksiot_FileIO, 2, (kso[]){ (kso)src, (kso)_ksv_wb });
        if (!fio) {
            KS_DECREF(sf);
            return NULL;
        }
        ds.out = (kso)fio;
    } else {
        KS_INCREF(src);
        ds.out = src;
    }

    int h = data.dims[0], w = data.dims[1], d = data.rank == 3 ? data.dims[2] : 1;
    nxc_uchar* dp = ks_zmalloc(sizeof(*dp), h * w * d);
    if (!nx_fpcast(
        (nxar_t) {
            dp,
            nxd_uchar,
            3,
            (ks_size_t[]){ h, w, d },
            (ks_ssize_t[]){ w * d, d, 1 }
        },
        data
    )) {
        ks_free(dp);
        KS_DECREF(sf);
        KS_DECREF(ds.out);
        return NULL;
    }

    int rc = 0;
    if (ks_str_eq_c(sf, "jpg", 3) || ks_str_eq_c(sf, "jpeg", 4)) {
        rc = stbi_write_jpg_to_func(my_stbi_write, &ds, w, h, d, dp, 90 /* Quality */);
    } else if (ks_str_eq_c(sf, "png", 3)) {
        rc = stbi_write_png_to_func(my_stbi_write, &ds, w, h, d, dp, w * d);
    } else if (ks_str_eq_c(sf, "tga", 3)) {
        rc = stbi_write_tga_to_func(my_stbi_write, &ds, w, h, d, dp);
    } else if (ks_str_eq_c(sf, "bmp", 3)) {
        rc = stbi_write_bmp_to_func(my_stbi_write, &ds, w, h, d, dp);
    } else {
        KS_THROW(kst_Error, "Unknown output format: %R", sf);
        ks_free(dp);
        KS_DECREF(sf);
        KS_DECREF(ds.out);
        return NULL;
    }
    KS_DECREF(sf);
    KS_DECREF(ds.out);
    ks_free(dp);
    return !ds.exc;
#endif
}

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

    ksav_IO res = ksav_open(ksavt_IO, KSO_NONE, sf, mode);
    KS_DECREF(sf);
    if (!res) return NULL;

    return (kso)res;
}

static KS_TFUNC(M, imread) {
    kso src;
    KS_ARGS("src", &src);

    return (kso)ksav_imread(src);
}

static KS_TFUNC(M, imwrite) {
    kso src, data, fmt = KSO_NONE;
    KS_ARGS("src data ?fmt", &src, &data, &fmt);

    nxar_t ar;
    kso rr;
    if (!nxar_get(data, NULL, &ar, &rr)) {
        return NULL;
    }

    if (!ksav_imwrite(src, ar, fmt)) {
        KS_NDECREF(rr);
        return NULL;
    }

    KS_NDECREF(rr);
    return KSO_NONE;
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

        
        {"imread",                 ksf_wrap(M_imread_, M_NAME ".imread(src)", "Reads an image from a media file")},
        {"imwrite",                ksf_wrap(M_imwrite_, M_NAME ".imwrite(src, data, fmt=none)", "Writes an image to a media file")},

    ));

    return res;
}

/* Loader function */
KS_CEXT_DECL(get);

