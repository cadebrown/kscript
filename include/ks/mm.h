/* ks/mm.h - header file for the kscript math builtin module `import mm'
 *
 * 
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSMM_H__
#define KSMM_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif /* KS_H__ */


/* We use the NumeriX library */
#include <ks/nx.h>


/* libav (--with-libav) 
 *
 * Adds support for different multimedia formats
 *
 */
#ifdef KS_HAVE_libav
  #include <libavutil/opt.h>
  #include <libavutil/imgutils.h>
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>

  #include <libswscale/swscale.h>
#else

#endif



/* Types */

#ifdef KS_HAVE_libav

/* mm._AVFormatContext - internal wrapper for the AVFormatContext* in C
 *
 */
typedef struct ksmm_AVFormatContext_s {
    KSO_BASE

    /* Wrapped value */
    AVFormatContext* val;

}* ksmm_AVFormatContext;

#endif

/* mm.MediaFile - represents a media item with multiple possible streams
 *
 */
typedef struct ksmm_MediaFile_s {
    KSO_BASE

    /* source of stream */
    ks_str src;

    /* List of 'mm.Stream' objects representing the individual streams */
    ks_list streams;

#ifdef KS_HAVE_libav
    /* Handle to the current context and format being used for this media file */
    ksmm_AVFormatContext fmtctx;
#endif


}* ksmm_MediaFile;


/* mm.Stream - single audio/video stream
 *
 */
typedef struct ksmm_Stream_s {
    KSO_BASE

    /* source of stream */
    ks_str src;

    /* Index of stream */
    int idx;

    /* Whether or not the stream has been opened */
    bool is_open;

#ifdef KS_HAVE_libav

    /* Format context describing the stream */
    ksmm_AVFormatContext fmtctx;

    /* libav stream */
    AVStream* stream;

#endif

}* ksmm_Stream;




/* Functions */


#ifdef KS_HAVE_libav

/* Wrap an AVFormatContext
 */
KS_API ksmm_AVFormatContext ksmm_AVFormatContext_wrap(AVFormatContext* val);

/* Custom callback negotiator for 'codctx->get_format' to pick our preferred formats
 */
KS_API enum AVPixelFormat ksmm_AV_getformat(struct AVCodecContext* codctx, const enum AVPixelFormat* fmt);

/* Filtering bad pixel formats and returning a better one (retains memory layout)
 * SEE: https://stackoverflow.com/questions/23067722/swscaler-warning-deprecated-pixel-format-used
 */
KS_API enum AVPixelFormat ksmm_AV_filterfmt(enum AVPixelFormat pix_fmt);



#endif


/* Open a media file
 */
KS_API ksmm_MediaFile ksmm_MediaFile_open(ks_type tp, ks_str src);


/* Read an image from a stream
 */
KS_API nx_array ksmm_get_image(ksmm_Stream self);


/* Export */

KS_API extern ks_type
    ksmmt_AVFormatContext,
    ksmmt_MediaFile,
    ksmmt_Stream
;


#endif /* KSMM_H__ */
