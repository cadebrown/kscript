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




/* mm.Stream - audio/video stream that can be opened by any URL
 *
 */
typedef struct ksmm_Stream_s {
    KSO_BASE

#ifdef KS_HAVE_libav

    /* Handle to the current context and format being used */
    AVFormatContext* fmtctx;

#endif

    /* Number of substreams */
    int nsub;

    /* Subtream, for a specific media type
     *
     */
    struct ksmm_SubStream_s {

        /* Whether the stream's codec is currently opened */
        bool is_open;

#ifdef KS_HAVE_LIBAV

        /* The specific stream */
        AVStream* stream;

        /* Codec context for decoding/encoding stream values */
        AVCodecContext* codec_ctx;

#endif

    }* sub;

}* ksmm_Stream;


/* Functions */

/* Open a stream from a given resource URL
 */
KS_API ksmm_Stream ksmm_Stream_open(ks_type tp, ks_str url);



#endif /* KSMM_H__ */
